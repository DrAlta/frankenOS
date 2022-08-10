/***********************************************************************
 *
 Copyright (c) 1999 Symbol Technologies, Inc.  
 All rights reserved.
   
 
 *************************************************************************
 *
 * PROJECT:  Palm III MSR
 * FILE:     SetTrack.C
 * AUTHOR:   H.Z.: Jan. 20, 1999
 *
 * DECLARER: Starter
 *
 * DESCRIPTION:
 *	  
 *
 **********************************************************************/
#define	NEW_SERIAL_MANAGER

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
 *   Internal Definition
 *
 ***********************************************************************/
#define	MAX_SEP_CHAR		4		// maximum character for track separator
#define	MAX_PREPOST_CHAR	16		// maximum character for pre-amble and post-amble
#define	TRACK_LIST_NUM		8		// number of item in track list

/***********************************************************************
 *
 *   Local variables
 *
 ***********************************************************************/
// index table for pop up menu
static	char	listIdx[TRACK_LIST_NUM] = { 
					MsrAnyTrack, MsrTrack1Only, 
					MsrTrack2Only, MsrTrack3Only, 
					MsrTrack1Track2, MsrTrack1Track3,
					MsrTrack2Track3, MsrAllThreeTracks };
						
/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/
static Err ShowTrackSelection(FormPtr frmP);
static Err SetTrackSelection(FormPtr frmP);
static Err ShowTerminator(FormPtr frmP);
static Err SetTerminator(FormPtr frmP);
static Err ShowSeparator(FormPtr frmP, VoidHand fldHandle);
static Err SetSeparator(FormPtr frmP);
static Err ShowPreamble(FormPtr frmP, VoidHand fldHandle);
static Err SetPreamble(FormPtr frmP);
static Err ShowPostamble(FormPtr frmP, VoidHand fldHandle);
static Err SetPostamble(FormPtr frmP);

/***********************************************************************
 *
 * FUNCTION:    SetTrackFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "TrackSettingForm" of this application.
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
Boolean SetTrackFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    
    Err			error;
    VoidHand	SepFldHandle=NULL;
    VoidHand	PreFldHandle=NULL;
    VoidHand	PostFldHandle=NULL;
    
	switch (eventP->eType) 
		{
		case menuEvent:
			break;

		case frmLoadEvent:
			FrmInitForm(SetTrackForm);			
			handled = true;
			break;
			
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm (frmP);
			// allocate handles for fields
			SepFldHandle = MemHandleNew(MAX_SEP_CHAR+1);
			PreFldHandle = MemHandleNew(MAX_PREPOST_CHAR+1);
			PostFldHandle = MemHandleNew(MAX_PREPOST_CHAR+1);
			// show current setting, and beep if error occurs
			error = MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			if (error) 
				SndPlaySystemSound(sndWarning);
			error = ShowTrackSelection(frmP);
			if (error)
				SndPlaySystemSound(sndWarning);
			error = ShowTerminator(frmP);
			if (error)
				SndPlaySystemSound(sndWarning);
			error = ShowSeparator(frmP, SepFldHandle);
			if (error)
				SndPlaySystemSound(sndWarning);
			error = ShowPreamble(frmP, PreFldHandle);
			if (error)
				SndPlaySystemSound(sndWarning);
			error = ShowPostamble(frmP, PostFldHandle);
			if (error)
				SndPlaySystemSound(sndWarning);
			
			handled = true;
			break;

		case frmCloseEvent:
			// free field handles
			if (SepFldHandle) {
				FldSetTextHandle(GetObjectPtr(SetTrackSeparatorField),NULL);
				MemHandleFree(SepFldHandle);
			}
			if (PreFldHandle) {
				FldSetTextHandle(GetObjectPtr(SetTrackPreambleField),NULL);
				MemHandleFree(PreFldHandle);
			}
			if (PostFldHandle) {
				FldSetTextHandle(GetObjectPtr(SetTrackPostambleField),NULL);
				MemHandleFree(PostFldHandle);
			}
			
			frmP = FrmGetActiveForm();
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			handled = true;
			break;

	   	case ctlSelectEvent:  // A control button was pressed and released.
			
	   		// If the OK button is pressed
	   		if (eventP->data.ctlEnter.controlID == SetTrackOKButton) {
				frmP = FrmGetActiveForm();
				
				error = SetTrackSelection(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				error = SetTerminator(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				error = SetSeparator(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				error = SetPreamble(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				error = SetPostamble(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}
				
				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
	   		if (eventP->data.ctlEnter.controlID == SetTrackCancelButton) {
				frmP = FrmGetActiveForm();
				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
		case keyDownEvent:
		
			if ((eventP->data.keyDown.chr==msrDataReadyKey))  {
			}
			break;
			
		default:
			break;
		}
	
	return handled;
}

/***********************************************************************
 *
 * FUNCTION:    ShowTrackSelection
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Track Selection.
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
static Err ShowTrackSelection(FormPtr frmP)
{
	ListPtr 	listP;
	ControlPtr	ctlP;
	CharPtr		label;
	char		i, index=-1;
	
	for (i=0; i<TRACK_LIST_NUM; i++)
		if (listIdx[i]==appSetting.Track_selection) {
			index = i;
			break;
		}
	
	if (index<0)
		return(-1);
		
	listP = GetObjectPtr(SetTrackSelectionList);
	label = LstGetSelectionText (listP, index);
	ctlP = GetObjectPtr (SetTrackTrackselectionPopTrigger);
	CtlSetLabel (ctlP, label);
    LstSetSelection(listP, index);
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    SetTrackSelection
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Track Selection.
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
static Err SetTrackSelection(FormPtr frmP)
{
	ListPtr 	listP;
	char		index;
		
	listP = GetObjectPtr(SetTrackSelectionList);
	index = LstGetSelection (listP);
	// Set MSR 3000, if track selection setting has be changed
	if (appSetting.Track_selection!=listIdx[index]) {
		appSetting.Track_selection=listIdx[index];
		return (MsrSetTrackSelection(GMsrMgrLibRefNum, listIdx[index]));
	}
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    ShowTerminator
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Terminator.
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
static Err ShowTerminator(FormPtr frmP)
{
    
	switch (appSetting.Terminator) {
		case MsrTerminatorCRLF:
			FrmSetControlGroupSelection(frmP, TrackSettingFormGroupID,SetTrackCRLFPushButton);
			break;
		case MsrTerminatorCR:
			FrmSetControlGroupSelection(frmP, TrackSettingFormGroupID,SetTrackCRPushButton);
			break;
		case MsrTerminatorLF:
			FrmSetControlGroupSelection(frmP, TrackSettingFormGroupID,SetTrackLFPushButton);
			break;
		case MsrTerminatorNone:
			FrmSetControlGroupSelection(frmP, TrackSettingFormGroupID,SetTrackNonePushButton);
			break;
		default:
			return (MsrMgrErrParam);
			break;
	}
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    SetTerminator
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to set Terminator.
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
static Err SetTerminator(FormPtr frmP)
{
	Byte	index;
				
	// get current track selection			
	index = FrmGetControlGroupSelection(frmP, TrackSettingFormGroupID);
	
	// CRLF ?
	if (index == FrmGetObjectIndex(frmP,SetTrackCRLFPushButton)
		&& appSetting.Terminator!= MsrTerminatorCRLF) {	
		appSetting.Terminator = MsrTerminatorCRLF;
		return(MsrSetTerminator(GMsrMgrLibRefNum, MsrTerminatorCRLF));			
	}
	
	// CR ?
	if (index == FrmGetObjectIndex(frmP,SetTrackCRPushButton)
		&& appSetting.Terminator != MsrTerminatorCR) {	
		appSetting.Terminator = MsrTerminatorCR;
		return (MsrSetTerminator(GMsrMgrLibRefNum, MsrTerminatorCR));			
	}
	
	// LF ?
	if (index == FrmGetObjectIndex(frmP,SetTrackLFPushButton)
		&& appSetting.Terminator != MsrTerminatorLF) {	
		appSetting.Terminator = MsrTerminatorLF;
		return (MsrSetTerminator(GMsrMgrLibRefNum, MsrTerminatorLF));			
	}
	
	// None ?
	if (index == FrmGetObjectIndex(frmP,SetTrackNonePushButton)
		&& appSetting.Terminator!= MsrTerminatorNone) {	
		appSetting.Terminator = MsrTerminatorNone;
		MsrSetTerminator(GMsrMgrLibRefNum, MsrTerminatorNone);			
	}
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    ShowSeparator
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Track Separator
 *				show ascii character if (0x20~0x7E)
 *				otherwise display as /xxx, xxx is a decimal digital
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
static Err ShowSeparator(FormPtr frmP, VoidHand fldHandle)
{
	FieldPtr 		fld;
	CharPtr			sepText;
	Byte			sepChar;
    
	fld = GetObjectPtr(SetTrackSeparatorField);
	// Lock down the handle and get a pointer to the memory chunk.
	sepText = MemHandleLock(fldHandle);
	// clear field
	*sepText = NULL;	
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);
	
	sepChar = appSetting.Track_separator;
	// space is invisble, show it as /XXX
	if (sepChar>0x20 && sepChar<0x7F) {
		*sepText++ = appSetting.Track_separator;
		*sepText = NULL;
	}
	else {
		*sepText++ = '/';
		*sepText++ = sepChar/100 + '0';
		sepChar = sepChar % 100;
		*sepText++ = sepChar/10 + '0';
		sepChar = sepChar % 10;
		*sepText++ = sepChar + '0';		
		*sepText = NULL;
	}
						
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);		
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);	
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    SetSeparator
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to set Track Separator.
 *				character can be either a ascii character
 *				or /xxx, xxx is a decimal code of this character
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
static Err SetSeparator(FormPtr frmP)
{
	VoidHand 		fldHandle;
	FieldPtr 		fld;
	CharPtr			sepText;
	int				i, sepChar=0;
    
	fld = GetObjectPtr(SetTrackSeparatorField);
	fldHandle = FldGetTextHandle(fld);
	// Lock down the handle and get a pointer to the memory chunk.
	sepText = MemHandleLock(fldHandle);
	if (*sepText=='/') {
		for (i=0;i<StrLen(sepText)-1;i++)
			sepChar = sepChar*10 + *(sepText+i+1) - '0';
		if (sepChar<0 || sepChar > 255) {
			MemHandleUnlock(fldHandle);
			return (MsrMgrErrParam);
		}
	}
	else {
		if (StrLen(sepText) > 1) {
			MemHandleUnlock(fldHandle);
			return (MsrMgrErrParam);		
		}
		sepChar = *sepText;
	}
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);
			
	if (appSetting.Track_separator != sepChar) {
		appSetting.Track_separator = sepChar;
		return (MsrSetTrackSeparator(GMsrMgrLibRefNum, sepChar));
	}
		
	return (MsrMgrNormal);	
}


/***********************************************************************
 *
 * FUNCTION:    ShowPreamble
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Preamble
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
static Err ShowPreamble(FormPtr frmP, VoidHand fldHandle)
{
	FieldPtr 		fld;
	CharPtr			preText;
	Byte			preChar, i;
    
	fld = GetObjectPtr(SetTrackPreambleField);
	// Lock down the handle and get a pointer to the memory chunk.
	preText = MemHandleLock(fldHandle);
	// clear field
	*preText = NULL;	
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);
	FldSetTextHandle(fld, NULL);
	
	for (i=0; i<StrLen(appSetting.Preamble); i++) {
		preChar = appSetting.Preamble[i];
		// space is invisible sometimes
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
 * FUNCTION:    SetPreamble
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to set Preamble.
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
static Err SetPreamble(FormPtr frmP)
{
	VoidHand 		fldHandle;
	FieldPtr 		fld;
	CharPtr			preText;
	char			i=0, preChar, preamble_str[MAX_PRE_POST_SIZE+1];
    
	fld = GetObjectPtr(SetTrackPreambleField);
	fldHandle = FldGetTextHandle(fld);
	// Lock down the handle and get a pointer to the memory chunk.
	preText = MemHandleLock(fldHandle);
	
	while (*preText!= NULL) {
		if (i==MAX_PRE_POST_SIZE) {
			MemHandleUnlock(fldHandle);	
			return(MsrMgrErrParam);
		}
		if (*preText>=0x20 && *preText<0x7F && *preText !='/') {
			preamble_str[i++]=*preText++;
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
			preamble_str[i++]=preChar;		
		else {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (MsrMgrErrParam);
		}		
	}
	preamble_str[i] = NULL;
	
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);	
	
	if (StrCompare(appSetting.Preamble, preamble_str)) {
		StrCopy(appSetting.Preamble, preamble_str);
		return (MsrSetPreamble(GMsrMgrLibRefNum, preamble_str));
	}
	
	return (MsrMgrNormal);	
}


/***********************************************************************
 *
 * FUNCTION:    ShowPostamble
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to show the current Postamble
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
static Err ShowPostamble(FormPtr frmP, VoidHand fldHandle)
{
	FieldPtr 		fld;
	CharPtr			postText;
	Byte			postChar, i;
    
	fld = GetObjectPtr(SetTrackPostambleField);
	// Lock down the handle and get a pointer to the memory chunk.
	postText = MemHandleLock(fldHandle);
	// clear field
	*postText = NULL;	
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);
	FldSetTextHandle(fld, NULL);
	
	for (i=0; i<StrLen(appSetting.Postamble); i++) {
		postChar = appSetting.Postamble[i];
		// space is invisble for last character
		if (postChar>0x20 && postChar<0x7F) {
			*postText++ = postChar;
		}
		else {
			*postText++ = '/';
			*postText++ = postChar/100 + '0';
			postChar = postChar % 100;
			*postText++ = postChar/10 + '0';
			postChar = postChar % 10;
			*postText++ = postChar + '0';
		}
	}
	*postText = NULL;
						
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);		
	// Set the field's text to the data in the new memory chunk.
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);	
	
	return (MsrMgrNormal);
}


/***********************************************************************
 *
 * FUNCTION:    SetPostamble
 *
 * DESCRIPTION: This routine is the dispaly routine 
 *              to set Postamble.
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
static Err SetPostamble(FormPtr frmP)
{
	VoidHand 		fldHandle;
	FieldPtr 		fld;
	CharPtr			postText;
	char			i=0, postChar, postamble_str[MAX_PRE_POST_SIZE+1];
    
	fld = GetObjectPtr(SetTrackPostambleField);
	fldHandle = FldGetTextHandle(fld);
	// Lock down the handle and get a pointer to the memory chunk.
	postText = MemHandleLock(fldHandle);
	
	while (*postText!= NULL) {
		if (i==MAX_PRE_POST_SIZE) {
			MemHandleUnlock(fldHandle);	
			return(MsrMgrErrParam);
		}
		if (*postText>=0x20 && *postText<0x7F && *postText !='/') {
			postamble_str[i++]=*postText++;
			continue;
		}
		// invisible character has to written as /xxx
		if (*postText!='/' || StrLen(postText)<4) {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (MsrMgrErrParam);
		}
		postChar = (*(postText+1)-'0')*100+(*(postText+2)-'0')*10+*(postText+3)-'0';
		postText += 4;
		// check character range
		if (postChar>0 && postChar<256)
			postamble_str[i++]=postChar;		
		else {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (MsrMgrErrParam);
		}		
	}
	postamble_str[i] = NULL;
	
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);	
	
	if (StrCompare(appSetting.Postamble, postamble_str)) {
		StrCopy(appSetting.Postamble, postamble_str);
		return (MsrSetPostamble(GMsrMgrLibRefNum, postamble_str));
	}
	
	return (MsrMgrNormal);	
}


