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
extern VoidPtr GetObjectPtr(Word objectID);

static Err ShowAddedField(FormPtr frmP, VoidHand fldHandle);
static Err SetAddedField(FormPtr frmP);

#define	MAX_AFLD	160			// maximum characters for added field

/***********************************************************************
 *
 * FUNCTION:    AddedFieldHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "AddedFieldForm" of this application.
 *
 *				"/n" is used to separate two added fields
 *				/XXX is used to show SP and invisible character
 *				XXX is a decimal digital of ascii code for the character
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
Boolean AddedFieldFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    Err			error;
    
    VoidHand	AFldHandle=NULL;

	switch (eventP->eType) 
		{
		case menuEvent:
			break;

		case frmLoadEvent:
			frmP = FrmInitForm(AddedFieldForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm ( frmP);

			// allocate handles for fields
			AFldHandle = MemHandleNew(MAX_AFLD+1);
			// show current setting, beep if error occurs
			error = MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			if (error) 
				SndPlaySystemSound(sndWarning);
			error = ShowAddedField(frmP, AFldHandle);
			if (error)
				SndPlaySystemSound(sndWarning);
			// set focus to added field
			FrmSetFocus(frmP, FrmGetObjectIndex(frmP, AddedFieldAddedField));
			handled = true;
			break;

		case frmCloseEvent:
			// free field handles
			if (AFldHandle) {
				FldSetTextHandle(GetObjectPtr(AddedFieldAddedField),NULL);
				MemHandleFree(AFldHandle);
			}
			frmP = FrmGetActiveForm();
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			handled = true;
			break;
			
	   	case ctlSelectEvent:  // A control button was pressed and released.
			
	   		// If the OK button is pressed
	   		if (eventP->data.ctlEnter.controlID == AddedFieldOKButton) {
				frmP = FrmGetActiveForm();
				// set added field if change has been made
				error = SetAddedField(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}

				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
	   		if (eventP->data.ctlEnter.controlID == AddedFieldCancelButton) {
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
	Byte			preChar, i, j;
    
	fld = GetObjectPtr(AddedFieldAddedField);
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
			// space is invisible sometimes, show it as /032
			if (preChar>0x20 && preChar<0x7F) {
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
		*preText++ = '\n';
	}
	*preText = NULL;
						
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);	
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);		
	
	FldSetInsertionPoint(fld,0);
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    SetAddedField
 *
 * DESCRIPTION: This routine is  
 *              to set Added Field.
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
static Err SetAddedField(FormPtr frmP)
{
	VoidHand 		fldHandle;
	FieldPtr 		fld;
	CharPtr			preText;
	char			preChar, added_str[MAX_AFLD_NUM][MAX_AFLD_LEN+1];
	unsigned char	i, j;
    
	fld = GetObjectPtr(AddedFieldAddedField);
	fldHandle = FldGetTextHandle(fld);
	// Lock down the handle and get a pointer to the memory chunk.
	preText = MemHandleLock(fldHandle);
	
	for (i=0; i<MAX_AFLD_NUM; i++)
		added_str[i][0] = NULL;
		
	i = 0;
	j = 0;
	// get the MAX_AFLD_NUM lines of input
	while (*preText!= NULL && i<MAX_AFLD_NUM) {
		// field number and length check
		if (j>MAX_AFLD_LEN) {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (MsrMgrErrParam);
		}
		// next added field
		if (*preText=='\n') {
			added_str[i++][j] = NULL;
			j = 0;
			preText++;
			continue;
		}
		// visible character
		if (*preText>=0x20 && *preText<0x7F && *preText !='/') {
			added_str[i][j++]=*preText++;
			continue;
		}
		// invisible character has to written as /xxx
		if (*preText!='/' || StrLen(preText)<4) {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (MsrMgrErrParam);
		}
		preChar = (*(preText+1)-'0')*100+(*(preText+2)-'0')*10+*(preText+3)-'0';
		preText += 4;
		// check character range
		if (preChar>0 && preChar<256)
			added_str[i][j++]=preChar;		
		else {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (MsrMgrErrParam);
		}		
	}
	added_str[i][j] = NULL;

	// may have several \cr on input fields
	while (*preText== '\n') 
		preText++;
	preChar = *preText;	
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);	
	
	// check input after line MAX_AFLD_NUM
	if (preChar!=NULL && i>=MAX_AFLD_NUM)
		return (MsrMgrErrParam);
		
	for (i=0; i<MAX_AFLD_NUM; i++) {
		if (StrCompare(appSetting.Added_field[i], added_str[i])) {
			for (j=0; j<MAX_AFLD_NUM; j++)
				StrCopy(appSetting.Added_field[j], added_str[j]);
			return (MsrSetAddedField(GMsrMgrLibRefNum, added_str));
		}
	}
	
	return (MsrMgrNormal);	
}



