/************************************************************************
* COPYRIGHT:   Copyright  ©  1999, 2000, & 2001 Symbol Technologies, Inc. 
*
* FILE:        FindMdb.c
*
* SYSTEM:      Symbol barcode scanner for Model 1740 from Symbol.
* 
* MODULE:      Spectrum24 Scan Enabled lookup program.
*              
* DESCRIPTION: This is the client program that works in conjunction with
* MdbHost.exe.  Both are hard coded to use TCPIP port 1950. The host 
* IP address is set in MainFormOnInit().  When activated the client
* gets a socket and expects to see the MdbHost.exe program active.  The
* user then scans a barcode from the server\MdbHost\BARCODES.doc file.
* The barcode is sent to the MdbHost program and waits for a reply of
* 120 bytes from the host.  When the reply arrives, we parse it into 3
* lines and display the result to the main form. When the user switches
* the application, the socket is closed.
* 
* REVISION HISTORY:
*         Name   Date       Description
*         -----  ---------- -----------
*         dcat   06/14/1999 Initial Release
*             
*************************************************************************/
#include "Pilot.h"				// all the system toolbox headers
#include <Menu.h>
#include <Field.h>
#include <unix_stdio.h>			// sprintf needs this
#include <string.h>
#include "S24API.h"
#include "ScanMgrDef.h"			// Scan Manager constant definitions
#include "ScanMgrStruct.h"		// Scan Manager structure definitions 
#include "ScanMgr.h"	 		// Scan Manager API function definitions
#include "FindMdbRsc.h"			// application resource defines
#include "Utils.h"				// miscellaneous utility functions for this app

/***********************************************************************
 * Prototypes  & Globals for internal functions
 **********************************************************************/
void OnAbout(void);
Boolean OnDecoderData(void);
Boolean AboutHandleEvent( EventPtr ev );
static void MainFormHandleMenu( Word menuSel );
static MenuBarPtr	CurrentMenu;

int		mySocket = 0;			//___returned by Connect()
char	msg[42];					//___general buffer for messages
char	recv_buf[120];		//___alocated for Host replies
char	szBarCode[32];		//___set in OnDecoderData()
char	host[32];					//___hardcoded in MainFormOnInit 
int 	port;							//___hardcoded in MainFormOnInit
UInt	TimeOut = 0;			//___value set in Connect 

int   g_Running = false;//___set to true in StartApplication

// Define the minimum OS version we support
#define ourMinVersion	sysMakeROMVersion(3,0,0,sysROMStageRelease,0)

/***********************************************************************
 *
 * FUNCTION:	OnDecoderData
 *
 * DESCRIPTION:	Called when the app receives a scanDecodeEvent, which
 * signals that a decode operation has been completed.  Calls the Scan
 * Manager function "ScanGetDecodedData" to get the scan data and 
 * barcode type from the last scan.  Fills in the controls on the main 
 * form that display the information.
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		zero, always (not currently used)
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
Boolean OnDecoderData(void) 
{
  //FieldPtr       fld;
  //FieldAttrType   attr;
  
  MESSAGE decodeDataMsg;
	int status = ScanGetDecodedData( &decodeDataMsg );
	FormPtr pForm = FrmGetActiveForm();
		
	// if we successfully got the decoded data from the API
	if( status == STATUS_OK ) 
	{
		// Check to see if this is No Read (NR).  If so, handle here.
		if( StrNCompare( (char *)decodeDataMsg.data, "NR", 2 ) == 0 )
		{
			strcpy (szBarCode,"No Decode");					
			SetFieldText( MainItemCodeField, "No Decode", 39, true );
		  	FrmSetFocus(pForm, FrmGetObjectIndex(pForm,MainISBNField));		  
		}
		else
		{
			strcpy (szBarCode,(char *)&decodeDataMsg.data[0]);
			SetFieldText( MainItemCodeField,(char *)&decodeDataMsg.data[0], 39, true);		
			FrmSetFocus(pForm, FrmGetObjectIndex(pForm,MainISBNField));	 
		}
		
		SetFieldText(MainStatusField, "Barcode processed...",39, true);
	}
		
	return(0);	    			

}

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
 * FUNCTION:     MainFormOnInit
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
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
static void MainFormOnInit()
{
	int nRtnCode;
	FormPtr pForm = FrmGetActiveForm();

	if( pForm )
	{
		FrmDrawForm( pForm );
		
		FldSetInsertionPoint(GetObjectPtr(MainItemCodeField),0);
		FrmSetFocus(pForm,FrmGetObjectIndex(pForm,MainItemCodeField));
		FldGrabFocus(GetObjectPtr(MainItemCodeField));
		
		if (g_Running)	// we got here from the about form
			return;

		//___open the NetLib interface
 		nRtnCode =CheckForNetwork();
		if (nRtnCode)
		{
			SetFieldText (MainStatusField, "Can't open NetLib", 39, true);
		}
		else //___connect to our host, hard-coded ip_addr & port
		{
			nRtnCode = Connect ("157.235.93.2",1950); 
			if (nRtnCode < 1)
			{		
				SetFieldText (MainStatusField,"Can't connect to host,", 39, true);
				SetFieldText (MainTitleField,"check MainFormOnInit() ", 39, true);
				SetFieldText (MainDescField,"values of host & port", 39, true);
			}
			else
			{
				SetFieldText (MainStatusField, "Connect complete...", 39, true);			
				mySocket = nRtnCode;
				g_Running = true;
			}
		}
	}
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
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
static void AboutOnInit( void )
{
	char chrDecoderVersion[MAX_PACKET_LENGTH];
	char chrScanMgrVersion[20];
	char chrRFDriverVersion[20];
		
	FormPtr pForm = FrmGetActiveForm();

	// initialize text in all the version string fields
	ScanGetDecoderVersion (chrDecoderVersion, MAX_PACKET_LENGTH);
	SetFieldText(AboutDecoderField, chrDecoderVersion, MAX_PACKET_LENGTH, false);
	
	ScanGetScanManagerVersion (chrScanMgrVersion, 20);
	SetFieldText(AboutScanManagerField, chrScanMgrVersion, 20, false);
	
	S24GetDriverVersion (chrRFDriverVersion, 20);
	SetFieldText(AboutRFDriverField, chrRFDriverVersion, 20, false);
	
	FrmDrawForm( pForm );	

}

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
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
void OnAbout(void)
{

	FrmGotoForm( AboutForm );
	
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
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
void MainFormHandleMenu( Word menuSel )
{
	switch( menuSel )
	{
		case OptionsAbout:
			OnAbout();
			break;
	}
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
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
Boolean AboutHandleEvent( EventPtr eventP )
{
	Boolean bHandled = false;
	FormPtr 	frmP = FrmGetActiveForm();
	
	switch(eventP->eType)
	{
		case frmLoadEvent:
			frmP = FrmInitForm (AboutForm);
			bHandled = true;
			break;

		case frmOpenEvent:
			AboutOnInit();
			bHandled = true;
			break;
			
		case frmCloseEvent:
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			bHandled = true;
			break;
			
		case ctlSelectEvent:
			if( eventP->data.ctlSelect.controlID == AboutOKButton )
			{
				ScanCmdScanEnable(); 
				FrmGotoForm(MainForm);
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
 * DESCRIPTION:	Handles processing of events for the Main Form.  The 
 * following events are handled:
 * 						frmOpenEvent and menuEvent - standard handling
 * 						scanDecodeEvent - indicates that a scan was completed
 * 						scanBatteryErrorEvent - indicates batteries too low to scan
 * 						ctlSelectEvent - for Scan button on the main form
 *
 * PARAMETERS:		event	- the most recent event.
 *
 * RETURNED:		True if the event is handled, false otherwise.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr event)
{
	Boolean		bHandled = false;
	FormPtr frmP = FrmGetActiveForm();	
	
	switch( event->eType )
	{
		case menuEvent:
			MainFormHandleMenu(event->data.menu.itemID);
			bHandled = true;
			break;
			
		case frmLoadEvent:
			frmP = FrmInitForm (MainForm);
			bHandled = true;
			break;
			
		case frmOpenEvent:
			MainFormOnInit();
			bHandled = true;
			break;
			
		case frmCloseEvent:
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			bHandled = true;
			break;

		case scanDecodeEvent:	// A decode has been performed.  
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
			// Submit Button, the only one on this form
			if (event->data.ctlEnter.controlID == MainSubmitButton)
			{
				LookUp (szBarCode);
				FldSetInsertionPoint(GetObjectPtr(MainItemCodeField),0);
				FrmSetFocus(frmP, FrmGetObjectIndex(frmP,MainItemCodeField));
				FldGrabFocus(GetObjectPtr(MainItemCodeField));
				bHandled = true;
			}
	   	break;
		}
		
	} //end switch

	return(bHandled);
}

/***********************************************************************
 *
 * FUNCTION:		ApplicationHandleEvent
 *
 * DESCRIPTION:	An event handler for this application.  Gives control 
 *    to the appropriate form (Main or About) by setting the
 *    newly-loaded form's event handler.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:		Nothing.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
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
 * FUNCTION: 		StartApplication
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
 * RETURNED: 		Nothing.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
static Err StartApplication(void)
{
	int i, status;
	BarType allTypes[] = {
				barCODE39, barUPCA, barUPCE, barUPCE1, barEAN13, barEAN8, barD25, barI2OF5, 
				barCODABAR, barCODE128, barCODE93, barTRIOPTIC39, barUCC_EAN128,
				barMSI_PLESSEY, barUPCE1, barBOOKLAND_EAN, barISBT128, barCOUPON};

	// make sure we're running on a Spectrum24 unit.
	if (!PalmIsSPT1740())
	{
		FrmAlert (NotScanEnabledAlert);            
		return 2;
	}

	status = ScanOpenDecoder();
	if (status != STATUS_OK)
	{
		sprintf (msg, "ScanOpenDecoder error: %d", status);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL);
		return 3;
	}
	
	//ScanSetTriggeringModes (HOST); // allow software-triggered scans 

	// we want to be able to scan everything we can get our hands on
	for (i = 0; i < sizeof(allTypes) / sizeof(*allTypes); i++)
			ScanSetBarcodeEnabled (allTypes[i], true);	// enable a barcode type 
			
	ScanSetBeepDuration (decodeDuration, 10);
	//ScanSetBeepAfterGoodDecode (false);
	 		
	status = ScanCmdSendParams (No_Beep); // send all the accumulated settings to scanner
	if (status != STATUS_OK)
	{
		sprintf (msg, "ScanCmdSendParams error: %d", status);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL);
		return 4;
	}	
	
	status = ScanCmdScanEnable (); // send all the accumulated settings to scanner
	if (status != STATUS_OK)
	{
		sprintf (msg, "ScanCmdScanEnable error: %d", status);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL);
		return 5;
	}
	
	return 0;

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
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
static void StopApplication(void)
{
  SetFieldText (MainStatusField, "ShutDown in Progress...", 39, true);	
			
	if (mySocket)
	{
		DisConnect();	// close socket & NetLib
		mySocket = 0;
	}
		
	ScanCmdParamDefaults ();
	ScanCmdScanDisable (); 	
	ScanCloseDecoder ();
	
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
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
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
	if (error) 
		return (error);

	switch (cmd)
		{
		case sysAppLaunchCmdNormalLaunch:
			error = StartApplication();			
			if (error) 
				return error;
				
			FrmGotoForm(MainForm);
			EventLoop();            
			StopApplication();          
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
