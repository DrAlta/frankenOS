/***********************************************************************
 *
 Copyright (c) 1999 Symbol Technologies, Inc.  
 All rights reserved.
   
 
 *************************************************************************
 *
 * PROJECT:  Palm III MSR
 * FILE:     MSR.C
 * AUTHOR:   H.Z.: Jan. 20, 1999
 *
 * DECLARER: Starter
 *
 * DESCRIPTION:
 *	  
 *
 **********************************************************************/
#define NEW_SERIAL_MANAGER

#include <Pilot.h>

#ifdef NEW_SERIAL_MANAGER
#include <StringMgr.h>
#else
#include <Hardware.h>
#include <String.h>
#endif

#include <SysEvtMgr.h>

#include "MsrMgrLib.h"
#include "MSRRsc.h"

/***********************************************************************
 *
 *   Entry Points
 *
 ***********************************************************************/


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
 *   Global variables
 *
 ***********************************************************************/
// setting from configurator
#include "configurator.h"

static MenuBarPtr		CurrentMenu = NULL;	// pointer to current menu

VoidHand	newHandle = NULL;				// handle for field
char	buff[MAX_CARD_DATA+1];				// card data buffer
MSRSetting	appSetting;						// setting for MSR 3000
UInt GMsrMgrLibRefNum = sysInvalidRefNum;	// MSR manager library reference number
unsigned long	msrVer, libVer;				// version number

/***********************************************************************
 *
 *   Private Global variables
 *
 ***********************************************************************/
static Boolean GMsrMgrLibWasPreLoaded = false;		// set to TRUE if the sample library was pre-loaded,
													// so we won't try to unload it.

/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator					'MSR3'
#define appVersionNum              0x01
#define appPrefID                  0x00
#define appPrefVersionNum          0x01


// Define the minimum OS version we support
#define ourMinVersion	sysMakeROMVersion(2,0,0,sysROMStageRelease,0)

/***********************************************************************
 *
 *   External Functions
 *
 ***********************************************************************/
extern Boolean SetTrackFormHandleEvent(EventPtr eventP);
extern Boolean ConfigurationFormHandleEvent(EventPtr eventP);
extern Boolean AddedFieldFormHandleEvent(EventPtr eventP);
extern Boolean SendCommandFormHandleEvent(EventPtr eventP);
extern Boolean FlexibleFieldFormHandleEvent(EventPtr eventP);
extern Boolean GenericDecoderFormHandleEvent(EventPtr eventP);
extern Boolean ReservedCharacterFormHandleEvent(EventPtr eventP);

/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/
static UInt	RS232_Receive();
static void Show_setting_result(Err error, char *cmd_str);
static void Show_card_info();
static void Show_status(Byte status);
static Boolean MSRInfoFormHandleEvent(EventPtr eventP);
static void Show_Version(unsigned long msrVer, unsigned long libVer);
static void	Scroll_Process(Byte	direction);

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
			if (romVersion < sysMakeROMVersion(2,0,0,sysROMStageRelease,0))
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			}
		
		return (sysErrRomIncompatible);
		}

	return (0);
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  objectID - id of the object
 *
 * RETURNED:    VoidPtr
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
VoidPtr GetObjectPtr(Word objectID)
	{
	FormPtr frmP;

	frmP = FrmGetActiveForm();
	return (FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID)));
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
 *
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    
    Err			error;
	FieldPtr	fld;
	Byte		status;

	switch (eventP->eType) 
		{
		case menuEvent:
			// handle menu selection			
			switch (eventP->data.menu.itemID)
			{
				case MainOptionsSetDefault:
					MenuEraseStatus (CurrentMenu);
					error = MsrSetDefault( GMsrMgrLibRefNum);
					Show_setting_result(error, "Set Default");
					handled = true;
					break;

				case MainOptionsSelfDiagnose:
					MenuEraseStatus (CurrentMenu);
					error = MsrSelfDiagnose( GMsrMgrLibRefNum);
					Show_setting_result(error, "Self Diagnose");
					handled = true;
					break;

				case MainOptionsBufferedMode:
					error = MsrSetBufferMode( GMsrMgrLibRefNum, MsrBufferedMode);
					Show_setting_result(error, "Set Buffer Mode");
					handled = true;
					break;
					
				case MainOptionsUnbufferedMode:
					error = MsrSetBufferMode( GMsrMgrLibRefNum, MsrUnbufferedMode);
					Show_setting_result(error, "Set Unbuffered Mode");
					handled = true;
					break;
					
				case MainOptionsArmtoRead:
					error = MsrArmtoRead( GMsrMgrLibRefNum);
					Show_setting_result(error, "Arm to Read");
					handled = true;
					break;
					
				case MainOptionsReadBufferedMSR:
					// read MSR data, timeout is 50*100ms
					error = MsrReadMSRBuffer(GMsrMgrLibRefNum, buff, 50);
					if (error==MsrMgrNormal && StrLen(buff))
						Show_card_info();
					else
						Show_setting_result(error, "Read MSR Buffer Mode");
					handled = true;
					break;
					
				case MainOptionsGetAllBufferedData:
					error = MsrGetDataBuffer(GMsrMgrLibRefNum, buff,MsrGetAllTracks);
					if (error==MsrMgrNormal && StrLen(buff))
						Show_card_info();
					else
						Show_setting_result(error, "Get Data Buffer Mode");
					handled = true;
					break;
					
				case MainOptionsGetBufferedTrack1:
					error = MsrGetDataBuffer(GMsrMgrLibRefNum, buff, MsrGetTrack1);
					if (error==MsrMgrNormal && StrLen(buff))
						Show_card_info();
					else
						Show_setting_result(error, "Get Buffer Mode Track 1");
					handled = true;
					break;
					
				case MainOptionsGetBufferedTrack2:
					error = MsrGetDataBuffer(GMsrMgrLibRefNum, buff, MsrGetTrack2);
					if (error==MsrMgrNormal && StrLen(buff))
						Show_card_info();
					else
						Show_setting_result(error, "Get Buffer Mode Track 2");
					handled = true;
					break;
					
				case MainOptionsGetBufferedTrack3:
					error = MsrGetDataBuffer(GMsrMgrLibRefNum, buff,MsrGetTrack3);
					if (error==MsrMgrNormal && StrLen(buff))
						Show_card_info();
					else
						Show_setting_result(error, "Get Buffer Mode Track 3");
					handled = true;
					break;
					
				case MainOptionsAboutMSRdemo:
					// Load the info form, then display it.
					MenuEraseStatus (CurrentMenu);
					FrmGotoForm(MSRInfoForm);
					handled = true;
					break;
			
				case MainOptionsConfiguratorSetting:
					MenuEraseStatus (CurrentMenu);
					error = MsrSendSetting(GMsrMgrLibRefNum, user_MsrSetting);
					Show_setting_result(error, "Configurator Setting");
					handled = true;
					break;
	
				case MSRSettingTrackSetting:
					// Load the Set Track form, then display it.
					MenuEraseStatus (CurrentMenu);
					FrmGotoForm(SetTrackForm);
					handled = true;
 					break;;
			
				case MSRSettingConfiguration:
					// Load the Configuration form, then display it.
					MenuEraseStatus (CurrentMenu);
					FrmGotoForm(ConfigurationForm);
					handled = true;
 					break;;
			
				case MSRSettingAddedField:
					// Load the Added field form, then display it.
					MenuEraseStatus (CurrentMenu);
					FrmGotoForm(AddedFieldForm);
					handled = true;
 					break;;

				case MSRSettingSendCommand:
					// Load the Send Command form, then display it.
					MenuEraseStatus (CurrentMenu);
					FrmGotoForm(SendCommandForm);
					handled = true;
 					break;;

				case MSRSettingFlexibleField:
					// Load the Flexible Field form, then display it.
					MenuEraseStatus (CurrentMenu);
					FrmGotoForm(FlexibleFieldForm);
					handled = true;
 					break;;

				case MSRSettingGenericDecoder:
					// Load the Generic Decoder form, then display it.
					MenuEraseStatus (CurrentMenu);
					FrmGotoForm(GenericDecoderForm);
					handled = true;
 					break;;

				case MSRSettingReservedCharacter:
					// Load the Reserved chracater form, then display it.
					MenuEraseStatus (CurrentMenu);
					FrmGotoForm(ReservedCharacterForm);
					handled = true;
 					break;;
			}
			break;

		case frmLoadEvent:
			FrmInitForm(MainForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			// show MSR and library version number at open
			Show_Version(msrVer, libVer);
			handled = true;
			break;

		case frmCloseEvent:
			frmP = FrmGetActiveForm();
			fld = GetObjectPtr(MainCardInfoField);
			FldSetTextHandle(fld, NULL);
			frmP = FrmGetActiveForm();
			FrmEraseForm(frmP);
			FrmDeleteForm(frmP);
			handled = true;
			break;

	   	case ctlSelectEvent:  // A control button was pressed and released.
	   		// If status button is pressed
	   		if (eventP->data.ctlEnter.controlID == MainStatusButton)
	   		{
				error = MsrGetStatus( GMsrMgrLibRefNum, &status);
				Show_setting_result(error, "Get Status");
				Show_status(status);
				handled = true;
				break;
			}
	   		// If version button is pressed
	   		if (eventP->data.ctlEnter.controlID == MainVersionButton)
	   		{
				error = MsrGetVersion( GMsrMgrLibRefNum, &msrVer, &libVer);
				if (error)
					Show_setting_result(error, "Get Version");
				else
					Show_Version(msrVer, libVer);
				handled = true;
				break;
			}
			break;
			
		case keyDownEvent:
			// show card data as soon as msrDataRedayKey event arrives
			if ((eventP->data.keyDown.chr==msrDataReadyKey))  {
				error = RS232_Receive();
				SndPlaySystemSound(sndInfo);
				handled=true;
			}
			break;
			
		case sclRepeatEvent:		
			if (eventP->data.sclRepeat.newValue < eventP->data.sclRepeat.value)
				// scroll up
				Scroll_Process(0);
			else if (eventP->data.sclRepeat.newValue > eventP->data.sclRepeat.value)
				// scroll down
				Scroll_Process(1);
			handled = true;
			break;
			
		default:
			break;
		}
	
	return handled;
}

/***********************************************************************
 *
 * FUNCTION:    Show_card_info
 *
 * DESCRIPTION: This routine is the display function 
 *              to show card information on MainCardInfoField.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    None
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void Show_card_info()
{
	FieldPtr 		fld;
	CharPtr			newText;

	// get Field pointer	
	fld = GetObjectPtr(MainCardInfoField);
	// Lock down the handle and get a pointer to the memory chunk.
	newText = MemHandleLock(newHandle);
	// clear screen
	*newText = NULL;	
	FldSetTextHandle(fld, newHandle);
	FldDrawField(fld);	
	// Copy the data from cradle port to the new memory chunk.
	StrCopy(newText, buff);
	// Unlock the new memory chunk.
	MemHandleUnlock(newHandle);		
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, newHandle);
	FldDrawField(fld);	
}

/***********************************************************************
 *
 * FUNCTION:    Show_setting_result
 *
 * DESCRIPTION: This routine is the dispaly function 
 *              to show the setting result on MainFormInfoField.
 *
 * PARAMETERS:  error  - return code after executing a setting command
 *				cmd_str- setting command
 *
 * RETURNED:    None
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void Show_setting_result(Err error, char *cmd_str)
{
	FieldPtr	fld;
	CharPtr	newText;
	
	
	fld = GetObjectPtr(MainCardInfoField);
	newText = MemHandleLock(newHandle);		
	// Copy the command string to the new memory chunk.
	StrCopy(newText, cmd_str);
	if (error==MsrMgrNormal)
		StrCat(newText, " was successed!\n");
	else if (error==MsrMgrErrTimeout)
		StrCat(newText, " was failed! \n No answer from MSR 3000!\n");
	else if (error==MsrMgrErrNAK)
		StrCat(newText, " was failed!\n Check the command! \n");
	else StrCat(newText, " was failed! \n");
	// Unlock the new memory chunk.
	MemHandleUnlock(newHandle);		
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, newHandle);
	FldDrawField(fld);
}

/***********************************************************************
 *
 * FUNCTION:    Show_status
 *
 * DESCRIPTION: This routine is the dispaly function 
 *              to show the decode status on MainFormInfoField.
 *
 * PARAMETERS:  status  - decoder status of last sampling and decoding
 *
 * RETURNED:    None
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void Show_status(Byte status)
{
	FieldPtr	fld;
	CharPtr		newText;
	
	// see manual for the format of status
	fld = GetObjectPtr(MainCardInfoField);
	newText = MemHandleLock(newHandle);		
	if (status & 0x01) {
		if (status & 0x08)
			StrCat(newText, "Track1 decode success!\n");
		else
			StrCat(newText, "Track1 decode fail!\n");
	}
	if (status & 0x02) {
		if (status & 0x10)
			StrCat(newText, "Track2 decode success!\n");
		else
			StrCat(newText, "Track2 decode fail!\n");
	}
	if (status & 0x04) {
		if (status & 0x20)
			StrCat(newText, "Track3 decode success!\n");
		else
			StrCat(newText, "Track3 decode fail!\n");
	}
	
	// Unlock the new memory chunk.
	MemHandleUnlock(newHandle);		
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, newHandle);
	FldDrawField(fld);
}

/***********************************************************************
 *
 * FUNCTION:    Show_Version
 *
 * DESCRIPTION: This routine is the dispaly function 
 *              to show the firmware version on MainFormInfoField.
 *
 * PARAMETERS:  msrVer- MSR3000 version on Palm version format
 *				libVer- library version 
 *
 * RETURNED:    None
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void Show_Version(unsigned long msrVer, unsigned long libVer)
{
	FieldPtr		fld;
	CharPtr			newText;
	unsigned char	version_str1[4], version_str[9];
	Byte			i;
	
	// see manual for the format of version number
	fld = GetObjectPtr(MainCardInfoField);
	newText = MemHandleLock(newHandle);
	StrCopy(newText, "MSR3000 version is ");
	MemMove(version_str1, (char *)&msrVer, sizeof(msrVer));
	for (i=0; i<4; i++) {
		version_str[2*i] = ((version_str1[i] & 0xF0) >> 4) +'0';
		version_str[2*i+1] = (version_str1[i] & 0x0F) + '0';
	}
		
	version_str[8] = '\n';
	version_str[9] = NULL;	
	StrCat(newText, (char *)version_str);
	
	StrCat(newText, "MSR shared library version is ");
	MemMove(version_str1, (char *)&libVer, sizeof(libVer));
	for (i=0; i<4; i++) {
		version_str[2*i] = ((version_str1[i] & 0xF0) >> 4) + '0';
		version_str[2*i+1] = (version_str1[i] & 0x0F) + '0';
	}
	version_str[8] = NULL;	
	StrCat(newText, (char *)version_str);
	
	// Unlock the new memory chunk.
	MemHandleUnlock(newHandle);		
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, newHandle);
	FldDrawField(fld);
}


/***********************************************************************
 *
 * FUNCTION:    RS232_Receive
 *
 * DESCRIPTION: This routine is check cradle Port 
 *              and display received data on DataInfo field
 *
 * PARAMETERS:  frmP  - a pointer to Main Form
 *
 * RETURNED:    true if no error occured.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt	RS232_Receive()
{
	FieldPtr 		fld;
	Err				error;
	unsigned short	numReceived;
	CharPtr			newText;
	ScrollBarPtr	sclP;

	// get Field pointer	
	fld = GetObjectPtr(MainCardInfoField);

	// get a card data
	error = MsrReadMSRUnbuffer( GMsrMgrLibRefNum, buff);
	if (error)
		return (error);
	numReceived = StrLen(buff);
	if ( numReceived ){
		// Lock down the handle and get a pointer to the memory chunk.
		newText = MemHandleLock(newHandle);
		// clear screen
		*newText = NULL;	
		FldSetTextHandle(fld, newHandle);
		FldDrawField(fld);	
		// Copy the data from the cradle port to the new memory chunk.
		if (numReceived && (numReceived<MAX_CARD_DATA)) {
			StrCopy(newText, buff);
		}
		// Unlock the new memory chunk.
		MemHandleUnlock(newHandle);		
		// Set the field's text to the data in the new memory chunk.
		FldSetTextHandle(fld, newHandle);
		FldDrawField(fld);	
		
		sclP = GetObjectPtr(MainInfoScrollBar);
		if (numReceived > 300)
			// show scroll bar if data > 300 characters
			SclSetScrollBar(sclP,0,0,1,1);
		else
			// hide scroll bar if data < 300 characters
			SclSetScrollBar(sclP,0,0,0,0);		
	}
	return (MsrMgrNormal);		

}


/***********************************************************************
 *
 * FUNCTION:    Scroll_Process
 *
 * DESCRIPTION: This routine is to dispaly 
 *              received data according to scroll bar
 *
 * PARAMETERS:  direction:	0  - scroll up
 *							1  - scroll down
 *
 * RETURNED:    true if no error occured.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void	Scroll_Process(Byte	direction)
{
	FieldPtr 		fld;
	CharPtr			newText;

	// get Field pointer	
	fld = GetObjectPtr(MainCardInfoField);
	// Lock down the handle and get a pointer to the memory chunk.
	newText = MemHandleLock(newHandle);
	// clear screen
	*newText = NULL;	
	FldSetTextHandle(fld, newHandle);
	FldDrawField(fld);	
	// Copy the data from the cradle port to the new memory chunk.
	if (direction==0)
		// show data from beginning
		StrCopy(newText, buff);
	else if (direction==1)
		// skip first 90 characters then show data
		StrCopy(newText, &buff[90]);
	
	// Unlock the new memory chunk.
	MemHandleUnlock(newHandle);		
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, newHandle);
	FldDrawField(fld);	
		
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
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId)
			{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

			case SetTrackForm:
				FrmSetEventHandler(frmP, SetTrackFormHandleEvent);
				break;

			case ConfigurationForm:
				FrmSetEventHandler(frmP, ConfigurationFormHandleEvent);
				break;

			case AddedFieldForm:
				FrmSetEventHandler(frmP, AddedFieldFormHandleEvent);
				break;

			case SendCommandForm:
				FrmSetEventHandler(frmP, SendCommandFormHandleEvent);
				break;

			case FlexibleFieldForm:
				FrmSetEventHandler(frmP, FlexibleFieldFormHandleEvent);
				break;

			case GenericDecoderForm:
				FrmSetEventHandler(frmP, GenericDecoderFormHandleEvent);
				break;

			case ReservedCharacterForm:
				FrmSetEventHandler(frmP, ReservedCharacterFormHandleEvent);
				break;

			case MSRInfoForm:
				FrmSetEventHandler(frmP, MSRInfoFormHandleEvent);
				break;

			default:
				ErrFatalDisplay("Invalid Form Load Event");
				break;

			}
			return true;
		}
		else if (FrmGetActiveFormID()!=MainForm)
		{
			if (eventP->eType == keyDownEvent && 
				eventP->data.keyDown.chr==msrDataReadyKey)  {
				// get a card data and throw it away
				// to prevent from blocking msrDataReadyKey event
				MsrReadMSRUnbuffer( GMsrMgrLibRefNum, buff);
				SndPlaySystemSound(sndWarning);
				return true;
			}
		}	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    MSRInfoFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "MSRInfoForm" of this application.
 *
 * PARAMETERS:  eventP  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean MSRInfoFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    
	switch (eventP->eType) 
		{
		case menuEvent:
			break;

		case frmLoadEvent:
			frmP = FrmInitForm(MSRInfoForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm ( frmP);			
			handled = true;
			break;

		case frmCloseEvent:
			frmP = FrmGetActiveForm();
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			handled = true;
			break;
			
	   	case ctlSelectEvent:  // A control button was pressed and released.			
	   		// If the OK button is pressed
	   		if (eventP->data.ctlEnter.controlID == MSRInfoOKButton) {								
				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
		default:
			break;
		}
	
	return handled;
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
 *
 *
 ***********************************************************************/
static void AppEventLoop(void)
{
	Word error;
	EventType event;
	
	do {
		EvtGetEvent(&event, evtWaitForever);
				
		// close application when battery is low				
		if (event.eType == keyDownEvent)
			if (event.data.keyDown.chr == lowBatteryChr)
			{
				FormPtr	frmP;
				// battery power check
				UInt			batteryVolts;
				UInt			warnThreshold, criticalThreshold, maxTicks;
				SysBatteryKind	kind;
				Boolean			pluggedIn;
				Byte			precent;
				
				batteryVolts = SysBatteryInfo(false, &warnThreshold, &criticalThreshold,
								&maxTicks, &kind, &pluggedIn, &precent);
				// lowBatteryChr was issued before sleep
				// display low battery message when less than 10% left
				if (precent >= 10)
					continue;
				FrmCloseAllForms();
				// display Low Battery message
				frmP = FrmInitForm(LowBatteryForm);
				FrmDoDialog(frmP);
				// Delete the LowBattery form.
 				FrmDeleteForm(frmP);

				break;
			};
		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(CurrentMenu, &event, &error))
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
 *
 *
 ***********************************************************************/
static Err AppStart(void)
{
    StarterPreferenceType 	prefs;
    Word 					prefsSize;

	Err						error;

	// Read the saved preferences / saved-state information.
	prefsSize = sizeof(StarterPreferenceType);
	if (PrefGetAppPreferences(appFileCreator, appPrefID, &prefs, &prefsSize, true) != 
		noPreferenceFound)
		{
		}
	
	// Load the MSR library
	error = SysLibFind(MsrMgrLibName, &GMsrMgrLibRefNum);
	if (!error) {
		GMsrMgrLibWasPreLoaded = true;
	}
	else {
		// to load MSR library
		error = SysLibLoad(MsrMgrLibTypeID, MsrMgrLibCreatorID, &GMsrMgrLibRefNum);
		if (error) {
			FormPtr	frmP;

			frmP = FrmInitForm(NoMSRLibraryForm);
			FrmDoDialog(frmP);		
			// Delete the info form.
 			FrmDeleteForm(frmP);
			return (error);
		};	
	}
	// open MSR 3000 shared library
	error = MsrOpen(GMsrMgrLibRefNum, &msrVer, &libVer);
	if (error != MsrMgrNormal) {
		FormPtr	frmP;

		// no enough battery
		if (error == MsrMgrLowBattery) {
			frmP = FrmInitForm(LowBatteryForm);
			FrmDoDialog(frmP);
			// Delete the LowBattery form.
 			FrmDeleteForm(frmP);
 		}
		// failed to talk with MSR 3000
		else {
			frmP = FrmInitForm(NoMSRForm);
			FrmDoDialog(frmP);
			// Delete the No MSR form.
 			FrmDeleteForm(frmP);
 		}
		return (error);
 	}

	// Allocate a new memory chunk that will contain received string.
	newHandle = MemHandleNew(MAX_CARD_DATA);		

	return (MsrMgrNormal);
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
 *
 *
 ***********************************************************************/
static void AppStop(void)
{
	StarterPreferenceType prefs;
 
	Err	error;  
   
	// Write the saved preferences / saved-state information.  This data 
	// will be backed up during a HotSync.
	PrefSetAppPreferences (appFileCreator, appPrefID, appPrefVersionNum, 
		&prefs, sizeof (prefs), true);

	// close MSR manager library
	error = MsrClose(GMsrMgrLibRefNum);
	
	// Uninstall the MSR Manager library
	if ( !GMsrMgrLibWasPreLoaded && GMsrMgrLibRefNum != sysInvalidRefNum )
		{
		error = SysLibRemove(GMsrMgrLibRefNum);
		ErrFatalDisplayIf(error, "error uninstalling MSR Manager library.");
		GMsrMgrLibRefNum = sysInvalidRefNum;
		}
	
	// Free a memory chunk
	if (newHandle)
		MemHandleFree(newHandle);		

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
 *
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
			if (error) {
				if (error!=sysErrLibNotFound)
					AppStop();
				return error;
			}
			
			FrmGotoForm(MSRInfoForm);
			AppEventLoop();
			AppStop();
			
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
 *
 *
 ***********************************************************************/
DWord PilotMain( Word cmd, Ptr cmdPBP, Word launchFlags)
{
    return StarterPilotMain(cmd, cmdPBP, launchFlags);
}
