/***********************************************************************
 *
 Copyright (c) 1999 Symbol Technologies, Inc.  
 All rights reserved.
   
 
 *************************************************************************
 *
 * PROJECT:  Palm III MSR
 * FILE:     Reservedchar.C
 * AUTHOR:   H.Z.: Mar. 15, 1999
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
#define	MAX_INPUT_CHAR	5		// maximum characters for input

static Err ShowReservedCharacter(FormPtr frmP, Word fieldIdx[2][6]);
static Err SetReservedCharacter(FormPtr frmP, Word fieldIdx[2][6]);

/***********************************************************************
 *
 *   Local variables
 *
 ***********************************************************************/
static	VoidHand	resCharFldHandle[2][6]={  {NULL,NULL,NULL,NULL,NULL,NULL},
    										{NULL,NULL,NULL,NULL,NULL,NULL}};
static	Byte		formatIdx[2] = {Msr5BitsFormat, Msr7BitsFormat};

/***********************************************************************
 *
 * FUNCTION:    ReservedCharacterHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "Reserved Character Form" of this application.
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
Boolean ReservedCharacterFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    Err			error;
    Byte		i, j;
	Word		objIdx[4][6] = { {ReservedCharacterFormat1List,
									ReservedCharacterFormat2List,
									ReservedCharacterFormat3List,
									ReservedCharacterFormat4List,
									ReservedCharacterFormat5List,
									ReservedCharacterFormat6List},
								   {ReservedCharacterFormat1PopTrigger,
									ReservedCharacterFormat2PopTrigger,
									ReservedCharacterFormat3PopTrigger,
									ReservedCharacterFormat4PopTrigger,
									ReservedCharacterFormat5PopTrigger,
									ReservedCharacterFormat6PopTrigger},
								   {ReservedCharacterPattern1Field,
									ReservedCharacterPattern2Field,
									ReservedCharacterPattern3Field,
									ReservedCharacterPattern4Field,
									ReservedCharacterPattern5Field,
									ReservedCharacterPattern6Field},
								   {ReservedCharacterChar1Field,
									ReservedCharacterChar2Field,
									ReservedCharacterChar3Field,
									ReservedCharacterChar4Field,
									ReservedCharacterChar5Field,
									ReservedCharacterChar6Field}};
    
	switch (eventP->eType) 
		{
		case menuEvent:
			break;

		case frmLoadEvent:
			frmP = FrmInitForm(ReservedCharacterForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm (frmP);

			error = MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			if (error) 
				SndPlaySystemSound(sndWarning);
			// allocate handles for fields
			for (i=0; i<2; i++)
				for (j=0; j<6; j++)
					resCharFldHandle[i][j] = MemHandleNew(MAX_INPUT_CHAR+1);
			// show Reserved character
			error = ShowReservedCharacter(frmP, objIdx);
			if (error) 
				SndPlaySystemSound(sndWarning);
			handled = true;
			break;

		case frmCloseEvent:
			frmP = FrmGetActiveForm();
			// free handles for fields
			for (i=0; i<2; i++)
				for (j=0; j<6; j++) 
					if (resCharFldHandle[i][j]) {
						FldSetTextHandle(GetObjectPtr(objIdx[i+2][j]),NULL);
						MemHandleFree(resCharFldHandle[i][j]);
					}
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			handled = true;
			break;
			
	   	case ctlSelectEvent:  // A control button was pressed and released.
			
	   		// If the OK button is pressed
	   		if (eventP->data.ctlEnter.controlID == ReservedCharacterOKButton) {
				frmP = FrmGetActiveForm();
				// set reserved character
				error = SetReservedCharacter(frmP, objIdx);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
	   		if (eventP->data.ctlEnter.controlID == ReservedCharacterCancelButton) {
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
 * FUNCTION:    ShowReservedCharacter
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Reserved character.
 *
 * PARAMETERS:  frmP	- a pointer to current form
 *				objIdx- objects name for reserved character form
 *
 * RETURNED:    MsrMgrNormal
 *              Other 
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err ShowReservedCharacter(FormPtr frmP, Word objIdx[4][6])
{
	ListPtr 	listP;
	ControlPtr	ctlP;
	CharPtr		label;
	char		index=-1;
	char		i, j;
	FieldPtr 	fld;
	CharPtr		fldText;
	char		fldChar;

		
	for (j=0; j<MAX_RES_CHAR_NUM; j++) {
		if (appSetting.Reserved_chars[j].format == Msr5BitsFormat)
			index = 0;
		else if (appSetting.Reserved_chars[j].format == Msr7BitsFormat)
			index = 1;
		else if (!appSetting.Reserved_chars[j].format)
			break;
		else return (MsrMgrErrRes);
		
		listP = GetObjectPtr(objIdx[0][j]);
		label = LstGetSelectionText (listP, index);
		ctlP = GetObjectPtr (objIdx[1][j]);
		CtlSetLabel (ctlP, label);
    	LstSetSelection(listP, index);
    }

	// show reserved characters	
	for (i=0; i<2; i++)
	for (j=0; j<MAX_RES_CHAR_NUM; j++) {
		fld = GetObjectPtr(objIdx[i+2][j]);
		// Lock down the handle and get a pointer to the memory chunk.
		fldText = MemHandleLock(resCharFldHandle[i][j]);
		// clear field
		*fldText = NULL;	
		FldSetTextHandle(fld, resCharFldHandle[i][j]);
		FldDrawField(fld);
	
		if (!appSetting.Reserved_chars[j].format) {
			// Unlock the new memory chunk.
			MemHandleUnlock(resCharFldHandle[i][j]);		
			continue;
		};
		
		if (i==0)
			fldChar = appSetting.Reserved_chars[j].SR_Bits;
		else
			fldChar = appSetting.Reserved_chars[j].SR_Chars;
		// Sp is invisible, show it as /XXX
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
		MemHandleUnlock(resCharFldHandle[i][j]);		
		// Set the field's text to the data in the new memory chunk.
		FldSetTextHandle(fld, resCharFldHandle[i][j]);
		FldDrawField(fld);
	}		
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    SetReservedCharacter
 *
 * DESCRIPTION: This routine is the routine 
 *              to set the current Reserved character.
 *
 * PARAMETERS:  frmP	- a pointer to current form
 *				fieldIdx- field objects name for reserved character
 *
 * RETURNED:    MsrMgrNormal
 *              Other 
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err SetReservedCharacter(FormPtr frmP, Word objIdx[4][6])
{
	ListPtr 	listP;
	char		index = -1;
	VoidHand 	fldHandle;
	FieldPtr 	fld;
	CharPtr		inputText;
	Byte		i, j, field;
	Byte		format[MAX_RES_CHAR_NUM];
	Byte		inputChar[2][MAX_RES_CHAR_NUM]={{0,0,0,0,0,0},{0,0,0,0,0,0}};
	Err			error;
													
	for (field=0; field<MAX_RES_CHAR_NUM; field++) { 
		// get format for each reserved character
		listP = GetObjectPtr(objIdx[0][field]);
		index = LstGetSelection (listP);
		format[field] = formatIdx[index];
		
		for (i=0; i<2; i++) {
			// get SS_B pattern 		
			fld = GetObjectPtr(objIdx[i+2][field]);
			fldHandle = FldGetTextHandle(fld);
			// Lock down the handle and get a pointer to the memory chunk.
			inputText = MemHandleLock(fldHandle);
			// input field length check
			if (StrLen(inputText)==0) {
				// Unlock the new memory chunk.
				MemHandleUnlock(fldHandle);
				format[field] = NULL;
				break;
			}
			if (StrLen(inputText)>4) {
				// Unlock the new memory chunk.
				MemHandleUnlock(fldHandle);
				return(MsrMgrErrParam);
			}
			if (*inputText=='/') {
				for (j=0;j<StrLen(inputText)-1;j++)
					inputChar[i][field] = inputChar[i][field]*10 + *(inputText+j+1) - '0';
			}
			else {
				// check redundant characters
				if (StrLen(inputText) > 1) {
					// Unlock the new memory chunk.
					MemHandleUnlock(fldHandle);
					return ( MsrMgrErrParam);
				}
				inputChar[i][field] = *inputText;
			}
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);
			// value range check	
			if (inputChar[i][field] > 255)
				return (MsrMgrErrParam);
		}
	}
	for (i=0; i<field; i++)
		if (appSetting.Reserved_chars[i].format != format[i] ||
			appSetting.Reserved_chars[i].SR_Bits != inputChar[0][i] ||
			appSetting.Reserved_chars[i].SR_Chars != inputChar[1][i]) 
		{
			for (j=i; j<field; j++){		
				appSetting.Reserved_chars[j].format = format[j];
				appSetting.Reserved_chars[j].SR_Bits = inputChar[0][j];
				appSetting.Reserved_chars[j].SR_Chars = inputChar[1][j];
			}		
				// Set MSR 3000
			error = MsrSetReservedChar(GMsrMgrLibRefNum, appSetting.Reserved_chars);
			if (error) {
				MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
				return (error);
			}
		}
			
	return (MsrMgrNormal);
}


