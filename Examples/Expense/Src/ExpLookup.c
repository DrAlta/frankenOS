/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ExpLookup.c
 *
 * Description:
 *	  This module contains routine that support the auto-fill feature 
 *   of the expense application
 *
 * History:
 *		January 4, 1995	Created by Art Lamb
 *
 *****************************************************************************/

#include <PalmOS.h>
#include "Expense.h"
#include <CharAttr.h>



/***********************************************************************
 *
 * FUNCTION:     LookupIntDB
 *
 * DESCRIPTION:  This routine initializes a lookup database from a 
 *               resource.  Each string in the resource becomes a record
 *               in the database.  The strings in the resouce are delimited
 *               by the '|' character.
 *
 * PARAMETERS:   type      - database type
 *               creator   - database creator type
 *               name      - name given to the database
 *               initRscID - reosource used to initialize the database
 *
 * RETURNED:     error code, 0 if successful
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/12/96	Initial Revision
 *			grant	4/7/99	Set backup bit on new databases.
 *
 ***********************************************************************/
Err LookupInitDB (UInt32 type, UInt32 creator, Char * name, 
	UInt16 initRscID)
{
	Char zero=0;
	UInt16 error = 0;
	UInt16 index;
	UInt32 strLen;
	UInt32 time;
	Char * endP;
	Char * resP;
	MemHandle resH;
	MemHandle rH;
	DmOpenRef dbP = 0;
	LookupRecordPtr rP;
	LookupRecordPtr nilP = 0;

	dbP = DmOpenDatabaseByTypeCreator (type, creator, dmModeReadWrite);
	if (dbP) goto exit;
	
	// If the description datebase does not exist, create it.
	error = DmCreateDatabase (0, name, creator, type, false);
	if (error) return (error);
		
	dbP = DmOpenDatabaseByTypeCreator (type, creator, dmModeReadWrite);
	if (! dbP) return (-1);

	// Set the backup bit.  This is to aid syncs with non-Palm software.
	// Also set hidden bit to avoid confusing record counts in launcher info.
	SetDBAttrBits(dbP,dmHdrAttrHidden|dmHdrAttrBackup);

	time = TimGetSeconds ();
	
	// Initial the description datebase from a resource contains strings
	// delimited by "|" characters.
	resH = DmGetResource (strRsc, initRscID);
	resP = MemHandleLock (resH);
	index = 0;
	while (true)
		{
		endP = StrChr (resP, '|');
		if (! endP) break;
		
		strLen = (UInt16) (endP - resP);
		
		rH = DmNewRecord (dbP, &index, sizeof (LookupRecordType) + strLen);
		
		rP = MemHandleLock (rH); 

		DmWrite (rP, (UInt32)&nilP->time, &time, sizeof (UInt32));
		DmWrite (rP, (UInt32)&nilP->text, resP, strLen);
		DmWrite (rP, (UInt32)&nilP->text+strLen, &zero, 1); // null-terminate
		MemPtrUnlock (rP);

		DmReleaseRecord (dbP, index, true);
		
		resP += strLen + 1;
		index++;
		}

	MemHandleUnlock (resH);

exit:
	DmCloseDatabase (dbP);
		
	return (0);
}


/************************************************************
 *
 *  FUNCTION: PartialCaselessCompare
 *
 *  DESCRIPTION: Compares two strings with case and accent insensitivity.
 *               If all of s1 matches all or the start of s2 then
 *               there is a match
 *
 *  PARAMETERS: 2 string pointers
 *
 *  RETURNS: 0 if they match, non-zero if not
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/4/96	Initial Revision
 *			kwk	05/16/99	Use StrNCaselessCompare.
 *
 *************************************************************/
static Int16 PartialCaselessCompare (Char * s1, Char * s2)
{

	// Check for err
	ErrFatalDisplayIf(s1 == NULL || s2 == NULL, "NULL string passed");
	
	return (StrNCaselessCompare (s1, s2, StrLen (s1)));
}


/***********************************************************************
 *
 * FUNCTION:     LookupString
 *
 * DESCRIPTION:  This routine seatches a database for a the string passed.
 *
 * PARAMETERS:   dpb       - description database
 *					  key       - string to lookup record with
 *					  indexP    - to contain the record found
 *
 * RETURNED:     true if a unique match was found.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/4/96	Initial Revision
 *
 ***********************************************************************/
Boolean LookupStringInDatabase (DmOpenRef dbP, Char * key, UInt16 * indexP)
{
	Int16 					result;
	Int16 					numOfRecords;
	UInt16					kmin, probe, i;
	MemHandle 			rH;
	Boolean				match = false;
	LookupRecordPtr 	rP;
	Boolean 				unique;
	
	unique = false;
	
	if ((! key) || (! *key) ) return false;
		
	numOfRecords = DmNumRecords(dbP);
	if (numOfRecords == 0) return false;
	
	result = 0;
	kmin = probe = 0;

	
	while (numOfRecords > 0)
		{
		i = numOfRecords >> 1;
		probe = kmin + i;


		// Compare the two records.  Treat deleted records as greater.
		// If the records are equal look at the following position.
		rH = DmQueryRecord (dbP, probe);
		if (rH == 0)
			{
			result = -1;		// Delete record is greater
			}
		else
			{
			rP = MemHandleLock(rH);				
			result = PartialCaselessCompare (key, &rP->text);
			MemHandleUnlock(rH);
			}


		// If the date passed is less than the probe's date , keep searching.
		if (result < 0)
			numOfRecords = i;

		// If the date passed is greater than the probe's date, keep searching.
		else if (result > 0)
			{
			kmin = probe + 1;
			numOfRecords = numOfRecords - i - 1;
			}

		// If equal stop here!  Make sure the match is unique by checking
		// the records before and after the probe,  if either matches then
		// we don't have a unique match.
		else
			{
			// Start by assuming we have a unique match.
			match = true;
			unique = true;
			*indexP = probe;

			// If we not have a unique match,  we want to return the 
			// index one the first item that matches the key.
			while (probe)
				{
				rH = DmQueryRecord (dbP, probe-1);
				rP = MemHandleLock(rH);				
				result = PartialCaselessCompare (key, &rP->text);
				MemHandleUnlock(rH);
				if (result != 0) break;

				unique = false;
				*indexP = probe-1;
				probe--;
				}
				
			if (! unique) break;
			

			if (probe + 1 < DmNumRecords(dbP))
				{
				rH = DmQueryRecord (dbP, probe+1);
				if (rH)
					{
					rP = MemHandleLock(rH);				
					result = PartialCaselessCompare (key, &rP->text);
					MemHandleUnlock(rH);
					if (result == 0) 
						unique = false;				
					}
				}
			break;		
			}
		}
	return (match);
}


/*
Boolean LookupStringInDatabase (DmOpenRef dbP, Char * key, UInt16 * indexP)
{
	Int16 					result;
	Int16 					numOfRecords;
	UInt16					kmin, probe, i;
	MemHandle 			rH;
	Boolean				match = false;
	LookupRecordPtr 	rP;
	
	
	if ((! key) || (! *key) ) return false;
		
	numOfRecords = DmNumRecords(dbP);
	if (numOfRecords == 0) return false;
	
	result = 0;
	kmin = probe = 0;

	
	while (numOfRecords > 0)
		{
		i = numOfRecords >> 1;
		probe = kmin + i;


		// Compare the two records.  Treat deleted records as greater.
		// If the records are equal look at the following position.
		rH = DmQueryRecord (dbP, probe);
		if (rH == 0)
			{
			result = -1;		// Delete record is greater
			}
		else
			{
			rP = MemHandleLock(rH);				
			result = PartialCaselessCompare (key, &rP->text);
			MemHandleUnlock(rH);
			}


		// If the date passed is less than the probe's date , keep searching.
		if (result < 0)
			numOfRecords = i;

		// If the date passed is greater than the probe's date, keep searching.
		else if (result > 0)
			{
			kmin = probe + 1;
			numOfRecords = numOfRecords - i - 1;
			}

		// If equal stop here!  Make sure the match is unique by checking
		// the records before and after the probe,  if either matches then
		// we don't have a unique match.
		else
			{
			if (probe)
				{
				rH = DmQueryRecord (dbP, probe-1);
				rP = MemHandleLock(rH);				
				result = PartialCaselessCompare (key, &rP->text);
				MemHandleUnlock(rH);
				if (result == 0) break;
				}
				
			if (probe + 1 < DmNumRecords(dbP))
				{
				rH = DmQueryRecord (dbP, probe+1);
				if (rH)
					{
					rP = MemHandleLock(rH);				
					result = PartialCaselessCompare (key, &rP->text);
					MemHandleUnlock(rH);
					if (result == 0) break;
					}
				}
			
			match = true;
			*indexP = probe;
			
			break;		
			}
		}
	return (match);
}
*/

/***********************************************************************
 *
 * FUNCTION:     LookupStringInList
 *
 * DESCRIPTION:  This routine seatches a list for a the string passed.
 *
 * PARAMETERS:   lst       - pointer to a list object
 *					  key       - string to lookup record with
 *					  indexP    - to contain the record found
 *            
 *
 * RETURNED:     true if a match was found.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/96	Initial Revision
 *
 ***********************************************************************/
Boolean LookupStringInList (ListPtr lst, Char * key, UInt16 * indexP, 
	Boolean * uniqueP)
{
	Int16 		result;
	Int16 		numItems;
	UInt16		kmin, probe, i;
	Char *	itemP;
	Boolean	match = false;
//	Boolean 	unigue;
	
	*uniqueP = false;
	
	if ((! key) || (! *key) ) return false;
		
	numItems = LstGetNumberOfItems (lst);
	if (numItems == 0) return false;
	
	result = 0;
	kmin = probe = 0;

	
	while (numItems > 0)
		{
		i = numItems >> 1;
		probe = kmin + i;


		// Compare the a list item to the key.
		itemP = LstGetSelectionText (lst, probe);
		result = PartialCaselessCompare (key, itemP);


		// If the date passed is less than the probe's date , keep searching.
		if (result < 0)
			numItems = i;

		// If the date passed is greater than the probe's date, keep searching.
		else if (result > 0)
			{
			kmin = probe + 1;
			numItems = numItems - i - 1;
			}

		// If equal stop here!  Make sure the match is unique by checking
		// the item before and after the probe,  if either matches then
		// we don't have a unique match.
		else
			{
			// Start by assuming we have a unique match.
			match = true;
			*uniqueP = true;
			*indexP = probe;

			// If we not have a unique match,  we want to return the 
			// index one the first item that matches the key.
			while (probe)
				{
				itemP = LstGetSelectionText (lst, probe-1);
				if (PartialCaselessCompare (key, itemP) != 0)
					break;
				*uniqueP = false;
				*indexP = probe-1;
				probe--;
				}

			if (!*uniqueP) break;

			if (probe + 1 < LstGetNumberOfItems (lst))
				{
				itemP = LstGetSelectionText (lst, probe+1);
				if (PartialCaselessCompare (key, itemP) == 0)
					*uniqueP = false;				
				}
			break;		
			}
		}
	return (match);
}


/***********************************************************************
 *
 * FUNCTION:     LookupSave
 *
 * DESCRIPTION:  Compare two lookup records.  This is a callback
 *               called by DmFindSortPosition.
 *
 * PARAMETERS:   database record 1
 *               database record 2
 *
 *  RETURNS:     -n if record one is less (n != 0)
 *			         n if record two is less
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/16/96	Initial Revision
 *
 ***********************************************************************/
static Int16 LookupCompareRecords (Char * str, LookupRecordPtr r2, Int16 /*dummy*/,
	SortRecordInfoPtr /*info1*/, SortRecordInfoPtr /*info2*/,
	MemHandle /*appInfoH*/)
	{
	return (StrCaselessCompare (str, &r2->text));
	}


/***********************************************************************
 *
 * FUNCTION:     LookupSave
 *
 * DESCRIPTION:  This routine saves the string passed to the specified
 *               lookup database.
 *
 * PARAMETERS:   type      - database type
 *               creator   - database creator type
 *               str       - string to save to lookup record
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/16/96	Initial Revision
 *
 ***********************************************************************/
void LookupSave (UInt32 type, UInt32 creator, Char * str)
{
	UInt16 i;
	UInt16 index;
	UInt16 numRecords;
	UInt32 strLen;
	UInt32 time;
	Boolean insert = false;
	MemHandle  h;
	DmOpenRef dbP;
	LookupRecordPtr r;
	LookupRecordPtr nilP = 0;


	dbP = DmOpenDatabaseByTypeCreator (type, creator, dmModeReadWrite);
	if (! dbP) return;
	
	strLen = StrLen (str);

	// There is a limit on the number of entries in a lookup database,
	// if we've reached that limit find the oldest entry and replace it.
	numRecords = DmNumRecords (dbP);
	if (numRecords >= maxLookupEntries)
		{
		time = -1;
		for (i = 0; i < numRecords; i++)
			{
			r = MemHandleLock (DmQueryRecord (dbP, i));
			if (r->time < time)
				{
				index = i;
				time = r->time;
				}
			MemPtrUnlock (r);
			}
		}

	else
		{
		// Check if the string passed already exist in the database,  if it 
		// doesn't insert it.  If it does,  write the passed string to
		// the record, the case of one or more of the character may
		// changed.
		index = DmFindSortPosition (dbP, str, NULL, (DmComparF *)LookupCompareRecords, 0);
	
		insert = true;
		if (index)
			{
			h = DmQueryRecord (dbP, index-1);
			
			r = MemHandleLock (h); 
			if (StrCaselessCompare (str, &r->text) == 0)
				{	
				insert = false;
				index--;
			 	}
			MemPtrUnlock (r);
			}
	
		}


	if (insert)
		{
		h = DmNewRecord (dbP, &index, sizeof (LookupRecordType) + strLen);
		if (! h) goto exit;
		
		DmReleaseRecord (dbP, index, true);
		}
	else
		{
		h = DmResizeRecord (dbP, index, sizeof (LookupRecordType) + strLen);
		if (! h) goto exit;
		}


	// Copy the string passed to the record and time stamp the entry with
	// the current time.
	time = TimGetSeconds ();
	h = DmGetRecord (dbP, index);
	r = MemHandleLock (h);
	DmWrite (r, (UInt32)&nilP->time, &time, sizeof (UInt32));
	DmWrite (r, (UInt32)&nilP->text, str, strLen + 1);
	MemPtrUnlock (r);

	DmReleaseRecord (dbP, index, true);


exit:
	DmCloseDatabase (dbP);
}

