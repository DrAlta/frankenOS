/***********************************************************************
 *
 * PROJECT:  Librarian
 * FILE:     librarianDB.c
 * AUTHOR:   Lonnon R. Foster
 *
 * DESCRIPTION:  Database routines for Librarian.
 *
 * From Palm OS Programming Bible
 * ©2000 Lonnon R. Foster.  All rights reserved.
 *
 ***********************************************************************/

#include <PalmOS.h>
#include "librarian.h"
////#include "librarianDB.h"
#include "librarianRsc.h"


// Utility macros for handling bit fields
#define BitAtPosition(pos)               ((UInt16)1 << (pos))
#define GetBitMacro(bitfield, index)     ((bitfield) & BitAtPosition(index))
#define SetBitMacro(bitfield, index)     ((bitfield) |= BitAtPosition(index))
#define RemoveBitMacro(bitfield, index)  ((bitfield) &= ~BitAtPosition(index))

// Macro to determine if any of the fields used to sort the database are
// present in a given record.
#define sortKeyFieldBits   (BitAtPosition(libFieldTitle) | \
                            BitAtPosition(libFieldLastName) | \
                            BitAtPosition(libFieldFirstName) | \
                            BitAtPosition(libFieldYear) )


/***********************************************************************
 *
 * FUNCTION:    LibAppInfoInit
 *
 * DESCRIPTION: Creates an app info chunk if missing and sets the
 *              category and status strings to default values.
 *
 * PARAMETERS:  db - open database pointer
 *
 * RETURNS:     0 if successful, error code if not
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
Err LibAppInfoInit(DmOpenRef db)
{
    UInt16      cardNo;
    LocalID   dbID, appInfoID;
    MemHandle    h;
    LibAppInfoType  *appInfo, *nil = 0;
    UInt8      defaultShowInList;
   
   
    if (DmOpenDatabaseInfo(db, &dbID, NULL, NULL, &cardNo, NULL))
        return dmErrInvalidParam;
    if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL,
                       NULL, NULL, &appInfoID, NULL, NULL, NULL))
        return dmErrInvalidParam;
		
    // If there isn't an app info block make space for one.
    if (appInfoID == NULL) {
        h = DmNewHandle(db, sizeof(LibAppInfoType));
        if (!h) return dmErrMemError;
		
        appInfoID = MemHandleToLocalID(h);
        DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL,
                          NULL, NULL, &appInfoID, NULL, NULL, NULL);
    }
	
    appInfo = MemLocalIDToLockedPtr(appInfoID, cardNo);
	
    // Clear the app info block.
    DmSet(appInfo, 0, sizeof(LibAppInfoType), 0);
	
    // Initialize categories.
    CategoryInitialize((AppInfoPtr) appInfo, CategoryAppInfoStr);
	
    // Initialize record sort order.
    defaultShowInList = libShowAuthorTitle;
    DmWrite(appInfo, (UInt32) &nil->showInList, &defaultShowInList,
            sizeof(appInfo->showInList));
	        
    // Initialize field labels for Edit view.
    LibGetAppInfoStr(appInfo, EditFieldLabelsAppInfoStr,
                     libNumFieldLabels, (UInt32) &appInfo->fieldLabels);
	
    // Initialize status strings for Record view.
    LibGetAppInfoStr(appInfo, BookStatusAppInfoStr,
                     libNumBookStatusStrings,
                     (UInt32) &appInfo->bookStatusStrings);
    LibGetAppInfoStr(appInfo, PrintStatusAppInfoStr,
                     libNumPrintStatusStrings,
                     (UInt32) &appInfo->printStatusStrings);
    LibGetAppInfoStr(appInfo, FormatStatusAppInfoStr,
                     libNumFormatStatusStrings,
                     (UInt32) &appInfo->formatStatusStrings);
    LibGetAppInfoStr(appInfo, ReadStatusAppInfoStr,
                     libNumReadStatusStrings,
                     (UInt32) &appInfo->readStatusStrings);
	
    MemPtrUnlock(appInfo);
	
    return 0;
}


/***********************************************************************
 *
 * FUNCTION:    LibChangeRecord
 *
 * DESCRIPTION: Change a record in the Librarian database.  Records are
 *              not stored with extra padding; they are always resized
 *              to their exact storage space.  This keeps the database
 *              small.  The handle to the record passed does not need
 *              to be unlocked; LibChangeRecord takes care of that.
 *
 * PARAMETERS:  db - open database pointer
 *              index - pointer to the index of the record to change
 *              record - pointer to the unpacked record data
 *              changeFields - fields changed
 *
 * RETURNS:     0 if successful, error code if not
 *
 * REVISION HISTORY:  
 *
 *
 ***********************************************************************/
Err LibChangeRecord(DmOpenRef db, UInt16 *index, LibDBRecordType *record, 
                    LibDBRecordFlags changedFields)
{
    LibDBRecordType  src;
    MemHandle  srcH;
    Err        result;
    MemHandle  recordH = 0;
    MemHandle  oldH;
    Int16      i;
    UInt32     changes = changedFields.allBits;
    Int16      showInList;
    LibAppInfoType  *appInfo;
    Boolean    move = true;
    UInt16     attributes;
    LibPackedDBRecord*  cmp;
    LibPackedDBRecord*  packed;
	
	
    // LibChangeRecord does not assume that record is completely valid,
    // so it retrieves a valid pointer to the record.
    if ((result = LibGetRecord(db, *index, &src, &srcH)) != 0)
        return result;
   
    // Apply the changes to the valid record.
    src.status = record->status;
    for (i = 0; i < libFieldsCount; i++)  {
        // If the flag is set, point to the string, otherwise point to
        // NULL.
        if (GetBitMacro(changes, i) != 0) {
            src.fields[i] = record->fields[i];
            RemoveBitMacro(changes, i);
        }
        if (changes == 0)
            break;      // no more changes
    }

    // Make a new chunk with the correct size.
    recordH = DmNewHandle(db, LibUnpackedSize(&src));
    if (recordH == NULL) {
        MemHandleUnlock(srcH);  // undo lock from LibGetRecord above
        return dmErrMemError;
    }
    packed = MemHandleLock(recordH);

    // Copy the data from the unpacked record to the packed one.
    PackRecord(&src, packed);
	
    // The original record is copied and no longer needed.
    MemHandleUnlock(srcH);
	
    // Check if any of the key fields have changed.  If they have not
    // changed, this record is already in its proper place in the
    // database, and LibChangeRecord can skip re-sorting the record.
    if ((changedFields.allBits & sortKeyFieldBits) == 0) 
        move = false;
   
    // Make sure *index-1 < *index < *index+1; if so, the record is
    // already in sorted order.  Deleted records are stored at the end
    // of the database, so LibChangeRecord must also make sure not to
    // sort this record past the end of any deleted records. 
    if (move) {
        appInfo = MemHandleLock(LibGetAppInfo(db));
        showInList = appInfo->showInList;
        MemPtrUnlock(appInfo);
	
        if (*index > 0) {
            // Compare this record to the record before it.
            cmp = MemHandleLock(DmQueryRecord(db, *index - 1));
            move = (LibComparePackedRecords(cmp, packed, showInList,
                                            NULL, NULL, NULL) > 0);
            MemPtrUnlock(cmp);
        } else {
            move = false;
        }
		    
        if (*index + 1 < DmNumRecords(db)) {
            // Be sure not to move the record beyond the deleted
            // records at the end of the database.
            DmRecordInfo(db, *index + 1, &attributes, NULL, NULL);
            if (! (attributes & dmRecAttrDelete)) {
				// Compare this record to the record after it.
                cmp = MemHandleLock(DmQueryRecord(db, *index + 1));
                move = (! move) && (LibComparePackedRecords(packed,
                                                            cmp, showInList, NULL, NULL,
                                                            NULL) > 0);
                MemPtrUnlock(cmp);
            }
        }
    }
	
    if (move) {
        // The record isn't in the right position, so move it.
        i = LibFindSortPosition(db, packed);
        DmMoveRecord(db, *index, i);
        if (i > *index)
            i--;
        *index = i;  // Return new record database position.
    }

    // Attach the new record to the old index, which preserves the 
    // category and record ID.
    result = DmAttachRecord(db, index, recordH, &oldH);
    MemPtrUnlock(packed);
    if (result) return result;
	
    MemHandleFree(oldH);
    return 0;
}


/***********************************************************************
 *
 * FUNCTION:    LibChangeSortOrder
 *
 * DESCRIPTION: Changes the sort order of the Librarian database.
 *
 * PARAMETERS:  db - open database pointer
 *              showInList - sort order to use
 *
 * RETURNS:     nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
void LibChangeSortOrder(DmOpenRef db, UInt8 showInList)
{
    LibAppInfoType  *appInfo, *nil = 0;
	
	
    // Set the new sort order in Librarian's application info block.
    appInfo = MemHandleLock(LibGetAppInfo(db));
    DmWrite(appInfo, (UInt32) &nil->showInList, &showInList,
            sizeof(appInfo->showInList));
    MemPtrUnlock(appInfo);
	
    DmQuickSort(db, (DmComparF *) &LibComparePackedRecords,
                (UInt16) showInList);
}


/***********************************************************************
 *
 * FUNCTION:    LibComparePackedRecords
 *
 * DESCRIPTION: Compares two packed records.  This function is a
 *              callback used by DmFindSortPosition, DmInsertionSort,
 *              and DmQuickSort.
 *
 * PARAMETERS:  r1 - pointer to the first record to compare
 *              r2 - pointer to the second record to compare
 *
 * RETURNS:     integer < 0 if r1 < r2
 *              integer > 0 if r1 > r2
 *              0 if r1 == r2
 *
 *  Empty (NULL) fields come before fields containing data.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Int16 LibComparePackedRecords(LibPackedDBRecord *r1,
                                     LibPackedDBRecord *r2, Int16 showInList, SortRecordInfoPtr info1,
                                     SortRecordInfoPtr info2, MemHandle appInfoH)
{
    UInt16  whichKey1, whichKey2;
    char  *key1, *key2;
    Int16   result;
	
	
#ifdef __GNUC__
    // CALLBACK_PROLOGUE;
#endif

    // Records that don't contain data in the primary sort field
    // for the current sort order should be sorted before records that
    // do contain data.  For example, in libShowAuthorTitle sort order,
    // any record containing author data should come after a record
    // without an author.
    switch (showInList) {
        case libShowAuthorTitle:
            // Does r1 have empty author data?
            if ( (! r1->flags.bits.lastName) &&
                 (! r1->flags.bits.firstName) ) {
				// Does r2 have empty author data?
                if ( (! r2->flags.bits.lastName) &&
                     (! r2->flags.bits.firstName) )
                    // Neither r1 nor r2 contains author data, so
                    // LibComparePackedRecords needs to compare
                    // the records field by field to determine sort
                    // order.
                    break;
				// r1 has no author data, r2 does have author data.
				// Therefore, r1 < r2.
                else {
                    result = -1;
                    return result;
                }
            }
            else {
				// r1 has author data, r2 does not have author data.
				// Therefore, r1 > r2
                if ( (! r2->flags.bits.lastName) &&
                     (! r2->flags.bits.firstName) ) {
                    result = 1;
                    return result;
                }
            }
            break;
			
        case libShowTitleAuthor:
        case libShowTitleOnly:
            // Does r1 have empty title data?
            if (! r1->flags.bits.title) {
				// Does r2 have empty author data?
                if (! r2->flags.bits.title)
                    // Neither r1 nor r2 contains title data, so
                    // LibComparePackedRecords needs to compare
                    // the records field by field to determine sort
                    // order.
                    break;
				// r1 has no title data, r2 does have title data.
				// Therefore, r1 < r2.
                else {
                    result = -1;
                    return result;
                }
            }
            else {
				// r1 has title data, r2 does not have title data.
				// Therefore, r1 > r2
                if (! r2->flags.bits.title) {
                    result = 1;
                    return result;
                }
            }
            break;
			
        default:
            break;
    }
	
    // Both records contain primary key data, or both records have
    // empty primary key data.  Either way, LibComparePackedRecords
    // must now compare the two records field by field to determine
    // sort order.
    whichKey1 = 1;
    whichKey2 = 1;
	
    do {
        LibFindKey(r1, &key1, &whichKey1, showInList);
        LibFindKey(r2, &key2, &whichKey2, showInList);
      
        // A key with NULL loses the StrCompare.
        if (key1 == NULL) {
            // If both are NULL then return them as equal.
            if (key2 == NULL) {
                result = 0;
                return result;
            }
            else
                result = -1;
        }
        else if (key2 == NULL)
            result = 1;
        else {
            result = StrCaselessCompare(key1, key2);
            if (result == 0)
                result = StrCompare(key1, key2);
        }

    } while (! result);
   
#ifdef __GNUC__
    // CALLBACK_EPILOGUE;
#endif

    return result;
}


/***********************************************************************
 *
 * FUNCTION:    LibFindKey
 *
 * DESCRIPTION: Returns the next valid key for comparing a record to
 *              another record.  If possible, it returns the key asked
 *              for in the key parameter, then advances the whichKey
 *              parameter.  If the key is not available, LibFindKey
 *              advances whichKey to the next key.  The order of keys is
 *              as follows, depending on the showInList value:
 *
 *              if (showInList == libShowAuthorTitle)
 *                  lastName, firstName, title, year
 *
 *              if (showInList == libShowTitleAuthor)
 *                  title, lastName, firstName, year
 *
 *              if (showInList == libShowTitleOnly)
 *                  title, lastName, firstName, year
 *
 * PARAMETERS:  record      -> packed Librarian record
 *              key        <-> key to use (pointer to string or NULL if no
 *                             key contains any data)
 *              whichKey   <-> which key (incremented for use again,
 *                             starts at 1)
 *              showInList  -> sort order
 *
 * RETURNS:     nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void LibFindKey(LibPackedDBRecord *record, char **key,
                       UInt16 *whichKey, Int16 showInList)
{
    LibDBRecordFlags  fieldFlags;
	
	
    fieldFlags.allBits = record->flags.allBits;
	
    ErrFatalDisplayIf(*whichKey == 0 || *whichKey == 6, "Bad sort key");
	
    switch (showInList) {
        case libShowAuthorTitle:
            if (*whichKey == 1 && fieldFlags.bits.lastName) {
                *whichKey = 2;
                goto returnLastNameKey;
            }
            if (*whichKey <= 2 && fieldFlags.bits.firstName) {
                *whichKey = 3;
                goto returnFirstNameKey;
            }
            if (*whichKey <= 3 && fieldFlags.bits.title) {
                *whichKey = 4;
                goto returnTitleKey;
            }
            if (*whichKey <= 4 && fieldFlags.bits.year) {
                *whichKey = 5;
                goto returnYearKey;
            }
            break;
			
        case libShowTitleAuthor:
        case libShowTitleOnly:
            if (*whichKey == 1 && fieldFlags.bits.title) {
                *whichKey = 2;
                goto returnTitleKey;
            }
            if (*whichKey <= 2 && fieldFlags.bits.lastName) {
                *whichKey = 3;
                goto returnLastNameKey;
            }
            if (*whichKey <= 3 && fieldFlags.bits.firstName) {
                *whichKey = 4;
                goto returnFirstNameKey;
            }
            if (*whichKey <= 4 && fieldFlags.bits.year) {
                *whichKey = 5;
                goto returnYearKey;
            }
            break;
			
        default:
            break;
    }
	
    // All possible fields have been tried.
    *whichKey = 7;
    *key = NULL;
    return;

  returnTitleKey:
    *key = &record->firstField;
    return;
   
  returnLastNameKey:
    *key = (char *) &record->firstField + record->lastNameOffset;
    return;
   
  returnFirstNameKey:
    *key = (char *) &record->firstField + record->firstNameOffset;
    return;

  returnYearKey:
    *key = (char *) &record->firstField + record->yearOffset;
    return;
}


/***********************************************************************
 *
 * FUNCTION:    LibFindSortPosition
 *
 * DESCRIPTION: Returns the correct index in the database for a packed
 *              Librarian record.  This function may be used to find the
 *              index of an existing record, or to find the location
 *              where a new record should be inserted.
 *
 * PARAMETERS:  db - open database pointer
 *              record - pointer to a packed Librarian record
 *
 * RETURNS:     index of the record's proper database location
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt16 LibFindSortPosition(DmOpenRef db, LibPackedDBRecord *record)
{
    UInt8  showInList;
    LibAppInfoType  *appInfo;

   
    // Retrieve the current sort order from Librarian's application info
    // block.
    appInfo = MemHandleLock(LibGetAppInfo(db));
    showInList = appInfo->showInList;
    MemPtrUnlock(appInfo);
	     
    return DmFindSortPosition(db, (MemPtr) record, NULL, (DmComparF *) 
                              &LibComparePackedRecords, (UInt16) showInList);
}


/***********************************************************************
 *
 * FUNCTION:    LibGetAppInfo
 *
 * DESCRIPTION: Returns a handle to Librarian's application info block
 *
 * PARAMETERS:  db - open database pointer
 *
 * RETURNS:     handle to the application info block
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
MemHandle LibGetAppInfo(DmOpenRef db)
{
    Err      error;
    UInt16     cardNo;
    LocalID  dbID, appInfoID;
	
	
    error = DmOpenDatabaseInfo(db, &dbID, NULL, NULL, &cardNo, NULL);
    ErrFatalDisplayIf(error, "Error retrieving Librarian app info block");

    error = DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, 
                           NULL, NULL, &appInfoID, NULL, NULL, NULL);
    ErrFatalDisplayIf(error, "Error retrieving Librarian app info block");

    return ((MemHandle) MemLocalIDToGlobal(appInfoID, cardNo));
}   


/***********************************************************************
 *
 * FUNCTION:    LibGetAppInfoStr
 *
 * DESCRIPTION: Retrieves field labels for Edit view and status strings
 *              for Record view from Librarian's resources and fills
 *              the appropriate array in the application info block.
 *
 * PARAMETERS:  appInfo - pointer to the application info block
 *              resourceID - resource ID of the AppInfoStr that contains
 *                           the desired array elements
 *              stringCount - number of strings in the array
 *              arrayAddress - address of the array in the app info
 *                             block that should be filled
 *
 * RETURNS:     nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void LibGetAppInfoStr(LibAppInfoType *appInfo, Int16 resourceID,
                             Int16 stringCount, UInt32 arrayAddress)
{
    MemHandle  rscH, stringArrayH;
    Char       *rsc;
    Char       **stringArray;
    Int16      i;
    UInt32     offset;
	
	
    // Retrieve the strings from a string list in Librarian's resources.
    rscH = DmGetResource(appInfoStringsRsc, resourceID);
    rsc = MemHandleLock(rscH);
    stringArrayH = SysFormPointerArrayToStrings(rsc, stringCount);
    stringArray = MemHandleLock(stringArrayH);
	
    // Set the initial offset at the beginning of the array in the
    // application info block.
    offset = arrayAddress - (UInt32) appInfo;
	
    // Copy each string into the application info block.
    for (i = 0; i < stringCount; i++) {
        if (stringArray[i][0] != '\0')
            DmStrCopy(appInfo, offset, stringArray[i]);
        // Increment the offset to the next array member.
        offset += sizeof(libLabel);
    }
	
    MemPtrFree(stringArray);
    MemPtrUnlock(rsc);
    DmReleaseResource(rscH);
}


/***********************************************************************
 *
 * FUNCTION:    LibGetDatabase
 *
 * DESCRIPTION: Attempts to open Librarian's database; if the database
 *              doesn't exist, LibGetDatabase creates it.
 *
 * PARAMETERS:  dbP  <-  pointer to receive a reference to the newly
 *                       opened or created database
 *              mode  -> mode to open the database in
 *
 * RETURNS:     0 if no error; otherwise, returns error code
 *
 * REVISION HISTORY:
 *
 ***********************************************************************/
Err LibGetDatabase(DmOpenRef *dbP, UInt16 mode)
{
    Err        error = errNone;
    DmOpenRef  db;
    UInt16     cardNo;
    LocalID    dbID;
    UInt16     attributes;


    *dbP = NULL;
	
    db = DmOpenDatabaseByTypeCreator(libDBType, libCreatorID, mode);
    if (! db) {
        error = DmCreateDatabase(0, libDBName, libCreatorID, libDBType,
                                 false);
        if (error) 
            return error;
      
        db = DmOpenDatabaseByTypeCreator(libDBType, libCreatorID, mode);
        if (! db) 
            return (1);
	      
        // Set the backup bit.  This allows for a database backup if the
        // user does not have the Librarian conduit installed.
        DmOpenDatabaseInfo(db, &dbID, NULL, NULL, &cardNo, NULL);
        DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
                       NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        attributes |= dmHdrAttrBackup;
        DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		
        // Initialize the application info block.
        error = LibAppInfoInit(db);
        if (error) {
            DmCloseDatabase(db);
            DmDeleteDatabase(cardNo, dbID);
            return error;
        }
    }

    *dbP = db;
    return errNone;	
}


/***********************************************************************
 *
 * FUNCTION:    LibGetPrimaryField
 *
 * DESCRIPTION: Retrieves the string contained in a packed record's
 *              primary field.  The primary field is determined from
 *              a specified sort order.
 *
 * PARAMETERS:  packed      -> pointer to packed database record
 *              showInList  -> databse sort order
 *              primary    <-  pointer to string to receive contents of
 *                             the primary field, or NULL if the primary
 *                             field is empty
 *
 * RETURNS:     nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void LibGetPrimaryField(LibPackedDBRecord *r, UInt8 showInList,
                               char **primary)
{
    switch (showInList) {
        case libShowAuthorTitle:
            if (r->flags.bits.lastName)
                *primary = (char *) &r->firstField + r->lastNameOffset;
            else if (r->flags.bits.firstName)
                *primary = (char *) &r->firstField + r->firstNameOffset;
            else
                *primary = NULL;
            break;
		
        case libShowTitleAuthor:
        case libShowTitleOnly:
            if (r->flags.bits.title)
                *primary = &r->firstField;
            else
                *primary = NULL;
            break;
			
        default:
            *primary = NULL;
            break;
    }
}


/***********************************************************************
 *
 * FUNCTION:    LibGetRecord
 *
 * DESCRIPTION: Retrieves a record from the Librarian database.
 *
 * PARAMETERS:  db - open database pointer
 *              index - index of record to lock
 *              record - pointer to record structure
 *              recordH - pointer to handle to unlock when done
 *
 * RETURNS:     0 if successful, error code if not
 *
 *  The record's handle is locked so that the pointers to strings within
 *  the record remain pointing to valid chunks, instead of the record
 *  randomly moving.  Unlock the handle when the record is destroyed.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
Err LibGetRecord(DmOpenRef db, UInt16 index, LibDBRecordType *record, 
                 MemHandle *recordH)
{
    LibPackedDBRecord  *packed;
	
	
    *recordH = DmQueryRecord(db, index);
    packed = (LibPackedDBRecord *) MemHandleLock(*recordH);
    if (packed == NULL)
        return dmErrIndexOutOfRange;
   
    UnpackRecord(packed, record);
	
    return 0;
}


/***********************************************************************
 *
 * FUNCTION:    LibGetSortOrder
 *
 * DESCRIPTION: Retrieves the sort order of the Librarian database.
 *
 * PARAMETERS:  db - open database pointer
 *
 * RETURNS:     Sort order
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt8 LibGetSortOrder(DmOpenRef db)
{
    UInt8  showInList;
    LibAppInfoType  *appInfo;
	
	
    appInfo = MemHandleLock(LibGetAppInfo(db));
    showInList = appInfo->showInList;
    MemPtrUnlock(appInfo);
	
    return (showInList);
}


/***********************************************************************
 *
 * FUNCTION:    LibLookupChar
 *
 * DESCRIPTION: Finds the first record in the database that starts with
 *              a specified character, respecting a given sort order and
 *              category.
 *
 * PARAMETERS:  db           -> open database pointer
 *              key          -> character to look up
 *              category     -> category in which to perform the lookup
 *              showInList   -> current database sort order
 *              foundRecord <-  pointer to receive the index of the
 *                              record, if found
 *
 * RETURNS:     true if a record beginning with the supplied character
 *              was found, false otherwise
 *
 * NOTES:       LibLookupChar uses StrNCaselessCompare to compare the key
 *              character with the first character in a record's primary
 *              field instead of comparing the two character values
 *              directly, because StrNCaselessCompare provides case- and
 *              accent-insensitive comparisons.  This allows the user
 *              to enter unaccented lowercase characters to scroll the
 *              List view.  For example, 'e' will scroll to the first
 *              record beginning with 'e', 'E', 'é', or any other
 *              'e'-like character.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
Boolean LibLookupChar(DmOpenRef db, char key, UInt16 category,
                      UInt8 showInList, UInt16 *foundRecord)
{
    UInt16    numRecords;
    int     low, high, middle;
    UInt16    found = noRecord;
    UInt16    firstMatch = noRecord;
    LibPackedDBRecord  *r;
    MemHandle  h = NULL;
    Int16     result;
    char    *middleField;
	
	
    // If there are no records in the database, return false.
    numRecords = DmNumRecords(db);
    if (numRecords == 0)
        return false;
	
    // Perform a binary search to find a record with the same first
    // character as the key character.
    result = 0;
    low = 0;
    high = numRecords;
	
    while (low <= high) {
        middle = (low + high) / 2;
		
        if (h)
            MemHandleUnlock(h);
        h = DmQueryRecord(db, middle);
		
        // A NULL record handle from DmQueryRecord indicates a deleted
        // record.  Since deleted records are stored at the end of the
        // database, the record we're looking for must occur earlier
        // than the middle record.
        if (h == NULL)
            result = -1;
        // Now that we've got a handle to the record in the middle of the
        // database, lock a pointer to the packed record and retrieve the
        // primary field from that record, depending on the current sort
        // order.
        else {
            r = MemHandleLock(h);
            LibGetPrimaryField(r, showInList, &middleField);

            // If the primary field in this record contains data, compare
            // the first character of the primary field with the key
            // character.
            if (middleField) {
                result = StrNCaselessCompare(&key, middleField, 1);
            }
            // Records that contain no data in their primary fields are
            // sorted before those that do contain primary field data.
            else
                result = 1;
        }
			
        // Found a match; break out of the loop.
        if (result == 0) {
            found = middle;
            break;
        }
        // Current middle is after the record we want; search the first
        // half of the records.
        else if (result < 0)
            high = middle - 1;
        // Current middle is before the record we want; search the last
        // half of the records.
        else
            low = middle + 1;
    }
	
    // If no record was found beginning with the key character, exit and
    // return false.
    if (found == noRecord) {
        if (h)
            MemHandleUnlock(h);
        return false;
    }
	
    // At this point, found holds the index of a record that starts with
    // the key character.  Now search backwards through the database for
    // records in the correct category that also start with the key
    // character.
    while (! DmSeekRecordInCategory(db, &found, 0, dmSeekBackward,
                                    category)) {
        // A record exists in the correct category.  Check to see if
        // it begins with the key character.
        middleField = NULL;
        if (h)
            MemHandleUnlock(h);
        h = DmQueryRecord(db, found);
        r = MemHandleLock(h);
        LibGetPrimaryField(r, showInList, &middleField);
		
        // If this record begins with the key character, save this
        // record as the first match and continue searching for more
        // records.  Decrement found to keep the next iteration of the
        // loop from finding the same record that this iteration found.
        // However, if is 0, break out of the loop, since negative values
        // don't work with DmSeekRecordInCategory.
        if (middleField) {
            result = StrNCaselessCompare(&key, middleField, 1);
            if (result == 0) {
                firstMatch = found;
                if (found > 0)
                    found--;
                else
                    break;
            }
            // If this record does not start with the key character, we've
            // passed all the possible records; break out of the loop.
            else
                break;
        }
        // If there is no data in this record's primary field, it cannot
        // possibly match the key character; break out of the loop.
        else
            break;
    }

    // If no records from found to the beginning of the database are part of
    // the current category, search forward to see if there are any matches
    // later in the category.
    if (firstMatch == noRecord) {
        while (! DmSeekRecordInCategory(db, &found, 1, dmSeekForward,
                                        category)) {
            middleField = NULL;
            if (h)
                MemHandleUnlock(h);
            h = DmQueryRecord(db, found);
            r = MemHandleLock(h);
            LibGetPrimaryField(r, showInList, &middleField);
			
            // If this record begins with the key character, save this
            // record as the first match and continue searching for more
            // records.
            if (middleField) {
                result = StrNCaselessCompare(&key, middleField, 1);
                if (result == 0) {
                    firstMatch = found;
                }
				// If this record does not start with the key character,
				// we've passed all the possible records; break out of the
				// loop.
                else
                    break;
            }
            // If there is no data in this record's primary field, it cannot
            // possibly match the key character; break out of the loop.
            else
                break;
        }
    }
	
    // If no records were found at all, return false.
    if (firstMatch == noRecord) {
        if (h)
            MemHandleUnlock(h);
        return false;
    }
	
    // Success!  Set the return value, clean up, and exit.
    *foundRecord = firstMatch;
	
    if (h)
        MemHandleUnlock(h);
		
    return true;
}


/***********************************************************************
 *
 * FUNCTION:    LibNewRecord
 *
 * DESCRIPTION: Create a new packed record in sorted position
 *
 * PARAMETERS:  db      -> open database pointer
 *              record  -> pointer to a record to copy into the database
 *              index  <-  set to the new record's index if a new record
 *                         is created successfully
 *
 * RETURNS:     0 if successful, error code if not
 *
 * REVISION HISTORY:
 *    
 *
 *
 ***********************************************************************/
Err LibNewRecord(DmOpenRef db, LibDBRecordType *record, UInt16 *index)
{
    MemHandle  recordH;
    Err        error;
    LibPackedDBRecord  *packed;
    UInt16     newIndex;
   

    // Allocate a chunk large enough to hold the new packed record.
    recordH = DmNewHandle(db, LibUnpackedSize(record));
    if (recordH == NULL)
        return dmErrMemError;

    // Copy the data from the unpacked record to the packed one.
    packed = MemHandleLock(recordH);
    PackRecord(record, packed);
	
    // Get the index of the new record.
    newIndex = LibFindSortPosition(db, packed);
    MemPtrUnlock(packed);
	
    // Attach new record in place and return the index of the new record
    // in the index parameter.
    error = DmAttachRecord(db, &newIndex, recordH, 0);
    if (error) 
        MemHandleFree(recordH);
    else
        *index = newIndex;
      
    return error;
}


/***********************************************************************
 *
 * FUNCTION:    LibUnpackedSize
 *
 * DESCRIPTION: Returns the size of a LibDBRecordType record.
 *
 * PARAMETERS:  record - pointer to the record whose size should be
 *              determined.
 *
 * RETURNS:     size of the record in bytes
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/03/06  Changed return value to UInt32 to avoid unnecessary
 *             casting elsewhere in librarianDB.c.
 *
 ***********************************************************************/
static UInt32 LibUnpackedSize(LibDBRecordType *record)
{
    UInt32  size;
    Int16   i;
	
    // Initial size is the size of a packed record, minus the char
    // placeholder that provides the position of the first field.
    size = sizeof(LibPackedDBRecord) - sizeof(char);
	
    // Add the length of each field that contains data, plus one byte
    // for each to accomodate a terminating null character.
    for (i = 0; i < libFieldsCount; i++) {
        if (record->fields[i] != NULL)
            size += StrLen(record->fields[i]) + 1;
    }
    return size;
}


/***********************************************************************
 *
 * FUNCTION:    PackRecord
 *
 * DESCRIPTION: Packs a record into its stored database format from
 *              a structure which Librarian may access more easily.
 *
 * PARAMETERS:  record - Pointer to the unpacked record.
 *              recordDBEntry - Pointer to the database record that
 *                will contain the packed record.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void PackRecord (LibDBRecordType *record, MemPtr recordDBEntry)
{
    UInt32  offset;
    Int16   index;
    UInt16  length;
    MemPtr  p;
    LibDBRecordFlags  flags;
    LibPackedDBRecord  *packed = 0;
    Char    lastNameOffset = 0, firstNameOffset = 0,
        yearOffset = 0, noteOffset = 0;
	
    flags.allBits = 0;
	
    // Write book status structure into packed record.
    DmWrite(recordDBEntry, (UInt32)&packed->status, &record->status,
            sizeof(record->status));
    offset = (UInt32)&packed->firstField;
	
    for (index = 0; index < libFieldsCount; index++) {
        if (record->fields[index] != NULL) {
            p = record->fields[index];
            length = StrLen(p) + 1;

            // Write text field data to packed record.
            DmWrite(recordDBEntry, offset, p, length);
            offset += length;
            SetBitMacro(flags.allBits, index);
        }
    }
	
    // Write field flags to packed record.
    DmWrite(recordDBEntry, (UInt32)&packed->flags.allBits,
            &flags.allBits, sizeof(flags.allBits));
	        
    // Set or clear field offsets, as necessary.
    index = 0;
    if (record->fields[libFieldTitle] != NULL)
        index += StrLen(record->fields[libFieldTitle]) + 1;
    if (record->fields[libFieldLastName] != NULL) {
        lastNameOffset = index;
        index += StrLen(record->fields[libFieldLastName]) + 1;
    }
    if (record->fields[libFieldFirstName] != NULL) {
        firstNameOffset = index;
        index += StrLen(record->fields[libFieldFirstName]) + 1;
    }
    if (record->fields[libFieldPublisher] != NULL)
        index += StrLen(record->fields[libFieldPublisher]) + 1;
    if (record->fields[libFieldYear] != NULL) {
        yearOffset = index;
        index += StrLen(record->fields[libFieldYear]) + 1;
    }
    if (record->fields[libFieldPrinting] != NULL)
        index += StrLen(record->fields[libFieldPrinting]) + 1;
    if (record->fields[libFieldIsbn] != NULL)
        index += StrLen(record->fields[libFieldIsbn]) + 1;
    if (record->fields[libFieldPrice] != NULL)
        index += StrLen(record->fields[libFieldPrice]) + 1;
    if (record->fields[libFieldNote] != NULL)
        noteOffset = index;
	
    DmWrite(recordDBEntry, (UInt32)(&packed->lastNameOffset),
            &lastNameOffset, sizeof(lastNameOffset));
    DmWrite(recordDBEntry, (UInt32)(&packed->firstNameOffset),
            &firstNameOffset, sizeof(firstNameOffset));
    DmWrite(recordDBEntry, (UInt32)(&packed->yearOffset),
            &yearOffset, sizeof(yearOffset));
    DmWrite(recordDBEntry, (UInt32)(&packed->noteOffset),
            &noteOffset, sizeof(noteOffset));
}


/***********************************************************************
 *
 * FUNCTION:    RecordContainsData
 *
 * DESCRIPTION: Checks a record to see if it contains any data.
 *
 * PARAMETERS:  record - a pointer to a Librarian record
 *
 * RETURNED:    true if one of the fields has data, false otherwise
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
Boolean RecordContainsData (LibDBRecordType *record)
{
    UInt16  i;
   
   
    // Look for a field that isn't empty.
    for (i = 0; i < libFieldsCount; i++) {
        if (record->fields[i] != NULL)
            return true;
    }
	
    return false;
}


/***********************************************************************
 *
 * FUNCTION:    UnpackRecord
 *
 * DESCRIPTION: Unpacks a record from its stored database format into
 *              a structure which Librarian may access more easily.
 *
 * PARAMETERS:  packed - Pointer to the packed record.
 *              record - Pointer to the unpacked record.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void UnpackRecord (LibPackedDBRecord *packed, 
                          LibDBRecordType *record)
{
    Int16   index;
    UInt16  flags;
    char    *p;
	
	
    record->status = packed->status;
    flags = packed->flags.allBits;
    p = &packed->firstField;
	
    for (index = 0; index < libFieldsCount; index++) {
        if (GetBitMacro(flags, index) != 0) {
            record->fields[index] = p;
            p += StrLen(p) + 1;
        }
        else {
            record->fields[index] = NULL;
        }
    }
}
