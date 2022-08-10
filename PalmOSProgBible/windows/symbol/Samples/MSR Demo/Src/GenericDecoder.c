/***********************************************************************
 *
 Copyright (c) 1999 Symbol Technologies, Inc.  
 All rights reserved.
   
 
 *************************************************************************
 *
 * PROJECT:  Palm III MSR
 * FILE:     Genericdecoder.C
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

/***********************************************************************
 *
 *   Internal Functions and definition
 *
 ***********************************************************************/
#define	MAX_INPUT_CHAR	5		// maximum character can be input
#define	MODE_LIST_NUM	3		// items in mode list
#define	FORMAT_LIST_NUM	2		// items in format list

static Err ShowDecoderMode(FormPtr frmP);
static Err SetDecoderMode(FormPtr frmP);
static Err ShowTrackFormat(FormPtr frmP, Byte track, Word fieldIdx[6], VoidHand fldHandle[4]);
static Err SetTrackFormat(FormPtr frmP, Byte track, Word fieldIdx[4]);

/***********************************************************************
 *
 *   Local variables
 *
 ***********************************************************************/
// index table for pop up menu
static	char	modeIdx[MODE_LIST_NUM] = { 
					MsrNormalDecoder, MsrGenericDecoder, MsrRawDataDecoder };
static	char	trackIdx[MAX_TRACK_NUM] = { 
					MsrTrack1Format, MsrTrack2Format, MsrTrack3Format };
static	char	formatIdx[FORMAT_LIST_NUM] = { Msr5BitsFormat, Msr7BitsFormat };
static	VoidHand	inputFldHandle[3][4]={  {NULL,NULL,NULL,NULL},
    									{NULL,NULL,NULL,NULL},
    									{NULL,NULL,NULL,NULL}};
					
/***********************************************************************
 *
 * FUNCTION:    GenericDecoderHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "Generic Decoder Form" of this application.
 *
 *				Space and invisible character will be shown as /XXX
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
Boolean GenericDecoderFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    Err			error;
    Byte		i,j;
	Word	fieldIdx[3][6] = { {GenericDecoderFormat1List,
								GenericDecoderSS_B1Field,
								GenericDecoderSS_A1Field,
								GenericDecoderES_B1Field,
								GenericDecoderES_A1Field,
								GenericDecoderFormat1PopTrigger},
							   {GenericDecoderFormat2List,
								GenericDecoderSS_B2Field,
								GenericDecoderSS_A2Field,
								GenericDecoderES_B2Field,
								GenericDecoderES_A2Field,
								GenericDecoderFormat2PopTrigger},
							   {GenericDecoderFormat3List,
								GenericDecoderSS_B3Field,
								GenericDecoderSS_A3Field,
								GenericDecoderES_B3Field,
								GenericDecoderES_A3Field,
								GenericDecoderFormat3PopTrigger}};
    
	switch (eventP->eType) {
		case menuEvent:
			break;

		case frmLoadEvent:
			frmP = FrmInitForm(GenericDecoderForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm (frmP);

			// allocate handles for fields
			for (i=0; i<3; i++)
				for (j=0; j<4; j++)
					inputFldHandle[i][j] = MemHandleNew(MAX_INPUT_CHAR+1);
			// show current setting, beep if error occurs
			error = MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			if (error) 
				SndPlaySystemSound(sndWarning);
			// show decoder mode
			error = ShowDecoderMode(frmP);
			if (error) 
				SndPlaySystemSound(sndWarning);
			// show track1 format
			error = ShowTrackFormat(frmP, 0, fieldIdx[0], inputFldHandle[0]);
			if (error) 
				SndPlaySystemSound(sndWarning);
			// show track2 format
			error = ShowTrackFormat(frmP, 1, fieldIdx[1], inputFldHandle[1]);
			if (error) 
				SndPlaySystemSound(sndWarning);
			// show track3 format
			error = ShowTrackFormat(frmP, 2, fieldIdx[2], inputFldHandle[2]);
			if (error) 
				SndPlaySystemSound(sndWarning);
			handled = true;
			break;

		case frmCloseEvent:
			frmP = FrmGetActiveForm();

			// free handles for fields
			for (i=0; i<3; i++)
				for (j=0; j<4; j++) 
					if (inputFldHandle[i][j]) {
						FldSetTextHandle(GetObjectPtr(fieldIdx[i][j+1]),NULL);
						MemHandleFree(inputFldHandle[i][j]);
					}
				
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			handled = true;
			break;
			
	   	case ctlSelectEvent:  // A control button was pressed and released.
			
	   		// If the OK button is pressed
	   		if (eventP->data.ctlEnter.controlID == GenericDecoderOKButton) {										
				frmP = FrmGetActiveForm();
				error = SetDecoderMode(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				// set track 1 format
				error = SetTrackFormat(frmP, 0, fieldIdx[0]);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				// set track 2 format
				error = SetTrackFormat(frmP, 1, fieldIdx[1]);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				// set track 3 format
				error = SetTrackFormat(frmP, 2, fieldIdx[2]);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				
				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
	   		if (eventP->data.ctlEnter.controlID == GenericDecoderCancelButton) {
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
 * FUNCTION:    ShowDecoderMode
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current decoder mode.
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
static Err ShowDecoderMode(FormPtr frmP)
{
	ListPtr 	listP;
	ControlPtr	ctlP;
	CharPtr		label;
	char		i, index=-1;
	
	for (i=0; i<MODE_LIST_NUM; i++)
		if (modeIdx[i]==appSetting.Decoder_mode) {
			index = i;
			break;
		}
	
	if (index<0)
		return(-1);
		
	listP = GetObjectPtr(GenericDecoderModeList);
	label = LstGetSelectionText (listP, index);
	ctlP = GetObjectPtr (GenericDecoderModePopTrigger);
	CtlSetLabel (ctlP, label);
    LstSetSelection(listP, index);
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    SetDecoderMode
 *
 * DESCRIPTION: This routine is the routine 
 *              to set the current decoder mode.
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
static Err SetDecoderMode(FormPtr frmP)
{
	ListPtr 	listP;
	char		index;					
		
	listP = GetObjectPtr(GenericDecoderModeList);
	index = LstGetSelection (listP);
	// Set MSR 3000
	if (modeIdx[index]!=appSetting.Decoder_mode) {
		appSetting.Decoder_mode = modeIdx[index];
		return (MsrSetDecoderMode(GMsrMgrLibRefNum, modeIdx[index]));
	}
	else return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    ShowTrackFormat
 *
 * DESCRIPTION: This routine is the display routine 
 *              to show the Track Format.
 *
 * PARAMETERS:  frmP	- a pointer to current form
 *				track	- track No. 0:Track1 1:Track2 2:Track3
 *				fieldIdx- field object name for track format
 *				fldHandle- field object handle for track format
 *
 * RETURNED:    MsrNormal
 *              Other
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err ShowTrackFormat(FormPtr frmP, Byte track, Word fieldIdx[6], VoidHand fldHandle[4])
{
	ListPtr 	listP;
	ControlPtr	ctlP;
	CharPtr		label;
	char		i, index=-1;
	FieldPtr 	fld;
	CharPtr		fldText;
	Byte		fldChar;
    
	
	for (i=0; i<FORMAT_LIST_NUM; i++)
		if (formatIdx[i]==appSetting.Track_format[track][0]) {
			index = i;
			break;
		}
	
	if (index<0)
		return(-1);
		
	listP = GetObjectPtr(fieldIdx[0]);
	label = LstGetSelectionText (listP, index);
	ctlP = GetObjectPtr (fieldIdx[5]);
	CtlSetLabel (ctlP, label);
    LstSetSelection(listP, index);
	
	for (i=0; i<4; i++) {
		fld = GetObjectPtr(fieldIdx[i+1]);
		// Lock down the handle and get a pointer to the memory chunk.
		fldText = MemHandleLock(fldHandle[i]);
		// clear field
		*fldText = NULL;	
		FldSetTextHandle(fld, fldHandle[i]);
		FldDrawField(fld);
	
		fldChar = appSetting.Track_format[track][i+1];
		// space might be invisible, show it as /XXX
		if (fldChar>0x20 && fldChar<0x7F) {
			*fldText++ = fldChar;
			*fldText = NULL;
		}
		else {
			*fldText++ = '/';
			*fldText++ = fldChar/100 + '0';
			fldChar = fldChar % 100;
			*fldText++ = fldChar/10 + '0';
			fldChar = fldChar % 10;
			*fldText++ = fldChar + '0';		
			*fldText = NULL;
		}
						
		// Unlock the new memory chunk.
		MemHandleUnlock(fldHandle[i]);		
		// Set the field's text to the data in the new memory chunk.
		FldSetTextHandle(fld, fldHandle[i]);
		FldDrawField(fld);
	}		
	return (MsrMgrNormal);

}


/***********************************************************************
 *
 * FUNCTION:    SetTrackFormat
 *
 * DESCRIPTION: This routine is the routine 
 *              to set the Track Format.
 *
 * PARAMETERS:  frmP	- a pointer to current form
 *				track	- track No. 0:Track1 1:Track2 2:Track3
 *				fieldIdx- field object name for track format
 *
 * RETURNED:    MsrNormal
 *              Other
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err SetTrackFormat(FormPtr frmP, Byte track, Word fieldIdx[6])
{
	ListPtr 	listP;
	char		index;
	Byte		formatIdx[2] = {Msr5BitsFormat,Msr7BitsFormat};
	VoidHand 	fldHandle;
	FieldPtr 	fld;
	CharPtr		inputText;
	Byte		i, field, inputChar[4]={0,0,0,0};
	Err			error;
					
	// get format from format list	
	listP = GetObjectPtr(fieldIdx[0]);
	index = LstGetSelection (listP);

	for (field=0; field<4; field++) {
		// get SS_B pattern 		
		fld = GetObjectPtr(fieldIdx[field+1]);
		fldHandle = FldGetTextHandle(fld);
		// Lock down the handle and get a pointer to the memory chunk.
		inputText = MemHandleLock(fldHandle);
		// input field length check
		if (StrLen(inputText)>4 || StrLen(inputText)==0) {
			MemHandleUnlock(fldHandle);
			return(MsrMgrErrParam);
		}
		if (*inputText=='/') {
			// length check
			if (StrLen(inputText) > 4) {
				// Unlock the new memory chunk.
				MemHandleUnlock(fldHandle);
				return (MsrMgrErrParam);
			}			
			for (i=0;i<StrLen(inputText)-1;i++)
				inputChar[field] = inputChar[field]*10 + *(inputText+i+1) - '0';
		}
		else {
			// length check
			if (StrLen(inputText) > 1) {
				// Unlock the new memory chunk.
				MemHandleUnlock(fldHandle);
				return (MsrMgrErrParam);
			}			
			inputChar[field] = *inputText;
		}
		// Unlock the new memory chunk.
		MemHandleUnlock(fldHandle);
		// value range check	
		if (inputChar[field] > 255)
			return (MsrMgrErrParam);
	}

	// set track format if values have been changed		
	if (formatIdx[index]!=appSetting.Track_format[track][0] ||
		inputChar[0]!= appSetting.Track_format[track][1] ||
		inputChar[1]!= appSetting.Track_format[track][2] ||
		inputChar[2]!= appSetting.Track_format[track][3] ||
		inputChar[3]!= appSetting.Track_format[track][4] ) {
		appSetting.Track_format[track][0] = formatIdx[index];
		for (i=0; i<TRACK_FORMAT_LEN-1; i++)
			appSetting.Track_format[track][i+1] = inputChar[i];
		
		// Set MSR 3000
		error = MsrSetTrackFormat(GMsrMgrLibRefNum, trackIdx[track], formatIdx[index],
						inputChar[0],inputChar[1],inputChar[2],inputChar[3]);
		if (error) {
			MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			return (error);
		}
	}
	
	return (MsrMgrNormal);
    
}

