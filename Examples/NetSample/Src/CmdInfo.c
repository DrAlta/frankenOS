/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: CmdInfo.c
 *
 * Description:
 * This module provides various commands to get info about the Net Library
 * like which interfaces are attached, protocol statistics, etc. 
 *
 *****************************************************************************/

// Include the PalmOS headers
#include <PalmOS.h>

// Socket Equates
#include <sys_socket.h>

// StdIO header
#include "AppStdIO.h"

// Application headers
#include "NetSample.h"


// Utility to get a UInt32 out of a charPtr which might be odd aligned
static UInt32	PrvDWord(Char * p)
{
	UInt32	result;
	
	result = 0;
	result = *p++;
	result = (result << 8) + *p++;
	result = (result << 8) + *p++;
	result = (result << 8) + *p++;
	
	return result;
}



/***********************************************************************
 *
 * FUNCTION:    PrvHToI
 *
 * DESCRIPTION: This routine converts a hex string to an integer. 
 *
 * PARAMETERS:  str    - string to convert
 *
 * RETURNED:    converted integer
 *
 ***********************************************************************/
static UInt32 PrvHToI (const Char* str)
{
	UInt32	result=0;
	Char		c;
	Boolean	valid;
	
	
	// First character can be a sign
	c = *str++;
	if (!c) return 0;
	
	// Accumulate digits will we reach the end of the string
	while(c) {
		valid = false;
		if (c >= '0' && c <= '9') 
			result = result*16 + c - '0';
		else if (c >= 'a' && c <= 'f') 	
			result = result*16 + c - 'a' + 10;
		else if (c >= 'A' && c <= 'F') 
			result = result*16 + c - 'A' + 10;
		else 
			break;

		c = *str++;
		}
		

	return result;
}


/***********************************************************************
 *
 * FUNCTION:   CmdDivider()
 *
 * DESCRIPTION: This command exists solely to print out a dividing line
 *		when the user does a help all.
 *
 *	CALLED BY:	 
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdDivider(int argc, Char * argv[])
{
	// Check for short help
	if (argc > 1 && !StrCompare(argv[1], "?")) 
		printf("\n-- Info Cmds --\n");
}


/***********************************************************************
 *
 * FUNCTION:   CmdIFLog()
 *
 * DESCRIPTION: Displays connect log for an interface
 *
 *	CALLED BY:	 AppProcessCommand in response to the "iflog" command
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY: BLT	11/18/99		Added call to MemPtrFree()
 *
 ***********************************************************************/
static void CmdIFLog(int argc, Char * argv[])
{
	Err							err=0;
	Char *						logP = 0;
	UInt32						ifCreator=0;
	UInt16						size=0;
	UInt16						index;
	UInt16						ifInstance=0;
	
	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	// Get the creator, if none specified, get the currently attached interface
	if (argc > 1) 
		ifCreator = PrvDWord(argv[1]);
	
	
	if (!ifCreator) {
		for (index = 0; 1; index++) {
			err = NetLibIFGet(AppNetRefnum, index, &ifCreator, &ifInstance);
			if (err) goto Exit;

			if (ifCreator != netIFCreatorLoop) break;
			}
		}	
	if (err) goto Exit;
		

	// Get the log size.
	err = NetLibIFSettingGet(AppNetRefnum,ifCreator, 0,
			netIFSettingConnectLog, 0, &size);
			
	// If there is one, allocate space for it and read it in
	if (size) {
		logP = MemPtrNew(size+1);
		if (logP) {
			err = NetLibIFSettingGet(AppNetRefnum,ifCreator, ifInstance,
				netIFSettingConnectLog, logP, &size);
			if (err) goto Exit;
			
			// 0 terminate it
			logP[size] = 0;
			}
		}
		
	// If we got a log, print it out 
	if (logP) {
		StdPutS(logP);
		MemPtrFree(logP);
	}
		

Exit:
	if (err) printf("\nError: %s", appErrString(err));
	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t\t# Interface connect log\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDisplay interface connect log");
	printf("\nSyntax: %s [<ifCreator>]", argv[0]);
	printf("\n");
}



/***********************************************************************
 *
 * FUNCTION:   CmdIFInfo()
 *
 * DESCRIPTION: Displays information on all attached interfaces
 *
 *	CALLED BY:	AppProcessCommand in response to the "ifinfo" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdIFInfo(int argc, Char * argv[])
{
	Err							err;
	NetMasterPBType			pb;
	char							str[8];
	UInt16						column = 15;
	Char							addrStr[32];

	
	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	// Process command
	for (pb.param.interfaceInfo.index = 0; 1; pb.param.interfaceInfo.index++) {
		if (pb.param.interfaceInfo.index > 0) StdPutS("\n\n");
		printf("==== Interface #%d",  pb.param.interfaceInfo.index);
		printf("...");
		
		// Get the info
		err = NetLibMaster(AppNetRefnum, netMasterInterfaceInfo, &pb, 
						sysTicksPerSecond*2);
		if (err == netErrParamErr) {
			printf(" Not present\n");			// no more interfaces....
			break;
			}
		
		// Print results
		if (err) {
			printf("\nErr: %s\n", appErrString(err));
			break;
			}
		else {
			*((UInt32 *)str) = pb.param.interfaceInfo.creator;
			str[4] = 0;
			printf("\ncreator: \t\t\t%s", str);  
			printf("\ninstance: \t\t\t%d", pb.param.interfaceInfo.instance);

			printf("\ndrvrName: \t\t%s", pb.param.interfaceInfo.drvrName);
			printf("\nhwName: \t\t\t%s", pb.param.interfaceInfo.hwName);
			printf("\nlnh: \t\t\t\t%d", pb.param.interfaceInfo.localNetHdrLen);
			printf("\nlnt: \t\t\t\t%d", pb.param.interfaceInfo.localNetTrailerLen);
			printf("\nmaxFrame: \t\t%d", pb.param.interfaceInfo.localNetMaxFrame);
			
			printf("\nifName: \t\t\t%s", pb.param.interfaceInfo.ifName);
			printf("\ndriverUp: \t\t%d", pb.param.interfaceInfo.driverUp);
			printf("\nifUp: \t\t\t%d",  pb.param.interfaceInfo.ifUp);
			printf("\nhwAddrLen: \t\t%d", pb.param.interfaceInfo.hwAddrLen);
			printf("\nmtu: \t\t\t%d", pb.param.interfaceInfo.mtu);
			printf("\nspeed: \t\t\t%ld", pb.param.interfaceInfo.speed);
			printf("\nlastStateChange: \t%ld",  pb.param.interfaceInfo.lastStateChange);
			
			printf("\nipAddr: \t\t\t%s", NetLibAddrINToA(AppNetRefnum, pb.param.interfaceInfo.ipAddr, addrStr));
			printf("\nsubnetMask: \t\t%s", NetLibAddrINToA(AppNetRefnum, pb.param.interfaceInfo.subnetMask, addrStr));
			printf("\nbroadcast: \t\t%s", NetLibAddrINToA(AppNetRefnum, pb.param.interfaceInfo.broadcast, addrStr));
			}
		}
	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# Interface info\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDisplay interface info");
	printf("\nSyntax: %s ", argv[0]);
	printf("\n");
}


/***********************************************************************
 *
 * FUNCTION:   CmdIFStats()
 *
 * DESCRIPTION: Displays stats on all attached interfaces
 *
 *	CALLED BY:	AppProcessCommand in response to the "ifstats" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdIFStats(int argc, Char * argv[])
{
	Err							err;
	NetMasterPBType			pb;
	UInt16						column = 22;
	Char							name[32];

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	for (pb.param.interfaceStats.index = 0; 1; pb.param.interfaceStats.index++) {
		if (pb.param.interfaceStats.index > 0) StdPutS("\n\n");
		printf("==== Interface #%d", pb.param.interfaceStats.index);
		
		// Get the name of this interface
		err = NetLibMaster(AppNetRefnum, netMasterInterfaceInfo, &pb, 
						sysTicksPerSecond*2);
		if (err) {
			printf(" Not present\n");			// no more interfaces....
			break;
			}
		StrCopy(name, pb.param.interfaceInfo.ifName);
		printf(" (%s)", name);


		// Get the info
		err = NetLibMaster(AppNetRefnum, netMasterInterfaceStats, &pb, 
						sysTicksPerSecond*2);
		// Print results
		if (err) {
			printf("\nErr: %s\n", appErrString(err));
			break;
			}
		else {
			printf("\ninOctets: \t\t\t%ld", pb.param.interfaceStats.inOctets);
			printf("\ninUcastPkts: \t\t%ld", pb.param.interfaceStats.inUcastPkts);
			printf("\ninNUcastPkts: \t\t%ld", pb.param.interfaceStats.inNUcastPkts);
			printf("\ninDiscards: \t\t%ld", pb.param.interfaceStats.inDiscards);
			printf("\ninErrors: \t\t\t%ld", pb.param.interfaceStats.inErrors);
			printf("\ninUnknownProtos: \t%ld",  pb.param.interfaceStats.inUnknownProtos);
			printf("\noutOctets: \t\t%ld", pb.param.interfaceStats.outOctets);
			printf("\noutUcastPkts: \t\t%ld", pb.param.interfaceStats.outUcastPkts);
			printf("\noutNUcastPkts: \t%ld", pb.param.interfaceStats.outNUcastPkts);
			printf("\noutDiscards: \t\t%ld", pb.param.interfaceStats.outDiscards);
			printf("\noutErrors: \t\t%ld",  pb.param.interfaceStats.outErrors);
			}
		}

	return;
	
ShortHelp:
	printf("%s\t\t# Interface stats\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDisplay interface stats");
	printf("\nSyntax: %s ", argv[0]);
	printf("\n");

}

/***********************************************************************
 *
 * FUNCTION:   CmdIPStats()
 *
 * DESCRIPTION: Displays IP stats
 *
 *	CALLED BY:	AppProcessCommand in response to the "ipstats" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdIPStats(int argc, Char * argv[])
{
	Err							err;
	NetMasterPBType			pb;
	UInt16						column = 22;

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	// Get the info
	err = NetLibMaster(AppNetRefnum, netMasterIPStats, &pb, 
					sysTicksPerSecond*2);

	// Print results
	if (err) {
		StdPutS("\nErr: $\n");
		StdPutS(appErrString(err));
		return;
		}
	else {
		printf("ipInReceives: \t\t\t%ld",  pb.param.ipStats.ipInReceives);
		printf("\nipInHdrErrors: \t\t\t%ld",  pb.param.ipStats.ipInHdrErrors);
		printf("\nipInAddrErrors: \t\t%ld",  pb.param.ipStats.ipInAddrErrors);
		printf("\nipForwDatagrams: \t\t%ld",  pb.param.ipStats.ipForwDatagrams);
		printf("\nipInUnknownProtos: \t%ld",  pb.param.ipStats.ipInUnknownProtos);
		printf("\nipInDiscards: \t\t\t%ld",  pb.param.ipStats.ipInDiscards);
		printf("\nipInDelivers: \t\t\t%ld",  pb.param.ipStats.ipInDelivers);
		printf("\nipOutRequests: \t\t%ld",  pb.param.ipStats.ipOutRequests);
		printf("\nipOutNoRoutes: \t\t%ld",  pb.param.ipStats.ipOutNoRoutes);
		printf("\nipReasmReqds: \t\t%ld",  pb.param.ipStats.ipReasmReqds);
		printf("\nipReasmOKs: \t\t\t%ld",  pb.param.ipStats.ipReasmOKs);
		printf("\nipReasmFails: \t\t\t%ld",  pb.param.ipStats.ipReasmFails);
		printf("\nipFragOKs: \t\t\t%ld",  pb.param.ipStats.ipFragOKs);
		printf("\nipFragFails: \t\t\t%ld",  pb.param.ipStats.ipFragFails);
		printf("\nipFragCreates: \t\t%ld",  pb.param.ipStats.ipFragCreates);
		printf("\nipRoutingDiscards: \t\t%ld",  pb.param.ipStats.ipRoutingDiscards);
		printf("\nipDefaultTTL: \t\t\t%ld",  pb.param.ipStats.ipDefaultTTL);
		printf("\nipReasmTimeout: \t\t%ld",  pb.param.ipStats.ipReasmTimeout);
		}
	printf("\n");		
	return;
	
ShortHelp:
	printf("%s\t\t# IP stats\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDisplay IP stats");
	printf("\nSyntax: %s ", argv[0]);
	printf("\n");

}


/***********************************************************************
 *
 * FUNCTION:   CmdICMPStats()
 *
 * DESCRIPTION: Displays ICMP stats
 *
 *	CALLED BY:	AppProcessCommand in response to the "icmpstats" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdICMPStats(int argc, Char * argv[])
{
	Err							err;
	NetMasterPBType			pb;
	UInt16						column = 22;

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	// Get the info
	err = NetLibMaster(AppNetRefnum, netMasterICMPStats, &pb, 
					sysTicksPerSecond*2);

	// Print results
	if (err) {
		printf("\nErr: %s\n", appErrString(err));
		return;
		}
	else {
		printf("icmpInMsgs: \t\t\t%ld",  pb.param.icmpStats.icmpInMsgs);
		printf("\nicmpInErrors: \t\t\t%ld",  pb.param.icmpStats.icmpInErrors);
		printf("\nicmpInDestUnreachs: \t%ld",  pb.param.icmpStats.icmpInDestUnreachs);
		printf("\nicmpInTimeExcds: \t\t%ld",  pb.param.icmpStats.icmpInTimeExcds);
		printf("\nicmpInParmProbs: \t\t%ld",  pb.param.icmpStats.icmpInParmProbs);
		printf("\nicmpInSrcQuenchs: \t\t%ld",  pb.param.icmpStats.icmpInSrcQuenchs);
		printf("\nicmpInRedirects: \t\t%ld",  pb.param.icmpStats.icmpInRedirects);
		printf("\nicmpInEchos: \t\t\t%ld",  pb.param.icmpStats.icmpInEchos);
		printf("\nicmpInEchoReps: \t\t%ld",  pb.param.icmpStats.icmpInEchoReps);
		printf("\nicmpInTimestamps: \t%ld",  pb.param.icmpStats.icmpInTimestamps);
		printf("\nicmpInTimestampReps: \t%ld",  pb.param.icmpStats.icmpInTimestampReps);
		printf("\nicmpInAddrMasks: \t\t%ld",  pb.param.icmpStats.icmpInAddrMasks);
		printf("\nicmpInAddrMaskReps: \t%ld",  pb.param.icmpStats.icmpInAddrMaskReps);
		printf("\nicmpOutMsgs: \t\t%ld",  pb.param.icmpStats.icmpOutMsgs);
		printf("\nicmpOutErrors: \t\t%ld",  pb.param.icmpStats.icmpOutErrors);
		printf("\nicmpOutDestUnreachs: \t%ld",  pb.param.icmpStats.icmpOutDestUnreachs);
		printf("\nicmpOutTimeExcds: \t%ld",  pb.param.icmpStats.icmpOutTimeExcds);
		printf("\nicmpOutParmProbs: \t%ld",  pb.param.icmpStats.icmpOutParmProbs);
		printf("\nicmpOutSrcQuenchs: \t%ld",  pb.param.icmpStats.icmpOutSrcQuenchs);
		printf("\nicmpOutRedirects: \t\t%ld",  pb.param.icmpStats.icmpOutRedirects);
		printf("\nicmpOutEchos: \t\t%ld",  pb.param.icmpStats.icmpOutEchos);
		printf("\nicmpOutEchoReps: \t\t%ld",  pb.param.icmpStats.icmpOutEchoReps);
		printf("\nicmpOutTimestamps: \t%ld",  pb.param.icmpStats.icmpOutTimestamps);
		printf("\nicmpOutTimestampReps: \t%ld",  pb.param.icmpStats.icmpOutTimestampReps);
		printf("\nicmpOutAddrMasks: \t%ld",  pb.param.icmpStats.icmpOutAddrMasks);
		printf("\nicmpOutAddrMaskReps: \t%ld",  pb.param.icmpStats.icmpOutAddrMaskReps);
		}
	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t# ICMP stats\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDisplay ICMP stats");
	printf("\nSyntax: %s ", argv[0]);
	printf("\n");

}


/***********************************************************************
 *
 * FUNCTION:   CmdUDPStats()
 *
 * DESCRIPTION: Displays UDP stats
 *
 *	CALLED BY:	AppProcessCommand in response to the "udpstats" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdUDPStats(int argc, Char * argv[])
{
	Err							err;
	NetMasterPBType			pb;
	UInt16						column = 22;

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	// Get the info
	err = NetLibMaster(AppNetRefnum, netMasterUDPStats, &pb, 
					sysTicksPerSecond*2);

	// Print results
	if (err) {
		printf("\nErr: %s\n", appErrString(err));
		return;
		}
	else {
		printf("udpInDatagrams: \t\t%ld",  pb.param.udpStats.udpInDatagrams);		
		printf("\nudpNoPorts: \t\t\t%ld",  pb.param.udpStats.udpNoPorts);
		printf("\nudpInErrors: \t\t\t%ld",  pb.param.udpStats.udpInErrors);
		printf("\nudpOutDatagrams: \t%ld",  pb.param.udpStats.udpOutDatagrams);
		}
	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# UDP stats\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDisplay UDP stats");
	printf("\nSyntax: %s ", argv[0]);
	printf("\n");

		
}


/***********************************************************************
 *
 * FUNCTION:   CmdTCPStats()
 *
 * DESCRIPTION: Displays TCP stats
 *
 *	CALLED BY:	AppProcessCommand in response to the "icmpstats" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdTCPStats(int argc, Char * argv[])
{
	Err							err;
	NetMasterPBType			pb;
	UInt16						column = 22;

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	// Get the info
	err = NetLibMaster(AppNetRefnum, netMasterTCPStats, &pb, 
					sysTicksPerSecond*2);

	// Print results
	if (err) {
		printf("\nErr: %s\n", appErrString(err));
		return;
		}
	else {
		printf("tcpRtoAlgorithm: \t%ld",  pb.param.tcpStats.tcpRtoAlgorithm);		
		printf("\ntcpRtoMin: \t\t%ld",  pb.param.tcpStats.tcpRtoMin);
		printf("\ntcpRtoMax: \t\t%ld",  pb.param.tcpStats.tcpRtoMax);
		printf("\ntcpMaxConn: \t\t%ld",  pb.param.tcpStats.tcpMaxConn);
		printf("\ntcpActiveOpens: \t%ld",  pb.param.tcpStats.tcpActiveOpens);
		printf("\ntcpPassiveOpens: \t%ld",  pb.param.tcpStats.tcpPassiveOpens);
		printf("\ntcpAttemptFails: \t%ld",  pb.param.tcpStats.tcpAttemptFails);
		printf("\ntcpEstabResets: \t%ld",  pb.param.tcpStats.tcpEstabResets);
		printf("\ntcpCurrEstab: \t\t%ld",  pb.param.tcpStats.tcpCurrEstab);
		printf("\ntcpInSegs: \t\t%ld",  pb.param.tcpStats.tcpInSegs);
		printf("\ntcpOutSegs: \t\t%ld",  pb.param.tcpStats.tcpOutSegs);
		printf("\ntcpRetransSegs: \t%ld",  pb.param.tcpStats.tcpRetransSegs);
		printf("\ntcpInErrs: \t\t%ld",  pb.param.tcpStats.tcpInErrs);
		printf("\ntcpOutRsts: \t\t%ld",  pb.param.tcpStats.tcpOutRsts);
		}
	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# TCP stats\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDisplay TCP stats");
	printf("\nSyntax: %s ", argv[0]);
	printf("\n");

		
}




/***********************************************************************
 *
 * FUNCTION:   CmdSettings()
 *
 * DESCRIPTION: Displays all NetLib settings
 *
 *	CALLED BY:	AppProcessCommand in response to the "settings" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdSettings(int argc, Char * argv[])
{
	Err							err;
	UInt32						ifCreator = 0;
	UInt16						ifInstance = 0;
	Char							text[32];
	UInt16						buflen;
	UInt32						dword=0;
	UInt16						word=0;
	UInt8							byte=0;
	Char *						textP = 0;
	UInt16						i=0;

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	//----------------------------------------------------------------------------
	// If an interface was specified, get it's settings
	//----------------------------------------------------------------------------
	if (argc > 1) {
		ifCreator = PrvDWord(argv[1]);
		if (!ifCreator) goto FullHelp;
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingUp, &byte, &buflen);
		printf("\nUp: \t\t\t\t%d", byte);
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingName, text, &buflen);
		printf("\nName: \t\t\t%s", text);
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingReqIPAddr, &dword, &buflen);
		printf("\nReq IP: \t\t\t%s", NetLibAddrINToA(AppNetRefnum, dword, text));
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingSubnetMask, &dword, &buflen);
		printf("\nSubnet mask: \t\t%s", NetLibAddrINToA(AppNetRefnum, dword, text));
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingBroadcast, &dword, &buflen);
		printf("\nBroadcast: \t\t%s", NetLibAddrINToA(AppNetRefnum, dword, text));
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingUsername, text, &buflen);
		printf("\nUsername: \t\t%s", text);
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingPassword, text, &buflen);
		printf("\nPassword: \t\t%s", text);
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingDialbackUsername, text, &buflen);
		printf("\nDBUsername: \t%s", text);
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingDialbackPassword, text, &buflen);
		printf("\nDBPassword: \t%s", text);
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingAuthUsername, text, &buflen);
		printf("\nAuthUsername: \t%s", text);
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingAuthPassword, text, &buflen);
		printf("\nAuthPassword: \t%s", text);
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingServiceName, text, &buflen);
		printf("\nServiceName: \t\t%s", text);
		
		
		buflen = 0;
		err = NetLibIFSettingGet(AppNetRefnum,ifCreator, ifInstance, netIFSettingLoginScript, 0, &buflen);
		printf("\nLoginScript:");
		if (buflen) {
			textP = MemPtrNew(buflen);
			if (textP) {
				textP[0] = 0;
				NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingLoginScript, textP, &buflen);
				for (i=0; i<buflen; i++) 
					if (textP[i] == 0) textP[i] = '\n';
				textP[buflen-1] = 0;
				printf("\n");
				StdPutS(textP);
				MemPtrFree(textP);
				}
			}
						

		buflen = sizeof(word);
		word = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingInactivityTimeout, &word, &buflen);
		printf("\nInactivityTimeout: \t\t%d", word);
		
		buflen = sizeof(word);
		word = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingEstablishmentTimeout, &word, &buflen);
		printf("\nEstablishmentTimeout: \t%d", word);
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingDynamicIP, &byte, &buflen);
		printf("\nDynamicIP: \t\t\t%d", byte);
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingVJCompEnable, &byte, &buflen);
		printf("\nVJCompression: \t\t%d", byte);
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingVJCompSlots, &byte, &buflen);
		printf("\nVJCompSlots: \t\t\t%d", byte);
		
		buflen = sizeof(word);
		word = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingMTU, &word, &buflen);
		printf("\nMTU: \t\t\t\t%d", word);
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingAsyncCtlMap, &dword, &buflen);
		printf("\nAsyncCtlMap: \t\t\t$%lx", dword);
		
		buflen = sizeof(word);
		word = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingPortNum, &word, &buflen);
		printf("\nPortNum: \t\t\t%d", word);
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingBaudRate, &dword, &buflen);
		printf("\nBaudRate: \t\t\t%ld", dword);
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingFlowControl, &byte, &buflen);
		printf("\nFlowControl: \t\t\t%d", byte);
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingStopBits, &byte, &buflen);
		printf("\nStopBits: \t\t\t\t%d", byte);
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingParityOn, &byte, &buflen);
		printf("\nParityOn: \t\t\t%d", byte);
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingParityEven, &byte, &buflen);
		printf("\nParityEven: \t\t\t%d", byte);
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingUseModem, &byte, &buflen);
		printf("\nUseModem: \t\t\t%d", byte);
		
		buflen = sizeof(byte);
		byte = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingPulseDial, &byte, &buflen);
		printf("\nPulseDial: \t\t\t%d", byte);
		

		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingModemInit, text, &buflen);
		printf("\nModemInit: \t\t%s", text);
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingModemPhone, text, &buflen);
		printf("\nModemPhone: \t\t%s", text);
		
		buflen = sizeof(word);
		word = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingRedialCount, &word, &buflen);
		printf("\nRedialCount: \t\t\t%d", word);
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingTraceBits, &dword, &buflen);
		printf("\nTraceBits: \t\t\t$%lx", dword);
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance, netIFSettingActualIPAddr, &dword, &buflen);
		printf("\nAct IP: \t\t\t\t%s", NetLibAddrINToA(AppNetRefnum, dword, text));
		
		}

	//----------------------------------------------------------------------------
	// Root settings
	//----------------------------------------------------------------------------
	else {
		buflen = sizeof(dword);
		dword = 0;
		NetLibSettingGet(AppNetRefnum, netSettingPrimaryDNS, &dword, &buflen);
		printf("\nPri. DNS: \t\t\t%s", NetLibAddrINToA(AppNetRefnum, dword, text));
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibSettingGet(AppNetRefnum, netSettingSecondaryDNS, &dword, &buflen);
		printf("\nSec. DNS: \t\t\t%s", NetLibAddrINToA(AppNetRefnum, dword, text));
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibSettingGet(AppNetRefnum, netSettingDefaultRouter, &dword, &buflen);
		printf("\nDef. router: \t\t%s", NetLibAddrINToA(AppNetRefnum, dword, text));
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibSettingGet(AppNetRefnum, netSettingDefaultIFCreator, &dword, &buflen);
		MemMove(text, &dword, sizeof(dword));
		text[4] = 0;
		buflen = sizeof(word);
		word = 0;
		NetLibSettingGet(AppNetRefnum, netSettingDefaultIFInstance, &word, &buflen);
		printf("\nDef. IF: \t\t\t%s,%d", text, word);
		
		buflen = sizeof(text);
		text[0] = 0;
		NetLibSettingGet(AppNetRefnum, netSettingHostName, text, &buflen);
		printf("\nHostname: \t%s", text);
		
		buflen = sizeof(text);
		text[0] = 0;
		err = NetLibSettingGet(AppNetRefnum, netSettingDomainName, text, &buflen);
		printf("\nDomainname: \t\t%s", text);
		
		buflen = 0;
		err = NetLibSettingGet(AppNetRefnum, netSettingHostTbl, 0, &buflen);
		printf("\nHostTable:");
		if (buflen) {
			textP = MemPtrNew(buflen);
			if (textP) {
				textP[0] = 0;
				NetLibSettingGet(AppNetRefnum, netSettingHostTbl, textP, &buflen);
				printf("\n");
				StdPutS(textP);
				MemPtrFree(textP);
				}
			}
						
		buflen = sizeof(dword);
		dword = 0;
		NetLibSettingGet(AppNetRefnum, netSettingCloseWaitTime, &dword, &buflen);
		printf("\nCloseWaitTime: \t%ld", dword);
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibSettingGet(AppNetRefnum, netSettingTraceBits, &dword, &buflen);
		printf("\nTraceBits: \t\t$%lx", dword);
		
		buflen = sizeof(dword);
		dword = 0;
		NetLibSettingGet(AppNetRefnum, netSettingTraceSize, &dword, &buflen);
		printf("\nTraceSize: \t\t%ld", dword);
		
		buflen = sizeof(byte);
		dword = 0;
		NetLibSettingGet(AppNetRefnum, netSettingTraceRoll, &byte, &buflen);
		printf("\nTraceRoll: \t\t%d", byte);
		}

	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# Settings\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDisplay NetLib settings");
	printf("\nSyntax: %s [<ifCreator>] ", argv[0]);
	printf("\n      <ifCreator>  # if specified, display settings for interface ", argv[0]);
	printf("\n");

		
}

/***********************************************************************
 *
 * FUNCTION:   CmdDM
 *
 * DESCRIPTION: Displays memory
 *
 *	CALLED BY:	AppProcessCommand in response to the "dm" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdDM(int argc, Char * argv[])
{
	Char							text[32];
	UInt16						i=0;
	UInt16						count;
	UInt8 *						addr;

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	// Get the address
	if (argc < 2) goto FullHelp;
	addr = (UInt8 *)PrvHToI(argv[1]);
	
	// Get the optional count
	if (argc > 2)
		count = PrvHToI(argv[2]);
	else
		count = 8;
		
		
	// Display the memory
	for (i=0; i<count; i++, addr++) {
		if ((i % 8) == 0) {
			StrIToH(text, (UInt32)addr);
			if (i > 0) printf("\n");
			printf("%s: ", text);
			}
		StrIToH(text, *addr);
		printf("%s", text+6);
		if ((UInt32)addr & 0x01) printf(" ");
		}
		

	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t\t# Display Memory\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDisplay Memory");
	printf("\nSyntax: %s <addr> [<count>] ", argv[0]);
	printf("\n");
}
		




/***********************************************************************
 *
 * FUNCTION:   CmdSM
 *
 * DESCRIPTION: Set memory
 *
 *	CALLED BY:	AppProcessCommand in response to the "dm" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdSM(int argc, Char * argv[])
{

	UInt16						i=0;
	UInt8 *						addr;
	UInt32						value;

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	// Get the address
	if (argc < 2) goto FullHelp;
	addr = (UInt8 *)PrvHToI(argv[1]);
	
	printf("\nMemory set at %lx\n", addr);
	
	// Get the data and write it
	for (i=2; i<argc; i++) {
		
		value = PrvHToI(argv[i]);
		switch (StrLen(argv[i])) {
			case 0: 
				break;
			case 1: 
			case 2:
				*((UInt8 *)addr) = value;
				addr+=1;
				break;
			case 3:
			case 4:
				*((UInt16 *)addr) = value;
				addr+=2;
				break;
			default:
				*((UInt32 *)addr) = value;
				addr+=4;
				break;
			}
		}
		
	return;
	
ShortHelp:
	printf("%s\t\t\t# Set Memory\n", argv[0]);
	return;
	
FullHelp:
	printf("\nSet Memory");
	printf("\nSyntax: %s <addr> <data...> ", argv[0]);
	printf("\n");
}
		


/***********************************************************************
 *
 * FUNCTION:   	CmdInfoInstall
 *
 * DESCRIPTION: 	Installs the commands from this module into the
 *		master command table used by AppProcessCommand.
 *
 *	CALLED BY:		AppStart 
 *
 * RETURNED:    	void
 *
 ***********************************************************************/
void CmdInfoInstall(void)
{
	AppAddCommand("_", CmdDivider);
	AppAddCommand("iflog", CmdIFLog);

	AppAddCommand("ifinfo", CmdIFInfo);
	AppAddCommand("ifstats", CmdIFStats);
	AppAddCommand("ipstats", CmdIPStats);
	AppAddCommand("icmpstats", CmdICMPStats);
	AppAddCommand("udpstats", CmdUDPStats);
	AppAddCommand("tcpstats", CmdTCPStats);
	
#if	INCLUDE_SETTINGS
	AppAddCommand("settings", CmdSettings);
#endif

#if	INCLUDE_DM
	AppAddCommand("dm", CmdDM);
#endif

#if	INCLUDE_SM
	AppAddCommand("sm", CmdSM);
#endif


}

