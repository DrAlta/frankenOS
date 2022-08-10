/*******************************************************************
 * 							 PalmPilot Software
 *
 *	Copyright (c) 1996-1997, USRobotics/Palm Computing., All Rights Reserved
 *
 *-------------------------------------------------------------------
 * FileName:
 *		SampleLib.h
 *
 * Description:
 *		Sample library API definitions.  The Sample library serves as an example
 *		of creating PalmPilot shared libraries.
 *
 * History:
 *   	 5/20/97	vmk
 *   	12/18/97	scl Added BUILDING_SAMPLE_LIB to fix building in CW Pro 2
 *    06/26/99  dcat now read/write update the struct with #bytes processed
 *******************************************************************/


#ifndef __SAMPLE_LIB_H__
#define __SAMPLE_LIB_H__


// If we're actually compiling the library code, then we need to
// eliminate the trap glue that would otherwise be generated from
// this header file in order to prevent compiler errors in CW Pro 2.
#ifdef BUILDING_SAMPLE_LIB
	#define SAMPLE_LIB_TRAP(trapNum)
#else
	#define SAMPLE_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif


// PalmPilot common definitions
#include <Common.h>
#include <SystemMgr.rh>

//___a structure for Palm_Pilot sockets
typedef struct {
	UInt		NetLibRefNum;				// NetLib reference number
	long 		NetLibTimeOut;			// value in micro-sec to give up
	DWord		TCPResendTimeout;		// no clue	
	char*		data_buffP;					// for read/write
	int			data_size;					// size of buffer to read/write
	//__the above field is updated by the library now...
	char*		hostIP;							// host ip_addr in dotted format
	int			port;								// port to open on the above host
	int 		mySocket;						// port fd returned from tcp_open
	Err 		errNo;							// if NetLib pukes, this is why	
	struct	sockaddr_in *srv_addrP;	
	struct	sockaddr_in *cli_addrP;	
} SocketType;

typedef SocketType *SocketTypeP;

/********************************************************************
 * Internal library name which can be passed to SysLibFind()
 ********************************************************************/
#define		LibName			"S24Ndk.lib"	


/************************************************************
 * Sample Library result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *************************************************************/

#define sampleErrParam			(appErrorClass | 1)		// invalid parameter
#define sampleErrNotOpen		(appErrorClass | 2)		// library is not open
#define sampleErrStillOpen	(appErrorClass | 3)		// returned from SampleLibClose() if
																									// the library is still open by others
#define sampleErrMemory			(appErrorClass | 4)		// memory error occurred


//-----------------------------------------------------------------------------
// Sample library function trap ID's. Each library call gets a trap number:
//   sampleLibTrapXXXX which serves as an index into the library's dispatch table.
//   The constant sysLibTrapCustom is the first available trap number after
//   the system predefined library traps Open,Close,Sleep & Wake.
//
// WARNING!!! The order of these traps MUST match the order of the dispatch
//  table in SampleLibDispatch.c!!!
//-----------------------------------------------------------------------------

typedef enum {
	sampleLibTrapGetLibAPIVersion = sysLibTrapCustom,
	
	sampleLibTrapGetSocket,
	sampleLibTrapWriteSocket,
	sampleLibTrapReadSocket,
	sampleLibTrapCloseSocket,
	
	sampleLibTrapLast
	} SampleLibTrapNumberEnum;

#ifdef __cplusplus
extern "C" {
#endif


//--------------------------------------------------
// Standard library open, close, sleep and wake functions
//--------------------------------------------------
extern Err NdkLibOpen (UInt refNum)
				SAMPLE_LIB_TRAP(sysLibTrapOpen);
				
extern Err NdkLibClose (UInt refNum)
				SAMPLE_LIB_TRAP(sysLibTrapClose);

extern Err NdkLibSleep (UInt refNum)
				SAMPLE_LIB_TRAP(sysLibTrapSleep);

extern Err NdkLibWake (UInt refNum)
				SAMPLE_LIB_TRAP(sysLibTrapWake);

//--------------------------------------------------
// Custom library API functions___the NDK lib interface
//--------------------------------------------------
extern Err NdkLibGetLibAPIVersion(UInt refNum, DWordPtr dwVerP)
				SAMPLE_LIB_TRAP(sampleLibTrapGetLibAPIVersion);

extern int NdkLibGetSocket (SocketTypeP TcpParmsP, int UDP);

extern int NdkLibWriteSocket (SocketTypeP TcpParmsP);

extern int NdkLibReadSocket (SocketTypeP TcpParmsP);

extern int NdkLibCloseSocket (SocketTypeP TcpParmsP);

// For loading the library in PalmPilot Mac emulation mode
extern Err SampleLibInstall (UInt refNum, SysLibTblEntryPtr entryP);

#ifdef __cplusplus 
}
#endif


#endif	// __SAMPLE_LIB_H__
