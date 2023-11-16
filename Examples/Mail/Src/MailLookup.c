/******************************************************************************
 *
 * Copyright (c) 1996-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MailLookup.c
 *
 * Description:
 *	  This is the Mail application's main module.
 *
 * History:
 *		June 26, 1996	Created by Art Lamb
 *
 *****************************************************************************/

#include <PalmOS.h>

#include "Mail.h"


/***********************************************************************
 *
 *	Internal Structutes
 *
 ***********************************************************************/


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
 *			art	06/26/96	Initial Revision
 *			kwk	12/02/98	Set userShouldInteract to true.
 *
 ***********************************************************************/
static MemHandle CallAddressApp (Char * keyP)
{
	Err						err = 0;
	UInt16					titleStrID;
	AddrLookupParamsType	params;


	// parameters for the lookup
	switch (EditViewField)
		{
		case editToField:		titleStrID = mailToLookupTitleStrID;		break;
		case editFromField:	titleStrID = mailFromLookupTitleStrID;		break;
		case editCCField:		titleStrID = mailCCLookupTitleStrID;		break;
		case editBCCField:	titleStrID = mailBCCLookupTitleStrID;		break;
		}

	MemSet(&params, sizeof(params), 0);

	params.title = MemHandleLock (DmGetResource (strRsc, titleStrID));

	params.pasteButtonText = MemHandleLock (DmGetResource (strRsc, 
							mailLookupAddStrID));

	params.formatStringP = MemHandleLock (DmGetResource (strRsc, 
							mailLookupFormatStrID));

	params.field1 = addrLookupSortField;
	params.field2 = addrLookupEmail;
	params.field2Optional = false;
	params.userShouldInteract = true;

	if (keyP)
		{
		MemSet(params.lookupString, addrLookupStringLength, 0);	
		StrNCopy(params.lookupString, keyP, addrLookupStringLength - 1);
		}
	else
		*params.lookupString = 0;
			

#if EMULATION_LEVEL == EMULATION_NONE
	{
	UInt16					cardNo;
	UInt32					result;
	LocalID					dbID;
	DmSearchStateType		searchState;

	// Get the card number and database id of the Address application.
	err = DmGetNextDatabaseByTypeCreator (true, &searchState, 
		sysFileTApplication, sysFileCAddress, true, &cardNo, &dbID);
	ErrFatalDisplayIf(err, "Address app not found");
	if (! err)
		{
	// <RM> 1-19-98, Fixed to pass 0 for flags, instead of sysAppLaunchFlagSubCall
	//  The sysAppLaunchFlagSubCall flag is for internal use by SysAppLaunch only
	//  and should NOT be set  on entry.
		err = SysAppLaunch (cardNo, dbID, 0,
				sysAppLaunchCmdLookup, (MemPtr)&params, &result);
		ErrFatalDisplayIf(err, "Error sending lookup action to app");
		}
	}

#else
	{
	Char * p;
	Char * str = ", #LOOKUP#";
	
	params.resultStringH = MemHandleNew (StrLen(str) + 1);
	p = MemHandleLock (params.resultStringH);
	StrCopy (p, str);
	MemPtrUnlock (p);
	}

#endif


	MemPtrUnlock (params.title);
	MemPtrUnlock (params.pasteButtonText);
	MemPtrUnlock (params.formatStringP);

	if (err) return (0);
	
	return (params.resultStringH);
}


/***********************************************************************
 *
 * FUNCTION:    AddressLookup
 *
 * DESCRIPTION: This routine initializes a row of the "Edit View" table.
 *
 * PARAMETERS:  fld - 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	06/26/96	Initial Revision
 *			meg	10/09/98 free the resultH to avoid a memory leak
 *			kwk	12/04/98	Modified to work for Japanese, plus fixed logic
 *								bug w/deciding when to search right for end of word.
 *
 ***********************************************************************/
void AddressLookup (FieldPtr fld)
{
	UInt16		pos;
	UInt16		end;
	UInt16		start;
	UInt16		resultLen;
	Char*			keyP;
	Char*			text;
	Char*			resultP;
	MemHandle		keyH;
	MemHandle		resultH;
	Boolean		isSelection = true;

	// DOLATER kwk - this routine should be modified to use TxtWordBounds.
	// If there is a selection then use it as the key.
	text = FldGetTextPtr (fld);
	FldGetSelection (fld, &start, &end);
	if (start == end)
		{
		isSelection = false;
		pos = FldGetInsPtPosition (fld);
		if (text && pos)
			{
			// Find the start of the key value. Search backwards from the 
			// insertion point position for a delimiter.
			start = pos;
			end = start;
			
			while (start > 0)
				{
				WChar theChar;
				UInt16 charSize = TxtGetPreviousChar(text, start, &theChar);
				
				if (TxtCharIsSpace(theChar) || TxtCharIsPunct(theChar))
					break;
				
				start -= charSize;
				}

			// If there are characters between the insertion point and the 
			// delimiter or start of the field if there are no deleimiter,
			// then find the end of the key.
			// DOLATER kwk - this seems wrong..basically this code doesn't search
			// right if the insertion point is one char to the right of the
			// start of the word.
			
//			if (start != pos - 1)

			if (start < pos)
				{
				UInt16 len = FldGetTextLength (fld);
				while (end < len) 
					{
					WChar theChar;
					UInt16 charSize = TxtGetNextChar(text, end, &theChar);
					
					if (TxtCharIsSpace(theChar) || TxtCharIsPunct(theChar))
						break;
					
					end += charSize;
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
	else
		keyP = NULL;


	resultH = CallAddressApp (keyP);
	if (keyP) MemPtrFree (keyP);

	// Replace the key string with the value returned by the Address 
	// application.
	if (resultH)
		{
		resultP = MemHandleLock (resultH);
		resultLen = StrLen(resultP);


		// Remove the comma and space delimiter, that are at the start of the 
		// string,  if there is already a comma,
		text = FldGetTextPtr (fld);
		pos = start;

		while (pos && text[pos-1] == spaceChr)
			pos--;
		if ((!pos) || text[pos-1] == commaChr || text[pos-1] == linefeedChr)
			{
			resultLen -= 2;
			MemMove (resultP, resultP+2, resultLen+1);
			}


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
			
		MemHandleFree(resultH);
		}
}

