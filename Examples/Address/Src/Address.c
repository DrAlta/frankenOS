/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Address.c
 *
 * Description:
 *   This is the Address Book application's main module.  This module
 *   starts the application, dispatches events, and stops
 *   the application. 
 *
 * History:
 *		Feb 13, 1995	Created by Art Lamb
 *
 *****************************************************************************/

#include <PalmOS.h>

#include "Address.h"
#include "AddressAutoFill.h"
#include "AddressRsc.h"

/***********************************************************************
 *
 *   Entry Points
 *
 ***********************************************************************/
UInt32   PilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags);


/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/

typedef struct {
	UInt16			currentCategory;
	FontID			v20NoteFont;				// For 2.0 compatibility (BGT)
	Boolean			showAllCategories;
	Boolean			saveBackup;
	Boolean			rememberLastCategory;
	
	// Version 3 preferences
	FontID			addrListFont;
	FontID			addrRecordFont;
	FontID			addrEditFont;
	UInt8 			reserved1;
	UInt32			businessCardRecordID;
	FontID			noteFont;
	UInt8 			reserved2;
} AddrPreferenceType;


// Info on how to draw the record view
typedef struct {
	UInt16			fieldNum;
	UInt16			length;
	UInt16			offset;
	UInt16			x;
} RecordViewLineType;


typedef struct {
	DmOpenRef		db;
	Char *			categoryName;
	UInt16			categoryIndex;
} AcceptBeamType;



/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/

static DmOpenRef			AddrDB = NULL;
static Char					CategoryName [dmCategoryLength];
static UInt16				AddressInCategory;
static privateRecordViewEnum		PrivateRecordVisualStatus;
static UInt16           TopVisibleRecord = 0;
static UInt16           CurrentRecord = noRecord;
static UInt16           CurrentView = ListView;
static UInt16           ListViewSelectThisRecord = noRecord;		// This must
                       								// be set whenever we leave a
                        							// dialog because a frmSaveEvent
                        							// happens whenever the focus is
                        							// lost in the EditView and then
                        							// a find and goto can happen
                        							// causing a wrong selection to
                        							// be used.
Boolean						SortByCompany;
const Int16					PhoneColumnWidth = 82; // (415)-000-0000x...
static AddrDBRecordType recordViewRecord;
static MemHandle			recordViewRecordH = 0;
static RecordViewLineType *RecordViewLines;
static UInt16				RecordViewLastLine;   // Line after last one containing data
static UInt16				TopRecordViewLine;
static UInt16				RecordViewFirstPlainLine;
static UInt16				TopVisibleFieldIndex;
static UInt16				CurrentFieldIndex;
static UInt16				EditRowIDWhichHadFocus;
static UInt16				EditFieldPosition;
static UInt16				PriorAddressFormID;   // Used for NoteView
static UInt16				EditLabelColumnWidth = 0;
static UInt16				RecordLabelColumnWidth = 0;
static Char *				UnnamedRecordStringPtr = 0;
static Boolean				RecordNeededAfterEditView;
static UInt32				TickAppButtonPushed = 0;
static UInt16				AppButtonPushed = nullChr;
static UInt16				AppButtonPushedModifiers = 0;
static Boolean				BusinessCardSentForThisButtonPress = false;

// The following global variable are saved to a state file.
static UInt16				CurrentCategory = dmAllCategories;
static Boolean				ShowAllCategories = true;
static Boolean				SaveBackup = true;
static Boolean				RememberLastCategory = false;
static FontID				NoteFont = stdFont;
static FontID				AddrListFont = stdFont;
static FontID				AddrRecordFont = largeBoldFont;
static FontID				AddrEditFont = largeBoldFont;
static UInt32				BusinessCardRecordID = dmUnusedRecordID;

// These are used for accelerated scrolling
static UInt16 				LastSeconds = 0;
static UInt16 				ScrollUnits = 0;

// These are used for controlling the display of the duplicated address records.
static UInt16				NumCharsToHilite = 0;

// The following structure maps row in the edit table to fields in the
// address record.  This controls the order in which fields are edited.
// Valid after EditViewInit.
static const AddressFields* FieldMap;

// Valid after StartApplication
static Char PhoneLabelLetters[numPhoneLabels];

// Valid after EditViewInit
static Char * EditPhoneListChoices[numPhoneLabels];

// Valid after DetailsDialogInit
static Char * DetailsPhoneListChoices[numPhoneFields];




/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
// Address list table columns
#define nameAndNumColumn               	0
#define noteColumn                     	1

// Address edit table's rows and columns
#define editLabelColumn                   0
#define editDataColumn                    1

#define spaceBeforeDesc                   2

#define editLastFieldIndex                17
#define editFirstFieldIndex            	0

// AutoFill database types and names
// Note that we prefix with "Address" to avoid name conflicts with Expense app
#define titleDBType 						'titl'
#define titleDBName						"AddressTitlesDB"

#define companyDBType					'cmpy'
#define companyDBName					"AddressCompaniesDB"

#define cityDBType						'city'
#define cityDBName						"AddressCitiesDB"

#define stateDBType						'stat'
#define stateDBName						"AddressStatesDB"

#define countryDBType					'cnty'
#define countryDBName					"AddressCountriesDB"

// Update codes, used to determine how the address list view should 
// be redrawn.
#define updateRedrawAll                   0x01
#define updateGrabFocus							0x02
#define updateItemHide							0x04
#define updateCategoryChanged					0x08
#define updateFontChanged						0x10
#define updateListViewPhoneChanged        0x20
#define updateCustomFieldLabelChanged     0x40
#define updateSelectCurrentRecord      	0x80

#define addrEditLabelFont						stdFont
#define addrEditBlankFont						stdFont

#define maxNameLength						255

// number of record view lines to store
#define recordViewLinesMax                55
#define recordViewBlankLine            	0xffff   // Half height if the next line.x == 0


// Scroll rate values
#define scrollDelay				2
#define scrollAcceleration		2
#define scrollSpeedLimit		5

#define maxDuplicatedIndString	20

#define noFieldIndex			0xff

// Time to depress the app's button to send a business card
#define AppButtonPushTimeout					(sysTicksPerSecond)

// Maximum label column width in Edit and Record views.
//  (Would be nice if this was based on window size or screen size, do 1/2 screen for now)
#define maxLabelColumnWidth		80	

// Resource type used to specify order of fields in Edit view.
#define	fieldMapRscType	'fmap'

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/
#define isPhoneField(f)      (f >= firstPhoneField && f <= lastPhoneField)

static Boolean PhoneIsANumber( Char* phone );

static void DrawRecordNameAndPhoneNumber (AddrDBRecordPtr record, 
	RectanglePtr bounds, Char * phoneLabelLetters, Boolean sortByCompany,
	Char **unnamedRecordStringPtr);

static void DeleteNote (void);
static void ListViewSelectRecord (UInt16 recordNum);
static void ListViewScroll (WinDirectionType direction, UInt16 units, Boolean byLine);

static void EditViewLoadTable (void);
static void EditViewInit (FormPtr frm, Boolean leaveDataLocked);

static Boolean AddrSendBusinessCard (DmOpenRef dbP);

static UInt32 AddrPilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags);

// Scroll rate prototypes
static void ResetScrollRate(void);
static void AdjustScrollRate(void);
static void AddressLoadPrefs(AddrAppInfoPtr	appInfoPtr);	// (BGT)
static void AddressSavePrefs(void);							// (BGT)
static UInt16 DuplicateCurrentRecord (UInt16 *numCharsToHilite, Boolean deleteCurrentRecord);
static char * GetStringResource (UInt16 stringResource, char * stringP);



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
 *			bhall	7/9/99	made non-static for access in AddressAutoFill.c
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
		localDBP = DmOpenDatabaseByTypeCreator (addrDBType, sysFileCAddress, dmModeReadWrite);
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


/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  This routine opens the application's resource file and
 *               database.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err - standard error code
 *
 * REVISION HISTORY:
 * 			Name	Date		Description
 * 			----	----		-----------
 * 			art	6/5/95	Initial Revision
 * 			vmk	12/12/97	Get amplitude for scroll sound
 *				BGT	1/8/98	Use AddressLoadPrefs to load and fix up the
 *									application preferences
 *				grant	4/6/99	Moved code to set backup bit into SetDBAttrBits.
 *				jmp	10/1/99	Call new AddrGetDataBase() to create database
 *									if it doesn't already exist.
 *
 ***********************************************************************/
static Err StartApplication(void)
{
	Err err = 0;
	UInt16 mode;
	AddrAppInfoPtr appInfoPtr;

   
	// Determime if secret records should be shown.
	PrivateRecordVisualStatus = (privateRecordViewEnum)PrefGetPreference(prefShowPrivateRecords);
	mode = (PrivateRecordVisualStatus == hidePrivateRecords) ?
					dmModeReadWrite : (dmModeReadWrite | dmModeShowSecret);


   // Find the application's data file.  If it doesn't exist create it.
	err = AddrGetDatabase(&AddrDB, mode);
   if (err)
   	return err;


	appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);
	ErrFatalDisplayIf(appInfoPtr == NULL, "Missing app info block");
   
   // Update the database to look and behave properly for the given country.
	if (appInfoPtr->country != PrefGetPreference(prefCountry))
   	AddrChangeCountry(appInfoPtr);


	InitPhoneLabelLetters(appInfoPtr, PhoneLabelLetters);
      
	SortByCompany = appInfoPtr->misc.sortByCompany;
	
	// Load the application preferences and fix them up if need be.	(BGT)
	AddressLoadPrefs(appInfoPtr);							// (BGT)

	// Initialize the default auto-fill databases
	AutoFillInitDB(titleDBType, sysFileCAddress, titleDBName, titleAFInitStr);
	AutoFillInitDB(companyDBType, sysFileCAddress, companyDBName, companyAFInitStr);
	AutoFillInitDB(cityDBType, sysFileCAddress, cityDBName, cityAFInitStr);
	AutoFillInitDB(stateDBType, sysFileCAddress, stateDBName, stateAFInitStr);
	AutoFillInitDB(countryDBType, sysFileCAddress, countryDBName, countryAFInitStr);

	// Start watching the button pressed to get into this app.  If it's held down
	// long enough then we need to send the business card.
	TickAppButtonPushed = TimGetTicks();
	
	// Mask off the key to avoid repeat keys causing clicking sounds
	KeySetMask(~KeyCurrentState());
	
	return (err);
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
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
static void StopApplication(void)
{
	// Write the preferences / saved-state information.
	AddressSavePrefs();

	// Send a frmSave event to all the open forms.
	FrmSaveAllForms ();

	// Close all the open forms.
	FrmCloseAllForms ();

	// Close the application's data file.
	DmCloseDatabase (AddrDB);

}


/***********************************************************************
 *
 * FUNCTION:    AppHandleSync
 *
 * DESCRIPTION: MemHandle details after the database has been synchronized.
 * This app resorts the database. 
 *
 * PARAMETERS:    findParams
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name   Date			Description
 *			----   ----			-----------
 *			roger	8/31/95		Initial Revision
 *			vmk	10/17/95		Changed to open db read/write
 *			jmp	10/01/99		Changed call to DmOpenDatabaseByTypeCreator() to
 *									AddrGetDatabase().
 *
 ***********************************************************************/
static void AppHandleSync(void)
{
	DmOpenRef dbP;
	AddrAppInfoPtr appInfoPtr;
	Err err;
//   char name [dmCategoryLength];
//   AddrPreferenceType prefs;


   // Find the application's data file.
	err = AddrGetDatabase (&dbP, dmModeReadWrite);
	if (err) 
      return;
   
   // If we have state information insure it's still valid.
/*   if ( PrefGetAppPreferences (sysFileCAddress, addrVersionNum, &prefs, 
      sizeof (AddrPreferenceType))){

      // Check if the currrent category still exist.
      CategoryGetName (dbP, prefs.currentCategory, name);   
      if (*name == 0){
         prefs.currentCategory = dmAllCategories;
         prefs.showAllCategories = true;
   
         PrefSetAppPreferences (sysFileCAddress, addrVersionNum, &prefs, 
            sizeof (AddrPreferenceType));
      }
   }
*/

	appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(dbP);
   
   AddrChangeSortOrder(dbP, appInfoPtr->misc.sortByCompany);

   MemPtrUnlock(appInfoPtr);

   DmCloseDatabase (dbP);   
}


/***********************************************************************
 *
 * FUNCTION:    AppLaunchCmdDatabaseInit
 *
 * DESCRIPTION: Initialize an empty database.
 *
 * PARAMETERS:    dbP - pointer to database opened for read & write
 *
 * RETURNED:    true if successful
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   10/19/95   Initial Revision
 *
 ***********************************************************************/
static Boolean AppLaunchCmdDatabaseInit(DmOpenRef dbP)
{
   Err err;
   if (!dbP) 
      return false;

	// Set the backup bit.  This is to aid syncs with non Palm software.
	SetDBAttrBits(dbP, dmHdrAttrBackup);

   // Initialize the database's app info block
   err = AddrAppInfoInit (dbP);
   if (err) 
      return false;
   return true;
}


/***********************************************************************
 *
 * FUNCTION:    AppValidateDataFiles
 *
 * DESCRIPTION: Validate all of the application's data files for
 * integrity.  Useful after a crash has occured. 
 *
 * Currently unused.
 *
 * PARAMETERS:    nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   9/1/95   Initial Revision
 *
 ***********************************************************************/
static void AppValidateDataFiles ()
{
   LocalID dbID;
   DmOpenRef dbP;
   AddrAppInfoPtr appInfoPtr;
   UInt16      cardNo=0;
/*   AddrDBRecordType record;
   UInt16 i;
   UInt16 pos;
   Char * header;
   UInt16 recordNum;
   MemHandle recordH;
   RectangleType r;
   Boolean done;
   char searchPhoneLabelLetters[numPhoneLabels];
*/

   // Open the record database.
   dbID = DmFindDatabase (cardNo, addrDBName);
   if (! dbID)
      return;
      
   dbP = DmOpenDatabase(cardNo, dbID, dmModeReadWrite | dmModeExclusive);
   if (! dbP)
      return;

   appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(dbP);
   
   AddrChangeSortOrder(dbP, appInfoPtr->misc.sortByCompany);

   MemPtrUnlock(appInfoPtr);
   DmCloseDatabase (dbP);   
}


/***********************************************************************
 *
 * FUNCTION:    Search
 *
 * DESCRIPTION: This routine searchs the the address database for records 
 *              contains the string passed. 
 *
 * PARAMETERS:  findParams
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 * 			Name	Date		Description
 *				----	----		-----------
 *				art	6/5/95   Initial Revision
 *				jmp	10/21/99	Changed params to findParams to match other routines
 *									like this one.
 *
 ***********************************************************************/
static void Search(FindParamsPtr findParams)
{
   AddrAppInfoPtr       appInfoPtr;
   AddrDBRecordType     record;
   Boolean              done;
   Boolean              match;
   char                 searchPhoneLabelLetters[numPhoneLabels];
   Char *               header;
   Char *               unnamedRecordStringPtr = NULL;
   DmOpenRef            dbP;
   DmSearchStateType    searchState;
   Err                  err;
   MemHandle            headerStringH;
   LocalID              dbID;
   RectangleType        r;
   UInt16               cardNo=0;
   UInt16               recordNum;
   MemHandle            recordH;
   UInt16               i;
   UInt16               pos;

   // Find the application's data file.
   err = DmGetNextDatabaseByTypeCreator (true, &searchState, addrDBType,
               sysFileCAddress, true, &cardNo, &dbID);
   if (err)
   	{
      findParams->more = false;
      return;
      }

   // Open the address database.
   dbP = DmOpenDatabase(cardNo, dbID, findParams->dbAccesMode);
   if (!dbP)
   	{
      findParams->more = false;
      return;
   	}

   // Display the heading line.
   headerStringH = DmGetResource(strRsc, FindAddrHeaderStr);
   header = MemHandleLock(headerStringH);
   done = FindDrawHeader (findParams, header);
   MemHandleUnlock(headerStringH);   
   DmReleaseResource(headerStringH);   
   if (done) 
      goto Exit;

   // Search the description and note fields for the "find" string.
   recordNum = findParams->recordNum;
   while (true)
   	{
      // Because applications can take a long time to finish a find when
      // the result may be on the screen or for other reasons, users like
      // to be able to stop the find.  Stop the find if an event is pending.
      // This stops if the user does something with the device.  Because
      // this call slows down the search we perform it every so many 
      // records instead of every record.  The response time should still
      // be Int16 without introducing much extra work to the search.
      
      // Note that in the implementation below, if the next 16th record is  
      // secret the check doesn't happen.  Generally this shouldn't be a 
      // problem since if most of the records are secret then the search 
      // won't take long anyways!
      if ((recordNum & 0x000f) == 0 &&         // every 16th record
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

      // Search all the fields of the address record.
      AddrGetRecord (dbP, recordNum, &record, &recordH);
      match = false;
      for (i = 0; i < addrNumFields; i++)
      	{
         if (record.fields[i])
         	{
            match = FindStrInStr (record.fields[i], findParams->strToFind, &pos);
            if (match) 
               break;
         	}
      	}
      
      if (match)
      	{
			// Add the match to the find paramter block,  if there is no room to
			// display the match the following function will return true.
         done = FindSaveMatch (findParams, recordNum, pos, i, 0, cardNo, dbID);
         if (done)
         	{
            MemHandleUnlock(recordH);
            break;
         	}

         // Get the bounds of the region where we will draw the results.
         FindGetLineBounds (findParams, &r);

         appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(dbP);
         InitPhoneLabelLetters(appInfoPtr, searchPhoneLabelLetters);

         // Display the title of the description.
			FntSetFont (stdFont);
         DrawRecordNameAndPhoneNumber (&record, &r, searchPhoneLabelLetters,
            appInfoPtr->misc.sortByCompany, &unnamedRecordStringPtr);
         
         MemPtrUnlock(appInfoPtr);

         findParams->lineNumber++;
	      }

      MemHandleUnlock(recordH);
      recordNum++;
   }
   
   if (unnamedRecordStringPtr)
   	{
      MemPtrUnlock(unnamedRecordStringPtr);
//      DmReleaseResource(strRsc, UnnamedRecordStr)
   	}
   
Exit:
   DmCloseDatabase (dbP);   
}


/***********************************************************************
 *
 * FUNCTION:    GoToItem
 *
 * DESCRIPTION: This routine is a entry point of this application.
 *              It is generally call as the result of hiting of 
 *              "Go to" button in the text search dialog.
 *
 * PARAMETERS:  recordNum - 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  7/12/95   Initial Revision
 *         jmp    9/17/99   Use NewNoteView instead of NoteView.
 *
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
   DmRecordInfo (AddrDB, recordNum, &attr, &uniqueID, NULL);

   // Change the current category if necessary.
   if (CurrentCategory != dmAllCategories)
   	{
      CurrentCategory = attr & dmRecAttrCategoryMask;
   	}

   
   // If the application is already running, close all the open forms.  If
   // the current record is blank, then it will be deleted, so we'll 
   // the record's unique id to find the record index again, after all 
   // the forms are closed.
   if (! launchingApp)
   	{
      FrmCloseAllForms ();
      DmFindRecordByID (AddrDB, uniqueID, &recordNum);
   	}
      

   // Set global variables that keep track of the currently record.
   CurrentRecord = recordNum;
   
   // Set PriorAddressFormID so the Note View returns to the List View
   PriorAddressFormID = ListView;


   if (goToParams->matchFieldNum == note)
      formID = NewNoteView;
   else
      formID = RecordView;

   MemSet (&event, sizeof(EventType), 0);

   // Send an event to load the form we want to goto.
   event.eType = frmLoadEvent;
   event.data.frmLoad.formID = formID;
   EvtAddEventToQueue (&event);

   // Send an event to goto a form and select the matching text.
   event.eType = frmGotoEvent;
   event.data.frmGoto.formID = formID;
   event.data.frmGoto.recordNum = recordNum;
   event.data.frmGoto.matchPos = goToParams->matchPos;
   event.data.frmGoto.matchLen = goToParams->searchStrLen;
   event.data.frmGoto.matchFieldNum = goToParams->matchFieldNum;
   EvtAddEventToQueue (&event);

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
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   2/21/95      Initial Revision
 *
 ***********************************************************************/
static FieldPtr GetFocusObjectPtr (void)
{
   FormPtr frm;
   UInt16 focus;
   
   frm = FrmGetActiveForm ();
   focus = FrmGetFocus (frm);
   if (focus == noFocus)
      return (NULL);
      
   return (FrmGetObjectPtr (frm, focus));
}


#pragma mark ----------------
/***********************************************************************
 *
 * FUNCTION:    ResetScrollRate
 *
 * DESCRIPTION: This routine resets the scroll rate
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			frigino	8/14/97	Initial Revision
 *
 ***********************************************************************/
static void ResetScrollRate(void)
{
	// Reset last seconds
	LastSeconds = TimGetSeconds();
	// Reset scroll units
	ScrollUnits = 1;
}

/***********************************************************************
 *
 * FUNCTION:    AdjustScrollRate
 *
 * DESCRIPTION: This routine adjusts the scroll rate based on the current
 *              scroll rate, given a certain delay, and plays a sound
 *              to notify the user of the change
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			frigino		8/14/97		Initial Revision
 *			vmk			12/2/97		Fix crash from uninitialized sndCmd and
 *									derive sound amplitude from system amplitude
 *
 ***********************************************************************/
static void AdjustScrollRate(void)
{
	// Accelerate the scroll rate every 3 seconds if not already at max scroll speed
	UInt16 newSeconds = TimGetSeconds();
	if ((ScrollUnits < scrollSpeedLimit) && ((newSeconds - LastSeconds) > scrollDelay))
		{
		// Save new seconds
		LastSeconds = newSeconds;
		
		// increase scroll units
		ScrollUnits += scrollAcceleration;
		}

}


#pragma mark ----------------
/***********************************************************************
 *
 * FUNCTION:    GetLabelColumnWidth
 *
 * DESCRIPTION: Calculate the width of the widest field label plus a ':'.  
 *
 * PARAMETERS:  appInfoPtr  - pointer to the app info block for field labels
 *              labelFontID - font 
 *
 * RETURNED:    width of the widest label.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   1/30/98   Initial Revision
 *
 ***********************************************************************/
static UInt16 GetLabelColumnWidth (AddrAppInfoPtr appInfoPtr, FontID labelFontID)
{
	Int16		i;
	UInt16	labelWidth;     // Width of a field label
	UInt16	columnWidth;    // Width of the label column (fits all label)
	FontID	curFont;
	Char *	label;


	// Calculate column width of the label column which is used by the Record View and the
	// Edit View.
	curFont = FntSetFont (labelFontID);

	columnWidth = 0;

	for (i = firstAddressField; i < lastLabel; i ++)
		{
		label = appInfoPtr->fieldLabels[i];
		labelWidth = FntCharsWidth(label, StrLen(label));
		columnWidth = max(columnWidth, labelWidth);
		}
	columnWidth += 1 + FntCharWidth(':');

	FntSetFont (curFont);
	
	if (columnWidth > maxLabelColumnWidth)
		columnWidth = maxLabelColumnWidth;

	return columnWidth;
}


/***********************************************************************
 *
 * FUNCTION:    LeaveForm
 *
 * DESCRIPTION: Leaves the current popup form and returns to the prior one.
 *
 * PARAMETERS:  formID  - resource id of form to return to 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/30/95   Initial Revision
 *
 ***********************************************************************/
static void LeaveForm  ()
{
   FormPtr frm;

   frm = FrmGetActiveForm();
   FrmEraseForm (frm);
   FrmDeleteForm (frm);
   FrmSetActiveForm (FrmGetFirstForm ());
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
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
static void ChangeCategory (UInt16 category)
{
   CurrentCategory = category;
   TopVisibleRecord = 0;
}


/***********************************************************************
 *
 * FUNCTION:    SeekRecord
 *
 * DESCRIPTION: Given the index of a to do record, this routine scans 
 *              forewards or backwards for displayable to do records.           
 *
 * PARAMETERS:  indexP  - pointer to the index of a record to start from;
 *                        the index of the record sought is returned in
 *                        this parameter.
 *
 *              offset  - number of records to skip:   
 *                           0 - mean seek from the current record to the
 *                             next display record, if the current record is
 *                             a display record, its index is retuned.
 *                         1 - mean seek foreward, skipping one displayable 
 *                             record
 *                        -1 - means seek backwards, skipping one 
 *                             displayable record
 *                             
 *
 * RETURNED:    false is return if a displayable record was not found.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
static Boolean SeekRecord (UInt16 * indexP, Int16 offset, Int16 direction)
{
   DmSeekRecordInCategory (AddrDB, indexP, offset, direction, CurrentCategory);
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
 * PARAMETERS: 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
static void DirtyRecord (UInt16 index)
{
   UInt16      attr;

   DmRecordInfo (AddrDB, index, &attr, NULL, NULL);
   attr |= dmRecAttrDirty;
   DmSetRecordInfo (AddrDB, index, &attr, NULL);
}


/***********************************************************************
 *
 * FUNCTION:    CreateNote
 *
 * DESCRIPTION: Make sure there is a note field to edit.  If one doesn't
 * exist make one.  
 *
 *   The main reason this routine exists is to make sure we can edit a note
 * before we close the current form.
 *
 * PARAMETERS:  CurrentRecord set
 *
 * RETURNED:    true if a note field exists to edit
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   10/19/95   Initial Revision
 *
 ***********************************************************************/
static Boolean CreateNote (void)
{
   AddrDBRecordType record;
   AddrDBRecordFlags bit;
   MemHandle recordH;
   Err err;

   
   AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);
   
   
   // Since we are going to edit in place, add a note field if there
   // isn't one
   if (!record.fields[note])
      {
      record.fields[note] = "";
      bit.allBits = (UInt32)1 << note;
      err = AddrChangeRecord(AddrDB, &CurrentRecord, &record, bit);
      if (err)
         {
         MemHandleUnlock(recordH);
         FrmAlert(DeviceFullAlert);
         return false;            // can't make an note field.
         }

      }
   else
      {
      MemHandleUnlock(recordH);
      }
      
   return true;                  // a note field exists.
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
 * FUNCTION:    DetailsSelectCategory
 *
 * DESCRIPTION: This routine handles selection, creation and deletion of
 *              categories form the Details Dialog.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the category was changed in a way that 
 *              require the list view to be redrawn.
 *
 *              The following global variables are modified:
 *                     CategoryName
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *       art   06/05/95   Initial Revision
 *       art   09/28/95   Fixed problem with merging and renaming
 *			gap	08/13/99   Update to use new constant categoryDefaultEditCategoryString.
 *
 ***********************************************************************/
static Boolean DetailsSelectCategory (UInt16 * category)
{
   Boolean categoryEdited;
   
   categoryEdited = CategorySelect (AddrDB, FrmGetActiveForm (),
      DetailsCategoryTrigger, DetailsCategoryList,
      false, category, CategoryName, 1, categoryDefaultEditCategoryString);
   
   return (categoryEdited);
}


/***********************************************************************
 *
 * FUNCTION:    DeleteRecord
 *
 * DESCRIPTION: Deletes an address record.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger	6/13/95	Initial Revision
 *			  grant	6/21/99	Unlock the record if it is being archived.
 *         jwm		1999-10-8 Swap forward/backward so users can work
 *									through deleting records from top to bottom.
 *
 ***********************************************************************/
static void DeleteRecord (Boolean archive)
{
   // Show the following record.  Users want to see where the record was and
   // they also want to return to the same location in the database because
   // they might be working their way through the records.  If there isn't
   // a following record show the prior record.  If there isn't a prior
   // record then don't show a record.
   ListViewSelectThisRecord = CurrentRecord;
	if (!SeekRecord(&ListViewSelectThisRecord, 1, dmSeekForward))
		if (!SeekRecord(&ListViewSelectThisRecord, 1, dmSeekBackward))
         ListViewSelectThisRecord = noRecord;
   	 
   // Delete or archive the record.
   if (archive)
   	{
      DmArchiveRecord (AddrDB, CurrentRecord);
      }
   else
      DmDeleteRecord (AddrDB, CurrentRecord);
   
   // Deleted records are stored at the end of the database
   DmMoveRecord (AddrDB, CurrentRecord, DmNumRecords (AddrDB));

   // Since we just moved the CurrentRecord to the end the 
   // ListViewSelectThisRecord may have been moved up one position.
   if (ListViewSelectThisRecord >= CurrentRecord &&
      ListViewSelectThisRecord != noRecord)
      ListViewSelectThisRecord--;
   

   // Use whatever record we found to select.   
   CurrentRecord = ListViewSelectThisRecord;
}


/***********************************************************************
 *
 * FUNCTION:    DetailsDeleteRecord
 *
 * DESCRIPTION: This routine deletes an address record. This routine is 
 *              called when the delete button in the details dialog is
 *              pressed or when delete record is used from a menu.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the record was delete or archived.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95   Initial Revision
 *         art   9/30/95   Remember the "save backup" settting.
 *
 ***********************************************************************/
static Boolean DetailsDeleteRecord (void)
{
   UInt16 ctlIndex;
   UInt16 buttonHit;
   FormPtr alert;
   Boolean archive;
      
   // Display an alert to comfirm the operation.
   alert = FrmInitForm (DeleteAddrDialog);

   // Set the "save backup" checkbox to its previous setting.
   ctlIndex = FrmGetObjectIndex (alert, DeleteAddrSaveBackup);
   FrmSetControlValue (alert, ctlIndex, SaveBackup);

   buttonHit = FrmDoDialog (alert);

   archive = FrmGetControlValue (alert, ctlIndex);

   FrmDeleteForm (alert);
   if (buttonHit == DeleteAddrCancel)
      return (false);

   // Remember the "save backup" checkbox setting.
   SaveBackup = archive;

   DeleteRecord(archive);
   
   return (true);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsApply
 *
 * DESCRIPTION: This routine applies the changes made in the Details Dialog.
 *
 * PARAMETERS:  category - new catagory
 *
 * RETURNED:    code which indicates how the to do list was changed,  this
 *              code is send as the update code, in the frmUpdate event.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95   Initial Revision
 *         kcr   10/9/95   added 'private records' alert
 *
 ***********************************************************************/
static UInt16 DetailsApply (UInt16 category, Boolean categoryEdited)
{
   UInt16               attr;
   UInt16                updateCode = 0;
   Boolean           secret;
   Boolean            dirty = false;
   AddrDBRecordType currentRecord;
   MemHandle recordH;
   AddrDBRecordFlags changedFields;
   UInt16               newPhoneFieldToDisplay;
   Err               err;



   // Get the phone number to show at the list view.
   AddrGetRecord(AddrDB, CurrentRecord, &currentRecord, &recordH);
   newPhoneFieldToDisplay = LstGetSelection(GetObjectPtr (DetailsPhoneList));
   if (currentRecord.options.phones.displayPhoneForList != newPhoneFieldToDisplay)
      {
      currentRecord.options.phones.displayPhoneForList = newPhoneFieldToDisplay;
      changedFields.allBits = 0;
      err = AddrChangeRecord(AddrDB, &CurrentRecord, &currentRecord,
         changedFields);
      if (err)
         {
         MemHandleUnlock(recordH);
         FrmAlert(DeviceFullAlert);
         }

      updateCode |= updateListViewPhoneChanged;
      }
   else
      MemHandleUnlock(recordH);
   
   
   // Get the category and the secret attribute of the current record.
   DmRecordInfo (AddrDB, CurrentRecord, &attr, NULL, NULL);   


   // Get the current setting of the secret checkbox and compare it the
   // the setting of the record.  Update the record if the values 
   // are different.  If the record is being set 'secret' for the
   //   first time, and the system 'hide secret records' setting is
   //   off, display an informational alert to the user.
   secret = CtlGetValue (GetObjectPtr (DetailsSecretCheckbox));
   if (((attr & dmRecAttrSecret) == dmRecAttrSecret) != secret)
      {
      if (PrivateRecordVisualStatus > showPrivateRecords)
      	{
         updateCode |= updateItemHide;
         }
      else if (secret)
         FrmAlert (privateRecordInfoAlert);
      
      dirty = true;
      
      if (secret)
         attr |= dmRecAttrSecret;
      else
         attr &= ~dmRecAttrSecret;
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

   // If current category was moved, deleted renamed, or merged with
   // another category, then the list view needs to be redrawn.
   if (categoryEdited)
      {
      CurrentCategory = category;
      updateCode |= updateCategoryChanged;
      }
         

   // Save the new category and/or secret status, and mark the record dirty.
   if (dirty)
      {
      attr |= dmRecAttrDirty;
      DmSetRecordInfo (AddrDB, CurrentRecord, &attr, NULL);
      }

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
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
static void DetailsInit (UInt16 * categoryP)
{
   UInt16 attr;
   UInt16 category;
   ControlPtr ctl;
   ListPtr popupPhoneList;
   AddrDBRecordType currentRecord;
   MemHandle recordH;
   AddrAppInfoPtr appInfoPtr;
   UInt16 phoneLabel;
   UInt16 i;


   // Make a list of the phone labels used by this record
   appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);
   AddrGetRecord(AddrDB, CurrentRecord, &currentRecord, &recordH);

   for (i = 0; i < 5; i++)
      {
      phoneLabel = GetPhoneLabel(&currentRecord, firstPhoneField + i);
      DetailsPhoneListChoices[i] = appInfoPtr->fieldLabels[phoneLabel + 
         ((phoneLabel < numPhoneLabelsStoredFirst) ? 
         firstPhoneField : (addressFieldsCount - numPhoneLabelsStoredFirst))];
      }
             

   // Set the default phone list to the list of phone labels just made
   popupPhoneList = GetObjectPtr (DetailsPhoneList);
   LstSetListChoices(popupPhoneList, DetailsPhoneListChoices, numPhoneFields);
   LstSetSelection (popupPhoneList, currentRecord.options.phones.displayPhoneForList);
   LstSetHeight (popupPhoneList, numPhoneFields);
   CtlSetLabel(GetObjectPtr (DetailsPhoneTrigger),
      LstGetSelectionText (popupPhoneList, currentRecord.options.phones.displayPhoneForList));

   // If the record is mark secret, turn on the secret checkbox.
   DmRecordInfo (AddrDB, CurrentRecord, &attr, NULL, NULL);
   ctl = GetObjectPtr (DetailsSecretCheckbox);
   CtlSetValue (ctl, attr & dmRecAttrSecret);

   // Set the label of the category trigger.
   category = attr & dmRecAttrCategoryMask;
   CategoryGetName (AddrDB, category, CategoryName);
   ctl = GetObjectPtr (DetailsCategoryTrigger);
   CategorySetTriggerLabel (ctl, CategoryName);

   // Return the current category and due date.
   *categoryP = category;


   MemHandleUnlock(recordH);
   MemPtrUnlock(appInfoPtr);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Details
 *              Dialog Box" of the Address application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art    6/5/95    Initial Revision
 *         jmp    9/17/99   Use NewNoteView instead of NoteView.
 *
 ***********************************************************************/
static Boolean DetailsHandleEvent (EventType * event)
{
   static UInt16       category;
   static Boolean      categoryEdited;

   UInt16 updateCode;
   Boolean handled = false;
   FormPtr frm;


   if (event->eType == ctlSelectEvent)
      {
      switch (event->data.ctlSelect.controlID)
         {
         case DetailsOkButton:
            updateCode = DetailsApply (category, categoryEdited);
				LeaveForm ();
            if (updateCode)
            	FrmUpdateForm (EditView, updateCode);
            handled = true;
            break;

         case DetailsCancelButton:
            if (categoryEdited)
            	FrmUpdateForm (EditView, updateCategoryChanged);
			LeaveForm ();
            handled = true;
            break;
            
         case DetailsDeleteButton:
            if ( DetailsDeleteRecord ())
               { 
               FrmCloseAllForms ();
               FrmGotoForm (ListView);
               }
            handled = true;
            break;
            
         case DetailsNoteButton:
            DetailsApply (category, categoryEdited);
            FrmReturnToForm (EditView);
            if (CreateNote())
            	{
               FrmGotoForm (NewNoteView);
               RecordNeededAfterEditView = true;
               }
            handled = true;
            break;

         case DetailsCategoryTrigger:
				categoryEdited = DetailsSelectCategory (&category) || categoryEdited;
            handled = true;
            break;
         }
      }


   else if (event->eType == frmOpenEvent)
      {
      frm = FrmGetActiveForm ();
      DetailsInit (&category);
      FrmDrawForm (frm);
      categoryEdited = false;
      handled = true;
      }

   return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    CustomEditSave
 *
 * DESCRIPTION: Write the renamed field labels
 *
 * PARAMETERS:  frm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   2/23/95   Initial Revision
 *
 ***********************************************************************/
static void CustomEditSave (FormPtr frm)
{
	UInt16      index;
	FieldPtr   fld;
	UInt16      objNumber;
	Char * textP;
	AddrAppInfoPtr appInfoPtr;
	Boolean sendUpdate = false;


	// Get the object number of the first field.
	objNumber = FrmGetObjectIndex (frm, CustomEditFirstField);


	// For each dirty field update the corresponding label.
	for (index = firstRenameableLabel; index <= lastRenameableLabel; index++)
		{
		fld = FrmGetObjectPtr (frm, objNumber++);
		if (FldDirty(fld))
			{
			sendUpdate = true;
			textP = FldGetTextPtr(fld);
			if (textP)
				AddrSetFieldLabel(AddrDB, index, textP);
			}
		}

	if (sendUpdate)
		{
		// Update the column width since a label changed.
		appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);
		EditLabelColumnWidth = GetLabelColumnWidth (appInfoPtr, stdFont);
		RecordLabelColumnWidth = GetLabelColumnWidth (appInfoPtr, AddrRecordFont);
		MemPtrUnlock(appInfoPtr);

		FrmUpdateForm (0, updateCustomFieldLabelChanged);
		}

}


/***********************************************************************
 *
 * FUNCTION:    CustomEditInit
 *
 * DESCRIPTION: Load field labels for editing.
 *
 * PARAMETERS:  frm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   2/23/95   Initial Revision
 *
 ***********************************************************************/
static void CustomEditInit (FormPtr frm)
{
   UInt16      index;
   UInt16      length;
   FieldPtr   fld;
   UInt16      objNumber;
   MemHandle textH;
   Char * textP;
   AddrAppInfoPtr appInfoPtr;
   addressLabel *fieldLabels;
   
   
   // Get the object number of the first field.
   objNumber = FrmGetObjectIndex (frm, CustomEditFirstField);

   appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);
   fieldLabels = appInfoPtr->fieldLabels;

   // For each label, allocate some global heap space and copy the
   // the string to the global heap.  Then set a field to use the
   // copied string for editing.  If the field is unused no space is
   // allocated.  The field will allocate space if text is typed in.
   for (index = firstRenameableLabel; index <= lastRenameableLabel; index++)
      {
      fld = FrmGetObjectPtr (frm, objNumber++);
      length = StrLen(fieldLabels[index]);
      if (length > 0)
         {
         length += 1;         // include space for a null terminator
         textH = MemHandleNew(length);
         if (textH)
            {
            textP = MemHandleLock(textH);
            MemMove(textP, fieldLabels[index], length);
            FldSetTextHandle (fld, textH);
            MemHandleUnlock(textH);
            }
         }
      }
      
   MemPtrUnlock(appInfoPtr);
}


/***********************************************************************
 *
 * FUNCTION:    CustomEditHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Edit Custom
 *              Fields" of the Address application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/23/95   Initial Revision
 *
 ***********************************************************************/
static Boolean CustomEditHandleEvent (EventType * event)
{
   Boolean handled = false;
   FormPtr frm;


   if (event->eType == ctlSelectEvent)
      {
      switch (event->data.ctlSelect.controlID)
         {
         case CustomEditOkButton:
            frm = FrmGetActiveForm();
            CustomEditSave(frm);
            LeaveForm();
            handled = true;
            break;

         case CustomEditCancelButton:
            LeaveForm();
            handled = true;
            break;
         default:
            break;
         
         }
      }


   else if (event->eType == frmOpenEvent)
      {
      frm = FrmGetActiveForm ();
      CustomEditInit (frm);
      FrmDrawForm (frm);
      handled = true;
      }

   return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    PreferencesDialogSave
 *
 * DESCRIPTION: Write the renamed field labels
 *
 * PARAMETERS:  frm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *				Name		Date		Description
 *				----		----		-----------
 *				roger		8/2/95	Initial Revision
 *				jmp		11/02/99	Don't reset CurrentRecord to zero if it's been set to
 *										noRecord.  Fixes bug #23571.
 *
 ***********************************************************************/
static void PreferencesDialogSave (FormPtr frm)
{
   FormPtr curFormP;
   FormPtr formP;
   Boolean sortByCompany;

   RememberLastCategory = CtlGetValue(FrmGetObjectPtr (frm, 
      FrmGetObjectIndex (frm, PreferencesRememberCategoryCheckbox)));

   sortByCompany = CtlGetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PreferencesCompanyName)));
   
   if (sortByCompany != SortByCompany)
	{
	   // Put up the sorting message dialog so the user knows what's going on
	   // while the sort locks up the device.
	   curFormP = FrmGetActiveForm ();
	   formP = FrmInitForm (SortingMessageDialog);
	   FrmSetActiveForm (formP);
	   FrmDrawForm (formP);
	   
	   // Peform the sort.
	   SortByCompany = sortByCompany;
	   AddrChangeSortOrder(AddrDB, SortByCompany);
	   CurrentRecord = (CurrentRecord == noRecord) ? noRecord : 0;
	   TopVisibleRecord = 0;
	   FrmUpdateForm (ListView, updateRedrawAll);
	   
	   // Remove the sorting message.
	   FrmEraseForm (formP);
	   FrmDeleteForm (formP);
	   FrmSetActiveForm (curFormP);
	}
}


/***********************************************************************
 *
 * FUNCTION:    PreferencesDialogInit
 *
 * DESCRIPTION: Initialize the dialog's ui.  Sets the database sort by
 * buttons.
 *
 * PARAMETERS:  frm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   8/2/95   Initial Revision
 *
 ***********************************************************************/
static void PreferencesDialogInit (FormPtr frm)
{
   UInt16      objNumber;
   
   
   CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PreferencesRememberCategoryCheckbox)),
      RememberLastCategory);
   
   
   // Set the current sort by setting
   if (SortByCompany)
      objNumber = PreferencesCompanyName;
   else
      objNumber = PreferencesLastName;

   CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objNumber)),
      true);
   
}


/***********************************************************************
 *
 * FUNCTION:    PreferencesDialogHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "List By
 *              Dialog" of the Address application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   8/2/95   Initial Revision
 *
 ***********************************************************************/
static Boolean PreferencesDialogHandleEvent (EventType * event)
{
   Boolean handled = false;
   FormPtr frm;


   if (event->eType == ctlSelectEvent)
      {
      switch (event->data.ctlSelect.controlID)
         {
         case PreferencesOkButton:
            frm = FrmGetActiveForm();
            PreferencesDialogSave(frm);
            LeaveForm();
            handled = true;
            break;

         case PreferencesCancelButton:
            LeaveForm();
            handled = true;
            break;
         default:
            break;
            
         }
      }


   else if (event->eType == frmOpenEvent)
      {
      frm = FrmGetActiveForm ();
      PreferencesDialogInit (frm);
      FrmDrawForm (frm);
      handled = true;
      }

   return (handled);
}


#pragma mark ----------------
/***********************************************************************
 *
 * FUNCTION:    NoteViewDrawTitleAndForm
 *
 * DESCRIPTION: Draw the form and the title of the note view.  The title should be
 * the names that appear for the record on the list view.
 *
 * PARAMETERS:  frm, FormPtr to the form to draw
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  6/21/95   Initial Revision
 *         jmp    9/27/99   Square off the NoteView title so that it covers up
 *                          the blank Form title used to trigger the menu on taps
 *                          to the title area.  Also, set the NoteView title's color
 *                          to match the standard Form title colors.  Eventually, we
 *                          should add a variant to Forms that allows for NoteView
 *                          titles directly.  This "fixes" bug #21610.
 *         jmp    9/29/99   Fix bug #22412.  Ensure that title-length metrics are
 *                          computed AFTER the NoteView's title font has been set.	
 *         jmp   12/02/99   Fix bug #24377.  Don't call WinScreenUnlock() if WinScreenLock()
 *                          fails.
 *                          
 ***********************************************************************/
 static void NoteViewDrawTitleAndForm (FormPtr frm)
 {
	Coord x, y;
	Int16 fieldSeparatorWidth;
	Int16 shortenedFieldWidth;
	Char * name1;
	Char * name2;
	Int16 name1Length;
	Int16 name2Length;
	Int16 name1Width;
	Int16 name2Width;
	Int16 nameExtent;
	Coord formWidth;
	RectangleType r;
	FontID curFont;
	RectangleType eraseRect,drawRect;
	Boolean name1HasPriority;
	IndexedColorType curForeColor;
	IndexedColorType curBackColor;
	IndexedColorType curTextColor;
	AddrDBRecordType record;
	MemHandle recordH;
	UInt8 * lockedWinP;
	Err error;

	// "Lock" the screen so that all drawing occurs offscreen to avoid
	// the anamolies associated with drawing the Form's title then drawing
	// the NoteView title.  We REALLY need to make a variant for doing
	// this in a more official way!
	//
	lockedWinP = WinScreenLock (winLockCopy);

	FrmDrawForm (frm);

	// Peform initial set up.
	//
   FrmGetFormBounds (frm, &r);
   formWidth = r.extent.x;
   x = 2;
   y = 1;
   nameExtent = formWidth - 4;
   
   RctSetRectangle (&eraseRect, 0, 0, formWidth, FntLineHeight()+4);
   RctSetRectangle (&drawRect, 0, 0, formWidth, FntLineHeight()+2);

	// Save/Set window colors and font.  Do this after FrmDrawForm() is called
	// because FrmDrawForm() leaves the fore/back colors in a state that we
	// don't want here.
	//
 	curForeColor = WinSetForeColor (UIColorGetTableEntryIndex(UIFormFrame));
 	curBackColor = WinSetBackColor (UIColorGetTableEntryIndex(UIFormFill));
 	curTextColor = WinSetTextColor (UIColorGetTableEntryIndex(UIFormFrame));
   curFont = FntSetFont (boldFont);

	// Erase the Form's title area and draw the NoteView's.
	//
	WinEraseRectangle (&eraseRect, 0);
   WinDrawRectangle (&drawRect, 3);
      
	error = AddrGetRecord(AddrDB, CurrentRecord, &record, &recordH);
	ErrNonFatalDisplayIf(error, "Record not found");
   name1HasPriority = DetermineRecordName(&record, &shortenedFieldWidth, &fieldSeparatorWidth, 
      SortByCompany, &name1, &name1Length, &name1Width, 
      &name2, &name2Length, &name2Width, &UnnamedRecordStringPtr, nameExtent);
      
   DrawRecordName(name1, name1Length, name1Width, name2, name2Length, name2Width,
      nameExtent, &x, y, shortenedFieldWidth, fieldSeparatorWidth, true,
      name1HasPriority || !SortByCompany, true);
      
	// Now that we've drawn everything, blast it all back on the screen at once.
	//
	if (lockedWinP)
		WinScreenUnlock ();

	// Unlock the record that AddrGetRecord() implicitly locked.
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
 * DESCRIPTION: Load the record's note field into the field object
 * for editing in place.  The note field is too big (4K) to edit in 
 * the heap.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			roger		6/21/95	Initial Revision
 *			roger		8/25/95	Changed to edit in place
 *			jmp		10/11/99	Replaced private MemDeref() call with MemHandleLock() so
 *									that the SDK build will work.
 *
 ***********************************************************************/
static void NoteViewLoadRecord (void)
{
   UInt16 offset;
   FieldPtr fld;
   MemHandle recordH;
   Char * ptr;
   AddrDBRecordType record;

   // Get a pointer to the memo field.
   fld = GetObjectPtr (NoteField);
   
   // Set the font used in the memo field.
   FldSetFont (fld, NoteFont);

   AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);

   // CreateNote will have been called before the NoteView was switched
   // to.  It will have insured that a note field exists.

   // Find out where the note field is to edit it
   ptr = MemHandleLock (recordH);
   offset = record.fields[note] - ptr;
   FldSetText (fld, recordH, offset, StrLen(record.fields[note])+1);
   
   // Unlock recordH twice because AddrGetRecord() locks it, and we had to lock
   // it to deref it.  Whacky.
   MemHandleUnlock(recordH);
   MemHandleUnlock(recordH);
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
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   	6/5/95    Initial Revision
 *         roger  8/25/95   Changed to edit in place
 *
 ***********************************************************************/
static void NoteViewSave (void)
{
   FieldPtr fld;
   int textLength;

   
   fld = GetObjectPtr (NoteField);

   
   // If the field wasn't modified then don't do anything
   if (FldDirty (fld))
      {      
      // Release any free space in the note field.
      FldCompactText (fld);

      DirtyRecord (CurrentRecord);
      }

      
   textLength = FldGetTextLength(fld);

   // Clear the handle value in the field, otherwise the handle
   // will be freed when the form is disposed of,  this call also unlocks
   // the handle that contains the note string.
   FldSetTextHandle (fld, 0);
   
   
   // Empty fields are not allowed because they cause problems
   if (textLength == 0)
      DeleteNote();
}


/***********************************************************************
 *
 * FUNCTION:    DeleteNote
 *
 * DESCRIPTION: Deletes the note field from the current record.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/21/95   Initial Revision
 *
 ***********************************************************************/
static void DeleteNote (void)
{
   AddrDBRecordType record;
   MemHandle recordH;
   AddrDBRecordFlags changedField;
   Err err;
   
      
   AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);
   record.fields[note] = NULL;
   changedField.allBits = (UInt32)1 << note;
   err = AddrChangeRecord(AddrDB, &CurrentRecord, &record, changedField);
   if (err)
      {
      MemHandleUnlock(recordH);
      FrmAlert(DeviceFullAlert);
      return;
      }


   // Mark the record dirty.   
   DirtyRecord (CurrentRecord);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewDeleteNote
 *
 * DESCRIPTION: This routine deletes a the note field from a to do record.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the note was deleted.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
static Boolean NoteViewDeleteNote (void)
{
   FieldPtr fld;
   
   if (FrmAlert(DeleteNoteAlert) != DeleteNoteYes)
      return (false);
      
   // Unlock the handle that contains the text of the memo.
   fld = GetObjectPtr (NoteField);
   ErrFatalDisplayIf ((! fld), "Bad field");

   // Clear the handle value in the field, otherwise the handle
   // will be freed when the form is disposed of. this call also 
   // unlocks the handle the contains the note string.
   FldCompactText (fld);
   FldSetTextHandle (fld, 0);   


   DeleteNote();
   
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
 *       art   6/5/95   Initial Revision
 *			jmp	9/8/99	Moved the noteFontCmd case to where it is in this
 *								routine in all the other NoteView implementations.
 *			jmp	9/17/99	Eliminate the goto top/bottom of page menu items
 *								as NewNoteView no longer supports them.
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
 * DESCRIPTION: This routine scrolls the Note View by the specified
 *					 number of lines.
 *
 * PARAMETERS:  linesToScroll - the number of lines to scroll,
 *						positive for down,
 *						negative for up
 *					 updateScrollbar - force a scrollbar update?
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 * 		Name	Date		Description
 * 		----	----		-----------
 * 		art	7/1/96	Initial Revision
 *			grant	2/2/99	Use NoteViewUpdateScrollBar()
 *
 ***********************************************************************/
static void NoteViewScroll (Int16 linesToScroll, Boolean updateScrollbar)
{
   UInt16           blankLines;
   FieldPtr         fld;
   
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
 * DESCRIPTION: This routine scrolls the message a page up or down.
 *
 * PARAMETERS:   direction     up or down
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   7/1/96   Initial Revision
 *			  grant 2/2/99		Use NoteViewScroll() to do actual scrolling
 *
 ***********************************************************************/
static void NoteViewPageScroll (WinDirectionType direction)
{
   Int16 linesToScroll;
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
 * DESCRIPTION: This routine initials the Edit View form.
 *
 * PARAMETERS:  frm - pointer to the Edit View form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *				Name	Date		Description
 *         	----	----		-----------
 *				art   6/5/95	Initial Revision
 *				jmp	9/23/99	Eliminate code to hide unused font controls
 *									now that we're using a NoteView form that doesn't
 *									have them anymore.
 *
 ***********************************************************************/
static void NoteViewInit (FormPtr frm)
{
   FieldPtr       fld;
   FieldAttrType  attr;
   
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
 *              of the ToDo application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *       art  	6/5/95   	Initial Revision
 *			jmp	9/8/99		Made this routine more consistent with the
 *									other built-in apps that have it.
 *			jmp	9/27/99		Combined NoteViewDrawTitle() & FrmUpdateForm()
 *									into a single routine that is now called
 *									NoteViewDrawTitleAndForm().
 *
 ***********************************************************************/
static Boolean NoteViewHandleEvent (EventType * event)
{
   FormPtr frm;
   Boolean handled = false;
   FieldPtr fldP;

   
   switch (event->eType)
   	{
      case keyDownEvent:
			if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
				{
				NoteViewSave ();
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
         break;


      case ctlSelectEvent:
         switch (event->data.ctlSelect.controlID)
         	{
            case NoteDoneButton:
               NoteViewSave ();
               
               // When we return to the ListView highlight this record.
               if (PriorAddressFormID == ListView)
                  ListViewSelectThisRecord = CurrentRecord;
               
               FrmGotoForm(PriorAddressFormID);
               handled = true;
               break;

            case NoteDeleteButton:
               if (NoteViewDeleteNote ())
                  FrmGotoForm (PriorAddressFormID);
               
               ListViewSelectThisRecord = noRecord;
               handled = true;
               break;
               
            default:
               break;
         	}
         break;

      case fldChangedEvent:
         frm = FrmGetActiveForm ();
         NoteViewUpdateScrollBar ();
         handled = true;
         break;
      
      case menuEvent:
         return NoteViewDoCommand (event->data.menu.itemID);
      
      case frmOpenEvent:
         frm = FrmGetActiveForm ();
         NoteViewInit (frm);
			NoteViewDrawTitleAndForm (frm);
			NoteViewUpdateScrollBar ();
         FrmSetFocus (frm, FrmGetObjectIndex (frm, NoteField));
         handled = true;
         break;

      case frmGotoEvent:
         frm = FrmGetActiveForm ();
         CurrentRecord = event->data.frmGoto.recordNum;
         NoteViewInit (frm);
         fldP = GetObjectPtr (NoteField);
         FldSetScrollPosition(fldP, event->data.frmGoto.matchPos);
         FldSetSelection(fldP, event->data.frmGoto.matchPos, 
               event->data.frmGoto.matchPos + event->data.frmGoto.matchLen);
         NoteViewDrawTitleAndForm (frm);
         NoteViewUpdateScrollBar ();
         FrmSetFocus (frm, FrmGetObjectIndex (frm, NoteField));
         handled = true;
         break;
      

      case frmUpdateEvent:
			if (event->data.frmUpdate.updateCode & updateFontChanged)
				{
				fldP = GetObjectPtr (NoteField);
				FldSetFont (fldP, NoteFont);
				NoteViewUpdateScrollBar ();
				}
			else
				{
				frm = FrmGetActiveForm ();
				NoteViewDrawTitleAndForm (frm);
				}
      	handled = true;
			break;

      case frmCloseEvent:
         if (UnnamedRecordStringPtr)
         	{
            MemPtrUnlock(UnnamedRecordStringPtr);
            UnnamedRecordStringPtr = NULL;
         	}
         if ( FldGetTextHandle (GetObjectPtr (NoteField)))
            NoteViewSave ();
         break;


      case sclRepeatEvent:
         NoteViewScroll (event->data.sclRepeat.newValue - 
            event->data.sclRepeat.value, false);
         break;
      
      default:
      	break;
   	}

   return (handled);
}


#pragma mark ----------------
/***********************************************************************
 *
 * FUNCTION:    EditViewRestoreEditState
 *
 * DESCRIPTION: This routine restores the edit state of the Edit
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/12/97	Initial Revision
 *
 ***********************************************************************/
static void EditViewRestoreEditState ()
{
	Int16			row;
	FormPtr		frm;
	TablePtr		table;
	FieldPtr		fld;

	if (CurrentFieldIndex == noFieldIndex) return;

	// Find the row that the current field is in.
	table = GetObjectPtr (EditTable);
	if ( ! TblFindRowID (table, CurrentFieldIndex, &row) )
		return;

	frm = FrmGetActiveForm ();
	FrmSetFocus (frm, FrmGetObjectIndex (frm, EditTable));
	TblGrabFocus (table, row, editDataColumn);
	
	// Restore the insertion point position.
	fld = TblGetCurrentField (table);
	FldSetInsPtPosition (fld, EditFieldPosition);
	FldGrabFocus (fld);
}


/***********************************************************************
 *
 * FUNCTION:    EditSetGraffitiMode
 *
 * DESCRIPTION: Set the graffiti mode based on the field being edited.
 *
 * PARAMETERS:  currentField - the field being edited.
 *
 * RETURNED:    the graffiti mode is set
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   9/20/95   Initial Revision
 *
 ***********************************************************************/
static void EditSetGraffitiMode (FieldPtr fld, UInt16 currentField)
{
   MemHandle currentRecordH;
   Boolean autoShift;
   FieldAttrType attr;
   AddrDBRecordType currentRecord;


   AddrGetRecord(AddrDB, CurrentRecord, &currentRecord, &currentRecordH);

   if (! isPhoneField(currentField))
      {
      // Set the field to support auto-shift.
      autoShift = true;
      }
   else
      {
      GrfSetState(false, true, false);
      autoShift = false;
      }
      
   if (fld)
      {
      FldGetAttributes (fld, &attr);
      attr.autoShift = autoShift;
      FldSetAttributes (fld, &attr);
      }


   MemHandleUnlock(currentRecordH);
}


/***********************************************************************
 *
 * FUNCTION:    EditViewGetRecordField
 *
 * DESCRIPTION: This routine returns a pointer to a field of the 
 *              address record.  This routine is called by the table 
 *              object as a callback routine when it wants to display or
 *              edit a field.
 *
 * PARAMETERS:  table  - pointer to the memo list table (TablePtr)
 *              row    - row of the table to draw
 *              column - column of the table to draw 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/6/95      Initial Revision
 *
 ***********************************************************************/
static Err EditViewGetRecordField (void * table, Int16 row, Int16 /*column*/, 
   Boolean editing, MemHandle * textH, Int16 * textOffset, Int16 * textAllocSize, 
   FieldPtr fld)
{
   UInt16 fieldNum;
   UInt16  fieldIndex;
   Char * recordP;
   Char * fieldP;
   MemHandle recordH, fieldH;
   UInt16 fieldSize;
   AddrDBRecordType record;

   
   // Get the field number that corresponds to the table item.
   // The field number is stored as the row id.
   //
   fieldIndex = TblGetRowID (table, row);
   fieldNum = FieldMap[fieldIndex];

   AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);

   if (editing)
      {
      EditSetGraffitiMode(fld, fieldNum);
      if (record.fields[fieldNum])
         {
         fieldSize = StrLen(record.fields[fieldNum]) + 1;
         fieldH = MemHandleNew(fieldSize);
         fieldP = MemHandleLock(fieldH);
         MemMove(fieldP, record.fields[fieldNum], fieldSize);
         *textAllocSize = fieldSize;
         MemHandleUnlock(fieldH);
         }
      else
         {
         fieldH = 0;
         *textAllocSize = 0;
         }
      MemHandleUnlock (recordH);
      *textOffset = 0;         // only one string
      *textH = fieldH;
      return (0);
      
      }
   else
      {
      // Calculate the offset from the start of the record.
      recordP = MemHandleLock (recordH);   // record now locked twice

      if (record.fields[fieldNum])
         {
         *textOffset = record.fields[fieldNum] - recordP;
         *textAllocSize = StrLen (record.fields[fieldNum]) + 1;  // one for null terminator
         }
      else
         {
         do
            {
            fieldNum++;
            } while (fieldNum < addressFieldsCount &&
               record.fields[fieldNum] == NULL);
      
         if (fieldNum < addressFieldsCount)
            *textOffset = record.fields[fieldNum] - recordP;
         else
            // Place the new field at the end of the text.
            *textOffset = MemHandleSize(recordH);   

         *textAllocSize = 0;  // one for null terminator
         }
      MemHandleUnlock (recordH);   // unlock the second lock

      }

   MemHandleUnlock (recordH);      // unlock the AddrGetRecord lock

   *textH = recordH;
   return (0);
}


/***********************************************************************
 *
 * FUNCTION:    EditViewSaveRecordField
 *
 * DESCRIPTION: This routine saves a field of an address to the 
 *              database.  This routine is called by the table 
 *              object, as a callback routine, when it wants to save
 *              an item.
 *
 * PARAMETERS:  table  - pointer to the memo list table (TablePtr)
 *              row    - row of the table to draw
 *              column - column of the table to draw 
 *
 * RETURNED:    true if the table needs to be redrawn
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   2/21/95   Initial Revision
 *
 ***********************************************************************/
static Boolean EditViewSaveRecordField (void * table, Int16 row, Int16 /*column*/)
{
   UInt16 fieldNum;
   UInt16 fieldIndex;
   FieldPtr fld;
   AddrDBRecordType record;
   MemHandle recordH;
   MemHandle textH;
   Char * textP;
   AddrDBRecordFlags bit;
   UInt16 i;
   Err err;
   Boolean redraw = false;
   UInt16 numOfRows;
   Int16 newSize;
   
   
   fld = TblGetCurrentField (table);
   textH = FldGetTextHandle(fld);
   
   // Get the field number that corresponds to the table item to save.
   fieldIndex = TblGetRowID (table, row);
   fieldNum = FieldMap[fieldIndex];
   
   // Save the field last edited.
   EditRowIDWhichHadFocus = fieldIndex;

   // Save the cursor position of the field last edited.
   // Check if the top of the text is scroll off the top of the 
   // field, if it is then redraw the field.
   if (FldGetScrollPosition (fld))
      {
      FldSetScrollPosition (fld, 0);
      EditFieldPosition = 0;
      }
   else
      EditFieldPosition = FldGetInsPtPosition (fld);

   // Make sure there any selection is removed since we will free
   // the text memory before the callee can remove the selection.
   FldSetSelection (fld, 0, 0);


   if (FldDirty (fld))
      {
      // Since the field is dirty, mark the record as dirty
      DirtyRecord (CurrentRecord);
      
      // Get a pointer to the text of the field.
      if (textH == 0)
         textP = NULL;
      else
         {
         textP = MemHandleLock(textH);
         if (textP[0] == '\0')
            textP = NULL;
         }
         
		// If we have text, and saving an auto-fill field, save the data to the proper database
		if (textP) {
			UInt32	dbType;
			
			// Select the proper database for the field we are editing,
			// or skip if not an autofill enabled field
			switch (fieldNum) {
				case title:		dbType = titleDBType; break;
				case company:	dbType = companyDBType; break;
				case city:		dbType = cityDBType; break;
				case state:		dbType = stateDBType; break;
				case country:	dbType = countryDBType; break;
				default:			dbType = 0;
			}

			if (dbType) LookupSave(dbType, sysFileCAddress, textP);
		}

      AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);
      record.fields[fieldNum] = textP;
      
      // If we have changed a phone field and if the show if
      // list view phone is blank set it to the first non blank phone
      // This rule should allow:
      // 1. Showing a blank field is possible
      // 2. Deleting the shown field switches to another
      // 3. Adding a field when there isn't one shows it.
      if (isPhoneField(fieldNum) &&
         record.fields[firstPhoneField + record.options.phones.displayPhoneForList] == NULL)
         {
         for (i = firstPhoneField; i <= lastPhoneField; i++)
            {
            if (record.fields[i] != NULL)
               {
               record.options.phones.displayPhoneForList = i - firstPhoneField;
               break;
               }
            }
         }
               
      
      bit.allBits = (UInt32)1 << fieldNum;
      err = AddrChangeRecord(AddrDB, &CurrentRecord, &record, bit);

      // The new field has been copied into the new record.  Unlock it.
      if (textP)
         MemPtrUnlock(textP);

      // The change was not made (probably storage out of memory)      
      if (err)
         {
         // Because the storage is full the text in the text field differs
         // from the text in the record.  EditViewGetFieldHeight uses
         // the text in the field (because it's being edited).
         // Make the text in the field the same as the text in the record.
         // Resizing should always be possible.
         MemHandleUnlock(recordH);      // Get original text
         AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);
         
         if (record.fields[fieldNum] == NULL)
            newSize = 1;
         else
            newSize = StrLen(record.fields[fieldNum]) + 1;
         
         // Have the field stop using the chunk to unlock it.  Otherwise the resize can't
         // move the chunk if more space is needed and no adjacent free space exists.
			FldSetTextHandle (fld, 0);
			if (!MemHandleResize(textH, newSize))
            {
            textP = MemHandleLock(textH);
            if (newSize > 1)
               StrCopy(textP, record.fields[fieldNum]);
            else
               textP[0] = '\0';
            MemPtrUnlock(textP);
            }
         else
            {
            ErrNonFatalDisplay("Resize failed.");
            }
         
         
         // Update the text field to use whatever text we have.
         FldSetTextHandle (fld, textH);
         
         MemHandleUnlock(recordH);
         FrmAlert(DeviceFullAlert);
         
         // The field may no longer be the same height.  This row and those
         // below may need to be recalced. Mark this row and those
         // below it not usable and reload the table.
         numOfRows = TblGetNumberOfRows(table);
         while (row < numOfRows)
            {
            TblSetRowUsable(table, row, false);
            row++;
            }
         EditViewLoadTable();
         redraw = true;                  // redraw the table showing change lost
         }

      }

   // Free the memory used for the field's text because the table suppresses it.
   FldFreeMemory (fld);


   return redraw;
}


/***********************************************************************
 *
 * FUNCTION:    EditViewSaveRecord
 *
 * DESCRIPTION: Checks the record and saves it if it's OK
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    The view that should be switched to.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         rsf   9/20/95   Initial Revision
 *
 ***********************************************************************/
static UInt16 EditViewSaveRecord ()
{
   MemHandle currentRecordH;
   AddrDBRecordType currentRecord;
   FormPtr frm;
   TablePtr tableP;
   Boolean hasData;


   // Make sure the field being edited is saved
   frm = FrmGetFormPtr (EditView);
   tableP = FrmGetObjectPtr (frm, FrmGetObjectIndex(frm, EditTable));
   TblReleaseFocus(tableP);
   
   
   // If this record is needed then leave.  This is a good time because
   // the data is saved and this is before the record could be deleted.
   if (RecordNeededAfterEditView)
      {
      ListViewSelectThisRecord = noRecord;
      return ListView;
      }
   
   
   // The record may have already been delete by the Delete menu command
   // or the details dialog.  If there isn't a CurrentRecord assume the
   // record has been deleted.
   if (CurrentRecord == noRecord)
      {
      ListViewSelectThisRecord = noRecord;
      return ListView;
      }
   
   // If there is no data then then delete the record.
   // If there is data but no name data then demand some.
   
   AddrGetRecord(AddrDB, CurrentRecord, &currentRecord, &currentRecordH);
   
   hasData = RecordContainsData(&currentRecord);
   
   // Unlock before the DeleteRecord.   We can only rely on
   // NULL pointers from here on out.   
   MemHandleUnlock(currentRecordH);


   // If none are the fields contained anything then 
   // delete the field.
   if (!hasData)
      {
      DeleteRecord(false);   // uniq ID wasted?  Yes. We don't care.
      return ListView;
      }

	
   // The record's category may have been changed.  The CurrentCategory
   // isn't supposed to change in this case.  Make sure the CurrentRecord
   // is still visible in this category or pick another one near it.
   if (!SeekRecord(&CurrentRecord, 0, dmSeekBackward))
      if (!SeekRecord(&CurrentRecord, 0, dmSeekForward))
         CurrentRecord = noRecord;
   
   
   ListViewSelectThisRecord = CurrentRecord;

   return ListView;
}


/***********************************************************************
 *
 * FUNCTION:    EditViewSelectCategory
 *
 * DESCRIPTION: This routine handles selection, creation and deletion of
 *              categories from the "Edit View".  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    The index of the new category.
 *
 *              The following global variables are modified:
 *                     CurrentCategory
 *                     ShowAllCategories
 *                     CategoryName
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   06/05/95   Initial Revision
 *			  gap	  08/13/99   Update to use new constant categoryDefaultEditCategoryString.
 *
 ***********************************************************************/
static void EditViewSelectCategory (void)
{
   UInt16 attr;
   FormPtr frm;
   UInt16 category;
   Boolean categoryEdited;

   
   // Process the category popup list.
   DmRecordInfo (AddrDB, CurrentRecord, &attr, NULL, NULL);
   category = attr & dmRecAttrCategoryMask;
   
   frm = FrmGetActiveForm();
   categoryEdited = CategorySelect (AddrDB, frm, EditCategoryTrigger,
               EditCategoryList, false, &category, CategoryName, 1, categoryDefaultEditCategoryString);
   
   if (categoryEdited || (category != (attr & dmRecAttrCategoryMask)))
      {
      // Change the category of the record.
      DmRecordInfo (AddrDB, CurrentRecord, &attr, NULL, NULL);   
      attr &= ~dmRecAttrCategoryMask;
      attr |= category | dmRecAttrDirty;
      DmSetRecordInfo (AddrDB, CurrentRecord, &attr, NULL);

      ChangeCategory (category);
      }
}

/***********************************************************************
 *
 * FUNCTION:    EditViewUpdateScrollers
 *
 * DESCRIPTION: This routine draws or erases the edit view scroll arrow
 *              buttons.
 *
 * PARAMETERS:  frm             -  pointer to the address edit form
 *              bottomField     -  field index of the last visible row
 *              lastItemClipped - true if the last visible row is clip at 
 *                                 the bottom
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/26/95   Initial Revision
 *
 ***********************************************************************/
static void EditViewUpdateScrollers (FormPtr frm, UInt16 bottomFieldIndex,
   Boolean lastItemClipped)
{
   UInt16 upIndex;
   UInt16 downIndex;
   Boolean scrollableUp;
   Boolean scrollableDown;
      
   // If the first field displayed is not the fist field in the record,
   // enable the up scroller.
   scrollableUp = TopVisibleFieldIndex > 0;

   // If the last field displayed is not the last field in the record,
   // enable the down scroller.
   scrollableDown = (lastItemClipped || (bottomFieldIndex < editLastFieldIndex));


   // Update the scroll button.
   upIndex = FrmGetObjectIndex (frm, EditUpButton);
   downIndex = FrmGetObjectIndex (frm, EditDownButton);
   FrmUpdateScrollers (frm, upIndex, downIndex, scrollableUp, scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    EditViewGetFieldHeight
 *
 * DESCRIPTION: This routine initialize a row in the to do list.
 *
 * PARAMETERS:  table        - pointer to the table of to do items
 *              fieldIndex   - the index of the field displayed in the row
 *              columnWidth  - height of the row in pixels
 *
 * RETURNED:    height of the field in pixels
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/26/95	Initial Revision
 *			art	9/11/97	Add font support.
 *
 ***********************************************************************/
static UInt16 EditViewGetFieldHeight (TablePtr table, UInt16 fieldIndex, Int16 columnWidth,
	Int16 maxHeight, AddrDBRecordPtr record, FontID * fontIdP)
{
	Int16 row;
	Int16 column;
	UInt16 index;
	Int16 height;
	UInt16 lineHeight;
	FontID currFont;
	Char * str;
	FieldPtr fld;

	if (TblEditing (table))
		{
		TblGetSelection (table, &row, &column);
		if (fieldIndex == TblGetRowID (table, row))
			{
			fld = TblGetCurrentField (table);
			str = FldGetTextPtr (fld);
			}
		else
			{
			index = FieldMap[fieldIndex];
			str = record->fields[index];
			}
		}
	else
		{
		index = FieldMap[fieldIndex];
		str = record->fields[index];
		}


	// If the field has text empty, or the field is the current field, or
	// the font used to display blank lines is the same as the font used
	// to display text then used the view's current font setting. 
	if ( (str && *str) || 
		  (CurrentFieldIndex == fieldIndex) ||
		  (AddrEditFont == addrEditBlankFont))
		{
		*fontIdP = AddrEditFont;
		currFont = FntSetFont (*fontIdP);
		}

	// If the height of the font used to display blank lines is the same 
	// height as the font used to display text then used the view's 
	// current font setting.
	else
		{
		currFont = FntSetFont (addrEditBlankFont);
		lineHeight = FntLineHeight ();
		
		FntSetFont (AddrEditFont);
		if (lineHeight == FntLineHeight ())
			*fontIdP = AddrEditFont;
		else
			{
			*fontIdP = addrEditBlankFont;
			FntSetFont (addrEditBlankFont);
			}
		}
		
	height = FldCalcFieldHeight (str, columnWidth);
	lineHeight = FntLineHeight ();
	height = min (height, (maxHeight / lineHeight));
	height *= lineHeight;

	FntSetFont (currFont);
		
		
	return (height);
}

/***********************************************************************
 *
 * FUNCTION:    EditInitTableRow
 *
 * DESCRIPTION: This routine initialize a row in the edit view.
 *
 * PARAMETERS:  table       - pointer to the table of to do items
 *              row         - row number (first row is zero)
 *              fieldIndex  - the index of the field displayed in the row
 *              rowHeight   - height of the row in pixels
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/26/95      Initial Revision
 *
 ***********************************************************************/
static void EditInitTableRow (TablePtr table, UInt16 row, UInt16 fieldIndex, 
	Int16 rowHeight, FontID fontID, AddrDBRecordPtr record, AddrAppInfoPtr appInfoPtr)
{

   // Make the row usable.
   TblSetRowUsable (table, row, true);
   
   // Set the height of the row to the height of the desc
   TblSetRowHeight (table, row, rowHeight);
   
   // Store the record number as the row id.
   TblSetRowID (table, row, fieldIndex);
   
   // Mark the row invalid so that it will draw when we call the 
   // draw routine.
   TblMarkRowInvalid (table, row);

	// Set the text font.
	TblSetItemFont (table, row, editDataColumn, fontID);

   // The label is either a text label or a popup menu (of phones)
   if (! isPhoneField(FieldMap[fieldIndex]))
      {      
      TblSetItemStyle (table, row, editLabelColumn, labelTableItem);
      TblSetItemPtr (table, row, editLabelColumn, 
         appInfoPtr->fieldLabels[FieldMap[fieldIndex]]);
      }
   else
      {
      // The label is a popup list
      TblSetItemStyle (table, row, editLabelColumn, popupTriggerTableItem);
      TblSetItemInt (table, row, editLabelColumn, GetPhoneLabel(record, FieldMap[fieldIndex]));
      TblSetItemPtr (table, row, editLabelColumn, 
         GetObjectPtr (EditPhoneList));
      }      
}


/***********************************************************************
 *
 * FUNCTION:    EditViewLoadTable
 *
 * DESCRIPTION: This routine reloads to do database records into
 *              the edit view.  This routine is called when:
 *                 o A field height changes (Typed text wraps to the next line)
 *                 o Scrolling
 *                 o Advancing to the next field causes scrolling
 *                 o The focus moves to another field
 *                 o A custom label changes
 *                 o The form is first opened.
 *
 *                The row ID is an index into FieldMap.
 *
 * PARAMETERS:  startingRow - index of the first row to redisplay.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/1/95	Initial Revision
 *			art	9/10/97	Rewrote to support user selectable fonts.
 *
 ***********************************************************************/
static void EditViewLoadTable (void)
{
	UInt16 row;
	UInt16 numRows;
	UInt16 lineHeight;
	UInt16 fieldIndex;
	UInt16 lastFieldIndex;
	UInt16 dataHeight;
	UInt16 tableHeight;
	UInt16 columnWidth;
	UInt16 pos, oldPos;
	UInt16 height, oldHeight;
	FontID fontID;
	FontID currFont;
	FormPtr frm;
	TablePtr table;
	Boolean rowUsable;
	Boolean rowsInserted = false;
	Boolean lastItemClipped;
	RectangleType r;
	AddrDBRecordType record;
	MemHandle recordH;
	AddrAppInfoPtr appInfoPtr;

	
	appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);
		
	frm = FrmGetActiveForm ();
	
	// Get the current record
	AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);

	// Get the height of the table and the width of the description
	// column.
	table = GetObjectPtr (EditTable);
	TblGetBounds (table, &r);
	tableHeight = r.extent.y;
	columnWidth = TblGetColumnWidth (table, editDataColumn);

	// If we currently have a selected record, make sure that it is not
	// above the first visible record.
	if (CurrentFieldIndex != noFieldIndex)
		{
		if (CurrentFieldIndex < TopVisibleFieldIndex)
			TopVisibleFieldIndex = CurrentFieldIndex;
		}

	row = 0;
	dataHeight = 0;
	oldPos = pos = 0;
	fieldIndex = TopVisibleFieldIndex;
	lastFieldIndex = fieldIndex;

	// Load records into the table.
	while (fieldIndex <= editLastFieldIndex)
		{		
		// Compute the height of the field's text string.
		height = EditViewGetFieldHeight (table, fieldIndex, columnWidth, tableHeight, &record, &fontID);

		// Is there enought room for at least one line of the the decription.
		currFont = FntSetFont (fontID);
		lineHeight = FntLineHeight ();
		FntSetFont (currFont);
		if (tableHeight >= dataHeight + lineHeight)
			{
			rowUsable = TblRowUsable (table, row);

			// Get the height of the current row.
			if (rowUsable)
				oldHeight = TblGetRowHeight (table, row);
			else
				oldHeight = 0;

			// If the field is not already being displayed in the current 
			// row, load the field into the table.
			if ((! rowUsable) ||
				 (TblGetRowID (table, row) != fieldIndex) ||
				 (TblGetItemFont (table, row, editDataColumn) != fontID))
				{
				EditInitTableRow (table, row, fieldIndex, height, fontID,
					&record, appInfoPtr);
				}
			
			// If the height or the position of the item has changed draw the item.
			else if (height != oldHeight)
				{
				TblSetRowHeight (table, row, height);
				TblMarkRowInvalid (table, row);
				}
			else if (pos != oldPos)
				{
				TblMarkRowInvalid (table, row);
				}

			pos += height;
			oldPos += oldHeight;
			lastFieldIndex = fieldIndex;
			fieldIndex++;
			row++;
			}

		dataHeight += height;


		// Is the table full?
		if (dataHeight >= tableHeight)		
			{
			// If we have a currently selected field, make sure that it is
			// not below the last visible field.  If the currently selected 
			// field is the last visible record, make sure the whole field 
			// is visible.
			if (CurrentFieldIndex == noFieldIndex)
				break;

			// Above last visible?
			else if  (CurrentFieldIndex < fieldIndex)
				break;

			// Last visible?
			else if (fieldIndex == lastFieldIndex)
				{
				if ((fieldIndex == TopVisibleFieldIndex) || (dataHeight == tableHeight))
					break;
				}	

			// Remove the top item from the table and reload the table again.
			TopVisibleFieldIndex++;
			fieldIndex = TopVisibleFieldIndex;


			// Below last visible.
//			else
//				TopVisibleFieldIndex = CurrentFieldIndex;
				
			row = 0;
			dataHeight = 0;
			oldPos = pos = 0;
			}
		}


	// Hide the item that don't have any data.
	numRows = TblGetNumberOfRows (table);
	while (row < numRows)
		{		
		TblSetRowUsable (table, row, false);
		row++;
		}
		
	// If the table is not full and the first visible field is 
	// not the first field	in the record, displays enough fields
	// to fill out the table by adding fields to the top of the table.
	while (dataHeight < tableHeight)
		{
		fieldIndex = TopVisibleFieldIndex;
		if (fieldIndex == 0) break;
		fieldIndex--;

		// Compute the height of the field.
		height = EditViewGetFieldHeight (table, fieldIndex, 
			columnWidth, tableHeight, &record, &fontID);

			
		// If adding the item to the table will overflow the height of
		// the table, don't add the item.
		if (dataHeight + height > tableHeight)
			break;
		
		// Insert a row before the first row.
		TblInsertRow (table, 0);

		EditInitTableRow (table, 0, fieldIndex, height, fontID, &record, appInfoPtr);
		
		TopVisibleFieldIndex = fieldIndex;
		
		rowsInserted = true;

		dataHeight += height;
		}
		
	// If rows were inserted to full out the page, invalidate the whole
	// table, it all needs to be redrawn.
	if (rowsInserted)
		TblMarkTableInvalid (table);

	// If the height of the data in the table is greater than the height
	// of the table, then the bottom of the last row is clip and the 
	// table is scrollable.
	lastItemClipped = (dataHeight > tableHeight);

	// Update the scroll arrows.
	EditViewUpdateScrollers (frm, lastFieldIndex, lastItemClipped);


	MemHandleUnlock(recordH);
	MemPtrUnlock(appInfoPtr);
}

/***********************************************************************
 *
 * FUNCTION:    EditViewDrawBusinessCardIndicator
 *
 * DESCRIPTION: Draw the business card indicator if the current record is
 * the business card.
 *
 * PARAMETERS:  formP - the form containing the business card indicator
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  12/3/97  Initial Revision
 *
 ***********************************************************************/
static void EditViewDrawBusinessCardIndicator (FormPtr formP)
{
	UInt32 uniqueID;
	
	DmRecordInfo (AddrDB, CurrentRecord, NULL, &uniqueID, NULL);
	if (BusinessCardRecordID == uniqueID)
		FrmShowObject(formP, FrmGetObjectIndex (formP, EditViewBusinessCardBmp));
	else
		FrmHideObject(formP, FrmGetObjectIndex (formP, EditViewBusinessCardBmp));
	
}   


/***********************************************************************
 *
 * FUNCTION:    EditViewResizeDescription
 *
 * DESCRIPTION: This routine is called when the height of address
 *              field is changed as a result of user input.
 *              If the new height of the field is shorter, more items
 *              may need to be added to the bottom of the list.
 *              
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/26/95      Initial Revision
 *
 ***********************************************************************/
static void EditViewResizeDescription (EventType * event)
{
	UInt16 pos;
	Int16 row;
	Int16 column;
	Int16 lastRow;
	UInt16 fieldIndex;
	UInt16 lastFieldIndex;
	UInt16 topFieldIndex;
	FieldPtr fld;
	TablePtr table;
	Boolean restoreFocus = false;
	Boolean lastItemClipped;
	RectangleType itemR;
	RectangleType tableR;
	RectangleType fieldR;


	// Get the current height of the field;
	fld = event->data.fldHeightChanged.pField;
	FldGetBounds (fld, &fieldR);

	// Have the table object resize the field and move the items below
	// the field up or down.
	table = GetObjectPtr (EditTable);
	TblHandleEvent (table, event);

	// If the field's height has expanded , we're done.
	if (event->data.fldHeightChanged.newHeight >= fieldR.extent.y)
	  {
	  topFieldIndex = TblGetRowID (table, 0);
	  if (topFieldIndex != TopVisibleFieldIndex)
	     TopVisibleFieldIndex = topFieldIndex;
	  else
	     {
	     // Since the table has expanded we may be able to scroll
	     // when before we might not have.
	     lastRow = TblGetLastUsableRow (table);
	     TblGetBounds (table, &tableR);
	     TblGetItemBounds (table, lastRow, editDataColumn, &itemR);
	     lastItemClipped = (itemR.topLeft.y + itemR.extent.y > 
	         tableR.topLeft.y + tableR.extent.y);
	     lastFieldIndex = TblGetRowID (table, lastRow);

	     EditViewUpdateScrollers (FrmGetActiveForm (), lastFieldIndex, 
	        lastItemClipped);
	     
	     return;
	     }
	  }

	// If the field's height has contracted and the field edit field
	// is not visible then the table may be scrolled.  Release the 
	// focus,  which will force the saving of the field we are editing.
	else if (TblGetRowID (table, 0) != editFirstFieldIndex)
		{
		TblGetSelection (table, &row, &column);
		fieldIndex = TblGetRowID (table, row);
		
		fld = TblGetCurrentField (table);
		pos = FldGetInsPtPosition (fld);
		TblReleaseFocus (table);

		restoreFocus = true;
		}

	// Add items to the table to fill in the space made available by the 
	// shorting the field.
	EditViewLoadTable ();
	TblRedrawTable (table);

	// Restore the insertion point position.
	if (restoreFocus)
		{
		TblFindRowID (table, fieldIndex, &row);
		TblGrabFocus (table, row, column);
		FldSetInsPtPosition (fld, pos);
		FldGrabFocus (fld);
		}
}


/***********************************************************************
 *
 * FUNCTION:    EditViewScroll
 *
 * DESCRIPTION: This routine scrolls the list of editable fields
 *              in the direction specified.
 *
 * PARAMETERS:  direction - up or dowm
 *              oneLine   - if true the list is scroll by a single line,
 *                          if false the list is scroll by a full screen.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   06/26/95	Initial Revision
 *         vmk   02/20/98	Move TblUnhighlightSelection before EditViewLoadTable
 *         gap   10/12/99	Close command bar before processing scroll
 *
 ***********************************************************************/
static void EditViewScroll (WinDirectionType direction)
{
   UInt16				row;
   UInt16				height;
   UInt16				fieldIndex;
   UInt16				columnWidth;
   UInt16				tableHeight;
   TablePtr				table;
   FontID				curFont;
   RectangleType		r;
   AddrDBRecordType	record;
   MemHandle			recordH;


	// Before processing the scroll, be sure that the command bar has been closed.
	MenuEraseStatus (0);

   curFont = FntSetFont (stdFont);
   
   table = GetObjectPtr (EditTable);
   TblReleaseFocus (table);

   // Get the height of the table and the width of the description
   // column.
   TblGetBounds (table, &r);
   tableHeight = r.extent.y;
   height = 0;
   columnWidth = TblGetColumnWidth (table, editDataColumn);

   // Scroll the table down.
   if (direction == winDown)
      {
      // Get the index of the last visible field, this will become 
      // the index of the top visible field, unless it occupies the 
      // whole screeen, in which case the next field will be the
      // top filed.
      
      row = TblGetLastUsableRow (table);
      fieldIndex = TblGetRowID (table, row);
      
      // If the last visible field is also the first visible field
      // then it occupies the whole screeen.
      if (row == 0)
         fieldIndex = min (editLastFieldIndex, fieldIndex+1);
      }

   // Scroll the table up.
   else
      {
      // Scan the fields before the first visible field to determine 
      // how many fields we need to scroll.  Since the heights of the 
      // fields vary,  we sum the height of the records until we get
      // a screen full.

      fieldIndex = TblGetRowID (table, 0);
      ErrFatalDisplayIf(fieldIndex > editLastFieldIndex, "Invalid field Index");
      if (fieldIndex == 0)
         goto exit;
         
      // Get the current record
      AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);

      height = TblGetRowHeight (table, 0);
      if (height >= tableHeight)
         height = 0;                     

      while (height < tableHeight && fieldIndex > 0)
         {
         height += FldCalcFieldHeight (record.fields[FieldMap[fieldIndex-1]], 
            columnWidth) * FntLineHeight ();
         if ((height <= tableHeight) || (fieldIndex == TblGetRowID (table, 0)))
            fieldIndex--;
         }
      MemHandleUnlock(recordH);
      }

   TblMarkTableInvalid (table);
	CurrentFieldIndex = noFieldIndex;
   TopVisibleFieldIndex = fieldIndex;
   EditRowIDWhichHadFocus = editFirstFieldIndex;
   EditFieldPosition = 0;

   TblUnhighlightSelection (table);		// remove the highlight before reloading the table to avoid
   												// having an out of bounds selection information in case
   												// the newly loaded data doesn't have as many rows as the old
   												// data.  This fixes the bug
   												// "File: Table.c, Line: 2599, currentRow violated constraint!"
   												// (fix suggested by Art) vmk 2/20/98
   EditViewLoadTable ();   

   //TblUnhighlightSelection (table);	// moved call before EditViewLoadTable vmk 2/20/98
   TblRedrawTable (table);

exit:   
   FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    EditViewHandleSelectField
 *
 * DESCRIPTION: Handle the user tapping an edit view field label.
 *   Either the a phone label is changed or the user wants to edit
 * a field by tapping on it's label.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and nothing else should
 *                be done
 *
 * REVISION HISTORY:
 *       Name   Date		Description
 *       ----	----		-----------
 *		roger	11/27/95	Cut from EditViewHandleEvent
 *		art		9/2/97		Add multi-font support
 *		roger	11/4/97		Changed parameters to support another routine
 *
 ***********************************************************************/
static Boolean EditViewHandleSelectField (Int16 row, Int8 column)
{
	Err					err;
	Int16					currRow;
	UInt16				fieldNum;
	UInt16				fieldIndex;
	MemHandle				currentRecordH;
	UInt16				i;
	UInt16				currentField;
	FontID				currFont;
	Boolean				redraw = false;
	FormPtr				formP;
	TablePtr			tableP;
	FieldPtr			fldP;
	AddrDBRecordType	currentRecord;
	AddrDBRecordFlags	changedFields;
	
	
	formP = FrmGetActiveForm();
	tableP = GetObjectPtr(EditTable);
	
	// If a phone label was changed then modify the record
	currentField = FieldMap[TblGetRowID(tableP, row)];
	if (column == editLabelColumn)
		{
		if (isPhoneField(currentField))
			{
			i = TblGetItemInt(tableP, row, editLabelColumn);
			AddrGetRecord(AddrDB, CurrentRecord, &currentRecord, &currentRecordH);
			
			switch (currentField)
				{
				case firstPhoneField:
					currentRecord.options.phones.phone1 = i;
					break;

				case firstPhoneField + 1:
					currentRecord.options.phones.phone2 = i;
					break;

				case firstPhoneField + 2:
					currentRecord.options.phones.phone3 = i;
					break;

				case firstPhoneField + 3:
					currentRecord.options.phones.phone4 = i;
					break;

				case firstPhoneField + 4:
					currentRecord.options.phones.phone5 = i;
					break;
				}

			changedFields.allBits = 0;
			err = AddrChangeRecord(AddrDB, &CurrentRecord, &currentRecord,
				changedFields);
			if (err)
				{
				MemHandleUnlock(currentRecordH);
				FrmAlert(DeviceFullAlert);

				// Redraw the table without the change.  The phone label
				// is unchanged in the record but the screen and the table row
				// are changed.  Reinit the table to fix it.  Mark the row
				// invalid and redraw it.
				EditViewInit (FrmGetActiveForm(), false);
				TblMarkRowInvalid(tableP, row);
				TblRedrawTable(tableP);

				return true;
				}
			}

		// The user selected the label of a field.
		// The user probably wants to edit it so
		// set the table to edit the field to the right of the label.
		
		TblReleaseFocus(tableP);
		TblUnhighlightSelection(tableP);
//		setFocus = true;
		}
		

	// Make sure the the heights the the field we are exiting and the 
	// that we are entering are correct.  They may be incorrect if the 
	// font used to display blank line is a different height then the
	// font used to display field text.
	fieldIndex = TblGetRowID (tableP, row);

	if (fieldIndex != CurrentFieldIndex || TblGetCurrentField(tableP) == NULL)
		{
		AddrGetRecord (AddrDB, CurrentRecord, &currentRecord, &currentRecordH);

		currFont = FntGetFont ();

		// Is there a current field and is it empty?
		if (CurrentFieldIndex != noFieldIndex && 
			!currentRecord.fields[FieldMap[CurrentFieldIndex]])
			{
			if (TblFindRowID (tableP, CurrentFieldIndex, &currRow))
				{
				// Is the height of the field correct?
				FntSetFont (addrEditBlankFont);
				if (FntLineHeight () != TblGetRowHeight (tableP, currRow))
					{
					TblMarkRowInvalid (tableP, currRow);
					redraw = true;
					}
				}
			}

		CurrentFieldIndex = fieldIndex;

		// Is the newly selected field empty?
		fieldNum = FieldMap[fieldIndex];
		if (! currentRecord.fields[fieldNum])
			{
			// Is the height of the field correct?
			FntSetFont (AddrEditFont);
			if (FntLineHeight () != TblGetRowHeight (tableP, row))
				{
				TblMarkRowInvalid (tableP, row);
				redraw = true;
				}
			}
		
		// Do before the table focus is released and the record is saved.
		MemHandleUnlock (currentRecordH);
		
		if (redraw)
			{
			TblReleaseFocus (tableP);
			EditViewLoadTable ();
			TblFindRowID (tableP, fieldIndex, &row);

			TblRedrawTable (tableP);
//			setFocus = true;
			}

		FntSetFont (currFont);
		}


//	if (TblGetCurrentField(tableP) != NULL)
//		TblReleaseFocus (tableP);
	
	// Set the focus
	if (TblGetCurrentField(tableP) == NULL)
		{
		FrmSetFocus(formP, FrmGetObjectIndex(formP, EditTable));
		TblGrabFocus (tableP, row, editDataColumn);
		fldP = TblGetCurrentField(tableP);
		FldGrabFocus (fldP);
		FldMakeFullyVisible (fldP);
		}

	
	return false;
	
}


/***********************************************************************
 *
 * FUNCTION:    EditViewNextField
 *
 * DESCRIPTION: If a field is being edited, advance the focus to the
 * edit view table's next field.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   7/27/95   Initial Revision
 *
 ***********************************************************************/
static void EditViewNextField (WinDirectionType direction)
{
   TablePtr tableP;
   Int16 row;
   Int16 column;
   UInt16 nextFieldNumIndex;
   
   
   tableP = GetObjectPtr (EditTable);
   
   if (!TblEditing(tableP))
      return;
   
   // Find out which field is being edited.
   TblGetSelection (tableP, &row, &column);
   nextFieldNumIndex = TblGetRowID (tableP, row);
   if (direction == winDown)
      {
      if (nextFieldNumIndex >= editLastFieldIndex)
         nextFieldNumIndex = 0;
      else
         nextFieldNumIndex++;
      }
   else
      {
      if (nextFieldNumIndex == 0)
         nextFieldNumIndex = editLastFieldIndex;
      else
         nextFieldNumIndex--;
      }
   TblReleaseFocus (tableP);

	CurrentFieldIndex = nextFieldNumIndex;

   // If the new field isn't visible move the edit view and then
   // find the row where the next field is.
   while (!TblFindRowID(tableP, nextFieldNumIndex, &row))
      {
      // Scroll the view down placing the item
      // on the top row
      TopVisibleFieldIndex = nextFieldNumIndex;
      EditViewLoadTable();
      TblRedrawTable(tableP);
      }

   
   // Select the item
#if 0
   TblGrabFocus (tableP, row, editDataColumn);
   fldP = TblGetCurrentField (tableP);
   FldSetInsPtPosition (fldP, 0);
   FldGrabFocus (fldP);
   FldMakeFullyVisible (fldP);
#endif

   EditViewHandleSelectField(row, editDataColumn);
}




/***********************************************************************
 *
 * FUNCTION:    EditViewUpdateCustomFieldLabels
 *
 * DESCRIPTION: Update the custom field labels by reloading those rows
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
static void EditViewUpdateCustomFieldLabels ()
{
   FormPtr frm;
   UInt16 row;
   UInt16 rowsInTable;
   TablePtr table;
   AddrAppInfoPtr appInfoPtr;
   UInt16 fieldIndex;
   UInt16 fieldNum;
   AddrDBRecordType record;
   MemHandle recordH;
   Boolean redraw = false;


   appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);
   frm = FrmGetActiveForm ();
   table = GetObjectPtr (EditTable);

   if (TblGetColumnWidth(table, editLabelColumn) != EditLabelColumnWidth)
      {
      EditViewInit (frm, false);
      redraw = true;
      }
   else
      {
      
   
      // Get the current record
      AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);
   
      
      rowsInTable = TblGetNumberOfRows(table);
   
      // Reload any renameable fields
      for (row = 0; row < rowsInTable; row++)
         {
         if (TblRowUsable (table, row))
            {
            fieldIndex = TblGetRowID (table, row);
            fieldNum = FieldMap[fieldIndex];
            if (fieldNum >= firstRenameableLabel &&
               fieldNum <= lastRenameableLabel)
               {
					EditInitTableRow (table, row, fieldIndex, 
						TblGetRowHeight (table, row),
						TblGetItemFont (table, row, editDataColumn),
						&record, appInfoPtr);
               redraw = true;
   
               // Mark the row invalid so that it will draw when we call the 
               // draw routine.
               TblMarkRowInvalid (table, row);
               }
            }
         }
   
   
      MemHandleUnlock(recordH);
      }
      
      
   if (redraw)
      TblRedrawTable(table);

      
   MemPtrUnlock(appInfoPtr);
}


/***********************************************************************
 *
 * FUNCTION:    EditViewUpdateDisplay
 *
 * DESCRIPTION: This routine update the display of the edit view
 *
 * PARAMETERS:  updateCode - a code that indicated what changes  been
 *                           have made to the view.
 *                		
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/12/97	Initial Revision
 *			jmp	11/02/99	Fixed problem on frmRedrawUpdateCode events when
 *								we're still in the edit state but we weren't redrawing
 *								the edit indicator.  Fixes Address part of bug #23235.
 *			jmp	12/09/99	Fix bug #23914:  On frmRedrawUpdateCode, don't reload
 *								the table, just redraw it!
 *
 ***********************************************************************/
static Boolean EditViewUpdateDisplay (UInt16 updateCode)
{
	TablePtr table;
	Boolean handled = false;

	if (updateCode & updateCustomFieldLabelChanged)
		{
		EditViewUpdateCustomFieldLabels();
		handled = true;
		}

	if (updateCode & updateCategoryChanged)
		{
		// Set the label of the category trigger.
		CategoryGetName (AddrDB, CurrentCategory, CategoryName);
		CategorySetTriggerLabel (GetObjectPtr (EditCategoryTrigger), CategoryName);
		handled = true;
		}

	// Perform common tasks as necessary, and in the proper order.
	if ((updateCode & updateFontChanged) || (updateCode & updateGrabFocus) || (updateCode & frmRedrawUpdateCode))
		{
		if (updateCode & frmRedrawUpdateCode)
			FrmDrawForm (FrmGetActiveForm ());
		
		if ((updateCode & updateFontChanged) || (updateCode & frmRedrawUpdateCode))
			table = GetObjectPtr (EditTable);
		
		if (updateCode & updateFontChanged)
			TblReleaseFocus (table);
		
		if (updateCode & frmRedrawUpdateCode)
			{
			// If we're editing, then find out which row is being edited,
			// mark it invalid, and redraw the table (below).
			if (TblEditing(table))
				{
				Int16 row;
				Int16 column;
				
				TblGetSelection (table, &row, &column);
				TblMarkRowInvalid(table, row);
				TblRedrawTable (table);
				}
			}
		
		if ((updateCode & updateFontChanged))
			{
			EditViewLoadTable ();
			TblRedrawTable (table);
			}
		
		if ((updateCode & updateFontChanged) || (updateCode & updateGrabFocus))
			EditViewRestoreEditState ();
		
		handled = true;
		}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    EditViewDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  6/27/95   Initial Revision
 *         jmp    9/17/99   Use NoteView instead of NoteView.
 *
 ***********************************************************************/
static Boolean EditViewDoCommand (UInt16 command)
{
	AddrDBRecordType record;
	MemHandle recordH;
	Boolean hasNote;
	UInt16 newRecord;
	UInt16 numCharsToHilite;
	Boolean hasData;

	switch (command)
		{
		case EditRecordDeleteRecordCmd:
			MenuEraseStatus (0);
			FrmSetFocus(FrmGetActiveForm(), noFocus);   // save the field
			if (DetailsDeleteRecord ())
				FrmGotoForm (ListView);
			else
				EditViewRestoreEditState ();
			return true;

	  	case EditRecordDuplicateAddressCmd:
			// Make sure the field being edited is saved
			FrmSetFocus(FrmGetActiveForm(), noFocus);

			AddrGetRecord(AddrDB, CurrentRecord, &record, &recordH);

			hasData = RecordContainsData(&record);
			MemHandleUnlock(recordH);
			
			newRecord = DuplicateCurrentRecord (&numCharsToHilite, !hasData);
			if (newRecord != noRecord)
				{
				NumCharsToHilite = numCharsToHilite;
				CurrentRecord = newRecord;
				FrmGotoForm (EditView);
				}
	  	  return true;

		case EditRecordAttachNoteCmd:
			MenuEraseStatus (0);
			TblReleaseFocus(GetObjectPtr(EditTable));   // save the field
			if (CreateNote())
				FrmGotoForm (NewNoteView);
			return true;

		case EditRecordDeleteNoteCmd:
			MenuEraseStatus (0);
			AddrGetRecord (AddrDB, CurrentRecord, &record, &recordH);
			hasNote = (record.fields[note] != NULL);
			MemHandleUnlock(recordH);

			if (hasNote && FrmAlert(DeleteNoteAlert) == DeleteNoteYes)
				DeleteNote ();
			return true;

		case EditRecordSelectBusinessCardCmd:
			MenuEraseStatus (0);
			if (FrmAlert(SelectBusinessCardAlert) == SelectBusinessCardYes)
				{
				DmRecordInfo (AddrDB, CurrentRecord, NULL, &BusinessCardRecordID, NULL);
				EditViewDrawBusinessCardIndicator (FrmGetActiveForm());
				}
			return true;

		case EditRecordSendBusinessCardCmd:
			// Make sure the field being edited is saved
			FrmSetFocus(FrmGetActiveForm(), noFocus);

			MenuEraseStatus (0);
			AddrSendBusinessCard(AddrDB);
			return true;

		case EditRecordSendRecordCmd:
			// Make sure the field being edited is saved
			FrmSetFocus(FrmGetActiveForm(), noFocus);

			MenuEraseStatus (0);
			AddrSendRecord(AddrDB, CurrentRecord);
			return true;

		case EditOptionsFontCmd:
			MenuEraseStatus (0);
			AddrEditFont = SelectFont (AddrEditFont);
			return true;

		case EditOptionsEditCustomFldsCmd:
			MenuEraseStatus (0);
			FrmSetFocus(FrmGetActiveForm(), noFocus);   // save the field
			FrmPopupForm (CustomEditDialog);
			return true;

		case EditOptionsAboutCmd:
			MenuEraseStatus (0);
			AbtShowAbout (sysFileCAddress);
			return true;

		default:
			break;
		}

	return false;
}


/***********************************************************************
 *
 * FUNCTION:    EditLabelColumnWidthInit
 *
 * DESCRIPTION: Set EditLabelColumnWidth to the width of the widest
 * field label plus a ':'.  This routine should be called once per run.
 *
 * PARAMETERS:  appInfoPtr - pointer to the app info block for field labels
 *
 * RETURNED:    EditLabelColumnWidth is set
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   8/3/95   Initial Revision
 *
 ***********************************************************************/
/* remove 1/30/98 Art

static void EditLabelColumnWidthInit (AddrAppInfoPtr appInfoPtr)
{
   Int16 maxWidth, i;
   FontID curFont;
   UInt16 labelWidth;      // Width of a field label
   UInt16 columnWidth;      // Width of the label column (fits all label)
   UInt16 tableWidth;      // Width of the label and data columns 
   
   
   ErrNonFatalDisplayIf(EditLabelColumnWidth != 0, "EditLabelColumnWidth already set");
   
   // no wider than the window
   WinGetWindowExtent ((Int16 *) &maxWidth, (Int16 *) &i);

   
   // Calculate EditLabelColumnWidth which is used by the Record View as
   // the amount of indentation.
   curFont = FntSetFont (stdFont);
   if (EditLabelColumnWidth == 0)
      {
      tableWidth = maxWidth - spaceBeforeDesc;      // less the table column gutter
      columnWidth = 0;
          
      for (i = firstAddressField; i < lastLabel; i ++)
         {
         labelWidth = FntCharsWidth(appInfoPtr->fieldLabels[i], 
            StrLen(appInfoPtr->fieldLabels[i]));
         columnWidth = max(columnWidth, labelWidth);
         }
      columnWidth += 1 + FntCharWidth(':');
      
      EditLabelColumnWidth = columnWidth;
      EditDataColumnWidth = tableWidth - columnWidth;
      }
   
   FntSetFont (curFont);
}
*/


/***********************************************************************
 *
 * FUNCTION:    EditViewInit
 *
 * DESCRIPTION: This routine initializes the "Edit View" of the 
 *              Address application.
 *
 * PARAMETERS:	frm					Pointer to the Edit form structure
 *					leaveDataLocked	T=>keep app info, form map data locked.
 *
 * RETURNED:	nothing
 *
 *	HISTORY:
 *		06/05/99	art	Created by Art Lamb.
 *		07/29/99	kwk	Set up locked FieldMap pointer.
 *
 ***********************************************************************/
static void EditViewInit (FormPtr frm, Boolean leaveDataLocked)
	{
	UInt16 attr;
	UInt16 row;
	UInt16 rowsInTable;
	UInt16 category;
	UInt16 dataColumnWidth;
	TablePtr table;
	AddrAppInfoPtr appInfoPtr;
	ListPtr popupPhoneList;
	FontID   currFont;
	RectangleType bounds;


	currFont = FntSetFont (stdFont);
	appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);
	FieldMap = (const AddressFields*)MemHandleLock(DmGetResource(fieldMapRscType, FieldMapID));

	// Set the choices to the phone list
	EditPhoneListChoices[0] = appInfoPtr->fieldLabels[firstPhoneField];
	EditPhoneListChoices[1] = appInfoPtr->fieldLabels[firstPhoneField + 1];
	EditPhoneListChoices[2] = appInfoPtr->fieldLabels[firstPhoneField + 2];
	EditPhoneListChoices[3] = appInfoPtr->fieldLabels[firstPhoneField + 3];
	EditPhoneListChoices[4] = appInfoPtr->fieldLabels[firstPhoneField + 4];
	EditPhoneListChoices[5] = appInfoPtr->fieldLabels[addressFieldsCount];
	EditPhoneListChoices[6] = appInfoPtr->fieldLabels[addressFieldsCount + 1];
	EditPhoneListChoices[7] = appInfoPtr->fieldLabels[addressFieldsCount + 2];
	popupPhoneList = GetObjectPtr (EditPhoneList);
	LstSetListChoices(popupPhoneList, EditPhoneListChoices, numPhoneLabels);
	LstSetHeight (popupPhoneList, numPhoneLabels);



	// Initialize the address list table.
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, EditTable));
	rowsInTable = TblGetNumberOfRows (table);
	for (row = 0; row < rowsInTable; row++)
	  {
	  // This sets the data column
	  TblSetItemStyle (table, row, editDataColumn, textTableItem);
	  TblSetRowUsable (table, row, false);
	  }

	TblSetColumnUsable (table, editLabelColumn, true);
	TblSetColumnUsable (table, editDataColumn, true);

	TblSetColumnSpacing (table, editLabelColumn, spaceBeforeDesc);


	// Set the callback routines that will load and save the 
	// description field.
	TblSetLoadDataProcedure (table, editDataColumn, EditViewGetRecordField);
	TblSetSaveDataProcedure (table, editDataColumn, EditViewSaveRecordField);


	// Set the column widths so that the label column contents fit exactly.
	// Those labels change as the country changes.
	if (EditLabelColumnWidth == 0)
		EditLabelColumnWidth = GetLabelColumnWidth (appInfoPtr, stdFont);
		
	// Compute the width of the data column, account for the table column gutter.
	TblGetBounds (table, &bounds);
	dataColumnWidth = bounds.extent.x - spaceBeforeDesc - EditLabelColumnWidth;

	TblSetColumnWidth(table, editLabelColumn, EditLabelColumnWidth);
	TblSetColumnWidth(table, editDataColumn, dataColumnWidth);
	  

	EditViewLoadTable ();


	// Set the label of the category trigger.
	if (CurrentCategory == dmAllCategories)
	  {
	  DmRecordInfo (AddrDB, CurrentRecord, &attr, NULL, NULL);   
	  category = attr & dmRecAttrCategoryMask;
	  }
	else 
	  category = CurrentCategory;
	CategoryGetName (AddrDB, category, CategoryName);
	CategorySetTriggerLabel ( GetObjectPtr (EditCategoryTrigger), CategoryName);


	FntSetFont (currFont);

	// if the caller is using us to reset the form, then we don't want
	// to repeatedly lock down the app info block.
	if (!leaveDataLocked)
		{
		MemPtrUnlock(appInfoPtr);
		MemPtrUnlock((MemPtr)FieldMap);
		}
	
	// In general, the record isn't needed after this form is closed.
	// It is if the user is going to the Note View.  In that case
	// we must keep the record.
	RecordNeededAfterEditView = false;
}


/***********************************************************************
 *
 * FUNCTION:    RecordViewAutoFill
 *
 * DESCRIPTION: This routine handles auto-filling the vendor or city 
 *              fields.
 *
 * PARAMETERS:  event  - pointer to a keyDownEvent.
 *
 * RETURNED:    true if the event has been handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	01/15/96		Initial Revision
 *			trm	07/20/97		Fixed Autofill bug
 *			kwk	11/20/98		Check for command key, return false if set.
 *			CSS	06/22/99		Standardized keyDownEvent handling
 *									(TxtCharIsHardKey, commandKeyMask, etc.)
 *			bhall	07/12/99		Modified from Expense.c/DetailsAutoFill
 *			bob	11/17/99		double check that table has a field before using it
 *
 ***********************************************************************/
static Boolean EditViewAutoFill (EventPtr event)
{
	UInt16		index;
	UInt16		focus;
	Char			*key;
	UInt32		dbType;
	FormPtr		frm;
	FieldPtr		fld;
	DmOpenRef	dbP;
	TablePtr		table;
   UInt16 		fieldIndex;
   UInt16		fieldNum;
	
	if	(	TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr)
		||	(EvtKeydownIsVirtual(event))
		|| (!TxtCharIsPrint(event->data.keyDown.chr)))
		return false;

	frm = FrmGetActiveForm();
	focus = FrmGetFocus(frm);
	if (focus == noFocus)
		return false;

	// Find the table
	table = GetObjectPtr(EditTable);
	
	// check to see if there really is a field before continuing.
	// in case table has stopped editing but form still thinks table is the
	// focus.
	if (TblGetCurrentField(table) == NULL)
		return false;

   // Get the field number that corresponds to the table item to save.
   {
   	Int16 row;
		Int16 column;

		TblGetSelection(table, &row, &column);
		fieldIndex = TblGetRowID(table, row);
	}
	fieldNum = FieldMap[fieldIndex];

	// Select the proper database for the field we are editing,
	// or return right away if not an autofill enabled field
	switch (fieldNum) {
		case title:		dbType = titleDBType; break;
		case company:	dbType = companyDBType; break;
		case city:		dbType = cityDBType; break;
		case state:		dbType = stateDBType; break;
		case country:	dbType = countryDBType; break;
		default:	return false;
	}
	
	// Let the OS insert the character into the field.
	FrmHandleEvent(frm, event);

	// Open the database
	dbP = DmOpenDatabaseByTypeCreator(dbType, sysFileCAddress, dmModeReadOnly);
	if (!dbP) return true;

	// The current value of the field with the focus.
	fld = TblGetCurrentField(table);
	key = FldGetTextPtr(fld);

	// Check for a match.
	if (LookupStringInDatabase(dbP, key, &index)) {
		MemHandle				h;
		UInt16				pos;
		LookupRecordPtr	r;
		UInt16				len;
		Char 					*ptr;

		pos = FldGetInsPtPosition(fld);

		h = DmQueryRecord(dbP, index);
		r = MemHandleLock(h);

		// Auto-fill.
		ptr = &r->text + StrLen (key);
		len = StrLen(ptr);

		FldInsert(fld, ptr, StrLen(ptr));
		
		// Highlight the inserted text.
		FldSetSelection(fld, pos, pos + len);

		MemHandleUnlock(h);
	}
		
	// Close the database
	DmCloseDatabase(dbP);

	return true;
}


/***********************************************************************
 *
 * FUNCTION:    EditViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Edit View"
 *              of the Address Book application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 *	HISTORY:
 *		06/05/95	art	Created by Art Lamb.
 *		07/29/99	kwk	Unlock FieldMap in frmCloseEvent block.
 *		09/17/99 jmp	Use NewNoteView instead of NoteView.
 *
 ***********************************************************************/
static Boolean EditViewHandleEvent (EventType * event)
{
   FormPtr frm;
   TablePtr tableP;
   Boolean handled = false;
   Int16 row;


   switch (event->eType)
   	{
      case keyDownEvent:
         if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
         	{
            TblReleaseFocus(GetObjectPtr(EditTable));
            TopVisibleRecord = 0;      // Same as when app switched to
				CurrentFieldIndex = noFieldIndex;
            FrmGotoForm (ListView);
            return (true);
         	}

			else if (EvtKeydownIsVirtual(event))
				{
				switch (event->data.keyDown.chr)
					{
					case vchrPageUp:
						EditViewScroll (winUp);
						handled = true;
						break;
						
					case vchrPageDown:
						EditViewScroll (winDown);
						handled = true;
						break;
	
					case vchrNextField:
						EditViewNextField (winDown);
						handled = true;
						break;
	
					case vchrPrevField:
						EditViewNextField (winUp);
						handled = true;
						break;
					
					case vchrSendData:
						// Make sure the field being edited is saved
						frm = FrmGetActiveForm ();
						tableP = FrmGetObjectPtr (frm, FrmGetObjectIndex(frm, EditTable));
						TblReleaseFocus(tableP);
						
						MenuEraseStatus (0);
						AddrSendRecord(AddrDB, CurrentRecord);
						handled = true;
						break;
					
					default:
						break;
					}
				}
			else
				{
				handled = EditViewAutoFill(event);
				}

         break;


      case ctlSelectEvent:
         switch (event->data.ctlSelect.controlID){
            case EditCategoryTrigger:
               EditViewSelectCategory ();
 					EditViewRestoreEditState ();	// DOLATER: Can cause problems when no field is editable
              handled = true;
               break;

            case EditDoneButton:
               FrmGotoForm (ListView);
               handled = true;
               break;

            case EditDetailsButton:
               FrmPopupForm (DetailsDialog);
               handled = true;
               break;

            case EditNoteButton:
               if (CreateNote())
                  {
                  RecordNeededAfterEditView = true;
                  FrmGotoForm (NewNoteView);
                  }
               handled = true;
               break;
            default:
               break;

         }
         break;

      case ctlRepeatEvent:
         switch (event->data.ctlRepeat.controlID){
            case EditUpButton:
               EditViewScroll (winUp);
               // leave unhandled so the buttons can repeat
               break;
               
            case EditDownButton:
               EditViewScroll (winDown);
               // leave unhandled so the buttons can repeat
               break;
            default:
               break;
         }
         break;

      case tblSelectEvent:
      	// Select the field if it's different than the one selected before.  This means the selection
      	// is on a different row or the selection is a phone label.
      	if (CurrentFieldIndex != TblGetRowID (event->data.tblSelect.pTable, event->data.tblSelect.row) ||
      		(event->data.tblSelect.column == editLabelColumn && isPhoneField(FieldMap[
      			TblGetRowID(event->data.tblSelect.pTable, event->data.tblSelect.row)])))
	         EditViewHandleSelectField (event->data.tblSelect.row, event->data.tblSelect.column);
         break;

      case menuEvent:
         return EditViewDoCommand (event->data.menu.itemID);

		case menuCmdBarOpenEvent:
			{
			FieldPtr fld;
			UInt16 startPos, endPos;
			
			fld = TblGetCurrentField(GetObjectPtr(EditTable));
			if (fld)
				FldGetSelection(fld, &startPos, &endPos);
			
			if ((fld) && (startPos != endPos))  // if there's some highlighted text
				{
				// (Note that we're adding buttons from right to left)
				MenuCmdBarAddButton(menuCmdBarOnLeft, BarPasteBitmap, menuCmdBarResultMenuItem, sysEditMenuPasteCmd, 0);
				MenuCmdBarAddButton(menuCmdBarOnLeft, BarCopyBitmap, menuCmdBarResultMenuItem, sysEditMenuCopyCmd, 0);
				MenuCmdBarAddButton(menuCmdBarOnLeft, BarCutBitmap, menuCmdBarResultMenuItem, sysEditMenuCutCmd, 0);
				MenuCmdBarAddButton(menuCmdBarOnLeft, BarUndoBitmap, menuCmdBarResultMenuItem, sysEditMenuUndoCmd, 0);
				}
			else 	// there's no highlighted text
				{
				MenuCmdBarAddButton(menuCmdBarOnLeft, BarDeleteBitmap, menuCmdBarResultMenuItem, EditRecordDeleteRecordCmd, 0);
				if (fld)
					{
					MenuCmdBarAddButton(menuCmdBarOnLeft, BarPasteBitmap, menuCmdBarResultMenuItem, sysEditMenuPasteCmd, 0);
					MenuCmdBarAddButton(menuCmdBarOnLeft, BarUndoBitmap, menuCmdBarResultMenuItem, sysEditMenuUndoCmd, 0);
					}
				MenuCmdBarAddButton(menuCmdBarOnLeft, BarBeamBitmap, menuCmdBarResultMenuItem, EditRecordSendRecordCmd, 0);
				}

			// tell the field package to not add cut/copy/paste buttons automatically
			event->data.menuCmdBarOpen.preventFieldButtons = true;

			// don't set handled to true; this event must fall through to the system.
			break;
			}

      
      case fldHeightChangedEvent:
         EditViewResizeDescription (event);
         handled = true;
         break;
      
      
      case frmUpdateEvent:
         handled = EditViewUpdateDisplay (event->data.frmUpdate.updateCode);
         break;

      case frmCloseEvent:
         {
            AddrAppInfoPtr appInfoPtr;
            
            // Check if the record is empty and should be deleted.  This cannot
            // be done earlier because if the record is deleted there is nothing
            // to display in the table.
            EditViewSaveRecord ();

            // We need to unlock the block containing the phone labels.   
            appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);
            MemPtrUnlock(appInfoPtr);	// Call to AddrAppInfoGetPtr did a lock
            MemPtrUnlock(appInfoPtr);   // Unlock lock in EditViewInit
            
            // We need to unlock the FieldMap resource, which was also locked
            // in EditViewInit.
            MemPtrUnlock((MemPtr)FieldMap);
            FieldMap = NULL;
         }
         break;

         
      case frmSaveEvent:
       	// Save the field being edited.  Do not delete the record if it's
         // empty because a frmSaveEvent can be sent without the form being
         // closed.  A canceled find does this.

			frm = FrmGetFormPtr (EditView);
			tableP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, EditTable));
         TblReleaseFocus(tableP);
         break;

         
      case frmOpenEvent:
         {
            UInt16 tableIndex;
            FieldPtr fldP;
			
            frm = FrmGetActiveForm ();
            EditViewInit (frm, true);
            tableIndex = FrmGetObjectIndex(frm, EditTable);
            tableP = FrmGetObjectPtr (frm, tableIndex);
            
            // Make sure the field which will get the focus is visible
            while (!TblFindRowID (tableP, EditRowIDWhichHadFocus, &row))
               {
               TopVisibleFieldIndex = EditRowIDWhichHadFocus;
					CurrentFieldIndex = EditRowIDWhichHadFocus;
               EditViewLoadTable();
               }
            FrmDrawForm (frm);
				EditViewDrawBusinessCardIndicator (frm);
			         
            // Now set the focus.
            FrmSetFocus(frm, tableIndex);
            TblGrabFocus (tableP, row, editDataColumn);
            fldP = TblGetCurrentField(tableP);
            FldGrabFocus (fldP);
            
            // If NumCharsToHilite is not 0, then we know that we are displaying
            // a duplicated message for the first time and we must hilite the last
            // NumCharsToHilite of the field (first name) to indicate the modification
            // to that duplicated field.
            if (NumCharsToHilite > 0)
					{
					EditFieldPosition = FldGetTextLength (fldP);
					
					// Now hilite the chars added.
					FldSetSelection (fldP, EditFieldPosition - NumCharsToHilite, EditFieldPosition);
					NumCharsToHilite = 0;
					}
				
            FldSetInsPtPosition (fldP, EditFieldPosition);
            
            PriorAddressFormID = FrmGetFormId (frm);
            handled = true;
         }
         break;
        
      default:
         break;
   }


   return (handled);
}

/***********************************************************************
 *
 * FUNCTION:    EditViewNewRecord
 *
 * DESCRIPTION: Makes a new record with some setup
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/13/95   Initial Revision
 *
 ***********************************************************************/
static void EditViewNewRecord ()
{
   AddrDBRecordType newRecord;
   AddressFields i;
   UInt16 attr;
   Err err;
   

   // Set up the new record
   newRecord.options.phones.displayPhoneForList = 0;
   newRecord.options.phones.phone1 = workLabel;
   newRecord.options.phones.phone2 = homeLabel;
   newRecord.options.phones.phone3 = faxLabel;
   newRecord.options.phones.phone4 = otherLabel;
   newRecord.options.phones.phone5 = emailLabel;
   
   for (i = firstAddressField; i < addressFieldsCount; i++)
      {
      newRecord.fields[i] = NULL;
      }
   
   err = AddrNewRecord(AddrDB, &newRecord, &CurrentRecord);
   if (err)
      {
      FrmAlert(DeviceFullAlert);
      return;
      }
      

   // Set it's category to the category being viewed.
   // If the category is All then set the category to unfiled.
   DmRecordInfo (AddrDB, CurrentRecord, &attr, NULL, NULL);   
   attr &= ~dmRecAttrCategoryMask;
   attr |= ((CurrentCategory == dmAllCategories) ? dmUnfiledCategory : 
      CurrentCategory) | dmRecAttrDirty;
   DmSetRecordInfo (AddrDB, CurrentRecord, &attr, NULL);


   // Set the global variable that determines which field is the top visible
   // field in the edit view.  Also done when New is pressed.
   TopVisibleFieldIndex = 0;
	CurrentFieldIndex = editFirstFieldIndex;
   EditRowIDWhichHadFocus = editFirstFieldIndex;
   EditFieldPosition = 0;

   FrmGotoForm (EditView);
}


#pragma mark ----------------
/***********************************************************************
 *
 * FUNCTION:    RecordViewNewLine
 *
 * DESCRIPTION: Adds the next field at the start of a new line
 *
 * PARAMETERS:  width - width already occupied on the line
 *
 * RETURNED:    width is set
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/21/95   Initial Revision
 *
 ***********************************************************************/
static void RecordViewNewLine (UInt16 *width)
{
   if (RecordViewLastLine >= recordViewLinesMax)
      return;
      
   if (*width == 0)
      {
      RecordViewLines[RecordViewLastLine].fieldNum = recordViewBlankLine;
      RecordViewLines[RecordViewLastLine].x = 0;
      RecordViewLastLine++;
      }
   else
      *width = 0;
}



/***********************************************************************
 *
 * FUNCTION:    RecordViewAddSpaceForText
 *
 * DESCRIPTION: Adds space for text to the RecordViewLines info. 
 *
 * PARAMETERS:  string - Char * to text to leave space for
 *                width - width already occupied on the line
 *
 * RETURNED:    width is increased by the width of the text
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/21/95   Initial Revision
 *
 ***********************************************************************/
static void RecordViewAddSpaceForText (const Char * const string, UInt16 *width)
{
   *width += FntCharsWidth(string, StrLen(string));
}


/***********************************************************************
 *
 * FUNCTION:    RecordViewPositionTextAt
 *
 * DESCRIPTION: Position the following text at the given position. 
 *
 * PARAMETERS:  position - position to indent to
 *                width - width already occupied on the line
 *
 * RETURNED:    width is increased if the position is greater
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   8/2/95   Initial Revision
 *
 ***********************************************************************/
static void RecordViewPositionTextAt (UInt16 *width, const UInt16 position)
{
   if (*width < position)
      *width = position;
}


/***********************************************************************
 *
 * FUNCTION:    RecordViewAddField
 *
 * DESCRIPTION: Adds a field to the RecordViewLines info. 
 *
 * PARAMETERS:  fieldNum - field to add
 *                width - width already occupied on the line
 *                maxWidth - can't add words past this width
 *                indentation - the amounnt of indentation wrapped lines of
 *                              text should begin with (except the last)
 *
 * RETURNED:    width is set to the width of the last line added
 *
 * HISTORY:
 *		06/21/95	rsf	Created by Roger Flores
 *		10/25/99	kwk	Fix sign extension w/calling TxtCharIsSpace
 *
 ***********************************************************************/
static void RecordViewAddField (const UInt16 fieldNum, UInt16 *width, 
   const UInt16 maxWidth, const UInt16 indentation)
{
   UInt16 length;
   UInt16 offset = 0;
   UInt16 newOffset;
   

   if (recordViewRecord.fields[fieldNum] == NULL ||
      RecordViewLastLine >= recordViewLinesMax)
      return;

   // If we're past the maxWidth already then start at the beginning
   // of the next line.
   if (*width >= maxWidth)
      *width = indentation;
      
   do {
		if (RecordViewLastLine >= recordViewLinesMax)
			break;
      
		// Check if we word wrapped in the middle of a word which could 
		// fit on the next line.  UInt16 wrap doesn't work well for use
		// when we call it twice on the same line.
		// The first part checks to see if we stopped in the middle of a line
		// The second part check to see if we didn't stop after a word break
		// The third part checks if this line wasn't a wide as it could be
		// because some other text used up space.
		// DOLATER kwk - Japanese version is completely different...decide
		// if it can be used for English & other languages. This code below
		// isn't exactly correct because it's checking the last byte of
		// the string in the call to TxtCharIsSpace, which might be the
		// low byte of a double-byte character.
		length = FldWordWrap(&recordViewRecord.fields[fieldNum][offset], 
							maxWidth - *width);
		if (recordViewRecord.fields[fieldNum][offset + length] != '\0'
			&& !TxtCharIsSpace((UInt8)recordViewRecord.fields[fieldNum][offset + length - 1])
			&& (*width > indentation))
			{
			length = 0;            // don't word wrap - try next line
			}

		// Lines returned from FldWordWrap may include a '\n' at the
		// end.  If present remove it to keep it from being drawn.
		// The alternative is to not draw linefeeds at draw time.  That
		// seem more complex (there's many WinDrawChars) and slower as well.
		// This way is faster but makes catching word wrapping problems
		// less obvious (length 0 also happens when word wrap fails).
		newOffset = offset + length;
		if (newOffset > 0 && recordViewRecord.fields[fieldNum][newOffset - 1] == linefeedChr)
			length--;
        
		RecordViewLines[RecordViewLastLine].fieldNum = fieldNum;
		RecordViewLines[RecordViewLastLine].offset = offset;
		RecordViewLines[RecordViewLastLine].x = *width;
		RecordViewLines[RecordViewLastLine].length = length;
		RecordViewLastLine++;
		offset = newOffset;
            
		// Wrap to the start of the next line if there's still more text
		// to draw (so we must have run out of room) or wrap if we
		// encountered a line feed character.
		if (recordViewRecord.fields[fieldNum][offset] != '\0')
			*width = indentation;
		else
			break;

		} while (true);


   // If the last character was a new line then there is no width.
   // Otherwise the width is the width of the characters on the last line.
   if (recordViewRecord.fields[fieldNum][offset - 1] == linefeedChr)
      *width = 0;
   else
      *width += FntCharsWidth(&recordViewRecord.fields[fieldNum][
         RecordViewLines[RecordViewLastLine - 1].offset], 
         RecordViewLines[RecordViewLastLine - 1].length);
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordViewInit
 *
 * DESCRIPTION: This routine initializes the "Record View" of the 
 *              Address application.  Most importantly it lays out the
 *                record and decides how the record is drawn.
 *
 * PARAMETERS:  frm - pointer to the view form.
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/21/95   Initial Revision
 *
 ***********************************************************************/
static void RecordViewInit (FormPtr frm)
{
   UInt16 attr;
   UInt16 index;
   UInt16 category;
   AddrAppInfoPtr appInfoPtr;
   MemHandle RecordViewLinesH;
   UInt16 width = 0;
   RectangleType r;
   UInt16 maxWidth;
   FontID curFont;
   UInt16 i;
   UInt16 fieldIndex;
   UInt16 phoneLabelNum;
   const AddressFields* fieldMap;
   Int16 cityIndex = -1;
   Int16 zipIndex = -1;


   // Set the category label.
   if (CurrentCategory == dmAllCategories)
      {
      DmRecordInfo (AddrDB, CurrentRecord, &attr, NULL, NULL);   
      category = attr & dmRecAttrCategoryMask;
      }
   else 
      category = CurrentCategory;

   CategoryGetName (AddrDB, category, CategoryName);
   index = FrmGetObjectIndex (frm, RecordCategoryLabel);
   FrmSetCategoryLabel (frm, index, CategoryName);



   // Allocate the record view lines
   RecordViewLinesH = MemHandleNew(sizeof(RecordViewLineType) * recordViewLinesMax);
   ErrFatalDisplayIf (!RecordViewLinesH, "Out of memory");
   
   RecordViewLines = MemHandleLock(RecordViewLinesH);
   RecordViewLastLine = 0;
   TopRecordViewLine = 0;
   
   FrmGetFormBounds(frm, &r);
   maxWidth = r.extent.x;
   
   appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);

   if (RecordLabelColumnWidth == 0)
		RecordLabelColumnWidth = GetLabelColumnWidth (appInfoPtr, AddrRecordFont);


   // Get the record to display.  recordViewRecordH may have data if
   // we are redisplaying the record (custom fields changed).
   if (recordViewRecordH)
      MemHandleUnlock(recordViewRecordH);
   AddrGetRecord (AddrDB, CurrentRecord, &recordViewRecord, &recordViewRecordH);
   
   // Here we construct the recordViewLines info by laying out
   // the record 
   curFont = FntSetFont (largeBoldFont);
   if (recordViewRecord.fields[name] == NULL &&
      recordViewRecord.fields[firstName] == NULL)
      {
      RecordViewAddField(company, &width, maxWidth, 0);
      RecordViewNewLine(&width);
      }
   else
      {
      if (recordViewRecord.fields[firstName] != NULL)
         {
         RecordViewAddField(firstName, &width, maxWidth, 0);
         
         // Separate the last name from the first name as long
         // as they are together on the same line.
         if (width > 0)
            RecordViewAddSpaceForText (" ", &width);
         }
      RecordViewAddField(name, &width, maxWidth, 0);
      RecordViewNewLine(&width);
      }
   RecordViewFirstPlainLine = RecordViewLastLine;
	FntSetFont (AddrRecordFont);

   if (recordViewRecord.fields[title])
      {
      RecordViewAddField(title, &width, maxWidth, 0);
      RecordViewNewLine(&width);
      }
   if (recordViewRecord.fields[company] != NULL && 
      (recordViewRecord.fields[name] != NULL ||
      recordViewRecord.fields[firstName] != NULL))
      {
      RecordViewAddField(company, &width, maxWidth, 0);
      RecordViewNewLine(&width);
      }      
      
   // If the line above isn't blank then add a blank line
   if (RecordViewLastLine > 0 &&
      RecordViewLines[RecordViewLastLine - 1].fieldNum != recordViewBlankLine)
      {
      RecordViewNewLine(&width);
      }



   // Layout the phone numbers.  Start each number on its own line.
   // Put the label first, followed by ": " and then the number
   for (fieldIndex = firstPhoneField; fieldIndex <= lastPhoneField; fieldIndex++)
      {
      if (recordViewRecord.fields[fieldIndex])
         {
         phoneLabelNum = GetPhoneLabel(&recordViewRecord, fieldIndex);
         RecordViewAddSpaceForText (appInfoPtr->fieldLabels[
            phoneLabelNum + ((phoneLabelNum < numPhoneLabelsStoredFirst) ? 
            firstPhoneField : (addressFieldsCount - numPhoneLabelsStoredFirst))], 
            &width);
         RecordViewAddSpaceForText (": ", &width);
         RecordViewPositionTextAt(&width, RecordLabelColumnWidth);
         RecordViewAddField(fieldIndex, &width, maxWidth, RecordLabelColumnWidth);
         RecordViewNewLine(&width);
         }
      }

      
   // If the line above isn't blank then add a blank line
   if (RecordViewLastLine > 0 &&
      RecordViewLines[RecordViewLastLine - 1].fieldNum != recordViewBlankLine)
      {
      RecordViewNewLine(&width);
      }
      


   // Now do the address information
   if (recordViewRecord.fields[address])
      {
      RecordViewAddField(address, &width, maxWidth, 0);
      RecordViewNewLine(&width);
      }

	// We need to format the city, state, and zip code differently depending
	// on which country it is. For now, assume that if the city comes first,
	// we use the standard US formatting of [city, ][state   ][zip]<cr>,
	// otherwise we'll use the "int'l" format of [zip ][city]<cr>[state]<cr>.
	// DOLATER kwk - A better way of handling this would be to use a formatting
	// resource, which had N records, one for each line, where each record
	// had M entries, one for each field, and each entry had a field id and
	// suffix text.
	i = 0;
	fieldMap = (const AddressFields*)MemHandleLock(DmGetResource(fieldMapRscType, FieldMapID));
   while ((cityIndex == -1) || (zipIndex == -1))
   	{
   	if (fieldMap[i] == city)
   		{
   		cityIndex = i;
   		}
   	else if (fieldMap[i] == zipCode)
   		{
   		zipIndex = i;
   		}
   	
   	i++;
   	}
   
   MemPtrUnlock((MemPtr)fieldMap);
   
   // Decide if we're formatting it US-style, or int'l-style
   if (cityIndex < zipIndex)
   	{
	   if (recordViewRecord.fields[city])
	      {
	      RecordViewAddField(city, &width, maxWidth, 0);
	      }
	   if (recordViewRecord.fields[state])
	      {
	      if (width > 0)
	         RecordViewAddSpaceForText (", ", &width);
	      RecordViewAddField(state, &width, maxWidth, 0);
	      }
	   if (recordViewRecord.fields[zipCode])
	      {
	      if (width > 0)
	         RecordViewAddSpaceForText ("   ", &width);
	      RecordViewAddField(zipCode, &width, maxWidth, 0);
	      }
	   if (recordViewRecord.fields[city] ||
	      recordViewRecord.fields[state] ||
	      recordViewRecord.fields[zipCode])
	      {
	      RecordViewNewLine(&width);
	      }
   	}
   else
   	{
	   if (recordViewRecord.fields[zipCode])
	      {
	      RecordViewAddField(zipCode, &width, maxWidth, 0);
	      }
	   if (recordViewRecord.fields[city])
	      {
	      if (width > 0)
	         RecordViewAddSpaceForText (" ", &width);
	      RecordViewAddField(city, &width, maxWidth, 0);
	      }
	   if (recordViewRecord.fields[zipCode] ||
	      recordViewRecord.fields[city])
	      {
	      RecordViewNewLine(&width);
	      }
	   if (recordViewRecord.fields[state])
	      {
	      RecordViewAddField(state, &width, maxWidth, 0);
	      RecordViewNewLine(&width);
	      }
   	}

   if (recordViewRecord.fields[country])
      {
      RecordViewAddField(country, &width, maxWidth, 0);
      RecordViewNewLine(&width);
      }


   // If the line above isn't blank then add a blank line
   if (RecordViewLastLine > 0 &&
      RecordViewLines[RecordViewLastLine - 1].fieldNum != recordViewBlankLine)
      {
      RecordViewNewLine(&width);
      }


   // Do the custom fields
   for (i = custom1; i < addressFieldsCount - 1; i++)
      {
      if (recordViewRecord.fields[i])
         {
         RecordViewAddSpaceForText (appInfoPtr->fieldLabels[i], &width);
         RecordViewAddSpaceForText (": ", &width);
         RecordViewPositionTextAt(&width, RecordLabelColumnWidth);
         RecordViewAddField(i, &width, maxWidth, RecordLabelColumnWidth);
         RecordViewNewLine(&width);
         RecordViewNewLine(&width);      // leave a blank line
         }
      }
            
   // Show the note field.
   if (recordViewRecord.fields[note])
      {
      RecordViewAddField(note, &width, maxWidth, 0);
      }
   

   // Now remove trailing blank lines
   while (RecordViewLastLine > 0 &&
      RecordViewLines[RecordViewLastLine - 1].fieldNum == recordViewBlankLine)
      {
      RecordViewLastLine--;
      }


   MemPtrUnlock(appInfoPtr);
   FntSetFont (curFont);
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordViewErase
 *
 * DESCRIPTION: Erases the record view
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/30/95   Initial Revision
 *
 ***********************************************************************/
static void RecordViewErase ()
{
   FormPtr frm;
   RectangleType r;


   frm = FrmGetActiveForm();
   FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, RecordViewDisplay), &r);
   WinEraseRectangle (&r, 0);
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordViewCalcNextLine
 *
 * DESCRIPTION: Calculate how far to advance to the next line.
 * A Blank line or text which begins to the left of text on the
 * current line advance the line down.  Multiple blank lines in
 * succession advance the line down half a line at a time.
 *
 * PARAMETERS:  i - the line to base how far to advance
 *                oneLine - the amount which advance one line down.
 *
 * RETURNED:    the amount to advance.  Typically oneLine or oneLine / 2.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   9/28/95   Initial Revision
 *
 ***********************************************************************/
static UInt16   RecordViewCalcNextLine(UInt16 i, UInt16 oneLine)
{
   // Advance down if the text starts before the text of the current line.
   if (RecordViewLines[i].x == 0 || 
      (i > 0 && 
         (RecordViewLines[i].x <= RecordViewLines[i - 1].x ||
         RecordViewLines[i - 1].fieldNum == recordViewBlankLine)))
      {
      // A non blank line moves down a full line.
      if (RecordViewLines[i].fieldNum != recordViewBlankLine)
         {
         return oneLine;
         }   
      else
         {
         // A recordViewBlankLine followed by another recordViewBlankLine
         // only moves down half a line to conserve vertical screen space.
/*         if (i < RecordViewLastLine && RecordViewLines[i + 1].x > 0)
            return oneLine;
         else
*/
         // A recordViewBlankLine is half-height.
         return oneLine / 2;
         }
      }
   return 0;      // Stay on the same line.
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordViewDrawSelectedText
 *
 * DESCRIPTION: Inverts text which is considered selected.
 *
 * PARAMETERS:  currentField - field containing the selected text
 *                selectPos - offset into field for start of selected text
 *                selectLen - length of selected text.  This field
 *                  should be zero if selected text isn't desired.
 *                textY - where on the screen the text was drawn
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   11/27/95   Cut from RecordViewDraw
 *
 ***********************************************************************/
static void RecordViewDrawSelectedText (UInt16 currentField, 
   UInt16 selectPos, UInt16 selectLen, UInt16 textY)
{
   UInt16 selectXLeft = 0;
   UInt16 selectXRight = 0;
   RectangleType invertRect;


   // If the start of the selected region is on this line calc an x
   if (RecordViewLines[currentField].offset <= selectPos && 
      selectPos < RecordViewLines[currentField].offset + 
         RecordViewLines[currentField].length)
      {
      selectXLeft = FntCharsWidth(&recordViewRecord.fields[
         RecordViewLines[currentField].fieldNum][
            RecordViewLines[currentField].offset], 
         selectPos - RecordViewLines[currentField].offset);
      }
   // If the end of the selected region is on this line calc an x
   if (RecordViewLines[currentField].offset <= selectPos + selectLen && 
      selectPos + selectLen <= RecordViewLines[currentField].offset + 
      RecordViewLines[currentField].length)
      {
      selectXRight = FntCharsWidth(&recordViewRecord.fields[
         RecordViewLines[currentField].fieldNum][
         RecordViewLines[currentField].offset], 
         selectPos + selectLen - RecordViewLines[currentField].offset);
      }
   
   // If either the left or right have been set then some
   // text needs to be selected.
   if (selectXLeft | selectXRight)
      {
      if (!selectXRight)
         selectXRight = RecordViewLines[currentField].x + 
            FntCharsWidth(&recordViewRecord.fields[
            RecordViewLines[currentField].fieldNum][
            RecordViewLines[currentField].offset], 
            RecordViewLines[currentField].length);
      // Now add in the left x of the text
      selectXLeft += RecordViewLines[currentField].x;
      selectXRight += RecordViewLines[currentField].x;
      
      // When hightlighting the text start the highlight one pixel to the left of the
      // text so the left edge of the inverted area isn't ragged.  If the text is 
      // at the far left we obviously can't go left more.
      if (selectXLeft > 0)
         selectXLeft--;
      
      // Invert the text
      invertRect.topLeft.x = selectXLeft;
      invertRect.extent.x = selectXRight - selectXLeft;
      invertRect.topLeft.y = textY;
      invertRect.extent.y = FntLineHeight();
      WinInvertRectangle (&invertRect, 0);
      
      // Reset incase text needs inversion on the next line
      selectXLeft = 0;
      selectXRight = 0;
      }
}


/***********************************************************************
 *
 * FUNCTION:    RecordViewDraw
 *
 * DESCRIPTION: This routine initializes the "Record View"
 *
 * PARAMETERS:  selectFieldNum - field to show selected text
 *                selectPos - offset into field for start of selected text
 *                selectLen - length of selected text.  This field
 *                  should be zero if selected text isn't desired.
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * HISTORY:
 *		06/21/95	rsf	Created by Roger Flores
 *		02/06/98	tlw	Change test below "If we are past bottom stop drawing" from
 *							 if >= bottomOfRecordViewDisplay - FntLineHeight() to
 *							 if > bottomOfRecordViewDisplay - FntLineHeight()
 *							 to allow last line to be drawn.
 *		07/29/99	kwk	When drawing zip code, load prefix (# of spaces) from resource.
 *
 ***********************************************************************/
static void RecordViewDraw (UInt16 selectFieldNum, UInt16 selectPos,
   UInt16 selectLen)
{
   AddrAppInfoPtr appInfoPtr;
   UInt16 y;
   UInt16 i;
   FontID curFont;
   FormPtr   frm;
   UInt16 phoneLabelNum;
   Char * fieldLabelString;
   UInt16 fieldLabelLength;
   UInt16 upIndex;
   UInt16 downIndex;
   Boolean scrollableUp;
   Boolean scrollableDown;
   RectangleType r;
   int bottomOfRecordViewDisplay;


   appInfoPtr = (AddrAppInfoPtr) AddrAppInfoGetPtr(AddrDB);


   frm = FrmGetActiveForm();
   FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, RecordViewDisplay), &r);
   bottomOfRecordViewDisplay = r.topLeft.y +  r.extent.y;
   
   if (TopRecordViewLine < RecordViewFirstPlainLine)
      curFont = FntSetFont (largeBoldFont);
   else
		curFont = FntSetFont (AddrRecordFont);

   y = r.topLeft.y - RecordViewCalcNextLine(TopRecordViewLine, FntLineHeight());
   
   for (i = TopRecordViewLine; i < RecordViewLastLine; i++)
      {
      
      // This must be done before the font shrinks or else
      // we move down less to the next row and overwrite the 
      // descenders of the last row which used a large font.
      y += RecordViewCalcNextLine(i, FntLineHeight());
      
      if (i == RecordViewFirstPlainLine)
			FntSetFont (AddrRecordFont);

      // If we are past the bottom stop drawing
      if (y > bottomOfRecordViewDisplay - FntLineHeight())
         break;
      
      ErrNonFatalDisplayIf(y < r.topLeft.y, "Drawing record out of gadget");
         


      if (RecordViewLines[i].offset == 0)
         {
         switch (RecordViewLines[i].fieldNum)
            {
            case recordViewBlankLine:
               break;
   
   
            case phone1:
            case phone2:
            case phone3:
            case phone4:
            case phone5:
               if (RecordViewLines[i].offset == 0)
                  {
                  phoneLabelNum = GetPhoneLabel(&recordViewRecord, RecordViewLines[i].fieldNum);
                  fieldLabelString = appInfoPtr->fieldLabels[phoneLabelNum + 
                     ((phoneLabelNum < numPhoneLabelsStoredFirst) ? 
                     firstPhoneField : (addressFieldsCount - numPhoneLabelsStoredFirst))];
                  fieldLabelLength = StrLen(fieldLabelString);
                  WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
                  WinDrawChars(": ", 2, FntCharsWidth(fieldLabelString, fieldLabelLength), y);
                  }
               WinDrawChars(&recordViewRecord.fields[
                  RecordViewLines[i].fieldNum][RecordViewLines[i].offset],
                  RecordViewLines[i].length, RecordViewLines[i].x, y);
               break;
               
               
            case custom1:
            case custom2:
            case custom3:
            case custom4:
               fieldLabelString = appInfoPtr->fieldLabels[RecordViewLines[i].fieldNum];
               fieldLabelLength = StrLen(fieldLabelString);
               if (RecordViewLines[i].length == 0
               		|| FntCharsWidth(fieldLabelString, fieldLabelLength) < RecordViewLines[i].x)
           	   	  {
	              WinDrawChars(fieldLabelString, fieldLabelLength, 0, y);
	              WinDrawChars(": ", 2, FntCharsWidth(fieldLabelString, fieldLabelLength), y);
	              }
               WinDrawChars(&recordViewRecord.fields[
                  RecordViewLines[i].fieldNum][RecordViewLines[i].offset],
                  RecordViewLines[i].length, RecordViewLines[i].x, y);
               break;
               

            case state:
               if (RecordViewLines[i].x > 0)
                  WinDrawChars(", ", 2, RecordViewLines[i].x - FntCharsWidth(", ", 2), y);
               WinDrawChars(&recordViewRecord.fields[
                  RecordViewLines[i].fieldNum][RecordViewLines[i].offset],
                  RecordViewLines[i].length, RecordViewLines[i].x, y);
               break;
               
            case zipCode:
               if (RecordViewLines[i].x > 0)
               	{
               	const Char* textP;
            		textP = (const Char*)MemHandleLock(DmGetResource(strRsc, ZipCodePrefixStr));
                  WinDrawChars(textP, StrLen(textP), RecordViewLines[i].x - FntCharsWidth(textP, StrLen(textP)), y);
						MemPtrUnlock((MemPtr)textP);
						}

               WinDrawChars(&recordViewRecord.fields[
                  RecordViewLines[i].fieldNum][RecordViewLines[i].offset],
                  RecordViewLines[i].length, RecordViewLines[i].x, y);
               break;
               
            default:
               WinDrawChars(&recordViewRecord.fields[
                  RecordViewLines[i].fieldNum][RecordViewLines[i].offset],
                  RecordViewLines[i].length, RecordViewLines[i].x, y);
               break;
            }
         }
      else
         {
         // Draw the remainder of the fields' lines without any 
         // other special handling.
         if (RecordViewLines[i].fieldNum != recordViewBlankLine)
            {
            WinDrawChars(&recordViewRecord.fields[
               RecordViewLines[i].fieldNum][RecordViewLines[i].offset],
               RecordViewLines[i].length, RecordViewLines[i].x, y);
            }
         }

      
      // Highlight text if it is within the selection bounds.  This is
      // used to select found text.
      if (RecordViewLines[i].fieldNum == selectFieldNum &&
         selectLen > 0)
         {
         RecordViewDrawSelectedText (i, selectPos, selectLen, y);
         }
      }
               
            
   MemPtrUnlock(appInfoPtr);
   FntSetFont (curFont);
   
   
   // Now show/hide the scroll arrows
   frm = FrmGetActiveForm ();

   scrollableUp = TopRecordViewLine != 0;
   scrollableDown = i < RecordViewLastLine; 


   // Update the scroll button.
   upIndex = FrmGetObjectIndex (frm, RecordUpButton);
   downIndex = FrmGetObjectIndex (frm, RecordDownButton);
   FrmUpdateScrollers (frm, upIndex, downIndex, scrollableUp, scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    RecordViewDrawBusinessCardIndicator
 *
 * DESCRIPTION: Draw the business card indicator if the current record is
 * the business card.
 *
 * PARAMETERS:  formP - the form containing the business card indicator
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  10/22/97  Initial Revision
 *
 ***********************************************************************/
static void RecordViewDrawBusinessCardIndicator (FormPtr formP)
{
	UInt32 uniqueID;
	
	DmRecordInfo (AddrDB, CurrentRecord, NULL, &uniqueID, NULL);
	if (BusinessCardRecordID == uniqueID)
		FrmShowObject(formP, FrmGetObjectIndex (formP, RecordViewBusinessCardBmp));
	else
		FrmHideObject(formP, FrmGetObjectIndex (formP, RecordViewBusinessCardBmp));
	
}   


/***********************************************************************
 *
 * FUNCTION:    RecordViewUpdate
 *
 * DESCRIPTION: Update the record view and redraw it.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   10/18/95   Initial Revision
 *
 ***********************************************************************/
static void RecordViewUpdate ()
{
   FormPtr frm;
   
   
   MemHandleFree(MemPtrRecoverHandle(RecordViewLines));
   RecordViewLines = 0;
   frm = FrmGetActiveForm ();
   RecordViewInit (frm);
   RecordViewErase ();
   RecordViewDraw(0, 0, 0);
   RecordViewDrawBusinessCardIndicator(frm);
}


/***********************************************************************
 *
 * FUNCTION:    RecordViewScrollOnePage
 *
 * DESCRIPTION: Scrolls the record view by one page less one line unless
 * we scroll from RecordViewLastLine (used by scroll code).
 *
 * PARAMETERS:  newTopRecordViewLine - top line of the display
 *              direction - up or dowm
 *
 * RETURNED:    new newTopRecordViewLine one page away
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *			roger		6/22/95	Initial Revision
 *			roger		8/2/95	Reworked to handle half height blank lines.
 *			roger		10/30/95	Reworked to obey FntLineHeight
 *			roger		10/31/95	Broke out of RecordViewScroll
 *
 ***********************************************************************/
static UInt16 RecordViewScrollOnePage (Int16 newTopRecordViewLine, 
   WinDirectionType direction)
{
   Int16 offset;
   FontID curFont;
   FormPtr frm;
   Int16 largeFontLineHeight;
   Int16 stdFontLineHeight;
   Int16 currentLineHeight;
   RectangleType r;
   Int16 recordViewDisplayHeight;


   // setup stuff
   curFont = FntSetFont (largeBoldFont);
   largeFontLineHeight = FntLineHeight();
	FntSetFont (AddrRecordFont);
   stdFontLineHeight = FntLineHeight();
   FntSetFont (curFont);
   
   frm = FrmGetActiveForm();
   FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, RecordViewDisplay), &r);
   recordViewDisplayHeight = r.extent.y;
   if (newTopRecordViewLine != RecordViewLastLine)
      recordViewDisplayHeight -= stdFontLineHeight;   // less one one line
   

   if (direction == winUp)
      offset = -1;
   else
      offset = 1;
   

   while (recordViewDisplayHeight >= 0 && 
      (newTopRecordViewLine > 0 || direction == winDown) && 
      (newTopRecordViewLine < (RecordViewLastLine - 1) || direction == winUp)) 
      {
      newTopRecordViewLine += offset;
      if (RecordViewLines[newTopRecordViewLine].fieldNum <= firstName)
         currentLineHeight = largeFontLineHeight;
      else
         currentLineHeight = stdFontLineHeight;

      recordViewDisplayHeight -= RecordViewCalcNextLine(newTopRecordViewLine, 
         currentLineHeight);
      };
      
   // Did we go too far?
   if (recordViewDisplayHeight < 0)
      {
      // The last line was too much so remove it
      newTopRecordViewLine -= offset;
      
      // Also remove any lines which don't have a height
      while (RecordViewCalcNextLine(newTopRecordViewLine, 2) == 0)
         {
         newTopRecordViewLine -= offset;   // skip it
         }
      }
   
   return newTopRecordViewLine;
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordViewScroll
 *
 * DESCRIPTION: Scrolls the record view
 *
 * PARAMETERS:  direction - up or dowm
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger	6/22/95	Initial Revision
 *         roger	8/2/95	Reworked to handle half height blank lines.
 *         roger	10/30/95	Reworked to obey FntLineHeight
 *				gap	10/12/99	Close command bar before processing scroll
 *
 ***********************************************************************/
static void RecordViewScroll (WinDirectionType direction)
{
   Int16 lastRecordViewLine;
   UInt16 newTopRecordViewLine;
   UInt16 category;
   UInt16 recordNum;
   Int16 seekDirection;
   UInt16	attr;


	// Before processing the scroll, be sure that the command bar has been closed.
	MenuEraseStatus (0);

   newTopRecordViewLine = TopRecordViewLine;
	if (direction == winUp)
		{
		newTopRecordViewLine = RecordViewScrollOnePage (newTopRecordViewLine, direction);
		}
   else
		{
		// Simple two part algorithm.
		// 1) Scroll down one page
		// 2) Scroll up one page from the bottom
		// Use the higher of the two positions
		// Find the line one page down

		newTopRecordViewLine = RecordViewScrollOnePage (newTopRecordViewLine, direction);

		// Find the line at the top of the last page 
		// (code copied to RecordViewMakeVisible).
		lastRecordViewLine = RecordViewScrollOnePage (RecordViewLastLine, winUp);


		// We shouldn't be past the top line of the last page
		if (newTopRecordViewLine > lastRecordViewLine)
			newTopRecordViewLine = lastRecordViewLine;

		}


   if (newTopRecordViewLine != TopRecordViewLine)
      {
      TopRecordViewLine = newTopRecordViewLine;
      
      RecordViewErase ();
      RecordViewDraw(0, 0, 0);
      }

   // If we couldn't scroll then scroll to the next record.
   else
      {
      // Move to the next or previous memo.
      if (direction == winUp)
         {
         seekDirection = dmSeekBackward;
         }
      else
         {
         seekDirection = dmSeekForward;
         }
   
      if (ShowAllCategories)
         category = dmAllCategories;
      else
         category = CurrentCategory;
   
      recordNum = CurrentRecord;
      
 		//skip masked records.
 		while (!DmSeekRecordInCategory (AddrDB, &recordNum, 1, seekDirection, category) &&
 				!DmRecordInfo (AddrDB, recordNum, &attr, NULL, NULL) &&
 		  		((attr & dmRecAttrSecret) && PrivateRecordVisualStatus == maskPrivateRecords))
 		  	{
 		  	}
      if (recordNum == CurrentRecord) return;
   
      SndPlaySystemSound (sndInfo);
   
      CurrentRecord = recordNum;
      RecordViewUpdate ();
      }
   
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordViewMakeVisible
 *
 * DESCRIPTION: Make a selection range visible 
 *
 * PARAMETERS:  selectFieldNum - field to show selected text
 *                selectPos - offset into field for start of selected text
 *                selectLen - length of selected text
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   8/3/95   Initial Revision
 *
 ***********************************************************************/
static void RecordViewMakeVisible (UInt16 selectFieldNum, UInt16 selectPos,
   UInt16 selectLen)
{
   UInt16 newTopRecordViewLine;
   UInt16 i;


   newTopRecordViewLine = RecordViewLastLine;
   for (i = 0; i < RecordViewLastLine; i++)
      {
      // Does the selected range end here?
      if (RecordViewLines[i].fieldNum == selectFieldNum &&
         RecordViewLines[i].offset <= selectPos + selectLen && 
         selectPos + selectLen <= RecordViewLines[i].offset + 
            RecordViewLines[i].length)
         {
         newTopRecordViewLine = i;
         }
      }


   // If the selected range doesn't seem to exist then
   // we shouldn't scroll the view.
   if (newTopRecordViewLine == RecordViewLastLine)
      return;


   // Display as much before the selected text as possible
   newTopRecordViewLine = RecordViewScrollOnePage (newTopRecordViewLine, winUp);

   if (newTopRecordViewLine != TopRecordViewLine)
      TopRecordViewLine = newTopRecordViewLine;
}


/***********************************************************************
 *
 * FUNCTION:    RecordViewHandlePen
 *
 * DESCRIPTION: Handle pen movement in the RecordViewDisplay. 
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if handled.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   11/27/95   Cut from RecordViewHandleEvent
 *
 ***********************************************************************/
static Boolean RecordViewHandlePen (EventType * event)
{
   Boolean      handled = false;
   FormPtr      frm;
   RectangleType r;
   Int16        x, y;
   Boolean      penDown;
   
   
   // If the user taps in the RecordViewDisplay take them to the Edit View
   frm = FrmGetActiveForm();
   FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, RecordViewDisplay), &r);
   if (RctPtInRectangle (event->screenX, event->screenY, &r))
      {
      do 
         {
         PenGetPoint (&x, &y, &penDown);
         } while (penDown);
      
      if (RctPtInRectangle (x, y, &r))
         FrmGotoForm (EditView);
         
      handled = true;
      }
      
   return handled;
}


/***********************************************************************
 *
 * FUNCTION:    RecordViewDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  6/27/95   Initial Revision
 *         jmp    9/17/99   Use NewNoteView instead of NoteView.
 *
 ***********************************************************************/
static Boolean RecordViewDoCommand (UInt16 command)
{
	UInt16 newRecord;
	UInt16 numCharsToHilite;
  
   switch (command)
   	{
      case RecordRecordDeleteRecordCmd:
         if (DetailsDeleteRecord ())
         	{
            recordViewRecordH = 0;      // freed by the last routine
            FrmGotoForm (ListView);
         	}
         return true;
         
	  case RecordRecordDuplicateAddressCmd:
	  	  newRecord = DuplicateCurrentRecord (&numCharsToHilite, false);
	  	  // If we have a new record take the user to be able to edit it
	  	  // automatically.
	  	  if (newRecord != noRecord)
	  	  {
	  	    NumCharsToHilite = numCharsToHilite;
	  	    CurrentRecord = newRecord;
	  	    FrmGotoForm (EditView);
	  	  }
	  	  return true;

      case RecordRecordAttachNoteCmd:
         if (CreateNote())
            FrmGotoForm (NewNoteView);
         // CreateNote may or may not have freed the record.  Compare
         // the record's handle to recordViewRecordH.  If they differ
         // the record is new and recordViewRecordH shouldn't be freed
         // by the frmClose.
         if (recordViewRecordH != DmQueryRecord(AddrDB, CurrentRecord))
            recordViewRecordH = 0;
         return true;
         
      case RecordRecordDeleteNoteCmd:
         if (recordViewRecord.fields[note] != NULL &&
            FrmAlert(DeleteNoteAlert) == DeleteNoteYes)
            {
            DeleteNote ();
            // Deleting the note caused the record to be unlocked
            // Get it again for the record view's usage
            AddrGetRecord (AddrDB, CurrentRecord, &recordViewRecord, 
               &recordViewRecordH);
            
            RecordViewUpdate ();
         	}
         return true;
         
      case RecordRecordSelectBusinessCardCmd:
         MenuEraseStatus (0);
         if (FrmAlert(SelectBusinessCardAlert) == SelectBusinessCardYes)
         	{
				DmRecordInfo (AddrDB, CurrentRecord, NULL, &BusinessCardRecordID, NULL);
	         RecordViewDrawBusinessCardIndicator (FrmGetActiveForm());
         	}
         return true;
      
      case RecordRecordSendBusinessCardCmd:
         MenuEraseStatus (0);
      	AddrSendBusinessCard(AddrDB);
         return true;
      
      case RecordRecordSendRecordCmd:
		MenuEraseStatus (0);
		AddrSendRecord(AddrDB, CurrentRecord);
		return true;
      
      /*
      case RecordRecordSendCategoryCmd:
         MenuEraseStatus (0);
      	AddrSendCategory(AddrDB, CurrentCategory);
         return true;
      */
      
		case RecordOptionsFontCmd:
         MenuEraseStatus (0);
			AddrRecordFont = SelectFont (AddrRecordFont);
			return true;

      case RecordOptionsEditCustomFldsCmd:
         MenuEraseStatus (0);
         FrmPopupForm (CustomEditDialog);
         return true;
                  
      case RecordOptionsAboutCmd:
         MenuEraseStatus (0);
         AbtShowAbout (sysFileCAddress);
         return true;
        
   	}
      
   return false;
}


/***********************************************************************
 *
 * FUNCTION:    RecordViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Address View"
 *              of the Address Book application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *				Name	Date		Description
 *				----	----		-----------
 *				art	6/5/95	Initial Revision
 *				jmp	10/01/99	Fix frmUpdateEvent so that it redraws the form
 *									and updated the RecordView now that we can get
 *									into a situation where the bits might not necessarily
 *									be restored except through and update event itself.
 *
 ***********************************************************************/
static Boolean RecordViewHandleEvent (EventType * event)
{
   FormPtr frm;
   Boolean handled = false;


   switch (event->eType)
		{
      case ctlSelectEvent:
         switch (event->data.ctlSelect.controlID)
         	{
            case RecordDoneButton:
               // When we return to the ListView highlight this record.
               ListViewSelectThisRecord = CurrentRecord;
               FrmGotoForm (ListView);
               handled = true;
               break;

            case RecordEditButton:
               FrmGotoForm (EditView);
               handled = true;
               break;

            case RecordNewButton:
               EditViewNewRecord();
               handled = true;
               break;
            default:
               break;
         	}
         break;
   
      
      case penDownEvent:
         handled = RecordViewHandlePen(event);
         break;
      
      
      case ctlRepeatEvent:
         switch (event->data.ctlRepeat.controlID)
         	{
            case RecordUpButton:
               RecordViewScroll(winUp);
               // leave unhandled so the buttons can repeat
               break;

            case RecordDownButton:
               RecordViewScroll(winDown);
               // leave unhandled so the buttons can repeat
               break;
            default:

               break;
         	}
         break;


      case keyDownEvent:
         if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
         	{
            FrmGotoForm (ListView);
            handled = true;
         	}
			
			else if (EvtKeydownIsVirtual(event))
				{
        		switch (event->data.keyDown.chr)
					{
					case vchrPageUp:
						RecordViewScroll (winUp);
						handled = true;
						break;
						
					case vchrPageDown:
						RecordViewScroll (winDown);
						handled = true;
						break;
						
					case vchrSendData:
						AddrSendRecord(AddrDB, CurrentRecord);
						handled = true;
						break;
					
					default:
						break;
					}
				}
			
         break;

      
      case menuEvent:
         return RecordViewDoCommand (event->data.menu.itemID);

      
		case menuCmdBarOpenEvent:
			MenuCmdBarAddButton(menuCmdBarOnLeft, BarDeleteBitmap, menuCmdBarResultMenuItem, RecordRecordDeleteRecordCmd, 0);
			MenuCmdBarAddButton(menuCmdBarOnLeft, BarBeamBitmap, menuCmdBarResultMenuItem, RecordRecordSendRecordCmd, 0);

			// tell the field package to not add cut/copy/paste buttons automatically
			event->data.menuCmdBarOpen.preventFieldButtons = true;

			// don't set handled to true; this event must fall through to the system.
			break;

      
      case frmUpdateEvent:
         frm = FrmGetActiveForm ();
         FrmDrawForm(frm);
         RecordViewUpdate ();
			handled = true;
         break;


      case frmCloseEvent:
         if (recordViewRecordH)
         	{
            MemHandleUnlock(recordViewRecordH);
            recordViewRecordH = 0;
         	}
         MemHandleFree(MemPtrRecoverHandle(RecordViewLines));
         RecordViewLines = 0;
         break;
      
      case frmOpenEvent:
         frm = FrmGetActiveForm ();
         RecordViewInit (frm);
         FrmDrawForm (frm);
         RecordViewDraw(0, 0, 0);
         RecordViewDrawBusinessCardIndicator (frm);
         PriorAddressFormID = FrmGetFormId (frm);

         handled = true;
         break;

      case frmGotoEvent:
         frm = FrmGetActiveForm ();
         CurrentRecord = event->data.frmGoto.recordNum;
         RecordViewInit (frm);
         RecordViewMakeVisible(event->data.frmGoto.matchFieldNum, 
            event->data.frmGoto.matchPos, event->data.frmGoto.matchLen);
         FrmDrawForm (frm);
         RecordViewDraw(event->data.frmGoto.matchFieldNum, 
            event->data.frmGoto.matchPos, event->data.frmGoto.matchLen);
         PriorAddressFormID = FrmGetFormId (frm);
         handled = true;
         break;
		
		default:
			break;
   	}

   return (handled);
}


#pragma mark ----------------
/***********************************************************************
 *
 * FUNCTION:    DetermineRecordName
 *
 * DESCRIPTION: Determines an address book record's name.  The name
 * varies based on which fields exist and what the sort order is.
 *
 * PARAMETERS:  name1, name2 - first and seconds names to draw
 *              name1Length, name2Length - length of the names in chars
 *              name1Width, name2Width - width of the names when drawn
 *              nameExtent - the space the names must be drawn in
 *              *x, y - where the names are drawn
 *              shortenedFieldWidth - the width in the current font
 *              
 *
 * RETURNED:    x is set after the last char drawn
 *					 Boolean - name1/name2 priority based on sortByCompany
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			roger		6/20/95	Initial Revision
 *			frigino	970813	Added priority return value
 *
 ***********************************************************************/
 
extern Boolean DetermineRecordName (AddrDBRecordPtr recordP, 
   Int16 *shortenedFieldWidth, Int16 *fieldSeparatorWidth, Boolean sortByCompany,
   Char **name1, Int16 *name1Length, Int16 *name1Width, 
   Char **name2, Int16 *name2Length, Int16 *name2Width,
   Char **unnamedRecordStringPtr, Int16 nameExtent)
{
   UInt16 fieldNameChoiceList[4];
   UInt16 fieldNameChoice;
   Boolean ignored;
	Boolean name1HasPriority;

   *shortenedFieldWidth = (FntCharWidth('.') * shortenedFieldLength);
   *fieldSeparatorWidth = FntCharsWidth (fieldSeparatorString, 
      fieldSeparatorLength);


   *name1 = NULL;
   *name2 = NULL;
         
   if (sortByCompany)
      {
		// When sorting by company, always treat name2 as priority.
		name1HasPriority = false;

      fieldNameChoiceList[3] = addressFieldsCount;
      fieldNameChoiceList[2] = firstName;
      fieldNameChoiceList[1] = name;
      fieldNameChoiceList[0] = company;
      fieldNameChoice = 0;
      
      while (*name1 == NULL && 
         fieldNameChoiceList[fieldNameChoice] != addressFieldsCount)
         {
         *name1 = recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
         }   
            
		// When sorting by company, treat name2 as priority if we
		// succeed in getting the company name as the name1
		// Did we get the company name?
		if (fieldNameChoice > 1) {
			// No. We got a last name, first name, or nothing. Priority switches to name1
			name1HasPriority = true;
		}

      while (*name2 == NULL && 
         fieldNameChoiceList[fieldNameChoice] != addressFieldsCount)
         {
         *name2 = recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
         }   
      
      }
   else
      {
		// When not sorting by company, always treat name1 as priority.
		name1HasPriority = true;

      fieldNameChoiceList[3] = addressFieldsCount;
      fieldNameChoiceList[2] = addressFieldsCount;
      fieldNameChoiceList[1] = firstName;
      fieldNameChoiceList[0] = name;
      fieldNameChoice = 0;
      
      while (*name1 == NULL && 
         fieldNameChoiceList[fieldNameChoice] != addressFieldsCount)
         {
         *name1 = recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
         }   
      
      if (*name1 == NULL)
         {
         *name1 = recordP->fields[company];
         *name2 = NULL;
         }
      else
         {
         while (*name2 == NULL && 
            fieldNameChoiceList[fieldNameChoice] != addressFieldsCount)
            {
            *name2 = recordP->fields[fieldNameChoiceList[fieldNameChoice++]];
            }
         }         
      }

   if (*name1)
      {
      // Only show text from the first line in the field
      *name1Length = nameExtent;            // longer than possible
      *name1Width = nameExtent;            // wider than possible
      FntCharsInWidth (*name1, name1Width, name1Length, &ignored); //lint !e64
      }
   else
      {
      // Set the name to the unnamed string
      if (*unnamedRecordStringPtr == NULL)
         {
         *unnamedRecordStringPtr = MemHandleLock(DmGetResource(strRsc, UnnamedRecordStr));
         }
      
      // The unnamed string is assumed to be well chosen to not need clipping.
      *name1 = *unnamedRecordStringPtr;
      *name1Length = StrLen(*unnamedRecordStringPtr);
      *name1Width = FntCharsWidth (*unnamedRecordStringPtr, *name1Length);
      }
      
   if (*name2)
      {
      // Only show text from the first line in the field
      *name2Length = nameExtent;            // longer than possible
      *name2Width = nameExtent;            // wider than possible
      FntCharsInWidth (*name2, name2Width, name2Length, &ignored);//lint !e64
      }
   else
      { 
      *name2Length = 0;
      *name2Width = 0;
      }

	// Return priority status
	return name1HasPriority;
}


/***********************************************************************
 *
 * FUNCTION:    DrawRecordName
 *
 * DESCRIPTION: Draws an address book record name.  It is used
 * for the list view and note view.
 *
 * PARAMETERS:  name1, name2 - first and seconds names to draw
 *              name1Length, name2Length - length of the names in chars
 *              name1Width, name2Width - width of the names when drawn
 *              nameExtent - the space the names must be drawn in
 *              *x, y - where the names are drawn
 *              shortenedFieldWidth - the width in the current font
 *              
 *
 * RETURNED:    x is set after the last char drawn
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			roger		6/20/95	Initial Revision
 *			frigino	970813	Rewritten. Now includes a variable ratio for
 *									name1/name2 width allocation, a prioritization
 *									parameter, and a word break search to allow
 *									reclaiming of space from the low priority name.
 *
 ***********************************************************************/

extern void DrawRecordName (
	Char * name1, Int16 name1Length, Int16 name1Width, 
	Char * name2, Int16 name2Length, Int16 name2Width,
	Int16 nameExtent, Int16 *x, Int16 y, Int16 shortenedFieldWidth, 
	Int16 fieldSeparatorWidth, Boolean center, Boolean priorityIsName1,
	Boolean inTitle)
{
	Int16		name1MaxWidth;
	Int16		name2MaxWidth;
	Boolean	ignored;
	Int16		totalWidth;
//	Char *	highPriName;
	Char *	lowPriName;
	Int16		highPriNameWidth;
	Int16		lowPriNameWidth;
	
	
	// Check if both names fit
	totalWidth = name1Width + (name2 ? fieldSeparatorWidth : 0) + name2Width;
	
	// If we are supposed to center the names then move in the x position
	// by the amount that centers the text
	if (center && (nameExtent > totalWidth))
		{
		*x += (nameExtent - totalWidth) / 2;
		}

	// Special case if only name1 is given
	if (name2 == NULL)
		{
		// Draw name portion that fits in extent
		FntCharsInWidth(name1, (Int16*)&nameExtent, &name1Length, &ignored);
		if (inTitle)
			WinDrawInvertedChars(name1, name1Length, *x, y);
		else
			WinDrawChars(name1, name1Length, *x, y);
		// Add width of characters actually drawn
		*x += FntCharsWidth(name1, name1Length);
		return;
		}

	// Remove name separator width
	nameExtent -= fieldSeparatorWidth;
	
	// Test if both names fit 
	if ((name1Width + name2Width) <= nameExtent)
		{
		name1MaxWidth = name1Width;
		name2MaxWidth = name2Width;
		}
	else
		{
		// They dont fit. One or both needs truncation
		// Establish name priorities and their allowed widths
		// Change this to alter the ratio of the low and high priority name spaces
		Int16	highPriMaxWidth = (nameExtent * 2) / 3;	// 1/3 to low and 2/3 to high
		Int16	lowPriMaxWidth = nameExtent - highPriMaxWidth;
		
		// Save working copies of names and widths based on priority
		if (priorityIsName1)
			{
			// Priority is name1
//			highPriName = name1;
			highPriNameWidth = name1Width;
			lowPriName = name2;
			lowPriNameWidth = name2Width;
			}
		else
			{
			// Priority is name2
//			highPriName = name2;
			highPriNameWidth = name2Width;
			lowPriName = name1;
			lowPriNameWidth = name1Width;
			}

		// Does high priority name fit in high priority max width?
		if (highPriNameWidth > highPriMaxWidth)
			{
			// No. Look for word break in low priority name
			Char * spaceP = StrChr(lowPriName, spaceChr);
			if (spaceP != NULL)
				{
				// Found break. Set low priority name width to break width
				lowPriNameWidth = FntCharsWidth(lowPriName, spaceP - lowPriName);
				// Reclaim width from low pri name width to low pri max width, if smaller
				if (lowPriNameWidth < lowPriMaxWidth)
					{
					lowPriMaxWidth = lowPriNameWidth;
					// Set new high pri max width
					highPriMaxWidth = nameExtent - lowPriMaxWidth;
					}
				}
			}
		else
			{
			// Yes. Adjust maximum widths
			highPriMaxWidth = highPriNameWidth;
			lowPriMaxWidth = nameExtent - highPriMaxWidth;
			}
		
		// Convert priority widths back to name widths
		if (priorityIsName1)
			{
			// Priority is name1
			name1Width = highPriNameWidth;
			name2Width = lowPriNameWidth;
			name1MaxWidth = highPriMaxWidth;
			name2MaxWidth = lowPriMaxWidth;
			}
		else
			{
			// Priority is name2
			name1Width = lowPriNameWidth;
			name2Width = highPriNameWidth;
			name1MaxWidth = lowPriMaxWidth;
			name2MaxWidth = highPriMaxWidth;
			}
		}

	// Does name1 fit in its maximum width?
	if (name1Width > name1MaxWidth)
		{
		// No. Draw it to max width minus the ellipsis
		name1Width = name1MaxWidth - shortenedFieldWidth;
		FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
		if (inTitle)
			WinDrawInvertedChars(name1, name1Length, *x, y);
		else
			WinDrawChars(name1, name1Length, *x, y);
		*x += name1Width;
		
		// Draw ellipsis
		if (inTitle)
			WinDrawInvertedChars(shortenedFieldString, shortenedFieldLength, *x, y);
		else
			WinDrawChars(shortenedFieldString, shortenedFieldLength, *x, y);
		*x += shortenedFieldWidth;
		}
	else
		{
		// Yes. Draw name1 within its width
		FntCharsInWidth(name1, &name1Width, &name1Length, &ignored);
		if (inTitle)
			WinDrawInvertedChars(name1, name1Length, *x, y);
		else
			WinDrawChars(name1, name1Length, *x, y);
		*x += name1Width;
		}
	
	// Draw name separator
	if (inTitle)
		WinDrawInvertedChars(fieldSeparatorString, fieldSeparatorLength, *x, y);
	else
		WinDrawChars(fieldSeparatorString, fieldSeparatorLength, *x, y);
	*x += fieldSeparatorWidth;
	
	// Draw name2 within its maximum width
	FntCharsInWidth(name2, &name2MaxWidth, &name2Length, &ignored);
	if (inTitle)
		WinDrawInvertedChars(name2, name2Length, *x, y);
	else
		WinDrawChars(name2, name2Length, *x, y);
	*x += name2MaxWidth;
}


/***********************************************************************
 *
 * FUNCTION:    PhoneIsANumber
 *
 * DESCRIPTION: Determines whether the phone field contains a number or a
 * 				 string using the following heuristic: if the string contains
 *					 more numeric characters than non-numeric, it is a number.
 *
 * PARAMETERS:  phone - pointer to phone string
 *
 * RETURNED:    true if the string is a number.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         jeff   4/15/99   Initial Revision
 *
 ***********************************************************************/
 
static Boolean PhoneIsANumber( Char* phone )
{
	UInt16	digitCount = 0;
	UInt16	charCount = 0;
	UInt32 byteLength = 0;
	WChar	ch;
	
	byteLength += TxtGetNextChar( phone, byteLength, &ch );
	while ( ch != 0 )
		{
		charCount++;
		if ( TxtCharIsDigit( ch ) ) digitCount++;
		byteLength += TxtGetNextChar( phone, byteLength, &ch );
		}
		
	return ( digitCount > ( charCount / 2 ) );
}


/***********************************************************************
 *
 * FUNCTION:    DrawRecordNameAndPhoneNumber
 *
 * DESCRIPTION: Draws the namd and phone number (plus which phone)
 * within the screen bounds passed.
 *
 * PARAMETERS:  record - record to draw
 *              bounds - bounds of the draw region
 *              phoneLabelLetters - the first letter of each phone label
 *              sortByCompany - true if the database is sorted by company
 *              unnamedRecordStringPtr - string to use for unnamed records
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/21/95   Initial Revision
 *
 ***********************************************************************/
 
static void DrawRecordNameAndPhoneNumber (AddrDBRecordPtr record, 
   RectanglePtr bounds, Char * phoneLabelLetters, Boolean sortByCompany,
   Char **unnamedRecordStringPtr)
{
   Int16 x, y;
   UInt16 phoneLabel;
   Int16 fieldSeparatorWidth;
   Int16 shortenedFieldWidth;
   Char * name1;
   Char * name2;
   Char * phone;
   Int16 name1Length;
   Int16 name2Length;
   Int16 phoneLength;
   Int16 name1Width;
   Int16 name2Width;
   Int16 phoneWidth;
   UInt16 nameExtent;
   Boolean ignored;
	Boolean name1HasPriority;
	UInt8 phoneLabelWidth;
   

   x = bounds->topLeft.x;
   y = bounds->topLeft.y;
   
   phoneLabelWidth = FntCharWidth('W') - 1;		// remove the blank trailing column

   bounds->extent.x -= (phoneLabelWidth + 1);
   
   name1HasPriority = DetermineRecordName(record, &shortenedFieldWidth,
   	&fieldSeparatorWidth, sortByCompany, &name1, &name1Length, &name1Width, 
      &name2, &name2Length, &name2Width, unnamedRecordStringPtr, bounds->extent.x);

      
   phone = record->fields[phone1 + record->options.phones.displayPhoneForList];
   if (phone)
      {
      // Only show text from the first line in the field
      phoneWidth = bounds->extent.x;
      phoneLength = phoneWidth;         // more characters than we can expect
      FntCharsInWidth (phone, &phoneWidth, &phoneLength, &ignored);
      }
   else
      {
      phoneLength = 0;
      phoneWidth = 0;
      }

      
   if (bounds->extent.x   >= name1Width + (name2 ? fieldSeparatorWidth : 0) +
      name2Width + (phone ? spaceBetweenNamesAndPhoneNumbers : 0) + phoneWidth)
      {
      // we can draw it all!
      WinDrawChars(name1, name1Length, x, y);
      x += name1Width;
            
      // Is there a second name?
      if (name2)
         {
         if (name1)
            {
            WinDrawChars(fieldSeparatorString, fieldSeparatorLength, x, y);
            x += fieldSeparatorWidth;
            }
            
         // draw name2
         WinDrawChars(name2, name2Length, x, y);
         x += name2Width;
         }
      
      if (phone)
         WinDrawChars(phone, phoneLength, bounds->topLeft.x + bounds->extent.x 
            - phoneWidth, y);
                  
      }
   else
      {
		// Shortened math (970812 maf)
		nameExtent = bounds->extent.x - min(phoneWidth, PhoneColumnWidth);

      // Leave some space between names and numbers if there is a phone number      
      if (phone)
         nameExtent -= spaceBetweenNamesAndPhoneNumbers;

      DrawRecordName (name1, name1Length, name1Width, name2, name2Length, name2Width,
         nameExtent, &x, y, shortenedFieldWidth, fieldSeparatorWidth, false,
         name1HasPriority || !sortByCompany, false);
      
      if (phone)
         {
         x += spaceBetweenNamesAndPhoneNumbers;
         nameExtent = x - bounds->topLeft.x;
         
         
         // Now draw the phone number
         if (bounds->extent.x - nameExtent >= phoneWidth)
            {
            // We can draw it all
            WinDrawChars(phone, phoneLength, bounds->topLeft.x + bounds->extent.x 
               - phoneWidth, y);
            }
         else
            {
            // The phone number should be right justified instead of using
            // x from above because the string printed may be shorter
            // than we expect (CharsInWidth chops off space chars).
            phoneWidth = bounds->extent.x - nameExtent - shortenedFieldWidth;
            FntCharsInWidth(phone, &phoneWidth, &phoneLength, &ignored);
            WinDrawChars(phone, phoneLength, bounds->topLeft.x + bounds->extent.x -
               shortenedFieldWidth - phoneWidth, y);
               
            WinDrawChars(shortenedFieldString, shortenedFieldLength, 
               bounds->topLeft.x + bounds->extent.x - shortenedFieldWidth, y);
            }
         }
      }
   
   
   if (phone)
      {
      // Draw the first letter of the phone field label
      phoneLabel = GetPhoneLabel(record, firstPhoneField + 
         record->options.phones.displayPhoneForList);
		
		// find out if the first letter of the phone label is an O(ther) or
		// E(mail). If it is email don't draw the letter. If it is other, and the
		// contents of the phone field is not a number, don't draw the letter.
		if ( phoneLabel != emailLabel )
			{
			if ( (phoneLabel != otherLabel) || PhoneIsANumber (phone) )
				{
		      WinDrawChars (&phoneLabelLetters[phoneLabel], 1,
		         bounds->topLeft.x + bounds->extent.x + 1 + ((phoneLabelWidth - 
		         (FntCharWidth(phoneLabelLetters[phoneLabel]) - 1)) >> 1), y);//lint !e702
				}
			}
      }
}


/***********************************************************************
 *
 * FUNCTION:    ListViewLookupString
 *
 * DESCRIPTION: Adds a character to ListLookupField, looks up the 
 * string in the database and selects the item that matches.
 *
 * PARAMETERS:  event - EventPtr containing character to add to ListLookupField
 *                      
 * RETURNED:    true if the field handled the event
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/15/95   Initial Revision
 *
 ***********************************************************************/
static Boolean ListViewLookupString (EventType * event)
{
   FormPtr frm;
   UInt16 fldIndex;
   FieldPtr fldP;
   Char * fldTextP;
   TablePtr tableP;
   UInt16 foundRecord;
   Boolean completeMatch;
   Int16 length;
   
            
   frm = FrmGetActiveForm();
   fldIndex = FrmGetObjectIndex(frm, ListLookupField);
   FrmSetFocus(frm, fldIndex);
   fldP = FrmGetObjectPtr (frm, fldIndex);

   
   if (FldHandleEvent (fldP, event) || event->eType == fldChangedEvent)
      {
      fldTextP = FldGetTextPtr(fldP);
      tableP = FrmGetObjectPtr (frm, FrmGetObjectIndex(frm, ListTable));

      if (!AddrLookupString(AddrDB, fldTextP, SortByCompany, 
         CurrentCategory, &foundRecord, &completeMatch, 
         (PrivateRecordVisualStatus == maskPrivateRecords)))
         {
         // If the user deleted the lookup text remove the
         // highlight.
			CurrentRecord = noRecord;
         TblUnhighlightSelection(tableP);
         }
      else
         {
         ListViewSelectRecord(foundRecord);
         }
      
      
      if (!completeMatch)
         {
         // Delete the last character added.
         length = FldGetTextLength(fldP);
         FldDelete(fldP, length - 1, length);
         
         SndPlaySystemSound (sndError);
         }
         
      return true;
      }

   // Event not handled
   return false;
   
}


/***********************************************************************
 *
 * FUNCTION:    ListViewDrawRecord
 *
 * DESCRIPTION: This routine draws an address book record.  It is called as
 *              a callback routine by the table object.
 *
 * PARAMETERS:  table  - pointer to the address list table
 *              row    - row number, in the table, of the item to draw
 *              column - column number, in the table, of the item to draw
 *              bounds - bounds of the draw region
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/21/95   Initial Revision
 *
 ***********************************************************************/
 
static void ListViewDrawRecord (void * table, Int16 row, Int16 column, 
   RectanglePtr bounds)
{
   UInt16 recordNum;
   Err error;
   AddrDBRecordType record;
   MemHandle recordH;
   char noteChar;
   FontID currFont;
   


   // Get the record number that corresponds to the table item to draw.
   // The record number is stored in the "intValue" field of the item.
   // 
   recordNum = TblGetRowID (table, row);

   error = AddrGetRecord (AddrDB, recordNum, &record, &recordH);
   if (error)
   	{
	   ErrNonFatalDisplay ("Record not found");
   	return;
   	}


   if (column == 0)
      {
      currFont = FntSetFont (AddrListFont);
      DrawRecordNameAndPhoneNumber (&record, bounds, PhoneLabelLetters, 
         SortByCompany, &UnnamedRecordStringPtr);
		FntSetFont (currFont);
      }
   else
      {
		// Draw a not symbol if the field has a note
      if (record.fields[note])
      	{
         currFont = FntSetFont (symbolFont);
         noteChar = symbolNote;
         WinDrawChars (&noteChar, 1, bounds->topLeft.x, bounds->topLeft.y);
			FntSetFont (currFont);
        	}
      }
   MemHandleUnlock(recordH);
}


/***********************************************************************
 *
 * FUNCTION:    ListClearLookupString
 *
 * DESCRIPTION: Clears the ListLookupField.  Does not unhighlight the item.
 *
 * PARAMETERS:  nothing
 *                      
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/16/95   Initial Revision
 *
 ***********************************************************************/
static void ListClearLookupString ()
{
   FormPtr frm;
   UInt16 fldIndex;
   FieldPtr fldP;
   Int16 length;


   frm = FrmGetActiveForm();
   FrmSetFocus(frm, noFocus);
   fldIndex = FrmGetObjectIndex(frm, ListLookupField);
   fldP = FrmGetObjectPtr (frm, fldIndex);

   length = FldGetTextLength(fldP);
   if (length > 0)
   	{
   	// Clear it this way instead of with FldDelete to avoid sending a
   	// fldChangedEvent (which would undesirably unhighlight the item).
   	FldFreeMemory (fldP);
      FldDrawField (fldP);
      }
}


/***********************************************************************
 *
 * FUNCTION:    ListViewNumberOfRows
 *
 * DESCRIPTION: This routine return the maximun number of visible rows,
 *              with the current list view font setting.
 *
 * PARAMETERS:  table - List View table
 *
 * RETURNED:    maximun number of displayable rows
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/28/97	Initial Revision
 *
 ***********************************************************************/
static UInt16 ListViewNumberOfRows (TablePtr table)
{
	UInt16				rows;
	UInt16				rowsInTable;
	UInt16				tableHeight;
	FontID			currFont;
	RectangleType	r;


	rowsInTable = TblGetNumberOfRows (table);

	TblGetBounds (table, &r);
	tableHeight = r.extent.y;

	currFont = FntSetFont (AddrListFont);
	rows = tableHeight / FntLineHeight ();
	FntSetFont (currFont);

	if (rows <= rowsInTable)
		return (rows);
	else
		return (rowsInTable);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewUpdateScrollButtons
 *
 * DESCRIPTION: Show or hide the list view scroll buttons.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/21/95   Initial Revision
 *
 ***********************************************************************/
static void ListViewUpdateScrollButtons (void)
{
   UInt16 row;
   UInt16 upIndex;
   UInt16 downIndex;
   UInt16 recordNum;
   Boolean scrollableUp;
   Boolean scrollableDown;
   FormPtr   frm;
   TablePtr table;

   frm = FrmGetActiveForm ();

   // Update the button that scroll the list.
   //
   // If the first record displayed is not the fist record in the category,
   // enable the up scroller.
   recordNum = TopVisibleRecord;
   scrollableUp = SeekRecord (&recordNum, 1, dmSeekBackward);


   // Find the record in the last row of the table
   table = GetObjectPtr (ListTable);
	row = TblGetLastUsableRow (table);
	if (row != tblUnusableRow)
		recordNum = TblGetRowID (table, row);


   // If the last record displayed is not the last record in the category,
   // enable the down scroller.
   scrollableDown = SeekRecord (&recordNum, 1, dmSeekForward);


   // Update the scroll button.
   upIndex = FrmGetObjectIndex (frm, ListUpButton);
   downIndex = FrmGetObjectIndex (frm, ListDownButton);
   FrmUpdateScrollers (frm, upIndex, downIndex, scrollableUp, scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    ListLoadTable
 *
 * DESCRIPTION: This routine loads address book database records into
 *              the list view form.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
static void ListLoadTable (void)
{
   UInt16      row;
   UInt16      numRows;
	UInt16		lineHeight;
	UInt16		recordNum;
	UInt16		visibleRows;
	FontID		currFont;
   TablePtr 	table;
	UInt16 		attr;
	Boolean		masked;

   
   // For each row in the table, store the record number as the row id.
   table = GetObjectPtr (ListTable);

   TblUnhighlightSelection(table);

   // Make sure we haven't scrolled too far down the list of records
   // leaving blank lines in the table.

   // Try going forward to the last record that should be visible
	visibleRows = ListViewNumberOfRows (table);
	recordNum = TopVisibleRecord;
	if (!SeekRecord (&recordNum, visibleRows - 1, dmSeekForward))
      {
      // We have at least one line without a record.  Fix it.
      // Try going backwards one page from the last record
      TopVisibleRecord = dmMaxRecordIndex;
		if (!SeekRecord (&TopVisibleRecord, visibleRows - 1, dmSeekBackward))
         {
         // Not enough records to fill one page.  Start with the first record
         TopVisibleRecord = 0;
         SeekRecord (&TopVisibleRecord, 0, dmSeekForward);
         }
      }


	currFont = FntSetFont (AddrListFont);
	lineHeight = FntLineHeight ();
	FntSetFont (currFont);

   numRows = TblGetNumberOfRows (table);
   recordNum = TopVisibleRecord;

	for (row = 0; row < visibleRows; row++)
      {
      if ( ! SeekRecord (&recordNum, 0, dmSeekForward))
         break;

      // Make the row usable.
      TblSetRowUsable (table, row, true);
      
		DmRecordInfo (AddrDB, recordNum, &attr, NULL, NULL);
   	masked = (((attr & dmRecAttrSecret) && PrivateRecordVisualStatus == maskPrivateRecords));
		TblSetRowMasked(table,row,masked);

      // Mark the row invalid so that it will draw when we call the 
      // draw routine.
      TblMarkRowInvalid (table, row);

      // Store the record number as the row id.
      TblSetRowID (table, row, recordNum);

		TblSetItemFont (table, row, nameAndNumColumn, AddrListFont);
		TblSetRowHeight (table, row, lineHeight);

      recordNum++;
      }
   

   // Hide the item that don't have any data.
   while (row < numRows)
      {      
      TblSetRowUsable (table, row, false);
      row++;
      }

   ListViewUpdateScrollButtons();
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
 *                     CurrentCategory
 *                     ShowAllCategories
 *                     CategoryName
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   06/05/95   Initial Revision
 *			  gap	  08/13/99   Update to use new constant categoryDefaultEditCategoryString.
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
   categoryEdited = CategorySelect (AddrDB, frm, ListCategoryTrigger,
                 ListCategoryList, true, &category, CategoryName, 1, categoryDefaultEditCategoryString);
   
   if (category == dmAllCategories)
      ShowAllCategories = true;
   else
      ShowAllCategories = false;
      
   if ( categoryEdited || (category != CurrentCategory))
      {
      ChangeCategory (category);

      // Display the new category.
      ListLoadTable ();
      table = GetObjectPtr (ListTable);
      TblEraseTable (table);
      TblDrawTable (table);
      
      ListClearLookupString ();
	   
	   // By changing the category the current record is lost.
	   CurrentRecord = noRecord;
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
 *                     CurrentCategory
 *                     ShowAllCategories
 *                     CategoryName
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   9/15/95   Initial Revision
 *         rsf   9/20/95   Copied from To Do
 *
 ***********************************************************************/
static void ListViewNextCategory (void)
{
   UInt16 category;
   TablePtr table;
   ControlPtr ctl;   


   category = CategoryGetNext (AddrDB, CurrentCategory);
	
	if (category != CurrentCategory)
		{
	   if (category == dmAllCategories)
	      ShowAllCategories = true;
	   else
	      ShowAllCategories = false;

	   ChangeCategory (category);

	   // Set the label of the category trigger.
	   ctl = GetObjectPtr (ListCategoryTrigger);
	   CategoryGetName (AddrDB, CurrentCategory, CategoryName);
	   CategorySetTriggerLabel (ctl, CategoryName);


	   // Display the new category.
	   ListLoadTable ();
	   table = GetObjectPtr (ListTable);
	   TblEraseTable (table);
	   TblDrawTable (table);
	   
	   // By changing the category the current record is lost.
	   CurrentRecord = noRecord;
	   }
}


/***********************************************************************
 *
 * FUNCTION:    ListViewScroll
 *
 * DESCRIPTION: This routine scrolls the list of names and phone numbers 
 *              in the direction specified.
 *
 * PARAMETERS:  direction	- up or dowm
 *              units		- unit amount to scroll
 *              byLine		- if true, list scrolls in line units
 *									- if false, list scrolls in page units
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			art		6/5/95	Initial Revision
 *       frigino	8/14/97	Modified to scroll by line or page in units
 *       gap   	10/12/99	Close command bar before processing scroll
 *       gap   	10/15/99	Clean up selection handling after scroll
 *       gap   	10/25/99	Optimized scrolling to only redraw if item position changed
 *
 ***********************************************************************/
static void ListViewScroll (WinDirectionType direction, UInt16 units, Boolean byLine)
{
	TablePtr table;
	UInt16 rowsInPage;
	UInt16 newTopVisibleRecord;
	UInt16 prevTopVisibleRecord = TopVisibleRecord;
	
	
	// Before processing the scroll, be sure that the command bar has been closed.
	MenuEraseStatus (0);

	table = GetObjectPtr (ListTable);
	// Safe. There must be at least one row in the table.
	rowsInPage = ListViewNumberOfRows (table) - 1;
	newTopVisibleRecord = TopVisibleRecord;

	// Scroll the table down.
	if (direction == winDown)
		{
		// Scroll down by line units
		if (byLine)
			{
			// Scroll down by the requested number of lines
			if (!SeekRecord (&newTopVisibleRecord, units, dmSeekForward))
				{
				// Tried to scroll past bottom. Goto last record
				newTopVisibleRecord = dmMaxRecordIndex;
				SeekRecord (&newTopVisibleRecord, 1, dmSeekBackward);
				}
			}
		// Scroll in page units
		else
			{
			// Try scrolling down by the requested number of pages
			if (!SeekRecord (&newTopVisibleRecord, units * rowsInPage, dmSeekForward))
				{
				// Hit bottom. Try going backwards one page from the last record
				newTopVisibleRecord = dmMaxRecordIndex;
				if (!SeekRecord (&newTopVisibleRecord, rowsInPage, dmSeekBackward))
					{
					// Not enough records to fill one page. Goto the first record
					newTopVisibleRecord = 0;
					SeekRecord (&newTopVisibleRecord, 0, dmSeekForward);
					}
				}
			}
		}
	// Scroll the table up
	else
		{
		// Scroll up by line units
		if (byLine)
			{
			// Scroll up by the requested number of lines
			if (!SeekRecord (&newTopVisibleRecord, units, dmSeekBackward))
				{
				// Tried to scroll past top. Goto first record
				newTopVisibleRecord = 0;
				SeekRecord (&newTopVisibleRecord, 0, dmSeekForward);
				}
			}
		// Scroll in page units
		else
			{
			// Try scrolling up by the requested number of pages
			if (!SeekRecord (&newTopVisibleRecord, units * rowsInPage, dmSeekBackward))
				{
				// Hit top. Goto the first record
				newTopVisibleRecord = 0;
				SeekRecord (&newTopVisibleRecord, 0, dmSeekForward);
				}
			}
		}


	// Avoid redraw if no change
	if (TopVisibleRecord != newTopVisibleRecord)
		{
		TopVisibleRecord = newTopVisibleRecord;
		CurrentRecord = noRecord;  	// scrolling always deselects current selection
		ListLoadTable();
		
		// Need to compare the previous top record to the current after ListLoadTable 
		// as it will adjust TopVisibleRecord if drawing from newTopVisibleRecord will
		// not fill the whole screen with items.
		if (TopVisibleRecord != prevTopVisibleRecord)
			TblRedrawTable(table);
		}
}


/***********************************************************************
 *
 * FUNCTION:    ListViewSelectRecord
 *
 * DESCRIPTION: Selects (highlights) a record on the table, scrolling
 *              the record if neccessary.  Also sets the CurrentRecord.
 *
 * PARAMETERS:  recordNum - record to select
 *                      
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  6/30/95   Initial Revision
 *
 ***********************************************************************/
static void ListViewSelectRecord (UInt16 recordNum)
{
   Int16 row, column;
   TablePtr tableP;
   UInt16 attr;

	if (recordNum == noRecord)
		return;
   ErrFatalDisplayIf (recordNum >= DmNumRecords(AddrDB), "Record outside AddrDB");


   tableP = GetObjectPtr (ListTable);
   
  	if (PrivateRecordVisualStatus > showPrivateRecords)
     {
     // If the record is hidden stop trying to show it.
     if (!DmRecordInfo(AddrDB, recordNum, &attr, NULL, NULL) && (attr & dmRecAttrSecret))
        {
        CurrentRecord = noRecord;
        TblUnhighlightSelection (tableP);
        return;
        }
     }



   // Don't change anything if the same record is selected
   if (TblGetSelection(tableP, &row, &column) &&
      recordNum == TblGetRowID (tableP, row))
      {
      return;
      }
      

   // See if the record is displayed by one of the rows in the table
   // A while is used because if TblFindRowID fails we need to
   // call it again to find the row in the reloaded table.
   while (!TblFindRowID(tableP, recordNum, &row))
      {
            
      // Scroll the view down placing the item
      // on the top row
      TopVisibleRecord = recordNum;

      // Make sure that TopVisibleRecord is visible in CurrentCategory
      if (CurrentCategory != dmAllCategories)
         {
         // Get the category and the secret attribute of the current record.
         DmRecordInfo (AddrDB, TopVisibleRecord, &attr, NULL, NULL);   
         if ((attr & dmRecAttrCategoryMask) != CurrentCategory)
            {
            ErrNonFatalDisplay("Record not in CurrentCategory");
            CurrentCategory = (attr & dmRecAttrCategoryMask);
            }
         }
   
      ListLoadTable();
      TblRedrawTable(tableP);
      }

   
   // Select the item
   TblSelectItem (tableP, row, nameAndNumColumn);
   
   CurrentRecord = recordNum;
}   
   

/***********************************************************************
 *
 * FUNCTION:    ListViewUpdateDisplay
 *
 * DESCRIPTION: This routine update the display of the list view
 *
 * PARAMETERS:  updateCode - a code that indicated what changes have been
 *                           made to the to do list.
 *                      
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *				Name	Date		Description
 *				----	----		-----------
 *				art	6/5/95	Initial Revision
 *				ppo	10/13/99	Fixed bug #22753 (selection not redrawn)
 *				jmp	10/19/99	Changed previous fix to actually to set everything
 *									back up.  The previous change caused bug #23053, and
 *									didn't work in several cases anyway!  Also, optimized
 *									this routine space-wise.
 *
 ***********************************************************************/
static void ListViewUpdateDisplay (UInt16 updateCode)
{
   TablePtr table = GetObjectPtr (ListTable);

	if (updateCode == frmRedrawUpdateCode)
		{
		FrmDrawForm (FrmGetActiveForm ());
		}

	if (updateCode & updateRedrawAll || 
		updateCode & updateFontChanged /*|| 
		updateCode == frmRedrawUpdateCode*/)
		{
		ListLoadTable ();
		TblRedrawTable (table);
		}

	if (updateCode & updateFontChanged || 
		updateCode & updateSelectCurrentRecord /*|| 
		updateCode == frmRedrawUpdateCode*/)
		{
		if (CurrentRecord != noRecord)
			ListViewSelectRecord (CurrentRecord);
		}
}


/***********************************************************************
 *
 * FUNCTION:    ListViewDeleteRecord
 *
 * DESCRIPTION: This routine deletes an address record. This routine is 
 *              called when the delete button in the command bar is
 *              pressed when address book is in list view.  The benefit
 *					 this routine proides over DetailsDeleteRecord is that the
 *					 selection is maintained right up to the point where the address
 *					 is being deleted.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the record was delete or archived.
 *
 * REVISION HISTORY:
 *         	Name	Date			Description
 *         	----	----			-----------
 *				gap	11/01/99		new
 *
 ***********************************************************************/
static Boolean ListViewDeleteRecord (void)
{
   UInt16	ctlIndex;
   UInt16	buttonHit;
   FormPtr	alert;
   Boolean	archive;
   TablePtr	table;

         
   // Display an alert to comfirm the operation.
   alert = FrmInitForm (DeleteAddrDialog);

   // Set the "save backup" checkbox to its previous setting.
   ctlIndex = FrmGetObjectIndex (alert, DeleteAddrSaveBackup);
   FrmSetControlValue (alert, ctlIndex, SaveBackup);

   buttonHit = FrmDoDialog (alert);

   archive = FrmGetControlValue (alert, ctlIndex);

   FrmDeleteForm (alert);
   if (buttonHit == DeleteAddrCancel)
      return (false);

   // Remember the "save backup" checkbox setting.
   SaveBackup = archive;

   // Clear the highlight on the selection before deleting the item.
   table = GetObjectPtr (ListTable);
   TblUnhighlightSelection(table);

   DeleteRecord(archive);
   
   return (true);
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
 *				Name		Date		Description
 *				----		----		-----------
 *				art		6/5/95	Initial Revision
 *				jmp		10/01/99	Changed call to DmOpenDatabaseByTypeCreator() to
 *										AddrGetDatabase().
 *				jwm	 1999-10-8	After deleting a record, highlight the new selection and
 *										clear the now possibly completely wrong lookup field.
 *
 ***********************************************************************/
static Boolean ListViewDoCommand (UInt16 command)
{
	UInt16 	newRecord;
	UInt16 	numCharsToHilite;
	Boolean	wasHiding;
	UInt16 	mode;
  
	switch (command)
      {
      /*
      case ListRecordSelectBusinessCardCmd:
			if (CurrentRecord != noRecord)
				{
		         MenuEraseStatus (0);
		         if (FrmAlert(SelectBusinessCardAlert) == SelectBusinessCardYes)
		         	{
					DmRecordInfo (AddrDB, CurrentRecord, NULL, &BusinessCardRecordID, NULL);
		         	}
				}
			else
				SndPlaySystemSound (sndError);
         return true;
      */
      
      case ListRecordSendBusinessCardCmd:
         MenuEraseStatus (0);
      	AddrSendBusinessCard(AddrDB);
         return true;
      
      case ListRecordSendCategoryCmd:
         MenuEraseStatus (0);
      	AddrSendCategory(AddrDB, CurrentCategory);
         return true;
      
	  case ListRecordDuplicateAddressCmd:
			if (CurrentRecord == noRecord)
				{
				//FrmAlert ();
				return true;
				}
			newRecord = DuplicateCurrentRecord (&numCharsToHilite, false);
			
			// If we have a new record take the user to be able to edit it
			// automatically.
			if (newRecord != noRecord)
				{
				NumCharsToHilite = numCharsToHilite;
				CurrentRecord = newRecord;
				FrmGotoForm (EditView);
				}
			return true;

      case ListRecordDeleteRecordCmd:
			if (CurrentRecord != noRecord)
				{
				if (ListViewDeleteRecord ())
					{
					ListClearLookupString ();
					ListViewUpdateDisplay (updateRedrawAll | updateSelectCurrentRecord);
					}
				}
			else
				SndPlaySystemSound (sndError);
			return true;

      case ListRecordSendRecordCmd:
			if (CurrentRecord != noRecord)
				{
				MenuEraseStatus (0);
				AddrSendRecord(AddrDB, CurrentRecord);
				}
			else
				SndPlaySystemSound (sndError);
			return true;


		case ListOptionsFontCmd:
         MenuEraseStatus (0);
			AddrListFont = SelectFont (AddrListFont);
			ListViewUpdateDisplay (updateRedrawAll);
         return true;

      case ListOptionsListByCmd:
         MenuEraseStatus (0);
         ListClearLookupString();
         FrmPopupForm (PreferencesDialog);
         return true;
         
      case ListOptionsEditCustomFldsCmd:
         MenuEraseStatus (0);
         FrmPopupForm (CustomEditDialog);
         return true;
         
		case ListOptionsSecurityCmd:
			wasHiding = (PrivateRecordVisualStatus == hidePrivateRecords);
			 
			PrivateRecordVisualStatus = SecSelectViewStatus();
			
			if (wasHiding != (PrivateRecordVisualStatus == hidePrivateRecords))
				{
				
				// Close the application's data file.
				DmCloseDatabase (AddrDB);	
				
				mode = (PrivateRecordVisualStatus == hidePrivateRecords) ?
					dmModeReadWrite : (dmModeReadWrite | dmModeShowSecret);
				
				AddrGetDatabase(&AddrDB, mode);
				ErrFatalDisplayIf(!AddrDB,"Can't reopen DB");
				
				}
			
			//For safety, simply reset the currentRecord
			TblReleaseFocus (GetObjectPtr (ListTable));
			ListViewUpdateDisplay (updateRedrawAll | updateSelectCurrentRecord);
				//updateSelectCurrentRecord will cause currentRecord to be reset to noRecord if hidden or masked
			break;
                  
      case ListOptionsAboutCmd:
         MenuEraseStatus (0);
         AbtShowAbout (sysFileCAddress);
         return true;

      default:
            break;
      }
   return false;
}


/***********************************************************************
 *
 * FUNCTION:    ListViewInit
 *
 * DESCRIPTION: This routine initializes the "List View" of the 
 *              Address application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
static void ListViewInit (FormPtr frm)
{
   UInt16 row;
   UInt16 rowsInTable;
   TablePtr table;
   ControlPtr ctl;

   if (ShowAllCategories)
      CurrentCategory = dmAllCategories;


   // Initialize the address list table.
   table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ListTable));
   rowsInTable = TblGetNumberOfRows (table);
   for (row = 0; row < rowsInTable; row++)
      {      
      TblSetItemStyle (table, row, nameAndNumColumn, customTableItem);
      TblSetItemStyle (table, row, noteColumn, customTableItem);
      TblSetRowUsable (table, row, false);
      }

   TblSetColumnUsable (table, nameAndNumColumn, true);
   TblSetColumnUsable (table, noteColumn, true);
   
   TblSetColumnMasked (table, nameAndNumColumn, true);
   TblSetColumnMasked (table, noteColumn, true);


   // Set the callback routine that will draw the records.
   TblSetCustomDrawProcedure (table, nameAndNumColumn, ListViewDrawRecord);
   TblSetCustomDrawProcedure (table, noteColumn, ListViewDrawRecord);


   // Load records into the address list.
   ListLoadTable ();


   // Set the label of the category trigger.
   ctl = GetObjectPtr (ListCategoryTrigger);
   CategoryGetName (AddrDB, CurrentCategory, CategoryName);
   CategorySetTriggerLabel (ctl, CategoryName);

   
//   ListViewUpdateScrollButtons();
   
   // Turn on the cursor in the lookup field.
//   FrmSetFocus(frm, FrmGetObjectIndex (frm, ListLookupField));
}


/***********************************************************************
 *
 * FUNCTION:    ListViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "List View"
 *              of the Address Book application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			art		6/5/95   Initial Revision
 *			frigino	8/15/97	Added scroll rate acceleration using
 *									ResetScrollRate() and AdjustScrollRate()
 *			jmp		9/17/99	Use NewNoteView instead of NoteView.
 *
 ***********************************************************************/
static Boolean ListViewHandleEvent (EventPtr event)
{
   FormPtr frm;
   Boolean handled = false;
   TablePtr table;
   Int16 row;
   Int16 column;
   Char *cmdText;
	
   switch (event->eType)
   	{
      case tblSelectEvent:
      	if (TblRowMasked(event->data.tblSelect.pTable, 
                                      event->data.tblSelect.row))
         	{
				if (SecVerifyPW (showPrivateRecords) == true)
					{
					PrivateRecordVisualStatus = showPrivateRecords;
					event->data.tblSelect.column = nameAndNumColumn; //force non-note view
					}
				else
					break;
				}
         	
         // An item in the list of names and phone numbers was selected, go to
         // the record view.
         CurrentRecord = TblGetRowID (event->data.tblSelect.pTable, 
                                      event->data.tblSelect.row);
                              
         // Set the global variable that determines which field is the top visible
         // field in the edit view.  Also done when New is pressed.
         TopVisibleFieldIndex = 0;
         EditRowIDWhichHadFocus = editFirstFieldIndex;
         EditFieldPosition = 0;
         
         if (event->data.tblSelect.column == nameAndNumColumn)
            FrmGotoForm (RecordView);
         else
            if (CreateNote())
               FrmGotoForm (NewNoteView);
         handled = true;
         break;

      case ctlSelectEvent:
         switch (event->data.ctlSelect.controlID) {
            case ListCategoryTrigger:
/*
// SCL: Commented out test code as per Art (by phone 1/16/98)
				//   Test code for the lookup function.
				{
				Char * resultString;
				AddrLookupParamsType params;

				// parameters for the lookup
				params.title = "Send Email To:";
				params.pasteButtonText = "Add";
				StrCopy (params.lookupString, "1");
				params.field1 = addrLookupSortField;
				params.field2 = addrLookupEmail;
				params.field2Optional = false;
				params.userShouldInteract = true;
				params.formatStringP = "^email";

				//	params.field1 = addrLookupSortField;
				//	params.field1 = addrLookupState;
				//	params.field2 = addrLookupEmail;
				//	params.field2 = addrLookupSortField;
				//	params.field2 = addrLookupState;
				//	params.field2 = addrLookupListPhone;
				//	params.field2 = addrLookupNoField;
				//	params.userShouldInteract = true;
				//params.formatStringP = "^first ^name, ^title, ^company";

				Lookup(&params);

				if (params.resultStringH)
				{
				resultString = MemHandleLock(params.resultStringH);
				MemHandleUnlock(params.resultStringH);
				}

				break;
				}
				// End test code
*/

               ListViewSelectCategory ();
               handled = true;
               break;

            case ListNewButton:
               EditViewNewRecord();
               handled = true;
               break;
         }
         break;

		case ctlEnterEvent:
			switch (event->data.ctlEnter.controlID)
				{
				case ListUpButton:
				case ListDownButton:
					// Reset scroll rate
					ResetScrollRate();
					// Clear lookup string
					ListClearLookupString ();
					// leave unhandled so the buttons can repeat
					break;
				}
         break;

      case ctlRepeatEvent:
			// Adjust the scroll rate if necessary
			AdjustScrollRate();

         switch (event->data.ctlRepeat.controlID)
         	{
            case ListUpButton:
					ListViewScroll (winUp, ScrollUnits, false);
					// leave unhandled so the buttons can repeat
               break;
               
            case ListDownButton:
					ListViewScroll (winDown, ScrollUnits, false);
					// leave unhandled so the buttons can repeat
               break;
            default:
               break;
	         }
         break;


      case keyDownEvent:
         // Address Book key pressed for the first time?
         if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
         	{
            if (! (event->data.keyDown.modifiers & poweredOnKeyMask))
            	{
               ListClearLookupString ();
               ListViewNextCategory ();
               handled = true;
            	}
            }
			
			else if (EvtKeydownIsVirtual(event))
				{
				switch (event->data.keyDown.chr)
					{
				   case vchrPageUp:
				   	// Reset scroll rate if not auto repeating
				   	if ((event->data.keyDown.modifiers & autoRepeatKeyMask) == 0)
				   		{
				   		ResetScrollRate();
				   		}
						// Adjust the scroll rate if necessary
						AdjustScrollRate();
				      ListViewScroll (winUp, ScrollUnits, false);
				      ListClearLookupString ();
				      handled = true;
				      break;
				      
				   case vchrPageDown:
				   	// Reset scroll rate if not auto repeating
				   	if ((event->data.keyDown.modifiers & autoRepeatKeyMask) == 0)
				   		{
				   		ResetScrollRate();
				   		}
						// Adjust the scroll rate if necessary
						AdjustScrollRate();
				      ListViewScroll (winDown, ScrollUnits, false);
				      ListClearLookupString ();
				      handled = true;
				      break;

	            case vchrSendData:
						if (CurrentRecord != noRecord)
							{
							MenuEraseStatus (0);
							AddrSendRecord(AddrDB, CurrentRecord);
							}
						else
							SndPlaySystemSound (sndError);
			         handled = true;
				      break;
					}
				}
			
			else if (event->data.keyDown.chr == linefeedChr)
				{
				frm = FrmGetActiveForm ();
				table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ListTable));
				if (TblGetSelection (table, &row, &column))
					{
					// Set the global variable that determines which field is the top visible
					// field in the edit view.  Also done when New is pressed.
					TopVisibleFieldIndex = 0;
				
					FrmGotoForm (RecordView);
					}
				handled = true;
				}
			
			else
				{
				handled = ListViewLookupString(event);
				}
         break;

      case fldChangedEvent:
         ListViewLookupString(event);
         handled = true;
         break;
      
      case menuEvent:
         return ListViewDoCommand (event->data.menu.itemID);
         
		case menuCmdBarOpenEvent:
			if (CurrentRecord != noRecord)
				{
				// because this isn't a real menu command, get the text for the button from a resource
				cmdText = MemHandleLock(DmGetResource (strRsc, DeleteRecordStr));
				MenuCmdBarAddButton(menuCmdBarOnLeft, BarDeleteBitmap, menuCmdBarResultMenuItem, ListRecordDeleteRecordCmd, cmdText);
				MemPtrUnlock(cmdText);
				}
			MenuCmdBarAddButton(menuCmdBarOnLeft, BarSecureBitmap, menuCmdBarResultMenuItem, ListOptionsSecurityCmd, 0);
			if (CurrentRecord != noRecord)
				{
				// because this isn't a real menu command, get the text for the button from a resource
				cmdText = MemHandleLock(DmGetResource (strRsc, SendRecordStr));
				MenuCmdBarAddButton(menuCmdBarOnLeft, BarBeamBitmap, menuCmdBarResultMenuItem, ListRecordSendRecordCmd, cmdText);
				MemPtrUnlock(cmdText);
				}

			// tell the field package to not add cut/copy/paste buttons automatically; we
			// don't want it for the lookup field since it'd cause confusion.
			event->data.menuCmdBarOpen.preventFieldButtons = true;

			// don't set handled to true; this event must fall through to the system.
			break;
      
      case frmCloseEvent:
         if (UnnamedRecordStringPtr)
         	{
            MemPtrUnlock(UnnamedRecordStringPtr);
            UnnamedRecordStringPtr = NULL;
//          DmReleaseResource(strRsc, UnnamedRecordStr)
         	}
         break;

      case frmOpenEvent:
         frm = FrmGetActiveForm ();
         ListViewInit (frm);


         // Make sure the record to be selected is one of the table's rows or
         // else it reloads the table with the record at the top.  Nothing is
         // drawn by this because the table isn't visible.
         if (ListViewSelectThisRecord != noRecord)
         	{
            ListViewSelectRecord(ListViewSelectThisRecord);
         	}


         FrmDrawForm (frm);

         // Select the record.  This finds which row to select it and does it.
         if (ListViewSelectThisRecord != noRecord)
         	{
            ListViewSelectRecord(ListViewSelectThisRecord);
            ListViewSelectThisRecord = noRecord;
         	}
      
         // Set the focus in the lookup field so that the user can easily
         // bring up the keyboard.
         FrmSetFocus(frm, FrmGetObjectIndex(frm, ListLookupField));
      
         PriorAddressFormID = FrmGetFormId (frm);
         handled = true;
         break;

      case frmUpdateEvent:
         ListViewUpdateDisplay (event->data.frmUpdate.updateCode);
         handled = true;
         break;
      
      default:
      	break;
   	}
      
   return (handled);
}


#pragma mark -


/***********************************************************************
 *
 * FUNCTION:    CustomAcceptBeamDialog
 *
 * DESCRIPTION: This routine uses uses a new exchange manager function to
 *				Ask the user if they want to accept the data as well as set
 *				the category to put the data in. By default all data will go 
 *				to the unfiled category, but the user can select another one.
 *				We store the selected category index in the appData field of 
 *				the exchange socket so we have it at the when we get the receive
 *				data launch code later.
 *
 * PARAMETERS:  dbP - open database that holds category information
 *				askInfoP - structure passed on exchange ask launchcode
 *
 * RETURNED:    Error if any
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bhall	9/7/99	Initial Revision
 *			gavin   11/9/99  Rewritten to use new ExgDoDialog function
 *
 ***********************************************************************/
static Err CustomAcceptBeamDialog(DmOpenRef dbP, ExgAskParamPtr askInfoP)
{
	ExgDialogInfoType	exgInfo;
	Err err;
	Boolean result;
	
	// set default category to unfiled
	exgInfo.categoryIndex = dmUnfiledCategory;
	// Store the database ref into a gadget for use by the event handler
	exgInfo.db = dbP;
	
	// Let the exchange manager run the dialog for us
	result = ExgDoDialog(askInfoP->socketP, &exgInfo, &err);


	if (!err && result) {

		// pretend as if user hit OK, we'll now accept the data
		askInfoP->result = exgAskOk;
		
		// Stuff the category index into the appData field
		askInfoP->socketP->appData = exgInfo.categoryIndex;
	} else {
		// pretend as if user hit cancel, we won't accept the data
		askInfoP->result = exgAskCancel;
	}

	return err;
}


#pragma mark -


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
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   6/5/95      Initial Revision
 *
 ***********************************************************************/
extern void * GetObjectPtr (UInt16 objectID)
{
   FormPtr frm;
   
   frm = FrmGetActiveForm ();
   return (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objectID)));

}




/***********************************************************************
 *
 * FUNCTION:    AddrSendBusinessCard
 *
 * DESCRIPTION: Send the Business Card record or complain if none selected.
 *
 * PARAMETERS:  dbP - the database
 *
 * RETURNED:    true if the record is found and sent
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  10/20/97  Initial Revision
 *
 ***********************************************************************/
static Boolean AddrSendBusinessCard (DmOpenRef dbP)
{
	UInt16 recordNum;
	
	
	if (DmFindRecordByID (AddrDB, BusinessCardRecordID, &recordNum) == dmErrUniqueIDNotFound ||
		DmQueryRecord(dbP, recordNum) == 0)
		FrmAlert(SendBusinessCardAlert);
	else
		{
		AddrSendRecord(dbP, recordNum);
		return true;
		}
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:     InitPhoneLabelLetters
 *
 * DESCRIPTION:  Init the list of first letters of phone labels.  Used
 * in the list view and for find.
 *
 * PARAMETERS:   appInfoPtr - contains the field labels
 *                 phoneLabelLetters - array of characters (one for each phone label)
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   7/24/95   Initial Revision
 *
 ***********************************************************************/
extern void InitPhoneLabelLetters(AddrAppInfoPtr appInfoPtr, Char * phoneLabelLetters)
{
   UInt16 i;


   // Get the first char of the phone field labels for the list view.
   for (i = 0; i < numPhoneLabels; i++){
      phoneLabelLetters[i] = appInfoPtr->fieldLabels[i + 
         ((i < numPhoneLabelsStoredFirst) ? firstPhoneField : 
          (addressFieldsCount - numPhoneLabelsStoredFirst))][0];
   }
}


/***********************************************************************
 *
 * FUNCTION:    AppHandleKeyDown
 *
 * DESCRIPTION: Handle the key being down.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed on
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger  10/22/97  Initial Revision
 *
 ***********************************************************************/
static Boolean AppHandleKeyDown (EventType * event)
{
   // Check if a button being held down is released
   if (TickAppButtonPushed != 0)
   	{
   	// This is the case when the button is let up
   	if ((KeyCurrentState() & (keyBitHard1 | keyBitHard2 | keyBitHard3 | keyBitHard4)) == 0)
	   	{
	   	if (BusinessCardSentForThisButtonPress)
	   		{
	   		BusinessCardSentForThisButtonPress = false;
	      	
	      	TickAppButtonPushed = 0;
	      	
				// Allow the masked off key to now send keyDownEvents.
				KeySetMask(keyBitsAll);
	   		}
	   	else if (event->eType == nilEvent)
	   		{
	      	// Send the keyDownEvent to the app.  It was stripped out
	      	// before but now it can be sent over the nullEvent.  It
	      	// may be nullChr from when the app was launched.  In that case
	      	// we don't need to send the app's key because the work expected,
	      	// which was switching to this app, has already been done.
	      	if (AppButtonPushed != nullChr)
	      		{
		      	event->eType = keyDownEvent;
		      	event->data.keyDown.chr = AppButtonPushed;
		      	event->data.keyDown.modifiers = AppButtonPushedModifiers;
		      	}
	      	
	      	TickAppButtonPushed = 0;
	      	
				// Allow the masked off key to now send keyDownEvents.
				KeySetMask(keyBitsAll);
	      	}
	   	}
	   // This is the case when the button is depresed long enough to send the business card
	   else if (TickAppButtonPushed + AppButtonPushTimeout <= TimGetTicks() &&
	   	!BusinessCardSentForThisButtonPress)
   		{
			BusinessCardSentForThisButtonPress = true;
			AddrSendBusinessCard(AddrDB);
			}
		}
	   
   
	else if (event->eType == keyDownEvent)
		{
      if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr) &&
      	!(event->data.keyDown.modifiers & autoRepeatKeyMask))
      	{
			// Remember which hard key is mapped to the Address Book
			// because it may need to be sent later.
			AppButtonPushed = event->data.keyDown.chr;
			AppButtonPushedModifiers = event->data.keyDown.modifiers;
			
			TickAppButtonPushed = TimGetTicks();
			
			// Mask off the key to avoid repeat keys causing clicking sounds
			KeySetMask(~KeyCurrentState());
			
			// Don't process the key
			return true;
			}
		}
   
   return false;
}


/***********************************************************************
 *
 * FUNCTION:    ApplicationHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art    9/11/95   Initial Revision
 *         jmp		9/17/99   Use NewNoteView instead of NoteView.
 *
 ***********************************************************************/
static Boolean ApplicationHandleEvent (EventType * event)
{
   UInt16 formId;
   FormPtr frm;

   if (event->eType == frmLoadEvent)
   	{
      
      // Load the form resource.
      formId = event->data.frmLoad.formID;
      frm = FrmInitForm (formId);
      FrmSetActiveForm (frm);      
      
      // Set the event handler for the form.  The handler of the currently
      // active form is called by FrmHandleEvent each time is receives an
      // event.
      switch (formId)
      	{
         case ListView:
            FrmSetEventHandler(frm, ListViewHandleEvent);
            break;
      
         case RecordView:
            FrmSetEventHandler(frm, RecordViewHandleEvent);
            break;
            
         case EditView:
            FrmSetEventHandler(frm, EditViewHandleEvent);
            break;
            
         case NewNoteView:
            FrmSetEventHandler(frm, NoteViewHandleEvent);
            break;
            
         case DetailsDialog:
            FrmSetEventHandler(frm, DetailsHandleEvent);
            break;
            
         case CustomEditDialog:
            FrmSetEventHandler(frm, CustomEditHandleEvent);
            break;
            
         case PreferencesDialog:
            FrmSetEventHandler(frm, PreferencesDialogHandleEvent);
            break;
            
         default:
            ErrNonFatalDisplay("Invalid Form Load Event");
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
 * DESCRIPTION: This routine is the event loop for the Address Book
 *              aplication.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         roger   6/5/95   Initial Revision
 *
 ***********************************************************************/
static void EventLoop (void)
{
   UInt16 error;
   EventType event;

   do
      {
      EvtGetEvent (&event, (TickAppButtonPushed == 0) ? evtWaitForever : 2);
      
      if (! SysHandleEvent (&event))
      
         if (! AppHandleKeyDown (&event))
         
	         if (! MenuHandleEvent (0, &event, &error))
	         
	            if (! ApplicationHandleEvent (&event))
	   
	               FrmDispatchEvent (&event); 
     
      
      #if EMULATION_LEVEL != EMULATION_NONE
//         MemHeapCheck(0);         // Check the dynamic heap after every event
//         MemHeapCheck(1);         // Check the first heap after every event
      #endif
      }
   while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the Address
 *              application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * NOTE:        We need to create a branch island to PilotMain in order to 
 *              successfully link this application for the device.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         art   7/24/95   Initial Revision
 *
 ***********************************************************************/
UInt32   PilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
   return AddrPilotMain(cmd, cmdPBP, launchFlags);
}


/***********************************************************************
 *
 * FUNCTION:    AddressMain
 *
 * DESCRIPTION: This is the main entry point for the Address Book 
 *              application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *				Name	Date		Description
 *				----	----		-----------
 *				art	6/5/95	Initial Revision
 *				jmp	10/01/99	Changed call to DmOpenDatabaseByTypeCreator() to
 *									AddrGetDatabase().
 *				jmp	10/02/99	Made the support for the sysAppLaunchCmdExgReceiveData
 *									sysAppLaunchCmdExgAskUser launch codes more like their
 *									counterparts in Datebook, Memo, and ToDo.
 *				jmp	10/13/99	Fix bug #22832:  Call AddrGetDatabase() on look-up
 *									sublaunch to create default database if it doesn't
 *									exists (at least the user can now see that nothing
 *									exists rather than just having nothing happen).
 *				jmp	10/14/99	Oops... wasn't closing the database when we opened it
 *									in the previous change!  Fixes bug #22944.
 *				jmp	10/16/99	Just create a database on hard reset if the default
 *									database doesn't exist.
 *				jmp	11/04/99	Eliminate extraneous FrmSaveAllForms() call from sysAppLaunchCmdExgAskUser
 *									since it was already being done in sysAppLaunchCmdExgReceiveData if
 *									the user affirmed sysAppLaunchCmdExgAskUser.
 *
 ***********************************************************************/
// Note: We need to create a branch island to PilotMain in order to successfully
//  link this application for the device.
static UInt32   AddrPilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
   Err error = errNone;
   DmOpenRef dbP;
          
	
   switch (cmd)
   	{
      case sysAppLaunchCmdNormalLaunch:
         error = StartApplication ();
         if (error) 
            return (error);
			
         FrmGotoForm (ListView);
         #if EMULATION_LEVEL != EMULATION_NONE
//            MemSetDebugMode(0x001A);   
         #endif
         EventLoop ();
         StopApplication ();
         break;
		
		
      case sysAppLaunchCmdFind:
         Search ((FindParamsPtr) cmdPBP);
         break;
		
		
      // This action code could be sent to the app when it's already running.
      case sysAppLaunchCmdGoTo:
         {
         Boolean launched;
         
         
         launched = launchFlags & sysAppLaunchFlagNewGlobals;

         if (launched) 
         	{
            error = StartApplication ();
            if (error) 
               return (error);
         	}

         GoToItem ((GoToParamsPtr)cmdPBP, launched);

         if (launched) 
         	{
            EventLoop ();
            StopApplication ();   
         	}      
         }
         break;
		
		
      case sysAppLaunchCmdSyncNotify:
         AppHandleSync();
         break;
		
		
      // Launch code sent to running app before sysAppLaunchCmdFind
      // or other action codes that will cause data searches or manipulation.
      case sysAppLaunchCmdSaveData:
         FrmSaveAllForms ();
         break;
		
		
      // We are requested to initialize an empty database (by sync app).
      case sysAppLaunchCmdInitDatabase:
         AppLaunchCmdDatabaseInit (((SysAppLaunchCmdInitDatabaseType*)cmdPBP)->dbP);
         break;
		
		
      // This launch code is sent after the system is reset.  We use this time
      // to create our default database.  If there is no default database image,
      // then we create an empty database.
      case sysAppLaunchCmdSystemReset:
         if (((SysAppLaunchCmdSystemResetType*)cmdPBP)->createDefaultDB)
         	{
            MemHandle resH;
            
            // Attempt to get our default data image and create our
            // database.
            resH = DmGet1Resource(sysResTDefaultDB, sysResIDDefaultDB);
            if (resH)
            	{
               error = DmCreateDatabaseFromImage(MemHandleLock(resH));
               
               if (!error)
               	{
               	MemHandleUnlock(resH);
               	DmReleaseResource(resH);
               
               	// Set the backup bit on the new database.
              		SetDBAttrBits(NULL, dmHdrAttrBackup);
              		}
            	}

				// If there is no default data, or we had a problem creating it,
				// then attempt to create an empty database.
				if (!resH || error)
					{
        			error = AddrGetDatabase (&dbP, dmModeReadWrite);

            	if (!error)
            		DmCloseDatabase(dbP);
            	}

				// Register to receive vcf files on hard reset.
				ExgRegisterData(sysFileCAddress, exgRegExtensionID, "vcf");
         	}
         break;
		
		
      // Present the user with ui to perform a lookup and return a string
      // with information from the selected record.
      case sysAppLaunchCmdLookup:
         Lookup((AddrLookupParamsPtr) cmdPBP);
	      break;


      case sysAppLaunchCmdExgAskUser:
         // if our app is not active, we need to open the database 
         // the subcall flag is used here since this call can be made without launching the app
         if (!(launchFlags & sysAppLaunchFlagSubCall))
         	{
         	error = AddrGetDatabase (&dbP, dmModeReadWrite);
         	}
         else
          	dbP = AddrDB;
         
         if (dbP != NULL)
         	{
	      	CustomAcceptBeamDialog (dbP, (ExgAskParamPtr) cmdPBP);
				
	         if (!(launchFlags & sysAppLaunchFlagSubCall))
					error = DmCloseDatabase(dbP);
				}
      	break;
      
      
      case sysAppLaunchCmdExgReceiveData:
         // if our app is not active, we need to open the database 
         // the subcall flag is used here since this call can be made without launching the app
         if (!(launchFlags & sysAppLaunchFlagSubCall))
         	{
         	error = AddrGetDatabase (&dbP, dmModeReadWrite);
         	}
         else
         	{
         	dbP = AddrDB;
         	
				// Save any data the we may be editing.
				FrmSaveAllForms ();
         	}
         
         if (dbP != NULL)
         	{
				error = AddrReceiveData(dbP, (ExgSocketPtr) cmdPBP);
				
	         if (!(launchFlags & sysAppLaunchFlagSubCall))
					error = DmCloseDatabase(dbP);
				}
         break;
      
      
      // Present the user with ui to perform a lookup and return a string
      // with information from the selected record.
/*      case sysAppLaunchCmdSendData:
      	if (((SendDataAppParamsPtr) ((SendDataParamsPtr) cmdPBP)->appDataP)->recordNum != noRecord &&
      		((SendDataParamsPtr) cmdPBP)->more == false)
      		{
	         AddrMakeRecord((SendDataParamsPtr) cmdPBP);
	         }
	      else
      		{
	         AddrMakeCategory((SendDataParamsPtr) cmdPBP);
	         }
         break;
   	
   	
*/
   	}
   
   return error;
}

/***********************************************************************
 *
 * FUNCTION:    AddressLoadPrefs
 *
 * DESCRIPTION: Load the application preferences and fix them up if
 *				there's a version mismatch.
 *
 * PARAMETERS:  appInfoPtr	-- Pointer to the app info structure
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         BGT   1/8/98     Initial revision
 *
 ***********************************************************************/

void AddressLoadPrefs(AddrAppInfoPtr			appInfoPtr)
{
	Int16 prefsVersion;
	UInt16 prefsSize;
	AddrPreferenceType prefs;

   // Read the preferences / saved-state information.  There is only one
   // version of the Address Book preferences so don't worry about multiple
   // versions.  Users appreciate the transferal of their preferences from prior
   // versions.
	prefsSize = sizeof (AddrPreferenceType);
	prefsVersion = PrefGetAppPreferences (sysFileCAddress, addrPrefID, &prefs, &prefsSize, true);
	if (prefsVersion > addrPrefVersionNum) {
		prefsVersion = noPreferenceFound;
	}
	if (prefsVersion > noPreferenceFound)
		{
		if (prefsVersion < addrPrefVersionNum) {
			prefs.noteFont = prefs.v20NoteFont;
		}
		SaveBackup = prefs.saveBackup;
		RememberLastCategory = prefs.rememberLastCategory;
		if (prefs.noteFont == largeFont)
			NoteFont = largeBoldFont;
		else
			NoteFont = prefs.noteFont;

		// If the preferences are set to use the last category and if the
		// category hasn't been deleted then use the last category.
		if (RememberLastCategory &&
			prefs.currentCategory != dmAllCategories &&
			appInfoPtr->categoryLabels[prefs.currentCategory][0] != '\0')
			{
			CurrentCategory = prefs.currentCategory;
			ShowAllCategories = prefs.showAllCategories;
			}
		
		// Support transferal of preferences from the previous version of the software.
		if (prefsVersion == addrPrefVersionNum)
			{
			// Values not set here are left at their default values
			AddrListFont = prefs.addrListFont;
			AddrRecordFont = prefs.addrRecordFont;
			AddrEditFont = prefs.addrEditFont;
			BusinessCardRecordID = prefs.businessCardRecordID;
			}
		}
	
	// The first time this app starts register to handle vCard data.
	if (prefsVersion != addrPrefVersionNum)
		ExgRegisterData(sysFileCAddress, exgRegExtensionID, "vcf");
   

	MemPtrUnlock(appInfoPtr);
}

/***********************************************************************
 *
 * FUNCTION:    AddressSavePrefs
 *
 * DESCRIPTION: Save the Address preferences with fixups so that
 *				previous versions won't go crazy.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         BGT   1/8/98     Initial Revision
 *         SCL   2/12/99    Clear reserved fields before writing saved prefs
 *
 ***********************************************************************/

void AddressSavePrefs(void)
{
	AddrPreferenceType prefs;


	// Write the preferences / saved-state information.
	prefs.currentCategory = CurrentCategory;
	ErrNonFatalDisplayIf(NoteFont > largeBoldFont, "Note font invalid.");
	prefs.noteFont = NoteFont;
	if (prefs.noteFont > largeFont) {
		prefs.v20NoteFont = stdFont;
	}
	else {
		prefs.v20NoteFont = prefs.noteFont;
	}
	prefs.addrListFont = AddrListFont;
	prefs.addrRecordFont = AddrRecordFont;
	prefs.addrEditFont = AddrEditFont;
	prefs.showAllCategories = ShowAllCategories;
	prefs.saveBackup = SaveBackup;
	prefs.rememberLastCategory = RememberLastCategory;
	prefs.businessCardRecordID = BusinessCardRecordID;

	// Clear reserved fields so prefs don't look "different" just from stack garbage!
	prefs.reserved1 = 0;
	prefs.reserved2 = 0;
	   
	// Write the state information.
	PrefSetAppPreferences (sysFileCAddress, addrPrefID, addrPrefVersionNum, &prefs, 
		sizeof (AddrPreferenceType), true);
}

/***********************************************************************
 *
 * FUNCTION:    DuplicateCurrentRecord
 *
 * DESCRIPTION: Duplicates a new record from the current record.
 *
 * PARAMETERS:  numCharsToHilite (Output):  The number of characters added to the
 *				first name field to indicate that it was a duplicated record.
 *
 * RETURNED:    The number of the new duplicated record.
 *
 * REVISION HISTORY:
 *         Name   Date      Description
 *         ----   ----      -----------
 *         css	  6/13/99   Initial Revision
 *
 ***********************************************************************/
static UInt16 DuplicateCurrentRecord (UInt16 *numCharsToHilite, Boolean deleteCurrentRecord)
{
	AddrDBRecordType recordToDup;
	UInt16 attr;
	Err err;
	UInt16 newRecordNum;
	MemHandle recordH;
	char *newFirstName = NULL;
	char duplicatedRecordIndicator [maxDuplicatedIndString + 1];
	UInt16 sizeToGet;
	UInt16 oldFirstNameLen;
	AddressFields fieldToAdd = firstName;
   
	AddrGetRecord (AddrDB, CurrentRecord, &recordToDup, &recordH);
   
	// Now we must add the "duplicated indicator" to the end of the First Name so that people
	// know that this was the duplicated record.
	GetStringResource (DuplicatedRecordIndicatorStr, duplicatedRecordIndicator);
	*numCharsToHilite = StrLen (duplicatedRecordIndicator);
	
	// Find the first non-empty field from (first name, last name, company) to add "copy" to.
	fieldToAdd = firstName;
	if (recordToDup.fields[fieldToAdd] == NULL)
		fieldToAdd = name;
	if (recordToDup.fields[fieldToAdd] == NULL)
		fieldToAdd = company;
	// revert to last name if no relevant fields exist
	if (recordToDup.fields[fieldToAdd] == NULL)
		fieldToAdd = name;
		 
	if (recordToDup.fields[fieldToAdd] == NULL)
   		{
    	recordToDup.fields[fieldToAdd] = duplicatedRecordIndicator;
   		}
	else
		{
    	// Get enough space for current string, one blank and duplicated record
    	// indicator string & end of string char.
    	oldFirstNameLen = StrLen (recordToDup.fields[fieldToAdd]);
    	sizeToGet = oldFirstNameLen + sizeOf7BitChar(spaceChr)+ StrLen (duplicatedRecordIndicator) + sizeOf7BitChar(nullChr);
    	newFirstName = MemPtrNew (sizeToGet);

    	if (newFirstName == NULL)
    		{
     		FrmAlert (DeviceFullAlert);
     		newRecordNum = noRecord;
     		goto Exit;
     		}
     
    	// make the new first name string with what was already there followed by
    	// a space and the duplicate record indicator string.
     
    	StrPrintF (newFirstName, "%s %s", recordToDup.fields[fieldToAdd], duplicatedRecordIndicator);
     
    	recordToDup.fields[fieldToAdd] = newFirstName;
    	// Must increment for the blank space that we add.
    	(*numCharsToHilite)++;
     
		// Make sure that this string is less than or equal to the maximum allowed for
		// the field.
		if (StrLen (newFirstName) > maxNameLength)
			{
			newFirstName [maxNameLength] = '\0';
			(*numCharsToHilite) = maxNameLength - oldFirstNameLen;
			}
		}
		
	EditRowIDWhichHadFocus = fieldToAdd; //this is a lucky coincidence, that the first two
				//enums are the first two rows, but:
	if (EditRowIDWhichHadFocus == company) //the third one's not
		EditRowIDWhichHadFocus++;
      
	MemHandleUnlock(recordH);

	// Make sure the attributes of the new record are the same.
	DmRecordInfo (AddrDB, CurrentRecord, &attr, NULL, NULL);
   
	// If we are to delete the current record, then lets do that now.  We have
	// all the information from the record that we need to duplicate it correctly.
	if (deleteCurrentRecord)
		{
		DeleteRecord (false);
		}

	// Now create the new record that has been duplicated from the current record.
	err = AddrNewRecord(AddrDB, &recordToDup, &newRecordNum);
	if (err)
		{
		FrmAlert(DeviceFullAlert);
		newRecordNum = noRecord;
		goto Exit;
		}
   
	// This includes the catagory so the catagories are the same between the original and 
	// duplicated record.
	attr |= dmRecAttrDirty;
	DmSetRecordInfo (AddrDB, newRecordNum, &attr, NULL);
   
   
Exit:
	if (newFirstName)
		{
		MemPtrFree (newFirstName);
		}
		
	return (newRecordNum);
}


/*****************************************************************************
* Function:			GetStringResource
*
* Description:			Reads the string associated with the resource into the passed
*						in string.
*
* Notes:				None.
*
* Parameters:			stringResource:	(Input) The string resource to be read.
*						stringP:		(Output) The string that represents the resource.
*
* Return Value(s):		The address of the string that contains a copy of the resource string.
******************************************************************************/
static char * GetStringResource (UInt16 stringResource, char * stringP)
{
	MemHandle 	nameH;
  
	nameH = DmGetResource(strRsc, stringResource);
	StrCopy (stringP, (Char *) MemHandleLock (nameH));
	MemHandleUnlock (nameH);
	DmReleaseResource (nameH);
  
	return (stringP);
}

