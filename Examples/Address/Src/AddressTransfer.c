/******************************************************************************
 *
 * Copyright (c) 1997-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: AddressTransfer.c
 *
 * Description:
 *      Address Book routines to transfer records.
 *
 * History:
 *      4/24/97		roger	Created
 *		7/16/99		bhall	Added support for send/receive of category name
 *
 *****************************************************************************/

// Set this to get to private database defines
#define __ADDRMGR_PRIVATE__

// DOLATER kwk - decide if all vCard parsing only involves single-byte
// ascii, and thus we don't have to worry about int'l charset issues.
// NOTE bhall - moved this include (CharAttr) to above PalmOS.h so that the define would be respected
#define	NON_INTERNATIONAL
#include <CharAttr.h>

#include <PalmOS.h>


#include "Address.h"
#include "AddressRsc.h"


#define identifierLengthMax			40
#define addrFilenameExtension		"vcf"
#define addrFilenameExtensionLength	3


// Stream interface to exgsockets to optimize performance
#define maxStreamBuf 512
 
typedef struct StreamType {
	ExgSocketPtr socket;
	UInt16 pos;
	UInt16 len;
	UInt16 bufSize;
	Char   buf[maxStreamBuf];
} StreamType;

/***********************************************************************
 *
 * FUNCTION:    PrvStreamInit
 *
 * DESCRIPTION: Function to put Initialize a stream socket. 
 *
 * PARAMETERS:  streamP - the output stream
 *				exgSocketP - pointer to an intitialized exgSocket
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *		   gavin   10/5/99   Initial revision
 *
 ***********************************************************************/
static void PrvStreamInit(StreamType *streamP, ExgSocketPtr exgSocketP)
{
	streamP->socket = exgSocketP;
	streamP->bufSize = maxStreamBuf;
	streamP->pos = 0;
	streamP->len = 0;
}

/***********************************************************************
 *
 * FUNCTION:    PrvStreamFlush
 *
 * DESCRIPTION: Function to put a string to the exg transport. 
 *
 * PARAMETERS:  streamP - the output stream
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *		   gavin   10/5/99   Initial revision
 *
 ***********************************************************************/
static void PrvStreamFlush(StreamType *streamP)
{
	Err err = 0;
	while (streamP->len && !err)
		streamP->len -= ExgSend(streamP->socket,streamP->buf,streamP->len,&err);

}

/***********************************************************************
 *
 * FUNCTION:    PrvStreamWrite
 *
 * DESCRIPTION: Function to put a string to the exg transport. 
 *
 * PARAMETERS:  streamP - the output stream
 *				stringP - the string to put
 *
 * RETURNED:    nothing
 *				If the all the string isn't sent an error is thrown using ErrThrow.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *		   gavin   10/5/99   Initial revision
 *
 ***********************************************************************/
static void PrvStreamWrite(StreamType *streamP, const Char * stringP)
{
	Err err = 0;
	
	while (*stringP && !err)
	{
		if (streamP->len < streamP->bufSize) 
			streamP->buf[streamP->len++] = *stringP++;
		else
		    streamP->len -= ExgSend(streamP->socket, streamP->buf, streamP->len, &err);
	}
	// If the bytes were not sent throw an error.
	if (*stringP || err)
		ErrThrow(err);
}

/***********************************************************************
 *
 * FUNCTION:    PrvStreamGetChar
 *
 * DESCRIPTION: Function to get a character from the input stream. 
 *
 * PARAMETERS:  streamP - the output stream
 *
 * RETURNED:    a character of EOF if no more data
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *		   gavin   10/5/99   Initial revision
 *
 ***********************************************************************/
static UInt16 PrvStreamGetChar(StreamType * streamP)
{
	UInt8 c;
	Err err=0;
	
	if (streamP->pos < streamP->len)
		c = streamP->buf[streamP->pos++];
	else
	{	streamP->pos = 0;
		streamP->len = ExgReceive(streamP->socket, streamP->buf, streamP->bufSize, &err);
		if (streamP->len && !err)
			c = streamP->buf[streamP->pos++];
		else
			return EOF;
	}
	return c;
}

/***********************************************************************
 *
 * FUNCTION:    PrvStreamSocket
 *
 * DESCRIPTION: returns the socket from a stream. 
 *
 * PARAMETERS:  streamP - the output stream
 *
 * RETURNED:    The socket associated with the stream
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *		   gavin   10/5/99   Initial revision
 *
 ***********************************************************************/
static ExgSocketPtr PrvStreamSocket(StreamType *streamP)
{
	return streamP->socket;
}


/***********************************************************************
 *
 * FUNCTION:		AddrRegisterData
 *
 * DESCRIPTION:		Register with the exchange manager to receive data
 * with a certain name extension.
 *
 * PARAMETERS:		nothing
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf		12/2/97		Created
 *
 ***********************************************************************/
extern void AddrRegisterData ()
{
	ExgRegisterData(sysFileCAddress, exgRegExtensionID, addrFilenameExtension);
}

/***********************************************************************
 *
 * FUNCTION:    GetChar
 *
 * DESCRIPTION: Function to get a character from the input stream. 
 *
 * PARAMETERS:  streamP - the stream connection
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   8/15/97   Initial Revision
 *		   gavin   10/5/99   use stream functions  
 *
 ***********************************************************************/
static UInt16 GetChar(const void * streamP)
{
	return PrvStreamGetChar((StreamType *)streamP);
}

/***********************************************************************
 *
 * FUNCTION:    PutString
 *
 * DESCRIPTION: Function to put a string to the exg transport. 
 *
 * PARAMETERS:  streamP - the output stream
 *				stringP - the string to put
 *
 * RETURNED:    nothing
 *				If the all the string isn't sent an error is thrown using ErrThrow.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   8/15/97   Initial Revision
 *		   gavin   10/5/99   use stream functions  
 *
 ***********************************************************************/
static void PutString(void *streamP, const Char * const stringP)
{
	PrvStreamWrite(streamP, stringP);
}



/***********************************************************************
 *
 * FUNCTION:    AddrSendRecordTryCatch
 *
 * DESCRIPTION: Send a record.  
 *
 * PARAMETERS:	dbP - pointer to the database to add the record to
 * 				recordNum - the record number to send
 * 				recordP - pointer to the record to send
 * 				outputStream - place to send the data
 *
 * RETURNED:    0 if there's no error
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  12/11/97  Initial Revision
 *
 ***********************************************************************/
static Err AddrSendRecordTryCatch (DmOpenRef dbP, Int16 recordNum, 
	AddrDBRecordPtr recordP, void * outputStream)
{
	Err error = 0;
	
	// An error can happen anywhere during the send process.  It's easier just to 
	// catch the error.  If an error happens, we must pass it into ExgDisconnect.
	// It will then cancel the send and display appropriate ui.
	ErrTry
		{
		AddrExportVCard(dbP, recordNum, recordP, outputStream, PutString, true);
		}
	
	ErrCatch(inErr)
		{
		error = inErr;
		} ErrEndCatch
	
	
	return error;
}


/***********************************************************************
 *
 * FUNCTION:    AddrSendRecord
 *
 * DESCRIPTION: Send a record.  
 *
 * PARAMETERS:	dbP - pointer to the database to add the record to
 * 				recordNum - the record to send
 *
 * RETURNED:    true if the record is found and sent
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   5/9/97   Initial Revision
 *
 ***********************************************************************/
extern void AddrSendRecord (DmOpenRef dbP, Int16 recordNum)
{
	AddrDBRecordType record;
	MemHandle recordH;
	MemHandle descriptionH;
	UInt16 descriptionSize = 0;
	Int16 descriptionWidth;
	Boolean descriptionFit;
	UInt16 newDescriptionSize;
	MemHandle nameH;
	Err error;
	ExgSocketType exgSocket;
	StreamType    stream;
	
	// important to init structure to zeros...
	MemSet(&exgSocket, sizeof(exgSocket), 0);
	
	// Form a description of what's being sent.  This will be displayed
	// by the system send dialog on the sending and receiving devices.
	error = AddrGetRecord (dbP, recordNum, &record, &recordH);
	ErrNonFatalDisplayIf(error, "Can't get record");
	
	if (RecordContainsData(&record))
		{
		// Figure out whether a person's name or company should be displayed.
		descriptionH = NULL;
		exgSocket.description = NULL;
		if (!(SortByCompany && record.fields[company]) &&
			(record.fields[name] || record.fields[firstName]))
			{
			if (record.fields[name] && record.fields[firstName])
				descriptionSize = sizeOf7BitChar(' ') + sizeOf7BitChar('\0');
			else
				descriptionSize = sizeOf7BitChar('\0');
			
			if (record.fields[name])
				descriptionSize += StrLen(record.fields[name]);
			
			if (record.fields[firstName])
				descriptionSize += StrLen(record.fields[firstName]);
			
			
			descriptionH = MemHandleNew(descriptionSize);
			if (descriptionH)
				{
				exgSocket.description = MemHandleLock(descriptionH);
				
				if (record.fields[firstName])
					{
					StrCopy(exgSocket.description, record.fields[firstName]);
					if (record.fields[name])
						StrCat(exgSocket.description, " ");
					}
				else
					exgSocket.description[0] = '\0';
				
				if (record.fields[name])
					{
					StrCat(exgSocket.description, record.fields[name]);
					}
				}
			
			}
	   else if (record.fields[company])
	     	{
			descriptionSize = StrLen(record.fields[company]) + sizeOf7BitChar('\0');
			
			descriptionH = MemHandleNew(descriptionSize);
			if (descriptionH)
				{
				exgSocket.description = MemHandleLock(descriptionH);
				StrCopy(exgSocket.description, record.fields[company]);
				}
			}
		
		// Truncate the description if too long
		if (descriptionSize > 0)
			{
			// Make sure the description isn't too long.
			newDescriptionSize = descriptionSize;
			WinGetDisplayExtent(&descriptionWidth, NULL);
			FntCharsInWidth (exgSocket.description, &descriptionWidth, (Int16 *)&newDescriptionSize, &descriptionFit);
			
			if (newDescriptionSize > 0)
				{
				if (newDescriptionSize != descriptionSize)
					{
					exgSocket.description[newDescriptionSize] = nullChr;
					MemHandleUnlock(descriptionH);
					MemHandleResize(descriptionH, newDescriptionSize + sizeOf7BitChar('\0'));
					exgSocket.description = MemHandleLock(descriptionH);
					}
				}
			else
				{
				MemHandleFree(descriptionH);
				}
			descriptionSize = newDescriptionSize;
			}
		
		// Make a filename
		if (descriptionSize > 0)
			{
			// Now make a filename from the description
			nameH = MemHandleNew(imcFilenameLength);
			exgSocket.name = MemHandleLock(nameH);
			StrNCopy(exgSocket.name, exgSocket.description, 
				min(descriptionSize+1, imcFilenameLength - addrFilenameExtensionLength - sizeOf7BitChar('.')));
			exgSocket.name[imcFilenameLength - 1 - addrFilenameExtensionLength - sizeOf7BitChar('.')] = nullChr;
			StrCat(exgSocket.name, ".");
			StrCat(exgSocket.name, addrFilenameExtension);
			}
		else
			{
			// A description is needed.  Either there never was one or the first line wasn't usable.
			descriptionH = DmGetResource(strRsc, BeamDescriptionStr);
			exgSocket.description = MemHandleLock(descriptionH);
			
			nameH = DmGetResource(strRsc, BeamFilenameStr);
			exgSocket.name = MemHandleLock(nameH);
			}
		
		
		exgSocket.length = MemHandleSize(recordH) + 100;		// rough guess
		exgSocket.target = sysFileCAddress;
		error = ExgPut(&exgSocket);   // put data to destination
		
		// use an extra stream layer on top of the exchange routines to improve performance
		PrvStreamInit(&stream,&exgSocket);
		if (!error)
			{
			error = AddrSendRecordTryCatch(dbP, recordNum, &record, &stream);

			PrvStreamFlush(&stream);  // make sure all data is written
			ExgDisconnect(&exgSocket, error);
			}
		
		// Clean up
		if (descriptionH)
			{
			MemHandleUnlock (descriptionH);
			if (MemHandleDataStorage (descriptionH))
				DmReleaseResource(descriptionH);
			else
				MemHandleFree(descriptionH);
			}
		if (nameH)
			{
			MemHandleUnlock (nameH);
			if (MemHandleDataStorage (nameH))
				DmReleaseResource(nameH);
			else
				MemHandleFree(nameH);
			}
		}
	else
		FrmAlert(NoDataToBeamAlert);
	
	
	MemHandleUnlock(recordH);
	DmReleaseRecord(dbP, recordNum, false);
	
	
	return;
}




/***********************************************************************
 *
 * FUNCTION:    AddrSendCategoryTryCatch
 *
 * DESCRIPTION: Send all visible records in a category.  
 *
 * PARAMETERS:	dbP - pointer to the database to add the record to
 * 				categoryNum - the category of records to send
 * 				exgSocketP - the exchange socket used to send
 * 				index - the record number of the first record in the category to send
 *
 * RETURNED:    0 if there's no error
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   5/9/97   Initial Revision
 *
 ***********************************************************************/
static Err AddrSendCategoryTryCatch (DmOpenRef dbP, UInt16 categoryNum, 
	void * outputStream, UInt16 index)
{
	volatile Err error = 0;
	volatile MemHandle outRecordH = 0;
	AddrDBRecordType outRecord;
	
	// An error can happen anywhere during the send process.  It's easier just to 
	// catch the error.  If an error happens, we must pass it into ExgDisconnect.
	// It will then cancel the send and display appropriate ui.
	ErrTry
		{

		// Loop through all records in the category.
		while (DmSeekRecordInCategory(dbP, &index, 0, dmSeekForward, categoryNum) == 0)
			{
			// Emit the record.  If the record is private do not emit it.
			if (AddrGetRecord(dbP, index, &outRecord, (MemHandle*)&outRecordH) == 0)
				{
				AddrExportVCard(dbP, index, &outRecord, outputStream, PutString, true);
				
				MemHandleUnlock(outRecordH);
				}
			
			index++;
			}
		}
	
	ErrCatch(inErr)
		{
		error = inErr;
		
		if (outRecordH)
			MemHandleUnlock(outRecordH);
		} ErrEndCatch
	
	return error;	
}


/***********************************************************************
 *
 * FUNCTION:    AddrSendCategory
 *
 * DESCRIPTION: Send all visible records in a category.  
 *
 * PARAMETERS:	dbP - pointer to the database to add the record to
 * 				categoryNum - the category of records to send
 *
 * RETURNED:    true if any records are found and sent
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   5/9/97   Initial Revision
 *
 ***********************************************************************/
extern void AddrSendCategory (DmOpenRef dbP, UInt16 categoryNum)
{
	Err error;
	Char description[dmCategoryLength];
	UInt16 index;
	Boolean foundAtLeastOneRecord;
	ExgSocketType exgSocket;
	UInt16 mode;
	LocalID dbID;
	UInt16 cardNo;
	Boolean databaseReopened;
	StreamType stream;
	
	// If the database was opened to show secret records, reopen it to not see 
	// secret records.  The idea is that secret records are not sent when a 
	// category is sent.  They must be explicitly sent one by one.
	DmOpenDatabaseInfo(dbP, &dbID, NULL, &mode, &cardNo, NULL);
	if (mode & dmModeShowSecret)
		{
		dbP = DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
		databaseReopened = true;
		}
	else
		databaseReopened = false;
	
	
	// important to init structure to zeros...
	MemSet(&exgSocket, sizeof(exgSocket), 0);
	
	
	// Make sure there is at least one record in the category.
	index = 0;
	foundAtLeastOneRecord = false;
	while (true)
		{
		if (DmSeekRecordInCategory(dbP, &index, 0, dmSeekForward, categoryNum) != 0)
			break;
		
		foundAtLeastOneRecord = DmQueryRecord(dbP, index) != 0;
		if (foundAtLeastOneRecord)
			break;
		
		
		index++;
		}
	
	
	// We should send the category because there's at least one record to send.
	if (foundAtLeastOneRecord)
		{
		// Form a description of what's being sent.  This will be displayed
		// by the system send dialog on the sending and receiving devices.
		CategoryGetName (dbP, categoryNum, description);
		exgSocket.description = description;
		
		// Now form a file name
		exgSocket.name = MemPtrNew(StrLen(description) + sizeOf7BitChar('.') + StrLen(addrFilenameExtension) + sizeOf7BitChar('\0'));
		if (exgSocket.name)
			{
			StrCopy(exgSocket.name, description);
			StrCat(exgSocket.name, ".");
			StrCat(exgSocket.name, addrFilenameExtension);
			}
		
		exgSocket.length = 0;		// rough guess
		exgSocket.target = sysFileCAddress;
		error = ExgPut(&exgSocket);   // put data to destination
		
		PrvStreamInit(&stream, &exgSocket);
		if (!error)
			{
			error = AddrSendCategoryTryCatch (dbP, categoryNum, &stream, index);
		
			PrvStreamFlush(&stream);
			ExgDisconnect(&exgSocket, error);
			}
		
		// Release file name
		if (exgSocket.name)
			MemPtrFree(exgSocket.name);
		}
	else
		FrmAlert(NoDataToBeamAlert);
	
	if (databaseReopened)
		DmCloseDatabase(dbP);
	
	return;
}


/***********************************************************************
 *
 * FUNCTION:		ReceiveData
 *
 * DESCRIPTION:		Receives data into the output field using the Exg API
 *
 * PARAMETERS:		exgSocketP, socket from the app code
 *						 sysAppLaunchCmdExgReceiveData
 *
 * RETURNED:		error code or zero for no error.
 *
 ***********************************************************************/
extern Err AddrReceiveData(DmOpenRef dbP, ExgSocketPtr exgSocketP)
{
	volatile Err err;	
	StreamType stream;
	
	// accept will open a progress dialog and wait for your receive commands
	err = ExgAccept(exgSocketP);
	
	if (!err)
		{
		PrvStreamInit(&stream, exgSocketP);
		// Catch errors receiving records.  The import routine will clean up the
		// incomplete record.  This routine passes the error to ExgDisconnect
		// which displays appropriate ui.
		ErrTry
			{
			// Keep importing records until it can't
			while (AddrImportVCard(dbP, &stream, GetChar, false, false))
				{};
			}
		
		ErrCatch(inErr)
			{
			err = inErr;
			} ErrEndCatch
		
		
		ExgDisconnect(exgSocketP, err); // closes transfer dialog
		}
	
	return err;
}


/***********************************************************************
 *
 * FUNCTION:		AddrSetGoToParams
 *
 * DESCRIPTION:	Store the information necessary to navigate to the 
 *                record inserted into the launch code's parameter block.
 *
 * PARAMETERS:		dbP        - pointer to the database to add the record to
 *					exgSocketP - parameter block passed with the launch code
 *					uniqueID   - unique id of the record inserted
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		10/17/97	Created
 *
 ***********************************************************************/
static void AddrSetGoToParams (DmOpenRef dbP, ExgSocketPtr exgSocketP, UInt32 uniqueID)
{
	UInt16		recordNum;
	UInt16		cardNo;
	LocalID 	dbID;

	
	if (! uniqueID) return;

	DmOpenDatabaseInfo (dbP, &dbID, NULL, NULL, &cardNo, NULL);

	// The this the the first record inserted, save the information
	// necessary to navigate to the record.
	if (! exgSocketP->goToParams.uniqueID)
		{
		DmFindRecordByID (dbP, uniqueID, &recordNum);

		exgSocketP->goToCreator = sysFileCAddress;
		exgSocketP->goToParams.uniqueID = uniqueID;
		exgSocketP->goToParams.dbID = dbID;
		exgSocketP->goToParams.dbCardNo = cardNo;
		exgSocketP->goToParams.recordNum = recordNum;
		}

	// If we already have a record then make sure the record index 
	// is still correct.  Don't update the index if the record is not
	// in your the app's database.
	else if (dbID == exgSocketP->goToParams.dbID &&
			   cardNo == exgSocketP->goToParams.dbCardNo)
		{
		DmFindRecordByID (dbP, exgSocketP->goToParams.uniqueID, &recordNum);

		exgSocketP->goToParams.recordNum = recordNum;
		}
}


/************************************************************
 *
 * FUNCTION: ReplaceSemicolonsWithNewlines
 *
 * DESCRIPTION: Replace all semicolons in a string with newlines.
 *
 * PARAMETERS: 
 *			stringP - pointer to a string
 *
 * RETURNS: nothing, the string is changed in place
 *
 *	REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf		12/2/97		Initial Revision
 *
 *************************************************************/

static void ReplaceSemicolonsWithNewlines(Char * stringP)
{
	Char * foundChrP;
	
	
	foundChrP = stringP;
	while ((foundChrP = StrChr(foundChrP, ';')) != NULL)
		{
		if (foundChrP == stringP || *(foundChrP - 1) != '\\')
			*foundChrP = linefeedChr;
		}	
}


/************************************************************
 *
 * FUNCTION: AddrImportVCard
 *
 * DESCRIPTION: Import a VCard record.
 *
 * PARAMETERS: 
 *			dbP - pointer to the database to add the record to
 *			inputStream	- pointer to where to import the record from
 *			inputFunc - function to get input from the stream
 *			obeyUniqueIDs - true to obey any unique ids if possible
 *			beginAlreadyRead - whether the begin statement has been read
 *
 * RETURNS: true if the input was read
 *
 *	REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf		4/24/97		Initial Revision
 *			bhall	8/12/99		moved category beaming code from gromit codeline
 *
 *************************************************************/

extern Boolean 
AddrImportVCard(DmOpenRef dbP, void * inputStream, GetCharF inputFunc, 
	Boolean obeyUniqueIDs, Boolean /* beginAlreadyRead */)
{
	UInt16 c;
	char identifier[identifierLengthMax];
	char parameterName[identifierLengthMax];
	int identifierEnd;
	int i;
	volatile AddrDBRecordType newRecord;
	AddrDBPhoneFlags phoneFlags;
	AddressPhoneLabels phoneLabel;
	UInt16 indexNew;
	UInt16 indexOld;
	UInt32 uid;
	Boolean lastCharWasQuotedPrintableContinueToNextLineChr;
	Boolean quotedPrintable;
	Err err;
	const UInt16 * charAttrP;
	Char * addressPostOfficeP;
	Char * addressExtendedP = NULL;
	UInt32 uniqueID;
	volatile Err error = 0;
	// user selected category is passed in appData field of the stream header
	UInt16	categoryID = PrvStreamSocket(inputStream)->appData;
	
	//char categoryName[dmCategoryLength];
	//categoryName[0] = 0;
	
	charAttrP = GetCharAttr();
	
	
	// Initialize a new record
	for (i=0; i < addrNumFields; i++)
		newRecord.fields[i] = NULL;	// clear the record
	
	newRecord.options.phones.phone1 = 0;	// Work
	newRecord.options.phones.phone2 = 1;	// Home
	newRecord.options.phones.phone3 = 2;	// Fax
	newRecord.options.phones.phone4 = 7;	// Other
	newRecord.options.phones.phone5 = 3;	// Email
	newRecord.options.phones.displayPhoneForList = phone1 - firstPhoneField;
	
	uid = 0;
	
	// An error happens usually due to no memory.  It's easier just to 
	// catch the error.  If an error happens, we remove the last record.  
	// Then we throw a second time so the caller receives it and displays a message.
	ErrTry
		{
		c = inputFunc(inputStream);
		ImcReadWhiteSpace(inputStream, inputFunc, charAttrP, &c);
		
		if (c == EOF) return false;
		
		identifierEnd = 0;
		// CAUTION: IsAlpha(EOF) returns true
		while (TxtCharIsAlpha(c) || c == ':' || 
			(c != linefeedChr && c != 0x0D && TxtCharIsSpace(c)))
			{
			if (!TxtCharIsSpace(c) && (identifierEnd < identifierLengthMax - 1))
		 		identifier[identifierEnd++] = (char) c;
		 	
			c = inputFunc(inputStream);
			if (c == EOF) return false;
			}
		identifier[identifierEnd++] = nullChr;
		
		// Read in the vcard entry
		if (StrCaselessCompare(identifier, "BEGIN:VCARD") == 0)
			{
			do
				{
				quotedPrintable = false;
				
				
				// Advance to the next line.  If the property wasn't handled we need to make sure that
				// if the property value contains quoted printable text that newlines within that 
				// section are skipped.
				lastCharWasQuotedPrintableContinueToNextLineChr = (c == '=');
				while ((c != endOfLineChr || lastCharWasQuotedPrintableContinueToNextLineChr) && c != EOF)
					{
					lastCharWasQuotedPrintableContinueToNextLineChr = (c == '=');
					c = inputFunc(inputStream);
					} 
				c = inputFunc(inputStream);
				
				// Skip the second part of a 0x0D 0x0A end of line sequence
				if (c == 0x0A)
					c = inputFunc(inputStream);
				else if (c == EOF)
					ErrThrow(exgErrBadData);
				
				
				// Read in an identifier.  Ignore any group specifier.
				do
					{
					// Consume any groupDelimeterChr
					if (c == groupDelimeterChr)
						c = inputFunc(inputStream);
					
					identifierEnd = 0;
					ImcReadWhiteSpace(inputStream, inputFunc, charAttrP, &c);
					while (c != valueDelimeterChr && c != parameterDelimeterChr && 
						c != groupDelimeterChr && c != endOfLineChr && c != EOF)
						{
						if (identifierEnd < identifierLengthMax)
							identifier[identifierEnd++] = (char) c;
						
						c = inputFunc(inputStream);
						}
					identifier[identifierEnd++] = nullChr;
					}
				while (c == groupDelimeterChr);
				
				
				// read in the decomposed name (this is the only required paramater in a vCard)
				if (StrCaselessCompare(identifier, "N") == 0)
				{
					// Free any memory already used because it's being replaced.
					if (newRecord.fields[name] != NULL)
						MemPtrFree(newRecord.fields[name]);
					if (newRecord.fields[firstName] != NULL)
						MemPtrFree(newRecord.fields[firstName]);

					ImcSkipAllPropertyParameters(inputStream, inputFunc, &c, identifier, &quotedPrintable);

					newRecord.fields[name] = ImcReadFieldNoSemicolon(inputStream, inputFunc, &c, tableMaxTextItemSize);
					if (c != endOfLineChr)	
						newRecord.fields[firstName] = ImcReadFieldNoSemicolon(inputStream, inputFunc, &c, tableMaxTextItemSize);
					
					// skip all other fields because we can't store them
				}
				
				// read in the decomposed address
				else if (StrCaselessCompare(identifier, "ADR") == 0)
					{
					// Free any memory already used because it's being replaced.
					if (newRecord.fields[address] != NULL) MemPtrFree(newRecord.fields[address]);
					if (newRecord.fields[city] != NULL) MemPtrFree(newRecord.fields[city]);
					if (newRecord.fields[state] != NULL) MemPtrFree(newRecord.fields[state]);
					if (newRecord.fields[zipCode] != NULL) MemPtrFree(newRecord.fields[zipCode]);
					if (newRecord.fields[country] != NULL) MemPtrFree(newRecord.fields[country]);
					
					ImcSkipAllPropertyParameters(inputStream, inputFunc, &c, identifier, &quotedPrintable);
					
					addressPostOfficeP = ImcReadFieldNoSemicolon(inputStream, inputFunc, &c, tableMaxTextItemSize);
					if (c != endOfLineChr)	
						addressExtendedP = ImcReadFieldNoSemicolon(inputStream, inputFunc, &c, tableMaxTextItemSize);
					if (c != endOfLineChr)	
						newRecord.fields[address] = ImcReadFieldNoSemicolon(inputStream, inputFunc, &c, tableMaxTextItemSize);
					if (c != endOfLineChr)	
						newRecord.fields[city] = ImcReadFieldNoSemicolon(inputStream, inputFunc, &c, tableMaxTextItemSize);
					if (c != endOfLineChr)	
						newRecord.fields[state] = ImcReadFieldNoSemicolon(inputStream, inputFunc, &c, tableMaxTextItemSize);
					if (c != endOfLineChr)	
						newRecord.fields[zipCode] = ImcReadFieldNoSemicolon(inputStream, inputFunc, &c, tableMaxTextItemSize);
					if (c != endOfLineChr)	
						newRecord.fields[country] = ImcReadFieldNoSemicolon(inputStream, inputFunc, &c, tableMaxTextItemSize);
					
					// Now include addressPostOfficeP and addressExtendedP in the address.
					if (addressPostOfficeP != NULL && addressPostOfficeP[0] != nullChr)
						{
						MemHandle stringH;
						Int16 newSize;
						
						if (newRecord.fields[address] != NULL)
							{
							// Resize addressPostOfficeP so that the address string can be copied after it. 
							newSize = min(tableMaxTextItemSize, StrLen(newRecord.fields[address]) + 
									sizeOf7BitChar(linefeedChr) + sizeOf7BitChar(spaceChr) + StrLen(addressPostOfficeP))
								+ sizeOf7BitChar(nullChr);
							stringH = (MemHandle) MemPtrRecoverHandle(addressPostOfficeP);
							MemHandleUnlock(stringH);
							MemHandleResize(stringH, newSize);
							addressPostOfficeP = (Char *) MemHandleLock(stringH);
							
							// Now add the address to the addressPostOfficeP, being careful not to overflow
							StrNCat(addressPostOfficeP, "\n ", newSize);
							StrNCat(addressPostOfficeP, newRecord.fields[address], newSize);
							
							// Append the string now that there's space
							MemPtrFree(newRecord.fields[address]);
							}
						
						// Use addressPostOfficeP for the address.
						newRecord.fields[address] = addressPostOfficeP;
						addressPostOfficeP = NULL;
						}
				
					if (addressExtendedP != NULL && addressExtendedP[0] != nullChr)
						{
						MemHandle stringH;
						Int16 newSize;
						
						if (newRecord.fields[address] != NULL)
							{
							// Resize addressExtendedP so that the address string can be copied after it. 
							newSize = min(tableMaxTextItemSize, StrLen(newRecord.fields[address]) +
									sizeOf7BitChar(linefeedChr) + sizeOf7BitChar(spaceChr) + StrLen(addressExtendedP))
								+ sizeOf7BitChar(nullChr);
							stringH = (MemHandle) MemPtrRecoverHandle(addressExtendedP);
							MemHandleUnlock(stringH);
							MemHandleResize(stringH, newSize);
							addressExtendedP = (Char *) MemHandleLock(stringH);
							
							// Now add the address to the addressExtendedP, being careful not to overflow
							StrNCat(addressExtendedP, "\n ", newSize);
							StrNCat(addressExtendedP, newRecord.fields[address], newSize);
							
							// Append the string now that there's space
							MemPtrFree(newRecord.fields[address]);
							}
						
						// Use addressExtendedP for the address.
						newRecord.fields[address] = addressExtendedP;
						addressExtendedP = NULL;
						}
					}
				
				// read in the full name as the last name.  Do this only if the name field has been
				// set.  The idea is to make use of this info if the name isn't set.
				else if (StrCaselessCompare(identifier, "FN") == 0 &&
					newRecord.fields[name] == NULL)
					{
					ImcSkipAllPropertyParameters(inputStream, inputFunc, &c, identifier, &quotedPrintable);
					
					newRecord.fields[name] = ImcReadFieldQuotablePrintable(inputStream, inputFunc, &c, endOfLineChr, quotedPrintable, tableMaxTextItemSize);
					}
				
				// read in the decomposed company
				else if (StrCaselessCompare(identifier, "ORG") == 0)
					{
					// Free any memory already used because it's being replaced.
					if (newRecord.fields[company] != NULL) MemPtrFree(newRecord.fields[company]);
					
					ImcSkipAllPropertyParameters(inputStream, inputFunc, &c, identifier, &quotedPrintable);
					
					
					newRecord.fields[company] = ImcReadFieldQuotablePrintable(inputStream, inputFunc, &c, endOfLineChr, quotedPrintable, tableMaxTextItemSize);
					ReplaceSemicolonsWithNewlines(newRecord.fields[company]);
					}
				
				// read in the title
				else if (StrCaselessCompare(identifier, "TITLE") == 0)
					{
					// Free any memory already used because it's being replaced.
					if (newRecord.fields[title] != NULL) MemPtrFree(newRecord.fields[title]);
					
					ImcSkipAllPropertyParameters(inputStream, inputFunc, &c, identifier, &quotedPrintable);
					
					
					newRecord.fields[title] = ImcReadFieldQuotablePrintable(inputStream, inputFunc, &c, endOfLineChr, quotedPrintable, tableMaxTextItemSize);
					ReplaceSemicolonsWithNewlines(newRecord.fields[title]);
					}
				
				// read in the note
				else if (StrCaselessCompare(identifier, "NOTE") == 0)
					{
					// Free any memory already used because it's being replaced.
					if (newRecord.fields[note] != NULL) MemPtrFree(newRecord.fields[note]);
					
					ImcSkipAllPropertyParameters(inputStream, inputFunc, &c, identifier, &quotedPrintable);
					
					
					newRecord.fields[note] = ImcReadFieldQuotablePrintable(inputStream, inputFunc, &c, endOfLineChr, quotedPrintable, noteViewMaxLength);
					}

#ifdef VCARD_CATEGORIES				
				// read in the category
				else if (StrCaselessCompare(identifier, "X-PALM-CATEGORY") == 0)
					{
					Char * categoryStringP;

					// Skip property parameters
					ImcSkipAllPropertyParameters(inputStream, inputFunc, &c, identifier, &quotedPrintable);
					
					// Read in the category
					categoryStringP = ImcReadFieldQuotablePrintable(inputStream, inputFunc, &c, endOfLineChr, quotedPrintable, tableMaxTextItemSize);
					if (categoryStringP != NULL)
						{
						// Make a copy
						StrNCopy(categoryName, categoryStringP, dmCategoryLength);
						
						// Free the string (Imc routines allocate the space)
						MemPtrFree(categoryStringP);

						// If we ever decide to use vCard 3.0 CATEGORIES, we would need to skip additional ones here
						}
					}
#endif				
				// read in the unique identifier
				else if (StrCaselessCompare(identifier, "UID") == 0 && obeyUniqueIDs)
					{
					Char * uniqueIDStringP;
					ImcSkipAllPropertyParameters(inputStream, inputFunc, &c, identifier, &quotedPrintable);
					
					
					uniqueIDStringP = ImcReadFieldQuotablePrintable(inputStream, inputFunc, &c, endOfLineChr, quotedPrintable, tableMaxTextItemSize);
					if (uniqueIDStringP != NULL)
						{
						uid = StrAToI(uniqueIDStringP);
						MemPtrFree(uniqueIDStringP);
						
						// Check the uid for reasonableness.
						if (uid < (dmRecordIDReservedRange << 12))
							uid = 0;
						}
					}
				
				// read in the decomposed name
				else if (StrCaselessCompare(identifier, "TEL") == 0 ||
					StrCaselessCompare(identifier, "EMAIL") == 0)
					{
					// Read in phone fields until either no more
					// strings are available or no more spots exist to store custom
					// information.
					i = firstPhoneField;
					
					// Advance to the first empty field
					while (newRecord.fields[i] != NULL &&
						i <= lastPhoneField)
						{
						i++;
						}
					
					// If all the phone fields are used then don't read anymore
					if (i <= lastPhoneField)
						{
						phoneFlags.allBits = 0;
						
						// Is this an email address?
						if (StrCaselessCompare(identifier, "EMAIL") == 0)
							{
							phoneFlags.bits.email = true;
							}
						
						
						// Parse the property parameters.
						while (c != valueDelimeterChr)
							{
							c = inputFunc(inputStream);			// consume the valueDelimeterChr
							if (c == EOF) break;
							
							ImcReadPropertyParameter(inputStream, inputFunc, &c, parameterName, identifier);
							
							if (StrCaselessCompare(identifier, "QUOTED-PRINTABLE") == 0)
								{
								quotedPrintable = true;
								}
							else if (StrCaselessCompare(identifier, "HOME") == 0)
								{
								phoneFlags.bits.home = true;
								}
							else if (StrCaselessCompare(identifier, "WORK") == 0)
								{
								phoneFlags.bits.work = true;
								}
							else if (StrCaselessCompare(identifier, "FAX") == 0)
								{
								phoneFlags.bits.fax = true;
								}
							else if (StrCaselessCompare(identifier, "PAGER") == 0)
								{
								phoneFlags.bits.pager = true;
								}
							else if (StrCaselessCompare(identifier, "CELL") == 0 ||
								StrCaselessCompare(identifier, "CAR") == 0)
								{
								phoneFlags.bits.mobile = true;
								}
							else if (StrCaselessCompare(identifier, "PREF") == 0)
								{
								newRecord.options.phones.displayPhoneForList = (i - firstPhoneField);
								}
							}
						c = inputFunc(inputStream);
						
						
						// Analyze the phone label bits and pick a single label to best
						// represent the bits.
						if (phoneFlags.bits.email)
							{
							phoneLabel = emailLabel;
							}
						else if (phoneFlags.bits.fax)
							{
							phoneLabel = faxLabel;
							}
						else if (phoneFlags.bits.pager)
							{
							phoneLabel = pagerLabel;
							}
						else if (phoneFlags.bits.mobile)
							{
							phoneLabel = mobileLabel;
							}
						else if (phoneFlags.bits.home)
							{
							phoneLabel = homeLabel;
							}
						else if (phoneFlags.bits.work)
							{
							phoneLabel = workLabel;
							}
						else
							{
							phoneLabel = otherLabel;
							}

						SetPhoneLabel(&newRecord, i, phoneLabel);
						
						newRecord.fields[i] = ImcReadFieldQuotablePrintable(inputStream, inputFunc, &c,
												parameterDelimeterChr, quotedPrintable, tableMaxTextItemSize);
						}
					
					}
				
				// read in the custom field.  Note we could do intelligent matching into custom fields
				// which have the same name in a different location.
				else if (StrCaselessCompare(identifier, "X-PALM-CUSTOM") == 0)
					{
					UInt16 customNumber = 0;
					Boolean numberSpecified = false;
					
					// Parse the property parameters.
					while (c != valueDelimeterChr)
						{
						c = inputFunc(inputStream);			// consume the valueDelimeterChr
						if (c == EOF) break;
						
						ImcReadPropertyParameter(inputStream, inputFunc, &c, parameterName, identifier);
						
						if (StrCaselessCompare(identifier, "QUOTED-PRINTABLE") == 0)
							{
							quotedPrintable = true;
							}
						// If we found a number and we don't have one already, then use the number for the category.
						// This survives the case when the category name is a number.
						else if (!numberSpecified && StrAToI(identifier) != 0)
							{
							customNumber = StrAToI(identifier) - 1;
							numberSpecified = true;
							}

						}
					c = inputFunc(inputStream);
					
					ErrNonFatalDisplayIf(customNumber >= (lastRenameableLabel - firstRenameableLabel + 1), 
						"Invalid Custom Field");
					
					// Free any memory already used because it's being replaced.
					if (newRecord.fields[custom1 + customNumber] != NULL) 
						MemPtrFree(newRecord.fields[custom1 + customNumber]);
					
					newRecord.fields[custom1 + customNumber] = ImcReadFieldQuotablePrintable(inputStream, inputFunc, &c,
																endOfLineChr, quotedPrintable, tableMaxTextItemSize);
					}
				
				// Begin a new vcard
				else if (StrCaselessCompare(identifier, "BEGIN") == 0)
					{
					// Advance to the next line.
					while (c != endOfLineChr && c != EOF)
						{
						c = inputFunc(inputStream);
						} 
					
					if (c != EOF)
						AddrImportVCard(dbP, inputStream, inputFunc, obeyUniqueIDs, true);
					
					break;
					}
				
				// stop if at the end
				else if (StrCaselessCompare(identifier, "END") == 0)
					{
					// Advance to the next line.
					while (c != endOfLineChr && c != EOF)
						{
						c = inputFunc(inputStream);
						} 
					
					break;
					}
				
				} while (true);

		
			// Fix up the record's data.
			
			// If the displayed phone doesn't exist then pick the first phone which does.
			if (newRecord.fields[firstPhoneField + newRecord.options.phones.displayPhoneForList] == NULL)
				for (i = firstPhoneField; i <= lastPhoneField; i++)
					if (newRecord.fields[i] != NULL)
						{
						newRecord.options.phones.displayPhoneForList = i - firstPhoneField;
						break;
						}

			// if the company and name fields are identical, assume company only
			if (newRecord.fields[name] != NULL
				&& newRecord.fields[company] != NULL
				&& newRecord.fields[firstName] == NULL
				&& StrCompare(newRecord.fields[name], newRecord.fields[company]) == 0)
			{
				MemPtrFree(newRecord.fields[name]);
				newRecord.fields[name] = NULL;
			}
		   
			// Add the record to the database
			err = AddrNewRecord(dbP, (AddrDBRecordType*)&newRecord, &indexNew);
			if (err)
				ErrThrow(exgMemError);
			
#ifdef VCARD_CATEGORIES				
			// If a category was included, try to use it
			if (categoryName[0]) {
				UInt16	categoryID;
				UInt16	attr;
				Err		err;

				// Get the category ID
				categoryID = CategoryFind(dbP, categoryName);
				
				// If it doesn't exist, and we have room, create it
				if (categoryID == dmAllCategories) {
					// Find the first unused category
					categoryID = CategoryFind(dbP, "");

					// If there is a slot, fill it with the name we were given
					if (categoryID != dmAllCategories) {
						CategorySetName(dbP, categoryID, categoryName);
					}
				}

				// Set the category for the record
				if (categoryID != dmAllCategories) {
					// Get the attributes
					err = DmRecordInfo(dbP, indexNew, &attr, NULL, NULL);

					// Set them to include the category, and mark the record dirty
					if ((attr & dmRecAttrCategoryMask) != categoryID) {
						attr &= ~dmRecAttrCategoryMask;
						attr |= categoryID | dmRecAttrDirty;
						err = DmSetRecordInfo(dbP, indexNew, &attr, NULL);
					}
				}
			}
#endif				
			// Set the category for the record
			if (categoryID){
				UInt16	attr;
				Err		err;
				
				// Get the attributes
				err = DmRecordInfo(dbP, indexNew, &attr, NULL, NULL);

				// Set them to include the category, and mark the record dirty
				if ((attr & dmRecAttrCategoryMask) != categoryID) {
					attr &= ~dmRecAttrCategoryMask;
					attr |= categoryID | dmRecAttrDirty;
					err = DmSetRecordInfo(dbP, indexNew, &attr, NULL);
				}
			}
			
			// If uid was set then a unique id was passed to be used.
			if (uid != 0 && obeyUniqueIDs)
				{
				// We can't simply remove any old record using the unique id and
				// then add the new record because removing the old record could
				// move the new one.  So, we find any old record, change the new
				// record, and then remove the old one.
				indexOld = indexNew;
				
				// Find any record with this uid.  indexOld changes only if 
				// such a record is found.
				DmFindRecordByID (dbP, uid, &indexOld);
				
				// Change this record to this uid.  The dirty bit is set from 
				// newly making this record.
				DmSetRecordInfo(dbP, indexNew, NULL, &uid);
				
				// Now remove any old record.
				if (indexOld != indexNew)
					{
					DmRemoveRecord(dbP, indexOld);
					}
				}


			// Store the information necessary to navigate to the record inserted.
			DmRecordInfo(dbP, indexNew, NULL, &uniqueID, NULL);

#if EMULATION_LEVEL != EMULATION_NONE
			// Don't call AddrSetGoToParams for shell commands.  Do this by seeing which 
			// input function is passed - the one for shell commands or the local one for exchange.
			if (inputFunc == GetChar)
#endif
			AddrSetGoToParams (dbP, PrvStreamSocket(inputStream), uniqueID);
			}
		
		}
	
	ErrCatch(inErr)
		{
		// Throw the error after the memory is cleaned up.
		error = inErr;
		} ErrEndCatch

	
	// Free any temporary buffers used to store the incoming data.
	for (i=0; i < addrNumFields; i++)
		{
		if (newRecord.fields[i] != NULL)
			{
			MemPtrFree(newRecord.fields[i]);
			newRecord.fields[i] = NULL;	// clear the record
			}
		}
	
	if (error)
		ErrThrow(error);
	
	// don't worry about protecting c from ErrCatch because this code is never reached when ErrCatch is.
	return (c != EOF);
}


/************************************************************
 *
 * FUNCTION: AddrExportVCard
 *
 * DESCRIPTION: Export a record as a Imc VCard record
 *
 * PARAMETERS: 
 *			dbP - pointer to the database to export the records from
 *			index - the record number to export
 *			recordP - whether the begin statement has been read
 *			outputStream - pointer to where to export the record to
 *			outputFunc - function to send output to the stream
 *			writeUniqueIDs - true to write the record's unique id
 *
 * RETURNS: nothing
 *
 *	HISTORY:
 *		08/06/97	rsf	Created by Roger Flores
 *		06/09/99	grant	Ensure that phone numbers labeled "other" aren't
 *							tagged as ";WORK" or ";HOME".
 *		08/12/99	bhall	moved category beaming code from gromit codeline
 *		10/30/99	kwk	Use TxtGetChar before calling TxtCharIsDigit.
 *
 *************************************************************/

void AddrExportVCard(DmOpenRef dbP, Int16 index, AddrDBRecordType *recordP, 
	void * outputStream, PutStringF outputFunc, Boolean writeUniqueIDs)
{
	int			i;
	UInt32			uid;
	AddrAppInfoPtr appInfoP= NULL;
	Char uidString[12];
	Boolean personOnlyAtHome = false;
	Boolean personOnlyAtWork = false;
	AddressPhoneLabels phoneLabel;
	MemHandle unnamedRecordStrH;
	Char * unnamedRecordStr;

	outputFunc(outputStream, "BEGIN:VCARD" imcLineSeparatorString "VERSION:2.1" imcLineSeparatorString);
	
	// Emit a name
	if (recordP->fields[name] != NULL || 
		recordP->fields[firstName] != NULL)
		{
		// The item
		outputFunc(outputStream, "N");
		
		// Check if "CHARSET=ISO-8859-1" is needed
		if (!ImcStringIsAscii(recordP->fields[name]) || 
			!ImcStringIsAscii(recordP->fields[firstName]))
			{
			outputFunc(outputStream, ";CHARSET=ISO-8859-1");
			}
		
		outputFunc(outputStream, ":");
		
		ImcWriteNoSemicolon(outputStream, outputFunc, recordP->fields[name]);
		outputFunc(outputStream, ";");
		
		ImcWriteNoSemicolon(outputStream, outputFunc, recordP->fields[firstName]);
		
		outputFunc(outputStream, imcLineSeparatorString);
		}
	else if (recordP->fields[company] != NULL)
		// no name field, so try emitting company in N: field
		{
		outputFunc(outputStream, "N");
		if (!ImcStringIsAscii(recordP->fields[company]))
			outputFunc(outputStream, ";CHARSET=ISO-8859-1");
		outputFunc(outputStream, ":");
		ImcWriteNoSemicolon(outputStream, outputFunc, recordP->fields[company]);
		outputFunc(outputStream, imcLineSeparatorString);
		}
	else
		// no company name either, so emit unnamed identifier
		{
		unnamedRecordStrH = DmGetResource(strRsc, UnnamedRecordStr);
		unnamedRecordStr = MemHandleLock(unnamedRecordStrH);

		outputFunc(outputStream, "N");
		if (!ImcStringIsAscii(unnamedRecordStr))
			outputFunc(outputStream, ";CHARSET=ISO-8859-1");
		outputFunc(outputStream, ":");
		ImcWriteNoSemicolon(outputStream, outputFunc, unnamedRecordStr);
		outputFunc(outputStream, imcLineSeparatorString);

		MemHandleUnlock(unnamedRecordStrH);
		DmReleaseResource(unnamedRecordStrH);   
		}
	
	
	// Emit an address
	if (recordP->fields[address] != NULL || 
		recordP->fields[city] != NULL || 
		recordP->fields[state] != NULL || 
		recordP->fields[zipCode] != NULL || 
		recordP->fields[country] != NULL)
		{
		// The item
		outputFunc(outputStream, "ADR");

		// if there is no country specified, assume domestic (default is international)
		if (!recordP->fields[country])
			outputFunc(outputStream,";DOM");
		
		// Check if "CHARSET=ISO-8859-1" is needed
		if (!ImcStringIsAscii(recordP->fields[address]) || 
			!ImcStringIsAscii(recordP->fields[city]) || 
			!ImcStringIsAscii(recordP->fields[state]) || 
			!ImcStringIsAscii(recordP->fields[zipCode]) || 
			!ImcStringIsAscii(recordP->fields[country]))
			{
			outputFunc(outputStream, ";CHARSET=ISO-8859-1");
			}
		
		// Skip the P.O. Box field and the Extended Address field
		outputFunc(outputStream, ":;;");
		
		ImcWriteNoSemicolon(outputStream, outputFunc, recordP->fields[address]);
		outputFunc(outputStream, ";");
		
		ImcWriteNoSemicolon(outputStream, outputFunc, recordP->fields[city]);
		outputFunc(outputStream, ";");
		
		ImcWriteNoSemicolon(outputStream, outputFunc, recordP->fields[state]);
		outputFunc(outputStream, ";");
		
		ImcWriteNoSemicolon(outputStream, outputFunc, recordP->fields[zipCode]);
		outputFunc(outputStream, ";");
		
		ImcWriteNoSemicolon(outputStream, outputFunc, recordP->fields[country]);
		
		outputFunc(outputStream, imcLineSeparatorString);
		}
	
	
	// Emit the organization
	if (recordP->fields[company] != NULL)
		{
		// The item
		outputFunc(outputStream, "ORG");
		
		ImcWriteQuotedPrintable(outputStream, outputFunc, recordP->fields[company], true);
		
		outputFunc(outputStream, imcLineSeparatorString);
		}
	
	
	// Emit a title
	if (recordP->fields[title] != NULL)
		{
		// The item
		outputFunc(outputStream, "TITLE");
		
		ImcWriteQuotedPrintable(outputStream, outputFunc, recordP->fields[title], false);
		
		outputFunc(outputStream, imcLineSeparatorString);
		}
	
	
	// Emit a note
	if (recordP->fields[note] != NULL)
		{
		// The item
		outputFunc(outputStream, "NOTE");
		
		ImcWriteQuotedPrintable(outputStream, outputFunc, recordP->fields[note], false);
		
		outputFunc(outputStream, imcLineSeparatorString);
		}
	
	
	// First find out if only home or work phones are used
	for (i = firstPhoneField; i <= lastPhoneField; i++)
		{
		if (recordP->fields[i] != NULL)
			{
			phoneLabel = (AddressPhoneLabels) GetPhoneLabel(recordP, i);
			if (phoneLabel == homeLabel)
				{
				if (personOnlyAtWork)
					{
					personOnlyAtWork = false;
					break;
					}
				else
					{
					personOnlyAtHome = true;
					}
				}
			else if (phoneLabel == workLabel)
				{
				if (personOnlyAtHome)
					{
					personOnlyAtHome = false;
					break;
					}
				else
					{
					personOnlyAtWork = true;
					}
				}

			}
		}
	
	
	// Now emit the phone fields
	for (i = firstPhoneField; i <= lastPhoneField; i++)
		{
		if (recordP->fields[i] != NULL)
			{
			phoneLabel = (AddressPhoneLabels) GetPhoneLabel(recordP, i);
			if (phoneLabel != emailLabel)
				{
				// The item
				outputFunc(outputStream, "TEL");
				
				// Is this prefered?  Assume so if listed in the list view.
				if (recordP->options.phones.displayPhoneForList == i - firstPhoneField)
					{
					outputFunc(outputStream, ";PREF");
					}
				
				// Add a home or work tag, unless this field is labeled "other".
				// We don't want "other" phone numbers to be tagged as ";WORK" or
				// ";HOME", because then they are interpreted as "work" or "home" numbers
				// on the receiving end.
				if (phoneLabel != otherLabel)
					{
					if (personOnlyAtHome || phoneLabel == homeLabel)
						outputFunc(outputStream, ";HOME");
					else if (personOnlyAtWork || phoneLabel == workLabel)
						outputFunc(outputStream, ";WORK");
					}

				switch (phoneLabel)
					{
					case faxLabel:
						outputFunc(outputStream, ";FAX");
						break;
						
					case pagerLabel:
						outputFunc(outputStream, ";PAGER");
						break;
						
					case mobileLabel:
						outputFunc(outputStream, ";CELL");
						break;
						
					case workLabel:
					case homeLabel:
					case mainLabel:
						outputFunc(outputStream, ";VOICE");
						break;
					
					case otherLabel:
						break;
					}
				
				
				ImcWriteQuotedPrintable(outputStream, outputFunc, recordP->fields[i], false);
				
				outputFunc(outputStream, imcLineSeparatorString);
				}
			else
				{
				// The item
				outputFunc(outputStream, "EMAIL");
				
				// Is this prefered?  Assume so if listed in the list view.
				if (recordP->options.phones.displayPhoneForList == i - firstPhoneField)
					{
					outputFunc(outputStream, ";PREF");
					}
				
				if (personOnlyAtHome)
					outputFunc(outputStream, ";HOME");
				else if (personOnlyAtWork)
					outputFunc(outputStream, ";WORK");

				// Now try to identify the email type by it's syntax.
				// A '@' indicates a probable internet address.
				if (StrChr(recordP->fields[i], '@') == NULL)
					{
					if (TxtCharIsDigit(TxtGetChar(recordP->fields[i], 0)))
						{
						if (StrChr(recordP->fields[i], ',') != NULL)
							outputFunc(outputStream, ";CIS");
						// We know that a hyphen is never part of a multi-byte char.
						else if (recordP->fields[i][3] == '-')
							{
							outputFunc(outputStream, ";MCIMail");
							}
						}
					}
				else
					{
					outputFunc(outputStream, ";INTERNET");
					}
				
				
				ImcWriteQuotedPrintable(outputStream, outputFunc, recordP->fields[i], false);
				
				outputFunc(outputStream, imcLineSeparatorString);
				}
			}
		}
	
	// Emit the custom fields
	for (i = firstRenameableLabel; i <= lastRenameableLabel; i++)
		{
		if (recordP->fields[i] != NULL)
			{
			// The item
			outputFunc(outputStream, "X-PALM-CUSTOM;");
			
			// Emit the custom field number
			StrIToA(uidString, i - firstRenameableLabel + 1);
			outputFunc(outputStream, uidString);
			
			if (appInfoP == NULL)
				appInfoP = (AddrAppInfoPtr) AddrAppInfoGetPtr(dbP);
			
			// Emit the custom label name if used.  This will enable smart matching.
			if (appInfoP->fieldLabels[i][0] != nullChr)
				{
				Char temp[2];
				Char * c;
				
				temp[1] = '\0';
				
				outputFunc(outputStream, ";");
				
				// Emit the custom field name.  Strip out all ':' and ';' chars because
				// they have special meanings to VCard.
				c = appInfoP->fieldLabels[i];
				while (*c != nullChr)
					{
					if (*c != parameterDelimeterChr && *c != valueDelimeterChr)
						{
						temp[0] = *c;
						outputFunc(outputStream, temp);
						}
					c++;
					}
				
				}
			
			
			ImcWriteQuotedPrintable(outputStream, outputFunc, recordP->fields[i], false);
			
			outputFunc(outputStream, imcLineSeparatorString);
			}
		}
	if (appInfoP != NULL)
		MemPtrUnlock(appInfoP);
	
	
	// Emit an unique id
	if (writeUniqueIDs)
		{
		// The item
		outputFunc(outputStream, "UID:");
		
		// Get the record's unique id and append to the string.
		DmRecordInfo(dbP, index, NULL, &uid, NULL);
		StrIToA(uidString, uid);
		outputFunc(outputStream, uidString);
		outputFunc(outputStream, imcLineSeparatorString);
		}

#ifdef VCARD_CATEGORIES	
	// Emit category
	{
		Char temp[2];
		Char * c;
		Char description[dmCategoryLength];
		UInt16 attr;
		UInt16 category;

		// Output the item name
		outputFunc(outputStream, "X-PALM-CATEGORY:");

		// Get category name
		DmRecordInfo (dbP, index, &attr, NULL, NULL);
		category = attr & dmRecAttrCategoryMask;
		CategoryGetName(dbP, category, description);
		
		// Output category name
		// Strip out all ':' and ';' chars because they have special meanings to VCard.
		c = description;
		while (*c != nullChr) {
			if (*c != parameterDelimeterChr && *c != valueDelimeterChr) {
				temp[0] = *c;
				outputFunc(outputStream, temp);
			}
			c++;
		}

		outputFunc(outputStream, imcLineSeparatorString);
	}	
#endif
	
	// We are now done - output an END
	outputFunc(outputStream, "END:VCARD" imcLineSeparatorString);
}

