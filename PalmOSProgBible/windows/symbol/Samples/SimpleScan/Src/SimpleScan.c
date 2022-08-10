/************************************************************************
* COPYRIGHT:   Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE:        SimpleScan.c
*
* SYSTEM:      Symbol barcode scanner for Palm III.
* 
* MODULE:      Simple Scan Demo Application
*              
* DESCRIPTION: Contains application support functions, including main form event handlers
*              and event loop for the SimpleScan sample application.
*					
* FUNCTIONS:   StartApplication
* 					StopApplication
*					PilotMain
*					MainFormHandleEvent
*					MainFormOnInit
*					MainFormHandleMenu
*					EventLoop
*					ApplicationHandleEvent
*					OnDecoderData
* 					OnAbout
*					AboutHandleEvent
* 					AboutOnInit
*
* HISTORY:     5/25/98    KEF   Created
*			   3/17/99	  CFS   Set bHandled to true so that the default error processing will 
*								not be used.  The app processing is used (line 175).
*              ...
*************************************************************************/
#include "Pilot.h"				// all the system toolbox headers
#include <Menu.h>

#include "ScanMgrDef.h"				// Scan Manager constant definitions
#include "ScanMgrStruct.h"			// Scan Manager structure definitions 
#include "ScanMgr.h"	 				// Scan Manager API function definitions

#include "SimpleScanRsc.h"			// application resource defines
#include "Utils.h"					// miscellaneous utility functions for this app

/***********************************************************************
 * Prototypes for internal functions
 **********************************************************************/
static void StartApplication(void);
static void StopApplication(void);
static Boolean MainFormHandleEvent(EventPtr event);
static void MainFormHandleMenu( Word menuSel );
static void EventLoop(void);
static MenuBarPtr	CurrentMenu;
static Boolean OnDecoderData();
static void MainFormOnInit();
static Boolean ApplicationHandleEvent (EventPtr event);
static void OnAbout();
static Boolean AboutHandleEvent( EventPtr ev );
static void AboutOnInit( void );


/***********************************************************************
 *
 * FUNCTION: 		StartApplication
 *
 * DESCRIPTION: 	This routine sets up the initial state of the application.
 * 					Set the Main Form as the initial form to display.
 * 					Checks to make sure we're running on Symbol hardware, then 
 * 					calls ScanOpenDecoder to initialize the Scan Manager.  
 * 					If successful, then we proceed with setting decoder 
 * 					parameters that we care about for this application.
 * 					ScanCmdSendParams is called to send our params to the decoder.
 *
 * PARAMETERS: 	None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void StartApplication(void)
{
	Err error;			
				
	// Call up the main form.
	FrmGotoForm( MainForm );
	
	// Make sure we're running on Symbol hardware before attempting to 
	// open the decoder or call any other Scan Manager functions.
	if (ScanIsPalmSymbolUnit())
	{
		// Now, open the scan manager library	
		error = ScanOpenDecoder();

		if (!error)
		{
			// Set decoder parameters we care about...
			ScanCmdScanEnable(); 				// enable scanning
			ScanSetTriggeringModes( HOST ); 	// allow software-triggered scans (from our Scan button)
			ScanSetBarcodeEnabled( barUPCA, true ); 	// Enable any barcodes to be scanned
			ScanSetBarcodeEnabled( barUPCE, true );
			ScanSetBarcodeEnabled( barUPCE1, true );
			ScanSetBarcodeEnabled( barEAN13, true );
			ScanSetBarcodeEnabled( barEAN8, true );
			ScanSetBarcodeEnabled( barBOOKLAND_EAN, true);
			ScanSetBarcodeEnabled( barCOUPON, true);

			// We've set our parameters...
			// Now call "ScanCmdSendParams" to send them to the decoder
			ScanCmdSendParams( No_Beep); 
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:     StopApplication
 *
 * DESCRIPTION:  This routine does any cleanup required, including shutting down
 * 				  the decoder and Scan Manager shared library.
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Nothing.
 *
 ***********************************************************************/
static void StopApplication(void)
{
	if (ScanIsPalmSymbolUnit())
	{
		// Disable the scanner and Close Scan Manager shared library
		ScanCmdScanDisable(); 			
		ScanCloseDecoder(); 	
	}
}

/***********************************************************************
 *
 * FUNCTION:		MainFormHandleEvent
 *
 * DESCRIPTION:	Handles processing of events for the Main Form.
 * 					The following events are handled:
 * 						frmOpenEvent and menuEvent - standard handling
 * 						scanDecodeEvent - indicates that a scan was completed
 * 						scanBatteryErrorEvent - indicates batteries too low to scan
 * 						ctlSelectEvent - for Scan button on the main form
 *
 * PARAMETERS:		event	- the most recent event.
 *
 * RETURNED:		True if the event is handled, false otherwise.
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr event)
{
	Boolean		bHandled = false;
	
	switch( event->eType )
	{
		case frmOpenEvent:
			MainFormOnInit();
			bHandled = true;
			break;
			
		case menuEvent:
			MainFormHandleMenu(event->data.menu.itemID);
			bHandled = true;
			break;

		case scanDecodeEvent:
			// A decode has been performed.  
			// Use the decoder API to get the decoder data into our memory
			OnDecoderData(); 
			bHandled = true;
			break;
								
		case scanBatteryErrorEvent:
		{
			Char szTemp[10];
			StrIToA( szTemp, ((ScanEventPtr)event)->scanData.batteryError.batteryLevel );
			FrmCustomAlert( BatteryErrorAlert, szTemp, NULL, NULL );
			bHandled = true;
			break;
		}
		
		case ctlSelectEvent:
		{
			if (ScanIsPalmSymbolUnit())
			{
				// Scan Button
				if (event->data.ctlEnter.controlID == MainSCANButton)
				{
					ScanCmdStartDecode();
					bHandled = true;
				}
			}
	   	break;
		}
	} //end switch

	return(bHandled);
}


/***********************************************************************
 *
 * FUNCTION:     MainFormOnInit
 *
 * DESCRIPTION:  This routine sets up the initial state of the main form
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Nothing.
 *
 ***********************************************************************/
static void MainFormOnInit()
{
	FormPtr pForm = FrmGetActiveForm();
	if( pForm )
	{
		// initialize the barcode type and barcode data fields
		SetFieldText( MainBarTypeField, "No Data", 20, false );
		SetFieldText( MainScandataField, "No Data", 80, false );
		FrmDrawForm( pForm );
	}
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
 ***********************************************************************/
void MainFormHandleMenu( Word menuSel )
{
	switch( menuSel )
	{
		// Options menu
		case OptionsResetDefaults:
			if (ScanIsPalmSymbolUnit())
				ScanCmdParamDefaults();
			break;
			
		case OptionsAbout:
			OnAbout();
			break;
	}
}


/***********************************************************************
 *
 * FUNCTION:		EventLoop
 *
 * DESCRIPTION:	A simple loop that obtains events from the Event
 *						Manager and passes them on to various applications and
 *						system event handlers before passing them on to
 *						FrmHandleEvent for default processing.
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		Nothing.
 *
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
 * FUNCTION:		ApplicationHandleEvent
 *
 * DESCRIPTION:	An event handler for this application.  Gives control 
 * 					to the appropriate form (Main or About) by setting the
 * 					newly-loaded form's event handler.
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		Nothing.
 *
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
				FrmSetEventHandler (frm, AboutHandleEvent );
				break;
		}
		return (true);
	}
	return (false);
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
DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
	// Check for a normal launch.
	if (cmd == sysAppLaunchCmdNormalLaunch)
	{
		Err error = STATUS_OK;
		
		// Set up Scan Manager and the initial (Main) form.
		StartApplication();
		
		// Start up the event loop.
		EventLoop();

		// Close down Scan Manager, decoder
		StopApplication();
	}
	
	return(0);
}

/***********************************************************************
 *
 * FUNCTION:	OnDecoderData
 *
 * DESCRIPTION:	Called when the app receives a scanDecodeEvent, which
 * 					signals that a decode operation has been completed.
 * 					Calls the Scan Manager function "ScanGetDecodedData" 
 * 					to get the scan data and barcode type from the last 
 * 					scan.  Fills in the controls on the main form that 
 * 					display this information.
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		zero, always (not currently used)
 *
 ***********************************************************************/
Boolean OnDecoderData() 
{
	static Char BarTypeStr[80]=" ";
	MESSAGE decodeDataMsg;
	
	int refNum = 0;

	int status = ScanGetDecodedData( &decodeDataMsg );
	
	if( status == STATUS_OK ) // if we successfully got the decode data from the API...
	{
		// Check to see if this is No Read (NR).  If so, handle here.
		if( StrNCompare( (char *)decodeDataMsg.data, "NR", 2 ) == 0 )
		{
			SetFieldText( MainBarTypeField, "No Scan", 30, true );
			SetFieldText( MainScandataField, "No Scan", 79, true );
		}
		else
		{
			// Get string representation of the barcode type
			ScanGetBarTypeStr( decodeDataMsg.type, BarTypeStr, 30 );	// in Utils.c
			SetFieldText( MainBarTypeField, BarTypeStr, 30, true );

			//Place the scandata into the field and display
			SetFieldText( MainScandataField, (char *)&decodeDataMsg.data[0], 79, true );
		}
	}
		
	return(0);	    			
}


/***********************************************************************
   About Form
************************************************************************/

/***********************************************************************
 *
 * FUNCTION:		OnAbout
 *
 * DESCRIPTION:	Changes the active form to the About Form
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		Nothing
 *
 ***********************************************************************/
void OnAbout()
{
	FrmGotoForm( AboutForm );
}

/***********************************************************************
 *
 * FUNCTION:		AboutOnInit
 *
 * DESCRIPTION:	Initialize controls on the about form, after getting 
 * 					the version strings from the Scan Manager API.
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		Nothing
 *
 ***********************************************************************/
static void AboutOnInit( void )
{
	char chrDecoderVersion[MAX_PACKET_LENGTH];
	char chrPortDriverVersion[20];
	char chrScanMgrVersion[20];
	FormPtr pForm = FrmGetActiveForm();

	if (ScanIsPalmSymbolUnit())
	{
		// initialize text in all the version string fields
		ScanGetDecoderVersion( chrDecoderVersion, MAX_PACKET_LENGTH);
		SetFieldText(AboutDecoderField, chrDecoderVersion, MAX_PACKET_LENGTH, false);

		ScanGetScanPortDriverVersion(chrPortDriverVersion, 20 );
		SetFieldText(AboutPortDriverField, chrPortDriverVersion, 20, false);
		
		ScanGetScanManagerVersion(chrScanMgrVersion, 20 );
		SetFieldText(AboutScanManagerField, chrScanMgrVersion, 20, false);
	}
	
	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION:		AboutHandleEvent
 *
 * DESCRIPTION:	Event handler for the About form.
 * 					Handles the frmOpenEvent, frmCloseEvent, and the 
 * 					ctlSelectEvent (for the OK button).
 *
 * PARAMETERS:		ev - pointer to the event information structure.
 *
 * RETURNED:		true - the event was handled by us.  false otherwise
 *
 ***********************************************************************/
Boolean AboutHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == AboutOKButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			break;

		case frmOpenEvent:
			AboutOnInit();
			bHandled = true;
			break;
			
		case frmCloseEvent:
			FreeFieldHandle(AboutDecoderField);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}


