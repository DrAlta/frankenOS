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

static Err ShowSendCmd(FormPtr frmP, VoidHand fldHandle);
static Err SetSendCommand(FormPtr frmP);

#define	MAX_SCMDFLD	320		// maximum character in command field

/***********************************************************************
 *
 * FUNCTION:    SendCommandHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "SendCommandForm" of this application.
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
Boolean SendCommandFormHandleEvent(EventPtr eventP)
{
    Boolean 	handled = false;
    FormPtr 	frmP;
    Err			error;
    
    VoidHand	SCmdFldHandle=NULL;

	switch (eventP->eType) 
		{
		case menuEvent:
			break;

		case frmLoadEvent:
			frmP = FrmInitForm(SendCommandForm);
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm ( frmP);

			// allocate handles for fields
			SCmdFldHandle = MemHandleNew(MAX_SCMDFLD+1);
			// show current setting, beep if error occurs
			error = MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			if (error)
				SndPlaySystemSound(sndWarning);
				
			error = ShowSendCmd(frmP, SCmdFldHandle);
			if (error)
				SndPlaySystemSound(sndWarning);
			// set focus to send command field
			FrmSetFocus(frmP, FrmGetObjectIndex(frmP, SendCommandSendCommandField));
			handled = true;
			break;

		case frmCloseEvent:
			// free field handles
			if (SCmdFldHandle) {
				FldSetTextHandle(GetObjectPtr(SendCommandSendCommandField),NULL);
				MemHandleFree(SCmdFldHandle);
			}
			frmP = FrmGetActiveForm();
			FrmEraseForm (frmP);
			FrmDeleteForm (frmP);
			handled = true;
			break;
			
	   	case ctlSelectEvent:  // A control button was pressed and released.
			
	   		// If the OK button is pressed
	   		if (eventP->data.ctlEnter.controlID == SendCommandOKButton) {
				frmP = FrmGetActiveForm();
				// set send command if change has been made
				error = SetSendCommand(frmP);
				if (error) {
					SndPlaySystemSound(sndWarning);
					handled = true;
					break;
				}

				FrmGotoForm(MainForm);
				handled = true;
				break;
			}
			
	   		if (eventP->data.ctlEnter.controlID == SendCommandCancelButton) {
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
    
	fld = GetObjectPtr(SendCommandSendCommandField);
	// Lock down the handle and get a pointer to the memory chunk.
	preText = MemHandleLock(fldHandle);
	// clear field
	*preText = NULL;	
	FldSetTextHandle(fld, fldHandle);
	FldDrawField(fld);
	// all byte in command are show as /xxx
	// xxx is a decimal digital of ascii code for this byte
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
 * FUNCTION:    SetSendCommand
 *
 * DESCRIPTION: This routine is 
 *              to set Send Command.
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
static Err SetSendCommand(FormPtr frmP)
{
	VoidHand 		fldHandle;
	FieldPtr 		fld;
	CharPtr			preText;
	char			scmd_str[MAX_SCMD_NUM][MAX_SCMD_LEN];
	unsigned char	i=0, j=0, preChar;
	Err				error;
    
	fld = GetObjectPtr(SendCommandSendCommandField);
	fldHandle = FldGetTextHandle(fld);
	// Lock down the handle and get a pointer to the memory chunk.
	preText = MemHandleLock(fldHandle);
	
	while (*preText!= NULL && i<MAX_SCMD_NUM && j<=MAX_SCMD_LEN) {
		// next SCMD field
		if (*preText=='\n') {
			scmd_str[i++][j] = NULL;
			j = 0;
			preText++;
			continue;
		}
		// all character has to written as /xxx
		if (*preText!='/' || StrLen(preText)<4) {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (MsrMgrErrParam);
		}
		preChar = (*(preText+1)-'0')*100+(*(preText+2)-'0')*10+*(preText+3)-'0';
		preText += 4;
		// check character range
		if (preChar>=0 && preChar<256)
			scmd_str[i][j++]=preChar;		
		else {
			// Unlock the new memory chunk.
			MemHandleUnlock(fldHandle);	
			return (MsrMgrErrParam);
		}		
	}
	scmd_str[i][j] = NULL;
	
	preChar = *preText;	
	// Unlock the new memory chunk.
	MemHandleUnlock(fldHandle);	
	
	// check input after line MAX_SCMD_NUM
	if (preChar!=NULL && i>=MAX_SCMD_NUM)
		return (MsrMgrErrParam);
	
	for (i=0; i<MAX_SCMD_NUM; i++) {
		if (MemCmp(appSetting.Send_cmd[i], scmd_str[i], scmd_str[i][0]+2)) {
			for (j=0; j<MAX_SCMD_NUM; j++)
				MemMove(appSetting.Send_cmd[j], scmd_str[j], scmd_str[j][0]+2);
			error = MsrSetDataEditSend(GMsrMgrLibRefNum, scmd_str);
			if (error)
				MsrGetSetting( GMsrMgrLibRefNum, &appSetting);
			return (error);			
		}
	}
	
	return (MsrMgrNormal);	
}

