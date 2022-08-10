/************************************************************************
* COPYRIGHT:   Copyright  ©  1999, 2000, & 2001 Symbol Technologies, Inc. 
*
* FILE:        CaDmv.c
*
*************************************************************************/
#include "Pilot.h"			// all the system toolbox headers
#include <sys_socket.h>		// required for sockaddr_in type

//___the stuff our application needs
#include "S24API.h"			// SanJose RF driver calls	
#include "CaDmvRsc.h"		// this programs resources		
#include "CaDmv.h"			// this programs data structures 
#include "NdkLib.h"			// DCat's stuff
#include "MsrMgrLib.h"	// Chuck's stuff
#include "Msr3000.h"		// gene'd by Apex Configurator program 


/***********************************************************************
 * Globals 
 **********************************************************************/
MenuBarPtr	CurrentMenu;		
static Boolean GMsrMgrLibWasPreLoaded = false;
char	msg[40];
int GotMilk;

VoidPtr theMagStripP;		// data from mag stripe input goes here
VoidPtr theImageP;			// image returned from host goes here

ULong	versionNum;
ULong	libNum;
UInt	GMsrMgrLibRefNum = sysInvalidRefNum;	// MSR manager library reference number

//___this main applications data structures
SocketType	SocketParms;	// sturcture to define the Tcp library socket 
DmvType		DmvRecord;		// structure for drivers license info
struct 		sockaddr_in srv_addr;	// server's Internet socket ip_addr 
struct 		sockaddr_in cli_addr;	// client's Internet socket ip_addr 

//___define the minimum required OS version we support
#define ourMinVersion	sysMakeROMVersion (3,0,0,sysROMStageRelease,0)

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
 *         dcat   05/01/1999 Initial Release
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
 * FUNCTION:     ImageFormOnInit - called from MainFormHandleEvent
 *
 * DESCRIPTION:  This routine sets up the initial state of the main form
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Nothing.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
static void ImageFormOnInit()
{
	FormPtr frmP = FrmGetActiveForm();	
	FrmDrawForm (frmP);	
	//___If we get no data back from the lookup, we crash here. Changed
	//   the NdkLibRead/Write to update the .data_size field. We test
	//   after the NdkLibReadSocket call & set GotMilk.	
	if (GotMilk)
		WinDrawBitmap (theImageP, 0, 10);	

}

/***********************************************************************
 *
 * FUNCTION:		AboutOnInit - called from AboutFormHandleEvent
 *
 * DESCRIPTION:	Initialize controls on the about form
 * PARAMETERS:		None.
 *
 * RETURNED:		Nothing
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
static void AboutFormOnInit( void )
{
	int status =0;
	char chrRFDriverVersion[20];

	FormPtr frmP = FrmGetActiveForm();
	
#ifndef Debug		
	status = MsrGetVersion (GMsrMgrLibRefNum, &versionNum, &libNum);
	if (status)
	{
		StrPrintF(msg, "MsrGetVersion Err: x%x",status);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
		return;	
	}
	else
	{ 
		StrPrintF (msg, "v %U", versionNum);
		SetFieldText (AboutMSDriverField, msg, 39);
	}	
#endif

	status = S24GetDriverVersion (chrRFDriverVersion, 20);
	if (status)
	{
		StrPrintF(msg, "24GetDriverVersion Err: x%x",status);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
		return;	
	}
	
	StrPrintF (msg, " v%s", chrRFDriverVersion);
	SetFieldText (AboutRFDriverField, msg, 39);

	FrmDrawForm (frmP );
	
}

/***********************************************************************
 *
 * FUNCTION:     MainFormOnInit - called from MainFormHandleEvent
 *
 * DESCRIPTION:  This routine sets up the initial state of the main form
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Nothing.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
static void MainFormOnInit()
{

	FormPtr frmP = FrmGetActiveForm();
	
	FrmDrawForm (frmP);

				
}

/***********************************************************************
 *
 * FUNCTION:     MainFormHandleMenu
 *
 * DESCRIPTION:  This routine handles menu selections off of the main form
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Nothing.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
void MainFormHandleMenu (Word menuSel)
{
	switch( menuSel )
	{
		case OptionsAbout:
			FrmGotoForm (AboutForm);
			break;
	}
}

/***********************************************************************
 *
 * FUNCTION:     StopApplication
 *
 * DESCRIPTION:  This routine does any cleanup required, including 
 * shutting down the decoder and Scan Manager shared library.
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Nothing.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
void StopApplication (void)  	
{


	MemPtrFree (theMagStripP);
	MemPtrFree (theImageP);
	NdkLibCloseSocket (&SocketParms);
	
#ifndef Debug	// switch is set in debug.h
Err status;
	MsrSetDefault (GMsrMgrLibRefNum);
	
	MsrClose (GMsrMgrLibRefNum);
		
	if (!GMsrMgrLibWasPreLoaded && GMsrMgrLibRefNum != sysInvalidRefNum )
	{
		status = SysLibRemove (GMsrMgrLibRefNum);
		ErrFatalDisplayIf (status, "error uninstalling MSR Manager library.");
		GMsrMgrLibRefNum = sysInvalidRefNum;
	}
#endif
						
}


/***********************************************************************
 *
 * FUNCTION: 		StartApplication-called by PilotMain
 *
 * DESCRIPTION: 	This routine sets up the initial state of the applica-
 * tion.  Set the Main Form as the initial form to display. Checks to 
 * make sure we're running on Symbol hardware, then calls ScanOpenDecoder
 * to initialize the Scan Manager. 
 *
 * If successful, then we proceed with setting decoder parameters that 
 * we care about for this application.  ScanCmdSendParams is called to 
 * send our params to the decoder.
 *
 * PARAMETERS: 	None.
 *
 * RETURNED: 		0 if AOK else an error code.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
static Err StartApplication(void)
{

	Err status;
	
	FrmGotoForm( MainForm );
	
	//___get the memory for MagStripe data
	theMagStripP = MemPtrNew (MAX_CARD_DATA+2);
	if (theMagStripP == NULL)
	{
		status = -1;
		StrPrintF (msg, "Out of Memory: x%x", status);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
		return (status);
	}
	MemSet (theMagStripP, MAX_CARD_DATA+2, 0);
	
	//___get the memory for our image from the host	
	theImageP = MemPtrNew (ImageSize);
	if (theImageP == NULL)
	{
		status = -2;
		StrPrintF (msg, "Out of Memory: x%x", status);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
		return (status);
	}
	MemSet (theImageP, ImageSize, 0);	
	
#ifndef Debug		
	//___Load the MSR library
	status = SysLibFind (MsrMgrLibName, &GMsrMgrLibRefNum);
	if (!status) 
	{
		GMsrMgrLibWasPreLoaded = true;
	}
	else 
	{
		status = SysLibLoad (MsrMgrLibTypeID, MsrMgrLibCreatorID, &GMsrMgrLibRefNum);
		if (status) 
		{
			StrPrintF (msg, "Msr SysLibLoad Err: x%x", status);
			FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
			return (status);
		}	
	}
	
	//___Now open the puppy & send the Configurator settings
	status = MsrOpen (GMsrMgrLibRefNum, &versionNum, &libNum);
	if (status)
	{
		StrPrintF (msg, "AppStart MsrOpen Err: x%x", status); //x8008
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );	
		return (status);
	}
	
	//___Here in lies the magic.  The Apex configurator tool allows
	//   us to dictate what the format of the returned data from the
	//   MSR device looks like.  We only need to do this once...	
	status = MsrSendSetting (GMsrMgrLibRefNum, user_MsrSetting);
	if (status)
	{
		StrPrintF (msg, "AppStart MsrSendSetting Err: x%x", status);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
		return (status);
	}
#endif	
	
	return 0;
}

/***********************************************************************
 *
 * FUNCTION:		AboutHandleEvent
 *
 * DESCRIPTION:	Event handler for the Inmage form.
 * 					Handles the frmOpenEvent, frmCloseEvent, and the 
 * 					ctlSelectEvent (for the OK button).
 *
 * PARAMETERS:		ev - pointer to the event information structure.
 *
 * RETURNED:		true - the event was handled by us.  false otherwise
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
Boolean ImageFormHandleEvent( EventPtr eventP )
{
	Boolean bHandled = false;	
	FormPtr frmP = FrmGetActiveForm();
	
	switch (eventP->eType)
	{	
		case frmLoadEvent:
			frmP = FrmInitForm (ImageForm);
			bHandled = true;
			break;

		case frmOpenEvent:	
			ImageFormOnInit();

			bHandled = true;
			break;

		case frmCloseEvent:
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			bHandled = true;
			break;
			
		case ctlSelectEvent:
			if (eventP->data.ctlSelect.controlID == ImageOKButton)
			{
				FrmGotoForm(MainForm);
				bHandled = true;
				break;
			}
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 *
 * FUNCTION:		AboutFormHandleEvent
 *
 * DESCRIPTION:	Event handler for the About form.
 * 					Handles the frmOpenEvent, frmCloseEvent, and the 
 * 					ctlSelectEvent (for the OK button).
 *
 * PARAMETERS:		ev - pointer to the event information structure.
 *
 * RETURNED:		true - the event was handled by us.  false otherwise
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
Boolean AboutFormHandleEvent( EventPtr eventP )
{
	Boolean bHandled = false;
   FormPtr 	frmP = FrmGetActiveForm();
	
	switch (eventP->eType)
	{
		case frmLoadEvent:
			frmP = FrmInitForm (AboutForm);
			bHandled = true;
			break;
	
		case frmOpenEvent:
			AboutFormOnInit();
			bHandled = true;
			break;
			
		case frmCloseEvent:
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			bHandled = true;
			break;
	
		case ctlSelectEvent:
			if (eventP->data.ctlSelect.controlID == AboutOKButton)
			{
				FrmGotoForm (MainForm);
				bHandled = true;
			}
			break;

		default:
			break;
	}
	
	return bHandled;
	

}
/***********************************************************************
 *
 * FUNCTION:		MainFormHandleEvent
 *
 * DESCRIPTION:	Handles processing of events for the Main Form.
 *
 * PARAMETERS:		event	- the most recent event.
 *
 * RETURNED:		True if the event is handled, false otherwise.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
static Boolean MainFormHandleEvent (EventPtr eventP)
{
	Boolean	bHandled = false;	
	FormPtr frmP = FrmGetActiveForm();
	
	switch( eventP->eType )
	{
		case menuEvent:
			MainFormHandleMenu (eventP->data.menu.itemID);
			bHandled = true;
			break;
						
		case frmLoadEvent:
			frmP = FrmInitForm (MainForm);
			bHandled = true;
			break;
			
		case frmOpenEvent:
			FrmDrawForm (frmP);
			
			if (GetSocket())	
			{
				SetFieldText (MainStatusField, "No Socket...", 39);
				bHandled = false;
				break;
			}
			else
			{
				SetFieldText (MainStatusField, "Good to go...", 39);	
				bHandled = true;
				break;
			}
			
		case frmCloseEvent:
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			bHandled = true;
			break;
			
		//___this event is generated when we have data from the MagStripe
		case keyDownEvent:		
			if ((eventP->data.keyDown.chr==msrDataReadyKey))  
			{						
				MsrReadMSRUnbuffer (GMsrMgrLibRefNum, theMagStripP);
				ParseDisplay();
				bHandled = true;
				break;
			}
		
		//___this event is generated by a button being pressed
		case ctlSelectEvent:
		{			
			switch (eventP->data.ctlEnter.controlID)
			{
				case MainSwipeButton:	// activate the MSR, display results
					
#ifdef Debug
					ParseDisplay();		// use the stub for input
#else
					SetFieldText (MainStatusField, "Enabled for Debug only..", 39);
#endif
					bHandled = true;
					break;
					
				case MainSubmitButton:	// send the data to the host
					if (SocketParms.mySocket) // make sure we have a socket
					{ 	
						if (DriverLookUp (&SocketParms))
							SetFieldText (MainStatusField, "LookUp Failed...", 39);
						else
						{
							SetFieldText (MainStatusField, "LookUp Complete...", 39);			
							FrmGotoForm (ImageForm);	
						}
					}
					else
						SetFieldText (MainStatusField, "No Socket...", 39);
						
					bHandled = true;					
					break;							
			}			
	   		break;
		}
		
		default:
			break;
	
	} //end switch event->eType
	
	return(bHandled);
}

/***********************************************************************
 *
 * FUNCTION:		ApplicationHandleEvent
 *
 * DESCRIPTION:	An event handler for this application.  Gives control 
 * 					    to the appropriate form (Main or About) by setting the
 * 					    newly-loaded form's event handler.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:		Nothing.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
static Boolean ApplicationHandleEvent (EventPtr event)
{
	Word formID;
	FormPtr frm;

	if (event->eType == frmLoadEvent)
	{
		// Load the form resource.
		formID = event->data.frmLoad.formID;
		frm = FrmInitForm (formID);
		FrmSetActiveForm (frm);		
		 
		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmDispatchEvent each time is receives an
		// event.
		switch (formID)
		{
			case MainForm:
				FrmSetEventHandler (frm, MainFormHandleEvent);
				break;
				
			case AboutForm:
				FrmSetEventHandler (frm, AboutFormHandleEvent );				
				break;
		
			case ImageForm:
				FrmSetEventHandler (frm, ImageFormHandleEvent );								
				break;
			
			default:
				break;

		}
		
		return (true);
	}
	return (false);
}


/***********************************************************************
 *
 * FUNCTION:		EventLoop
 *
 * DESCRIPTION:	A simple loop that obtains events from the EventManager
 * and passes them on to various applications and	system event handlers
 * before passing them on to FrmHandleEvent for default processing.
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		Nothing.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
static void EventLoop(void)
{
	static EventType	event;
	Word error;
		
	do
	{
		// Get the next available event.
		EvtGetEvent(&event, /*5*/ evtWaitForever);

		// Give the system a chance to handle the event.
	  	if( !SysHandleEvent (&event))
			if( !MenuHandleEvent (CurrentMenu, &event, &error))
				// Give the application a chance to handle the event.
				if( !ApplicationHandleEvent(&event) )
					// Let the form object provide default handling of the event.
 					FrmDispatchEvent(&event);
	} 
	while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:		PilotMain
 *
 * DESCRIPTION:	This function is the equivalent of a main() function
 *						in standard C.  It is called by the Emulator to begin
 *						execution of this application.
 *
 * PARAMETERS:		cmd - command specifying how to launch the application.
 *						cmdPBP - parameter block for the command.
 *						launchFlags - flags used to configure the launch.			
 *
 * RETURNED:		Any applicable error code.
 *
 ***********************************************************************/
DWord PilotMain (Word cmd, Ptr cmdPBP, Word launchFlags)
{	
	Err error;	

	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) 
		return (error);

	switch (cmd)
	{
		case sysAppLaunchCmdNormalLaunch:
			error = StartApplication();
			if (error) 
				return error;
			
			EventLoop();
		
			StopApplication();
			
		default:
			break;

	}
	
	return 0;

}


