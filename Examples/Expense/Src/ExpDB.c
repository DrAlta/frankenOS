/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ExpDB.c
 *
 * Description:
 *	  This is the Expense application's datebase management routines.
 *
 * History:
 *		January 3, 1995	Created by Art Lamb
 *
 *****************************************************************************/

#include "PalmOS.h"
#include "FeatureMgr.h"
#include "Expense.h"


/************************************************************
 *
 *  FUNCTION: DateTypeCmp
 *
 *  DESCRIPTION: Compare two dates
 *
 *  PARAMETERS: 
 *
 *  RETURNS: 
 *
 *  CREATED: 1/20/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static Int16 DateTypeCmp(DateType d1, DateType d2)
{
	Int16 result;
	
	result = d1.year - d2.year;
	if (result != 0)
		{
		if ((*(int *) &d1) == -1)
			return 1;
		if ((*(int *) &d2) == -1)
			return -1;
		return result;
		}
	
	result = d1.month - d2.month;
	if (result != 0)
		return result;
	
	result = d1.day - d2.day;

	return result;

}


/************************************************************
 *
 *  FUNCTION: ExpenseTypeCmp
 *
 *  DESCRIPTION: Compare two expense types
 *
 *  PARAMETERS: 
 *
 *  RETURNS: Sorting result (< 0, 0, > 0)
 *
 *  CREATED: 11/27/98
 *
 *  BY: Konno Hiroharu
 *
 *	HISTORY:
 *		1999-11-10	jwm	noExpenseTypes now go to the bottom
 *
 *************************************************************/
static Int16 ExpenseTypeCmp (UInt8 type1, UInt8 type2)
{
	MemHandle expensesH;
	
	Int16 result = 0;
	
	if (type1 != type2)
		{
		if (type1 == noExpenseType)
			{
			result = +1;
			}
		else if (type2 == noExpenseType)
			{
			result = -1;
			}
		else if (FtrGet (sysFileCExpense, expFeatureNum, (UInt32*) &expensesH) != 0)
			{
			/* FtrGet returns an error */
			if (type1 > type2)
				result = 1;
			else
				result = -1;
			}
		else
			{
			UInt8* expensesP = MemHandleLock (expensesH);
			result = (Int16) expensesP[type1] - (Int16) expensesP[type2];
			MemPtrUnlock (expensesP);
			}
		}
		
	return (result);
}


/************************************************************
 *
 *  FUNCTION: ExpenseAppInfoInit
 *
 *  DESCRIPTION: Create an app info chunk if missing.  Set
 *		the strings to a default.
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS: 0 if successful, errorcode if not
 *
 *  CREATED: 1/20/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
Err	ExpenseAppInfoInit (DmOpenRef dbP)
{
	UInt16 cardNo;
	MemHandle h;
	LocalID dbID;
	LocalID appInfoID;
	ExpenseAppInfoPtr appInfoP;
	ExpenseAppInfoPtr	nilP = 0;
	
	if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
		return dmErrInvalidParam;

	if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, 
		 NULL, &appInfoID, NULL, NULL, NULL))
		return dmErrInvalidParam;
	
	if (appInfoID == NULL)
		{
		h = DmNewHandle(dbP, sizeof (ExpenseAppInfoType));
		if (! h) return dmErrMemError;

		appInfoID = MemHandleToLocalID (h);
		DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, 
			NULL, &appInfoID, NULL, NULL, NULL);
		}
	
	appInfoP = MemLocalIDToLockedPtr(appInfoID, cardNo);

	// Clear the app info block.
	DmSet (appInfoP, 0, sizeof (ExpenseAppInfoType), 0);

	// Initialize the categories.
	CategoryInitialize ((AppInfoPtr) appInfoP, expenseLocalizedAppInfoStr);

	// Initialize the sort order.
	DmSet (appInfoP, (UInt32)&nilP->sortOrder, sizeof(appInfoP->sortOrder), 
		sortByDate);

	MemPtrUnlock (appInfoP);

	return 0;
}


/************************************************************
 *
 *  FUNCTION: ExpenseGetAppInfo
 *
 *  DESCRIPTION: Get the app info chunk 
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS: MemHandle to the expense application info block (ExpenseAppInfoType)
 *
 *  CREATED: 5/12/95 
 *
 *  BY: Art Lamb
 *
 *************************************************************/
MemHandle ExpenseGetAppInfo (DmOpenRef dbP)
{
	Err error;
	UInt16 cardNo;
	LocalID dbID;
	LocalID appInfoID;
	
	error = DmOpenDatabaseInfo (dbP, &dbID, NULL, NULL, &cardNo, NULL);
	ErrFatalDisplayIf (error,  "Get getting expense app info block");

	error = DmDatabaseInfo (cardNo, dbID, NULL, NULL, NULL, NULL, NULL, 
			NULL, NULL, &appInfoID, NULL, NULL, NULL);
	ErrFatalDisplayIf (error,  "Get getting expense app info block");

	return ((MemHandle) MemLocalIDToGlobal (appInfoID, cardNo));
}


/************************************************************
 *
 *	FUNCTION:	ExpenseCompareRecords
 *
 *	DESCRIPTION: Compare two records key by key until
 *		there is a difference.  Return -n if r1 is less or n if r2
 *		is less. Note that a zero could be returned if both the
 * 	date and expense type are the same, which menas that expense
 *		entries which match in these two respects wind up having a
 *		random order. DOLATER - maybe ensure that 0 never gets
 * 	returned by comparing their unique IDs
 *
 * 	This function accepts record data chunk pointers to avoid
 * 	requiring that the record be within the database.  This is
 * 	important when adding records to a database.  This prevents
 * 	determining if a record is a deleted record (which are kept
 * 	at the end of the database and should be considered "greater").
 * 	The caller should test for deleted records before calling this
 * 	function!
 *
 *	PARAMETERS:
 *		database record 1
 *		database record 2
 *
 *	RETURNS: -n if record one is less (n != 0)
 *			  n if record two is less
 *
 *	HISTORY:
 *		01/23/95	rsf	Created by Roger Flores.
 *		10/20/99	kwk	Enabled Konno-san's two-pass sorting code,
 *							which uses the date (or expense type) to
 *							break ties.
 *
 *************************************************************/
#define maxExpenseNameLength	25

static Int16 ExpenseCompareRecords (ExpensePackedRecordPtr r1, ExpensePackedRecordPtr r2, 
	Int16 sortOrder, SortRecordInfoPtr /*info1*/, SortRecordInfoPtr /*info2*/,
	MemHandle /*appInfoH*/)
	{
	Int16 result;
	if (sortOrder == sortByDate)
		{
		// Sort by date.
		result = DateTypeCmp (r1->date, r2->date);
		if (result == 0)
			{
			result = ExpenseTypeCmp (r1->type, r2->type);
			}
		}
	else
		{
		// Sort by expense.
		result = ExpenseTypeCmp (r1->type, r2->type);
		if (result == 0)
			{
			result = DateTypeCmp (r1->date, r2->date);
			}
		}

	return (result);
	}


/************************************************************
 *
 *  FUNCTION: ExpenseGetSortOrder
 *
 *  DESCRIPTION: This routine get the sort order value from the 
 *               to do application info block.
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS:    true if the to do record are sorted by priority, 
 *              false if the records are sorted by due date.
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/21/96	Initial Revision
 *
 *************************************************************/
UInt8 ExpenseGetSortOrder (DmOpenRef dbP)
{
	UInt8 sortOrder;
	ExpenseAppInfoPtr appInfoP;
			
	appInfoP = MemHandleLock (ExpenseGetAppInfo (dbP));
	sortOrder = appInfoP->sortOrder;
	MemPtrUnlock (appInfoP);	

	return (sortOrder);
}


/************************************************************
 *
 *  FUNCTION: ExpenseFindSortPosition
 *
 *  DESCRIPTION: Return where a record is or should be
 *		Useful to find or find where to insert a record.
 *
 *  PARAMETERS: database record (not deleted!)
 *
 *  RETURNS: the size in bytes
 *
 *  CREATED: 1/11/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static UInt16 ExpenseFindSortPosition (DmOpenRef dbP, ExpensePackedRecord *newRecord)
{
	Int16 sortOrder;
	
	sortOrder = ExpenseGetSortOrder (dbP);

	return (DmFindSortPosition (dbP, newRecord, NULL, 
		(DmComparF *)ExpenseCompareRecords, sortOrder));
}




/************************************************************
 *
 *  FUNCTION:    ExpenseChangeSortOrder
 *
 *  DESCRIPTION: Change the Expense Database's sort order
 *
 *  PARAMETERS:  database pointer
 *				     TRUE if sort by company
 *
 *  RETURNS:     nothing
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/21/96	Initial Revision
 *
 *************************************************************/
Err ExpenseChangeSortOrder(DmOpenRef dbP, Boolean sortOrder)
{
	ExpenseAppInfoPtr appInfoP;
	ExpenseAppInfoPtr	nilP = 0;


	appInfoP = MemHandleLock (ExpenseGetAppInfo (dbP));

	if (appInfoP->sortOrder != sortOrder)
		{
		DmWrite(appInfoP, (UInt32)&nilP->sortOrder, &sortOrder, sizeof(appInfoP->sortOrder));
		
		DmInsertionSort(dbP, (DmComparF *) &ExpenseCompareRecords, (Int16) sortOrder);
		}
		
	MemPtrUnlock (appInfoP);	

	return 0;
}



/************************************************************
 *
 *  FUNCTION: ExpenseSort
 *
 *  DESCRIPTION: Sort the appointment database.
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: nothing
 *
 *  CREATED: 10/17/95 
 *
 *  BY: Art Lamb
 *
 *************************************************************/
void ExpenseSort (DmOpenRef dbP)
{
	Int16 sortOrder;
	
	sortOrder = ExpenseGetSortOrder (dbP);

	DmInsertionSort(dbP, (DmComparF *) &ExpenseCompareRecords, sortOrder);
}


/************************************************************
 *
 *  FUNCTION: 	  ExpensetPackedSize
 *
 *  DESCRIPTION: Returns the packed size of an ExpensePackedRecordType 
 *
 *  PARAMETERS:  database record
 *
 *  RETURNS:     the size in bytes
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 *************************************************************/
static UInt16 ExpensePackedSize (ExpenseRecordPtr r)
{
	UInt16 size;

	
	size = sizeof (ExpensePackedRecord);
	
	if (r->amount)
		size += StrLen (r->amount) + 1;
	else
		size++;

	if (r->vendor)
		size += StrLen (r->vendor) + 1;
	else
		size++;

	if (r->city)
		size += StrLen (r->city) + 1;
	else
		size++;

	if (r->attendees)
		size += StrLen (r->attendees) + 1;
	else
		size++;

	if (r->note)
		size += StrLen (r->note) + 1;
	else
		size++;

	return size;
}


/************************************************************
 *
 *  FUNCTION: ExpenseUnpack
 *
 *  DESCRIPTION: Fills in the ExpensePackedRecord structure
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: the record unpacked
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/21/96	Initial Revision
 *
 *************************************************************/
static void ExpenseUnpack (ExpensePackedRecordPtr src, ExpenseRecordPtr dest)
{
	Char * p;

	
	dest->date = src->date;
	dest->type = src->type;
	dest->paymentType = src->paymentType;
	dest->currency = src->currency;


	p = (Char *)src + sizeof(ExpensePackedRecord);
	
	// Get the amount.
	dest->amount = p;
	p += StrLen(p) + 1;

	// Get the vendor
	dest->vendor = p;
	p += StrLen(p) + 1;

	// Get the city
	dest->city = p;
	p += StrLen(p) + 1;

	// Get the attendees
	dest->attendees = p;
	p += StrLen(p) + 1;

	// Get the note
	dest->note = p;
}


/************************************************************
 *
 *  FUNCTION:    ExpensePack
 *
 *  DESCRIPTION: Pack an ExpenseRecordPtr
 *
 *  PARAMETERS:  database record
 *
 *  RETURNS:     a pack ExpensePackedRecord
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 *************************************************************/
static void ExpensePack (ExpenseRecordPtr src, ExpensePackedRecordPtr dest)
{
	Char							zero = 0;
	UInt32						offset;
	ExpensePackedRecordPtr	nilP = 0;
	
	
	DmWrite (dest, (UInt32)&nilP->date, &src->date, sizeof(nilP->date));
	DmWrite (dest, (UInt32)&nilP->type, &src->type, sizeof(nilP->type));
	DmWrite (dest, (UInt32)&nilP->paymentType, &src->paymentType, sizeof(nilP->paymentType));
	DmWrite (dest, (UInt32)&nilP->currency, &src->currency, sizeof(nilP->currency));

	offset = sizeof (ExpensePackedRecord);


	// Add the amount.
	if (src->amount)
		{
		DmStrCopy (dest, offset, src->amount);
		offset += StrLen (src->amount) + 1;
		}
	else
		{
		DmWrite (dest, offset, &zero, 1);
		offset++;
		}
	

	// Add the vendor.
	if (src->vendor)
		{
		DmStrCopy (dest, offset, src->vendor);
		offset += StrLen (src->vendor) + 1;
		}
	else
		{
		DmWrite (dest, offset, &zero, 1);
		offset++;
		}
	

	// Add the city.
	if (src->amount)
		{
		DmStrCopy (dest, offset, src->city);
		offset += StrLen (src->city) + 1;
		}
	else
		{
		DmWrite (dest, offset, &zero, 1);
		offset++;
		}
	

	// Add the attendees.
	if (src->attendees)
		{
		DmStrCopy (dest, offset, src->attendees);
		offset += StrLen (src->attendees) + 1;
		}
	else
		{
		DmWrite (dest, offset, &zero, 1);
		offset++;
		}
	

	// Add the note.
	if (src->note)
		{
		DmStrCopy (dest, offset, src->note);
		}
	else
		{
		DmWrite (dest, offset, &zero, 1);
		}
}


/************************************************************
 *
 *  FUNCTION: ExpenseGetRecord
 *
 *  DESCRIPTION: Get a record from a Appointment Database
 *
 *  PARAMETERS: dbP     - database pointer
 *				    index   - database index
 *				    r       - database record
 *              handleP - returned: handled of record
 *              
 *
 *  RETURNS: 	errorcode, 0 if successful
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/21/96	Initial Revision
 *
 *************************************************************/
Err ExpenseGetRecord (DmOpenRef dbP, UInt16 index, ExpenseRecordPtr r, 
	MemHandle * handleP)
{
	MemHandle MemHandle;
	ExpensePackedRecordPtr src;
	

	MemHandle = DmQueryRecord (dbP, index);
	ErrFatalDisplayIf(DmGetLastErr(), "Error Querying record");
	
	src = MemHandleLock (MemHandle);

	if (DmGetLastErr())
		{
		*handleP = 0;
		return DmGetLastErr();
		}
	
	ExpenseUnpack (src, r);
	
	*handleP = MemHandle;
	return 0;
}


/************************************************************
 *
 *  FUNCTION: ExpenseNewRecord
 *
 *  DESCRIPTION: Create a new record in sorted position
 *
 *  PARAMETERS: database pointer
 *				database record
 *
 *  RETURNS: ##0 if successful, errorcode if not
 *
 *  CREATED: 1/15/96
 *
 *  BY: Art Lamb
 *
 *************************************************************/
Err ExpenseNewRecord (DmOpenRef dbP, ExpenseRecordPtr item, UInt16 *index)
{
	Err 						err;
	Int16						size;
	MemHandle 				recordH;
	UInt16 						newIndex;
	ExpensePackedRecordPtr	recordP;
	
	ErrNonFatalDisplayIf(item->type >= numExpenseTypes && item->type != noExpenseType, "invalid expense type");
	
	// Compute the size of the new expense record.
	size = ExpensePackedSize (item);
		
	//  Allocate a chunk in the database for the new record.
	recordH = (MemHandle)DmNewHandle(dbP, (UInt32) size);
	if (recordH == NULL)
		return dmErrMemError;

	// Pack the the data into the new record.
	recordP = MemHandleLock (recordH);
	ExpensePack (item, recordP);
	ErrNonFatalDisplayIf(recordP->type >= numExpenseTypes && recordP->type != noExpenseType, "invalid expense type");
	
		
	// Determine the sort position of the new record.
	newIndex = ExpenseFindSortPosition (dbP, recordP);

	MemPtrUnlock (recordP);

	// Insert the record.
	err = DmAttachRecord(dbP, &newIndex, recordH, 0);
	if (err) 
		MemHandleFree(recordH);
	else
		*index = newIndex;

	return err;
}


/************************************************************
 *
 *  FUNCTION:    ExpenseChangeRecordField
 *
 *  DESCRIPTION: Replace a field in a record with the passed value.
 *
 *  PARAMETERS:  scr    - packed database record
 *               size   - size of packed database record
 *               data   - data to add to the record
 *               fieldP - position to write to
 *
 *  RETURNS:     nothing
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/22/96	Initial Revision
 *
 *************************************************************/
static void ExpenseChangeRecordField (ExpensePackedRecordPtr src, 
	UInt32 size, void * data, Char * fieldP)
{
	UInt32	offset;
	UInt32	bytes;
	Char * 	ptr;

	// Move the rest of the record.
	offset = (fieldP - (Char *)src) + StrLen (data) + 1;
	ptr = fieldP + StrLen (fieldP) + 1;
	bytes = size - (UInt32)(ptr - (Char *)src);
	if (bytes)
		DmWrite (src, offset, ptr, bytes);
			
	// Write the new vendor field.
	offset = (fieldP - (Char *)src);
	DmStrCopy (src, offset, data);
}


/************************************************************
 *
 *  FUNCTION: ExpenseChangeRecord
 *
 *  DESCRIPTION: Change a record in the Expense Database
 *
 *  PARAMETERS: database pointer
 *					 database index
 *					 database record
 *					 changed fields
 *
 *  RETURNS: ##0 if successful, errorcode if not
 *
 *  CREATED: 1/14/95 
 *
 *  BY: Roger Flores
 *
 *	COMMENTS:	Records are not stored with extra padding - they
 *	are always resized to their exact storage space.  This avoids
 *	a database compression issue.  The code works as follows:
 *	
 *
 *************************************************************/
Err ExpenseChangeRecord(DmOpenRef dbP, UInt16 *index, 
	ExpenseRecordFieldType changedField, void * data)
{
	Err 							err;
	UInt16 						curSize;
	UInt16 						newSize;
	UInt16 						newIndex;
	MemHandle 					recordH;
	ExpenseRecordType			record;
	ExpensePackedRecord		temp;
	ExpensePackedRecordPtr 	src;
	ExpensePackedRecordPtr 	nilP = 0;
	
	// Get the record which we are going to change
	recordH = DmQueryRecord(dbP, *index);
	src = MemHandleLock (recordH);
	

	// If the  record is being changes such that its sort position will 
	// change,  move the record to its new sort position.
	if (changedField == expenseDate || changedField == expenseType)
		{
		if (changedField == expenseDate)
			{
			temp.type = src->type;
			temp.date = *((DatePtr)data);
			}
		else
			{
			temp.type = *((UInt8 *)data);
			temp.date = src->date;
			}
		newIndex = ExpenseFindSortPosition (dbP, &temp);
		DmMoveRecord (dbP, *index, newIndex);
		if (newIndex > *index) newIndex--;
		*index = newIndex;
		}


	if (changedField == expenseDate)
		{ 
		DmWrite (src, (UInt32)&nilP->date, data, sizeof(src->date));
		goto exit;
		}

	if (changedField == expenseType)
		{
		DmWrite (src, (UInt32)&nilP->type, data, sizeof(src->type));
		ErrNonFatalDisplayIf(src->type >= numExpenseTypes && src->type != noExpenseType, "invalid expense type");
		goto exit;
		}
			
	if (changedField == expensePaymentType)
		{
		DmWrite (src, (UInt32)&nilP->paymentType, data, sizeof(src->paymentType));
		goto exit;
		}
			
	if (changedField == expenseCurrency)
		{
		DmWrite (src, (UInt32)&nilP->currency, data, sizeof(src->currency));
		goto exit;
		}
			

	// Calculate the size of the changed record
	curSize = MemPtrSize (src);
	ExpenseUnpack (src, &record);
	newSize = curSize;
	
	if (changedField == expenseAmount)
		newSize += StrLen (data) - StrLen (record.amount);

	else if (changedField == expenseVendor)
		newSize += StrLen (data) - StrLen (record.vendor);

	else if (changedField == expenseCity)
		newSize += StrLen (data) - StrLen (record.city);

	else if (changedField == expenseAttendees)
		newSize += StrLen (data) - StrLen (record.attendees);

	else if (changedField == expenseNote)
		newSize += StrLen (data) - StrLen (record.note);


	// If the new record is longer, expand the record.
	if (newSize > curSize)
		{
		MemPtrUnlock (src);
		err = MemHandleResize (recordH, newSize);
		if (err) return (err);

		src = MemHandleLock (recordH);
		ExpenseUnpack (src, &record);
		}


	// Change the amount field.
	if (changedField == expenseAmount)
		ExpenseChangeRecordField (src, curSize, data, record.amount);

	// Change the vendor field.
	if (changedField == expenseVendor)
		ExpenseChangeRecordField (src, curSize, data, record.vendor);

	// Change the city field.
	else if (changedField == expenseCity)
		ExpenseChangeRecordField (src, curSize, data, record.city);

	// Change the attendees field.
	else if (changedField == expenseAttendees)
		ExpenseChangeRecordField (src, curSize, data, record.attendees);

	// Change the note field
	else if (changedField == expenseNote)
		ExpenseChangeRecordField (src, curSize, data, record.note);


	// If the new record is shorter, shrink the record.
	if (newSize < curSize)
		MemPtrResize (src, newSize);


exit:
	MemPtrUnlock (src);

	return 0;
}

