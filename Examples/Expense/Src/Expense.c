/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Expense.c
 *
 * Description:
 *	  This is the Expense application's main module.
 *
 * History:
 *		January 2, 1995	Created by Art Lamb
 *		grant	3/25/99	Added Euro to default currency list.
 *
 *****************************************************************************/

#include <PalmOS.h>
#include <CharAttr.h>
#include <FeatureMgr.h>
#include <SysEvtMgr.h>
#include <FntGlue.h>

#include "Expense.h"


extern Int16 LstPopupListWithScrollBar (ListPtr pList);


/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
#define expenseDBName					"ExpenseDB"
#define expenseDBType					'DATA'
#define expensePrefID					1

#define expensePrefsVersion3			3
#define expensePrefsVersion4			4
#define expensePrefsVersionNum		expensePrefsVersion4

#define vendorDBType 					'vend'
#define vendorDBName						"VendorsDB"

#define cityDBType						'city'
#define cityDBName						"CitiesDB"

#define version20							0x02000000

// Fonts used by application
#define expenseDescFont					stdFont
#define noteTitleFont					boldFont

// Column in the expense table on the list view.
#define dateColumn						0
#define typeColumn						1
#define currencyColumn					2
#define amountColumn						3

#define expenseListColumnSpacing		3

#define maxAmountChars					7

#define newExpenseSize  				16

#define maxNoteTitleLen					40


// Field numbers used the id where search string was found
#define amountSeacrchFieldNum			0
#define vendorSeacrchFieldNum			1
#define citySeacrchFieldNum			2
#define attendeesSeacrchFieldNum		3
#define noteSeacrchFieldNum			4

#define searchDateWidth					25

#define maxExpenseNameLength			25


#define noRecordSelected				-1

#define maxAutoFillLen					8

#define noExpenseType					255

// Maximun width of label of attendees selector in the Details Dialog.
#define maxAttendeesWidth				84

#define numCurrenciesDisplayed		5

// Unit of distance
#define distanceInMiles					0
#define distanceInKilometers			1

/***********************************************************************
 *
 *	Global variables
 *
 ***********************************************************************/

		 DmOpenRef			ExpenseDB;									// expense database
static Char					CategoryName [dmCategoryLength];		// name of the current categorg
static Boolean				HideSecretRecords;						// true if serect records are hidden
static Boolean				NoteUpScrollerVisible;					// true if note can be scroll winUp
static Boolean				NoteDownScrollerVisible;				// true if note can be scroll winUp
static UInt16				TopVisibleRecord = 0;					// top visible record in list view
static UInt16				PendingUpdate = 0;						// code of pending list view update
static DateFormatType	DateFormat;
static MemHandle			ExpenseTypeConvertH = 0;
static MemHandle			ExpenseTypeUnconvertH = 0;

// The following global variables are used to keep track of the edit
// state of the application.
		 UInt16				CurrentRecord = -1;						// record being edited
		 Boolean				ItemSelected = false;					// true if a list view item is selected
static Boolean				RecordDirty = false;						// true if a record has been modified
static UInt16				ListEditPosition = 0;					// position of the insertion point in the desc field
static UInt16				ListEditSelectionLength;				// length of the current selection.

// The following global variables are saved to a state file.
		 FontID				NoteFont = stdFont;						// font used in note view
static UInt16				CurrentCategory = dmAllCategories;	// currently displayed category
static Boolean				ShowAllCategories = true;				// true if all categories are being displayed
static Boolean				SaveBackup = true;						// true if save backup to PC is the default
static Boolean				ShowCurrency = true;						// true if currrency symbols are displated in liew view
static Boolean				AllowQuickfill = true;					// true to enable the Quickfill feature
static UInt16				DefaultCurrency = cUnitedStates;		// the default currency
static UInt8				UnitOfDistance;							// true if distances are in miles, false for kilometers

// DOLATER figure out how to correct for first version overriding the
// meaning of check to be JCB charge card in Japan. This seems wrong,
// as in a different type should have been added, versus the meaning of
// the value 3 being different on Japanese devices versus all others.

static ExpensePaymentType* ListSelectionToPaymentTypeTableP;
static UInt16					NumPaymentTypes;

// DOLATER - possibly make this another pointer to a resource???

		 UInt8					Currencies [maxCurrenciesDisplayed];

// The following global variables are used to support the expense
// list auto-fill feature.
static Boolean				AutoFill = false;
static Char					AutoFillBuffer [maxAutoFillLen+1];
static UInt16				AutoFillLen = 0;							// number of character in the auto-fill buffer

static Char					DecimalSeperator = '.';

		 UInt16				HomeCountry;

		 UInt8				CurrentCustomCurrency = 0;


/***********************************************************************
 *
 *	Internal Structutes
 *
 ***********************************************************************/

// This is the structure of the data stated to the state file.
typedef struct {
	UInt16			currentCategory;
	UInt16			defaultCurrency;
	FontID			v20NoteFont;
	Boolean			showAllCategories;
	Boolean			showCurrency;
	Boolean			saveBackup;
	Boolean			allowQuickfill;
	Boolean			unitOfDistance;
	UInt8				currencies [maxCurrenciesDisplayed];
	
	// New preferences for 3.0	(BGT)
	FontID			noteFont;
} ExpensePreferenceType;


/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/
static void ListViewLoadTable (Boolean fillTable);

static Boolean ClearEditState (void);

static void ListViewSaveAmount (TablePtr table);

static void ListViewInitAmount (TablePtr table, FieldPtr fld, Int16 row,
	Int16 column, RectanglePtr bounds, Boolean visible);

static void DrawDate (DateType date, Int16 x, Int16 y, Int16 width,
		DateFormatType dateFormat);

static MemHandle SortExpenseTypes (Boolean createReverseList);

static void ExpenseLoadPrefs(void);	// (BGT)
static void ExpenseSavePrefs(void);	// (BGT)


/***********************************************************************
 *
 * FUNCTION:    SyncNotification
 *
 * DESCRIPTION: This routine is a entry point of the expense application.
 *              It is call when the expense application's database is 
 *              synchronized.  This routine will resort the database and
 *              reset the state file info if necessary.
 *
 * PARAMETERS:	 nothing
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void SyncNotification (void)
{
	Err err = 0;
	UInt16 prefsSize;
	Char name [dmCategoryLength];
	DmOpenRef dbP;
	MemHandle expensesH;
	ExpensePreferenceType prefs;

	// Open the application's database.
	dbP = DmOpenDatabaseByTypeCreator(expenseDBType, sysFileCExpense, dmModeReadWrite);
	if (!dbP) return;
	
	// Get the sorted list of expense types, if we don't have a list 
	// create one.  This list is saved as a feature ans used by the 
	// ExpenseSort rotine.
	err = FtrGet (sysFileCExpense, expFeatureNum, (UInt32 *) &expensesH);
	if (err) 
		{
		expensesH = SortExpenseTypes (false);	// don't create reverse list
		if (expensesH)
			{
			FtrSet (sysFileCExpense, expFeatureNum, (UInt32) expensesH);
			}
		}

	// Resort the database.
	ExpenseSort (dbP);
		
	// Check if the currrent category still exists.
	prefsSize = sizeof (ExpensePreferenceType);
	if (PrefGetAppPreferences (sysFileCExpense, expensePrefID, &prefs, &prefsSize, 
		true) != noPreferenceFound)
		{
		CategoryGetName (dbP, prefs.currentCategory, name);	
		if (*name == 0) 
			{
			prefs.currentCategory = dmAllCategories;
			prefs.showAllCategories = true;
			prefs.showCurrency = true;
	
			PrefSetAppPreferences (sysFileCExpense, expensePrefID, expensePrefsVersionNum, 
				&prefs, sizeof (ExpensePreferenceType), true);
			}
		}

	// Free the sorted list of expense types.
	// (StopApplication is not called after SyncNotification.)
	err = FtrGet (sysFileCExpense, expFeatureNum, (UInt32 *) &expensesH);
	if (! err) 
		{
		MemHandleFree (expensesH);
		FtrUnregister (sysFileCExpense, expFeatureNum);

		}
	
	// Close the database.
	DmCloseDatabase (dbP);

}


/***********************************************************************
 *
 * FUNCTION:    CompareExpenseTypes
 *
 * DESCRIPTION: Case blind comparision of two expense names. This routine is 
 *              used as a callback routine when sorting the expense names.
 *
 * PARAMETERS:  e1 - a expense type
 *              e2 - a expense type
 *
 * RETURNED:    if e1 > e2  - a positive integer
 *              if e1 = e2  - zero
 *              if e1 < e2  - a negative integer
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/3/96	Initial Revision
 *
 ***********************************************************************/
static Int16 CompareExpenseTypes (UInt8 * e1, UInt8 * e2, Int32 other)
{
	Char **expenseStrings = (Char **) other;

	return StrCompare (expenseStrings[*e1], expenseStrings[*e2]);
}


/***********************************************************************
 *
 * FUNCTION:    SortExpenseTypes
 *
 * DESCRIPTION: Create a sorted list of expense types.  The purpose
 *						of this list is to be able to go from the english
 *						sorted expense type (which is stored in the database
 *						even in non-US versions) to the translated type.
 *
 * PARAMETERS:	 createReverseList - saves the reverse (unconvert) list.
 *						This can only be done when globals are available!
 *
 * RETURNED:    MemHandle of a sorted array of expese types.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/3/97	Initial Revision
 *			scl	4/10/97	Fixed
 *
 ***********************************************************************/
static MemHandle SortExpenseTypes (Boolean createReverseList)
{
	Int16 i;
	MemHandle h;
	UInt8 * expenseTypes;
	MemHandle resH;
	Char * srcP;
	Char * expenseStringP[numExpenseTypes];
	UInt8 tempTypes[numExpenseTypes];
	

	// Allocate a MemHandle to hold the sort list of expense types.
	h = MemHandleNew (numExpenseTypes);
	if (h)
		{
		expenseTypes = MemHandleLock (h);
		
		// To avoid having to call SysStringByIndex twice every time
		// CompareExpenseTypes is called, let's build and pass an
		// array of pointers to the expense type strings...
		resH = DmGetResource(sysResTErrStrings, expenseStrListID);
		if (!resH) return (0);
		srcP = (Char *) MemHandleLock(resH);
		srcP += StrLen(srcP)+3;		// skip prefix string & number of entries

		// Initialize the list of expense types and strings.
		for (i = 0; i < numExpenseTypes; i++)
			{
			tempTypes [i] = i;
			expenseStringP [i] = srcP;	// save pointer to i'th string
			srcP += StrLen(srcP)+1;		// bump string pointer
			}

	 	// Sort the list of expense types.
		SysQSort (tempTypes, numExpenseTypes, sizeof (UInt8), 
			(CmpFuncPtr)CompareExpenseTypes, (Int32) &expenseStringP[0]);
			
		// "Fix" and copy the list of expense types.
		for (i = 0; i < numExpenseTypes; i++)
			expenseTypes [ tempTypes[i] ] = i;

		// When the app is running (as opposed to simply HotSyncing),
		// we need a reverse list, too.
		if (createReverseList)
			{
			UInt8 * expenseTypeUnconvertP;
			ExpenseTypeUnconvertH = MemHandleNew (numExpenseTypes);
			if (ExpenseTypeUnconvertH)
				{
				expenseTypeUnconvertP = MemHandleLock (ExpenseTypeUnconvertH);
				for (i = 0; i < numExpenseTypes; i++)
					expenseTypeUnconvertP [i] = tempTypes[i];
				MemHandleUnlock(ExpenseTypeUnconvertH);
				}
			}

		MemHandleUnlock(resH);
		DmReleaseResource(resH);
		MemHandleUnlock (h);
		
		// If we needed but couldn't allocate the reverse list,
		// purge the regular list so the application launch will fail.
		if (createReverseList && !ExpenseTypeUnconvertH)
			{
			MemHandleFree (h);
			h = NULL;
			}
		}	
	return (h);
}


/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version is meet your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *                             
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/15/96	Initial Revision
 *
 ***********************************************************************/
static Err RomVersionCompatible (UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	// See if we're on in minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
		{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
			{
			FrmAlert (RomIncompatibleAlert);
		
			// Pilot 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < 0x02000000)
				{
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
				}
			}
		
		return (sysErrRomIncompatible);
		}

	return (0);
}


/***********************************************************************
 *
 * FUNCTION:    SearchDraw
 *
 * DESCRIPTION: This routine draws the description of a expense item found
 *              by the text search routine 
 *
 * PARAMETERS:	 desc  - pointer to a descrition field
 *              x     - draw position
 *              y     - draw position
 *              width - maximum width to draw.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/9/96	Initial Revision
 *
 ***********************************************************************/
static void SearchDraw (ExpenseRecordPtr recordP, Int16 x, Int16 y, Int16 width)
{
	Char decimalSeperator;
	Char expenseName [maxExpenseNameLength];
	UInt16 len;
	Int16 drawX;
	FontID currFont;
	Char * str;
	MemHandle resH;
	DateFormatType dateFormat;


	currFont = FntSetFont (stdFont);

	// Get the home country's currency decimal seperator.
	decimalSeperator = CurrencyGetDecimalSeperator (PrefGetPreference (prefCountry));

	// Display the date of the expense
	dateFormat = (DateFormatType) PrefGetPreference (prefDateFormat);
	DrawDate (recordP->date, x, y, searchDateWidth, dateFormat);
	

	// Display the expense type
	drawX = x + searchDateWidth + 4;
	
	if (recordP->type != noExpenseType)
		{
		SysStringByIndex (expenseStrListID, recordP->type, expenseName, 
			maxExpenseNameLength-1);
		WinDrawChars (expenseName, StrLen (expenseName), drawX, y);
		}
	else
		{
		resH = DmGetResource (strRsc, noExpenseTypeStr);
		str = MemHandleLock (resH);
		WinDrawChars (str, StrLen (str), drawX, y);
		MemHandleUnlock (resH);
		}


	// Display the expense amount.
	str = StrChr (recordP->amount, periodChr);
	if (str)
		{
		drawX = x + width - 
				FntCharsWidth (recordP->amount, str - recordP->amount) -
				FntCharWidth (decimalSeperator) -
				FntCharsWidth (str+1, StrLen (str+1));
		
		WinDrawChars (recordP->amount, str - recordP->amount, drawX, y);
		drawX += FntCharsWidth (recordP->amount, str - recordP->amount);

		WinDrawChars (&decimalSeperator, 1, drawX, y);
		drawX += FntCharWidth (decimalSeperator);
		
		WinDrawChars (str+1, StrLen (str+1), drawX, y); 
		}
	else
		{
		len = StrLen (recordP->amount);
		drawX = x + width - FntCharsWidth (recordP->amount, len);
		WinDrawChars (recordP->amount, len, drawX, y);
		}
		
	FntSetFont (currFont);
}


/***********************************************************************
 *
 * FUNCTION:    SearchAmount
 *
 * DESCRIPTION: This routine determines if the amount field of a database
 *              record should be searched.  If it should, the amount string
 *              to search for is returned.  Because the amount value is 
 *					 stored with a period as the decimal seperator, and the
 *					 find string may contain a comma decimal seperator, we need 
 *					 to reformat the amount string such that it contains the
 *					 correct seperator.
 *
 * PARAMETERS:	 strToFind - text search parameter block
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	09/10/96	Initial Revision
 *			kwk	12/04/98	Use TxtGetNextChar to load character.
 *
 ***********************************************************************/
static Boolean SearchAmount (Char * strToFind,	Char decimalSeperator)
{
	Int16 i = 0;
	UInt16 len;
	

	len = StrLen (strToFind);
	if ((len == 0) || (len > maxAmountChars))
		return (false);

	while (i < len)
		{
		WChar theChar;
		i += TxtGetNextChar(strToFind, i, &theChar);
		if (! TxtCharIsDigit (theChar) &&
			(theChar != decimalSeperator))
			{
			return (false);
			}
		}

	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    Search
 *
 * DESCRIPTION: This routine searchs the the expense database for records 
 *              contains the string passed. 
 *
 * PARAMETERS:	 findParams - text search parameter block
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *			grant	5/4/99	Skip empty records.  In particular, if expense is
 *								the current app and the current record is empty
 *								then the record will be deleted after a successful find.
 *								This causes problems if the deleted record is the
 *								one selected by the user!
 *			kwk	07/04/99	Rolled in mods for int'l string searching.
 *			jmp	10/21/99	Changed params to findParams to match other routines
 *								like this one.
 *
 ***********************************************************************/
static void Search (FindParamsPtr findParams)
{
	Err err;
	Char amountStr [maxAmountChars + 1];
	Char decimalSeperator;
	UInt16 pos;
	UInt16 fieldNum;
	UInt16 cardNo = 0;
	UInt16 recordNum;
	Char * header;
	Char * MemPtr;
	Boolean done;
	Boolean match;
	Boolean searchAmount;
	Boolean empty;
	MemHandle recordH;
	MemHandle headerH;
	LocalID dbID;
	DmOpenRef dbP;
	RectangleType r;
	ExpenseRecordType record;
	DmSearchStateType	searchState;
	UInt32 longPos;
	UInt16 matchLength;

	// Find the application's data file.
	err = DmGetNextDatabaseByTypeCreator (true, &searchState, expenseDBType,
					sysFileCExpense, true, &cardNo, &dbID);
	if (err)
		{
		findParams->more = false;
		return;
		}

	// Open the expense database.
	dbP = DmOpenDatabase(cardNo, dbID, findParams->dbAccesMode);
	if (!dbP) 
		{
		findParams->more = false;
		return;
		}

	// Display the heading line.
	headerH = DmGetResource (strRsc, findExpenseHeaderStr);
	header = MemHandleLock (headerH);
	done = FindDrawHeader (findParams, header);
	MemHandleUnlock(headerH);
	if (done)
		goto Exit;

	// Determine if we should search the amount field.
	decimalSeperator = CurrencyGetDecimalSeperator (PrefGetPreference (prefCountry));

	// Determine if we should search the amount field.
	searchAmount = SearchAmount (findParams->strAsTyped, decimalSeperator);

	// Search all the text field start from when we last stoped.
	recordNum = findParams->recordNum;
	while (true)
		{
		// Because applications can take a long time to finish a find when
		// the result may be on the screen or for other reasons, users like
		// to be able to stop the find.  Stop the find if an event is pending.
		// This stops if the user does something with the device.  Because
		// this call slows down the search we perform it every so many 
		// records instead of every record.  The response time should still
		// be short without introducing much extra work to the search.
		
		// Note that in the implementation below, if the next 16th record is  
		// secret the check doesn't happen.  Generally this shouldn't be a 
		// problem since if most of the records are secret then the search 
		// won't take long anyways!
		if ((recordNum & 0x000f) == 0 &&			// every 16th record
			EvtSysEventAvail(true))
			{
			// Stop the search process.
			findParams->more = true;
			break;
			}

		recordH = DmQueryNextInCategory (dbP, &recordNum, dmAllCategories);

		// Have we run out of records?
		if (! recordH)
			{
			findParams->more = false;			
			break;
			}

		ExpenseGetRecord (dbP, recordNum, &record, &recordH);

		// If this record is empty, skip it.  It must not be allowed
		// to come up as a match because it will be deleted when the
		// user selects an item in the find results list.
		empty = (record.type == noExpenseType);
		if (empty)
			{
			MemHandleUnlock(recordH);
			recordNum++;
			continue;
			}

		// Search all the text fields of the record (vendor, city, attendees,
		// and note).
		match = false;

		if (searchAmount)
			{
			// Reformat the amount string such that it contains the correct
			// decimal seperator.
			StrCopy (amountStr, record.amount);
			MemPtr = StrChr (amountStr, periodChr);
			if (MemPtr)
				*MemPtr = decimalSeperator;

			fieldNum = amountSeacrchFieldNum;
			match = TxtFindString (amountStr, findParams->strToFind, &longPos, &matchLength);
			pos = longPos;
			}

		if ((! match) && (*record.vendor))
			{
			fieldNum = vendorSeacrchFieldNum;
			match = TxtFindString (record.vendor, findParams->strToFind, &longPos, &matchLength);
			pos = longPos;
			}

		if ((! match) && (*record.city))
			{
			fieldNum = citySeacrchFieldNum;
			match = TxtFindString (record.city, findParams->strToFind, &longPos, &matchLength);
			pos = longPos;
			}

		if ((! match) && (*record.attendees))
			{
			fieldNum = attendeesSeacrchFieldNum;
			match = TxtFindString (record.attendees, findParams->strToFind, &longPos, &matchLength);
			pos = longPos;
			}

		if ((! match) && (*record.note))
			{
			fieldNum = noteSeacrchFieldNum;
			match = TxtFindString (record.note, findParams->strToFind, &longPos, &matchLength);
			pos = longPos;
			}

		if (match)
			{
			// Add the match to the find paramter block,  if there is no room to
			// display the match the following function will return true.
			done = FindSaveMatch (findParams, recordNum, pos, fieldNum, matchLength, cardNo, dbID);
			if (done)
				{
				MemHandleUnlock (recordH);
				break;
				}

			// Get the bounds of the region where we will draw the results.
			FindGetLineBounds (findParams, &r);
			
			// Display the title of the description.
			SearchDraw (&record, r.topLeft.x+1, r.topLeft.y, r.extent.x-2);

			findParams->lineNumber++;
			}

		MemHandleUnlock (recordH);
		recordNum++;
		}

Exit:
	DmCloseDatabase (dbP);	
}


/***********************************************************************
 *
 * FUNCTION:    GoToItem
 *
 * DESCRIPTION: This routine is a entry point of the expense application.
 *              It is generally call as the result of hiting of 
 *              "Go to" button in the text search dialog.
 *
 * PARAMETERS:	 goToParams   - 
 *              launchingApp - true if application is being launched
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	01/02/96	Initial Revision
 *			kwk	12/03/98	Fixed param order in call to MemSet.
 *			jmp	09/17/99	Use NewNoteView instead of NoteView.
 *
 ***********************************************************************/
static void GoToItem (GoToParamsPtr goToParams, Boolean launchingApp)
{
	UInt16 formID;
	UInt16 recordNum;
	UInt16 attr;
	UInt32 uniqueID;
	EventType event;

	recordNum = goToParams->recordNum;


	// If the application is already running, close all the open forms.  If
	// the current record is blank, then it will be deleted, so we'll get 
	// the unique id of the record we're going to and use it to find the 
	// record index after all the forms are closed.
	if (! launchingApp)
		{
		DmRecordInfo (ExpenseDB, recordNum, NULL, &uniqueID, NULL); 
		FrmCloseAllForms ();
		ClearEditState ();
		DmFindRecordByID (ExpenseDB, uniqueID, &recordNum);
		}
		

	CurrentRecord = recordNum;

	// Make the item the first item displayed.
	TopVisibleRecord = recordNum;

	// Change the current category if necessary.
	if (CurrentCategory != dmAllCategories)
		{
		DmRecordInfo (ExpenseDB, recordNum, &attr, NULL, NULL);
		if (CurrentCategory != (attr & dmRecAttrCategoryMask))
			{
			CurrentCategory = dmAllCategories;
			ShowAllCategories = true;
			}
		}


	// Send an event to goto a form and select the matching text.
	MemSet (&event, sizeof(EventType), 0);

	switch (goToParams->matchFieldNum)
		{
		case amountSeacrchFieldNum:
			formID = ListView;
			break;

		case vendorSeacrchFieldNum:
		case citySeacrchFieldNum:
			FrmGotoForm (ListView);
			formID = DetailsDialog;			
			break;
		
		case attendeesSeacrchFieldNum:
			FrmGotoForm (ListView);
			FrmPopupForm (DetailsDialog);
			formID = AttendeesDialog;			
			break;
		
		case noteSeacrchFieldNum:
			formID = NewNoteView;
			break;
		}

	event.eType = frmLoadEvent;
	event.data.frmLoad.formID = formID;
	EvtAddEventToQueue (&event);
 
	event.eType = frmGotoEvent;
	event.data.frmGoto.formID = formID;
	event.data.frmGoto.recordNum = recordNum;
	event.data.frmGoto.matchPos = goToParams->matchPos;
	event.data.frmGoto.matchLen = goToParams->matchCustom;
	event.data.frmGoto.matchFieldNum = goToParams->matchFieldNum;
	EvtAddEventToQueue (&event);
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
void * GetObjectPtr (UInt16 objectID)
{
	FormPtr frm;
	
	frm = FrmGetActiveForm ();
	return (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objectID)));

}


/***********************************************************************
 *
 * FUNCTION:    GetFocusObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to the field object, in 
 *              the current form, that has the focus.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    pointer to a field object or NULL of there is no fucus
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95		Initial Revision
 *
 ***********************************************************************/
static FieldPtr GetFocusObjectPtr (void)
{
	FormPtr frm;
	UInt16 focus;
	FormObjectKind objType;
	
	frm = FrmGetActiveForm ();
	focus = FrmGetFocus (frm);
	if (focus == noFocus)
		return (NULL);
		
	objType = FrmGetObjectType (frm, focus);
	
	if (objType == frmFieldObj)
		return (FrmGetObjectPtr (frm, focus));
	
	else if (objType == frmTableObj)
		return (TblGetCurrentField (FrmGetObjectPtr (frm, focus)));
	
	return (NULL);
}


/***********************************************************************
 *
 * FUNCTION:    DrawDate
 *
 * DESCRIPTION: This routine draws the date passed.
 *
 * PARAMETERS:	 date       -  date to draw
 *              x, y       - row the item is in
 *              width      - width the draw region.
 *              dateFormat - format of the date string.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	09/09/96	Initial Revision
 *
 ***********************************************************************/
static void DrawDate (DateType date, Int16 x, Int16 y, Int16 width,
		DateFormatType dateFormat)
{
	char dateBuffer [dateStringLength];
	Char * dateStr;
	UInt16 monthStrLen;
	Int16 drawX;
	FontID curFont;
	
	
	curFont = FntSetFont (stdFont);
	
	DateToAscii (date.month, date.day, date.year + firstYear, 
					dateFormat, dateBuffer);

	// Remove the year from the date string.
	dateStr = dateBuffer;
	if ((dateFormat == dfYMDWithSlashes) ||
		 (dateFormat == dfYMDWithDots) ||
		 (dateFormat == dfYMDWithDashes))
		dateStr += 3;
	else
		{
		dateStr[StrLen(dateStr) - 3] = 0;
		}

	// Center the date string on the delimitor between the month and day.
	if (date.month < 10)
		monthStrLen = 1;
	else
		monthStrLen = 2;

	drawX = x + ((width - FntCharWidth (dateStr[monthStrLen])) / 2) -
		FntCharsWidth (dateStr, monthStrLen);

	WinDrawChars (dateStr, StrLen (dateStr), drawX, y);
	
	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    TruncateString
 *
 * DESCRIPTION: This routine make a copy of the passed string and 
 *              truncates it to the specified width or the first 
 *              linefeed, which ever is less.  An ellipsis is added
 *              to the string if any character are truncated.
 *
 * PARAMETERS:  str   - string to truncate
 *              width - width to truncate to
 *
 * RETURNED:    truncated string
 *
 * NOTES:		 This rountine allocates a pointer the must be free 
 *              by the caller.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	08/25/96	Initial Revision
 *			kwk	07/04/99	Rolled in int'l fixes from Instant Karma.
 *
 ***********************************************************************/
static Char * TruncateString (Char * str, Int16 width)
{
	Int16 strLen;
	Int16 strWidth;
	Char * MemPtr;
	Char * newStr;
	Boolean stringFit;
	Boolean addEllipsis = false;
	

	// Skip leading linefeeds.
	while (*str == linefeedChr) str++;
	
	// Find the first linefeed character, it position will be the end of the 
	// string if we find one.
	MemPtr = StrChr (str, linefeedChr);
	if (MemPtr)
		strLen = (UInt16) (MemPtr - str);
	else
		strLen = StrLen (str);

	strWidth = width;
	FntCharsInWidth (str, &strWidth, &strLen, &stringFit);

	if ((! stringFit) || MemPtr)
		{
		strWidth = width - FntCharWidth (chrEllipsis);
		FntCharsInWidth (str, &strWidth, &strLen, &stringFit);
		addEllipsis = true;
		}

	// Allocate a pointer, allow for a null-terminator and an
	// ellipsis if necessary.
	newStr = MemPtrNew (strLen + (addEllipsis ? 2 : 1));
	StrNCopy (newStr, str, strLen);

	if (addEllipsis)
		{
		newStr[strLen++] = chrEllipsis;
		}
	
	newStr[strLen] = nullChr;
	
	return (newStr);
}


/***********************************************************************
 *
 * FUNCTION:    ChangeCategory
 *
 * DESCRIPTION: This routine updates the global varibles that keep track
 *              of category information.  
 *
 * PARAMETERS:  category  - new category (index)
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ChangeCategory (UInt16 category)
{
	CurrentCategory = category;
	TopVisibleRecord = 0;
}


/***********************************************************************
 *
 * FUNCTION:    SelectFont
 *
 * DESCRIPTION: This routine handles selection of a font in the List 
 *              View. 
 *
 * PARAMETERS:  currFontID - id of current font
 *
 * RETURNED:    id of new font
 *
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/10/97	Initial Revision
 *
 ***********************************************************************/
static FontID SelectFont (FontID currFontID)
{
	UInt16 formID;
	FontID fontID;
	
	formID = (FrmGetFormId (FrmGetActiveForm ()));

	// Call the OS font selector to get the id of a font.
	fontID = FontSelect (currFontID);

	if (fontID != currFontID)
		FrmUpdateForm (formID, updateFontChanged);

	return (fontID);
}


/***********************************************************************
 *
 * FUNCTION:    SeekRecord
 *
 * DESCRIPTION: Given the index of a expense record, this routine scans 
 *              forewards or backwards for displayable expense records.           
 *
 * PARAMETERS:  indexP    - pointer to the index of a record to start from;
 *                          the index of the record sought is returned in
 *                          this parameter.
 *
 *              offset    - number of records to skip:  
 *              direction - forwards or backwards
 *                             
 *
 * RETURNED:    false is return if a displayable record was not found.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static Boolean SeekRecord (UInt16 * indexP, Int16 offset, Int16 direction)
{
	DmSeekRecordInCategory (ExpenseDB, indexP, offset, direction, CurrentCategory);
	if (DmGetLastErr()) return (false);

	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    DirtyRecord
 *
 * DESCRIPTION: Mark a record dirty (modified).  Record marked dirty 
 *              will be synchronized.
 *
 * PARAMETERS:  index
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void DirtyRecord (UInt16 index)
{
	UInt16		attr;

	DmRecordInfo (ExpenseDB, index, &attr, NULL, NULL);
	attr |= dmRecAttrDirty;
	DmSetRecordInfo (ExpenseDB, index, &attr, NULL);
}


/***********************************************************************
 *
 * FUNCTION:    DeleteRecord
 *
 * DESCRIPTION: This routine deletes the selected expense item.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the delete occurred,  false if it was canceled.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static Boolean DeleteRecord (UInt16 index)
{
	UInt16 ctlIndex;
	UInt16 buttonHit;
	FormPtr alert;
	Boolean saveBackup;

	// Display an alert to comfirm the operation.
	alert = FrmInitForm (DeleteExpenseDialog);

	ctlIndex = FrmGetObjectIndex (alert, DeleteExpenseSaveBackup);
	FrmSetControlValue (alert, ctlIndex, SaveBackup);
	buttonHit = FrmDoDialog (alert);
	saveBackup = FrmGetControlValue (alert, ctlIndex);;

	FrmDeleteForm (alert);

	if (buttonHit == DeleteExpenseCancel)
		return (false);

	SaveBackup = saveBackup;

	// Delete or archive the record.
	if (SaveBackup)
		DmArchiveRecord (ExpenseDB, index);
	else
		DmDeleteRecord (ExpenseDB, index);
	DmMoveRecord (ExpenseDB, index, DmNumRecords (ExpenseDB));

	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    ClearEditState
 *
 * DESCRIPTION: This routine take the application out of edit mode.
 *              The edit state of the current record is remembered until
 *              this routine is called.  
 *
 *              If the current record is empty, it will be deleted 
 *              by this routine.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true is current record is deleted by this routine.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static Boolean ClearEditState (void)
{
	UInt16 recordNum;
	Boolean empty;
	MemHandle recordH;
	ExpensePackedRecordPtr recordP;

	if (! ItemSelected) return (false);
	
	recordNum = CurrentRecord;

	// Clear the global variables that keep track of the edit start of the
	// current record.
	ItemSelected = false;
	CurrentRecord = noRecordSelected;
	ListEditPosition = 0;
	ListEditSelectionLength = 0;
	PendingUpdate = 0;
	
	// If the description field is empty and the note field is empty, delete
	// the expense record.
	recordH = DmQueryRecord (ExpenseDB, recordNum);
	recordP = MemHandleLock (recordH);
	empty = (recordP->type == noExpenseType);
	MemHandleUnlock (recordH);

	if (empty)
		{
		// If the field was not modified, and the description and 
		// note fields are empty, remove the record from the database.
		// This can occur when a new empty record is deleted.
		if (RecordDirty)
			{
			DmDeleteRecord (ExpenseDB, recordNum);
			DmMoveRecord (ExpenseDB, recordNum, DmNumRecords (ExpenseDB));
			}
		else
			DmRemoveRecord (ExpenseDB, recordNum);
		
		return (true);
		}

	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    PurgeCategory
 *
 * DESCRIPTION: This routine delected all the records in the selected 
 *              category and removes the category name.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/27/96	Initial Revision
 *
 ***********************************************************************/
static void PurgeCategory (void)
{
	Err err;
	UInt16 index;
	UInt16 category;
	UInt16 numRecord;
	UInt16 numItems;
	UInt16 selectedItem;
	UInt16 alertResponse;
	ListPtr list;
	Char * name;
	Char ** itemsP;


	list = GetObjectPtr (PurgeList);
	selectedItem = LstGetSelection (list);

	// Is there a category selected?
	if (selectedItem == noListSelection)
		{
		FrmAlert (PurgeSelectAlert);
		return;
		}

	// Get the catagory id of the selected list item.
	name = LstGetSelectionText (list, selectedItem);
	category = CategoryFind (ExpenseDB, name);

	
	// If the catagory contains any records, then comfirm their deletion.
	index = 0;
	DmSeekRecordInCategory (ExpenseDB, &index, 0, dmSeekForward, category);
	if (! DmGetLastErr())
		{
		alertResponse = FrmAlert (PurgeComfirmAlert);
		if (alertResponse == PurgeComfirmNo)
			return;
		
		numRecord = DmNumRecords (ExpenseDB);
		do
			{
			DmDeleteRecord (ExpenseDB, index);
					
			// Move deleted record to the end of the index so that the 
			// sorting routine will work.
			DmMoveRecord (ExpenseDB, index, numRecord);

			err = DmSeekRecordInCategory (ExpenseDB, &index, 0, dmSeekForward, category);
			}
		while (! err);
		}


	// Remove the category name.
	if (category != dmUnfiledCategory)
		CategorySetName (ExpenseDB, category, NULL);
	

	// Remove the selected item move the list.
	LstSetSelection (list, noListSelection);
	numItems = LstGetNumberOfItems (list);
	
	if (selectedItem < numItems)
		{ 
		itemsP = list->itemsText;
		MemMove (&itemsP[selectedItem], &itemsP[selectedItem+1], 
			(numItems - selectedItem  - 1) * sizeof (Char *));
		}

	LstSetListChoices (list, itemsP, numItems-1);
	LstDrawList (list);

	if (category == CurrentCategory)
		ChangeCategory (dmAllCategories);
}	


/***********************************************************************
 *
 * FUNCTION:    PurgeInit
 *
 * DESCRIPTION: This routine initializes the Prefer Dialog.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/27/96	Initial Revision
 *			gap	   08/13/99	Update to use new constant categoryHideEditCategory 
 *
 ***********************************************************************/
static void PurgeInit (void)
{	
	ListPtr list;

	list = GetObjectPtr (PurgeList);
	CategoryCreateList (ExpenseDB, list, CurrentCategory, false, true, 1, categoryHideEditCategory, false);
	LstSetSelection (list, noListSelection);
//	LstSetHeight (listP, 9);
}


/***********************************************************************
 *
 * FUNCTION:    PurgeHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Purge
 *              Dialog Box".
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/27/96	Initial Revision
 *			grant	6/22/99	Free the category list when the form closes.
 *			CS		06/22/99	Standardized keyDownEvent handling
 *								(TxtCharIsHardKey, commandKeyMask, etc.)
 *
 ***********************************************************************/
static Boolean PurgeHandleEvent (EventType * event)
{
	FormPtr frm;
	ListPtr list;
	Boolean handled = false;
	
	if (event->eType == keyDownEvent)
		{
		if	(	(!TxtCharIsHardKey(	event->data.keyDown.modifiers,
											event->data.keyDown.chr))
			&&	(EvtKeydownIsVirtual(event)))
			{
			list = GetObjectPtr (PurgeList);
			
			if (event->data.keyDown.chr == vchrPageUp)
				LstScrollList (list, winUp, LstGetVisibleItems (list) - 1);
			else if (event->data.keyDown.chr == vchrPageDown)
				LstScrollList (list, winDown, LstGetVisibleItems (list) - 1);
			}
		}


	else if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case PurgeDoneButton:
				// We must close the List View and then goto it.  Otherwise 
				// the ListView gets drawn when closing just the PurgeDialog,
				// but often the data displayed in ListView has been purged.
				FrmCloseAllForms();
				FrmGotoForm (ListView);
				handled = true;
				break;

			case PurgePurgeButton:
				PurgeCategory ();
				handled = true;
				break;
			}
		}


	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		PurgeInit ();
		FrmDrawForm (frm);
		handled = true;
		}

	else if (event->eType == frmCloseEvent)
		{
		list = GetObjectPtr(PurgeList);
		CategoryFreeList(ExpenseDB, list, false, 0);
		}

	return (handled);

}


/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  This routine opens the application's database, laods the 
 *               saved-state information and initializes global variables.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     0 if no error, otherwise result code.
 *
 *	HISTORY:
 *		01/02/96	art	Created by Art Lamb
 *		07/??/99	SPK	Initialize country-conditional data from resources.
 *		08/10/99	kwk	Cleanup of resource init code.
 *
 ***********************************************************************/
static Err StartApplication (void)
{
	Err result = errNone;
	UInt16 mode;
	UInt16 cardNo;
	LocalID dbID;
	MemHandle h;
	UInt16* rCurrencies;
	UInt16* rlstSelectionToPaymentTypeTable;
	int i;
	
	// Determime if secert record should be shown.
	HideSecretRecords = PrefGetPreference (prefHidePrivateRecordsV33);

	if (HideSecretRecords)
		mode = dmModeReadWrite;
	else
		mode = dmModeReadWrite | dmModeShowSecret;

	// Get the date format from the system preferences.
	DateFormat = (DateFormatType) PrefGetPreference (prefDateFormat);

	// Get the home country's currency decimal seperator.
	HomeCountry = PrefGetPreference (prefCountry);
	DecimalSeperator = CurrencyGetDecimalSeperator (HomeCountry);

	// Find the application's data file.  If it don't exist create it.
	ExpenseDB = DmOpenDatabaseByTypeCreator (expenseDBType, sysFileCExpense, mode);
	if (! ExpenseDB)
		{
		result = DmCreateDatabase (0, expenseDBName, sysFileCExpense, expenseDBType, false);
		if (result) return result;
		
		ExpenseDB = DmOpenDatabaseByTypeCreator (expenseDBType, sysFileCExpense, mode);
		if (! ExpenseDB) return (1);
		
		// Set the backup bit.  This is to aid syncs with non Palm software.
		SetDBAttrBits(ExpenseDB, dmHdrAttrBackup);
		
		result = ExpenseAppInfoInit (ExpenseDB);
      if (result) 
      	{
			DmOpenDatabaseInfo(ExpenseDB, &dbID, NULL, NULL, &cardNo, NULL);
      	DmCloseDatabase(ExpenseDB);
      	DmDeleteDatabase(cardNo, dbID);
         return result;
         }
		}

	// Set up the Currencies global using data from a resource.
	// (This must be done before calling ExpenseLoadPrefs since it'll override with the user's choices)
	h = DmGetResource(wrdListRscType, idCurrencies);
	ErrNonFatalDisplayIf(h == NULL, "No currency list resource");
	rCurrencies = (UInt16*)MemHandleLock(h);
	ErrNonFatalDisplayIf(rCurrencies[0] != maxCurrenciesDisplayed, "Currency list resource has wrong size");
	
	for (i = 1; i <= maxCurrenciesDisplayed; i++)
		Currencies[i - 1] = rCurrencies[i];
	
	MemPtrUnlock((MemPtr)rCurrencies);
		

	// Read the preferences / saved-state information.
	ExpenseLoadPrefs();

	TopVisibleRecord = 0;
	CurrentRecord = noRecordSelected;


	// Initialize the default lookup databases used by the details 
	// dailog.
	LookupInitDB (vendorDBType, sysFileCExpense, vendorDBName, vendorStr);
	LookupInitDB (cityDBType, sysFileCExpense, cityDBName, cityStr);
	
	// Get the sorted list of expense types, if we don't have a list 
	// create one.  This list is saved as a feature and used by the 
	// ExpenseSort rotine.
	result = FtrGet (sysFileCExpense, expFeatureNum, (UInt32 *) &ExpenseTypeConvertH);
	if (result)
		{
		ExpenseTypeConvertH = SortExpenseTypes (true);	// also create reverse list
		if (! ExpenseTypeConvertH) return (1);
		FtrSet (sysFileCExpense, expFeatureNum, (UInt32) ExpenseTypeConvertH);
		FtrSet (sysFileCExpense, expRevFeatureNum, (UInt32) ExpenseTypeUnconvertH);
		}

	// Set up the table used to map from an entry in the payment list
	// to an ExpensePaymentType.
	h = DmGetResource(wrdListRscType, idPaymentTypes);
	ErrNonFatalDisplayIf(h == NULL, "No list payment mapping table resource");
	rlstSelectionToPaymentTypeTable = (UInt16*)MemHandleLock(h);
	NumPaymentTypes = rlstSelectionToPaymentTypeTable[0];
	
	ListSelectionToPaymentTypeTableP = MemPtrNew(NumPaymentTypes * sizeof(ExpensePaymentType));
	ErrFatalDisplayIf(ListSelectionToPaymentTypeTableP == NULL, "Out of memory");
		
	for (i=1; i <= NumPaymentTypes; i++)
		{
		ListSelectionToPaymentTypeTableP[i - 1] =
			(ExpensePaymentType)rlstSelectionToPaymentTypeTable[i];
		}
		
	MemPtrUnlock((void*)rlstSelectionToPaymentTypeTable);

	return (errNone);
}


/***********************************************************************
 *
 * FUNCTION:    StopApplication
 *
 * DESCRIPTION: This routine close the application's database
 *              and save the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void StopApplication (void)
{
	Err err;

	MemHandle expensesH;
	
	if (ListSelectionToPaymentTypeTableP != NULL)
		MemPtrFree((MemPtr)ListSelectionToPaymentTypeTableP);

	// Write the preferences / saved-state information.
	ExpenseSavePrefs();

	// Send a frmSave event to all the open forms.
	FrmSaveAllForms ();
	
	// Close all the open forms.
	FrmCloseAllForms ();

	// Close the application's data file.
	DmCloseDatabase (ExpenseDB);

	// Free the sorted list of expense types.
	err = FtrGet (sysFileCExpense, expFeatureNum, (UInt32 *) &expensesH);
	if (! err) 
		{
		MemHandleFree (expensesH);
		FtrUnregister (sysFileCExpense, expFeatureNum);

		// Free the reverse list of expense types, if it was created.
		// (It is only created during normal launch when Globals are available.)
		err = FtrGet (sysFileCExpense, expRevFeatureNum, (UInt32 *) &expensesH);
		if (! err) 
			{
			MemHandleFree (expensesH);
			FtrUnregister (sysFileCExpense, expRevFeatureNum);
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    PreferSelectCurrency
 *
 * DESCRIPTION: This routine handles selection of the currency.
 *
 * PARAMETERS:  currency - the current currency, returns to new
 *                         currency
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/30/96	Initial Revision
 *
 ***********************************************************************/
static void PreferSelectCurrency (UInt16 * currency)
{
	Char * name;
	ListPtr lst;
	ControlPtr ctl;
	
	ctl = GetObjectPtr (PreferCurrencyTrigger);
	lst = GetObjectPtr (PreferCurrencyList);

	// we're going to modify memory that the control is using,
	// OK only because we're calling CtlSetLablel immediately.
	name = (Char *)CtlGetLabel (ctl);
	CurrencySelect (lst, currency, name);

	CtlSetLabel (ctl, name);
}


/***********************************************************************
 *
 * FUNCTION:    PreferApply
 *
 * DESCRIPTION: This routine applies the changes made in the Preferences
 *					 Dialog.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/96	Initial Revision
 *
 ***********************************************************************/
static void PreferApply (void)
{
//	DecimalSeperator = CurrencyGetDecimalSeperator (DefaultCurrency);
	AllowQuickfill = CtlGetValue (GetObjectPtr (PreferQuickFillCheckbox));
}


/***********************************************************************
 *
 * FUNCTION:    PreferInit
 *
 * DESCRIPTION: This routine initializes the Prefer Dialog.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/96	Initial Revision
 *
 ***********************************************************************/
static void PreferInit (void)
{
	Char * label;
	ListPtr lst;
	ControlPtr ctl;
	
	// Set the trigger and popup list that indicates default currency.
	lst = GetObjectPtr (PreferCurrencyList);
	ctl = GetObjectPtr (PreferCurrencyTrigger);
	LstSetSelection (lst, DefaultCurrency);

	// we're going to modify memory that the control is using,
	// OK only because we're calling CtlSetLabel immediately.
	label = (Char *)CtlGetLabel (ctl);
	CurrencyGetSymbol (DefaultCurrency, label);
	CtlSetLabel (ctl, label);

	// Initialize the check box that enables / disables the Quickfill 
	// feature.
	ctl = GetObjectPtr (PreferQuickFillCheckbox);
	CtlSetValue (ctl, AllowQuickfill);
}


/***********************************************************************
 *
 * FUNCTION:    PreferHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Preferences
 *              Dialog Box".
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/96	Initial Revision
 *
 ***********************************************************************/
static Boolean PreferHandleEvent (EventType * event)
{
	static UInt16 currency;
	
	FormPtr frm;
	Boolean handled = false;

	if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case PreferOkButton:
				DefaultCurrency = currency;

				PreferApply ();
				FrmReturnToForm (0);
				FrmUpdateForm (0, updateDisplayOptsChanged);
				handled = true;
				break;

			case PreferCancelButton:
				FrmReturnToForm (0);
				handled = true;
				break;
				
			case PreferCurrencyTrigger:
				PreferSelectCurrency (&currency);
				handled = true;
				break;
			}
		}

	else if (event->eType == frmOpenEvent)
		{
		currency = DefaultCurrency;

		frm = FrmGetActiveForm ();
		PreferInit ();
		FrmDrawForm (frm);
		handled = true;
		}

	return (handled);
}


#pragma mark -
/***********************************************************************
 *
 * FUNCTION:    OptionsApply
 *
 * DESCRIPTION: This routine applies the changes made in the Options Dialog
 *              (aka Preferences).
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    update code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/21/96	Initial Revision
 *
 ***********************************************************************/
static UInt16 OptionsApply (void)
{
	UInt8 distance;
	UInt16 sortOrder;
	UInt16 updateCode = 0;
	ControlPtr ctl;
	
	// Update the sort order.  Reset the expense list to the top.
	sortOrder = LstGetSelection (GetObjectPtr (OptionsSortByList));
	if (ExpenseGetSortOrder (ExpenseDB) != sortOrder)
		{
		ExpenseChangeSortOrder (ExpenseDB, sortOrder);
		TopVisibleRecord = 0;
		updateCode = updateDisplayOptsChanged;
		}


	// Update the unit of distance (miles or kilometers).
	distance = LstGetSelection (GetObjectPtr (OptionsDistanceList));
	if (UnitOfDistance != distance)
		{
		UnitOfDistance = distance;
		updateCode = updateDisplayOptsChanged;
		}


	// Show or hide items marked complete.  Reset the list to the top.
	ctl = GetObjectPtr (OptionsShowCurrency);
	if (ShowCurrency != CtlGetValue (ctl))
		{
		ShowCurrency = CtlGetValue (ctl);
		updateCode = updateDisplayOptsChanged;
		}
		
	return (updateCode);
}


/***********************************************************************
 *
 * FUNCTION:    OptionsInit
 *
 * DESCRIPTION: This routine initializes the Options Dialog.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/21/96	Initial Revision
 *
 ***********************************************************************/
static void OptionsInit (void)
{
	UInt16 sortOrder;
	Char * label;
	ListPtr lst;
	ControlPtr ctl;
	
	// Set the trigger and popup list that indicates the sort order.
	sortOrder = ExpenseGetSortOrder (ExpenseDB);

	lst = GetObjectPtr (OptionsSortByList);
	ctl = GetObjectPtr (OptionsSortByTrigger);
	LstSetSelection (lst, sortOrder);
	label = LstGetSelectionText (lst, sortOrder);
	CtlSetLabel (ctl, label);


	// Set the trigger and popup list that indicates unit on distance
	// (miles or kilometers).
	lst = GetObjectPtr (OptionsDistanceList);
	ctl = GetObjectPtr (OptionsDistanceTrigger);
	LstSetSelection (lst, UnitOfDistance);
	label = LstGetSelectionText (lst, UnitOfDistance);
	CtlSetLabel (ctl, label);


	// Initialize the show currency check box
	ctl = GetObjectPtr (OptionsShowCurrency);
	CtlSetValue (ctl, ShowCurrency);
}


/***********************************************************************
 *
 * FUNCTION:    OptionsHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Options
 *              Dialog Box" of the ToDo application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/21/96	Initial Revision
 *
 ***********************************************************************/
static Boolean OptionsHandleEvent (EventType * event)
{
	UInt16 updateCode;
	FormPtr frm;
	Boolean handled = false;

	if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case OptionsOkButton:
				updateCode = OptionsApply ();
				FrmReturnToForm (ListView);
				if (updateCode)
					FrmUpdateForm (ListView, updateCode);
				handled = true;
				break;

			case OptionsCancelButton:
				FrmReturnToForm (ListView);
				handled = true;
				break;
				
			}
		}

	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		OptionsInit ();
		FrmDrawForm (frm);
		handled = true;
		}

	return (handled);
}


#pragma mark -
/***********************************************************************
 *
 * FUNCTION:    DetailsPaymentTypeToLstSelection
 *
 * DESCRIPTION: Convert the real payment type value to selection of
 *              payment type list in detail dialog.
 *
 * PARAMETERS:  paymentType - payment type value
 *
 * RETURNED:    selection of list object
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			konno	08/14/98	Initial Revision
 *			kwk	07/04/99	Modified to use only one global variable.
 *
 ***********************************************************************/
static UInt8
DetailsPaymentTypeToLstSelection (ExpensePaymentType paymentType)
{
	UInt8 i;
	UInt8 unfiledItem = 0xff;
	
	for (i = 0; i < NumPaymentTypes; i++)
		{
		if (ListSelectionToPaymentTypeTableP[i] == paymentType)
			{
			return(i);
			}
		else if (ListSelectionToPaymentTypeTableP[i] == payUnfiled)
			{
			unfiledItem = i;
			}
		}
	
	// If we didn't find the requested payment type, then return the index
	// in the list of the unfiled payment type.
	ErrNonFatalDisplayIf(unfiledItem == 0xff, "No unfiled payment type in list");
	return(unfiledItem);
}

/***********************************************************************
 *
 * FUNCTION:    DetailsLstSelectionToPaymentType
 *
 * DESCRIPTION: Convert selection of payment type list in detail dialog
 *              to real payment type value.
 *
 * PARAMETERS:  lstSelection - selection value
 *
 * RETURNED:    payment type
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			konno	1998-08-14	Initial Revision
 *
 ***********************************************************************/
static ExpensePaymentType
DetailsLstSelectionToPaymentType (UInt8 lstSelection)
{
	return (lstSelection < NumPaymentTypes ? ListSelectionToPaymentTypeTableP[lstSelection] : payUnfiled);
}


/***********************************************************************
 *
 * FUNCTION:	DetailsAutoFill
 *
 *	DESCRIPTION: This routine handles auto-filling the vendor or city 
 *		fields.
 *
 *	PARAMETERS:	event  - pointer to a keyDownEvent.
 *
 *	RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 *	HISTORY:
 *		01/15/96	art	Created by Art Lamb.
 *		07/20/97	trm	Fixed Autofill bug
 *		11/20/98	kwk	Check for command key, return false if set.
 *		06/22/99	CS		Standardized keyDownEvent handling
 *							(TxtCharIsHardKey, commandKeyMask, etc.)
 *		11/16/99	kwk	Honor the AutoQuickFill flag if language = Japanese.
 *
 ***********************************************************************/
static Boolean DetailsAutoFill (EventType * event)
{
	UInt16 pos;
	UInt16 len;
	UInt16 index;
	UInt16 focus;
	MemHandle h;
	Char * key;
	Char * ptr;
	UInt32 dbType;
	FormPtr frm;
	FieldPtr fld;
	DmOpenRef dbP;
	LookupRecordPtr r;
	UInt32 language;
	
	if	(	TxtCharIsHardKey(	event->data.keyDown.modifiers,
									event->data.keyDown.chr)
		||	(EvtKeydownIsVirtual(event))
		|| (!TxtCharIsPrint(event->data.keyDown.chr)))
		return (false);

	frm = FrmGetActiveForm ();
	focus = FrmGetFocus (frm);

	// Depending on which field has the focus, open the vendor or city
	// data base. 
	if (focus == FrmGetObjectIndex (frm, DetailsVendorField))
		dbType = vendorDBType;
		
	else if (focus == FrmGetObjectIndex (frm, DetailsCityField))
		dbType = cityDBType;
		
	else
		return (false);
		
	// Let the UI insert the character into the field.
	FrmHandleEvent (frm, event);

	// DOLATER kwk - decide if we should always check the AllowQuickfill flag
	// here, or just do it for Japanese.
	if ((!AllowQuickfill)
	&& (FtrGet(sysFtrCreator, sysFtrNumLanguage, &language) == errNone)
	&& (language == lJapanese))
		return(true);
		
	dbP = DmOpenDatabaseByTypeCreator (dbType, sysFileCExpense, dmModeReadOnly);
	if (! dbP) return (true);

	// The current value of the field with the focus.
	fld = FrmGetObjectPtr (frm, focus);
	key = FldGetTextPtr(fld);

	// Check for a match.
	if (LookupStringInDatabase (dbP, key, &index))
		{
		pos = FldGetInsPtPosition (fld);

		h = DmQueryRecord (dbP, index);
		r = MemHandleLock (h);

		// Auto-fill.
		ptr = &r->text + StrLen (key);
		len = StrLen(ptr);

		FldInsert (fld, ptr, StrLen(ptr));
		
		// Highlight the inserted text.
		FldSetSelection (fld, pos, pos + len);

		MemHandleUnlock (h);
		}
		
	DmCloseDatabase (dbP);

	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsSetAttendeesTrigger
 *
 * DESCRIPTION: This routine sets the label of the trigger in a details
 *              dialog that displays the attendees.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/25/96	Initial Revision
 *
 ***********************************************************************/
static void DetailsSetAttendeesTrigger (void)
{
	Char * label;
	Char * rscP;
	ControlPtr ctl;
	MemHandle recordH;
	ExpenseRecordType recordP;

	ctl = GetObjectPtr (DetailsAttendeesSelector);

	// we're going to modify memory that the control is using,
	// OK only because we're calling CtlSetLabel immediately.
	label = (Char *)CtlGetLabel (ctl);	
	if (label) MemPtrFree (label);

	ExpenseGetRecord (ExpenseDB, CurrentRecord, &recordP, &recordH);
	
	if (recordP.attendees == NULL || *recordP.attendees == 0)
		{
		rscP = MemHandleLock (DmGetResource (strRsc, whoStrID));
		label = MemPtrNew (StrLen(rscP) + 1);
		StrCopy (label, rscP);
		MemPtrUnlock (rscP);
		}

	else
		{
		label = TruncateString (recordP.attendees, maxAttendeesWidth);
		}

	MemHandleUnlock (recordH);	
	
	CtlSetLabel (ctl, label);
}


/***********************************************************************
 *
 * FUNCTION:    DetemineDueDate
 *
 * DESCRIPTION: This routine is called when an item of the due date 
 *              popup list is selected.  For items such as "today" and 
 *              "end of week" the due date is computed,  for "select
 *              date" the date picker is displayed.
 *
 * PARAMETERS:  item selected in due date popup list.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void DetermineDueDate (DateType * dateP)
{
	Int16 month, day, year;
	UInt32 timeInSeconds;
	Char * titleP;
	MemHandle titleH;
	DateTimeType date;
	
	// "Select date" item selected?
	if ( *((Int16 *) dateP) == -1)
		{
		timeInSeconds = TimGetSeconds ();
		TimSecondsToDateTime (timeInSeconds, &date);
		year = date.year;
		month = date.month;
		day = date.day;
		}
	else
		{
		year = dateP->year + firstYear;
		month = dateP->month;
		day = dateP->day;
		}

	titleH = DmGetResource (strRsc, dueDateTitleStr);
	titleP = (Char *) MemHandleLock (titleH);

	if (SelectDay (selectDayByDay, &month, &day, &year, titleP))
		{
		dateP->day = day;
		dateP->month = month;
		dateP->year = year - firstYear;
		}

	MemHandleUnlock (titleH);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsExpenseTypeSelected
 *
 * DESCRIPTION: This routine handles selection of the expense type.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/20/96	Initial Revision
 *
 ***********************************************************************/
static void DetailsExpenseTypeSelected (EventType * event)
{
	FormPtr frm;

	frm = FrmGetActiveForm ();

	if (event->data.popSelect.selection != event->data.popSelect.priorSelection)
		{
		// Need to convert selection/priorSelection from popup list order to expense type.
		// Note: selection will never be noListSelection, but priorSelection might!
		UInt8 selection = event->data.popSelect.selection;
		UInt8 priorSelection = event->data.popSelect.priorSelection;
		if (ExpenseTypeUnconvertH != NULL)
			{
			UInt8* expenseTypeUnconvertP = MemHandleLock (ExpenseTypeUnconvertH);
			selection = expenseTypeUnconvertP[selection];
			if (priorSelection != (UInt8)noListSelection)
				priorSelection = expenseTypeUnconvertP[priorSelection];
			MemHandleUnlock (ExpenseTypeUnconvertH);
			}
		
		if (selection == expMileage)
			{
			FrmHideObject (frm, FrmGetObjectIndex (frm, DetailsPaymentTypeTrigger));
			FrmHideObject (frm, FrmGetObjectIndex (frm, DetailsCurrencyTrigger));
			}
		else if (priorSelection == expMileage)
			{
			FrmShowObject (frm, FrmGetObjectIndex (frm, DetailsPaymentTypeTrigger));
			FrmShowObject (frm, FrmGetObjectIndex (frm, DetailsCurrencyTrigger));
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    DetailsSelectCategory
 *
 * DESCRIPTION: This routine handles selection, creation and deletion of
 *              categories form the Details Dialog.  
 *
 * PARAMETERS:  category - the current catagory, returns to new
 *                         category
 *
 * RETURNED:    true if the category was changed in a way that 
 *              require the list view to be redrawn.
 *
 *              The following global variables are modified:
 *							CategoryName
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	01/02/96	  Initial Revision
 *			gap	08/13/99   Update to use new constant categoryDefaultEditCategoryString.
 *
 ***********************************************************************/
static Boolean DetailsSelectCategory (UInt16 * category)
{
	const Char * name;
	Boolean categoryEdited;
	
	name = CtlGetLabel (GetObjectPtr (DetailsCategoryTrigger));

	categoryEdited = CategorySelect (ExpenseDB, FrmGetActiveForm (),
		DetailsCategoryTrigger, DetailsCategoryList,
		false, category, (Char *)name, 1, categoryDefaultEditCategoryString);
	
	return (categoryEdited);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsSelectCurrency
 *
 * DESCRIPTION: This routine handles selection of the currency.
 *
 * PARAMETERS:  currency - the current currency, returns to new
 *                         currency
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/28/96	Initial Revision
 *
 ***********************************************************************/
static void DetailsSelectCurrency (UInt16 * currency)
{
	Char * name;
	ListPtr lst;
	ControlPtr ctl;


	ctl = GetObjectPtr (DetailsCurrencyTrigger);
	lst = GetObjectPtr (DetailsCurrencyList);

	// we're going to modify memory that the control is using,
	// OK only because we're calling CtlSetLable immediately.
	name = (Char *)CtlGetLabel (ctl);
	CurrencySelect (lst, currency, name);

	CtlSetLabel (ctl, name);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsDeleteExpense
 *
 * DESCRIPTION: This routine deletes a expense item. This routine is called 
 *              when the delete button in the details dialog is pressed.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the record was delete or archived.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static Boolean DetailsDeleteExpense (void)
{
	UInt16 recordNum;
	Boolean empty;
	MemHandle recordH;
	ExpensePackedRecordPtr recordP;
		
	recordNum = CurrentRecord;

	// Check if the record is empty, if it is clear the edit state, 
	// this will delete the current record when its blank.
	// If the description field is empty and the note field is empty, delete
	// the expense record.
	recordH = DmQueryRecord (ExpenseDB, recordNum);
	recordP = (ExpensePackedRecordPtr) MemHandleLock (recordH);
	empty = (recordP->type == noExpenseType);
	MemHandleUnlock (recordH);

	if (empty)
		{
		ClearEditState ();
		return (true);
		}
		
	// Display an alert to comfirm the delete operation, and delete the
	// record if the alert is confirmed.
	if (!  DeleteRecord (recordNum) )
		return (false);
	
	ItemSelected = false;
	
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsGoToItem
 *
 * DESCRIPTION: This routine sets winUp the Details Dialog to display the 
 *              results of a text search (Find).
 *
 * PARAMETERS:  event - frmGotoEvent 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/10/96	Initial Revision
 *
 ***********************************************************************/
static void DetailsGoToItem (EventType * event)
{
	UInt16 fieldID;
	FormPtr frm;
	FieldPtr fld;

	if (event->data.frmGoto.matchFieldNum == vendorSeacrchFieldNum)
		fieldID = DetailsVendorField;
	else
		fieldID = DetailsCityField;

	
	fld = GetObjectPtr (fieldID);

	FldSetInsPtPosition (fld, event->data.frmGoto.matchPos);

	FldSetSelection (fld, event->data.frmGoto.matchPos, 
		event->data.frmGoto.matchPos + event->data.frmGoto.matchLen);

	frm = FrmGetActiveForm ();
	FrmSetFocus (frm, FrmGetObjectIndex (frm, fieldID));
}


/***********************************************************************
 *
 * FUNCTION:    DetailsClose
 *
 * DESCRIPTION: This routine free the pointer allocated for the 
 *              attendees trigger.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/25/96	Initial Revision
 *			jmp	9/29/99	Use FrmGetFormPtr() & FrmGetObjectIndex() instead of
 *								GetObjectPtr() because GetObjectPtr() calls FrmGetActiveForm(),
 *								and FrmGetActiveForm() may return a form that isn't the one we
 *								want when other forms are up when we are called.
 *								Fixes bug #22418.
 *
 ***********************************************************************/
static void DetailsClose (void)
{
	void * label;
	FormPtr frm;

	frm = FrmGetFormPtr (DetailsDialog);
	label = (void *)CtlGetLabel (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, DetailsAttendeesSelector)));	
	if (label) MemPtrFree (label);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsApply
 *
 * DESCRIPTION: This routine applies the changes made in the Details Dialog.
 *              Also, the category label in the "Edit Expense View" title is 
 *              redrawn by this routine.
 *
 * PARAMETERS:  category        - new catagory
 *              dateP        - new due date
 *              categoryEdited - true is current category has moved, deleted
 *                 renamed, or merged with another category
 *
 * RETURNED:    code which indicates how the expense list was changed,  this
 *              code is send as the update code, in the frmUpdate event.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *			trm	7/21/97	Prevents default to Airfare when not selected
 *
 ***********************************************************************/
static UInt16 DetailsApply (UInt16 category, DateType * dateP, 
	Boolean categoryEdited, UInt16 currency)
{
	FormPtr					frm;
	FieldPtr					fld;
	Char *					str;
	UInt16						attr;
	UInt8						newExpenseType;
	UInt8						curExpenseType;
	UInt8						newPaymentType;
	UInt8						curPaymentType;
	UInt8						newCurrency;
	UInt8						curCurrency;
	UInt16 						updateCode = 0;
	UInt16						selection;
	Boolean					dirty = false;
	ListPtr					lst;
	MemHandle 				recordH;
	DateType					curDueDate;
	ExpensePackedRecordPtr	expenseRec;
	UInt8 *					expenseTypeUnconvertP;
	
	frm = FrmGetActiveForm ();

	// Get the category and the sercrt attribute of the current record.
	DmRecordInfo (ExpenseDB, CurrentRecord, &attr, NULL, NULL);	


	recordH = DmQueryRecord (ExpenseDB, CurrentRecord);
	expenseRec = (ExpensePackedRecordPtr) MemHandleLock (recordH);
	curDueDate = expenseRec->date;
	curExpenseType = expenseRec->type;
	curPaymentType = expenseRec->paymentType;
	curCurrency = expenseRec->currency;
	MemHandleUnlock (recordH);

	// Compare the due date setting in the dialog with the due date in the
	// current record.  Update the record if necessary.
	if (MemCmp (dateP, &curDueDate, sizeof (DateType)))
		{
		ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expenseDate, dateP);
		updateCode |= updateItemMove;
		dirty = true;
		}
	

	// Compare the expense type setting in the dialog with the expense type
	//  in the current record.  Update the record if necessary.
	lst = GetObjectPtr (DetailsExpenseTypeList);
	selection = LstGetSelection (lst);
	if (selection != noListSelection)
		{
		// Need to convert newExpenseType from popup list order to english expense type.
		expenseTypeUnconvertP = MemHandleLock (ExpenseTypeUnconvertH);
		newExpenseType = expenseTypeUnconvertP[selection];
		MemHandleUnlock (ExpenseTypeUnconvertH);
		
		ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expenseType, &newExpenseType);
		updateCode |= updateItemChanged;
		dirty = true;
		}
	

	// Compare the payment type setting in the dialog with the payment type
	// in the current record.  Update the record if necessary.
	lst = GetObjectPtr (DetailsPaymentTypeList);
	if (newExpenseType == expMileage)
		newPaymentType = payUnfiled;
	else
		newPaymentType = DetailsLstSelectionToPaymentType (LstGetSelection (lst));

	if (curPaymentType != newPaymentType)
		{
		ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expensePaymentType, &newPaymentType);
		dirty = true;
		}
	

	// Compare the currency setting in the dialog with the currency
	//  in the current record.  Update the record if necessary.
	if (newExpenseType == expMileage)
		newCurrency = DefaultCurrency;
	else
		newCurrency = currency;
	if (curCurrency != newCurrency)
		{
		ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expenseCurrency, &newCurrency);
		updateCode |= updateItemChanged;
		dirty = true;
		}
	

	// Compare the current category to the category setting of the dialog.
	// Update the record if the category are different.	
	if ((attr & dmRecAttrCategoryMask) != category)
		{
		attr &= ~dmRecAttrCategoryMask;
		attr |= category;
		dirty = true;

		CurrentCategory = category;
		updateCode |= updateCategoryChanged;
		}
	
	// If the current category was deleted, renamed, or merged with
	// another category, then the list view needs to be redrawn.
	if (categoryEdited)
		{
		CurrentCategory = category;
		updateCode |= updateCategoryChanged;
		}

#if 1
	// DOLATER - Japanese version has this code moved to the end of the
	// routine...I think it's because they added some code subsequent to this
	// which can set the dirty flag to true.
	// Save the new category and/or secret status, and mark the record dirty.
	if (dirty)
		{
		attr |= dmRecAttrDirty;
		DmSetRecordInfo (ExpenseDB, CurrentRecord, &attr, NULL);
		}
#endif

	// Check if the vendor field was been nodified,  if it has save in the 
	// current record and also add the vendor name to the vendor lookup 
	// database.
	fld = GetObjectPtr (DetailsVendorField);
	if (FldDirty(fld))
		{
		str = FldGetTextPtr (fld);
		ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expenseVendor, str);
#if 0
		// DOLATER - figure out if this is a general bug fix, or something
		// specific to Japanese.
		dirty = true;
#endif
				
		// Save the vedor name to the quick-fill datebase.
		LookupSave (vendorDBType, sysFileCExpense, str);
		}


	// Check if the city field was been nodified,  if it has save in the 
	// current record and also add the city name to the city lookup 
	// database.
	fld = GetObjectPtr (DetailsCityField);
	if (FldDirty(fld))
		{
		str = FldGetTextPtr (fld);
		ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expenseCity, str);
#if 0
		// DOLATER - figure out if this is a general bug fix, or something
		// specific to Japanese.
		dirty = true;
#endif
				
		// Save the vedor name to the quick-fill datebase.
		LookupSave (cityDBType, sysFileCExpense, str);
		}

#if 0
	// DOLATER - figure out if this is a general bug fix, or something
	// specific to Japanese.
	// Save the new category and/or secret status, and mark the record dirty.
	if (dirty)
		{
		attr |= dmRecAttrDirty;
		DmSetRecordInfo (ExpenseDB, CurrentRecord, &attr, NULL);
		}
#endif

	return (updateCode);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsInit
 *
 * DESCRIPTION: This routine initializes the Details Dialog.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/10/95	Initial Revision
 *			trm	7/21/97	Prevents default to airfare if not selected
 *
 ***********************************************************************/
static void DetailsInit (UInt16 * categoryP, DateType * dateP, UInt16 * currencyP)
{
	UInt16 attr;
	UInt16 category;
	Char * p;
	Char * name;
	Char * label;
	FormPtr frm;
	ListPtr lst;
	FieldPtr fld;
	ControlPtr ctl;
	MemHandle h;
	MemHandle recordH;
	ExpenseRecordType recordP;
	UInt8 * expenseTypeConvertP;
	UInt8 selection;


	frm = FrmGetActiveForm ();

	// Set the label of the category trigger.
	DmRecordInfo (ExpenseDB, CurrentRecord, &attr, NULL, NULL);
	category = attr & dmRecAttrCategoryMask;
	ctl = GetObjectPtr (DetailsCategoryTrigger);
	name = (Char *)CtlGetLabel (ctl);
	CategoryGetName (ExpenseDB, category, name);
	CategorySetTriggerLabel (ctl, name);


	// Get a pointer to the expense record.
	ExpenseGetRecord (ExpenseDB, CurrentRecord, &recordP, &recordH);


	// Set the expense type trigger and list.
	lst = GetObjectPtr (DetailsExpenseTypeList);
	ctl = GetObjectPtr (DetailsExpenseTypeTrigger);
	if (recordP.type != noExpenseType)
		{
		// recordP.type is an English-based index.  Convert to translated
		// index since Popup list is in translated alphabetical order.
		expenseTypeConvertP = MemHandleLock (ExpenseTypeConvertH);
		LstSetSelection (lst, expenseTypeConvertP[recordP.type]);
		label = LstGetSelectionText (lst, expenseTypeConvertP[recordP.type]);
		CtlSetLabel (ctl, label);
		MemHandleUnlock (ExpenseTypeConvertH);
		}
	else
		{
		LstSetSelection (lst, noListSelection);
		}	


	// Set the payment type trigger and list.
	lst = GetObjectPtr (DetailsPaymentTypeList);
	ctl = GetObjectPtr (DetailsPaymentTypeTrigger);
	selection = DetailsPaymentTypeToLstSelection ((ExpensePaymentType)recordP.paymentType);
	LstSetSelection (lst, selection);
	label = LstGetSelectionText (lst, selection);
	CtlSetLabel (ctl, label);
	if (recordP.type == expMileage)
		FrmHideObject (frm, FrmGetObjectIndex (frm, DetailsPaymentTypeTrigger));


	// Set the currency trigger and list.
	lst = GetObjectPtr (DetailsCurrencyList);
	ctl = GetObjectPtr (DetailsCurrencyTrigger);
	LstSetSelection (lst, recordP.currency);
	label = (Char *)CtlGetLabel (ctl);
	CurrencyGetSymbol (recordP.currency, label);
	CtlSetLabel (ctl, label);
	if (recordP.type == expMileage)
		FrmHideObject (frm, FrmGetObjectIndex (frm, DetailsCurrencyTrigger));
	*currencyP = recordP.currency;


	// If there is a vendor in the record copy it into the vendor field.
	if (recordP.vendor && *recordP.vendor)
		{
		h = MemHandleNew (StrLen (recordP.vendor)  + 1);
		p = MemHandleLock (h);
		StrCopy (p, recordP.vendor);
		MemPtrUnlock (p);

		fld = GetObjectPtr (DetailsVendorField);
		FldSetTextHandle (fld, h);
		}


	// If there is a city in the record copy it into the city field.
	if (recordP.city && *recordP.city)
		{
		h = MemHandleNew (StrLen (recordP.city)  + 1);
		p = MemHandleLock (h);
		StrCopy (p, recordP.city);
		MemPtrUnlock (p);

		fld = GetObjectPtr (DetailsCityField);
		FldSetTextHandle (fld, h);
		}


	// Set the label of the attendees selector.
	CtlSetLabel (GetObjectPtr (DetailsAttendeesSelector), NULL);
	DetailsSetAttendeesTrigger ();


	// Return the current category and due date.
	*categoryP = category;
	*dateP = recordP.date;

	// Unlock the expense record
	MemHandleUnlock (recordH);	
}


/***********************************************************************
 *
 * FUNCTION:    DetailsHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Details
 *              Dialog Box" of the Expense application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *			jmp	10/26/99	Prevent double redraw at frmUpdateEvent time
 *								by always running the Attendee trigger (either
 *								by itself or in conjuction with a full FrmDrawForm()).
 *			jmp	10/31/99	Call FrmDrawForm() before calling DetailsSetAttendeesTrigger()
 *								to prevent ctl lable from showing up before the form is
 *								drawn.
 *
 ***********************************************************************/
static Boolean DetailsHandleEvent (EventType * event)
{
	static UInt16		currency;
	static UInt16 		category;
	static Boolean		categoryEdited;
	static DateType 	date;
	UInt16				updateCode;
	FormPtr				frm;
	Boolean				handled = false;

	if (event->eType == keyDownEvent)
		{
		handled = DetailsAutoFill (event);
		}

	else if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case DetailsOkButton:
				updateCode = DetailsApply (category, &date, categoryEdited, currency);
				DetailsClose ();
				FrmReturnToForm (ListView);
				FrmUpdateForm (ListView, updateCode);
				handled = true;
				break;

			case DetailsCancelButton:
				if (categoryEdited)
					updateCode = updateCategoryChanged;
				else
					updateCode = 0;
				DetailsClose ();
				FrmUpdateForm (ListView, updateCode);
				FrmReturnToForm (ListView);
				handled = true;
				break;
				
			case DetailsDeleteButton:
				DetailsClose ();
				FrmReturnToForm (ListView);
				if ( DetailsDeleteExpense ()) 
					FrmUpdateForm (ListView, updateItemDelete);
				else
					FrmUpdateForm (ListView, 0);
				handled = true;
				break;
				
			case DetailsNoteButton:
				PendingUpdate = DetailsApply (category, &date, categoryEdited, currency);
				FrmCloseAllForms ();
				FrmGotoForm (NewNoteView);
				handled = true;
				break;

			case DetailsCategoryTrigger:
				categoryEdited = DetailsSelectCategory (&category) || categoryEdited;
				handled = true;
				break;

			case DetailsCurrencyTrigger:
				DetailsSelectCurrency (&currency);
				handled = true;
				break;

			case DetailsAttendeesSelector:
				FrmPopupForm (AttendeesDialog);
				handled = true;
				break;
			}
		}


	else if (event->eType == popSelectEvent)
		{
		if (event->data.popSelect.listID == DetailsExpenseTypeList)
			DetailsExpenseTypeSelected (event);
		}


	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		DetailsInit (&category, &date, &currency);
		FrmDrawForm (frm);
		FrmSetFocus (frm, FrmGetObjectIndex (frm, DetailsVendorField));

		categoryEdited = false;
		handled = true;
		}


	else if (event->eType == frmGotoEvent)
		{
		ItemSelected = true;

		frm = FrmGetActiveForm ();
		DetailsInit (&category, &date, &currency);
		DetailsGoToItem (event);
		FrmDrawForm (frm);

		categoryEdited = false;
		handled = true;
		}


	else if (event->eType == frmCloseEvent)
		{
		DetailsClose ();
		}
		

	else if (event->eType == frmUpdateEvent)
		{
		if (event->data.frmUpdate.updateCode == frmRedrawUpdateCode)
			{
			frm = FrmGetActiveForm ();
			FrmDrawForm (frm);
			DetailsSetAttendeesTrigger ();
			handled = true;
			}
		else if (event->data.frmUpdate.updateCode == updateAttendees)
			{
			DetailsSetAttendeesTrigger ();
			handled = true;
			}
		}

	return (handled);
}


#pragma mark -
/***********************************************************************
 *
 * FUNCTION:    NoteViewDrawTitleAndForm
 *
 * DESCRIPTION: This routine draws the form and title of the note view.
 *
 * PARAMETERS:  frm, FormPtr to the form to draw
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *    	jmp   9/27/99	Square off the NoteView title so that it covers up
 *                   	the blank Form title used to trigger the menu on taps
 *                   	to the title area.  Also, set the NoteView title's color
 *                   	to match the standard Form title colors.  Eventually, we
 *                   	should add a variant to Forms that allows for NoteView
 *                  		titles directly.  This "fixes" bug #21610.
 *       jmp   12/02/99 Fix bug #24377.  Don't call WinScreenUnlock() if WinScreenLock()
 *                      fails.
 *                          
 ***********************************************************************/
 static void NoteViewDrawTitleAndForm (FormPtr frm)
{
 	Coord x;
   Coord formWidth;
   RectangleType r;
	Char expenseName [maxExpenseNameLength];
	FontID curFont;
	Char * str;
	MemHandle resH;
	MemHandle recordH;
	RectangleType eraseRect, drawRect;
	ExpenseRecordType recordP;
	IndexedColorType curForeColor;
	IndexedColorType curBackColor;
	IndexedColorType curTextColor;
	UInt8 * lockedWinP;

	// Get current record and related info.
	//
	ExpenseGetRecord (ExpenseDB, CurrentRecord, &recordP, &recordH);

	// "Lock" the screen so that all drawing occurs offscreen to avoid
	// the anamolies associated with drawing the Form's title then drawing
	// the NoteView title.  We REALLY need to make a variant for doing
	// this in a more official way!
	//
	lockedWinP = WinScreenLock(winLockCopy);

	FrmDrawForm(frm);
	
	// Peform initial set up.
	//
   FrmGetFormBounds(frm, &r);
   formWidth = r.extent.x;
   
   RctSetRectangle (&eraseRect, 0, 0, formWidth, FntLineHeight()+4);
   RctSetRectangle (&drawRect, 0, 0, formWidth, FntLineHeight()+2);

	// Save/Set window colors and font.  Do this after FrmDrawForm() is called
	// because FrmDrawForm() leaves the fore/back colors in a state that we
	// don't want here.
	//
 	curForeColor = WinSetForeColor (UIColorGetTableEntryIndex(UIFormFrame));
 	curBackColor = WinSetBackColor (UIColorGetTableEntryIndex(UIFormFill));
 	curTextColor = WinSetTextColor (UIColorGetTableEntryIndex(UIFormFrame));
	curFont = FntSetFont (noteTitleFont);

	// Erase the Form's title area and draw the NoteView's.
	//
	WinEraseRectangle (&eraseRect, 0);
	WinDrawRectangle (&drawRect, 3);

	if (recordP.type != noExpenseType)
		{
		// String list is in English alphabetical order; no need to convert.
		SysStringByIndex (expenseStrListID, recordP.type, expenseName, 
			maxExpenseNameLength-1);

		x = (formWidth - FntCharsWidth (expenseName, StrLen (expenseName)) + 1 ) / 2;
		WinDrawInvertedChars (expenseName, StrLen (expenseName), x, 1);
		}

	else
		{
		resH = DmGetResource (strRsc, noExpenseTypeStr);
		str = MemHandleLock (resH);
		x = (formWidth - FntCharsWidth (str, StrLen (str)) + 1 ) / 2;
		WinDrawInvertedChars (str, StrLen (str), x, 1);
		MemHandleUnlock (resH);
		}

	// Now that we've drawn everything, blast it all back on the screen at once.
	//
	if (lockedWinP)
   	WinScreenUnlock();

	// Unlock the record that ExpenseGetRecord() implicitly locked.
	//
	MemHandleUnlock (recordH);

   // Restore window colors and font.
   //
   WinSetForeColor (curForeColor);
   WinSetBackColor (curBackColor);
   WinSetTextColor (curTextColor);
   FntSetFont (curFont);
}
 

/***********************************************************************
 *
 * FUNCTION:    NoteViewUpdateScrollBar
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
 *			art	07/01/96	Initial Revision
 *			gap	11/02/96	Fix case where field and scroll bars get out of sync
 *
 ***********************************************************************/
static void NoteViewUpdateScrollBar (void)
{
	UInt16 scrollPos;
	UInt16 textHeight;
	UInt16 fieldHeight;
	Int16 maxValue;
	FieldPtr fld;
	ScrollBarPtr bar;

	fld = GetObjectPtr (NoteField);
	bar = GetObjectPtr (NoteScrollBar);
	
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
	else if (scrollPos)
		maxValue = scrollPos;
	else
		maxValue = 0;

	SclSetScrollBar (bar, scrollPos, 0, maxValue, fieldHeight-1);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewLoadRecord
 *
 * DESCRIPTION: This routine loads the note filed of a expense record into 
 *              the note edit field.
 *
 * PARAMETERS:  frm - pointer to the Edit View form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *			jmp	9/8/99	In the Great Substitution, MemPtr became a local
 *								variable; changed MemPtr back to ptr.
 *
 ***********************************************************************/
static void NoteViewLoadRecord (void)
{
	UInt16 offset;
	FieldPtr fld;
	MemHandle recordH;
	Char * ptr;
	ExpenseRecordType recordP;
	
	// Get a pointer to the memo field.
	fld = GetObjectPtr (NoteField);
	
	// Set the font used in the memo field.
	FldSetFont (fld, NoteFont);
	
	// Compute the offset within the expense record of the note string.
	// The field object will edit the note in place; it is not copied
	// to the dynamic heap.
	ExpenseGetRecord (ExpenseDB, CurrentRecord, &recordP, &recordH);

	ptr = MemHandleLock (recordH);
	offset = recordP.note - ptr;
	FldSetText (fld, recordH, offset, StrLen(recordP.note)+1);

	MemHandleUnlock (recordH);
	MemHandleUnlock (recordH);		// was also locked in ExpenseGetRecord
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewSave
 *
 * DESCRIPTION: This routine 
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void NoteViewSave (void)
{
	FieldPtr fld;
	
	fld = GetObjectPtr (NoteField);

	// Was the note string modified by the user.
	if (FldDirty (fld))
		{
		// Release any free space in the note field.
		FldCompactText (fld);

		// Mark the record dirty.	
		DirtyRecord (CurrentRecord);
		}


	// Clear the handle value in the field, otherwise the handle
	// will be free when the form is disposed of,  this call also unlocks
	// the handle that contains the note string.
	FldSetTextHandle (fld, 0);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewDeleteNote
 *
 * DESCRIPTION: This routine deletes a the note field from a expense record.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the note was deleted.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static Boolean NoteViewDeleteNote (void)
{
	FieldPtr fld;
	
	if (FrmAlert(DeleteNoteAlert) != DeleteNoteYes)
		return (false);

	// Unlock the MemHandle that contains the text of the memo.
	fld = GetObjectPtr (NoteField);
	ErrFatalDisplayIf ((! fld), "Bad field");

	// Clear the handle value in the field, otherwise the handle
	// will be free when the form is disposed of. this call also 
	// unlocks the MemHandle the contains the note string.
	FldSetTextHandle (fld, 0);	

	ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expenseNote, "");

	// Mark the record dirty.	
	DirtyRecord (CurrentRecord);
	
	return (true);
}



/***********************************************************************
 *
 * FUNCTION:    NoteViewDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *			jmp	9/8/99	To make Expense more consistent with the
 *								other built-in apps, the font controls
 *								have been hidden, so we now handle the
 *								NoteView's font menu item.
 *			jmp	9/17/99	Eliminate obsolete goto top/bottom menu items.
 *
 ***********************************************************************/
static Boolean NoteViewDoCommand (UInt16 command)
{
	FieldPtr fld;
	Boolean handled = true;
	
	switch (command)
		{
		case newNoteFontCmd:
			NoteFont = SelectFont (NoteFont);
			break; 
		
		case newNotePhoneLookupCmd:
			fld = GetObjectPtr (NoteField);
			PhoneNumberLookup (fld);
			break;
			
		default:
			handled = false;
		}	
	return (handled);
}




/***********************************************************************
 *
 * FUNCTION:    NoteViewScroll
 *
 * DESCRIPTION: This routine scrolls the mote Note View a page or a 
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
 *			art	7/1/96	Initial Revision
 *			jmp	9/8/99	Copying updates Grant did to other built-in
 *								apps on 2/2/99:  Use NoteVewUpdateScrollBar().
 *
 ***********************************************************************/
static void NoteViewScroll (Int16 linesToScroll, Boolean updateScrollbar)
{
   UInt16	blankLines;
   FieldPtr	fld;
   
   fld = GetObjectPtr (NoteField);
   blankLines = FldGetNumberOfBlankLines (fld);

   if (linesToScroll < 0)
      FldScrollField (fld, -linesToScroll, winUp);
   else if (linesToScroll > 0)
      FldScrollField (fld, linesToScroll, winDown);
      
   // If there were blank lines visible at the end of the field
   // then we need to update the scroll bar.
   if (blankLines && linesToScroll < 0 || updateScrollbar)
      {
      NoteViewUpdateScrollBar();
      }
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewPageScroll
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
 *			art	7/1/96	Initial Revision
 *			jmp	9/8/99	Copying updates Grant did to other built-in
 *								apps on 2/2/99:  Use NoteViewScroll() to do actual
 *								scrolling
 *
 ***********************************************************************/
static void NoteViewPageScroll (WinDirectionType direction)
{
   UInt16 linesToScroll;
   FieldPtr fld;

   fld = GetObjectPtr (NoteField);
   
   if (FldScrollable (fld, direction))
      {
      linesToScroll = FldGetVisibleLines (fld) - 1;

		if (direction == winUp)
			linesToScroll = -linesToScroll;
		
		NoteViewScroll(linesToScroll, true);
      }
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewInit
 *
 * DESCRIPTION: This routine initializes the NoteView form.
 *
 * PARAMETERS:  frm - pointer to the NoteView form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *			jmp	9/8/99	To make Expense more consistent with the
 *								other built-in apps, hide the font controls
 *								in the NoteView.
 *			jmp	9/23/99	Eliminate code that hides the obsolete font
 *								controls since we're using a form that no longer
 *								has them.
 *
 ***********************************************************************/
static void NoteViewInit (FormPtr frm)
{
	FieldPtr 		fld;
	FieldAttrType	attr;

	NoteViewLoadRecord ();

	// Have the field send events to maintain the scroll bar.
	fld = GetObjectPtr (NoteField);
	FldGetAttributes (fld, &attr);
	attr.hasScrollBar = true;
	FldSetAttributes (fld, &attr);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the NoteView
 *              of the Expense application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	1/2/96		Initial Revision
 *			CS		06/22/99		Standardized keyDownEvent handling
 *									(TxtCharIsHardKey, commandKeyMask, etc.)
 *			jmp	9/8/99		To make Expense more consistent with the
 *									other built-in apps, the font controls
 *									have been hidden, so we no longer need to
 *									handle their events either.
 *			jmp	9/27/99		Combined NoteViewDrawTitle() & FrmUpdateForm()
 *									into a single routine that is now called
 *									NoteViewDrawTitleAndForm().
 *
 ***********************************************************************/
static Boolean NoteViewHandleEvent (EventType * event)
{
	UInt16 pos;
	FormPtr frm;
	FieldPtr fld;
	Boolean handled = false;


	if (event->eType == keyDownEvent)
		{
		if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
			{
			NoteViewSave ();
			ClearEditState ();
			FrmGotoForm (ListView);
			handled = true;
			}
		else if (EvtKeydownIsVirtual(event))
			{
			if (event->data.keyDown.chr == vchrPageUp)
				{
				NoteViewPageScroll (winUp);
				handled = true;
				}
	
			else if (event->data.keyDown.chr == vchrPageDown)
				{
				NoteViewPageScroll (winDown);
				handled = true;
				}
			}
		}

	else if (event->eType == ctlSelectEvent)
		{		
		switch (event->data.ctlSelect.controlID)
			{
			case NoteDoneButton:
				NoteViewSave ();
				FrmGotoForm (ListView);
				handled = true;
				break;

			case NoteDeleteButton:
				if (NoteViewDeleteNote())
					FrmGotoForm (ListView);
				handled = true;
				break;
			}
		}


	else if (event->eType == fldChangedEvent)
		{
		frm = FrmGetActiveForm ();
		NoteViewUpdateScrollBar ();
		handled = true;
		}
		

	else if (event->eType == menuEvent)
		{
		handled = NoteViewDoCommand (event->data.menu.itemID);
		}
	
	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		NoteViewInit (frm);
		NoteViewDrawTitleAndForm (frm);
		NoteViewUpdateScrollBar ();
		FrmSetFocus (frm, FrmGetObjectIndex (frm, NoteField));
		handled = true;
		}
	

	else if (event->eType == frmGotoEvent)
		{
		frm = FrmGetActiveForm ();

		ItemSelected = true;
		CurrentRecord = event->data.frmGoto.recordNum;
		NoteViewInit (frm);

		fld = GetObjectPtr (NoteField);
		pos = event->data.frmGoto.matchPos;
		FldSetScrollPosition (fld, pos);
		FldSetSelection (fld, pos, pos + event->data.frmGoto.matchLen);
		NoteViewDrawTitleAndForm (frm);
		NoteViewUpdateScrollBar ();
		FrmSetFocus (frm, FrmGetObjectIndex (frm, NoteField));
		handled = true;
		}
			
	else if (event->eType == frmUpdateEvent)
		{
		if (event->data.frmUpdate.updateCode & updateFontChanged)
			{
			fld = GetObjectPtr (NoteField);
			FldSetFont (fld, NoteFont);
			NoteViewUpdateScrollBar ();
			}
		else
			{
			frm = FrmGetActiveForm ();
			NoteViewDrawTitleAndForm (frm);
			}
		handled = true;
		}


	else if (event->eType == frmCloseEvent)
		{
		if ( FldGetTextHandle (GetObjectPtr (NoteField)))
			NoteViewSave ();
		}

	
	else if (event->eType == sclRepeatEvent)
		{
		NoteViewScroll (event->data.sclRepeat.newValue - 
			event->data.sclRepeat.value, false);
		}

	return (handled);
}


#pragma mark -
/***********************************************************************
 *
 * FUNCTION:    ListViewDrawDate
 *
 * DESCRIPTION: This routine draws the items date.
 *
 * PARAMETERS:	 table  - pointer to a table object
 *              row    - row the item is in
 *              column - column the item is in
 *              bounds - bounds of region to draw in
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewDrawDate (void * table, Int16 row, Int16 /* column */, 
	RectanglePtr bounds)
{
	DateType date;
	
	// Get the due date to the item being drawn.
	*((Int16 *) (&date)) = TblGetItemInt (table, row, dateColumn);
	DrawDate (date, bounds->topLeft.x, bounds->topLeft.y, bounds->extent.x,
			DateFormat);
}

/***********************************************************************
 *
 * FUNCTION:    DrawInversionEffect
 *
 * DESCRIPTION: This routine does an inversion effect by swapping colors
 *					 this is NOT undoable by calling it a second time, rather
 *					 it just applies a selected look on top of already
 *					 rendered data.  (It's kind of a hack.)
 *
 * PARAMETERS:	 rP  - pointer to a rectangle to 'invert'
 *					 cornerDiam	- corner diameter
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	10/28/99	Initial revision
 *
 ***********************************************************************/
static void DrawInversionEffect (const RectangleType *rP, UInt16 cornerDiam) 
{
	WinPushDrawState();
	WinSetDrawMode(winSwap);
	WinSetPatternType (blackPattern);
	WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetTableEntryIndex(UIObjectSelectedFill));
	WinPaintRectangle(rP, cornerDiam);
	
	if (UIColorGetTableEntryIndex(UIObjectSelectedFill) != UIColorGetTableEntryIndex(UIObjectForeground)) {
		WinSetBackColor(UIColorGetTableEntryIndex(UIObjectForeground));
		WinSetForeColor(UIColorGetTableEntryIndex(UIObjectSelectedForeground));
		WinPaintRectangle(rP, cornerDiam);
	}
	WinPopDrawState();
}

/***********************************************************************
 *
 * FUNCTION:    ListViewSelectEditIndicator
 *
 * DESCRIPTION: This routine selects or unselects the date region
 *					 as specified.
 *
 * PARAMETERS:	 table    - pointer to a table object
 *					 selected - select or unselect item
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/11/96	Initial Revision
 *			jmp	10/31/99	Eliminate WinInvertRectangle(); redraw item
 *								either selected or unselected as indicated.
 *			jmp	11/01/99	Erase rectangle BEFORE redrawing because
 *								ListViewDrawDate() only redraws as much as
 *								needs to, not the entire rectangle.
 *
 ***********************************************************************/
static void ListViewSelectEditIndicator (TablePtr table, Boolean selected)
{
	Int16 row;
	Int16 column;
	RectangleType indictorR;

	TblGetSelection (table, &row, &column);
   TblGetItemBounds (table, row, dateColumn, &indictorR);

	WinPushDrawState();
	WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));

	WinEraseRectangle (&indictorR, 0);
	ListViewDrawDate (table, row, dateColumn, &indictorR);
	
	if (selected)
		DrawInversionEffect(&indictorR, 3);
	
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    ListViewGrabFocus
 *
 * DESCRIPTION: This routine set the focus to the amount field in the 
 *              specified row.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/9/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewGrabFocus (Int16 row)
{
	FormPtr frm;
	TablePtr table;
	FieldPtr fld;
	RectangleType r;

	frm = FrmGetActiveForm ();
	table = GetObjectPtr (ListTable);
	ErrFatalDisplayIf(table->attr.editing, "Table already has focus");

	FrmSetFocus (frm, FrmGetObjectIndex (frm, ListTable));

	// DOLATER zzz Fix API
	table->attr.editing = true;
	table->currentRow = row;
	table->currentColumn = amountColumn;

	fld = TblGetCurrentField (table);
	TblGetItemBounds (table, row, amountColumn, &r);
	ListViewInitAmount (table, fld, row, amountColumn, &r, true);

	FldGrabFocus (fld);

	ListViewSelectEditIndicator (table, true);

	ItemSelected = true;
}


/***********************************************************************
 *
 * FUNCTION:    ListRestoreEditState
 *
 * DESCRIPTION: This routine restores the edit state of the expense 
 *              list, if the list is in edit mode. This routine is 
 *              called after the date of an item is changed, or 
 *              after returning from the details dialog or note view.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListRestoreEditState ()
{
	Int16 row;
	TablePtr table;
	FieldPtr fld;

	if ( ! ItemSelected) return;

	// Find the row the the current record is in.  Its posible 
	// that the current record is nolong displayable (ex: only due 
	// item are being display and the due date was changed
	// such that the record don't display).
	table = GetObjectPtr (ListTable);
	if ( ! TblFindRowID (table, CurrentRecord, &row) )
		{
		ClearEditState ();
		return;
		}

	ListViewGrabFocus (row);

	// Restore the insertion point position.
	fld = TblGetCurrentField (table);
	FldSetInsPtPosition (fld, ListEditPosition);
	if (ListEditSelectionLength)
		FldSetSelection (fld, ListEditPosition, 
			ListEditPosition + ListEditSelectionLength);
}



/***********************************************************************
 *
 * FUNCTION:    ListViewClearEditState
 *
 * DESCRIPTION: This routine clears the edit state of the expense list.
 *              It is caled whenever a table item is selected.
 *
 *              If the new item selected is in a different row than
 *              the current record the edit state is cleared,  and if 
 *              current record is empty it is deleted.
 *
 * PARAMETERS:  newRow - row number of newly table item
 *
 * RETURNED:    true if the current record is deleted.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *			jmp	9/29/99	Use FrmGetFormPtr() & FrmGetObjectIndex() instead of
 *								GetObjectPtr() because GetObjectPtr() calls FrmGetActiveForm(),
 *								and FrmGetActiveForm() may return a form that isn't the one we
 *								want when other forms are up when we are called.
 *								Fixes bug #22418.
 *
 ***********************************************************************/
static Boolean ListViewClearEditState (void)
{
	Int16 row;
	Int16 rowsInTable;
	TablePtr table;
	FormPtr frm;

	if ( ! ItemSelected) return (false);

	frm = FrmGetFormPtr (ListView);
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ListTable));
	if ( ! TblFindRowID (table, CurrentRecord, &row) )
		return (false);

	ListViewSaveAmount (table);

	// If a different row has been selected, clear the edit state, this 
	// wiil delete the current record if its empty.
	if (ClearEditState ())
		{
		rowsInTable = TblGetNumberOfRows (table);
		for (; row < rowsInTable; row++)
			TblSetRowUsable (table, row, false);

		ListViewLoadTable (true);
		TblRedrawTable (table);
		
		return (true);
		}

	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewDrawExpenseType
 *
 * DESCRIPTION: This routine draws the description of the expense type.
 *
 * PARAMETERS:	 table  - pointer to a table object
 *              item   - pointer to an item in the table
 *              row    - row the item is in
 *              column - column the item is in
 *              x      - left bound to the item
 *              y      - top bound of the item
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		01/08/96	art	Created by Art Lamb.
 *		04/10/97	SL		Fixed first character hiliting bug by setting
 *							width to width of AutoFillLen characters of str
 *							instead of AutoFillBuffer; solves case problem.
 *		09/20/99	kwk	Use FtrGet to load expense type conversion handle,
 *							since globals might not be available.
 *		10/31/99	jmp	Eliminate WinInvertRectangle() by using DrawInversionEffect()
 *							routine.
 *
 ***********************************************************************/
static void ListViewDrawExpenseType (void * table, Int16 row, Int16 /* column */, 
	RectanglePtr bounds)
{
	Int16 type;
	UInt16 width;
	Int16 x, y;
	ListPtr lst;
	Char * str;
	MemHandle resH = 0;
	RectangleType rect;
	RectangleType clip;
	RectangleType saveClip;
	UInt8 * expenseTypeConvertP;
	Err result;
	MemHandle convertH;
	
	WinPushDrawState();
	WinSetBackColor(UIColorGetTableEntryIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetTableEntryIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetTableEntryIndex(UIObjectForeground));
	FntSetFont (stdFont);
	
	WinEraseRectangle (bounds, 0);

	// Prevent drawing outside the bounds of the region passed.
	WinGetClip (&saveClip);
	RctCopyRectangle (bounds, &clip);
	WinClipRectangle (&clip);
	WinSetClip (&clip);

	// Get the due date to the item being drawn.
	type = TblGetItemInt (table, row, typeColumn);

	x = bounds->topLeft.x;
	y = bounds->topLeft.y;


	lst = GetObjectPtr (ListExpenseTypeList);
	if (type != noExpenseType)
		{
		ErrNonFatalDisplayIf(type >= numExpenseTypes && type != noExpenseType, "invalid expense type");
		
		// type is an English-based index.  Convert to translated
		// index since Popup list is in translated alphabetical order.
		// Note that we can't access globals here, since we might be
		// getting called to update our form when a sublauched app is
		// redrawing its form.
		result = FtrGet(sysFileCExpense, expFeatureNum, (UInt32*)&convertH);
		ErrNonFatalDisplayIf(result, "Missing convert handle feature");
		expenseTypeConvertP = MemHandleLock (convertH);
		str = LstGetSelectionText (lst, expenseTypeConvertP[type]);
		MemHandleUnlock (convertH);
		WinDrawChars (str, StrLen (str), x, y);

		// If the row passed has the focus and we're in quick fill
		// mode then highlingth the unigue portion of the expense name.
		if ((CurrentRecord != noRecordSelected) && AutoFill && AutoFillLen)
			{
			if (TblGetRowID (table, row) == CurrentRecord)
				{
				width = FntCharsWidth (str, AutoFillLen);
				rect.topLeft.x = x + width;
				rect.topLeft.y = y;
				rect.extent.x = FntCharsWidth (str, StrLen(str)) - width;
				rect.extent.y = FntLineHeight ();
				DrawInversionEffect (&rect, 0);
				}
			}
		}

	else
		{
		resH = DmGetResource (strRsc, noExpenseTypeStr);
		str = MemHandleLock (resH);
		WinDrawChars (str, StrLen (str), x, y);
		MemHandleUnlock (resH);
		}

	WinSetClip (&saveClip);
	WinPopDrawState ();
}


/***********************************************************************
 *
 * FUNCTION:    ListViewDrawCurrency
 *
 * DESCRIPTION: This routine draws the items date.
 *
 * PARAMETERS:	 table  - pointer to a table object
 *              row    - row the item is in
 *              column - column the item is in
 *              bounds - bounds of region to draw in
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/21/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewDrawCurrency (void * table, Int16 row, Int16 /* column */, 
	RectanglePtr bounds)
{
	UInt16 len;
	UInt16 currency;
	Int16 drawX;
	FontID curFont;
	Char symbol [currencySymbolLength];
	
	curFont = FntSetFont (stdFont);
	
	// Get the curreny type to the item being drawn.
	currency = TblGetItemInt (table, row, currencyColumn);
	
	// Get the currency symbol or the unit of distance if the expense 
	// type is mileage.
	if (currency != currencyDistance)
		CurrencyGetSymbol (currency, symbol);
		
	else
		SysStringByIndex (distanceStrListID, UnitOfDistance, symbol, sizeof(symbol)-1);

	// Right justify the currency symbol.
	len = StrLen (symbol);
	drawX = bounds->topLeft.x + + bounds->extent.x - 
		FntCharsWidth (symbol, len);

	WinDrawChars (symbol, len, drawX, bounds->topLeft.y);
	
	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    FormatAmount
 *
 * DESCRIPTION: This routine formats the amount string with the correct
 *              number of digits after the decimal separator.
 *
 * PARAMETERS:  fld - amount field
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/96	Initial Revision
 *
 ***********************************************************************/
static void FormatAmount (FieldPtr fld, UInt16 decimalPlaces)
{
	Int16 i;
	Int16 len;
	Int16 start;
	Int16 deleteCount;
	Int16 decimalCount;
	Int16 bufferLen = 0;
	Char buffer [6];
	Char * str;
	Char * MemPtr;
	
	str = FldGetTextPtr (fld);
	len = StrLen (str);
	if (! len) return;

	// Strip leading zeros.
	// DOLATER kwk - we should use TxtGetNextChar here.
	deleteCount = 0;
	while (len-1 > deleteCount)
		{
		if (str[deleteCount] == '0' && str [deleteCount+1] != DecimalSeperator)
			deleteCount++;
		else
			break;
		}
	if (deleteCount)
		{
		FldDelete (fld, 0, deleteCount);
		len -= deleteCount;
		}
		


	// Find the position of the decimal seperator.
	MemPtr = StrChr (str, DecimalSeperator);

	// If there no decimal seperator insert one and insert zeros 
	// after the seperator.
	if ((! MemPtr) && (decimalPlaces))
		{
		buffer[0] = DecimalSeperator;
		bufferLen = 1;
		
		for (i = 0; i < decimalPlaces; i++)
			buffer[bufferLen++] = '0';
		}

	// There is a decimal seperator
	else if (MemPtr)
		{
		decimalCount = len - (MemPtr - str) - 1;


		// If we're not displaying and decimal places then delete the 
		// decimal seperator and all the character to the right of it.
		if (! decimalPlaces)
			{
			start = (MemPtr - str);
			FldDelete (fld, start, len);
			}
		
		// Append zeros to the end of the string if there are to few 
		// character after the decimal seperator.
		else if (decimalCount < decimalPlaces)
			{
			for ( i = decimalCount; i < decimalPlaces; i++)
				buffer[bufferLen++] = '0';
			}

		// If the are too many characters after the decimal seperator
		// desired then truncate the extras.
		else if (decimalCount > decimalPlaces)
			{
			start = (MemPtr - str) + 1 + decimalPlaces;
			FldDelete (fld, start, len);
			}
		}

	// If we're need to appending characters to the field then do it.
	if (bufferLen)
		{
		if (len + bufferLen > maxAmountChars)
			bufferLen = maxAmountChars - len;

		FldSetInsPtPosition (fld, len);
		FldSetSelection (fld, len, len);
		FldInsert (fld, buffer, bufferLen);
		}
}


/***********************************************************************
 *
 * FUNCTION:    ListViewSaveAmount
 *
 * DESCRIPTION: This routine saves the amount field of a expense item 
 *              to its db record.  This routine is called by the table 
 *              object, as a callback routine, when it wants to save
 *              a expense description.
 *
 * PARAMETERS:  table  - pointer to the memo list table (TablePtr)
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	01/02/96	Initial Revision
 *			kwk	01/14/99	Validate saved selection info if we call FormatAmount.
 *			jmp	10/31/99	Move WinEraseRectangle() to ListViewDrawExpenseType().
 *
 ***********************************************************************/
static void ListViewSaveAmount (TablePtr table)
{
	Int16 row;
	UInt16 recordNum;
	UInt16 selectStart;
	UInt16 selectEnd;
	UInt16 decimalPlaces;
	Boolean dirty;
	Char * MemPtr;
	Char * textP;
	MemHandle recordH;
	FieldPtr fld;
	UInt16 textLength;
	RectangleType bounds;
	ExpenseRecordType recordP;
	
	// Get the current field, if there isn't one then we not in edit mode.
	fld = TblGetCurrentField (table);
	if (!fld) return;
		
	// Get the record number that corresponds to the table item to save.
	recordNum = CurrentRecord;

	// If the description has been modified mark the record dirty,  any 
	// change make to the expense's description were written directly
	//  to the expense record.
	dirty = FldDirty (fld);
	if (dirty)
		DirtyRecord (recordNum);

	// Save the dirty state, we're need it if we auto-delete an empty record.
	RecordDirty = dirty;


	// Save the insertion point position, and length of the selection.  
	// We'll need the insertion point position an slection length
	// if we put the table back into edit mode, and 
	ListEditPosition = FldGetInsPtPosition (fld);
	
	FldGetSelection (fld, &selectStart, &selectEnd);
	ListEditSelectionLength = selectEnd - selectStart;
	if (ListEditSelectionLength)
		ListEditPosition = selectStart;

	textP = FldGetTextPtr (fld);
	if (textP && dirty)
		{
		// Formats the amount string with the correct number of digits 
		// after the decimal.
		ExpenseGetRecord (ExpenseDB, recordNum, &recordP, &recordH);
		if (recordP.type == expMileage)
			decimalPlaces = 0;
		else
			decimalPlaces = CurrencyGetDecimalPlaces (recordP.currency);
		FormatAmount (fld, decimalPlaces);
		MemHandleUnlock (recordH);

		// We potentially changed the amount of text in the field with the
		// call to FormatAmount, so make sure our ListEditPosition &
		// ListEditSelectionLength globals are still valid.
		
		textLength = FldGetTextLength(fld);
		if (ListEditPosition > textLength)
			ListEditPosition = textLength;
		
		if (ListEditPosition + ListEditSelectionLength > textLength)
			ListEditSelectionLength = textLength - ListEditPosition;
		
		// Store the amount with a period as the decimal seperator.
		textP = FldGetTextPtr (fld);
		MemPtr = StrChr (textP, DecimalSeperator);
		if (MemPtr)
			*MemPtr = periodChr;
		
		ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expenseAmount, textP);
		}
	
	FldReleaseFocus (fld);
	FldSetSelection (fld, 0, 0);
	FldFreeMemory (fld);

	// We're no longer editing the field, so lets clean winUp, clear the 
	// editing flag and turn off the inserting point or selection and 
	// release the memory allocated for the field.  Do not free the 
	// block the contains the string we edited.  
	table->attr.editing = false;
	ListViewSelectEditIndicator (table, false);
	
	// if the row is in auto-fill mode 
	if (AutoFill)
		{
		AutoFill = false;

		// Redraw the expense type.
		if (TblFindRowID (table, recordNum, &row))
			{
			TblGetItemBounds (table, row, typeColumn, &bounds);
			ListViewDrawExpenseType (table, row, typeColumn, &bounds);
			}
		}	
}


/***********************************************************************
 *
 * FUNCTION:    ListViewInitAmount
 *
 * DESCRIPTION: This routine initializes a current amount field in the 
 *              expense table.
 *
 * PARAMETERS:	 table   - pointer to a table object
 *              fld     - pointer to a field object
 *              row     - row in the table of the field
 *              column  - column in the table of the field
 *              bounds  - bounds of the field object
 *              visible - true if the field has been drawn already
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/8/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewInitAmount (TablePtr table, FieldPtr fld, Int16 row, 
	Int16 /* column */, RectanglePtr bounds, Boolean visible)
{
	UInt16 recordNum;
	MemHandle recordH;
	MemHandle textH;
	Char * MemPtr;
	Char * textP;
	ExpenseRecordType recordP;
	
	MemSet (fld, sizeof(FieldType), 0);
	
	RctCopyRectangle (bounds, &fld->rect);

	// DOLATER use API routines
	fld->attr.usable = true;
	fld->attr.visible = visible;
	fld->attr.editable = true;
	fld->attr.singleLine = true;
	fld->attr.dynamicSize = false;
	fld->attr.underlined = true;
	fld->attr.insPtVisible = true;
	
	fld->attr.numeric = true;
	fld->attr.justification = rightAlign;
	
	fld->maxChars = maxAmountChars;

	// Get the record number that corresponds to the table item.
	// The record number is stored as the row id.
	recordNum = TblGetRowID (table, row);
	
	ExpenseGetRecord (ExpenseDB, recordNum, &recordP, &recordH);


	// The amount value is stored with a period as a decimal seperator.
	// Change to seperator to the home country's currency seperator.
	if (*recordP.amount)
		{
		textH = MemHandleNew (StrLen (recordP.amount) + 1);
		textP = MemHandleLock (textH);
		StrCopy (textP, recordP.amount);
		MemPtr = StrChr (textP, periodChr);
		if (MemPtr)
			*MemPtr = DecimalSeperator;

		MemPtrUnlock (textP);
		FldSetTextHandle (fld, textH);
		}

	MemHandleUnlock (recordH);
}


/***********************************************************************
 *
 * FUNCTION:    DrawTexttem
 *
 * DESCRIPTION: This routine draws the amount value of an expense item.
 *
 * PARAMETERS:	 table   - pointer to a table object
 *              row     - row in the table of the aounnt field
 *              column  - column in the table of the amount field
 *              bounds  - bounds of the field object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/8/96	Initial Revision
 *			bob	09/10/99	delete workaround for right justified underlines
 *
 ***********************************************************************/
static void ListViewDrawAmount (void * table, Int16 row, Int16 column, 
	RectanglePtr bounds)
{
	FieldType field;
	
	ListViewInitAmount (table, &field, row, column, bounds, false);

	FldDrawField (&field);

	FldFreeMemory (&field);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewUpdateScrollers
 *
 * DESCRIPTION: This routine draws or erases the list view scroll arrow
 *              buttons.
 *
 * PARAMETERS:  frm          -  pointer to the expense list form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *			jmp	9/29/99	Use FrmGetFormPtr() & FrmGetObjectIndex() instead of
 *								GetObjectPtr() because GetObjectPtr() calls FrmGetActiveForm(),
 *								and FrmGetActiveForm() may return a form that isn't the one we
 *								want when other forms are up when we are called.
 *								Fixes bug #22418.
 *
 ***********************************************************************/
static void ListViewUpdateScrollers (FormPtr frm)
{
	Int16 rows;
	UInt16 minValue;
	UInt16 maxValue;
	UInt16 recordsInCategory;
	
	rows = TblGetNumberOfRows (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ListTable)));
	recordsInCategory = DmNumRecordsInCategory (ExpenseDB, CurrentCategory);

	if (recordsInCategory)
		minValue = DmPositionInCategory (ExpenseDB, TopVisibleRecord, CurrentCategory);
	else
		minValue = 0;

	if (recordsInCategory > rows)
		maxValue = recordsInCategory - rows;
	else
		maxValue = 0;

	SclSetScrollBar (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm,ListScrollBar)), minValue, 0, maxValue, rows);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewInitTableRow
 *
 * DESCRIPTION: This routine initialize a row in the expense list.
 *
 * PARAMETERS:  table      - pointer to the table of expense items
 *              row        - row number (first row is zero)
 *              recordNum  - the index of the record display in the row
 *              rowHeight  - height of the row in pixels
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewInitTableRow (TablePtr table, Int16 row, UInt16 recordNum, 
	Int16 rowHeight, UInt32 uniqueID)
{
	UInt16 currency;
	MemHandle recordH;
	ExpensePackedRecordPtr expenseRec;

	// Get a pointer to the expense record.
	recordH = DmQueryRecord( ExpenseDB, recordNum);
	expenseRec = (ExpensePackedRecordPtr) MemHandleLock (recordH);

	// Make the row usable.
	TblSetRowUsable (table, row, true);
	
	// Set the height of the row to the height of the desc
	TblSetRowHeight (table, row, rowHeight);
	
	// Store the record number as the row id.
	TblSetRowID (table, row, recordNum);
	
	// Store the unique id of the record in the row.
	TblSetRowData (table, row, uniqueID);

	// Set the due date.
	TblSetItemInt (table, row, dateColumn, (*(int *) &expenseRec->date));

	// Set the expense type.
	ErrNonFatalDisplayIf(expenseRec->type >= numExpenseTypes && expenseRec->type != noExpenseType, 
		"invalid expense type");
	TblSetItemInt (table, row, typeColumn, expenseRec->type);

	// Set the currency
	if (expenseRec->type == expMileage)
		currency = currencyDistance;
	else
		currency = expenseRec->currency;
	TblSetItemInt (table, row, currencyColumn, currency);

	// Mark the row invalid so that it will draw when we call the 
	// draw routine.
	TblMarkRowInvalid (table, row);
	
	MemHandleUnlock (recordH);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewLoadTable
 *
 * DESCRIPTION: This routine reloads expense database records into
 *              the list view.  This routine is called when:
 *              	o A new item is inserted
 *              	o An item is deleted
 *              	o The date of an items is changed
 *              	o An item is marked complete
 *              	o Hidden items are shown
 *              	o Completed items are hidden
 *
 * PARAMETERS:  fillTable - if true the top visible item will be scroll winDown
 *                          such that a full table is displayed
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
*			jmp	9/29/99	Use FrmGetFormPtr() & FrmGetObjectIndex() instead of
 *								GetObjectPtr() because GetObjectPtr() calls FrmGetActiveForm(),
 *								and FrmGetActiveForm() may return a form that isn't the one we
 *								want when other forms are up when we are called.
 *								Fixes bug #22418.
 *
 ***********************************************************************/
static void ListViewLoadTable (Boolean fillTable)
{
	Int16 row;
	UInt16 height;
	UInt16 numRows;
	UInt16 recordNum;
	UInt16 lastRecordNum;
	UInt16 dataHeight;
	UInt16 tableHeight;
	UInt32	uniqueID;
	FontID curFont;
	FormPtr frm;
	TablePtr table;
	Boolean rowsInserted = false;
	RectangleType r;
	
	frm = FrmGetFormPtr (ListView);
	
	curFont = FntSetFont (expenseDescFont);

	// Make sure the global variable that hold the index of the 
	// first visible has a valid index.
	if (! SeekRecord (&TopVisibleRecord, 0, dmSeekForward))
		if (! SeekRecord (&TopVisibleRecord, 0, dmSeekBackward))
			TopVisibleRecord = 0;

	// If we have a currently selected record, make sure that it is not
	// above the first visible record.
	if (CurrentRecord != noRecordSelected)
		if (CurrentRecord < TopVisibleRecord)
			TopVisibleRecord = CurrentRecord;
		

	// Get the height of the table and the width of the description
	// column.
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ListTable));
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;

	row = 0;
	dataHeight = 0;
	recordNum = TopVisibleRecord;
	lastRecordNum = recordNum;

	// Load records into the table.
	while (true)
		{		
		// Get the next record in the currunt category.
		if ( ! SeekRecord (&recordNum, 0, dmSeekForward))
			break;

		// Compute the height of the expense item's description.
		height = FntLineHeight ();
		dataHeight += height;

		// Determine if the row needs to be initialized.  We will initialize 
		// the row if: the row is not usable (not displayed),  the unique
		// id of the record does not match the unique id stored in the 
		// row.
		DmRecordInfo (ExpenseDB, recordNum, NULL, &uniqueID, NULL);
		if ((! TblRowUsable (table, row)) ||
				(TblGetRowID (table, row) != recordNum) ||
				(TblGetRowData (table, row) != uniqueID))
			{
			ListViewInitTableRow (table, row, recordNum, height, uniqueID);
			}
		
		
//		// If the record is not already being displayed in the current 
//		// row load the record into the table.
//		if ((TblGetRowID (table, row) != recordNum) ||
//			 (! TblRowUsable (table, row)))
//			{
//			ListViewInitTableRow (table, row, recordNum, height);
//			}
//		else if (TblGetRowHeight (table, row) != height)
//			{
//			TblSetRowHeight (table, row, height);
//			TblMarkRowInvalid (table, row);
//			}

		lastRecordNum = recordNum;
		row++;
		recordNum++;

		// Is the table full?
		if (dataHeight >= tableHeight)		
			{
			// If we have a currently selected record, make sure that it is
			// not below  the last visible record.
			if ((CurrentRecord == noRecordSelected) ||
				 (CurrentRecord <= lastRecordNum)) break;

			TopVisibleRecord = recordNum;
			row = 0;
			dataHeight = 0;
			}
		}


	// Hide the item that don't have any data.
	numRows = TblGetNumberOfRows (table);
	while (row < numRows)
		{		
		TblSetRowUsable (table, row, false);
		row++;
		}
		
	// If the table is not full and the first visible record is 
	// not the first record	in the database, displays enough records
	// to fill out the table.
	while (dataHeight < tableHeight)
		{
		if (! fillTable) 
			break;
			
		recordNum = TopVisibleRecord;
		if ( ! SeekRecord (&recordNum, 1, dmSeekBackward))
			break;

		// Compute the height of the expense item's description.
		height = FntLineHeight ();
			
		// If adding the item to the table will overflow the height of
		// the table, don't add the item.
		if (dataHeight + height > tableHeight)
			break;
		
		// Insert a row before the first row.
		TblInsertRow (table, 0);

		DmRecordInfo (ExpenseDB, recordNum, NULL, &uniqueID, NULL);
		ListViewInitTableRow (table, 0, recordNum, height, uniqueID);
		
		TopVisibleRecord = recordNum;
		
		rowsInserted = true;

		dataHeight += height;
		}
		
	// If rows were inserted to full out the page, invalidate the whole
	// table, it all needs to be redrawn.
	if (rowsInserted)
		TblMarkTableInvalid (table);

	// Update the scroll arrows.
	ListViewUpdateScrollers (frm);

	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewLookupType
 *
 * DESCRIPTION: Search the expense type list for a match with the
 *              string in the auto-fill buffer.  If a match 
 *              is found, auto-fill the expense type. 
 *
 * PARAMETERS:  nothing
 *                		
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/5/96	Initial Revision
 *			kwk	07/04/99	Changed parameter to WChar (was Char).
 *
 ***********************************************************************/
static UInt16 ListViewLookupType (Boolean newSearch, WChar chr)
{
	UInt16 index = noExpenseType;
	ListPtr lst;
	Boolean unique;
	Char type;
	MemHandle recordH;
	ExpenseRecordType recordP;
	UInt8 * expenseTypeUnconvertP;


	// Is this the start of a new search (a new item).
	if (newSearch)
		{
		AutoFill = true;

		// if the "new" button was pressed then we won't have a key to search
		// on.
		if (chr == nullChr)
			{
			AutoFillLen = 0;
			AutoFillBuffer[0] = 0;
			return (index);
			}
		else
			{
			AutoFillLen = TxtSetNextChar(AutoFillBuffer, 0, chr);
			AutoFillBuffer[AutoFillLen] = '\0';
			}
		}

	else if (AutoFill)
		{
		if (chr == backspaceChr)
			{
			if (AutoFillLen)
				{
				AutoFillLen -= TxtPreviousCharSize(AutoFillBuffer, AutoFillLen);
				AutoFillBuffer[AutoFillLen] = '\0';
				}
			}
		else if (AutoFillLen + TxtCharSize(chr) <= maxAutoFillLen)
			{
			AutoFillLen += TxtSetNextChar(AutoFillBuffer, AutoFillLen, chr);
			AutoFillBuffer[AutoFillLen] = '\0';
			}
		else
			return (index);
		}

	else
		return (index);



	// Check for a match.
	lst = GetObjectPtr (ListExpenseTypeList);
	if (LookupStringInList (lst, AutoFillBuffer, &index, &unique))
		{
		// Need to convert index from popup list order to english expense type.
		expenseTypeUnconvertP = MemHandleLock (ExpenseTypeUnconvertH);
		index = expenseTypeUnconvertP[ index ];
		MemHandleUnlock (ExpenseTypeUnconvertH);

		type = index;
		ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expenseType, &type);
			
		// Mark the record dirty.
		DirtyRecord (CurrentRecord);
		}

	// If the character was a backspace and the auto fill buffer is empty
	// then set the expense type to "none".
	else if (chr == backspaceChr)
		{
		if (AutoFillLen == 0)
			{		
			type = noExpenseType;
			ExpenseChangeRecord (ExpenseDB, &CurrentRecord, expenseType, &type);

			// Mark the record dirty.
			DirtyRecord (CurrentRecord);
			}
		}

	// If we didn't get a match, remove the character from the search key.
	else
		{
		SndPlaySystemSound (sndError);

		AutoFillLen -= TxtPreviousCharSize(AutoFillBuffer, AutoFillLen);
		AutoFillBuffer[AutoFillLen] = '\0';

		ExpenseGetRecord (ExpenseDB, CurrentRecord, &recordP, &recordH);
		index = recordP.type;
		MemHandleUnlock (recordH);
		}
		
	
	return (index);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewNewExpense
 *
 * DESCRIPTION: This routine adds a new expense item to the expense list. 
 *              If a expense item is currently selected, the new items
 *              will be added before the selected item,  if not the
 *              new item will be appended to the end of the list.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	1/2/96	Initial Revision
 *			kwk	07/04/99	Changed parameter to WChar (was Char).
 *
 ***********************************************************************/
static void ListViewNewExpense (WChar chr)
{
	Err error;
	UInt16 attr;
	Int16 row;
	UInt16 recordNum;
	UInt16 category;
	TablePtr table;
	DateTimeType today;
	ExpenseRecordType newExpense;
	

	ListViewClearEditState ();

	MemSet (&newExpense, sizeof (newExpense), 0);

	// Insert a new expense record.
	TimSecondsToDateTime (TimGetSeconds(), &today);
	newExpense.date.year = today.year - firstYear;
	newExpense.date.month = today.month;
	newExpense.date.day = today.day;

	newExpense.type = noExpenseType;
	newExpense.paymentType = payUnfiled;
	newExpense.currency = DefaultCurrency;
	
	error = ExpenseNewRecord (ExpenseDB, &newExpense, &recordNum);

	// Display an alert that indicates that the new record could 
	// not be created.
	if (error)
		{
		FrmAlert (DeviceFullAlert);
		return;
		}

	// Set the category of the new record,  if we're showing all 
	// categories the new item will be uncatorized.
	if (CurrentCategory == dmAllCategories)
		category = dmUnfiledCategory;
	else
		category = CurrentCategory;

	DmRecordInfo (ExpenseDB, recordNum, &attr, NULL, NULL);
	attr &= ~dmRecAttrCategoryMask;
	attr |= category;
	DmSetRecordInfo (ExpenseDB, recordNum, &attr, NULL);

	CurrentRecord = recordNum;

	// If we're auto-filling the expense type, base on the first few 
	// character written, check for a match with the first character.
	ListViewLookupType (true, chr);


	table = GetObjectPtr (ListTable);
	ListViewLoadTable (true);
	TblRedrawTable (table);

	// Give the focus to the new item.
	TblFindRowID (table, CurrentRecord, &row);
	ListViewGrabFocus (row);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewAutoFill
 *
 * DESCRIPTION: Search the expense type list for a match with the
 *              string in the auto-fill buffer.  If a match 
 *              is found, auto-fill the expense type. 
 *
 * PARAMETERS:  nothing
 *                		
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/96	Initial Revision
 *			kwk	07/04/99	Change param from Char to WChar type.
 *			jmp	10/31/99	Move WinEraseRectangle() to ListViewDrawExpenseType().
 *
 ***********************************************************************/
static Boolean ListViewAutoFill (WChar chr)
{
	Int16 row;
	UInt16 type;
	UInt16 oldType;
	UInt16 currency;
	UInt16 recordNum;
	TablePtr table;
	Boolean redraw = false;
	Boolean handled = false;
	Boolean lookup;
	Boolean autoFillSaved;
	MemHandle recordH;
	RectangleType bounds;
	ExpenseRecordType recordP;

	// DOLATER Handle WChars in this routine. Currently auto-fill is always
	// disabled w/Japanese, thus this isn't an issue.
	
	if ((! AutoFill) || (! AllowQuickfill))
		return (false);

	table = GetObjectPtr (ListTable);
	if (TblFindRowID (table, CurrentRecord, &row))
		{
		if (TxtCharIsAlpha (chr))
			lookup = true;

		else if (chr == backspaceChr)
			lookup = (FldGetTextLength (TblGetCurrentField (table)) == 0);
			
		else
			lookup = false;
		

		if (lookup)
			{
			recordNum = CurrentRecord;
			// type and oldType are English-based expense type indexes
			oldType = TblGetItemInt (table, row, typeColumn);
			type = ListViewLookupType (false, chr);
			TblSetItemInt (table, row, typeColumn, type);
			redraw = (recordNum != CurrentRecord);
			handled = true;
			}
		else if ((TxtCharIsDigit (chr) || chr == DecimalSeperator) &&
					 AutoFillLen)
			{
			AutoFill = false;
			}

		// If the record moved, redraw the table.
		if (redraw)
			{
			autoFillSaved = AutoFill;
			ListViewSaveAmount (table);
			AutoFill = autoFillSaved;
			ListViewLoadTable (true);
			TblRedrawTable (table);
			ListRestoreEditState ();
			}
		else
			{
			// Redraw the expense type.
			TblGetItemBounds (table, row, typeColumn, &bounds);
			ListViewDrawExpenseType (table, row, typeColumn, &bounds);
			
			// If the expense type changed to or from mileage then redraw
			// the currency indicator.
			if (lookup && (oldType != type) && 
				(oldType == expMileage || type == expMileage))
				{
				if (type == expMileage)
					currency = currencyDistance;
				else
					{
					ExpenseGetRecord (ExpenseDB, CurrentRecord, &recordP, &recordH);
					currency = recordP.currency;
					MemHandleUnlock (recordH);
					}
				TblSetItemInt (table, row, currencyColumn, currency);

				TblGetItemBounds (table, row, currencyColumn, &bounds);
				WinEraseRectangle (&bounds, 0);
				ListViewDrawCurrency (table, row, currencyColumn, &bounds);
				}
			}
		}

	return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewDeleteExpense
 *
 * DESCRIPTION: This routine deletes the selected expense item.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewDeleteExpense (void)
{
	UInt16 i;
	Int16 row;
	Int16 column;
	UInt16 numRows;
	UInt16 recordNum;
	TablePtr table;
		
	// If no expense item is selected, return.
	table = GetObjectPtr (ListTable);

	// Check if we are editing an item.
	if (! TblEditing (table))
		return;

	TblGetSelection (table, &row, &column);
	TblReleaseFocus (table);

	// Check if the record is empty, if it is clear the edit state, 
	// this will delete the current record when its blank.
	recordNum = TblGetRowID (table, row);

	// Clear the edit state, this will delete the current record if is 
	// blank.
	if (ListViewClearEditState ())
		return;

	// Display an alert to comfirm the delete operation, and delete the
	// record if the alert is confirmed.
	if (! DeleteRecord (recordNum))
		return;

	// Invalid the row deleted and all the row following the deleted record.
	numRows = TblGetNumberOfRows (table);
	for (i = row; i < numRows; i++)
		TblSetRowUsable (table, i, false);

	ListViewLoadTable (true);
	TblRedrawTable (table);
	
	ItemSelected = false;
}


/***********************************************************************
 *
 * FUNCTION:    ListVewSelectDate
 *
 * DESCRIPTION: This routine is called when a date of an item in the 
 *              expense list is selected.  The date picker is displayed, if 
 *              the date of the item is changed, the record is updated
 *              and the re-sorted list is redrawn.
 *
 * PARAMETERS:  table -
 *              row   -
 *              
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListVewSelectDate (TablePtr table, Int16 row)
{
	UInt16 recordNum;
	UInt16 newRecordNum;
	DateType date;
	MemHandle recordH;
	ExpensePackedRecordPtr expenseRec;

	// Unhighlight the selected item.
	TblUnhighlightSelection (table);

	// Get the date from the expense record.
	recordNum = TblGetRowID (table, row);
	recordH = DmQueryRecord (ExpenseDB, recordNum);
	expenseRec = (ExpensePackedRecordPtr) MemHandleLock (recordH);
	date = expenseRec->date;
	MemHandleUnlock (recordH);

	DetermineDueDate (&date);

	// Update the database record.
	newRecordNum = recordNum;
	ExpenseChangeRecord (ExpenseDB, &newRecordNum, expenseDate, &date);

	// Changing the due date may change the record's index.
	CurrentRecord = newRecordNum;

	// Mark the record dirty.
	DirtyRecord (newRecordNum);

	// If the new position of the modified record is before the first
	// visible record or after the last visible record, redraw the 
	// table starting at the modified record.  Also, check if the 
	// index of the modified record  matches a record currently displayed,
	// if it does mark that record invalid so that it will be redrawn.
	if (newRecordNum < TopVisibleRecord)
		{
		TopVisibleRecord = newRecordNum;
		}

	else if (newRecordNum > TblGetRowID (table, TblGetLastUsableRow (table)))
		TopVisibleRecord = newRecordNum;


	// Always force the current row to be reinitialize.
	TblSetRowUsable (table, row, false);

	ListViewLoadTable (true);
	TblRedrawTable (table);
	ListRestoreEditState ();
}


/***********************************************************************
 *
 * FUNCTION:    ListVewSelectExpenseType
 *
 * DESCRIPTION: This routine is called when a expense type item in the 
 *              expense list is selected.  A popup list of expense types
 *              is displayed, if the type of an item is changed, the 
 *              record is updated.
 *
 * PARAMETERS:  table - expense table
 *              row   - row in the table
 *              
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/9/96	Initial Revision
 *			scl	5/12/97	noExpenseType no longer converted to translated index
 *
 ***********************************************************************/
static void ListVewSelectExpenseType (TablePtr table, Int16 row)
{

	int curType;
	int newType;
	Char type;
	UInt8 currency;
	UInt16 recordNum;
	ListPtr lst;
	FieldPtr fld;
	RectangleType r;
	UInt8 * expenseTypeConvertP;
	UInt8 * expenseTypeUnconvertP;


	lst = GetObjectPtr (ListExpenseTypeList);

	// Unhighlight the expense type.
	TblUnhighlightSelection (table);

	// Set the list's selection to the current expense.
	curType = TblGetItemInt (table, row, typeColumn);
	if (curType == noExpenseType) 
		{
		// if nothing selected, default to the first one in the list
		curType = 0;
		}
	else
		{
		// type is an English-based index.  Convert to translated
		// index since Popup list is in translated alphabetical order.
		expenseTypeConvertP = MemHandleLock (ExpenseTypeConvertH);
		curType = expenseTypeConvertP[curType];
		MemHandleUnlock (ExpenseTypeConvertH);
		}

	LstSetSelection (lst, curType);
	LstMakeItemVisible (lst, curType);
	
	// Position the list.
	TblGetItemBounds (table, row, typeColumn, &r);
	LstSetPosition (lst, r.topLeft.x, r.topLeft.y);
	

	// DOLATER zzz API routine.
	lst->attr.search = true;
	
	newType = LstPopupList (lst);

	// -1 indicates the popup list was dismissed without a selection being made.
	if (newType != -1)
		{
		// Need to convert newType from popup list order to english expense type.
		expenseTypeUnconvertP = MemHandleLock (ExpenseTypeUnconvertH);
		newType = expenseTypeUnconvertP[ newType ];
		MemHandleUnlock (ExpenseTypeUnconvertH);
		}

	// -1 indicates the popup list was dismissed without a selection being made.
	curType = TblGetItemInt (table, row, typeColumn);
	if ((newType == -1) || (newType == curType) )
		{
		ListRestoreEditState ();
		return;
		}
	
	// Update the database record.
	recordNum = TblGetRowID (table, row);
	type = newType;
	ExpenseChangeRecord (ExpenseDB, &recordNum, expenseType, &type);
	

	// If the expense type is mileage then reset the currency value.
	if (newType == expMileage)
		{
		currency = DefaultCurrency;
		ExpenseChangeRecord (ExpenseDB, &recordNum, expenseCurrency, &currency);
		}


	// Changing the due expense type may change the record's index.
	CurrentRecord = recordNum;

	// Mark the record dirty.
	DirtyRecord (recordNum);


	// If the new position of the modified record is before the first
	// visible record or after the last visible record, redraw the 
	// table starting at the modified record.  Also, check if the 
	// index of the modified record  matches a record currently displayed,
	// if it does mark that record invalid so that it will be redrawn.
	if (recordNum < TopVisibleRecord)
		{
		TopVisibleRecord = recordNum;
		}

	else if (recordNum > TblGetRowID (table, TblGetLastUsableRow (table)))
		TopVisibleRecord = recordNum;


	// Always force the current row to be reinitialize.
	TblSetRowUsable (table, row, false);

	ListViewLoadTable (true);
	TblRedrawTable (table);
	ListRestoreEditState ();


	// If the old or new expense type is mileage then mark the amount 
	// field dirty so the is will be reformatted with the correct number
	// of decimal positions when it is saved.
	if (curType == expMileage || newType == expMileage)
		{
		fld = TblGetCurrentField (table);
		if (fld)
			FldSetDirty (fld, true);
		}
}


/***********************************************************************
 *
 * FUNCTION:    ListVewSelectCurrency
 *
 * DESCRIPTION: This routine is called when a currency item in the 
 *              expense list is selected.  A popup list of currencies
 *              is displayed, if the currency of an item is changed, the 
 *              record is updated.
 *
 * PARAMETERS:  table - expensc table
 *              row   - row in the table
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	08/21/96	Initial Revision
 *			trm	07/22/97	Changed to truncate for 0 decimal place currencies.
 *			kwk	01/14/99	Really do currency formatting here.
 *
 ***********************************************************************/
static void ListVewSelectCurrency (TablePtr table, Int16 row)
{
	UInt16 curCurrency;
	UInt16 newCurrency;
	UInt8 currency;
	UInt16 recordNum;
	ListPtr lst;
	Boolean currencyEdited;
	RectangleType r;
	FieldPtr fld;

	// Get the list's selection to the current expense. If the expense type
	// is mileage the don't display the curreny list.
	curCurrency = TblGetItemInt (table, row, currencyColumn);
	if (curCurrency == currencyDistance)
		return;

	// Unhighlight the expense currency.
	TblUnhighlightSelection (table);


	// Position the list.
	TblGetItemBounds (table, row, currencyColumn, &r);
	lst = GetObjectPtr (ListCurrencyList);
	LstSetPosition (lst, r.topLeft.x, r.topLeft.y);
	
	newCurrency = curCurrency;
	currencyEdited = CurrencySelect (lst, &newCurrency, NULL);
	
	// DOLATER kwk - the currencyEdited flag will currently always be false. On the
	// other hand, it's unclear to me why you'd want to do anything special if
	// the user did edit the currencies, as you're not going to change the
	// currencies for any existing table items, thus no redrawing is required.
	
	// If nothing has changed then just restore the edit state.
	if (( ! currencyEdited) && (newCurrency == curCurrency) )
		{
		ListRestoreEditState ();
		return;
		}
	

	if (newCurrency != curCurrency)
		{
		// Update the database record.
		recordNum = TblGetRowID (table, row);
		currency = newCurrency;
		ExpenseChangeRecord (ExpenseDB, &recordNum, expenseCurrency, &currency);
		
		// Mark the record dirty.
		DirtyRecord (recordNum);
	
		// Mark the record invalid so that is will redraw.
		TblMarkRowInvalid (table, row);
	
		TblSetItemInt (table, row, currencyColumn, newCurrency);
		}
		

	// If the currency was edited then redraw the whole table, the curreny
	// sysmols may need redrawing.
	if (currencyEdited)
		TblMarkTableInvalid (table);

	TblRedrawTable (table);

	ListRestoreEditState ();
	fld = TblGetCurrentField (table);
	
	// If either the currency was edited or a new currency was selected,
	// we want to reformat the field (if it has any text) so that the
	// number of decimal points is correct for the current currency.
	if (((currencyEdited) || (newCurrency != curCurrency))
	&& (FldGetTextPtr(fld) != NULL))
		FormatAmount (fld, CurrencyGetDecimalPlaces (newCurrency));
	
	// Mark the amount field dirty so it is redrawn.
	FldSetDirty (fld, true);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewSelectAmount
 *
 * DESCRIPTION: This routine is called when the tableEnter event received
 *              within the amount column.  If the item that the pen 
 *              is on is not the item that is currently being edited, 
 *               a field obejct is initialized for editing the 
 *              amount text.
 *
 * PARAMETERS:	 pEvent  - a penDawn event
 *              table   - pointer to the expense table
 *              row     - row of the amount
 *
 * RETURNED:	 nothing
 *
 *	HISTORY:
 *		01/09/96	art	Created by Art Lamb.
 *		08/11/99	kwk	Set up new fieldTapState field in fldEnterEvent.
 *		09/12/99	gap	Update for new multi-tap implementation.
 *
 ***********************************************************************/
static void ListViewSelectAmount (EventType * event, TablePtr table, Int16 row)
{
	FieldPtr fld;
	EventType newEvent;
	RectangleType r;
	
	TblGetItemBounds (table, row, amountColumn, &r);

	// If we are not already editing this item, initialize the field
	// structure that's used for editing text item and highlight the edit
	// indication.
	if ((row != table->currentRow) || 
		 (amountColumn != table->currentColumn) ||
		 (! table->attr.editing) )
		{
		table->attr.editing = true;
		fld = TblGetCurrentField (table);

		ListViewInitAmount (table, fld, row, amountColumn, &r, true);

		table->currentRow = row;
		table->currentColumn = amountColumn;
		
		ListViewSelectEditIndicator (table, true);
		}

	// Convert the table enter event to a field enter event.
	fld = TblGetCurrentField (table);
	EvtCopyEvent (event, &newEvent);

	newEvent.eType = fldEnterEvent;
	newEvent.data.fldEnter.fieldID = fld->id;
	newEvent.data.fldEnter.pField = fld;

	FldHandleEvent (fld, &newEvent);
}

/***********************************************************************
 *
 * FUNCTION:    ListViewItemSelected
 *
 * DESCRIPTION: This routine is called when an item in the do to list
 *              is selected.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewItemSelected (EventType * event)
{
	Int16 row;
	Int16 column;
	TablePtr table;
	
	table = event->data.tblSelect.pTable;
	row = event->data.tblSelect.row;
	column = event->data.tblSelect.column;

//	ListViewSaveAmount (table);

	if (column == dateColumn)
		{
		ListVewSelectDate (table, row);
		}
	
	else if (column == typeColumn)
		{
		ListVewSelectExpenseType (table, row);
		}

	else if (column == currencyColumn)
		{
		ListVewSelectCurrency (table, row);
		}

}


/***********************************************************************
 *
 * FUNCTION:    ListViewSelectCategory
 *
 * DESCRIPTION: This routine handles selection, creation and deletion of
 *              categories form the Details Dialog.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    The index of the new category.
 *
 *              The following global variables are modified:
 *							CurrentCategory
 *							ShowAllCategories
 *							CategoryName
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	01/02/96	Initial Revision
 *			gap	08/13/99 Update to use new constant categoryDefaultEditCategoryString.
 *
 ***********************************************************************/
static UInt16 ListViewSelectCategory (void)
{
	FormPtr frm;
	TablePtr table;
	UInt16 category;
	Boolean categoryEdited;
	
	// Process the category popup list.  
	category = CurrentCategory;

	frm = FrmGetActiveForm();
	categoryEdited = CategorySelect (ExpenseDB, frm, ListCategoryTrigger,
					    ListCategoryList, true, &category, CategoryName, 1, categoryDefaultEditCategoryString);
	
	if (category == dmAllCategories)
		ShowAllCategories = true;
	else
		ShowAllCategories = false;
		
	if ( (categoryEdited) || (CurrentCategory != category))
		{
		ChangeCategory (category);

		// Display the new category.
		ListViewLoadTable (true);
		table = GetObjectPtr (ListTable);
		TblEraseTable (table);
		TblDrawTable (table);
		}

	return (category);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewNextCategory
 *
 * DESCRIPTION: This routine display the next category,  if the last
 *              catagory is being displayed  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 *              The following global variables are modified:
 *							CurrentCategory
 *							ShowAllCategories
 *							CategoryName
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewNextCategory (void)
{
	UInt16 category;
	FormPtr frm;
	TablePtr table;
	ControlPtr ctl;

	category = CategoryGetNext (ExpenseDB, CurrentCategory);

	if (category == dmAllCategories)
		ShowAllCategories = true;
	else
		ShowAllCategories = false;

	ChangeCategory (category);

	// Set the label of the category trigger.
	frm = FrmGetActiveForm ();
	ctl = GetObjectPtr (ListCategoryTrigger);
	CategoryGetName (ExpenseDB, CurrentCategory, CategoryName);
	CategorySetTriggerLabel (ctl, CategoryName);


	// Display the new category.
	ListViewLoadTable (true);
	table = GetObjectPtr (ListTable);
	TblEraseTable (table);
	TblDrawTable (table);
}

/***********************************************************************
 *
 * FUNCTION:    ListViewGotoAppointment
 *
 * DESCRIPTION: This routine set up the gloabal variable such the 
 *              list view will display text found by the text search
 *              command.
 *
 * PARAMETERS:  event - frmGotoEvent 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewGotoItem (EventType * event)
{
	ItemSelected = true;
	CurrentRecord = event->data.frmGoto.recordNum;
	TopVisibleRecord = CurrentRecord;
	ListEditPosition = event->data.frmGoto.matchPos;
	ListEditSelectionLength = event->data.frmGoto.matchLen;
}


/***********************************************************************
 *
 * FUNCTION:    ListViewScroll
 *
 * DESCRIPTION: This routine scrolls the list of of memo titles
 *              in the direction specified.
 *
 * PARAMETERS:  linesToScroll - number of lines to scroll
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/21/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewScroll (Int16 linesToScroll)
{
	Int16 				i;
	UInt16				rows;
	UInt16				lastRow;
	UInt16 				scrollAmount;
	UInt16 				newTopVisibleRecord;
	TablePtr 		table;
	RectangleType	scrollR;
	RectangleType	vacated;
	WinDirectionType	direction;


	table = GetObjectPtr (ListTable);
	CurrentRecord = noRecordSelected;


	// Find the new top visible record
	newTopVisibleRecord = TopVisibleRecord;

	// Scroll winDown.
	if (linesToScroll > 0)
		SeekRecord (&newTopVisibleRecord, linesToScroll, dmSeekForward);

	// Scroll winUp.
	else if (linesToScroll < 0)
		SeekRecord (&newTopVisibleRecord, -linesToScroll, dmSeekBackward);

	ErrFatalDisplayIf (TopVisibleRecord == newTopVisibleRecord, 
		"Invalid scroll value");

	TopVisibleRecord = newTopVisibleRecord;


	// Move the bits that will remain visible.
	rows = TblGetNumberOfRows (table);
	if (((linesToScroll > 0) && (linesToScroll < rows)) ||
		 ((linesToScroll < 0) && (-linesToScroll < rows)))
		{
		scrollAmount = 0;
	
		if (linesToScroll > 0)
			{
			lastRow = TblGetLastUsableRow (table) - 1;
			for (i = 0; i < linesToScroll; i++)
				{
				scrollAmount += TblGetRowHeight (table, lastRow);
				TblRemoveRow (table, 0);
				}
			direction = winUp;
			}
		else
			{
			for (i = 0; i < -linesToScroll; i++)
				{
				scrollAmount += TblGetRowHeight (table, 0);
				TblInsertRow (table, 0);
				}
			direction = winDown;
			}

		TblGetBounds (table, &scrollR);
		WinScrollRectangle (&scrollR, direction, scrollAmount, &vacated);
		WinEraseRectangle (&vacated, 0);
		}
	

	ListViewLoadTable (false);
	TblRedrawTable(table);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewPageScroll
 *
 * DESCRIPTION: This routine scrolls the list of of expense items
 *              in the direction specified.
 *
 * PARAMETERS:  direction - winUp or dowm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewPageScroll (WinDirectionType direction)
{
	UInt16				row;
	UInt16				index;
	UInt16				height;
	UInt16 				recordNum;
	UInt16 				tableHeight;
	FontID			curFont;
	TablePtr 		table;
	RectangleType	r;

	
	table = GetObjectPtr (ListTable);

	CurrentRecord = noRecordSelected;

	// Get the height of the table and the width of the description
	// column.
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;
	height = 0;

	// Scroll the table winDown.
	if (direction == winDown)
		{
		// Get the record index of the last visible record.  A row 
		// number of minus one indicates that there are no visible rows.
		row = TblGetLastUsableRow (table);
		if (row == -1) return;
		
		recordNum = TblGetRowID (table, row);				

		// If there is only one record visible, this is the case 
		// when a record occupies the whole screeen, move to the 
		// next record.
		if (row == 0)
			SeekRecord (&recordNum, 1, dmSeekForward);
		}

	// Scroll the table winUp.
	else
		{
		// Scan the records before the first visible record to determine 
		// how many record we need to scroll.  Since the heights of the 
		// records vary,  we sum the height of the records until we get
		// a screen full.
		recordNum = TblGetRowID (table, 0);
		height = TblGetRowHeight (table, 0);
		if (height >= tableHeight)
			height = 0;

		curFont = FntSetFont (expenseDescFont);
		while (height < tableHeight)
			{
			index = recordNum;
			if ( ! SeekRecord (&index, 1, dmSeekBackward) ) break;
			height += FntLineHeight ();
			if ((height <= tableHeight) || (recordNum == TblGetRowID (table, 0)))
				recordNum = index;
			}
		FntSetFont (curFont);
		}

	TblMarkTableInvalid (table);
	TopVisibleRecord = recordNum;
	ListViewLoadTable (true);	

	TblUnhighlightSelection (table);
	TblRedrawTable (table);
}



/***********************************************************************
 *
 * FUNCTION:    ListViewDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static Boolean ListViewDoCommand (UInt16 command)
{
	Boolean 	handled = true;

	switch (command)
		{
		case DeleteCmd:
			if (ItemSelected)
				ListViewDeleteExpense ();
			else
				FrmAlert (SelectItemAlert);
			break;

		case PurgeCmd:
			ListViewClearEditState ();
			FrmPopupForm (PurgeDialog);
			break;

		case PreferensesCmd:
			ListViewClearEditState ();
			FrmPopupForm (PreferDialog);
			break;

		case CustomCurrenciesCmd:
			ListViewClearEditState ();
			FrmPopupForm (CurrencyDialog);
			break;

		case AboutCmd:
			ListViewClearEditState ();
			AbtShowAbout (sysFileCExpense);
			break;	

		default:
			handled = false;
		}	
	return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewInit
 *
 * DESCRIPTION: This routine initializes the "List View" of the 
 *              Expense application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void ListViewInit (FormPtr frm)
{
	Int16 row;
	UInt16 width;
	Int16 rowsInTable;
	TablePtr table;
	ControlPtr ctl;
	RectangleType r;

	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ListTable));

	rowsInTable = TblGetNumberOfRows (table);
	for (row = 0; row < rowsInTable; row++)
		{
		TblSetItemStyle (table, row, dateColumn, customTableItem);
		TblSetItemStyle (table, row, typeColumn, customTableItem);
		TblSetItemStyle (table, row, currencyColumn, customTableItem);
		TblSetItemStyle (table, row, amountColumn, customTableItem);
		TblSetRowUsable (table, row, false);
		}


	TblSetColumnUsable (table, dateColumn, true);
	TblSetColumnUsable (table, typeColumn, true);
	TblSetColumnUsable (table, amountColumn, true);
	TblSetColumnUsable (table, currencyColumn, ShowCurrency);

	TblSetColumnSpacing (table, dateColumn, expenseListColumnSpacing);
	TblSetColumnSpacing (table, typeColumn, expenseListColumnSpacing);
	TblSetColumnSpacing (table, currencyColumn, expenseListColumnSpacing);
	TblSetColumnSpacing (table, amountColumn, 0);


	// Set the width of the expense type column.
	TblGetBounds (table, &r);
	width = r.extent.x;
	width -= TblGetColumnWidth (table, dateColumn) + 
				TblGetColumnSpacing (table, dateColumn) +
				TblGetColumnSpacing (table, typeColumn) +
				TblGetColumnWidth (table, amountColumn);
	if (ShowCurrency)
		width -= TblGetColumnWidth (table, currencyColumn) + 
				   TblGetColumnSpacing (table, currencyColumn);
	TblSetColumnWidth (table, typeColumn, width);	


	// Set the callback routine that draws the due date field.
	TblSetCustomDrawProcedure (table, dateColumn, ListViewDrawDate);
	TblSetCustomDrawProcedure (table, typeColumn, ListViewDrawExpenseType);
	TblSetCustomDrawProcedure (table, currencyColumn, ListViewDrawCurrency);
	TblSetCustomDrawProcedure (table, amountColumn, ListViewDrawAmount);
	
	ListViewLoadTable (true);

	// Set the label of the category trigger.
	ctl = GetObjectPtr (ListCategoryTrigger);
	CategoryGetName (ExpenseDB, CurrentCategory, CategoryName);
	CategorySetTriggerLabel (ctl, CategoryName);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewUpdateDisplay
 *
 * DESCRIPTION: This routine update the display of the view view
 *
 * PARAMETERS:  updateCode - a code that indicated what changes have been
 *                           made to the expense list.
 *                		
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *			jmp	11/02/99	Fixed problem on frmRedrawUpdateCode events when
 *								we're still in the edit state but we weren't redrawing
 *								the edit indicator.  Fixes ToDo part of bug #23235.
 *
 ***********************************************************************/
static Boolean ListViewUpdateDisplay (UInt16 updateCode)
{
	UInt16 attr;
	Int16 row;
	Int16 column;
	UInt16 numRows;
	UInt16 lastVisible;
	TablePtr table;
	ControlPtr ctl;
	
	table = GetObjectPtr (ListTable);

	// Was the UI unable to save an image of the day view when is 
	// obscured part of the day view with another dialog?  If not,
	// we'll handle the event here.
	if (updateCode & frmRedrawUpdateCode)
		{
		FormPtr frm = FrmGetActiveForm ();
		FrmDrawForm (frm);
		
		// If we're editing, then redraw the edit indicator as
		// selected.
		if (TblEditing(table))
			ListViewSelectEditIndicator (table, true);

		return (true);
		}

	// Were the display options modified (Show Options Dialog)?
	if (updateCode & updateDisplayOptsChanged)
		{
		TblEraseTable (table);
		ListViewInit (FrmGetActiveForm ());
		TblDrawTable (table);
		return (true);
		}

	// Was the category of an item changed?
	if (updateCode & updateCategoryChanged)
		{
		if (ShowAllCategories)
			CurrentCategory = dmAllCategories;
		else
			{
			DmRecordInfo (ExpenseDB, CurrentRecord, &attr, NULL, NULL);
			CurrentCategory = attr & dmRecAttrCategoryMask;
			}
		// Set the label of the category trigger.
		ctl = GetObjectPtr (ListCategoryTrigger);
		CategoryGetName (ExpenseDB, CurrentCategory, CategoryName);
		CategorySetTriggerLabel (ctl, CategoryName);

		TopVisibleRecord = CurrentRecord;
		}

	// Was an item deleted?  If so Invalid all the row 
	// following the delete/secret record.
	if (updateCode & updateItemDelete)
		{
		TblGetSelection (table, &row, &column);
		numRows = TblGetNumberOfRows (table);
		for ( ; row < numRows; row++)
			TblSetRowUsable (table, row, false);
		}

	// Was an item changed such that it needs to be redrawn?  If so 
	// Invalid the row of the item.
	if (updateCode & updateItemChanged)
		{
		TblGetSelection (table, &row, &column);
		numRows = TblGetNumberOfRows (table);
		for ( ; row < numRows; row++)
			TblSetRowUsable (table, row, false);
		}

	// Was the item move?  If so, make sure the record is still visible.
	// Items are moved when thier date is changed.  
	else if (updateCode & updateItemMove)
		{
		// Is the record is before the first visible record or after the 
		// last last visible record.
		lastVisible = TblGetLastUsableRow (table);

		if ((CurrentRecord < TopVisibleRecord) || (CurrentRecord > lastVisible))
			{
			TopVisibleRecord = CurrentRecord;
			TblMarkTableInvalid (table);
			}
		else
			{
			numRows = TblGetNumberOfRows (table);
			for (row = 0 ; row < numRows; row++)
				TblSetRowUsable (table, row, false);
			}
		}

	ListViewLoadTable (true);
	TblRedrawTable (table);
	
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewHandleKey
 *
 * DESCRIPTION: This routine handles key events in the expense list.
 *
 * PARAMETERS:  event  - a pointer to an keyEvnet.
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	01/12/96		Initial Revision
 *			trm	07/14/97		Modified to allow hard button Category changing 
 *			kwk	11/20/98		Return false if it's a non-handled cmd char.
 *			CS		06/22/99		Standardized keyDownEvent handling
 *									(TxtCharIsHardKey, commandKeyMask, etc.)
 *
 ***********************************************************************/
static Boolean ListViewHandleKey (EventType * event)
{
	WChar chr;
	Char * MemPtr;
	FieldPtr fld;
	TablePtr table;
	Boolean numeric;

	chr = event->data.keyDown.chr;

	// Is hard button pressed? If so, it must be the one assigned to us, so
	// use it to cycle through the categories.
	if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
		{
		ListViewClearEditState();
		ListViewNextCategory();
		return (true);
		}
	else if (EvtKeydownIsVirtual(event))
		{
		// Scroll winUp key presed?
		if (chr == vchrPageUp)
			{
			ListViewClearEditState ();
			ListViewPageScroll (winUp);
			return (true);
			}
	
		// Scroll winDown key presed?
		else if (chr == vchrPageDown)
			{
			ListViewClearEditState ();
			ListViewPageScroll (winDown);
			return (true);
			}
			
		return (false);
		}

	// If no item is selected, create a new expense item. If the character
	// is numeric pass it along to the field package.
	if (! ItemSelected)
		{
		if (! TxtCharIsPrint (chr))
			return (false);

		numeric = (TxtCharIsDigit (chr) || chr == DecimalSeperator);
		if (numeric)
			{		
			ListViewNewExpense (0);
			return (false);
			}
		else if (AllowQuickfill)
			{		
			ListViewNewExpense (event->data.keyDown.chr);
			return (true);
			}
		}

	// If the charcater was used by the auto-fill feature, we're done.
	if (ListViewAutoFill (event->data.keyDown.chr))
		return (true);


	// If we're here the character will be directed to the focus, which is 
	// the  amount field, we'll let the field package process valid, we
	// need to filter out invalid ones.
	if (TxtCharIsDigit (chr))
		return (false);

	// if the character is a decimal point make sure we don't already 
	// have one.
	if (chr == DecimalSeperator)
		{
		table = GetObjectPtr (ListTable);
		fld = TblGetCurrentField (table);
		if (! fld) return (true);
		
		MemPtr = FldGetTextPtr (fld);
		if (! MemPtr) return (false);
		
		return (StrChr (MemPtr, DecimalSeperator) != NULL);
		}
		
	if ((chr == backspaceChr) || (chr == leftArrowChr) || (chr == rightArrowChr))
		return (false);
		
	// The return character takes us out of edit mode.
	if (chr == linefeedChr)
		{
		ListViewClearEditState ();
		return (true);
		}

	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "List View"
 *              of the To Do application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	01/02/96		Initial Revision
 *			gap	10/28/99		Add command bar support.
 *			jmp	11/02/99		Update frmUpdateEvent code to not restore
 *									the edit state on frmRedrawUpdateCode events.
 *			gap	11/30/99		Updated command bar support.
 *			gap	11/02/99		Added undo icon to command bar for text insertion 
 *									as well as for text selection range.
 *
 ***********************************************************************/
static Boolean ListViewHandleEvent (EventType * event)
{
	Int16 row;
	FormPtr frm;
	TablePtr table;
	Boolean handled = false;
	

	if (event->eType == keyDownEvent)
		{
		handled = ListViewHandleKey (event);
		}
		

	else if (event->eType == penDownEvent)
		{
		handled = FrmHandleEvent (FrmGetActiveForm (), event);
		if (! handled)
			ListViewClearEditState ();
		}


	else if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case ListNewButton:
				ListViewNewExpense (0);
				handled = true;
				break;

			case ListDetailsButton:
				if (ItemSelected)
					{
					ListViewSaveAmount (GetObjectPtr (ListTable));
					FrmPopupForm (DetailsDialog);
					}
				else
					FrmAlert (SelectItemAlert);
				handled = true;
				break;

			case ListShowButton:
				FrmPopupForm (OptionsDialog);
				handled = true;
				break;
				
			case ListCategoryTrigger:
				ListViewSelectCategory ();
				handled = true;
				break;
			}
		}


	else if (event->eType == ctlEnterEvent)
		{
		switch (event->data.ctlEnter.controlID)
			{
			case ListCategoryTrigger:
			case ListShowButton:
				ListViewClearEditState ();
				break;

			case ListNewButton:
			case ListDetailsButton:
				if (ItemSelected)
					ListViewSaveAmount (GetObjectPtr (ListTable));
				break;
			}
		}

	else if (event->eType == ctlExitEvent)
		{
		switch (event->data.ctlEnter.controlID)
			{
			case ListNewButton:
			case ListDetailsButton:
				ListRestoreEditState ();
				break;
			}
		}


	else if (event->eType == tblSelectEvent)
		{
		ListViewItemSelected (event);
		handled = true;
		}
		

	else if (event->eType == tblEnterEvent)
		{
		table = event->data.tblEnter.pTable;
		if (ItemSelected)
			{
			if (TblFindRowID (table, CurrentRecord, &row))
				{
				if (event->data.tblEnter.row != row)
					handled = ListViewClearEditState ();
				else if (event->data.tblEnter.column != amountColumn)
					ListViewSaveAmount (table);
				}
			}

		if (! handled)
			{
			row = event->data.tblSelect.row;
			if (event->data.tblEnter.column == amountColumn)
				{
				ListViewSelectAmount (event, table, row);
				CurrentRecord = TblGetRowID (table, row);
				ItemSelected = true;
				handled = true;
				}
			else if (! ItemSelected)
				{
				ListViewGrabFocus (row);
				CurrentRecord = TblGetRowID (table, row);
				ItemSelected = true;
				handled = true;
				}
			else if (event->data.tblEnter.column == currencyColumn)
				{
				// If the expense type is mileage the don't display the curreny list.
				if (TblGetItemInt (table, row, currencyColumn) == currencyDistance)
					handled = true;
				}
			}
		}
		

	else if (event->eType == tblExitEvent)
		{
		ListViewClearEditState ();
		handled = true;
		}


	else if (event->eType == menuEvent)
		{
		handled = ListViewDoCommand (event->data.menu.itemID);
		}
		
	else if (event->eType == menuCmdBarOpenEvent)
		{
		if (CurrentRecord != noRecordSelected)
			{
			FieldPtr fld;
			UInt16 startPos, endPos;
			
			table = GetObjectPtr (ListTable);
			fld = TblGetCurrentField (table);
			if (fld)
				{
				FldGetSelection(fld, &startPos, &endPos);
				if (startPos != endPos)  // if there's some highlighted text
					{
					// (Note that we're adding buttons from right to left)
					MenuCmdBarAddButton(menuCmdBarOnLeft, BarPasteBitmap, menuCmdBarResultMenuItem, sysEditMenuPasteCmd, 0);
					MenuCmdBarAddButton(menuCmdBarOnLeft, BarCopyBitmap, menuCmdBarResultMenuItem, sysEditMenuCopyCmd, 0);
					MenuCmdBarAddButton(menuCmdBarOnLeft, BarCutBitmap, menuCmdBarResultMenuItem, sysEditMenuCutCmd, 0);
					MenuCmdBarAddButton(menuCmdBarOnLeft, BarUndoBitmap, menuCmdBarResultMenuItem, sysEditMenuUndoCmd, 0);
					}
				else 	// there's no highlighted text, but an item is chosen
					{
					MenuCmdBarAddButton(menuCmdBarOnLeft, BarDeleteBitmap, menuCmdBarResultMenuItem, DeleteCmd, 0);
					MenuCmdBarAddButton(menuCmdBarOnLeft, BarPasteBitmap, menuCmdBarResultMenuItem, sysEditMenuPasteCmd, 0);
					MenuCmdBarAddButton(menuCmdBarOnLeft, BarUndoBitmap, menuCmdBarResultMenuItem, sysEditMenuUndoCmd, 0);
					}
					
				// tell the field package to not add cut/copy/paste buttons automatically
				event->data.menuCmdBarOpen.preventFieldButtons = true;
				}
			}

		// don't set handled to true; this event must fall through to the system.
		}
	
	else if (event->eType == sclRepeatEvent)
		{
		if (! ListViewClearEditState ())
			ListViewScroll (event->data.sclRepeat.newValue - 
				event->data.sclRepeat.value);
		}


	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		ListViewInit (frm);
		if (PendingUpdate)
			{
			ListViewUpdateDisplay (PendingUpdate);
			PendingUpdate = 0;
			}
		FrmDrawForm (frm);
		ListRestoreEditState ();
		handled = true;
		}


	else if (event->eType == frmGotoEvent)
		{
		frm = FrmGetActiveForm ();
		ListViewGotoItem (event);
		ListViewInit (frm);
		FrmDrawForm (frm);
		ListRestoreEditState ();
		handled = true;
		}


	else if (event->eType == frmUpdateEvent)
		{
		handled = ListViewUpdateDisplay (event->data.frmUpdate.updateCode);
		if (handled && (event->data.frmUpdate.updateCode != frmRedrawUpdateCode))
			ListRestoreEditState ();
		}


	else if (event->eType == frmSaveEvent)
		{
		ListViewClearEditState ();
		}

	return (handled);
}


#pragma mark -
/***********************************************************************
 *
 * FUNCTION:    ApplicationHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *			jmp	9/17/99	Use NewNoteView instead of NoteView.
 *
 ***********************************************************************/
static Boolean ApplicationHandleEvent (EventType * event)
{
	UInt16 formID;
	FormPtr frm;

	if (event->eType == frmLoadEvent)
		{
		// Load the form resource.
		formID = event->data.frmLoad.formID;
		frm = FrmInitForm (formID);
		FrmSetActiveForm (frm);		
		
		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formID)
			{
			case ListView:
				FrmSetEventHandler (frm, ListViewHandleEvent);
				break;
		
			case NewNoteView:
				FrmSetEventHandler (frm, NoteViewHandleEvent);
				break;
		
			case DetailsDialog:
				FrmSetEventHandler (frm, DetailsHandleEvent);
				break;

			case OptionsDialog:
				FrmSetEventHandler (frm, OptionsHandleEvent);
				break;

			case PurgeDialog:
				FrmSetEventHandler (frm, PurgeHandleEvent);
				break;

			case AttendeesDialog:
				FrmSetEventHandler (frm, AttendeesHandleEvent);
				break;
				
			case PreferDialog:
				FrmSetEventHandler (frm, PreferHandleEvent);
				break;

			case CurrencyDialog:
				FrmSetEventHandler (frm, CurrencyHandleEvent);
				break;

			case CurrencyPropDialog:
				FrmSetEventHandler (frm, CurrencyPropHandleEvent);
				break;

			case CurrencySelectDialog:
				FrmSetEventHandler (frm, CurrencySelectHandleEvent);
				break;
			}

		return (true);
		}
	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    EventLoop
 *
 * DESCRIPTION: This routine is the event loop for the Expense
 *              aplication.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
static void EventLoop (void)
{
	UInt16 error;
	EventType event;

	do
		{
		EvtGetEvent (&event, evtWaitForever);

		if (! SysHandleEvent (&event))
		
			if (! MenuHandleEvent (NULL, &event, &error))
			
				if (! ApplicationHandleEvent (&event))
	
					FrmDispatchEvent (&event); 
		}

	while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:    TDMain
 *
 * DESCRIPTION: This is the main entry point for the Expense
 *              application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/2/96	Initial Revision
 *
 ***********************************************************************/
UInt32	PilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	UInt16 error;

	error = RomVersionCompatible (version20, launchFlags);
	if (error) return (error);


	// Normal Launch
	if (cmd == sysAppLaunchCmdNormalLaunch)
		{
		error = StartApplication ();
		if (error) return (error);

		FrmGotoForm (ListView);
		EventLoop ();

		StopApplication ();
		}
	
	else if (cmd == sysAppLaunchCmdFind)
		{
		Search ((FindParamsPtr)cmdPBP);
		}
	
	
	// This action code might be sent to the app when it's already running
	//  if the use hits the Find soft key next to the Graffiti area.
	else if (cmd == sysAppLaunchCmdGoTo)
		{
		Boolean	launched;
		launched = launchFlags & sysAppLaunchFlagNewGlobals;
		
		if (launched) 
			{
			error = StartApplication ();
			if (error) return (error);

			GoToItem ((GoToParamsPtr) cmdPBP, launched);
			
			EventLoop ();
			StopApplication ();	
			}
		else
			GoToItem ((GoToParamsPtr) cmdPBP, launched);
		}
			

	else if (cmd == sysAppLaunchCmdSyncNotify)
		{
		SyncNotification ();
		}
	

	// This action code is sent after the system is reset. We use this time
	//  to create our default database if this is a hard reset
	else if (cmd == sysAppLaunchCmdSystemReset) {
		if (((SysAppLaunchCmdSystemResetType*)cmdPBP)->createDefaultDB) {
			MemHandle	resH;
			resH = DmGet1Resource(sysResTDefaultDB, sysResIDDefaultDB);
			if (resH) {
				DmCreateDatabaseFromImage(MemHandleLock(resH));
				MemHandleUnlock(resH);
				DmReleaseResource(resH);
				
				// set the backup bit on the new DB
				SetDBAttrBits(NULL, dmHdrAttrBackup);
				}
			}
		}
	

	// This action code is sent by the DesktopLink server when it create 
	// a new database.  We will initializes the new database.
	else if (cmd == sysAppLaunchCmdInitDatabase)
		{
		ExpenseAppInfoInit (((SysAppLaunchCmdInitDatabaseType*)cmdPBP)->dbP);

		// Set the backup bit.  This is to aid syncs with non Palm software.
		SetDBAttrBits(((SysAppLaunchCmdInitDatabaseType*)cmdPBP)->dbP, dmHdrAttrBackup);
		}


	return (0);
}

/***********************************************************************
 *
 * FUNCTION:    ExpenseLoadPrefs
 *
 * DESCRIPTION: Load the preferences and deal with older/newer versions.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 *	HISTORY:
 *		01/09/98	BGT	Initial Revision
 *		04/29/99	grant	Changed prefs version checking.  Add Euro sign
 *							to currency list from old versions of prefs.
 *		11/16/99	kwk	Disable auto-fill if language == Japanese.
 *
 ***********************************************************************/
void ExpenseLoadPrefs(void)
{
	UInt16 prefsSize;
	ExpensePreferenceType prefs;
	Int16 prefsVersion;
	Int16 i;
	UInt32 attributes;
	
	prefsSize = sizeof (ExpensePreferenceType);
	prefsVersion = PrefGetAppPreferences (sysFileCExpense, expensePrefID, &prefs, &prefsSize, 
		true);
	if (prefsVersion > expensePrefsVersionNum)
		{
		prefsVersion = noPreferenceFound;
		}
		
	if (prefsVersion != noPreferenceFound)
		{
		if (prefsVersion < expensePrefsVersion3)
			{
			prefs.noteFont = prefs.v20NoteFont;
			}
		
		if (prefsVersion < expensePrefsVersion4)
			{
			// add the euro to the currency list if there is an open spot
			for (i = 0; i < numCurrenciesDisplayed; i++)
				{
				if (prefs.currencies[i] == currencyNone)
					{
					prefs.currencies[i] = currencyCustomAppEuro;
					break;
					}
				}
			}
		
		CurrentCategory = prefs.currentCategory;
		NoteFont = prefs.noteFont;
		ShowAllCategories = prefs.showAllCategories;
		ShowCurrency = prefs.showCurrency;
		SaveBackup = prefs.saveBackup;
		AllowQuickfill = prefs.allowQuickfill;
		DefaultCurrency = prefs.defaultCurrency;
		UnitOfDistance = prefs.unitOfDistance;
		
		MemMove (Currencies, prefs.currencies, sizeof(Currencies));
		}
	else
		{
		DefaultCurrency = HomeCountry;
		NoteFont = FntGlueGetDefaultFontID(defaultSystemFont);
		
		if (PrefGetPreference(prefMeasurementSystem) == unitsMetric)
			{
			UnitOfDistance = distanceInKilometers;
			}
		else
			{
			UnitOfDistance = distanceInMiles;
			}
		}
	
	// Japanese doesn't support the auto-fill feature, so we'll disable
	// it here for now.
	if ((FtrGet(sysFtrCreator, sysFtrNumLanguage, &attributes) == errNone)
	&& (attributes == lJapanese))
		{
		// The Japanese Prefs form has this checkbox disabled, so the
		// user can't ever change the value to true.
		AllowQuickfill = false;
		}
}

/***********************************************************************
 *
 * FUNCTION:    ExpenseSavePrefs
 *
 * DESCRIPTION: Save the application preferences and ensure backward
 *					 compatibility
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			BGT	1/9/98	Initial Revision
 *			grant	4/29/99	Replaced expenseVersionNum with expensePrefsVersionNum
 *
 ***********************************************************************/
void ExpenseSavePrefs(void)
{
	ExpensePreferenceType prefs;

	prefs.currentCategory = CurrentCategory;
	prefs.noteFont = NoteFont;
	
	// If we're using a font that existed on older roms, set the old note font
	// pref to it, otherwise for safety use a default font.
	if (prefs.noteFont <= largeFont) {
		prefs.v20NoteFont = prefs.noteFont;
	}
	else {
		prefs.v20NoteFont = stdFont;
	}
	
	prefs.showAllCategories = ShowAllCategories;
	prefs.showCurrency = ShowCurrency;
	prefs.saveBackup = SaveBackup;
	prefs.allowQuickfill = AllowQuickfill;
	prefs.defaultCurrency = DefaultCurrency;
	prefs.unitOfDistance = UnitOfDistance;

	MemMove (prefs.currencies, Currencies, sizeof(Currencies));		

	// Write the state information.
	PrefSetAppPreferences (sysFileCExpense, expensePrefID, expensePrefsVersionNum, 
		&prefs, sizeof (ExpensePreferenceType), true);
}


/***********************************************************************
 *
 * FUNCTION:     SetDBAttrBits
 *
 * DESCRIPTION:  This routine sets the backup bit on the given database.
 *					  This is to aid syncs with non Palm software.
 *					  If no DB is given, open the app's default database and set
 *					  the backup bit on it.
 *
 * PARAMETERS:   dbP -	the database to set backup bit,
 *								can be NULL to indicate app's default database
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	4/1/99	Initial Revision
 *
 ***********************************************************************/
void SetDBAttrBits(DmOpenRef dbP, UInt16 attrBits)
{
	DmOpenRef localDBP;
	LocalID dbID;
	UInt16 cardNo;
	UInt16 attributes;

	// Open database if necessary. If it doesn't exist, simply exit (don't create it).
	if (dbP == NULL)
		{
		localDBP = DmOpenDatabaseByTypeCreator (expenseDBType, sysFileCExpense, dmModeReadWrite);
		if (localDBP == NULL)  return;
		}
	else
		{
		localDBP = dbP;
		}
	
	// now set the backup bit on localDBP
	DmOpenDatabaseInfo(localDBP, &dbID, NULL, NULL, &cardNo, NULL);
	DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	attributes |= attrBits;
	DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	
	// close database if necessary
   if (dbP == NULL) 
   	{
   	DmCloseDatabase(localDBP);
      }
}

