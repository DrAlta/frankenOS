/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: CmdSetup.c
 *
 * Description:
 * This module provides various commands to open, close, and change the
 *	timeout value used by the Berkeley sockets API macros of the Net Library.
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
		printf("\n-- Setup Cmds --\n");
}


/***********************************************************************
 *
 * FUNCTION:   CmdTimeout
 *
 * DESCRIPTION:Setup Timeout for all subsequent Net commands
 *
 *	CALLED BY:	 AppExecCommand  
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdTimeout(int argc, Char * argv[])
{

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;

	
	// Get the new timeout value in seconds
	if (argc > 1) 
		AppNetTimeout = sysTicksPerSecond * atol(argv[1]);
	printf("Timeout set to %ld seconds\n", AppNetTimeout/sysTicksPerSecond);
	return;
	
ShortHelp:
	printf("%s\t\t# Get/Set AppNetTimeout\n", argv[0]);
	return;
	
FullHelp:
	printf("\nGet/Set timeout (in seconds) for NetLib commands");
	printf("\nSyntax: %s [<seconds>]", argv[0]);
	printf("\n");
}

/***********************************************************************
 *
 * FUNCTION:   CmdOpen()
 *
 * DESCRIPTION: Test NetLibOpen
 *
 *	CALLED BY:	AppProcessCommand in response to the "open" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdOpen(int argc, Char * argv[])
{
	UInt16		ifErrs;
	Err			err;
	UInt32		ifCreator;
	UInt16		ifInstance;
	
	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;
	
	// If there are no interfaces attached, print error message
	err = NetLibIFGet(AppNetRefnum, 0, &ifCreator, &ifInstance);
	if (err) {
		StdPutS(	"\n##Error: No internet service setup.\n\n");
		#if EMULATION_LEVEL == EMULATION_NONE
			StdPutS(	"Please switch to the Network preference panel "
						"and setup your internet service before you "
						"continue.\n");
		#else
			StdPutS(	"Please quit, "
					 	"run the Network preference panel application, "
					 	"save it's memory card images, and copy them (Pilot Card 0,1) "
					 	"to the current directory before re-launching "
					 	"this application.\n");
		#endif
		return;
		}

	printf("Opening Connection...");
	err = NetLibOpen(AppNetRefnum, &ifErrs);

	if (err == netErrAlreadyOpen)  {
		printf("\nNote: Net Lib was open, open count incremented");
		err = 0;
		}
	if (err) {
		printf("\nOpen Error: %s\n", appErrString(err));
		return;
		}
	if (!err) {
		AppNeedNetLibClose++;
		printf("\nOpened.");
		}

	//-------------------------------------------------------------------
	// If there were errors bringup any interfaces, see which ones
	//-------------------------------------------------------------------
	if (ifErrs) {
		UInt16		index;
		UInt8		up;
		UInt16		settingSize;
		Char		ifName[32];
		
		printf("\nInterface Error: %s", appErrString(ifErrs));
		if (ifErrs == netErrPrefNotFound) {
			printf("\nThis most likely means that your internet setup is incomplete.");
			printf("\nSwitch to the Network preference panel and check your setup.");
			}
		for (index = 0; 1; index++) {
			err = NetLibIFGet(AppNetRefnum, index, &ifCreator, &ifInstance);
			if (err) break;

			settingSize = sizeof(up);
			err = NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance,
				netIFSettingUp, &up, &settingSize);
			if (err || up) continue;
			
			settingSize = 32;
			err = NetLibIFSettingGet(AppNetRefnum, ifCreator, ifInstance,
				netIFSettingName, ifName, &settingSize);
			if (err) continue;
			
			printf("\nInterface %s did not come up.", ifName);
			}

			
		// Close the NetLib if any interfaces failed to come up.
		AppExecCommand("close");
		}


	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# Open NetLib\n", argv[0]);
	return;
	
FullHelp:
	printf("\nOpen NetLib");
	printf("\nSyntax: %s ", argv[0]);
	printf("\n");

}


/***********************************************************************
 *
 * FUNCTION:   CmdClose()
 *
 * DESCRIPTION: Test NetLibClose
 *
 *	CALLED BY:	AppProcessCommand in response to the "close" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdClose(int argc, Char * argv[])
{
	Err		err;
	
	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;
	
	printf("\nClosing NetLib...");
	err = NetLibClose(AppNetRefnum, false);
	if (err) printf("\nError closing: %s", appErrString(err));
	else printf("\nClosed.");
	
	if (AppNeedNetLibClose) AppNeedNetLibClose--;

	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# Close NetLib\n", argv[0]);
	return;
	
FullHelp:
	printf("\nClose NetLib");
	printf("\nSyntax: %s ", argv[0]);
	printf("\n");

}

/***********************************************************************
 *
 * FUNCTION:   	CmdSetupInstall
 *
 * DESCRIPTION: 	Installs the commands from this module into the
 *		master command table used by AppProcessCommand.
 *
 *	CALLED BY:		AppStart 
 *
 * RETURNED:    	void
 *
 ***********************************************************************/
void CmdSetupInstall(void)
{
	AppAddCommand("_", CmdDivider);
	
	AppAddCommand("timeout", CmdTimeout);
	AppAddCommand("open", CmdOpen);
	AppAddCommand("close", CmdClose);
}

