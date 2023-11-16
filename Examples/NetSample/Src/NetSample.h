/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: NetSample.h
 *
 * Description:
 * This application is provided as an example of how to use the Net Library
 * of the PalmOS when writing internet applications. It does NOT have a pretty
 * UI and should not be used as a model application. It does however show
 * how to use the Net Library to implement various internet applications like
 * ftp, telnet, etc. 
 *
 * When launched, this application shows a scrolling text window where commands
 * can be entered using Graffiti, or chosen from the menus. Most of the
 * commands take a wide range of command-line arguments. The 'help' command
 * will list the available commands and 'help <command>' will list help
 * on a specific command.
 *
 * IMPORTANT: In order to run this application in the Simulator, you must 
 * have a memory card image file ("Pilot Card 0") in it's directory that has 
 * the "Net Prefs" database created by the "Network" preference panel 
 * in it. Without this database present, you won't be able to dial up and
 * establish a connection with an internet service provider. To create this
 * memory card image, run the "Network" preference panel simulator app and
 * set things up for your ISP. Then, save the memory card image using the 
 * "File" menu and then copy it to the NetSample directory. 
 *
 *****************************************************************************/

/*#include <unix_stdarg.h>*/


/********************************************************************
 * #define these to include the corresponding commands in the final 
 *  executable.
 *
 * When compiling for the device, we don't have room for all the commands
 *
 ********************************************************************/
#if EMULATION_LEVEL != EMULATION_NONE	
	#define	INCLUDE_ALL		1
#else
	#define	INCLUDE_ALL		0
#endif


#define	INCLUDE_HOSTENT				(1 | INCLUDE_ALL)	
#define	INCLUDE_DAYTIME				(1 | INCLUDE_ALL)	
#define	INCLUDE_TIMER					(1 | INCLUDE_ALL)	
#define	INCLUDE_ECHO					(1 | INCLUDE_ALL)	
#define	INCLUDE_DISCARD				(1 | INCLUDE_ALL)	
#define	INCLUDE_READ					(1 | INCLUDE_ALL)	
#define	INCLUDE_CHARGEN				(1 | INCLUDE_ALL)	
#define	INCLUDE_TORTURE				(1 | INCLUDE_ALL)	
#define	INCLUDE_SETTINGS				(1 | INCLUDE_ALL)	
#define	INCLUDE_DM						(1 | INCLUDE_ALL)
#define	INCLUDE_SM						(1 | INCLUDE_ALL)
#define	INCLUDE_FTP						(1 | INCLUDE_ALL)
#define	INCLUDE_TELNET					(1 | INCLUDE_ALL)



/********************************************************************
 * Normal stuff
 ********************************************************************/
// General Equates
#define	kAppCreator							'NETS'

//--------------------------------------------
// Error Handling
//--------------------------------------------
#define	kAppErrStrLen		32
extern	Char					AppErrStr[kAppErrStrLen];
#define	appErrString(err)	\
				SysErrString(err,AppErrStr,kAppErrStrLen)


//--------------------------------------------
// Functions
//--------------------------------------------
typedef 	void (*CmdProcPtr)(int argc, char* argv[]);

void 		AppExecCommand(char * cmd);
void 		AppAddCommand(char * cmdStr, CmdProcPtr cmdProcP);

void 		CmdInfoInstall(void);
void 		CmdStevensInstall(void);
void 		CmdSetupInstall(void);
/*void 		CmdFTPInstall(void); */
void 		CmdTelnetInstall(void);


//--------------------------------------------
// Globals
//--------------------------------------------
extern UInt16		AppNeedNetLibClose;

