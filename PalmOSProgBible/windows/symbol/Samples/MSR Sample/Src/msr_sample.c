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
#include "MSRSampleRsc.h"



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

/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
static MenuBarPtr		CurrentMenu = NULL;	// pointer to current menu

VoidHand	newHandle = NULL;
char	buff[MAX_CARD_DATA+1];
UInt GMsrMgrLibRefNum = sysInvalidRefNum;	// MSR manager library reference number

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
#define appFileCreator					'MSR2'
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

/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/
static UInt	MSR_Receive();
static Boolean MSRInfoFormHandleEvent(EventPtr eventP);

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

	switch (eventP->eType) 
		{
		case menuEvent:
			switch (eventP->data.menu.itemID)
			{
				case MainOptionsAboutMSRSample:
					// Load the info form, then display it.
					MenuEraseStatus (CurrentMenu);
					FrmGotoForm(MSRInfoForm);
					handled = true;
					break;
			}
			break;

		case frmLoadEvent:
			FrmInitForm(MainForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
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

	   	case ctlSelectEvent:  	// A control button was pressed and released.
			break;
			
		case keyDownEvent:		// display card data if it is msrDataReadyKey
		
			if ((eventP->data.keyDown.chr==msrDataReadyKey))  {
				handled=true;
				error = MSR_Receive();
			}
			break;
			
		default:
			break;
		}
	
	return handled;
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
						
		case keyDownEvent:	// card data ready on unbuffered mode		
			if ((eventP->data.keyDown.chr==msrDataReadyKey))  {
				handled=true;
				MsrReadMSRUnbuffer( GMsrMgrLibRefNum, buff);
				SndPlaySystemSound(sndWarning);
			}
			break;

		default:
			break;
		}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    MSR_Receive
 *
 * DESCRIPTION: This routine is check Cradle Port 
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
static UInt	MSR_Receive()
{
	FieldPtr 		fld;
	Err				error;
	unsigned short	numReceived;
	CharPtr			newText;

	// get Field pointer	
	fld = GetObjectPtr(MainCardInfoField);

	// get card data
	error = MsrReadMSRUnbuffer( GMsrMgrLibRefNum, buff);
	if (error)
		return (error);
	// get the length of card data
	numReceived = StrLen(buff);
	if ( numReceived ){
		// Lock down the handle and get a pointer to the memory chunk.
		newText = MemHandleLock(newHandle);
		// clear screen
		*newText = NULL;	
		FldSetTextHandle(fld, newHandle);
		FldDrawField(fld);	
		// Copy the data from the buffer to the new memory chunk.
		if (numReceived && (numReceived<MAX_CARD_DATA)) {
			StrCopy(newText, buff);
		}
		// Unlock the new memory chunk.
		MemHandleUnlock(newHandle);		
		// Set the field's text to the data in the new memory chunk.
		FldSetTextHandle(fld, newHandle);
		FldDrawField(fld);	
	}
	return (MsrMgrNormal);		

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

			case MSRInfoForm:
				FrmSetEventHandler(frmP, MSRInfoFormHandleEvent);
				break;

			default:
				ErrFatalDisplay("Invalid Form Load Event");
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
	Err						error;
	unsigned long			msrVer, libVer;
	
	// Load the MSR library
	error = SysLibFind(MsrMgrLibName, &GMsrMgrLibRefNum);
	if (error) {
		// to load MSR library
		error = SysLibLoad(MsrMgrLibTypeID, MsrMgrLibCreatorID, &GMsrMgrLibRefNum);
		// display error message if MSR library has not been found
		ErrFatalDisplayIf(error, "No MSR Manager library.");
		if (error)
			return (error);
	}
	else
		GMsrMgrLibWasPreLoaded = true;		// global flag
	
	error = MsrOpen(GMsrMgrLibRefNum, &msrVer, &libVer);
	// display error message if open() failed
	if (error == MsrMgrLowBattery) {
		ErrFatalDisplayIf(error, "No Enough Battery For MSR 3000.");  }
	else
		ErrFatalDisplayIf(error, "No MSR 3000 has been found.");
	if (error)
		return (error);
	// set MSR 3000 to default setting
	// because "MSR Sample" does not work at buffered mode
	error = MsrSetDefault( GMsrMgrLibRefNum);
	if (error)
		return (error);
		
	// Allocate a new memory chunk that will contain receivd string.
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
	Err	error;  
   
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

