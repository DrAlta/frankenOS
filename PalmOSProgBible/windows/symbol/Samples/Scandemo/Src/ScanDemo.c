/************************************************************************
* COPYRIGHT: 	Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE: 		ScanDemo.c
*
* SYSTEM: 		Symbol barcode scanner for Palm III.
* 
* MODULE: 		Scan Demo Application
*              
* DESCRIPTION: 	Contains application support functions, including main form event handlers
* 				and event loop.
*					
* FUNCTIONS: 	StartApplication
* 				StopApplication
*				MainFormHandleEvent
*				MainFormOnInit
*				MainFormHandleMenu
*				EventLoop
*				ApplicationHandleEvent
*				PilotMain
*				OnDecoderData
*				OnBarCodeEnableCheckbox
*              
*
* HISTORY: 		4/13/98    SS   Created
*				4/01/99	   CFS  Removed Aim Mode option from the main menu 	
* 				...
*************************************************************************/
#include "Pilot.h"				// all the system toolbox headers
#include <Hardware.h>			// gives HwrBatteryLevel

#include "ScanMgrDef.h"			// scan manager constants
#include "ScanMgrStruct.h" 	// scan manager structures
#include "ScanMgr.h" 			// scan manager API functions

#include "ScanDemoRsc.h"		// application resource defines
#include "SetupDlgs.h"			// Setup dialogs for this app 
#include "Utils.h"				// Utilities for this app

/***********************************************************************
 * Prototypes for internal functions
 **********************************************************************/
static void StartApplication(void);
static void StopApplication(void);
static Boolean MainFormHandleEvent(EventPtr event);
static void MainFormHandleMenu( Word menuSel );
static void EventLoop(void);
static MenuBarPtr	CurrentMenu;
static Boolean OnDecoderData(); // GetSerialData();
static void MainFormOnInit();
static void UpdateMainForm(void);
static Boolean ApplicationHandleEvent (EventPtr event);
static void OnBarGroupEnableCheckbox( int CheckboxId);

static Boolean bOpenDecoderOK = false;

/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION: 	This routine sets up the initial state of the application.
 * 					First, it checks whether we are running on Palm/Symbol 
 * 					hardware.  If so, it opens the decoder.  If the decoder 
 * 					is opened successfully, a global flag is set, and scanning
 * 					is enabled.  Finally, we call up the apps main form.
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Nothing.
 *
 ***********************************************************************/
static void StartApplication(void)
{
	Err error;			

	// first check and be sure we're running on Palm/Symbol hardware
	if (ScanIsPalmSymbolUnit())
	{
		error = ScanOpenDecoder();	// open the scan manager library	
		if (!error)
		{
			bOpenDecoderOK = true; // set global variable that all is well
			ScanCmdScanEnable();	// Enable scanning
		}
		else 
			FrmAlert( OpenFailedAlert );
	}
	else
		FrmAlert( WrongHardwareAlert );
		
	// Initialize and draw the main form.
	FrmGotoForm( MainForm );
}

/***********************************************************************
 *
 * FUNCTION: 		StopApplication
 *
 * DESCRIPTION: 	This routine disables scanning and closes the decoder.
 * 					Calling ScanCloseDecoder is required of scan-aware apps.
 * 					
 * PARAMETERS: 	None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void StopApplication(void)
{
	// check to be sure that we earlier opened the decoder successfully 
	if (bOpenDecoderOK)
	{
		// Disable the scanner, and then close the decoder.
		ScanCmdScanDisable();
		
		ScanCloseDecoder();
	}
}


/***********************************************************************
 *
 * FUNCTION:		MainFormHandleEvent
 *
 * DESCRIPTION:	Handles processing of events for the main form.
 *
 * PARAMETERS:		event	- the most recent event.
 *
 * RETURNED:		True if the event is handled, false otherwise.
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr event)
{
	Boolean		bHandled = false;
//	Char szTemp[10];
//	UInt batteryVolts;	

	switch( event->eType )
	{
		case frmOpenEvent:
			MainFormOnInit();
			bHandled = true;
			break;
			
		case frmUpdateEvent:
			UpdateMainForm();
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
								
		case popSelectEvent:
		{
			if (event->data.popSelect.listID == MainTriggerModeListList)
			{
				int nTrigMode = 0;
				if (event->data.popSelect.selection == 0)
					nTrigMode = LEVEL;
				else if (event->data.popSelect.selection == 1)
					nTrigMode = PULSE;
				else if (event->data.popSelect.selection == 2)
					nTrigMode = HOST;
				else  // should not happen
					nTrigMode = LEVEL;
					
				ScanSetTriggeringModes( nTrigMode);
				ScanCmdSendParams( No_Beep);
			}
			break;
		}

		case ctlSelectEvent:
		{
			switch(event->data.ctlEnter.controlID)
			{
				// Scan Button
				case MainSCANButton:
					ScanCmdStartDecode();
				  	bHandled = true;
	   				break;
	   				
				// Stop Button
				case MainStopButton:
					ScanCmdStopDecode();
				  	bHandled = true;
	   				break;

				// Scan Enable Button
				case MainScanEnCheckbox:
				{
					ControlPtr pCtl;
					Boolean bEn;
					
					pCtl = GetObjectPtr( MainScanEnCheckbox );
					bEn = CtlGetValue( pCtl );
					if( bEn )
						ScanCmdScanEnable();
					else
						ScanCmdScanDisable();
					break;
				}
				
				
				// "LED" checkbox
				case 	MainLEDCheckbox:
				{
					ControlPtr pCtl;
					Boolean bEn; 
					pCtl = GetObjectPtr(MainLEDCheckbox);
					bEn = CtlGetValue( pCtl); 
					if (bEn) 
						ScanCmdLedOn();
					else 
						ScanCmdLedOff();
				}
				
				// checkboxes for the various groups of barcode types 
				case  MainUPCEnCheckbox:
				case  MainCODE128EnCheckbox:
				case  MainCODE39EnCheckbox: 
				case  MainCODE93EnCheckbox:
				case  MainI2OF5EnCheckbox:  
				case  MainD2OF5EnCheckbox:  
				case  MainCODABAREnCheckbox:
				case  MainMSIEnCheckbox:
					// enable/disable a group of barcode types for the selected checkbox
					OnBarGroupEnableCheckbox(event->data.ctlEnter.controlID);
					break;
				
				// the rest are buttons to launch setup dialogs 	
	   		case MainUPCButton:
	   			OnUPCSetup();
	   			break;
	   		case MainCODE128Button:
	   			OnCode128Setup();
	   			break;
				case MainCODE39Button:
					OnCode39Setup();
					break;
				case MainCODE93Button:
					OnCode93Setup();
					break;
				case MainCODABARButton:
					OnCodabarSetup();
					break;
				case MainMSIButton:
					OnMSIPlesseySetup();
					break;
				case MainD2OF5Button:
					OnD2of5Setup();
					break;
				case MainI2OF5Button:
					OnI2of5Setup();
					break;
			}  //end switch
		}
	}//end if

	// draw the battery level
//	batteryVolts = (HwrBatteryLevel() * 100) / 78;	
//	StrIToA( szTemp, batteryVolts);
//	WinDrawChars( szTemp, StrLen(szTemp), 13, 130);

	return(bHandled);
}


/***********************************************************************
 *
 * FUNCTION:		MainFormOnInit
 *
 * DESCRIPTION: 	Initialize the main form (in response to a frmOpenEvent)
 * 					Calls "UpdateMainForm" to initialize all the controls on
 * 					the main form.  Then it draws the form (FrmDrawForm).
 *
 * PARAMETERS:		None
 *
 * RETURNED:		Nothing
 *
 ***********************************************************************/
static void MainFormOnInit()
{
	FormPtr pForm = FrmGetActiveForm();
	if( pForm )
	{
		UpdateMainForm();

		FrmDrawForm( pForm );
	}
}

/***********************************************************************
 *
 * FUNCTION:		UpdateMainForm
 *
 * DESCRIPTION: 	Initializes all the controls on the main form.
 * 					Uses the Scan Manager API functions to get the current
 * 					values of all the decoder parameters to be displayed on
 * 					the main form.  Note that we check the return value of  
 * 					the first API function we call just to be sure that 
 * 					communications with the decoder are OK.  
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		Nothing.
 *
 ***********************************************************************/
void UpdateMainForm(void)
{
	ControlPtr pCtl;
	int nTrigMode;
	CharPtr label;
	ListPtr pList;
	Boolean bEnable;
	int nStatusCheck = STATUS_OK;
	
	// initialize the barcode type and barcode data fields
	SetFieldText( MainBarTypeField, "No Data", 20, false );
	SetFieldText( MainScandataField, "No Data", 80, false );

	// call one API function simply to make sure decoder communications are OK
	nStatusCheck = ScanGetBarcodeEnabled(barUPCA);
	if (nStatusCheck >= 0) 
	{			
		// since our test call did not generate a negative value, we know it's ok to proceed.
		
		// initialize the checkbox controls on the main screen
		pCtl = GetObjectPtr( MainUPCEnCheckbox );
		bEnable = ScanGetBarcodeEnabled(barUPCA) || ScanGetBarcodeEnabled(barUPCE) ||
			ScanGetBarcodeEnabled(barUPCE1) || ScanGetBarcodeEnabled(barEAN13) ||
			ScanGetBarcodeEnabled(barEAN8) || ScanGetBarcodeEnabled(barBOOKLAND_EAN);
		CtlSetValue( pCtl, bEnable);
		
		pCtl = GetObjectPtr( MainCODE128EnCheckbox );
		bEnable = ScanGetBarcodeEnabled( barISBT128 ) || 
						ScanGetBarcodeEnabled( barCODE128 ) ||
						ScanGetBarcodeEnabled( barUCC_EAN128 );
		CtlSetValue( pCtl, bEnable);
		
		pCtl = GetObjectPtr( MainCODE39EnCheckbox );
		bEnable = ScanGetBarcodeEnabled(barCODE39) || 
					ScanGetBarcodeEnabled(barTRIOPTIC39);
		CtlSetValue( pCtl, bEnable);
		
		pCtl = GetObjectPtr( MainCODE93EnCheckbox );
		bEnable = ScanGetBarcodeEnabled( barCODE93 );
		CtlSetValue( pCtl, bEnable);
		
		pCtl = GetObjectPtr( MainI2OF5EnCheckbox );
		bEnable = ScanGetBarcodeEnabled( barI2OF5 );
		CtlSetValue( pCtl, bEnable);
		
		pCtl = GetObjectPtr( MainD2OF5EnCheckbox );
		bEnable = ScanGetBarcodeEnabled( barD25 );
		CtlSetValue( pCtl, bEnable);
		
		pCtl = GetObjectPtr( MainCODABAREnCheckbox );
		bEnable = ScanGetBarcodeEnabled( barCODABAR );
		CtlSetValue( pCtl, bEnable);

		pCtl = GetObjectPtr( MainMSIEnCheckbox );
		bEnable = ScanGetBarcodeEnabled( barMSI_PLESSEY );
		CtlSetValue( pCtl, bEnable);

		nTrigMode = ScanGetTriggeringModes();
		switch (nTrigMode)
		{
			case LEVEL:
				nTrigMode = 0; // first item in list
				break;
			case PULSE:
				nTrigMode = 1; // second item in list
				break;
			case HOST:
				nTrigMode = 2; // third item in list
				break;
			default:
				nTrigMode = 0; // default to level mode
				break;
		}	
		pList = GetObjectPtr( MainTriggerModeListList );
		LstSetSelection (pList, nTrigMode);
		label = LstGetSelectionText (pList, nTrigMode);
		pCtl = GetObjectPtr (MainTriggerModePopTriggerPopTrigger);
		CtlSetLabel (pCtl, label);
		
		pCtl = GetObjectPtr(MainScanEnCheckbox);
		bEnable = ScanGetScanEnabled();
		CtlSetValue( pCtl, bEnable);
		
		pCtl = GetObjectPtr(MainLEDCheckbox);
		bEnable = ScanGetLedState();
		CtlSetValue( pCtl, bEnable);
	}
}


/***********************************************************************
 *
 * FUNCTION:		MainFormHandleMenu
 *
 * DESCRIPTION: 	Handles menu events for the main form (Options menu,
 * 					Beep menu, and Setup menu).  For most menu choices 
 * 					another function is called to display a different form.
 * 
 * 					Called by MainFormHandleEvent for any menuEvent received.
 *
 * PARAMETERS:		menuSel - the menu item that was selected
 *
 * RETURNED:		Nothing
 *
 ***********************************************************************/
void MainFormHandleMenu( Word menuSel )
{
	switch( menuSel )
	{
	// Options menu
		case OptionsResetDefaults:
			ScanCmdParamDefaults();
			FrmUpdateForm( MainForm, frmRedrawUpdateCode); // force repaint of form
			break;
			
		case OptionsAbout:
			OnAbout();
			break;
			
	// Beep menu
		case BeepFrequency:
			OnBeepFrequencySetup();
			break;
		
		case BeepDuration:
			OnBeepDurationSetup();
			break;
		
		case BeepPatterns:
			OnBeepTest();
			break;
				
	// Setup menu
		case SetupHardware:
			OnHardwareSetup();
			break;
			
		case SetupCodeFormat:
			OnCodeFormatSetup();
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
		EvtGetEvent(&event, evtWaitForever);

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
 * DESCRIPTION:	An event handler routine that handles events at the application
 * 					level, particularly frmLoadEvents which control the GUI flow by 
 * 					setting the appropriate form handler into effect.  Also handles 
 * 					the scanBatteryEvent, should it happen anywhere in the app.
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
	
	if (event->eType == scanBatteryErrorEvent)
	{
//		Char szTemp[10];
//		StrIToA( szTemp, ((ScanEventPtr)event)->scanData.batteryError.batteryLevel );
//		FrmCustomAlert( BatteryErrorAlert, szTemp, NULL, NULL );
//		return (true);
// We changed this so that ScanDemo doesn't handle the battery event.
// Let the default handler take care of it...
		return (false);
	}
	else if (event->eType == frmLoadEvent)
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
		
			case HardwareForm:
				FrmSetEventHandler (frm, HardwareSetupHandleEvent);
				break;
					
			case UPCForm:
				FrmSetEventHandler (frm, UPCSetupHandleEvent);
				break;
					
			case UPCMoreForm:
				FrmSetEventHandler (frm, UPCMoreSetupHandleEvent);
				break;
					
			case Code128Form:
				FrmSetEventHandler (frm, Code128SetupHandleEvent);
				break;
					
			case Code39Form:
				FrmSetEventHandler (frm, Code39SetupHandleEvent);
				break;
					
			case MoreCode39Form:
				FrmSetEventHandler (frm, MoreCode39SetupHandleEvent);
				break;
					
			case Code93Form:
				FrmSetEventHandler (frm, Code93SetupHandleEvent);
				break;
					
			case I2of5Form:
				FrmSetEventHandler (frm, I2of5SetupHandleEvent);
				break;
					
			case D2of5Form:
				FrmSetEventHandler (frm, D2of5SetupHandleEvent);
				break;
					
			case CodabarForm:
				FrmSetEventHandler (frm, CodabarSetupHandleEvent);
				break;
					
			case MSIPlesseyForm:
				FrmSetEventHandler (frm, MSIPlesseySetupHandleEvent);
				break;
					
			case CodeFormatForm:
				FrmSetEventHandler (frm, CodeFormatSetupHandleEvent);
				break;

			case BeepFrequenciesForm:
				FrmSetEventHandler (frm, BeepFrequencySetupHandleEvent );
				break;
			
			case BeepDurationsForm:
				FrmSetEventHandler (frm, BeepDurationSetupHandleEvent );
				break;

			case BeepTestForm:
				FrmSetEventHandler (frm, BeepTestHandleEvent );
				break;
			
			case AboutForm:
				FrmSetEventHandler (frm, AboutHandleEvent );
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
 * FUNCTION:		PilotMain
 *
 * DESCRIPTION:	This function is the equivalent of a main() function
 *						in standard C.  It is called by the Emulator to begin
 *						execution of this application.
 *
 * 					We call StartApplication to do any initial processing.
 * 					If the decoder was successfully initialized, we proceed
 * 					with the application.  Otherwise we exit right away, 
 * 					after calling StopApplication to do its cleanup job.
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
		// Open the decoder and specify our initial (main) form.
		StartApplication();

		// only continue if the decoder was successfully opened
		if (bOpenDecoderOK)		
			EventLoop(); // Start up the event loop.

		StopApplication();
	}
	
	return(0);
}

/***********************************************************************
 *
 * FUNCTION: 		OnDecoderEvent
 *
 * DESCRIPTION:	Called when the app receives a scanDecodeEvent, which
 * 					signals that a decode operation has been completed.
 * 					Calls the Scan Manager function "ScanGetDecodedData" 
 * 					to get the scan data and barcode type from the last 
 * 					scan.  Fills in the controls on the main form that 
 * 					display this information.
 * 					
 *
 * PARAMETERS:		event	- the most recent event.
 *
 * RETURNED:		True if the event is handled, false otherwise.
 *
 ***********************************************************************/
Boolean OnDecoderData()
{
	static Char BarTypeStr[80]=" ";
	MESSAGE decodeDataMsg;
	
	int status = ScanGetDecodedData( &decodeDataMsg );
	
	if( status == STATUS_OK ) // if we successfully got the decode data from the API...
	{
		// Check to see if this scan was a "No Data Read" (indicated by type of zero).
		if( decodeDataMsg.type == 0)
		{
			SetFieldText( MainBarTypeField, "No Scan", 30, true );
			SetFieldText( MainScandataField, "No Scan", 79, true );
		}
		else
		{
			// call a function to translate barcoce type into a string, and display it
			ScanGetBarTypeStr( decodeDataMsg.type, BarTypeStr, 30 );	// in Utils.c
			SetFieldText( MainBarTypeField, BarTypeStr, 30, true );

			// Place the barcode data into the field and display
			SetFieldText( MainScandataField, (char *)&decodeDataMsg.data[0], 79, true );
		}
	}

	return(0);	    			
}

/***********************************************************************
 *
 * FUNCTION:		OnBarGroupEnableCheckbox
 *
 * DESCRIPTION: 	This function is called when one of the barcode-type 
 * 					checkboxes on the main form is selected.  We enable or
 * 					disable the entire group of barcode types covered by 
 * 					that checkbox.  
 *
 * PARAMETERS:		CheckboxId - Specifies which checkbox was selected.
 *
 * RETURNED:		Nothing
 *
 ***********************************************************************/
void OnBarGroupEnableCheckbox( int CheckboxId)
{
	ControlPtr pCtl;
	Boolean bEnable = false;
	pCtl = GetObjectPtr( CheckboxId );
	if (pCtl)
	{
		bEnable = CtlGetValue( pCtl );
	
		switch (CheckboxId)
		{
			case  MainUPCEnCheckbox:
				ScanSetBarcodeEnabled( barUPCA, bEnable);
				ScanSetBarcodeEnabled( barUPCE, bEnable);
				ScanSetBarcodeEnabled( barUPCE1, bEnable);
				ScanSetBarcodeEnabled( barEAN13, bEnable);
				ScanSetBarcodeEnabled( barEAN8, bEnable);
				ScanSetBarcodeEnabled( barBOOKLAND_EAN, bEnable);
				break;
			case  MainCODE128EnCheckbox:
				ScanSetBarcodeEnabled( barISBT128, bEnable);
				ScanSetBarcodeEnabled( barCODE128, bEnable);
				ScanSetBarcodeEnabled( barUCC_EAN128, bEnable);
				break;
			case  MainCODE39EnCheckbox: 
				ScanSetBarcodeEnabled( barCODE39, bEnable);
				ScanSetBarcodeEnabled( barTRIOPTIC39, bEnable);
				break;
			case  MainCODE93EnCheckbox:
				ScanSetBarcodeEnabled( barCODE93, bEnable);
				break;
			case  MainI2OF5EnCheckbox:  
				ScanSetBarcodeEnabled( barI2OF5, bEnable);
				break;
			case  MainD2OF5EnCheckbox:  
				ScanSetBarcodeEnabled( barD25, bEnable);
				break;
			case  MainCODABAREnCheckbox:
				ScanSetBarcodeEnabled( barCODABAR, bEnable);
				break;
			case  MainMSIEnCheckbox:
				ScanSetBarcodeEnabled( barMSI_PLESSEY, bEnable);
				break;
			default:
				break;
		} // end switch
		
		ScanCmdSendParams( No_Beep);
		
	} // end if (pCtl)
}

