/***********************************************************************
 *
 Copyright © 1995 - 1998, ID Technologies.  
 All rights reserved.
   
 
 *************************************************************************
 *
 * PROJECT:  Palm III MSR
 * FILE:     Config.C
 * AUTHOR:   Howard Zong: Feb. 16, 1999
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
#include "MsrMgrLibLoc.h"
#include "MSRRsc.h"

/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
extern MSRSetting	appSetting;
extern UInt GMsrMgrLibRefNum;
extern VoidPtr GetObjectPtr(Word objectID);

static Err ShowAddedField(FormPtr frmP, VoidHand fldHandle);
static Err ShowSendCmd(FormPtr frmP, VoidHand fldHandle);

#define	MAX_AFLD	80
#define	MAX_SCMDFLD	240

/***********************************************************************
 *
 * FUNCTION:    DataEditHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "DataEditForm" of this application.
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
Boolean DataEditFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    Err			error;
    
    VoidHand	AFldHandle=NULL;
    VoidHand	SCmdFldHandle=NULL;

	switch (eventP->eType) 
		{
		case menuEvent:
			break;

		case frmLoadEvent:
			frmP = FrmInitForm(DataEditForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm ( frmP);

			// allocate handles for fields
			AFldHandle = MemHandleNew(MAX_AFLD+1);
			SCmdFldHandle = MemHandleNew(MAX_SCMDFLD+1);
			
			error = MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			if (error) 
				SndPlaySystemSound(sndWarning);
			error = ShowAddedField(frmP, AFldHandle);
			if (error)
				SndPlaySystemSound(sndWarning);
			error = ShowSendCmd(frmP, SCmdFldHandle);
			if (error)
				SndPlaySystemSound(sndWarning);
			handled = true;
			break;

		case frmCloseEvent:
			// free field handles
			if (AFldHandle) {
				FldSetTextHandle(GetObjectPtr(DataEditAddedField),NULL);
				MemHandleFree(AFldHandle);
			}
			if (SCmdFldHandle) {
				FldSetTextHandle(GetObjectPtr(DataEditSendCommandField),NULL);
				MemHandleFree(SCmdFldHandle);
			}
			frmP = FrmGetActiveForm();
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			handled = true;
			break;
			
	   	case ctlSelectEvent:  // A control button was pressed and released.
			
	   		// If the OK button is pressed
	   		if (eventP->data.ctlEnter.controlID == DataEditOKButton) {

				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
	   		if (eventP->data.ctlEnter.controlID == DataEditCancelButton) {
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
 * FUNCTION:    ShowAddedField
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Added Field
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
static Err ShowAddedField(FormPtr frmP, VoidHand fldHandle)
{
	FieldPtr 		fld;
	CharPtr			preText;
	char			preChar, i, j;
    
	fld = GetObjectPtr(DataEditAddedField);
	// Lock down the handle and get a pointer to the memory chunk.
	preText = MemHandleLock(fldHandle);
	// clear field
	*preText = NULL;	
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);
	FldSetTextHandle(fld, NULL);

	for (i=0; i<MAX_AFLD_NUM; i++) {	
		for (j=0; j<StrLen(appSetting.Added_field[i]); j++) {
			preChar = appSetting.Added_field[i][j];
			if (preChar>=0x20 && preChar<0x7F) {
				*preText++ = preChar;
			}
			else {
				*preText++ = '/';
				*preText++ = preChar/100 + '0';
				preChar = preChar % 100;
				*preText++ = preChar/10 + '0';
				preChar = preChar % 10;
				*preText++ = preChar + '0';
			}
		}
		*preText++ = ' ';
	}
	*preText = NULL;
						
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);		
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);	
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    ShowSendCmd
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Send Command
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
static Err ShowSendCmd(FormPtr frmP, VoidHand fldHandle)
{
	FieldPtr 		fld;
	CharPtr			preText;
	unsigned char	preChar, i, j;
    
	fld = GetObjectPtr(DataEditSendCommandField);
	// Lock down the handle and get a pointer to the memory chunk.
	preText = MemHandleLock(fldHandle);
	// clear field
	*preText = NULL;	
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);
	FldSetTextHandle(fld, NULL);

	for (i=0; i<MAX_SCMD_NUM; i++) {
		for (j=0; j<=appSetting.Send_cmd[i][0]; j++) {
			*preText++ = '/';
			preChar = appSetting.Send_cmd[i][j];
			*preText++ = preChar/100 + '0';
			preChar = preChar % 100;
			*preText++ = preChar/10 + '0';
			preChar = preChar % 10;
			*preText++ = preChar + '0';
		}
		MemMove(preText, "/255\n", 5);
		preText += 5;
	}
	*preText = NULL;
						
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);		
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);	
	
	return (MsrMgrNormal);
}


