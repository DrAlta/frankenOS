/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ExpAttendees.c
 *
 * Description:
 *	  This is the Expense application's Attendee module.
 *
 * History:
 *		January 2, 1995	Created by Art Lamb
 *
 *****************************************************************************/

#include <PalmOS.h>
#include <FntGlue.h>

#include "Expense.h"


/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
#define AttendeesFont	NoteFont


/***********************************************************************
 *
 * FUNCTION:    CallAddressApp
 *
 * DESCRIPTION: This routine sends the Address application a launch 
 *              code to lookup an email address
 *
 * PARAMETERS:  key - key to search for
 *
 * RETURNED:    handle of string return by address application.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/26/96	Initial Revision
 *
 ***********************************************************************/
#if EMULATION_LEVEL == EMULATION_NONE
static MemHandle CallAddressApp (Char * keyP)
#else
static MemHandle CallAddressApp (Char *)
#endif
{
#if EMULATION_LEVEL == EMULATION_NONE
	Err						err;
	UInt16					cardNo;
	UInt32					result;
	LocalID					dbID;
	DmSearchStateType		searchState;
	AddrLookupParamsType	params;
			
	// Get the card number and database id of the Address application.
	err = DmGetNextDatabaseByTypeCreator (true, &searchState, 
		sysFileTApplication, sysFileCAddress, true, &cardNo, &dbID);
	ErrFatalDisplayIf(err, "Address app not found");
	if (err) return (0);


	// parameters for the lookup
	params.title = MemHandleLock (DmGetResource (strRsc, 
							attendeesLookupTitleStrID));

	params.pasteButtonText = MemHandleLock (DmGetResource (strRsc, 
							attendeesLookupAddStrID));

	params.formatStringP = MemHandleLock (DmGetResource (strRsc, 
							attendeesLookupFormatStrID));

	params.field1 = addrLookupSortField;
	params.field2 = addrLookupCompany;
	params.field2Optional = true;
	params.userShouldInteract = true;
			
	if (keyP)
		{
		MemSet(params.lookupString, addrLookupStringLength, 0);	
		StrNCopy(params.lookupString, keyP, addrLookupStringLength - 1);
		}
	else
		*params.lookupString = 0;

	// <RM> 1-19-98, Fixed to pass 0 for flags, instead of sysAppLaunchFlagSubCall
	//  The sysAppLaunchFlagSubCall flag is for internal use by SysAppLaunch only
	//  and should NOT be set  on entry.
	err = SysAppLaunch (cardNo, dbID, 0 ,
				sysAppLaunchCmdLookup, (MemPtr)&params, &result);

	ErrFatalDisplayIf(err, "Error sending lookup action to app");

	MemPtrUnlock (params.title);
	MemPtrUnlock (params.pasteButtonText);
	MemPtrUnlock (params.formatStringP);

	if (err) return (0);
	
	return (params.resultStringH);

#else
	{
	Char * p;
	Char * str = "#LOOKUP#";
	MemHandle h;
	
	h = MemHandleNew (StrLen(str) + 1);
	p = MemHandleLock (h);
	StrCopy (p, str);
	MemPtrUnlock (p);
	return (h);
	}

#endif
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesLookup
 *
 * DESCRIPTION: This routine called the Address Book application to
 *              lookup a attendees name, title and company.
 *
 * PARAMETERS:  fld - field object
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/23/96	Initial Revision
 *			roger	10/12/98	Initialize KeyP to NULL for cases where it wasn't initialized at all.
 *			kwk	05/16/99	Use TxtWordBounds if no selected text.
 *
 ***********************************************************************/
static void AttendeesLookup (void)
{
	UInt16		len;
	UInt16		pos;
	UInt16		end;
	UInt16		start;
	UInt16		resultLen;
	UInt32		token = ', \0\0';			// ", "
	Char* 		MemPtr;
	Char*			keyP = NULL;
	Char*			text;
	Char*			resultP;
	FieldPtr 	fld;
	MemHandle	keyH = 0;
	MemHandle	resultH;
	Boolean		isSelection = true;

	fld = GetObjectPtr (AttendeesField);

	// If there is a selection then use it as the key.
	text = FldGetTextPtr (fld);
	FldGetSelection (fld, &start, &end);
	if (start == end)
		{
		isSelection = false;
		pos = FldGetInsPtPosition (fld);
		if (text && pos)
			{
			UInt32 wstart;
			UInt32 wend;

			len = FldGetTextLength (fld);

			start = pos;
			end = start;
			
			if (TxtWordBounds (text, len, pos, &wstart, &wend))
				{
				start = wstart;
				end = wend;
				}
			else if (pos > 0)
				{
				pos -= TxtPreviousCharSize(text, pos);
				if (TxtWordBounds (text, len, pos, &wstart, &wend))
					{
					start = wstart;
					end = wend;
					}
				}
			}
		}

	// Copy the key value for the string.
	if (start != end)
		{
		keyH = MemHandleNew (end - start + 1);
		keyP = MemHandleLock (keyH);
		MemMove (keyP, &text[start], end - start);
		keyP[end - start] = 0;
		}


	resultH = CallAddressApp (keyP);
	if (keyH) MemHandleFree (keyH);

	// Replace the key string with the value returned by the Address 
	//application.
	if (resultH)
		{
		resultP = MemHandleLock (resultH);

		// If the record did not contain all the field we requested we may have
		// pairs of ", " together.  We need to remove the extra commas and spaces.  
		MemPtr = StrStr (resultP, (Char*) &token);
		while (MemPtr)
			{
			MemPtr += 2;
			len = StrLen (MemPtr);
			if (len && (StrNCompare (MemPtr, (Char*) &token, 2) == 0))
				MemMove (MemPtr,	MemPtr+2, len-1);
			
			MemPtr = StrStr (MemPtr, (Char*) &token);
			}

		resultLen = StrLen(resultP);

		// I we can paste the result into the field through the clipboard so that
		// change can be undone with to "undo" command.
		if (resultLen <= cbdMaxTextLength)
			{
			ClipboardAddItem (clipboardText, resultP, resultLen);
			if (! isSelection)
				FldSetSelection (fld, start, end);
			FldPaste (fld);
			}
		else
			{
			if (start != end)
				FldDelete (fld, start, end);
			FldInsert (fld, resultP, resultLen);
			}

		MemHandleFree (resultH);
		}
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesUpdateScrollBar
 *
 * DESCRIPTION: This routine update the scroll bar.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *			gap	11/02/96	Fix case where field and scroll bars get out of sync
 *
 ***********************************************************************/
static void AttendeesUpdateScrollBar (void)
{
	UInt16 scrollPos;
	UInt16 textHeight;
	UInt16 fieldHeight;
	Int16 maxValue;
	FieldPtr fld;
	ScrollBarPtr bar;

	fld = GetObjectPtr (AttendeesField);
	bar = GetObjectPtr (AttendeesScrollBar);
	
	FldGetScrollValues (fld, &scrollPos, &textHeight,  &fieldHeight);

	if (textHeight > fieldHeight)
		{
		// On occasion, such as after deleting a multi-line selection of text,
		// the display might be the last few lines of a field followed by some
		// blank lines.  To keep the current position in place and allow the user
		// to "gracefully" scroll out of the blank area, the number of blank lines
		// visible needs to be added to max value.  Otherwise the scroll position
		// may be greater than maxValue, get pinned to maxvalue in SclSetScrollBar
		// resulting in the scroll bar and the display being out of sync.
		maxValue = (textHeight - fieldHeight) + FldGetNumberOfBlankLines (fld);
		}
	else
		maxValue = 0;

	SclSetScrollBar (bar, scrollPos, 0, maxValue, fieldHeight-1);
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesChangeFont
 *
 * DESCRIPTION: This routine changes the font used to display a attendees.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 ***********************************************************************/
static void AttendeesChangeFont (UInt16 controlID)
{
	FieldPtr fld;
	
	fld = GetObjectPtr (AttendeesField);
	
	if (controlID == AttendeesSmallFontButton)
		AttendeesFont = FntGlueGetDefaultFontID(defaultSmallFont);
	else
		AttendeesFont = FntGlueGetDefaultFontID(defaultLargeFont);
		
	// FldSetFont will redraw the field if it is visible.
	FldSetFont (fld, AttendeesFont);
	
	AttendeesUpdateScrollBar ();
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesLoadRecord
 *
 * DESCRIPTION: This routine loads a attendees from a to do record into 
 *              the attendees edit field.
 *
 * PARAMETERS:  frm - pointer to the Attendees View form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 ***********************************************************************/
static void AttendeesLoadRecord (void)
{
	UInt16 textOffset;
	UInt16 textAllocSize;
	FieldPtr fld;
	MemHandle recordH;
	Char * recordP;
	ExpenseRecordType record;
	
	// Get a pointer to the attendees field.
	fld = GetObjectPtr (AttendeesField);
	
	// Set the font used in the attendees field.
	FldSetFont (fld, AttendeesFont);
	
	ExpenseGetRecord (ExpenseDB, CurrentRecord, &record, &recordH);
	recordP = MemHandleLock (recordH);
	textOffset = record.attendees - recordP;
	textAllocSize = StrLen (record.attendees) + 1;  // one for null terminator
	MemHandleUnlock (recordH);
	MemHandleUnlock (recordH);		// was also locked in ExpenseGetRecord

	FldSetText (fld, recordH, textOffset, textAllocSize);
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesSave
 *
 * DESCRIPTION: This routine release any unused memory allocated for
 *              the attendees and mark the to do record dirty.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 ***********************************************************************/
static void AttendeesSave (void)
{
	UInt16		attr;
	FieldPtr fld;
	
	fld = GetObjectPtr (AttendeesField);

	// Was the attendees string modified by the user.
	if (FldDirty (fld))
		{
		// Release any free space in the attendees field.
		FldCompactText (fld);

		// Mark the record dirty.	
		DmRecordInfo (ExpenseDB, CurrentRecord, &attr, NULL, NULL);
		attr |= dmRecAttrDirty;
		DmSetRecordInfo (ExpenseDB, CurrentRecord, &attr, NULL);
		}


	// Clear the handle value in the field, otherwise the handle
	// will be freed when the form is disposed of,  this call also unlocks
	// the handle that contains the attendees string.
	FldSetTextHandle (fld, 0);
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesDoCommand
 *
 * DESCRIPTION: This routine preforms the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 ***********************************************************************/
static Boolean AttendeesDoCommand (UInt16 command)
{
//	FieldPtr fld;
	Boolean handled = true;
	
	switch (command)
		{
//		case attendeesPhoneLookupCmd:
//			fld = GetObjectPtr (AttendeesField);
//			PhoneNumberLookup (fld);
//			break;
			
		default:
			handled = false;
		}	

	return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesScroll
 *
 * DESCRIPTION: This routine scrolls the mote Attendees View a page or a 
 *              line at a time.
 *
 * PARAMETERS:  direction - winUp or dowm
 *              oneLine   - true if scrolling a single line
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 ***********************************************************************/
static void AttendeesScroll (Int16 linesToScroll)
{
	UInt16			blankLines;
	Int16				min;
	Int16				max;
	Int16				value;
	Int16				pageSize;
	FieldPtr			fld;
	ScrollBarPtr	bar;
	
	fld = GetObjectPtr (AttendeesField);

	if (linesToScroll < 0)
		{
		blankLines = FldGetNumberOfBlankLines (fld);
		FldScrollField (fld, -linesToScroll, winUp);
		
		// If there were blank lines visible at the end of the field
		// then we need to update the scroll bar.
		if (blankLines)
			{
			// Update the scroll bar.
			bar = GetObjectPtr (AttendeesScrollBar);
			SclGetScrollBar (bar, &value, &min, &max, &pageSize);
			if (blankLines > -linesToScroll)
				max += linesToScroll;
			else
				max -= blankLines;
			SclSetScrollBar (bar, value, min, max, pageSize);
			}
		}

	else if (linesToScroll > 0)
		FldScrollField (fld, linesToScroll, winDown);
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesPageScroll
 *
 * DESCRIPTION: This routine scrolls the message a page winUp or winDown.
 *
 * PARAMETERS:   direction     winUp or winDown
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 ***********************************************************************/
static void AttendeesPageScroll (WinDirectionType direction)
{
	Int16 value;
	Int16 min;
	Int16 max;
	Int16 pageSize;
	UInt16 linesToScroll;
	FieldPtr fld;
	ScrollBarPtr bar;

	fld = GetObjectPtr (AttendeesField);
	
	if (FldScrollable (fld, direction))
		{
		linesToScroll = FldGetVisibleLines (fld) - 1;
		FldScrollField (fld, linesToScroll, direction);

		// Update the scroll bar.
		bar = GetObjectPtr (AttendeesScrollBar);
		SclGetScrollBar (bar, &value, &min, &max, &pageSize);

		if (direction == winUp)
			value -= linesToScroll;
		else
			value += linesToScroll;
		
		SclSetScrollBar (bar, value, min, max, pageSize);
		return;
		}
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesInit
 *
 * DESCRIPTION: This routine initializes the Attendees View form.
 *
 * PARAMETERS:  frm - pointer to the Attendees View form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 ***********************************************************************/
static void AttendeesInit (FormPtr frm)
{
	UInt16 			controlID;
	FieldPtr 		fld;
	FieldAttrType	attr;
	
	AttendeesLoadRecord ();

	// Highlight the font push button.
	if (AttendeesFont == FntGlueGetDefaultFontID(defaultSmallFont))
		controlID = AttendeesSmallFontButton;
	else
		controlID = AttendeesLargeFontButton;

	FrmSetControlGroupSelection (frm, AttendeesFontGroup, controlID);

	// Have the field send events to maintain the scroll bar.
	fld = GetObjectPtr (AttendeesField);
	FldGetAttributes (fld, &attr);
	attr.hasScrollBar = true;
	FldSetAttributes (fld, &attr);
}


/***********************************************************************
 *
 * FUNCTION:    AttendeesHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Attendees View"
 *              of the Expense application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	8/22/96	Initial Revision
 *			CSS	06/22/99	Standardized keyDownEvent handling
 *								(TxtCharIsHardKey, commandKeyMask, etc.)
 *
 ***********************************************************************/
Boolean AttendeesHandleEvent (EventType * event)
{
	UInt16 pos;
	FormPtr frm;
	FieldPtr fld;
	Boolean handled = false;


	if (event->eType == keyDownEvent)
		{
		if	(	(!TxtCharIsHardKey(	event->data.keyDown.modifiers,
											event->data.keyDown.chr))
			&&	(EvtKeydownIsVirtual(event)))
			{
			if (event->data.keyDown.chr == vchrPageUp)
				{
				AttendeesPageScroll (winUp);
				handled = true;
				}
	
			else if (event->data.keyDown.chr == vchrPageDown)
				{
				AttendeesPageScroll (winDown);
				handled = true;
				}
			}
		}


	else if (event->eType == ctlSelectEvent)
		{		
		switch (event->data.ctlSelect.controlID)
			{
			case AttendeesDoneButton:
				AttendeesSave ();
				FrmUpdateForm (DetailsDialog, updateAttendees);
				FrmReturnToForm (DetailsDialog);
				handled = true;
				break;

			case AttendeesLookupButton:
				AttendeesLookup ();
				handled = true;
				break;

			case AttendeesSmallFontButton:
			case AttendeesLargeFontButton:
				AttendeesChangeFont (event->data.ctlSelect.controlID);
				handled = true;
				break;
			}
		}


	else if (event->eType == fldChangedEvent)
		{
		frm = FrmGetActiveForm ();
		AttendeesUpdateScrollBar ();
		handled = true;
		}
		

	else if (event->eType == menuEvent)
		{
		handled = AttendeesDoCommand (event->data.menu.itemID);
		}
		

	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		AttendeesInit (frm);
		FrmDrawForm (frm);
		AttendeesUpdateScrollBar ();
		FrmSetFocus (frm, FrmGetObjectIndex (frm, AttendeesField));
		handled = true;
		}
	

	else if (event->eType == frmGotoEvent)
		{
		ItemSelected = true;

		frm = FrmGetActiveForm ();
		AttendeesInit (frm);

		fld = GetObjectPtr (AttendeesField);
		pos = event->data.frmGoto.matchPos;
		FldSetScrollPosition (fld, pos);
		FldSetSelection (fld, pos, pos + event->data.frmGoto.matchLen);

		FrmDrawForm (frm);
		AttendeesUpdateScrollBar ();
		FrmSetFocus (frm, FrmGetObjectIndex (frm, AttendeesField));
		handled = true;
		}
			
	else if (event->eType == frmUpdateEvent)
		{
		frm = FrmGetActiveForm ();
		FrmDrawForm (frm);
		handled = true;
		}


	else if (event->eType == frmCloseEvent ||
				event->eType == frmSaveEvent)
		{
		if ( FldGetTextHandle (GetObjectPtr (AttendeesField)))
			AttendeesSave ();
		}

	
	else if (event->eType == sclRepeatEvent)
		{
		AttendeesScroll (event->data.sclRepeat.newValue - 
			event->data.sclRepeat.value);
		}

	return (handled);
}

