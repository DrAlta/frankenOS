/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: NetSample.c
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

// Include the PalmOS headers
#include <PalmOS.h>

#include <NetMgr.h>

// Include this header so that we can use the Berkeley sockets API
#include <sys_socket.h>

// This header is currently located in this application's project
// directory but could be used by other applications as well. It
// provides calls for primitive stdio to a window.
#include "AppStdIO.h"

// Application specific includes
#include "NetSample.h"
#include "NetSampleRsc.h"

/***********************************************************************
 *
 *	Prototypes of private functions to this module
 *
 ***********************************************************************/
static Err		AppNetInit(void);
static Err		AppNetFree(void);
static Err 		NetLibCompatible (UInt32 requiredVersion, UInt16 launchFlags);


/***********************************************************************
 *
 *	Global variables
 *
 ***********************************************************************/
static MenuBarPtr		CurrentMenuP;
static UInt16			CurrentViewID;
UInt16					AppNeedNetLibClose = 0;
Char						AppErrStr[kAppErrStrLen];

// This is needed because we don't link with stdlib when we compile
//  for the device.
#if EMULATION_LEVEL == EMULATION_NONE
Err			errno;
#endif


/***********************************************************************
 *
 * FUNCTION:    MainViewInit
 *
 * DESCRIPTION: This routine initializes the "Main View" of the 
 *              NetSample application.
 *
 * PARAMETERS:  frmP - pointer to form
 *
 * RETURNED:    void
 *
 ***********************************************************************/
static void MainViewInit (FormPtr frmP)
{
	CurrentViewID = MainViewForm;
}

/***********************************************************************
 *
 * FUNCTION:    MainViewDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static Boolean MainViewDoCommand (UInt16 command)
{
	NetMasterPBType	pb;
	Err					err;
	Char					text[100];
	UInt32				oldTrace;
	Int16					i;
	UInt32				value;
	UInt16				settingSize;
	Boolean				setTraceBits = false;
	UInt16				index;
	UInt8					traceRoll;
	Char *				cmdP;
	Boolean 				handled = true;
	
	switch (command) {
		//...........................................................
		// Utility Menu
		//...........................................................
		case MainViewUDPDayTime:
			cmdP = "daytime -t best.com";
			printf("\n%s...\n", cmdP);
			AppExecCommand(cmdP);
			break;
			
		case MainViewUDPDNS:
			cmdP = "hostent www.usr.com";
			printf("\n%s...\n", cmdP);
			AppExecCommand(cmdP);
			break;
			
		case MainViewUDPEcho:
			cmdP = "echo -p t -b 2000 -l 3 best.com";
			printf("\n%s...\n", cmdP);
			AppExecCommand(cmdP);
			break;
			
		case MainViewUDPDiscard:
			cmdP = "discard -p t -b 2000 -l 3 best.com";
			printf("\n%s...\n", cmdP);
			AppExecCommand(cmdP);
			break;
			
		case MainViewUDPCharGen:
			cmdP = "chargen -p t -b 2000 -l 3 best.com";
			printf("\n%s...\n", cmdP);
			AppExecCommand(cmdP);
			break;
			
		case MainViewUDPTelnet:
			cmdP = "telnet best.com";
			printf("\n%s...\n", cmdP);
			AppExecCommand(cmdP);
			break;
			
		case MainViewUDPFTP:
			FrmAlert(UnImplementedAlert);
		/*	cmdP = "ftp ftp.best.com";
			printf("\n%s...\n", cmdP);
			AppExecCommand(cmdP);
		 */	
			break;
			
		//...........................................................
		// Debug Menu
		//...........................................................
		case MainviewDebugConnectLog:
			AppExecCommand("iflog");
			break;
			
		case MainviewDebugInterfaceInfo: 
			AppExecCommand("ifinfo");
			break;
		case MainviewDebugInterfaceStats:
			AppExecCommand("ifstats");
			break;
		case MainviewDebugIPStats:
			AppExecCommand("ipstats");
			break;
		case MainviewDebugICMPStats:
			AppExecCommand("icmpstats"); 
			break;
		case MainviewDebugUDPStats:
			AppExecCommand("udpstats");
			break;
		case MainviewDebugTCPStats:
			AppExecCommand("tcpstats");
			break;
			
		case MainviewDebugTraceAll: 
		case MainviewDebugTraceErrMsg: 
		case MainviewDebugTraceNone: 

			// Assign a trace buffer
			if (command != MainviewDebugTraceNone) 
				value = 0x0800;
			else 
				value = 0;
			settingSize = sizeof(value);
			err = NetLibSettingSet(AppNetRefnum, netSettingTraceSize, &value, settingSize);
			if (err) {
				printf("Error - %s", appErrString(err));
				break;
				}
				
			// Figure out trace bits
			if (command == MainviewDebugTraceNone)  
				value = 0;
			else if (command == MainviewDebugTraceErrMsg)
				value = netTracingErrors | netTracingMsgs | netTracingAppMsgs;
			else
				value = 0xFFFFFFFF;
				
			printf("Tracing: %s %s %s %s %s %s",
				value ? " " : "off.",
				value & netTracingErrors ? "Errs" : " ",
				value & netTracingMsgs ? "Msgs" : " ",
				value & netTracingPkts ? "Pkts" : " ",
				value & netTracingAppMsgs ? "App" : " ",
				value & netTracingFuncs ? "Funcs" : " ");
				
			// Set trace level for stack
			NetLibSettingSet(AppNetRefnum, netSettingTraceBits, &value, settingSize);

			// Set rollover to none
			traceRoll = false;
			NetLibSettingSet(AppNetRefnum, netSettingTraceRoll, &traceRoll, sizeof(traceRoll));

			// Set trace bits for all attached interfaces
			for (index = 0; 1; index++) {
				UInt32	ifCreator;
				UInt16	ifInstance;
				
				err = NetLibIFGet(AppNetRefnum, index, &ifCreator, &ifInstance);
				if (err) {err = 0; break; }
				NetLibIFSettingSet(AppNetRefnum, ifCreator, ifInstance, netIFSettingTraceBits, 
							&value, settingSize);
				}
			printf("\n");
			break;
			
			
		case MainviewDebugTraceDisplay:

			// Temporarily disable tracing
			settingSize = sizeof(oldTrace);
			NetLibSettingGet(AppNetRefnum, netSettingTraceBits, &oldTrace, &settingSize);
			value = 0;
			err = NetLibSettingSet(AppNetRefnum, netSettingTraceBits, &value, settingSize);

			if (oldTrace == 0)
				StdPutS("Tracing not on\n");
			else {
				StdPutS("From oldest to newest...");
				StdPutS("\nTICKS EVENT  ROUTINE\n");
				}
				
			// See what the oldest entry is
			for (i=0; 1; i++) {
				pb.param.traceEventGet.index = i;
				pb.param.traceEventGet.textP = text;
				err = NetLibMaster(AppNetRefnum, netMasterTraceEventGet, &pb, AppNetTimeout);
				if (err) {
					printf("Error: %s\n", appErrString(err));
					break;
					}
				}

			// Print them in oldest to newest order
			for (i=i-1; i>=0 ; i--) {
				pb.param.traceEventGet.index = i;
				pb.param.traceEventGet.textP = text;
				err = NetLibMaster(AppNetRefnum, netMasterTraceEventGet, &pb, AppNetTimeout);
				if (err) break;
				StdPutS(text);
				StdPutS("\n");
				}
			StdPutS("Done.\n");

			// Restore tracing level
			err = NetLibSettingSet(AppNetRefnum, netSettingTraceBits, &oldTrace, settingSize);
			break;
			
			
		//...........................................................
		// Misc Menu
		//...........................................................
		case MainviewMiscMenu:
			MenuEraseStatus (CurrentMenuP);
			AbtShowAbout (kAppCreator);
			break;
			
		default:
			// allow edit events to be handled by frmhandleevent
			handled = false;
			break;
		
		}	

	return handled;
}

/***********************************************************************
 *
 * FUNCTION:    MainViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Main View"
 *              of the NetSample application
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 ***********************************************************************/
static Boolean MainViewHandleEvent (EventPtr event)
{
	FormPtr 	frmP;
	Boolean 	handled = false;
	
	
	// See if StdIO can handle it
	if (StdHandleEvent(event)) return true;
	
	//---------------------------------------------------------------
	// Key Events
	//---------------------------------------------------------------
	if (event->eType == keyDownEvent) {
		}
		
	//---------------------------------------------------------------
	// Controls
	//---------------------------------------------------------------
	else if (event->eType == ctlSelectEvent) {
		switch (event->data.ctlSelect.controlID) {
			/* A case statement for each button
			case NewMemoButton:
				handled = true;
				break;
			*/
			}
		}

	//---------------------------------------------------------------
	// Menus
	//---------------------------------------------------------------
	else if (event->eType == menuEvent) {
		return MainViewDoCommand (event->data.menu.itemID);
		}

	//---------------------------------------------------------------
	// Form Open
	//---------------------------------------------------------------
	else if (event->eType == frmOpenEvent) {
		frmP = FrmGetActiveForm ();
		MainViewInit (frmP);
		FrmDrawForm (frmP);
		handled = true;
		
		// Open the Net Lib.
		AppExecCommand("open");
		
		// If successfully opened, print welcome message
		if (AppNeedNetLibClose) {
			printf("\nWelcome to NetSample!");
			printf("\nType 'help' to get a list of commands, or choose a"
					 " menu item.\n");
			}
		}

	return (handled);
}

/***********************************************************************
 *
 * FUNCTION:   	AppAddCommand
 *
 * DESCRIPTION: 	Adds a command to the master command table
 *						Each of the command modules makes calls to this routine
 *						to install it's commands into the table.
 *
 *	CALLED BY:		CmdInfoInstall(), CmdSockInstall(), etc. during startup 
 *
 * RETURNED:    	void
 *
 ***********************************************************************/
typedef struct {
	Char *				nameP;							// command name
	CmdProcPtr			procP;							// procedure
	} CmdTableEntryType;
#define					kMaxCommands 	64				// Max # of commands
UInt16					NumCommands = 0;
CmdTableEntryType		CmdTable[kMaxCommands];		// command table
	
void AppAddCommand(Char * cmdStr, CmdProcPtr procP)
{
	if (NumCommands >= kMaxCommands-1) {
		ErrDisplay("Too many commands");
		return;
		}
		
	CmdTable[NumCommands].nameP = cmdStr;
	CmdTable[NumCommands++].procP = procP;
}

/******************************************************************************
 *
 * FUNCTION:   	AppExecCommand
 *
 * DESCRIPTION: 	Separates a command line into argc, argv components and then
 *						calls the appropriate routine.
 *
 *	CALLED BY:		Menu command handlers, command line processor, etc.
 *
 * RETURNED:    	void
 *
 *****************************************************************************/
void AppExecCommand(Char * cmdParamP)
{
	const int	maxArgc=10;
	Char *		argv[maxArgc+1];
	int			argc;
	Boolean		done = false;
	UInt16			i;
	Char *		cmdBufP = 0;
	Char *		cmdP;

	// if null string, return
	if (!cmdParamP[0]) return;

	// Make a copy of the command since we'll need to write
	// 0's between the words and it might be from read-only space
	cmdP = cmdBufP = MemPtrNew(StrLen(cmdParamP) + 1);
	if (!cmdP) {
		printf("\nOut of memory\n");
		goto Exit;
		}
	MemMove(cmdP, cmdParamP, StrLen(cmdParamP) + 1);

	// Separate the cmd line into arguments
	argc = 0;
	argv[0] = 0;
	for (argc=0; !done && argc<=maxArgc; argc++) {
	
		// Skip leading spaces
		while((*cmdP == ' ' || *cmdP == '\t') && (*cmdP != 0))
			cmdP++;
			
		// Break out on null
		if (*cmdP == 0) break;
		
		// Get pointer to command
		argv[argc] = cmdP;
		if (*cmdP == 0) break;
		
		// Find the end of a quoted argument
		if (*cmdP == '"') {
			cmdP++;
			argv[argc] = cmdP;
			while(*cmdP) {
				if (*cmdP == '"') {*cmdP = 0; break;}
				if (*cmdP == 0) {done = true; break;}
				cmdP++;
				}
			cmdP++;
			}
			
		// Find the end of an unquoted argument
		else {
			while(1) {
				if (*cmdP == 0) {done = true; break;}
				if ((*cmdP == ' ') || (*cmdP == '\t')) {
					*cmdP = 0; 
					break;
					}
				cmdP++;
				}
			cmdP++;
			}
		}
	
	// Return if no arguments
	if (!argc) goto Exit;
		
	//--------------------------------------------------------------------
	// Call the appropriate routine
	//--------------------------------------------------------------------
	
	// Look for the help command
	if (!StrCompare(argv[0], "help") || !StrCompare(argv[0], "?")) {
	
		// If asking about a particular command, get extended help (??)
		if (argc > 1) {
			argv[0] = argv[1];
			argv[1] = "??";
			}
			
		// Else, display 1 line help (?)
		else {
			Char *	helpP;
			helpP = "?";
			for (i=0; i<NumCommands; i++) {
				argv[0] = CmdTable[i].nameP;
				argv[1] = helpP;
				(CmdTable[i].procP)(2, argv);
				
				// Pause after every "page"
				if (i > 0 && (i % 8) == 0) {
					printf("<cr> to continue...");
					if (StdGetChar(true) != '\n') break;
					printf("\n");
					}
				}
			printf("\n");
			goto Exit;
			}
		}
		
	// Else, look for this command
	for (i=0; i<NumCommands; i++) {
		if (!StrCompare(argv[0], CmdTable[i].nameP)) {
			(CmdTable[i].procP)(argc, argv);
			goto Exit;
			}
		}
	
	// Not found
	printf("Unknown command: %s\n", argv[0]);
	
Exit:
	if (cmdBufP) MemPtrFree(cmdBufP);
	cmdP = 0;
	cmdBufP = 0;
			
}

/***********************************************************************
 *
 * FUNCTION:     AppStart
 *
 * DESCRIPTION:  This routine opens the application's database, loads the 
 *               saved-state information and initializes global variables.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
static Boolean AppStart (void)
{
	Err						err;

	// Init our terminal functions for printing to the screen
	StdInit(MainViewForm, MainViewTextField, MainViewScrollbarScrollBar,
					AppExecCommand);
	
	// Get the refNum of the Net Library
	err = SysLibFind("Net.lib", &AppNetRefnum);
	if (err) {
		ErrDisplay("\nNet Library not found, exiting...");
		return err;
		}

	// Add commands from each of the modules
	CmdInfoInstall();
	CmdStevensInstall();
	CmdSetupInstall();
/*	CmdFTPInstall(); */
	CmdTelnetInstall();
		
	// Set our default Timeout
	AppNetTimeout = 10*sysTicksPerSecond;

	return 0;
}

/***********************************************************************
 *
 * FUNCTION:    AppStop
 *
 * DESCRIPTION: This routine closes the application's database
 *              and saves the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:  
 *					blt	10/25/99	added calls to FrmCloseAllForms and StdFree
 *										to fix a memory leak problem that came out of 
 *										hiding for the debug roms.  FrmCloseAllForms
 *										is essential for a well-behaved app and StdFree
 *										is used whenever an app uses any of the AppStdIO
 *										calls (for easy drawing to the screen).
 *
 ***********************************************************************/
static void AppStop (void)
{
	// Close Net Library, if necessary
	while (AppNeedNetLibClose)
		AppExecCommand("close");

	// Under emulation, force the NetLib out of the close-wait state
	// before we exit.
	#if EMULATION_LEVEL != EMULATION_NONE
		if (AppNetRefnum) 
			NetLibFinishCloseWait(AppNetRefnum);
	#endif
	
	FrmCloseAllForms();
		
	StdFree();		// See AppStdIO.h for details on when and how to use this.
}

/***********************************************************************
 *
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has been handled and should not be passed
 *              to a higher level handler.
 *
 ***********************************************************************/
static Boolean AppHandleEvent (EventPtr event)
{
	UInt16	formID;
	FormPtr 	frm;

	if (event->eType == frmLoadEvent) {
		// Load the form resource.
		formID = event->data.frmLoad.formID;
		frm = FrmInitForm (formID);
		FrmSetActiveForm (frm);		
		
		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formID) {
			case MainViewForm:
				FrmSetEventHandler (frm, MainViewHandleEvent);
				break;
	
			}
		return (true);
		}
	return (false);
}

/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: Infinite (almost) loop to process events
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void AppEventLoop(void)
{
	UInt16	 	error;
	EventType 	event;

	do {
		EvtGetEvent (&event, evtWaitForever);
		
		if (! SysHandleEvent (&event))
		
			if (! MenuHandleEvent (CurrentMenuP, &event, &error))
			
				if (! AppHandleEvent (&event))
	
					FrmDispatchEvent (&event); 
					
					
		} while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the Pilot
 *              application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
UInt32	PilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	UInt16 error = 0;

	// If normal launch
	if (cmd == sysAppLaunchCmdNormalLaunch) 
	{
		/* can we load the net library? not on 1.0, possibly on 2.0, always on 3.0 */	
		error = NetLibCompatible (0x02000000, launchFlags);
		if (error) return (error);
	
		error = AppStart ();
		if (error)
			return (error);

		FrmGotoForm (MainViewForm);
		AppEventLoop ();
		AppStop ();
		}


	return 0;
}

/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version meets your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *                             
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/15/96	Initial Revision
 *			pbs   6/9/98   Added check for net.lib
 *
 ***********************************************************************/
static Err NetLibCompatible (UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;
	UInt16 err;
	
	// See if we have at least the minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
	{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
		(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
		{
			FrmAlert (RomIncompatibleAlert);
		
			// Pilot 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < 0x02000000)
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
		}
		
		return (sysErrRomIncompatible);
	}
	else
	{
		// proper version, however, do we have net.lib?
		err = SysLibFind("Net.lib", &AppNetRefnum);
		if (err) 
		{
			if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
			{
				FrmAlert (NetLibIncompatibleAlert);
			
				// Pilot 1.0 will continuously relaunch this app unless we switch to 
				// another safe one.
				if (romVersion < 0x02000000)
					AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			}
		
			return (sysErrRomIncompatible);
		}
	}

	return (0);
}

