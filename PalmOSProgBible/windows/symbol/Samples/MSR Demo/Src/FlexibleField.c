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

static Err ShowFlexibleField(FormPtr frmP, VoidHand fldHandle);
static Err SetFlexibleField(FormPtr frmP);

#define	MAX_FFLD	400			// maximum character for flexible field

/***********************************************************************
 *
 * FUNCTION:    FlexibleFieldHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "FlexibleFieldForm" of this application.
 *
 *				All the character in command has to be writtern as /XXX
 *				XXX is a decimal digital of ascii code for the character
 *				"/n" is used to separate two commands
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
Boolean FlexibleFieldFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    Err			error;
    
    VoidHand	FFldHandle=NULL;

	switch (eventP->eType) 
		{
		case menuEvent:
			break;

		case frmLoadEvent:
			frmP = FrmInitForm(FlexibleFieldForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm ( frmP);

			// allocate handles for fields
			FFldHandle = MemHandleNew(MAX_FFLD+1);
			// show current setting, beep is error occurs
			error = MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			if (error)
				SndPlaySystemSound(sndWarning);
				
			error = ShowFlexibleField(frmP, FFldHandle);
			if (error)
				SndPlaySystemSound(sndWarning);
			// set focus to flexible field
			FrmSetFocus(frmP, FrmGetObjectIndex(frmP, FlexibleFieldFlexibleField));
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
	   		if (eventP->data.ctlEnter.controlID == FlexibleFieldOKButton) {
				frmP = FrmGetActiveForm();
				// set flexible field if change has been made
				error = SetFlexibleField(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}

				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
	   		if (eventP->data.ctlEnter.controlID == FlexibleFieldCancelButton) {
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
 * FUNCTION:    ShowFlexible Field
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Flexible Field
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
static Err ShowFlexibleField(FormPtr frmP, VoidHand fldHandle)
{
	FieldPtr 		fld;
	CharPtr			preText;
	unsigned char	preChar, i, j;
    
	fld = GetObjectPtr(FlexibleFieldFlexibleField);
	// Lock down the handle and get a pointer to the memory chunk.
	preText = MemHandleLock(fldHandle);
	// clear field
	*preText = NULL;	
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);

	for (i=0; i<MAX_FFLD_NUM; i++) {
		if (!appSetting.Flexible_field[i][0])
			break;
		for (j=0; j<=appSetting.Flexible_field[i][0]; j++) {
			*preText++ = '/';
			preChar = appSetting.Flexible_field[i][j];
			*preText++ = preChar/100 + '0';
			preChar = preChar % 100;
			*preText++ = preChar/10 + '0';
			preChar = preChar % 10;
			*preText++ = preChar + '0';
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
 * FUNCTION:    SetFlexibleField
 *
 * DESCRIPTION: This routine is 
 *              to set Flexible Field.
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
static Err SetFlexibleField(FormPtr frmP)
{
	VoidHand 		fldHandle;
	FieldPtr 		fld;
	CharPtr			preText;
	char			ffld_str[MAX_FFLD_NUM][MAX_FFLD_LEN];
	unsigned char	i=0, j=0, preChar;
	Err				error;
    
	fld = GetObjectPtr(FlexibleFieldFlexibleField);
	fldHandle = FldGetTextHandle(fld);
	// Lock down the handle and get a pointer to the memory chunk.
	preText = MemHandleLock(fldHandle);
	
	while (*preText!= NULL && i<MAX_FFLD_NUM && j<=MAX_FFLD_LEN) {
		// next flexible field
		if (*preText=='\n') {
			ffld_str[i++][j] = NULL;
			j = 0;
			preText++;
			continue;
		}
		// all character has to be written as /xxx
		if (*preText!='/' || StrLen(preText)<4) {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (-1);
		}
		preChar = (*(preText+1)-'0')*100+(*(preText+2)-'0')*10+*(preText+3)-'0';
		preText += 4;
		// check character range
		if (preChar>=0 && preChar<256)
			ffld_str[i][j++]=preChar;		
		else {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (-1);
		}		
	}
	// last field without \CR
	if (j)
		ffld_str[i++][j] = NULL;
	// indicates no more flexible fields
	if (i<MAX_FFLD_NUM)
		ffld_str[i++][0] = NULL;
	
	preChar = *preText;	
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);	
	
	// check input after line MAX_FFLD_NUM
	if (preChar!=NULL && i>=MAX_FFLD_NUM)
		return (MsrMgrErrParam);
	
	for (i=0; i<MAX_FFLD_NUM; i++) {
		if (MemCmp(appSetting.Flexible_field[i], ffld_str[i], ffld_str[i][0]+1)) {
			for (j=0; j<MAX_FFLD_NUM; j++)
				MemMove(appSetting.Flexible_field[j], ffld_str[j], ffld_str[j][0]+1);
			error = MsrSetFlexibleField(GMsrMgrLibRefNum, ffld_str);
			if (error)
				MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			return (error);
		}
	}
	
	return (MsrMgrNormal);	
}

