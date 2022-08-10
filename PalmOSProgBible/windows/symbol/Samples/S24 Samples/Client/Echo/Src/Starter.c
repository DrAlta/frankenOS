/************************************************************************
* COPYRIGHT:   Copyright  ©  1999, 2000, & 2001 Symbol Technologies, Inc. 
*
* FILE:        Starter.c
*
* SYSTEM:      Symbol barcode scanner for Model 1740 from Symbol.
* 
* MODULE:      Spectrum24 Scan Enabled lookup program.
*              
* DESCRIPTION: This is the client program that works in conjunction with
* UdpEcho.exe or TcpEcho.exe. The program allows the user to select the
* IP address of the echo host, the size in bytes of the data to transfer,
* the delay in seconds between write/read loops, and the number of loops
* to perform.  The user can select either IP or UDP mode to connect.
*
* When the user hits the start button, we connect to the echo host and
* send the data to the host.  We then wait for the data to be echoed back
* back.  When all the data has been received, we compare the send buffer
* & store it in the received buffer. If the buffers don't match we close 
* teh socket and return to the main form. If they do, we delay the ]
* specified number of seconds, and then clear the receive buffer and 
* repeat the loop.
*
* REVISION HISTORY:
*         Name   Date       Description
*         -----  ---------- -----------
*         dcat   06/14/1999 Initial Release
*             
*************************************************************************/
#include <Pilot.h>
#include <SysEvtMgr.h>
#include <sys_socket.h>
#include <netmgr.h>
#include "EchoRsc.h"
         

/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/
typedef struct 
	{
	Byte replaceme;
	} StarterPreferenceType;

typedef struct 
	{
	Byte replaceme;
	} StarterAppInfoType;

typedef StarterAppInfoType* StarterAppInfoPtr;


/***********************************************************************
 *
 *   Protos & Global variables
 *
 ***********************************************************************/

//Boolean		KeepGoing = true;
int 		fd;
char		msg [40];
char		*send_buf = 0;
char		*recv_buf = 0;
UInt		TimeOut = 0;
extern		Word AppNetRefnum;
/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator			's240'
#define appVersionNum            0x01
#define appPrefID                0x00
#define appPrefVersionNum        0x01

// Define the minimum OS version we support
#define ourMinVersion	sysMakeROMVersion(3,0,0,sysROMStageRelease,0)

/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version is meet your
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
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 *
 ***********************************************************************/
static Err RomVersionCompatible(DWord requiredVersion, Word launchFlags)
{
	DWord romVersion;

	// See if we're on in minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
		{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
			{
			FrmAlert (RomIncompatibleAlert);
		
			// Pilot 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			}
		
		return (sysErrRomIncompatible);
		}

	return (0);
}



/***********************************************************************
 *
 * FUNCTION:    MainFormInit
 *
 * DESCRIPTION: This routine initializes the MainForm form.
 *
 * PARAMETERS:  frm - pointer to the MainForm form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 *
 ***********************************************************************/
static void MainFormInit(FormPtr frmP)
{		
		// initialize the data fields...
		SetFieldText (MainHostIPField,  "157.235.93.2",79, false);
		SetFieldText (MainSizeField,    "1024",79, false);		
		SetFieldText (MainDelayField,   "0",79, false);
		SetFieldText (MainStatusField,  "Press start now.",79, false);
		FrmDrawForm  (frmP );
}


/***********************************************************************
 *
 * FUNCTION:    MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
static Boolean MainFormDoCommand(Word command)
{
	Boolean handled = false;

	switch (command)
		{
		case MainOptionsAbout:
			MenuEraseStatus (0);
			AbtShowAbout (appFileCreator);
			handled = true;
			break;

		}
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "MainForm" of this application.
 *
 * PARAMETERS:  eventP  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr eventP)
{
  Boolean handled = false;
  FormPtr frmP;
  
	switch (eventP->eType) 
	{
		case menuEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			MainFormInit( frmP);
			FrmDrawForm ( frmP);
			handled = true;
			break;
			
		case ctlSelectEvent:			
			if (eventP->data.ctlEnter.controlID == MainStartButton)
			{  		
				if (cmdStart())	// do the real work
				{
					FrmCustomAlert( ConnectAlert, "Failed to Connect", NULL, NULL );	
					break;
				}
					
				handled = true;				
			}
	   	break;

		default:
			break;
	}		
	
	return handled;
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
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 *
 ***********************************************************************/
static Boolean AppHandleEvent( EventPtr eventP)
{
	Word formId;
	FormPtr frmP;


	if (eventP->eType == frmLoadEvent)
		{
		// Load the form resource.
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time it receives an
		// event.
		switch (formId)
			{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;
				
			default:
//				ErrFatalDisplay("Invalid Form Load Event");
				break;

			}
		return true;
		}
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 *
 ***********************************************************************/
static void AppEventLoop(void)
{
	Word error;
	EventType event;


	do {
		EvtGetEvent(&event, evtWaitForever);
		
		
		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(0, &event, &error))
				if (! AppHandleEvent(&event))
					FrmDispatchEvent(&event);

	} while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:     AppStart
 *
 * DESCRIPTION:  Get the current application's preferences.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err value 0 if nothing went wrong
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 *
 ***********************************************************************/
static Err AppStart(void)
{

   return 0;
}


/***********************************************************************
 *
 * FUNCTION:    AppStop
 *
 * DESCRIPTION: Save the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 *
 ***********************************************************************/
static void AppStop (void)
{
	
}


/***********************************************************************
 *
 * FUNCTION:    StarterPilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 *
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
static DWord StarterPilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
	Err error;


	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) return (error);

	switch (cmd)
		{
		case sysAppLaunchCmdNormalLaunch:
			error = AppStart();
			if (error) 
				return error;
				
			FrmGotoForm(MainForm);
			AppEventLoop();
			AppStop();
			break;

		default:
			break;

		}
	
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 *
 ***********************************************************************/
DWord PilotMain( Word cmd, Ptr cmdPBP, Word launchFlags)
{
    return StarterPilotMain(cmd, cmdPBP, launchFlags);
}


