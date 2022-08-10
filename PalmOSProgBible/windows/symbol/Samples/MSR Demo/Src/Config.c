/***********************************************************************
 *
 Copyright (c) 1999 Symbol Technologies, Inc.  
 All rights reserved.
   
 
 *************************************************************************
 *
 * PROJECT:  Palm III MSR
 * FILE:     Config.C
 * AUTHOR:   H.Z.: Feb. 16, 1999
 *
 * DECLARER: Starter
 *
 * DESCRIPTION:
 *	  
 *
 **********************************************************************/
#define NEW_SERIAL_MANAGER

#include <Pilot.h>
#include <SysEvtMgr.h>

#ifdef NEW_SERIAL_MANAGER
#include <StringMgr.h>
#else
#include <Hardware.h>
#include <String.h>
#endif

#include "MsrMgrLib.h"
#include "MSRRsc.h"

/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
extern MSRSetting	appSetting;
extern UInt GMsrMgrLibRefNum;

static Err ShowLRC(FormPtr frmP);
static Err SetLRC(FormPtr frmP);
static Err ShowDataEditMode(FormPtr frmP);
static Err SetDataEditMode(FormPtr frmP);

/***********************************************************************
 *
 * FUNCTION:    ConfigurationHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "ConfigurationForm" of this application.
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
Boolean ConfigurationFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    Err			error;
    
	switch (eventP->eType) 
		{
		case menuEvent:
			break;

		case frmLoadEvent:
			frmP = FrmInitForm(ConfigurationForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm ( frmP);
			// show current setting, beep if error occurs
			error = MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			if (error)
				SndPlaySystemSound(sndWarning);
			error = ShowLRC(frmP);
			if (error)
				SndPlaySystemSound(sndWarning);
			error = ShowDataEditMode(frmP);
			if (error)
				SndPlaySystemSound(sndWarning);
			
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
	   		if (eventP->data.ctlEnter.controlID == ConfigurationOKButton) {
				frmP = FrmGetActiveForm();
				
				error = SetLRC(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				error = SetDataEditMode(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				
				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
	   		if (eventP->data.ctlEnter.controlID == ConfigurationCancelButton) {
				frmP = FrmGetActiveForm();
				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
		case keyDownEvent:
		
			if ((eventP->data.keyDown.chr==msrDataReadyKey))  {
				handled=true;
			}
			break;
			
		default:
			break;
		}
	
	return handled;
}



/***********************************************************************
 *
 * FUNCTION:    ShowLRC
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current LRC mode.
 *
 * PARAMETERS:  frmP	- a pointer to current form
 *
 * RETURNED:    MsrMgrNormal
 *              Other
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err ShowLRC(FormPtr frmP)
{
    Boolean 	handled = false;
    
	switch (appSetting.LRC_setting) {
		case MsrNoLRC:
			FrmSetControlGroupSelection(frmP, TrackSettingFormGroupID,ConfigurationNotSendPushButton);
			break;
		case MsrSendLRC:
			FrmSetControlGroupSelection(frmP, TrackSettingFormGroupID,ConfigurationSendLRCPushButton);
			break;
		default:
			return (MsrMgrErrParam);
			break;
	}
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    SetLRC
 *
 * DESCRIPTION: This routine is  
 *              to set LRC mode.
 *
 * PARAMETERS:  frmP	- a pointer to current form
 *
 * RETURNED:    MsrMgrNormal
 *              Other
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err SetLRC(FormPtr frmP)
{
	Byte	index;
				
	// get current LRC mode			
	index = FrmGetControlGroupSelection(frmP, ConfigurationFormGroupID);
	
	// No LRC?
	if (index == FrmGetObjectIndex(frmP,ConfigurationNotSendPushButton)
		&& appSetting.LRC_setting!= MsrNoLRC) {	
		appSetting.LRC_setting = MsrNoLRC;
		return(MsrSetLRC(GMsrMgrLibRefNum, MsrNoLRC));			
	}
	
	// Send LRC ?
	if (index == FrmGetObjectIndex(frmP,ConfigurationSendLRCPushButton)
		&& appSetting.LRC_setting != MsrSendLRC) {	
		appSetting.LRC_setting = MsrSendLRC;
		return (MsrSetLRC(GMsrMgrLibRefNum, MsrSendLRC));			
	}
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    ShowDataEditMode
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the Data Edit Mode.
 *
 * PARAMETERS:  frmP	- a pointer to current form
 *
 * RETURNED:    MsrMgrNormal
 *              Other 
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err ShowDataEditMode(FormPtr frmP)
{
    
	switch (appSetting.Data_edit_setting) {
		case MsrDisableDataEdit:
			FrmSetControlGroupSelection(frmP, ConfigurationFormGroupID2,ConfigurationDisablePushButton);
			break;
		case MsrDataEditMatch:
			FrmSetControlGroupSelection(frmP, ConfigurationFormGroupID2,ConfigurationMatchPushButton);
			break;
		case MsrDataEditUnmatch:
			FrmSetControlGroupSelection(frmP, ConfigurationFormGroupID2,ConfigurationUnmatchPushButton);
			break;
		default:
			return (MsrMgrErrParam);
			break;
	}
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    SetDataEdit
 *
 * DESCRIPTION: This routine is 
 *              to set the Data Edit Mode.
 *
 * PARAMETERS:  frmP	- a pointer to current form
 *
 * RETURNED:    MsrNormal
 *              Other
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err SetDataEditMode(FormPtr frmP)
{
	Byte	index;
				
	// get current Data Edit Mode			
	index = FrmGetControlGroupSelection(frmP, ConfigurationFormGroupID2);
	
	// Disable ?
	if (index == FrmGetObjectIndex(frmP,ConfigurationDisablePushButton)
		&& appSetting.Data_edit_setting != MsrDisableDataEdit) {	
		appSetting.Data_edit_setting = MsrDisableDataEdit;
		return(MsrSetDataEdit(GMsrMgrLibRefNum, MsrDisableDataEdit));			
	}
	
	// Match ?
	if (index == FrmGetObjectIndex(frmP,ConfigurationMatchPushButton)
		&& appSetting.Data_edit_setting != MsrDataEditMatch) {	
		appSetting.Data_edit_setting = MsrDataEditMatch;
		return (MsrSetDataEdit(GMsrMgrLibRefNum, MsrDataEditMatch));			
	}
	
	// Unmatch ?
	if (index == FrmGetObjectIndex(frmP,ConfigurationUnmatchPushButton)
		&& appSetting.Data_edit_setting != MsrDataEditUnmatch) {	
		appSetting.Data_edit_setting = MsrDataEditUnmatch;
		return (MsrSetDataEdit(GMsrMgrLibRefNum, MsrDataEditUnmatch));			
	}
	
	return (MsrMgrNormal);
}


