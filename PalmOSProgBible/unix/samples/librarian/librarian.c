/***********************************************************************
 *
 * PROJECT:  Librarian
 * FILE:     librarian.c
 * AUTHOR:   Lonnon R. Foster
 *
 * DESCRIPTION:  Main routines for Librarian.
 *
 * From Palm OS Programming Bible
 * ©2000 Lonnon R. Foster.  All rights reserved.
 *
 ***********************************************************************/

#include <PalmOS.h>
#include <Graffiti.h>
#include <SysEvtMgr.h>
#include <CharAttr.h>
#include "librarian.h"
#include "librarianTransfer.h"
#include "librarianRsc.h"


/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/

// LibPreferenceType
// Preferences for the Librarian application.  Stores the state of the
// application between invocations.

typedef struct {
    UInt16	 currentCategory;
    Boolean  showAllCategories;
    Boolean	 showBookStatus;
    Boolean  showPrintStatus;
    Boolean  showFormat;
    Boolean  showReadUnread;
    Boolean	 saveBackup;
    FontID	 listFont;
    FontID   recordFont;
    FontID	 editFont;
    FontID	 noteFont;
} LibPreferenceType;



/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/

// Librarian's database reference
static DmOpenRef  gLibDB;

// System preferences
static privateRecordViewEnum  gPrivateRecordStatus;
//static Boolean  gHideSecretRecords;

// Librarian saves these globals with its preferences
static UInt16   gCurrentCategory = dmAllCategories;
static Boolean  gShowAllCategories = true;
static Boolean  gShowBookStatus = false;
static Boolean  gShowPrintStatus = false;
static Boolean  gShowFormat = false;
static Boolean  gShowReadUnread = false;
static Boolean  gSaveBackup = true;
static FontID   gListFont;
static FontID   gRecordFont;
static FontID   gEditFont;
static FontID   gNoteFont;

// Current sort order for database
static UInt8    gShowInList;

// Database and table positions
static UInt16   gCurrentRecord = noRecord;
static UInt16   gTopVisibleRecord = 0;
static UInt16   gListFormSelectThisRecord = noRecord;

// Maximum width of second name in List view
const  Int16    gName2MaxWidth = 60;

// Current category name
static char     gCategoryName[dmCategoryLength];

// Pointers to strings for book records with no title or author
static char  *gNoAuthorRecordString = NULL;
static char  *gNoTitleRecordString = NULL;

// ROM version on which Librarian is running
static UInt32   gROMVersion;

// Form ID for the current version of Palm OS.  This is NewNoteView in
// 3.5, NoteView in earlier versions.  StartApplication() sets this
// global during a normal launch.
static UInt16   gNoteFormID;

// Variables for keeping track of various Edit view values
static UInt16   gEditLabelColumnWidth = 0;
static Boolean  gRecordNeededAfterEditView;
static UInt16   gTopVisibleFieldIndex;
static UInt16   gCurrentFieldIndex;
static UInt16   gEditRowIDWhichHadFocus;
static UInt16   gEditFieldPosition;

// Keep track of the previous form so the Note view knows where to
// return to.
static UInt16     gPriorFormID;

// Record view variables
static LibDBRecordType  gRecordFormRecord;
static MemHandle  gRecordFormRecordH = 0;
static RecordFormLineType  *gRecordFormLines;
static UInt16   gRecordFormLastLine;
static UInt16   gTopRecordFormLine;
static UInt16   gRecordFormFirstPlainLine;
static Char     *gRecordFormAuthorString = NULL;
static Char     *gRecordFormTitleString = NULL;
static Char     *gRecordFormPrintingString = NULL;
static Boolean  gRecordFormAuthorConcatenated = false;
static Boolean  gRecordFormPrintingLocked = false;

// Used by duplicate record feature to determine how many characters to
// highlight in the new record.
static UInt16  gNumCharsToHighlight = 0;


/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/

// Define the minimum OS version supported by Librarian.
#define libMinVersion	sysMakeROMVersion(2,0,0,sysROMStageRelease,0)

// Update codes, used to determine how Librarian should redraw various 
// forms.
#define updateRedrawAll                0x01
//#define updateGrabFocus				   0x02
//#define updateItemHide				   0x04
#define updateCategoryChanged		   0x08
#define updateFontChanged			   0x10
#define updateListStatusChanged        0x20
#define updatePopupListChanged         0x40
#define updateSelectCurrentRecord      0x80


/***********************************************************************
 *
 * FUNCTION:    ChangeCategory
 *
 * DESCRIPTION: Updates the globals that keep track of category info.
 *
 * PARAMETERS:  category - index of the new category
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ChangeCategory(UInt16 category)
{
    gCurrentCategory = category;
    gTopVisibleRecord = 0;
}


/***********************************************************************
 *
 * FUNCTION:    DeleteRecord
 *
 * DESCRIPTION: Deletes a Librarian record.
 *
 * PARAMETERS:  archive - If true, archive the record, otherwise delete
 *                        it.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void DeleteRecord(Boolean archive)
{
    // Show the prior record.  This provides context for the user, as it
    // shows where the record was, and it allows a return to the same
    // location in the database if the user is working through the
    // records sequentially.  If there isn't a prior record, show the
    // following record.  If there isn't a following record, don't show
    // a record at all.
    gListFormSelectThisRecord = gCurrentRecord;
    if (! SeekRecord(&gListFormSelectThisRecord, 1, dmSeekBackward))
        if (! SeekRecord(&gListFormSelectThisRecord, 1, dmSeekForward))
            gListFormSelectThisRecord = noRecord;
	
    // Delete or archive the record.
    if (archive)
        DmArchiveRecord(gLibDB, gCurrentRecord);
    else
        DmDeleteRecord(gLibDB, gCurrentRecord);
	
    // Deleted records are stored at the end of the database.
    DmMoveRecord(gLibDB, gCurrentRecord, DmNumRecords(gLibDB));
	
    // Since we just moved the gCurrentRecord to the end, the 
    // gListFormSelectThisRecord may need to be moved up one position.
    if (gListFormSelectThisRecord >= gCurrentRecord &&
        gListFormSelectThisRecord != noRecord)
        gListFormSelectThisRecord--;
	
    // Use whatever record we found to select.   
    gCurrentRecord = gListFormSelectThisRecord;
}


/***********************************************************************
 *
 * FUNCTION:    DirtyRecord
 *
 * DESCRIPTION: Marks a record dirty (modified).  Records marked dirty 
 *              will be synchronized.
 *
 * PARAMETERS:  index - index of the record to mark dirty
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void DirtyRecord(UInt16 index)
{
    UInt16  attr;
	
    DmRecordInfo(gLibDB, index, &attr, NULL, NULL);
    attr |= dmRecAttrDirty;
    DmSetRecordInfo(gLibDB, index, &attr, NULL);
}


/***********************************************************************
 *
 * FUNCTION:    DuplicateCurrentRecord
 *
 * DESCRIPTION: Creates a new record based on the contents of the
 *              current record.
 *
 * PARAMETERS:  numCharsToHighlight - number of characters added to a
 *				                      field to indicate that this record
 *                                    is a duplicate
 *              deleteCurrentRecord - true if the current record should
 *                                    be deleted
 *
 * RETURNED:    index number of the new duplicated record
 *
 * REVISION HISTORY:
 *
 ***********************************************************************/
static UInt16 DuplicateCurrentRecord(UInt16 *numCharsToHighlight,
                                     Boolean deleteCurrentRecord)
{
    LibDBRecordType recordToDup;
    UInt16     attr;
    Err        error;
    UInt16     newRecordNum;
    MemHandle  recordH, nameH;
    Char       *newName = NULL;
    Char       dupRecordIndicator[maxDupIndicatorString + 1];
    UInt16     sizeToGet;
    UInt16     oldNameLen;
    LibFields  fieldToAdd;
   
    LibGetRecord(gLibDB, gCurrentRecord, &recordToDup, &recordH);
   
    // Add some text to the end of the record's title to indicate that
    // the new record is a duplicate of an older record.
    nameH = DmGetResource(strRsc, DupIndicatorString);
    StrCopy(dupRecordIndicator, (Char *) MemHandleLock(nameH));
    MemHandleUnlock(nameH);
    DmReleaseResource(nameH);

    *numCharsToHighlight = StrLen(dupRecordIndicator);
	
    // Find the first non-empty field among title, first name, and last
    // name.  This is where DuplcateCurrentRecord will append a "(copy)"
    // indicator.
    fieldToAdd = libFieldTitle;
    if (recordToDup.fields[fieldToAdd] == NULL)
        fieldToAdd = libFieldFirstName;
    if (recordToDup.fields[fieldToAdd] == NULL)
        fieldToAdd = libFieldLastName;
    // Revert to title field if no relevant fields exist.
    if (recordToDup.fields[fieldToAdd] == NULL)
        fieldToAdd = libFieldTitle;
		 
    if (recordToDup.fields[fieldToAdd] == NULL) {
    	recordToDup.fields[fieldToAdd] = dupRecordIndicator;
    }
    else {
    	// Get enough space for current string, one space character,
    	// a duplicate record indicator string, and a terminating null.
    	oldNameLen = StrLen(recordToDup.fields[fieldToAdd]);
    	sizeToGet = oldNameLen + sizeOf7BitChar(spaceChr) +
            StrLen(dupRecordIndicator) + sizeOf7BitChar(nullChr);
    	newName = MemPtrNew(sizeToGet);

    	if (newName == NULL) {
            FrmAlert(DeviceFullAlert);
            newRecordNum = noRecord;
            goto ExitDuplicateCurrentRecord;
     	}
     
    	// Create the new field contents from what was already there,
    	// followed by a space and the duplicate record indicator string.
    	StrPrintF(newName, "%s %s", recordToDup.fields[fieldToAdd],
    	          dupRecordIndicator);
     
    	recordToDup.fields[fieldToAdd] = newName;

    	// Must increment to add a byte for the space character.
    	(*numCharsToHighlight)++;
     
        // Make sure that this string fits within the maximum allowed for
        // the field.  If not, truncate the string to fit.
        if (StrLen(newName) > maxNameLength) {
            newName[maxNameLength] = '\0';
            (*numCharsToHighlight) = maxNameLength - oldNameLen;
        }
    }
		
    // Set the previously focused field in the Edit view in preparation
    // so that when the display shifts to Edit view to show the new
    // record, the proper field will have the focus.
    gEditRowIDWhichHadFocus = fieldToAdd;
      
    MemHandleUnlock(recordH);

    // Make sure the attributes of the new record are the same.
    DmRecordInfo(gLibDB, gCurrentRecord, &attr, NULL, NULL);
   
    // If the current record should be deleted, do it now, since all the
    // information required to duplicate the record has been copied at
    // this point.
    if (deleteCurrentRecord) {
        DeleteRecord(false);
    }

    // Now create the new duplicate record.
    error = LibNewRecord(gLibDB, &recordToDup, &newRecordNum);
    if (error) {
        FrmAlert(DeviceFullAlert);
        newRecordNum = noRecord;
        goto ExitDuplicateCurrentRecord;
    }
   
    // Make sure the new record's category matches the category of the
    // original record.
    attr |= dmRecAttrDirty;
    DmSetRecordInfo(gLibDB, newRecordNum, &attr, NULL);
   
   
  ExitDuplicateCurrentRecord:
    // Clean up.
    if (newName) {
        MemPtrFree(newName);
    }
		
    return (newRecordNum);
}


/***********************************************************************
 *
 * FUNCTION:    GetAuthorString
 *
 * DESCRIPTION: Retrieves a concatenation of the author's first and last
 *              names from a given database record.  If the record does
 *              not contain a title, returns a pointer to a predefined
 *              string resource.  Note that this pointer must be unlocked
 *              later.
 *
 * PARAMETERS:  record  -> database record containing names
 *              str    <-  pointer to string to fill with the name
 *              length <-  pointer to length of the string returned
 *              width  <-  pointer to width of the string in the current
 *                         font
 *              firstLast -> if true, return the names first name first,
 *                           followed by a space and then the last name;
 *                           if false, return the names last name first,
 *                           followed by a comma, a space, then the first
 *                           name
 *              unnamed <-> pointer to string to hold the "-No author-"
 *                          string if this record has no firstName and
 *                          lastName data
 *
 * RETURNED:    true if the result is a concatenation of last and first
 *              names, in which case, the pointer created in this
 *              function must later be unlocked.  Otherwise, returns
 *              false.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean GetAuthorString(LibDBRecordType *record, char **str,
                               Int16 *length, Int16 *width, Boolean firstLast,
                               char **unnamed)
{
    MemHandle  h;
	
	
    // If neither name field is present in the record, lock a pointer to
    // the NoAuthorRecordString resource and return it.
    if (record->fields[libFieldFirstName] == NULL &&
        record->fields[libFieldLastName] == NULL) {
        if (*unnamed == NULL)
            *unnamed = MemHandleLock(DmGetResource(strRsc,
                                                   NoAuthorRecordString));
        *str = *unnamed;
        *length = StrLen(*unnamed);
        *width = FntCharsWidth(*unnamed, *length);
        return (false);
    }
	
    // If one or the other of the name fields is missing, return a
    // pointer to the one that exists.
    if (record->fields[libFieldFirstName] == NULL) {
        *str = record->fields[libFieldLastName];
        *length = StrLen(*str);
        *width = FntCharsWidth(*str, *length);
        return (false);
    }
	
    if (record->fields[libFieldLastName] == NULL) {
        *str = record->fields[libFieldFirstName];
        *length = StrLen(*str);
        *width = FntCharsWidth(*str, *length);
        return (false);
    }
	
    // If both names exist, concatenate them and return a pointer to the
    // new string.  This creates a new locked pointer which must be
    // unlocked later.
    if (firstLast) {
        *length = StrLen(record->fields[libFieldFirstName]) + 1 +
            StrLen(record->fields[libFieldLastName]) + 1;
        h = MemHandleNew(*length);
        *str = MemHandleLock(h);
        StrPrintF(*str, "%s %s", record->fields[libFieldFirstName],
                  record->fields[libFieldLastName]);
        *width = FntCharsWidth(*str, *length);
        return (true);
    }
	
    else {
        *length = StrLen(record->fields[libFieldLastName]) + 2 +
            StrLen(record->fields[libFieldFirstName]) + 1;
        h = MemHandleNew(*length);
        *str = MemHandleLock(h);
        StrPrintF(*str, "%s, %s", record->fields[libFieldLastName],
                  record->fields[libFieldFirstName]);
        *width = FntCharsWidth(*str, *length);
        return (true);
    }
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the
 *              current form.
 *
 * PARAMETERS:  objectID - ID of the object to retrieve
 *
 * RETURNED:    pointer to the object requested
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static MemPtr GetObjectPtr(UInt16 objectID)
{
    FormPtr  form;


    form = FrmGetActiveForm();
    return (FrmGetObjectPtr(form, FrmGetObjectIndex(form, objectID)));
}


/***********************************************************************
 *
 * FUNCTION:    GetFocusFieldPtr
 *
 * DESCRIPTION: Returns a pointer to the field object, in the current
 *              form, that has the focus.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    pointer to a field object or NULL of there is no field
 *              object with the focus
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static FieldPtr GetFocusFieldPtr(void)
{
    FormPtr  form;
    UInt16     focus;
	
    form = FrmGetActiveForm();
    focus = FrmGetFocus(form);
    if (focus == noFocus)
        return (NULL);
		
    // Check to see if the focus object is a table.  If so, find the
    // current field in that table and return a pointer to the field.
    if (FrmGetObjectType(form, focus) == frmTableObj)
        return (TblGetCurrentField(FrmGetObjectPtr(form, focus)));
    else if (FrmGetObjectType(form, focus) == frmFieldObj)
        return (FrmGetObjectPtr(form, focus));
    else
        return (NULL);
}


/***********************************************************************
 *
 * FUNCTION:    GetPrintingString
 *
 * DESCRIPTION: Retrieves the printing string to print in the Record view
 *              from a given database record.  If the record's printing
 *              field contains an integer value, appends an ordinal
 *              suffix to the number (1st, 2nd, 3rd, etc.).
 *
 * PARAMETERS:  record  ->  database record containing title
 *              str    <-   pointer to string to fill with the title
 *
 * RETURNED:    true if GetPrintingString locked new memory to hold the
 *              printing string, which must be unlocked later; false
 *              if no new memory was allocated for the string
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean GetPrintingString(LibDBRecordType *record, char **str)
{
    char     *printing = NULL;
    int      i = 0;
    Boolean  isInteger = true;
    MemHandle   h;
    char     *newStr;
	
	
    printing = record->fields[libFieldPrinting];
	
    // If there is no printing data, set the string to NULL and exit.
    if (! printing) {
        *str = NULL;
        return false;
    }

    // If all the characters in the printing string are digits, it's
    // an integer.
    while (printing[i] != '\0') {
        if (! TxtCharIsDigit(printing[i])) {
            isInteger = false;
            break;
        }
        i++;
    }
	
    if (isInteger) {
        // Allocate memory for the new string, plus "nn printing" and a
        // trailing null.
        h = MemHandleNew(StrLen(printing) + 12);
        newStr = MemHandleLock(h);
		
        // Determine the ordinal suffix for the number from its last digit.
        switch(StrAToI(&printing[i - 1])) {
            case 1:
                StrPrintF(newStr, "%s%s", printing, "st printing");
                break;
			
            case 2:
                StrPrintF(newStr, "%s%s", printing, "nd printing");
                break;
				
            case 3:
                StrPrintF(newStr, "%s%s", printing, "rd printing");
                break;
				
            default:
                StrPrintF(newStr, "%s%s", printing, "th printing");
                break;
        }
        *str = newStr;
        return true;
    }
	
    else {
        *str = printing;
        return false;
    }
}


/***********************************************************************
 *
 * FUNCTION:    GetTitleString
 *
 * DESCRIPTION: Retrieves the title from a given database record.
 *              If the record does not contain a title, returns a
 *              pointer to a predefined string resource.  Note that this
 *              pointer must be unlocked later.
 *
 * PARAMETERS:  record  ->  database record containing title
 *              str    <-   pointer to string to fill with the title
 *              length <-   pointer to length of the string returned
 *              width  <-   pointer to width of the string in the current
 *                          font
 *              unnamed <-> pointer to string to contain the "-Untitled-"
 *                          string if this record has no title
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void GetTitleString(LibDBRecordType *record, char **str,
                           Int16 *length, Int16 *width, char **unnamed)
{
    *str = record->fields[libFieldTitle];
    if (*str == NULL) {
        if (*unnamed == NULL)
            *unnamed = MemHandleLock(DmGetResource(strRsc,
                                                   NoTitleRecordString));
        *str = *unnamed;
        *length = StrLen(*unnamed);
        *width = FntCharsWidth(*unnamed, *length);
    }
    else {
        *length = StrLen(*str);
        *width = FntCharsWidth(*str, *length);
    }	
}


/***********************************************************************
 *
 * FUNCTION:    GoToItem
 *
 * DESCRIPTION: Opens the Record or Note view to display text found by
 *              the system global find feature.
 *
 * PARAMETERS:  goToParams - parameters from the sysAppLaunchCmdGoTo
 *                           launch code
 *              launchingApp - true if launching Librarian as a result
 *                             of the find, false if Librarian is already
 *                             running
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void GoToItem (GoToParamsPtr goToParams, Boolean launchingApp)
{
    UInt16   formID;
    UInt16   recordNum;
    UInt16   attr;
    UInt32   uniqueID;
    EventType  event;
    UInt32   romVersion;
	
	
    recordNum = goToParams->recordNum;
    DmRecordInfo(gLibDB, recordNum, &attr, &uniqueID, NULL);
	
    // Change the current category if necessary.
    if (gCurrentCategory != dmAllCategories) {
        gCurrentCategory = attr & dmRecAttrCategoryMask;
    }
	
    // If the application is already running, close all the open forms.
    // If the record currently displayed is blank, it will be deleted,
    // which knocks all the record indices off by one.  Use the found
    // record's unique ID to find the record index again once all 
    // the forms are closed.
    if (! launchingApp) {
        FrmCloseAllForms();
        DmFindRecordByID(gLibDB, uniqueID, &recordNum);
    }
	  
    // Set global variable to keep track of the current record.
    gCurrentRecord = recordNum;
	
    // Set gPriorFormID so the Note view returns to the List view.
    gPriorFormID = ListForm;
	
    if (goToParams->matchFieldNum == libFieldNote) {
        // If running on Palm OS 3.5 or above, use the NewNoteView form;
        // otherwise, stick with the original NoteView.
        FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
        if (romVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0))
            formID = NewNoteView;
        else
            formID = NoteView;
    }
    else {
        formID = RecordForm;
    }
	
    MemSet(&event, sizeof(EventType), 0);
	
    // Send an event to load the form.
    event.eType = frmLoadEvent;
    event.data.frmLoad.formID = formID;
    EvtAddEventToQueue(&event);
	
    // Send an event to go to a form and select the matching text.
    event.eType = frmGotoEvent;
    event.data.frmGoto.formID = formID;
    event.data.frmGoto.recordNum = recordNum;
    event.data.frmGoto.matchPos = goToParams->matchPos;
    event.data.frmGoto.matchLen = goToParams->searchStrLen;
    event.data.frmGoto.matchFieldNum = goToParams->matchFieldNum;
    EvtAddEventToQueue(&event);
}


/***********************************************************************
 *
 * FUNCTION:    HandleCommonMenus
 *
 * DESCRIPTION: Handles common Edit and Options menu commands.
 *
 * PARAMETERS:  menuID - menu ID of the selected menu item
 *
 * RETURNED:    true if the menu command was handled, false if not
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean HandleCommonMenus(UInt16 menuID)
{
    UInt16    formID;
    FieldPtr  field;
    Boolean   handled;
    MemHandle  recordH;
    LibDBRecordType  record;
    Boolean   hasNote;
    Boolean   hasData;
    UInt16    newRecord;
    UInt16    numCharsToHighlight;
    Boolean   wasHiding;
    UInt16    mode;


    field = GetFocusFieldPtr();
    formID = FrmGetActiveFormID();

    switch (menuID) {
        case EditUndo:
            if (! field) return false;
            FldUndo(field);
            handled = true;
            break;

        case EditCut:
            if (! field) return false;
            FldCut(field);
            handled = true;
            break;

        case EditCopy:
            if (! field) return false;
            FldCopy(field);
            handled = true;
            break;
			
        case EditPaste:
            if (! field) return false;
            FldPaste(field);
            handled = true;
            break;
			
        case EditSelectAll:
            if (! field) return false;
            FldSetSelection(field, 0, FldGetTextLength(field));
            handled = true;
            break;
			
        case EditKeyboard:
            SysKeyboardDialog(kbdDefault);
            handled = true;
            break;
			
        case EditGraffitiHelp:
            SysGraffitiReferenceDialog(referenceDefault);
            handled = true;
            break;
		
        case OptionsAboutLibrarian:
        case OptionsV20AboutLibrarian:
        case OptionsV35AboutLibrarian:
            FrmPopupForm(AboutForm);
            handled = true;
            break;
		
        case OptionsFont:
        case OptionsV35Font:
            switch (formID) {
                case ListForm:
                    gListFont = SelectFont(gListFont);
                    handled = true;
                    break;
					
                case RecordForm:
                    gRecordFont = SelectFont(gRecordFont);
                    handled = true;
                    break;
					
                case EditForm:
                    gEditFont = SelectFont(gEditFont);
                    handled = true;
                    break;
					
                case NoteView:
                case NewNoteView:
                    gNoteFont = SelectFont(gNoteFont);
                    handled = true;
                    break;
					
                default:
                    break;
            }
            break;
		

        case RecordDeleteBook:
        case RecordV20DeleteBook:
            switch (formID) {
                case RecordForm:
                    if (DetailsDeleteRecord()) {
                        FrmGotoForm(ListForm);
                        // DetailsDeleteRecord freed the handle, so it is
                        // no longer valid.
                        gRecordFormRecordH = 0;
                    }
                    handled = true;
                    break;
					
                case EditForm:
                    // Save the current field from the table..
                    TblReleaseFocus(GetObjectPtr(EditTable));
                    if (DetailsDeleteRecord())
                        FrmGotoForm(ListForm);
                    else
                        EditFormRestoreEditState();
                    handled = true;
                    break;
					
                default:
                    break;
            }
            break;

        case RecordDuplicateBook:
        case RecordV20DuplicateBook:
            switch (formID) {
                case RecordForm:
                    newRecord = DuplicateCurrentRecord(&numCharsToHighlight,
                                                       false);
					                                   
                    // If a duplicate record was successfully created,
                    // open it in the Edit view to allow the user to edit
                    // it immediately.
                    if (newRecord != noRecord) {
                        gNumCharsToHighlight = numCharsToHighlight;
                        gCurrentRecord = newRecord;
                        FrmGotoForm(EditForm);
                    }
                    handled = true;
                    break;
					
                case EditForm:
                    // Save the current field from the table.
                    FrmSetFocus(FrmGetActiveForm(), noFocus);
		
                    LibGetRecord(gLibDB, gCurrentRecord, &record,
                                 &recordH);
		
                    hasData = RecordContainsData(&record);
                    MemHandleUnlock(recordH);
					
                    newRecord = DuplicateCurrentRecord(&numCharsToHighlight,
                                                       !hasData);
                    if (newRecord != noRecord) {
                        gNumCharsToHighlight = numCharsToHighlight;
                        gCurrentRecord = newRecord;
                        FrmGotoForm(EditForm);
                    }
                    handled = true;
                    break;
					
                default:
                    break;
            }
            break;
			
        case RecordAttachNote:
        case RecordV20AttachNote:
            switch (formID) {
                case RecordForm:
                    if (CreateNote())
                        FrmGotoForm(gNoteFormID);
                    // CreateNote may or may not have freed the record.
                    // Compare the actual record's handle with
                    // gRecordFormRecordH.  If they are different, the
                    // record is new and shouldn't be freed when
                    // RecordFormHandleEvent takes care of the
                    // frmCloseEvent.
                    if (gRecordFormRecordH != DmQueryRecord(gLibDB,
                                                            gCurrentRecord))
                        gRecordFormRecordH = 0;
                    handled = true;
                    break;
					
                case EditForm:
                    // Save the current field from the table.
                    TblReleaseFocus(GetObjectPtr(EditTable));
                    if (CreateNote())
                        FrmGotoForm(gNoteFormID);
                    handled = true;
                    break;
					
                default:
                    break;
            }		
            break;
			
        case RecordDeleteNote:
        case RecordV20DeleteNote:
            switch (formID) {
                case RecordForm:
                    if (gRecordFormRecord.fields[libFieldNote] != NULL &&
                        FrmAlert(DeleteNoteAlert) == DeleteNoteYes) {
                        DeleteNote();
                        // Deleting the note unlocked its handle.
                        // Retrieve the handle again for the Record
                        // view's use.
                        LibGetRecord(gLibDB, gCurrentRecord,
                                     &gRecordFormRecord,
                                     &gRecordFormRecordH);
                        RecordFormUpdate();
                    }
                    handled = true;
                    break;
					
                case EditForm:
                    LibGetRecord(gLibDB, gCurrentRecord, &record,
                                 &recordH);
                    hasNote = (record.fields[libFieldNote] != NULL);
                    MemHandleUnlock(recordH);
                    if (hasNote && FrmAlert(DeleteNoteAlert) ==
                        DeleteNoteYes)
                        DeleteNote();
                    handled = true;
                    break;
					
                default:
                    break;
            }
            break;
		
        case RecordBeamBook:
            if (formID == EditForm) {
				// Save the contents of the Edit form before
				// proceeding withh the beam operation; otherwise,
				// a new record will still be treated as empty by
				// the beaming code.
                EditFormSaveRecord();
            }
			
            LibBeamRecord(gLibDB, gCurrentRecord);
            handled = true;
            break;
		
        case ListRecordBeamCategory:
            LibBeamCategory(gLibDB, gCurrentCategory);
            handled = true;
            break;
		
        case OptionsV35Security:
            wasHiding = (gPrivateRecordStatus == hidePrivateRecords);
			 
            gPrivateRecordStatus = SecSelectViewStatus();
			
            if (wasHiding != (gPrivateRecordStatus == hidePrivateRecords)) {
				// Close the application's data file.
                DmCloseDatabase(gLibDB);	
				
                mode = (gPrivateRecordStatus == hidePrivateRecords) ?
                    dmModeReadWrite : (dmModeReadWrite | dmModeShowSecret);
				
                gLibDB = DmOpenDatabaseByTypeCreator(libDBType,
                                                     libCreatorID, mode);

                ErrFatalDisplayIf(!gLibDB, "Can't reopen DB");
            }
			
            //For safety, simply reset the currentRecord
            TblReleaseFocus(GetObjectPtr(ListTable));
            ListFormUpdateDisplay(updateRedrawAll |
                                  updateSelectCurrentRecord);
            // updateSelectCurrentRecord will reset the current record
            // to noRecord if hidden or masked.
            break;
		
        default:
            break;
    }
	
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: Checks that a the ROM version meets Librarian's
 *              minimum requirement (version 2.0).
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code, otherwise zero if rom is compatible
 *
 * NOTE:        RomVersionCompatible can't use any globals, since it is
 *              called first thing from PilotMain.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
    UInt32  romVersion;
	
    // See if we're on minimum required version of the ROM or later.
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    if (romVersion < requiredVersion) {
        if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
            (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) {
            FrmAlert(RomIncompatibleAlert);
		
            // Pilot 1.0 will continuously relaunch this app unless we
            // switch to another safe one.
            if (romVersion < sysMakeROMVersion(2,0,0,sysROMStageRelease,0))
                AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
        }
		
        return (sysErrRomIncompatible);
    }

    return (0);
}


/***********************************************************************
 *
 * FUNCTION:    Search
 *
 * DESCRIPTION: Searches the Librarian database for records containing
 *              a particular string.  Used by the global Find feature.
 *
 * PARAMETERS:  params - find parameters passed in the
 *                       sysAppLaunchCmdFind launch code
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void Search(FindParamsPtr params)
{
    LibAppInfoType  *appInfo;
    LibDBRecordType  record;
    MemHandle  recordH;
    Boolean  done, match;
    DmOpenRef  db;
    DmSearchStateType  searchState;
    Err      error;
    Char     *header;
    MemHandle  headerStringH;
    RectangleType  r;
    LocalID  dbID;
    UInt16   cardNo = 0;
    UInt16   recordNum;
    UInt16   i;
    UInt16   pos;
    Char     *noAuthor = NULL;
    Char     *noTitle = NULL;
	
	
    // Find the Librarian database.
    error = DmGetNextDatabaseByTypeCreator(true, &searchState, libDBType,
                                           libCreatorID, true, &cardNo, &dbID);
    if (error) {
        params->more = false;
        return;
    }
	
    // Open the Librarian database.
    db = DmOpenDatabase(cardNo, dbID, params->dbAccesMode);
    if (! db) {
        params->more = false;
        return;
    }
	
    // Display the heading line.
    headerStringH = DmGetResource(strRsc, FindHeaderString);
    header = MemHandleLock(headerStringH);
    done = FindDrawHeader(params, header);
    MemHandleUnlock(headerStringH);   
    DmReleaseResource(headerStringH);   
    if (done) 
        goto Exit;
	
    // Search the description and note fields for the "find" string.
    recordNum = params->recordNum;
    while (true) {
        // Applications may take a long time to finish a find,
        // so it is a good idea to allow the user to interrupt
        // the find at any time.  This allows the user to
        // immediately go to displayed record by tapping on
        // it, even before the global find finishes filling
        // the screen, or to cancel the find entirely by
        // tapping the Stop button.  To accomplish this, check
        // to see if an event is pending, and stop the find if
        // there is an event.  This call slows down the
        // search, so it should only be performed every
        // sixteen records instead of at each and every
        // record.  If that 16th record is marked secret, and
        // the system is currently set to hide private
        // records, the check does not occur, because
        // DmQueryNextInCategory respects the database access
        // mode used earlier to open the database.
        if ((recordNum & 0x000f) == 0 &&  // every 16th record
            EvtSysEventAvail(true)) {
            // Stop the search process.
            params->more = true;
            break;
        }
	  
        recordH = DmQueryNextInCategory(db, &recordNum, dmAllCategories);
		
        // Stop searching if there are no more records.
        if (! recordH) {
            params->more = false;         
            break;
        }
	
        // Search all the fields of the Librarian record.
        LibGetRecord(db, recordNum, &record, &recordH);
        match = false;
        for (i = 0; i < libFieldsCount; i++) {
            if (record.fields[i]) {
                match = FindStrInStr(record.fields[i], params->strToFind,
                                     &pos);
                if (match) 
                    break;
            }
        }
	  
        if (match) {
            done = FindSaveMatch(params, recordNum, pos, i, 0, cardNo, dbID);
            if (done) { 
                MemHandleUnlock(recordH);
                break;
            }
		
            // Get the bounds of the region where we will draw the results.
            FindGetLineBounds(params, &r);
			
            appInfo = MemHandleLock(LibGetAppInfo(db));
			
            // Display the title of the description.
            FntSetFont(stdFont);
            DrawRecordName(&record, &r, appInfo->showInList, &noAuthor,
                           &noTitle);
			
            MemPtrUnlock(appInfo);
			
            params->lineNumber++;
        }
		
        MemHandleUnlock(recordH);
        recordNum++;
    }
	
    // Unlock handles to unnamed items.
    if (noAuthor != NULL)
        MemPtrUnlock(noAuthor);
    if (noTitle != NULL)
        MemPtrUnlock(noTitle);
	
  Exit:
    DmCloseDatabase(db);   
}


/***********************************************************************
 *
 * FUNCTION:    SeekRecord
 *
 * DESCRIPTION: Given the index of a record, this routine scans 
 *              forwards or backwards for displayable records.           
 *
 * PARAMETERS:  index - pointer to the index of a record to start from;
 *                the index of the record sought is also returned in
 *                this parameter.
 *
 *              offset - number of records to skip:   
 *                0 - seek from the current record to the next displayable
 *                    record; if the current record is a displayable
 *                    record, its index is returned
 *                1 - skip one displayable record
 *
 *              direction - either dmSeekForward or dmSeekBackward
 *
 * RETURNED:    false if no displayable record was not found, true if
 *              found.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean SeekRecord(UInt16 *index, Int16 offset, Int16 direction)
{
    DmSeekRecordInCategory(gLibDB, index, offset, direction,
                           gCurrentCategory);
    if (DmGetLastErr()) return (false);
	
    return (true);
}


/***********************************************************************
 *
 * FUNCTION:    SelectFont
 *
 * DESCRIPTION: Presents a dialog for font selection on PalmOS 3.0 and
 *              later.  Earlier versions do not have a system font
 *              selector, in which case SelectFont simply parrots back
 *              the FontID passed to it.
 *
 * PARAMETERS:  curFont - current FontID, to initialize the dialog
 *
 * RETURNED:    FontID selected by the user
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static FontID SelectFont(FontID curFont)
{
    UInt16    formID;
    FontID  newFont;
	
	
    formID = (FrmGetFormId(FrmGetActiveForm()));

    // If Librarian is running on Palm OS 3.0 or later, call the
    // system font selector to change the font.  Otherwise, return
    // without bothering to change the font.
    if (gROMVersion >= sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
        newFont = FontSelect(curFont);
    else
        return (curFont);
	
    // Update the current form if the font was changed.
    if (newFont != curFont)
        FrmUpdateForm(formID, updateFontChanged);
	
    return (newFont);
}


#pragma mark ----------------

/***********************************************************************
 *
 * FUNCTION:    AboutFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for Librarian's
 *              about box.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler; false if not handled
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean AboutFormHandleEvent(EventPtr event)
{
    Boolean  handled = false;
    FormPtr  form;


    switch (event->eType) {
        case frmOpenEvent:
            form = FrmGetActiveForm();
            AboutFormInit(form);
            FrmDrawForm(form);
            handled = true;
            break;

        case ctlSelectEvent:
            if (event->data.ctlSelect.controlID == AboutOKButton) {
                FrmReturnToForm(0);
                handled = true;
            }
			
            break;
		
        default:
            break;
    }
	
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    AboutFormInit
 *
 * DESCRIPTION: Initializes the About box.
 *
 * PARAMETERS:  form - pointer to the AboutForm form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void AboutFormInit(FormPtr form)
{
}


#pragma mark ----------------

/***********************************************************************
 *
 * FUNCTION:    DetailsDeleteRecord
 *
 * DESCRIPTION: Deletes a book record. This routine is 
 *              called when the delete button in the details dialog is
 *              pressed or when Delete Book is used from the Edit form's
 *              Record menu.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the record was deleted or archived, false
 *              otherwise.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean DetailsDeleteRecord(void)
{
    UInt16     ctlIndex;
    UInt16     buttonHit;
    FormPtr  form;
    Boolean  archive;
	  
    // Display an alert to comfirm the operation.
    form = FrmInitForm(DeleteForm);
	
    // Set the "save archive copy" check box to its previous setting.
    ctlIndex = FrmGetObjectIndex(form, DeleteArchiveCheckbox);
    FrmSetControlValue(form, ctlIndex, gSaveBackup);
	
    buttonHit = FrmDoDialog(form);
	
    archive = FrmGetControlValue(form, ctlIndex);
	
    FrmDeleteForm(form);
    if (buttonHit == DeleteCancelButton)
        return (false);

    // Remember the "save archive copy" check box setting.
    gSaveBackup = archive;
	
    DeleteRecord(archive);
	
    return (true);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "DetailsForm" of this application.
 *
 * PARAMETERS:  event - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean DetailsFormHandleEvent(EventPtr event)
{
    static UInt16     category;
    static Boolean  categoryEdited;
    
    Boolean  handled = false;
    FormPtr  form;
    UInt16     updateCode;

    switch (event->eType) {
        case frmOpenEvent:
            form = FrmGetActiveForm();
            DetailsFormInit(&category);
            FrmDrawForm(form);
            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID) {
                case DetailsOKButton:
                    updateCode = DetailsFormSave(category,
                                                 categoryEdited);
                    FrmReturnToForm(0);
                    if (updateCode)
                        FrmUpdateForm(EditForm, updateCode);
                    handled = true;
                    break;
				
                case DetailsCancelButton:
                    if (categoryEdited)
                        FrmUpdateForm(EditForm, updateCategoryChanged);
                    FrmReturnToForm(0);
                    handled = true;
                    break;
				
                case DetailsDeleteButton:
                    if (DetailsDeleteRecord()) {
                        FrmCloseAllForms();
                        FrmGotoForm(ListForm);
                    }
                    handled = true;
                    break;
					
                case DetailsNoteButton:
                    DetailsFormSave(category, categoryEdited);
                    FrmReturnToForm(EditForm);
                    if (CreateNote()) {
                        FrmGotoForm(gNoteFormID);
                        gRecordNeededAfterEditView = true;
                    }
                    handled = true;
                    break;
				
                case DetailsCategoryPopTrigger:
                    categoryEdited = DetailsSelectCategory(&category);
                    handled = true;
                    break;
				
                default:
                    break;
            }
            break;
		
        default:
            break;
    }
	
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    DetailsFormInit
 *
 * DESCRIPTION: Initializes the Details dialog.
 *
 * PARAMETERS:  category -> pointer to receive the category of the
 *                          current record
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void DetailsFormInit(UInt16 *category)
{
    LibDBRecordType  record;
    MemHandle  recordH;
    ListPtr  list;
    UInt16   curCategory;
    ControlPtr  ctl;
    UInt16   attr;
    Err      error;
    char     *label;
	
	
    // If the record is marked secret, set the Private checkbox.
    DmRecordInfo(gLibDB, gCurrentRecord, &attr, NULL, NULL);
    ctl = GetObjectPtr(DetailsPrivateCheckbox);
    CtlSetValue(ctl, attr & dmRecAttrSecret);
	
    // Set the category popup list.
    curCategory = attr & dmRecAttrCategoryMask;
    CategoryGetName(gLibDB, curCategory, gCategoryName);
    ctl = GetObjectPtr(DetailsCategoryPopTrigger);
    CategorySetTriggerLabel(ctl, gCategoryName);
	
    // Retrieve the current record to initialize the status popup triggers
    // and read check box.
    error = LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
    ErrNonFatalDisplayIf((error), "Record not found");
	
    list = GetObjectPtr(DetailsBookStatusList);
    LstSetSelection(list, record.status.bookStatus);
    ctl = GetObjectPtr(DetailsBookStatusPopTrigger);
    label = (char *) CtlGetLabel(ctl);
    StrCopy(label, LstGetSelectionText(list, LstGetSelection(list)));
    CtlSetLabel(ctl, label);
	
    list = GetObjectPtr(DetailsPrintStatusList);
    LstSetSelection(list, record.status.printStatus);
    ctl = GetObjectPtr(DetailsPrintStatusPopTrigger);
    label = (char *) CtlGetLabel(ctl);
    StrCopy(label, LstGetSelectionText(list, LstGetSelection(list)));
    CtlSetLabel(ctl, label);
	
    list = GetObjectPtr(DetailsFormatList);
    LstSetSelection(list, record.status.format);
    ctl = GetObjectPtr(DetailsFormatPopTrigger);
    label = (char *) CtlGetLabel(ctl);
    StrCopy(label, LstGetSelectionText(list, LstGetSelection(list)));
    CtlSetLabel(ctl, label);
	
    ctl = GetObjectPtr(DetailsReadCheckbox);
    CtlSetValue(ctl, record.status.read);
	
    MemHandleUnlock(recordH);
	
    // Return the currently selected category.
    *category = curCategory;
}


/***********************************************************************
 *
 * FUNCTION:    DetailsFormSave
 *
 * DESCRIPTION: Saves the changes made in the Details dialog.
 *
 * PARAMETERS:  category - new catagory
 *              categoryEdited - true if the category was edited,
 *                               false if unchanged
 *
 * RETURNED:    update code specifying what was updated so the Edit
 *              form can properly respond to a frmUpdateEvent and redraw,
 *              if necessary
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/02/26  Changed code to handle 3.5 masked records.
 *
 ***********************************************************************/
static UInt16 DetailsFormSave(UInt16 category, Boolean categoryEdited)
{
    UInt16      attr;
    UInt16      updateCode = 0;
    Boolean     secret;
    Boolean     dirty = false;
    ListPtr     list;
    ControlPtr  ctl;
    LibStatusType  status;
    LibDBRecordFlags  bits;
    LibDBRecordType  record;
    MemHandle   recordH;
    Boolean     changed = false;


    // Get the category and the secret attribute of the current record.
    DmRecordInfo(gLibDB, gCurrentRecord, &attr, NULL, NULL);   

    // Get the current setting of the private checkbox and compare it the
    // the setting of the record.  Update the record if the values 
    // are different.  If the record is being set 'secret' for the
    // first time, and the system 'hide secret records' setting is
    // off, display an informational alert to the user.  The alert is
    // part of the ROM resources; privateRecordInfoAlert is defined in
    // UICommon.h.
    secret = CtlGetValue(GetObjectPtr(DetailsPrivateCheckbox));
    if (((attr & dmRecAttrSecret) == dmRecAttrSecret) != secret) {
        if ( (gPrivateRecordStatus > showPrivateRecords) && secret)
            FrmAlert(privateRecordInfoAlert);
      
        dirty = true;
		
        if (secret)
            attr |= dmRecAttrSecret;
        else
            attr &= ~dmRecAttrSecret;
    }

    // Compare the current category to the category setting of the dialog.
    // Update the record if the categories are different.   
    if ((attr & dmRecAttrCategoryMask) != category) {
        attr &= ~dmRecAttrCategoryMask;
        attr |= category;
        dirty = true;
		
        gCurrentCategory = category;
        updateCode |= updateCategoryChanged;
    }

    // If the current category was moved, deleted, renamed, or merged
    // with another category, then the Edit view needs to be redrawn.
    if (categoryEdited) {
        gCurrentCategory = category;
        updateCode |= updateCategoryChanged;
    }
         
    // Save the new category and/or secret status, and mark the record
    // dirty.
    if (dirty) {
        attr |= dmRecAttrDirty;
        DmSetRecordInfo(gLibDB, gCurrentRecord, &attr, NULL);
    }

    // Retrieve the values of the status popup triggers and the read check
    // box from the form.
    list = GetObjectPtr(DetailsBookStatusList);
    status.bookStatus = LstGetSelection(list);
	
    list = GetObjectPtr(DetailsPrintStatusList);
    status.printStatus = LstGetSelection(list);
	
    list = GetObjectPtr(DetailsFormatList);
    status.format = LstGetSelection(list);
	
    ctl = GetObjectPtr(DetailsReadCheckbox);
    status.read = CtlGetValue(ctl);
	
    // Look at the current record to get its status values.
    LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
	
    // Only bother saving changes if there were, in fact, changes.
    changed = ( (status.bookStatus != record.status.bookStatus) ||
                (status.printStatus != record.status.printStatus) ||
                (status.format != record.status.format) ||
                (status.read != record.status.read) );
    if (changed) {
        record.status.bookStatus = status.bookStatus;
        record.status.printStatus = status.printStatus;
        record.status.format = status.format;
        record.status.read = status.read;
        DirtyRecord(gCurrentRecord);
        bits.allBits = 0;
        // LibChangeRecord unlocks the handle locked in LibGetRecord.
        LibChangeRecord(gLibDB, &gCurrentRecord, &record, bits);
    }
    else
        // Unlock the handle locked by LibGetRecord.
        MemHandleUnlock(recordH);
	
    return (updateCode);
}


/***********************************************************************
 *
 * FUNCTION:    DetailsSelectCategory
 *
 * DESCRIPTION: Handles selection, creation and deletion of
 *              categories in the Details dialog.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    true if the category was changed, false if not.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean DetailsSelectCategory(UInt16 *category)
{
    Boolean  categoryEdited;
	
    categoryEdited = CategorySelect(gLibDB, FrmGetActiveForm(),
                                    DetailsCategoryPopTrigger, DetailsCategoryList,
                                    false, category, gCategoryName, 1, 0);
	
    return(categoryEdited);
}


#pragma mark ----------------

/***********************************************************************
 *
 * FUNCTION:    EditFormDoCommand
 *
 * DESCRIPTION: Performs the specified menu command.
 *
 * PARAMETERS:  command - menu item ID
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/02/20  Menu code moved to HandleCommonMenus to support
 *             pilrc-generated shared menu resources.
 *
 ***********************************************************************/
static Boolean EditFormDoCommand(UInt16 command)
{
    Boolean    handled = false;


    if (HandleCommonMenus(command))
        handled = true;
	
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    EditFormGetFieldHeight
 *
 * DESCRIPTION: Initializes a row in the Edit form's table.
 *
 * PARAMETERS:  table       - pointer to the table
 *              fieldIndex  - the index of the field displayed in the row
 *              columnWidth - width of the data column in pixels
 *              maxHeight   - maximum height of a text field in the table
 *              record      - pointer to the record displayed by the Edit
 *                            form
 *              fontID      - pointer to the ID of the current font used
 *                            to display Edit table field text
 *
 * RETURNED:    height of the field in pixels
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt16 EditFormGetFieldHeight (TablePtr table, UInt16 fieldIndex,
                                      UInt16 columnWidth, UInt16 maxHeight, LibDBRecordType *record,
                                      FontID *fontID)
{
    Int16    row, column;
    UInt16    height, lineHeight;
    FontID    curFont;
    char      *str;
    FieldPtr  field;


    if (TblEditing(table)) {
        TblGetSelection(table, &row, &column);
        if (fieldIndex == TblGetRowID(table, row)) {
            field = TblGetCurrentField(table);
            str = FldGetTextPtr(field);
        }
        else {
            str = record->fields[fieldIndex];
        }
    }
    else {
        str = record->fields[fieldIndex];
    }

    // If the field contains text, or the field is the current field,
    // or the font used to display blank lines is the same as the font
    // used to display text, then use the view's current font setting. 
    if ( (str && *str) || 
         (gCurrentFieldIndex == fieldIndex) ||
         (gEditFont == libEditBlankFont)) {
        *fontID = gEditFont;
        curFont = FntSetFont(*fontID);
    }

    // If the height of the font used to display blank lines is the same 
    // height as the font used to display text then use the view's 
    // current font setting.
    else {
        curFont = FntSetFont(libEditBlankFont);
        lineHeight = FntLineHeight();
		
        FntSetFont(gEditFont);
        if (lineHeight == FntLineHeight())
            *fontID = gEditFont;
        else {
            *fontID = libEditBlankFont;
            FntSetFont(libEditBlankFont);
        }
    }
		
    height = FldCalcFieldHeight(str, columnWidth);
    lineHeight = FntLineHeight();
    height = min(height, (maxHeight / lineHeight));
    height *= lineHeight;

    FntSetFont(curFont);
		
    return (height);
}


/***********************************************************************
 *
 * FUNCTION:    EditFormGetLabelWidth
 *
 * DESCRIPTION: Determine the width of the label column from the widest
 *              label, plus a colon (':') character.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    Width of the label column in pixels.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt16 EditFormGetLabelWidth()
{
    int		 i;
    UInt16	 labelWidth;
    UInt16	 columnWidth = 0;
    FontID	 curFont;
    char     *label;
    LibAppInfoType  *appInfo;

	
    // Set the font used by the label column, saving the current font
    // for later restoration.
    curFont = FntSetFont(stdFont);

    // Retrieve the field labels from the application info block.
    appInfo = MemHandleLock(LibGetAppInfo(gLibDB));
	
    for (i = 0; i < libNumFieldLabels; i++) {
        label = appInfo->fieldLabels[i];
        labelWidth = FntCharsWidth(label, StrLen(label));
        columnWidth = max(columnWidth, labelWidth);
    }
    columnWidth += 1 + FntCharWidth(':');

    MemPtrUnlock(appInfo);
	
    // Restore the current font.
    FntSetFont(curFont);
	
    if (columnWidth > maxLabelColumnWidth)
        columnWidth = maxLabelColumnWidth;

    return columnWidth;
}


/***********************************************************************
 *
 * FUNCTION:    EditFormGetRecordField
 *
 * DESCRIPTION: Returns a pointer to a field of a Librarian record. 
 *              This routine is called by the table object as a callback
 *              when the table wants to display or edit a field.
 *
 * PARAMETERS:  table    -> pointer to the Edit form table
 *              row      -> row of the table to draw
 *              column   -> column of the table to draw
 *              editing  -> true if the user is currently editing the
 *                          field for this row, false otherwise
 *              textH   <-  pointer to receive a handle to the memory
 *                          chunk containing the string for the field's
 *                          contents
 *              textOffset <- offset from the start of the chunk to the
 *                            start of the string
 *              textAllocSize <- amount of space allocated for the string
 *              field    -> pointer to the field that will hold the
 *                          text string
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err EditFormGetRecordField(MemPtr table, Int16 row, Int16 column, 
                                  Boolean editing, MemHandle *textH, Int16 *textOffset,
                                  Int16 *textAllocSize, FieldPtr fldText)
{
    UInt16     fieldNum;
    char       *recordP, *field;
    MemHandle  recordH, fieldH;
    UInt16     fieldSize;
    LibDBRecordType  record;

   
#ifdef __GNUC__
    // CALLBACK_PROLOGUE;
#endif

    // Get the field number that corresponds to the table item.
    // The field number is stored as the row ID.
    //
    fieldNum = TblGetRowID(table, row);
	
    LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
	
    if (editing) {
        EditSetGraffitiMode(fldText);
        if (record.fields[fieldNum]) {
            fieldSize = StrLen(record.fields[fieldNum]) + 1;
            fieldH = MemHandleNew(fieldSize);
            field = MemHandleLock(fieldH);
            MemMove(field, record.fields[fieldNum], fieldSize);
            *textAllocSize = fieldSize;
            MemHandleUnlock(fieldH);
      	}
        else {
            fieldH = 0;
            *textAllocSize = 0;
        }
        MemHandleUnlock(recordH);
        *textOffset = 0;
        *textH = fieldH;

#ifdef __GNUC__
        // CALLBACK_EPILOGUE;
#endif
		
        return (0);
    }
    else {
        // Calculate the offset from the start of the record.
        recordP = MemHandleLock(recordH);   // record now locked twice
		
        if (record.fields[fieldNum]) {
            *textOffset = record.fields[fieldNum] - recordP;
            *textAllocSize = StrLen (record.fields[fieldNum]) + 1;  // one for null terminator
        }
        else {
            do {
                fieldNum++;
            } while (fieldNum < libFieldsCount &&
                     record.fields[fieldNum] == NULL);
      
            if (fieldNum < libFieldsCount)
                *textOffset = record.fields[fieldNum] - recordP;
            else
                // Place the new field at the end of the text.
                *textOffset = MemHandleSize(recordH);   

            *textAllocSize = 0;
        }
        MemHandleUnlock(recordH);   // unlock the second lock
    }

    MemHandleUnlock(recordH);      // unlock the LibGetRecord lock

    *textH = recordH;

#ifdef __GNUC__
    // CALLBACK_EPILOGUE;
#endif

    return (0);
}


/***********************************************************************
 *
 * FUNCTION:    EditFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "EditForm" of this application.
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
static Boolean EditFormHandleEvent(EventType * event)
{
    Boolean   handled = false;
    FormPtr   form;
    UInt16    tableIndex;
    TablePtr  table;
    FieldPtr  field;
    Int16     row;
    UInt16    startPos, endPos;


    switch (event->eType) {
        case menuEvent:
            handled = EditFormDoCommand(event->data.menu.itemID);
            break;

        case menuCmdBarOpenEvent:
            field = GetFocusFieldPtr();
            FldGetSelection(field, &startPos, &endPos);
			
            if (startPos != endPos) {
                MenuCmdBarAddButton(menuCmdBarOnLeft, BarPasteBitmap,
                                    menuCmdBarResultMenuItem, EditPaste, NULL);
                MenuCmdBarAddButton(menuCmdBarOnLeft, BarCopyBitmap,
                                    menuCmdBarResultMenuItem, EditCopy, NULL);
                MenuCmdBarAddButton(menuCmdBarOnLeft, BarCutBitmap,
                                    menuCmdBarResultMenuItem, EditCut, NULL);
            } else {
                MenuCmdBarAddButton(menuCmdBarOnLeft, BarDeleteBitmap,
                                    menuCmdBarResultMenuItem, RecordDeleteBook, NULL);
                MenuCmdBarAddButton(menuCmdBarOnLeft, BarPasteBitmap,
                                    menuCmdBarResultMenuItem, EditPaste, NULL);
                MenuCmdBarAddButton(menuCmdBarOnLeft, BarUndoBitmap,
                                    menuCmdBarResultMenuItem, EditUndo, NULL);
                MenuCmdBarAddButton(menuCmdBarOnLeft, BarBeamBitmap,
                                    menuCmdBarResultMenuItem, RecordBeamBook, NULL);
            }
			
            // Prevent the field manager from adding its own buttons;
            // this command bar already contains all the buttons it
            // needs.
            event->data.menuCmdBarOpen.preventFieldButtons = true;
			
            // Leave event unhandled so the system can catch it.
            break;
		
        case frmOpenEvent:
            form = FrmGetActiveForm();
            EditFormInit(form);
			
            tableIndex = FrmGetObjectIndex(form, EditTable);
            table = FrmGetObjectPtr(form, tableIndex);
			
            // Make sure the field that will get the focus is visible.
            while (!TblFindRowID(table, gEditRowIDWhichHadFocus, &row)) {
                gTopVisibleFieldIndex = gEditRowIDWhichHadFocus;
                gCurrentFieldIndex = gEditRowIDWhichHadFocus;
                EditFormLoadTable();
            }

            FrmDrawForm(form);
			
            // Set the focus.
            FrmSetFocus(form, tableIndex);
            TblGrabFocus(table, row, dataColumn);
            field = TblGetCurrentField(table);
            FldGrabFocus(field);
            
            // If gNumCharsToHighlight is not 0, then this is the first
            // time a newly duplicated record is appearing to the user.
            // Highlight the duplicate record indicator string.
            if (gNumCharsToHighlight > 0) {
                gEditFieldPosition = FldGetTextLength(field);
                FldSetSelection(field, gEditFieldPosition -
                                gNumCharsToHighlight, gEditFieldPosition);
                gNumCharsToHighlight = 0;
            }
            
            FldSetInsPtPosition(field, gEditFieldPosition);
			
            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID) {
                case EditDoneButton:
                    FrmGotoForm(ListForm);
                    handled = true;
                    break;
					
                case EditDetailsButton:
                    FrmPopupForm(DetailsForm);
                    handled = true;
                    break;

                case EditNoteButton:
                    if (CreateNote()) {
                        gRecordNeededAfterEditView = true;
                        FrmGotoForm(gNoteFormID);
                    }
                    handled = true;
                    break;

                case EditCategoryPopTrigger:
                    EditFormSelectCategory();
                    handled = true;
                    break;
				
                default:
                    break;
            }
			
            break;
		
        case ctlRepeatEvent:
            switch (event->data.ctlRepeat.controlID) {
                case EditScrollUpRepeating:
                    EditFormScroll(winUp);
                    // Leave unhandled so the buttons can repeat.
                    break;
					
                case EditScrollDownRepeating:
                    EditFormScroll(winDown);
                    // Leave unhandled so the buttons can repeat.
                    break;
				
                default:
                    break;
            }
            break;
		
        case tblSelectEvent:
            if (gCurrentFieldIndex !=
                TblGetRowID(event->data.tblSelect.pTable,
                            event->data.tblSelect.row))
                EditFormHandleSelectField(event->data.tblSelect.row,
                                          event->data.tblSelect.column);
            // Leave unhandled to give the system a chance to
            // process table events.
            break;
		
        case keyDownEvent:
            if (TxtCharIsHardKey(event->data.keyDown.modifiers,
                                 event->data.keyDown.chr)) {
                TblReleaseFocus(GetObjectPtr(EditTable));
                gTopVisibleRecord = 0;
                gCurrentFieldIndex = noFieldIndex;
                FrmGotoForm(ListForm);
                return true;
            }

            switch (event->data.keyDown.chr) {
                case nextFieldChr:
                    EditFormNextField(winDown);
                    handled = true;
                    break;
					
                case prevFieldChr:
                    EditFormNextField(winUp);
                    handled = true;
                    break;
					
                case pageUpChr:
                    EditFormScroll(winUp);
                    handled = true;
                    break;
				
                case pageDownChr:
                    EditFormScroll(winDown);
                    handled = true;
                    break;
					
                default:
                    break;
            }
            break;	
		
        case fldHeightChangedEvent:
            EditFormResizeData(event);
            handled = true;
            break;
      
        case frmUpdateEvent:
            handled = EditFormUpdateDisplay(event->data.frmUpdate.updateCode);
            break;
		
        case frmCloseEvent:
            // Check the record to see if it contains any data, and if it
            // does, save the record.
            EditFormSaveRecord();
			
            // Keep track of the last form so the Note view knows where
            // to return to.
            gPriorFormID = EditForm;
            break;
		
        default:
            break;	
    }
	
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    EditFormHandleSelectField
 *
 * DESCRIPTION: Handles the user tapping an Edit view field label.
 *
 * PARAMETERS:  row - table row tapped
 *              column - table column tapped
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormHandleSelectField (Int16 row, const UInt8 column)
{
    Int16     curRow;
    UInt16    fieldIndex;
    MemHandle   recordH;
    FontID    curFont;
    Boolean   redraw = false;
    TablePtr  table;
    FieldPtr  field;
    LibDBRecordType  record;
	
	
    table = GetObjectPtr(EditTable);
    // If the user selects a label, set the table to edit the field to
    // the right of the label.
    if (column == labelColumn) {
        TblReleaseFocus(table);
        TblUnhighlightSelection(table);
    }
		
    // Make sure the the heights the the field we are exiting and the 
    // that we are entering are correct.  They may be incorrect if the 
    // font used to display a blank line is a different height than the
    // font used to display field text.
    fieldIndex = TblGetRowID(table, row);

    if (fieldIndex != gCurrentFieldIndex || TblGetCurrentField(table) ==
        NULL) {
        LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);

        curFont = FntGetFont();

        // If the current field is empty, set its font and row height to
        // the blank row height.
        if (! record.fields[gCurrentFieldIndex]) {
            if (TblFindRowID(table, gCurrentFieldIndex, &curRow)) {
                FntSetFont(libEditBlankFont);
                if (FntLineHeight() != TblGetRowHeight(table, curRow)) {
                    TblMarkRowInvalid(table, curRow);
                    redraw = true;
                }
            }
        }

        gCurrentFieldIndex = fieldIndex;

        // If the newly selected field is empty, set its font and row
        // height to the current font height for the Edit view.
        if (! record.fields[fieldIndex]) {
            FntSetFont(gEditFont);
            if (FntLineHeight() != TblGetRowHeight(table, row)) {
                TblMarkRowInvalid(table, row);
                redraw = true;
            }
        }
		
        // Unlock the handle to the current record before the table
        // focus is released and the record is saved.
        MemHandleUnlock(recordH);
		
        if (redraw) {
            TblReleaseFocus(table);
            EditFormLoadTable();
            TblFindRowID(table, fieldIndex, &row);
            TblRedrawTable (table);
        }

        FntSetFont(curFont);
    }

    // Set the focus
    if (TblGetCurrentField(table) == NULL) {
        TblGrabFocus(table, row, dataColumn);
        field = TblGetCurrentField(table);
        FldGrabFocus(field);
        FldMakeFullyVisible(field);
    }
}


/***********************************************************************
 *
 * FUNCTION:    EditFormInit
 *
 * DESCRIPTION: This routine initializes the EditForm form.
 *
 * PARAMETERS:  form - pointer to the EditForm form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormInit(FormPtr form)
{
    UInt16      attr;
    UInt16      row;
    UInt16      rowsInTable;
    UInt16      category;
    UInt16      dataColumnWidth;
    TablePtr  table;
    FontID    curFont;
    RectangleType  bounds;


    curFont = FntSetFont(stdFont);

    // Retrieve the label width for the edit table.
    gEditLabelColumnWidth = EditFormGetLabelWidth();
	
    // Initialize the edit table.
    table = FrmGetObjectPtr(form, FrmGetObjectIndex(form, EditTable));
    rowsInTable = TblGetNumberOfRows(table);
    for (row = 0; row < rowsInTable; row++) {
        TblSetItemStyle(table, row, labelColumn, labelTableItem);
        TblSetItemStyle(table, row, dataColumn, textTableItem);
        TblSetRowUsable(table, row, false);
    }

    TblSetColumnUsable(table, labelColumn, true);
    TblSetColumnUsable(table, dataColumn, true);

    TblSetColumnSpacing(table, labelColumn, spaceBeforeData);

    // Set the callback routines that will load and save the data column.
    TblSetLoadDataProcedure(table, dataColumn, EditFormGetRecordField);
    TblSetSaveDataProcedure(table, dataColumn, EditFormSaveRecordField);

    // Compute the width of the data column; account for the space
    // between the two columns.
    TblGetBounds(table, &bounds);
    dataColumnWidth = bounds.extent.x - spaceBeforeData -
        gEditLabelColumnWidth;

    TblSetColumnWidth(table, labelColumn, gEditLabelColumnWidth);
    TblSetColumnWidth(table, dataColumn, dataColumnWidth);
	  
    EditFormLoadTable();

    // Set the label of the category trigger.
    if (gCurrentCategory == dmAllCategories) {
        DmRecordInfo(gLibDB, gCurrentRecord, &attr, NULL, NULL);   
        category = attr & dmRecAttrCategoryMask;
    }
    else 
        category = gCurrentCategory;
	
    CategoryGetName(gLibDB, category, gCategoryName);
    CategorySetTriggerLabel(GetObjectPtr(EditCategoryPopTrigger),
                            gCategoryName);
	                        
    FntSetFont(curFont);
	
    // In general, the record isn't needed after the Edit form is closed.
    // It is if the user is going to the Note view.  In that case,
    // keep the record.
    gRecordNeededAfterEditView = false;
	
    // If Librarian is running on Palm OS 2.0, change the menus to the V20
    // versions.
    if (gROMVersion < sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
        FrmSetMenu(form, EditV20MenuBar);
}


/***********************************************************************
 *
 * FUNCTION:    EditFormLoadTable
 *
 * DESCRIPTION: Loads a Librarian database record into the Edit view.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormLoadTable(void)
{
    UInt16    row, numRows;
    UInt16    lineHeight;
    UInt16    fieldIndex, lastFieldIndex;
    UInt16    dataHeight;
    UInt16    tableHeight;
    UInt16    columnWidth;
    UInt16    pos, oldPos;
    UInt16    height, oldHeight;
    FontID    fontID;
    FontID    curFont;
    FormPtr   form;
    TablePtr  table;
    Boolean   rowUsable;
    Boolean   rowsInserted = false;
    Boolean   lastItemClipped;
    RectangleType  r;
    LibDBRecordType  record;
    MemHandle  recordH;
    LibAppInfoType  *appInfo;
    Boolean   fontChanged;

	
    appInfo = MemHandleLock(LibGetAppInfo(gLibDB));
		
    form = FrmGetActiveForm();
	
    // Get the current record
    LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);

    // Get the height of the table and the width of the data column.
    table = GetObjectPtr(EditTable);
    TblGetBounds(table, &r);
    tableHeight = r.extent.y;
    columnWidth = TblGetColumnWidth(table, dataColumn);

    // If a field is currently selected, make sure that it is not
    // above the first visible field.
    if (gCurrentFieldIndex != noFieldIndex) {
        if (gCurrentFieldIndex < gTopVisibleFieldIndex)
            gTopVisibleFieldIndex = gCurrentFieldIndex;
    }

    row = 0;
    dataHeight = 0;
    oldPos = pos = 0;
    fieldIndex = gTopVisibleFieldIndex;
    lastFieldIndex = fieldIndex;

    // Load fields into the table.
    while (fieldIndex <= editLastFieldIndex) {		
        // Compute the height of the field's text string.
        height = EditFormGetFieldHeight(table, fieldIndex, columnWidth,
                                        tableHeight, &record, &fontID);

        // Is there enough room for at least one line of the data?
        curFont = FntSetFont(fontID);
        lineHeight = FntLineHeight();
        FntSetFont (curFont);
        if (tableHeight >= dataHeight + lineHeight) {
            rowUsable = TblRowUsable(table, row);

            // Get the height of the current row.
            if (rowUsable)
                oldHeight = TblGetRowHeight(table, row);
            else
                oldHeight = 0;

            // If the field is not already displayed in the current 
            // row, load the field into the table.  
            if (gROMVersion >=
                sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
                fontChanged = (TblGetItemFont(table, row, dataColumn) !=
                               fontID);
            else
                fontChanged = false;
			
            if ((! rowUsable) ||
                (TblGetRowID(table, row) != fieldIndex) ||
                fontChanged) {
                EditInitTableRow(table, row, fieldIndex, height, fontID,
                                 &record, appInfo);
            }
			
            // If the height or the position of the item has changed draw
            // the item.
            else if (height != oldHeight) {
                TblSetRowHeight(table, row, height);
                TblMarkRowInvalid(table, row);
            }
            else if (pos != oldPos) {
                TblMarkRowInvalid(table, row);
            }

            pos += height;
            oldPos += oldHeight;
            lastFieldIndex = fieldIndex;
            fieldIndex++;
            row++;
        }

        dataHeight += height;

        // Is the table full?
        if (dataHeight >= tableHeight) 	{
            // If a field is currently selected, make sure that it is
            // not below the last visible field.  If the currently
            // selected field is the last visible record, make sure the
            // whole field is visible.
            if (gCurrentFieldIndex == noFieldIndex)
                break;

            // Above last visible?
            else if (gCurrentFieldIndex < fieldIndex)
                break;

            // Last visible?
            else if (fieldIndex == lastFieldIndex) {
                if ((fieldIndex == gTopVisibleFieldIndex) ||
                    (dataHeight == tableHeight))
                    break;
            }	

            // Remove the top item from the table and reload the table
            // again.
            gTopVisibleFieldIndex++;
            fieldIndex = gTopVisibleFieldIndex;

            row = 0;
            dataHeight = 0;
            oldPos = pos = 0;
        }
    }

    // Hide the items that don't have any data.
    numRows = TblGetNumberOfRows(table);
    while (row < numRows) {		
        TblSetRowUsable(table, row, false);
        row++;
    }
		
    // If the table is not full and the first visible field is 
    // not the first field in the record, display enough fields
    // to fill out the table by adding fields to the top of the table.
    while (dataHeight < tableHeight) {
        fieldIndex = gTopVisibleFieldIndex;
        if (fieldIndex == 0) break;
        fieldIndex--;

        // Compute the height of the field.
        height = EditFormGetFieldHeight(table, fieldIndex, 
                                        columnWidth, tableHeight, &record, &fontID);

        // If adding the item to the table will overflow the height of
        // the table, don't add the item.
        if (dataHeight + height > tableHeight)
            break;
		
        // Insert a row before the first row.
        TblInsertRow(table, 0);

        EditInitTableRow(table, 0, fieldIndex, height, fontID, &record,
                         appInfo);
		
        gTopVisibleFieldIndex = fieldIndex;
        rowsInserted = true;
        dataHeight += height;
    }
		
    // If rows were inserted to fill out the page, invalidate the whole
    // table, it all needs to be redrawn.
    if (rowsInserted)
        TblMarkTableInvalid(table);

    // If the height of the data in the table is greater than the height
    // of the table, then the bottom of the last row is clipped and the 
    // table is scrollable.
    lastItemClipped = (dataHeight > tableHeight);

    // Update the scroll arrows.
    EditFormUpdateScrollers(form, lastFieldIndex, lastItemClipped);

    MemHandleUnlock(recordH);
    MemPtrUnlock(appInfo);
}


/***********************************************************************
 *
 * FUNCTION:    EditFormNewRecord
 *
 * DESCRIPTION: Makes a new record, initializing a few values as
 *              necessary.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormNewRecord()
{
    LibDBRecordType  record;
    Int16   i;
    UInt16  attr;
    Err   error;
	
	
    // Set up the new record.
    record.status.bookStatus = bookStatusHave;
    record.status.printStatus = printStatusInPrint;
    record.status.format = formatHardcover;
    record.status.read = 0;
   
    for (i = 0; i < libFieldsCount; i++) {
        record.fields[i] = NULL;
    }
   
    error = LibNewRecord(gLibDB, &record, &gCurrentRecord);
    if (error) {
        // DeviceFullAlert is defined in UICommon.h.  The alert resource
        // itself is part of the ROM resources.
        FrmAlert(DeviceFullAlert);
        return;
    }
      
    // Set the record's category to the category being viewed.
    // If the category is All then set the category to Unfiled.
    DmRecordInfo(gLibDB, gCurrentRecord, &attr, NULL, NULL);   
    attr &= ~dmRecAttrCategoryMask;
    attr |= ((gCurrentCategory == dmAllCategories) ? dmUnfiledCategory : 
             gCurrentCategory) | dmRecAttrDirty;
    DmSetRecordInfo(gLibDB, gCurrentRecord, &attr, NULL);

    // Set the global variable that determines which field is the top
    // visible field in the Edit view.
    gTopVisibleFieldIndex = 0;
    gCurrentFieldIndex = editFirstFieldIndex;
    gEditRowIDWhichHadFocus = editFirstFieldIndex;
    gEditFieldPosition = 0;

    FrmGotoForm(EditForm);
}


/***********************************************************************
 *
 * FUNCTION:    EditFormNextField
 *
 * DESCRIPTION: If the user is currently editing a field in the Edit
 *              table, change the focus to the next or previous field in
 *              the table.
 *
 * PARAMETERS:  direction - direction to change the focus to, either up
 *                          or down
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormNextField (WinDirectionType direction)
{
    TablePtr  table;
    Int16      row;
    Int16      column;
    UInt16      nextFieldNumIndex;
	
	
    table = GetObjectPtr(EditTable);
	
    if (! TblEditing(table))
        return;
   
    // Find out which field is being edited.  If at the end of the table
    // and trying to advance to the next field, wrap to the first field.
    // Likewise, when at the beginning of the table and trying to go to
    // the previous field, wrap to the last field in the table.
    TblGetSelection(table, &row, &column);
    nextFieldNumIndex = TblGetRowID(table, row);
    if (direction == winDown) {
        if (nextFieldNumIndex >= editLastFieldIndex)
            nextFieldNumIndex = 0;
        else
            nextFieldNumIndex++;
    }
    else {
        if (nextFieldNumIndex == 0)
            nextFieldNumIndex = editLastFieldIndex;
        else
            nextFieldNumIndex--;
    }
    TblReleaseFocus (table);

    gCurrentFieldIndex = nextFieldNumIndex;

    // If the new field isn't visible move the Edit table and then
    // find the row where the next field is.
    while (! TblFindRowID(table, nextFieldNumIndex, &row)) {
        // Scroll the view down placing the item on the top row.
        gTopVisibleFieldIndex = nextFieldNumIndex;
        EditFormLoadTable();
        TblRedrawTable(table);
    }

    EditFormHandleSelectField(row, dataColumn);
}


/***********************************************************************
 *
 * FUNCTION:    EditFormResizeData
 *
 * DESCRIPTION: EditFormHandleEvent calls this routine when the height
 *              of a data field is changed as a result of user input.
 *              If the new height of the field is shorter, more items
 *              may need to be added to the bottom of the list.
 *              
 * PARAMETERS:  event - pointer to the fldChangedEvent
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormResizeData(EventPtr event)
{
    UInt16      pos;
    Int16      row, column;
    UInt16      lastRow;
    UInt16      fieldIndex, lastFieldIndex, topFieldIndex;
    FieldPtr  field;
    TablePtr  table;
    Boolean   restoreFocus = false;
    Boolean   lastItemClipped;
    RectangleType  itemR;
    RectangleType  tableR;
    RectangleType  fieldR;


    // Get the current height of the field;
    field = event->data.fldHeightChanged.pField;
    FldGetBounds(field, &fieldR);

    // Have the table object resize the field and move the items below
    // the field up or down.
    table = GetObjectPtr(EditTable);
    TblHandleEvent(table, event);

    // If the field's height has expanded, we're done.
    if (event->data.fldHeightChanged.newHeight >= fieldR.extent.y) {
        topFieldIndex = TblGetRowID(table, 0);
        if (topFieldIndex != gTopVisibleFieldIndex)
            gTopVisibleFieldIndex = topFieldIndex;
        else {
            // Since the table has expanded we may be able to scroll
            // when before we might not have.
            lastRow = TblGetLastUsableRow(table);
            TblGetBounds(table, &tableR);
            TblGetItemBounds(table, lastRow, dataColumn, &itemR);
            lastItemClipped = (itemR.topLeft.y + itemR.extent.y > 
                               tableR.topLeft.y + tableR.extent.y);
            lastFieldIndex = TblGetRowID(table, lastRow);
			
            EditFormUpdateScrollers(FrmGetActiveForm(), lastFieldIndex,
                                    lastItemClipped);
			
            return;
        }
    }

    // If the field's height has contracted and the field edit field
    // is not visible then the table may be scrolled.  Release the 
    // focus,  which will force the saving of the field we are editing.
    else if (TblGetRowID (table, 0) != editFirstFieldIndex) {
        TblGetSelection(table, &row, &column);
        fieldIndex = TblGetRowID(table, row);
		
        field = TblGetCurrentField(table);
        pos = FldGetInsPtPosition(field);
        TblReleaseFocus(table);

        restoreFocus = true;
    }

    // Add items to the table to fill in the space made available by 
    // shortening the field.
    EditFormLoadTable();
    TblRedrawTable(table);

    // Restore the insertion point position.
    if (restoreFocus) {
        TblFindRowID(table, fieldIndex, &row);
        TblGrabFocus(table, row, column);
        FldSetInsPtPosition(field, pos);
        FldGrabFocus(field);
    }
}


/***********************************************************************
 *
 * FUNCTION:    EditFormRestoreEditState
 *
 * DESCRIPTION: Restores the edit state of Edit table.  Librarian calls
 *              this function when it needs to redraw the Edit view, but
 *              the current selection state of the field the user is
 *              currently editing should be restored after the redraw.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormRestoreEditState()
{
    Int16      row;
    FormPtr   form;
    TablePtr  table;
    FieldPtr  field;


    if (gCurrentFieldIndex == noFieldIndex)
        return;

    // Find the row that the current field is in.
    table = GetObjectPtr(EditTable);
    if (! TblFindRowID(table, gCurrentFieldIndex, &row))
        return;

    form = FrmGetActiveForm();
    FrmSetFocus(form, FrmGetObjectIndex(form, EditTable));
    TblGrabFocus(table, row, dataColumn);
	
    // Restore the insertion point position.
    field = TblGetCurrentField(table);
    FldSetInsPtPosition(field, gEditFieldPosition);
    FldGrabFocus(field);
}


/***********************************************************************
 *
 * FUNCTION:    EditFormSaveRecord
 *
 * DESCRIPTION: Checks the record to see if it contains any data, and if
 *              it does, saves the record.  Note that a record only
 *              contains data if it has text in one or more of its text
 *              fields; the book status, print status, and read check box
 *              have no influence on whether or not Librarian considers
 *              the record to contain data.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormSaveRecord(void)
{
    MemHandle    recordH;
    LibDBRecordType  record;
    FormPtr   form;
    TablePtr  table;
    Err       error;
	
	
    // Make sure the field being edited is saved.  Releasing the focus
    // from the current table field triggers the EditFormSaveRecordField
    // callback and saves the current field.
    form = FrmGetActiveForm();
    table = FrmGetObjectPtr(form, FrmGetObjectIndex(form, EditTable));
    TblReleaseFocus(table);
	
    // If the record is needed after the Edit form closes, don't bother
    // saving data yet, since Librarian will return to the Edit form
    // again momentarily.  Also, all the changes to this record's field
    // have already been saved via EditFormSaveRecordField.
    if (gRecordNeededAfterEditView) {
        gListFormSelectThisRecord = noRecord;
        return;
    }
	
    // If the record has already been deleted from the menu command or
    // from the Details dialog, there won't be a gCurrentRecord.  In that
    // case, don't bother saving the record.
    if (gCurrentRecord == noRecord) {
        gListFormSelectThisRecord = noRecord;
        return;
    }
		
    // Retrieve the record.
    error = LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
    ErrNonFatalDisplayIf((error), "Record not found");
		
    // If there is no data in any field, delete the record.
    if (! RecordContainsData(&record)) {
        MemHandleUnlock(recordH);
        DeleteRecord(false);
        return;
    }
    else
        MemHandleUnlock(recordH);
	
    // The record's category may have been changed.  The current cateogory
    // isn't supposed to change in this case.  Make sure the current
    // record is still visible in this category; otherwise, pick another
    // record near it.
    if (! SeekRecord(&gCurrentRecord, 0, dmSeekBackward))
        if (! SeekRecord(&gCurrentRecord, 0, dmSeekForward))
            gCurrentRecord = noRecord;
	
    // Make sure this record is highlighted in the List view.
    gListFormSelectThisRecord = gCurrentRecord;
}


/***********************************************************************
 *
 * FUNCTION:    EditFormSaveRecordField
 *
 * DESCRIPTION: Callback routine to save a field in the Edit form to
 *              Librarian's database.  The Edit table object calls this
 *              routine when it wants to save an item.
 *
 * PARAMETERS:  table  - pointer to the Edit form table (TablePtr)
 *              row    - row of the table to save
 *              column - column of the table to save 
 *
 * RETURNED:    true if the table needs to be redrawn, false otherwise
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean EditFormSaveRecordField (MemPtr table, Int16 row,
                                        Int16 column)
{
    UInt16      fieldNum;
    FieldPtr  field;
    LibDBRecordType  record;
    MemHandle    recordH;
    MemHandle  textH;
    char *  text;
    LibDBRecordFlags  bits;
    Err       error;
    Boolean   redraw = false;
    UInt16      numOfRows;
    Int16       newSize;
   
   
#ifdef __GNUC__
    // CALLBACK_PROLOGUE;
#endif

    field = TblGetCurrentField(table);
    textH = FldGetTextHandle(field);
	
    // Get the field number that corresponds to the table item to save.
    fieldNum = TblGetRowID(table, row);
	
    // Save the field last edited.
    gEditRowIDWhichHadFocus = fieldNum;
	
    // Save the cursor position of the field last edited.
    // Check if the top of the text is scrolled off the top of the 
    // field; if it is, redraw the field.
    if (FldGetScrollPosition(field)) {
        FldSetScrollPosition(field, 0);
        gEditFieldPosition = 0;
    }
    else
        gEditFieldPosition = FldGetInsPtPosition(field);

    // Make sure that any selection is removed since we will free
    // the text memory before the callee can remove the selection.
    FldSetSelection(field, 0, 0);


    if (FldDirty(field)) {
        DirtyRecord(gCurrentRecord);
      
        if (textH == 0)
            text = NULL;
        else {
            text = MemHandleLock(textH);
            if (text[0] == '\0')
                text = NULL;
        }
         
      
        LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
        record.fields[fieldNum] = text;
      
        bits.allBits = (UInt32)1 << fieldNum;
        error = LibChangeRecord(gLibDB, &gCurrentRecord, &record, bits);

        // The new field has been copied into the new record.  Unlock it.
        if (text)
            MemPtrUnlock(text);

        // The change was not made (probably out of storage memory)      
        if (error) {
            // Because the storage is full the text in the text field
            // differs from the text in the record.
            // EditFormGetFieldHeight uses the text in the field
            // (because it's being edited).  Make the text in the field
            // the same as the text in the record.  Resizing should
            // always be possible.
            MemHandleUnlock(recordH);
            LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
         
            if (record.fields[fieldNum] == NULL)
                newSize = 1;
            else
                newSize = StrLen(record.fields[fieldNum]) + 1;
            
            if (! MemHandleResize(textH, newSize)) {
                text = MemHandleLock(textH);
                if (newSize > 1)
                    StrCopy(text, record.fields[fieldNum]);
                else
                    text[0] = '\0';
                MemPtrUnlock(text);
            
				// Update the text field.
                FldSetTextHandle(field, 0);
                FldSetTextHandle(field, textH);
            }
            else {
                ErrFatalDisplayIf(true, "Resize failed.");
            }
         
            MemHandleUnlock(recordH);
            FrmAlert(DeviceFullAlert);
         
            // The field may no longer be the same height.  This row
            // and those below may need to be recalculated. Mark this
            // row and those below it not usable and reload the table.
            numOfRows = TblGetNumberOfRows(table);
            while (row < numOfRows) {
                TblSetRowUsable(table, row, false);
                row++;
            }
            EditFormLoadTable();
            // Set the return value true to redraw the table, thereby
            // showing that some data was lost due to lack of storage
            // space.
            redraw = true;
        }
    }

    // Free the memory used for the field's text.
    FldFreeMemory(field);

#ifdef __GNUC__
    // CALLBACK_PROLOGUE;
#endif

    return redraw;
}


/***********************************************************************
 *
 * FUNCTION:    EditFormScroll
 *
 * DESCRIPTION: Scrolls the table of editable fields in the direction
 *              specified.
 *
 * PARAMETERS:  direction - up or dowm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormScroll(WinDirectionType direction)
{
    UInt16      row;
    UInt16      height;
    UInt16      fieldIndex;
    UInt16      columnWidth;
    UInt16      tableHeight;
    TablePtr  table;
    FontID    curFont;
    RectangleType  r;
    LibDBRecordType  record;
    MemHandle    recordH;


    curFont = FntSetFont(stdFont);
	
    table = GetObjectPtr(EditTable);
    TblReleaseFocus(table);
	
    // Get the height of the table and the width of the description
    // column.
    TblGetBounds(table, &r);
    tableHeight = r.extent.y;
    height = 0;
    columnWidth = TblGetColumnWidth(table, dataColumn);
	
    // Scroll the table down.
    if (direction == winDown) {
        // Get the index of the last visible field; this will become 
        // the index of the top visible field, unless it occupies the 
        // whole screeen, in which case the next field will be the
        // top field.
        row = TblGetLastUsableRow(table);
        fieldIndex = TblGetRowID(table, row);
		
        // If the last visible field is also the first visible field,
        // then it occupies the whole screeen.
        if (row == 0)
            fieldIndex = min(editLastFieldIndex, fieldIndex + 1);
    }

    // Scroll the table up.
    else {
        // Scan the fields before the first visible field to determine 
        // how many fields we need to scroll.  Since the heights of the 
        // fields vary, total the height of the records until we get
        // a screenful.
        fieldIndex = TblGetRowID(table, 0);
        ErrFatalDisplayIf(fieldIndex > editLastFieldIndex,
                          "Invalid field Index");
        // If we're at the top of the fields already, there is no need
        // to scroll.
        if (fieldIndex == 0) {
            FntSetFont(curFont);
            return;
        }
         
        // Get the current record.
        LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
		
        height = TblGetRowHeight(table, 0);
        if (height >= tableHeight)
            height = 0;                     

        while (height < tableHeight && fieldIndex > 0) {
            height += FldCalcFieldHeight(record.fields[fieldIndex - 1], 
                                         columnWidth) * FntLineHeight();
            if ( (height <= tableHeight) ||
                 (fieldIndex == TblGetRowID(table, 0)) )
                fieldIndex--;
        }

        MemHandleUnlock(recordH);
    }

    TblMarkTableInvalid(table);
    gCurrentFieldIndex = noFieldIndex;
    gTopVisibleFieldIndex = fieldIndex;
    gEditRowIDWhichHadFocus = editFirstFieldIndex;
    gEditFieldPosition = 0;
	
    // Remove the highlight before reloading the table to prevent the
    // selection information from being out of bounds, which can happen
    // if the newly loaded data has fewer rows than the old data.
    TblUnhighlightSelection(table);
    EditFormLoadTable();   
    TblRedrawTable(table);
    FntSetFont(curFont);
}


/***********************************************************************
 *
 * FUNCTION:    EditFormSelectCategory
 *
 * DESCRIPTION: Handles selection, creation and deletion of categories
 *              in the Edit view.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNS:     nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormSelectCategory(void)
{
    UInt16     attr;
    FormPtr  form;
    UInt16     category;
    Boolean  categoryEdited;
	
	
    // Process the category popup list.
    DmRecordInfo(gLibDB, gCurrentRecord, &attr, NULL, NULL);
    category = attr & dmRecAttrCategoryMask;
	
    form = FrmGetActiveForm();
    categoryEdited = CategorySelect(gLibDB, form, EditCategoryPopTrigger,
                                    EditCategoryList, false, &category, gCategoryName, 1, 0);
   
    if (categoryEdited || (category != (attr & dmRecAttrCategoryMask))) {
        // Change the category of the record.
        DmRecordInfo(gLibDB, gCurrentRecord, &attr, NULL, NULL);   
        attr &= ~dmRecAttrCategoryMask;
        attr |= category | dmRecAttrDirty;
        DmSetRecordInfo(gLibDB, gCurrentRecord, &attr, NULL);
		
        ChangeCategory (category);
    }
	
    // Restore the current selection in the Edit table.
    EditFormRestoreEditState();
}


/***********************************************************************
 *
 * FUNCTION:    EditFormUpdateDisplay
 *
 * DESCRIPTION: Updates the display of the Edit view.
 *
 * PARAMETERS:  updateCode - a code that indicates what changes have
 *                           been made to the view; part of a
 *                           frmUpdateEvent.
 *                		
 * RETURNED:    true if frmUpdateEvent was handled, false otherwise.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean EditFormUpdateDisplay(UInt16 updateCode)
{
    Boolean   handled = false;
    TablePtr  table;


    if (updateCode & updateCategoryChanged) {
        // Set the label of the category trigger.
        CategoryGetName(gLibDB, gCurrentCategory, gCategoryName);
        CategorySetTriggerLabel(GetObjectPtr(EditCategoryPopTrigger),
                                gCategoryName);
        handled = true;
    }

    if ( (updateCode & updateFontChanged) ||
         (updateCode & frmRedrawUpdateCode) ) {
        if (updateCode & frmRedrawUpdateCode) {
            FrmDrawForm(FrmGetActiveForm());
        }

        table = GetObjectPtr(EditTable);

        if (updateCode & updateFontChanged) {
            TblReleaseFocus(table);
        }
			
        if (updateCode & frmRedrawUpdateCode) {
            if (TblEditing(table)) {
                Int16  row;
                Int16  column;
				
                TblGetSelection(table, &row, &column);
                TblMarkRowInvalid(table, row);
                TblRedrawTable(table);
            }
        }
		
        if (updateCode & updateFontChanged) {
            EditFormLoadTable();
            TblRedrawTable(table);
            EditFormRestoreEditState();
        }
		
        handled = true;
    }
	
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    EditFormUpdateScrollers
 *
 * DESCRIPTION: This routine draws or erases the edit view scroll arrow
 *              buttons.
 *
 * PARAMETERS:  form            - pointer to the Edit form
 *              bottomField     - field index of the last visible row
 *              lastItemClipped - true if the last visible row is
 *                                clipped at the bottom
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditFormUpdateScrollers(FormPtr form, UInt16 bottomFieldIndex,
                                    Boolean lastItemClipped)
{
    UInt16 upIndex;
    UInt16 downIndex;
    Boolean scrollableUp;
    Boolean scrollableDown;
	  
    // If the first field displayed is not the first field in the record,
    // enable the up scroller.
    scrollableUp = gTopVisibleFieldIndex > 0;
	
    // If the last field displayed is not the last field in the record,
    // enable the down scroller.
    scrollableDown = (lastItemClipped ||
                      (bottomFieldIndex < editLastFieldIndex));
	
    // Update the scroll button.
    upIndex = FrmGetObjectIndex(form, EditScrollUpRepeating);
    downIndex = FrmGetObjectIndex(form, EditScrollDownRepeating);
    FrmUpdateScrollers(form, upIndex, downIndex, scrollableUp,
                       scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    EditInitTableRow
 *
 * DESCRIPTION: Initializes a row in the Edit view's table.
 *
 * PARAMETERS:  table      - pointer to the Edit table
 *              row        - row number to initialize (first row is zero)
 *              fieldIndex - the index of the field displayed in the row
 *              rowHeight  - height of the row in pixels
 *              fontID     - current data field display font in Edit view
 *              record     - pointer to the record currently displayed
 *                           by the Edit view
 *              appInfo    - pointer to Librarian's application info block
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditInitTableRow(TablePtr table, UInt16 row, UInt16 fieldIndex, 
                             short rowHeight, FontID fontID, LibDBRecordType *record,
                             LibAppInfoType *appInfo)
{
    // Make the row usable.
    TblSetRowUsable(table, row, true);
	
	// Set the height of the row to the height of the data text field.
    TblSetRowHeight(table, row, rowHeight);
	
    // Store the record number as the row ID.
    TblSetRowID(table, row, fieldIndex);
	
    // Mark the row invalid so that it will draw when we call the 
    // draw routine.
    TblMarkRowInvalid(table, row);
	
	// Set the text font if Librarian is running on version 3.0 or later.
    if (gROMVersion >= sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
        TblSetItemFont(table, row, dataColumn, fontID);
	
	// Set the labels in the label column.
    TblSetItemPtr(table, row, labelColumn,
                  appInfo->fieldLabels[fieldIndex]);
}


/***********************************************************************
 *
 * FUNCTION:    EditSetGraffitiMode
 *
 * DESCRIPTION: Turns on Graffiti auto-shifting for the specified field.
 *
 * PARAMETERS:  field - pointer to the field
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EditSetGraffitiMode(FieldPtr field)
{
    FieldAttrType  attr;
	
	
    if (field) {
        FldGetAttributes(field, &attr);
        attr.autoShift = true;
        FldSetAttributes(field, &attr);
    }
}


#pragma mark ----------------

/***********************************************************************
 *
 * FUNCTION:    DrawCharsInWidth
 *
 * DESCRIPTION: Draws the indicated string to fit within a given width.
 *              If the string does not fit in the width allowed, truncates
 *              the string and appends an ellipsis (...) to the end of
 *              the characters drawn to indicate that the string is
 *              incomplete.
 *
 * PARAMETERS:  str     -> string to draw
 *              width  <-> maximum width allowed for the string; replaced
 *                         with the amount of space actually used to draw
 *                         the string
 *              length <-> length of the string in bytes; replaced with
 *                         the actual length in bytes that were used
 *              x       -> x coordinate at which to draw the string; if
 *                         rightJustify is true, x represents the location
 *                         of the right side of the text string
 *              y       -> y coordinate at which to draw the string
 *              rightJustify -> if true, right-justify the text drawn;
 *                              if false, left-justify the text drawn
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void DrawCharsInWidth(char *str, Int16 *width, Int16 *length,
                             Int16 x, Int16 y, Boolean rightJustify)
{	
    Int16      ellipsisWidth;
    Boolean  fitInWidth;
    Int16      maxWidth;
    Int16    newX;
	
	
    maxWidth = *width;
	
    // Determine whether the string will fit within the maximum width.
    FntCharsInWidth(str, width, length, &fitInWidth);
	
    // If the string fit within the maximum width, draw it.
    if (fitInWidth) {
	//if (*extent < width) {
        if (rightJustify)
            WinDrawChars(str, *length, x - *width, y);
        else
            WinDrawChars(str, *length, x, y);
    }
	
    // The string was truncated; append an ellipsis to the
    // end of the string, and recalculate the portion of the string
    // that can be drawn, since the ellipsis shortens the width
    // available.
    else {
        // Set the width of the ellipsis.
        ellipsisWidth = (FntCharWidth('.') * ellipsisLength);
		
        *width -= ellipsisWidth;
        FntCharsInWidth(str, width, length, &fitInWidth);
		
        if (rightJustify)
            newX = x - *width - ellipsisWidth;
        else
            newX = x;
			
        WinDrawChars(str, *length, newX, y);
        newX += *width;
        WinDrawChars(ellipsisString, ellipsisLength, newX, y);
		
        // Add the width of the ellipsis to return the actual width used
        // to draw the string.
        *width += ellipsisWidth;
    }
}


/***********************************************************************
 *
 * FUNCTION:    DrawRecordName
 *
 * DESCRIPTION: Draws the appropriate part of a book record into the
 *              given screen coordinates.  DrawRecordName draws
 *              the text of a couple fields from the record, depending on
 *              the gShowInList selection.  Used in the List view and
 *              by the global Find feature.
 *
 * PARAMETERS:  record - record to draw
 *              bounds - bounds of the draw region
 *              showInList - current sort order for the Librarian database
 *              noAuthor - pointer to string to contain the "-No Author-"
 *                         string if this record does not have an author
 *              noTitle - pointer to string to contain the "-Untitled-"
 *                        string if this record does not have a title
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void DrawRecordName(LibDBRecordType *record, RectanglePtr bounds,
                           UInt8 showInList, char **noAuthor,
                           char **noTitle)
{
    char * name1 = NULL;
    char * name2 = NULL;
    Int16      name1Length, name2Length;
    Int16      name1Width, name2Width, nameTotalWidth;
    Int16      name1Extent, name2Extent;
    Int16      concatenated = 0;
    Int16    x;
	   

    // Determine what the display should look like and retrieve the
    // appropriate strings.
    switch (showInList) {
        case libShowAuthorTitle:
            if (GetAuthorString(record, &name1, &name1Length, 
                                &name1Width, false, noAuthor))
                concatenated = 1;
            GetTitleString(record, &name2, &name2Length, &name2Width, 
                           noTitle);
            break;
			
        case libShowTitleAuthor:
            GetTitleString(record, &name1, &name1Length, &name1Width,
                           noTitle);
            if (GetAuthorString(record, &name2, &name2Length,
                                &name2Width, false, noAuthor))
                concatenated = 2;
            break;
			
        case libShowTitleOnly:
            GetTitleString(record, &name1, &name1Length, &name1Width,
                           noTitle);
            break;
			
        default:
            break;
    }
	
    // Draw the two appropriate fields from the record.
    nameTotalWidth = bounds->extent.x;
    x = bounds->topLeft.x;
	
    // If there is only one name (which happens when the sort order is
    // libShowTitleOnly), draw it using all the space available.  Otherwise,
    // draw both names, sharing the available space between them.
    if (name2 == NULL) {
        DrawCharsInWidth(name1, &nameTotalWidth, &name1Length, x,
                         bounds->topLeft.y, false);
    }
    // Otherwise, draw both names.
    else {
        // Leave space between names.
        nameTotalWidth -= spaceBetweenNames;
		
        name1Extent = nameTotalWidth - min(name2Width, gName2MaxWidth);
		
        // Draw the first name; DrawCharsInWidth adjusts name1Extent to the
        // actual length used to draw the string.
        DrawCharsInWidth(name1, &name1Extent, &name1Length, x,
                         bounds->topLeft.y, false);
		
        // Draw the second name, right-justified.
        name2Extent = nameTotalWidth - name1Extent;
        x = bounds->topLeft.x + nameTotalWidth + spaceBetweenNames;
        DrawCharsInWidth(name2, &name2Extent, &name2Length, x,
                         bounds->topLeft.y, true);
    }
		
    // Check to see if any calls to GetAuthorString resulted in a
    // concatenation of last and first names.  If so, unlock the pointer
    // associated with the concatenated string.
    switch (concatenated) {
        case 1:
            MemHandleFree(MemPtrRecoverHandle(name1));
            break;
		
        case 2:
            MemHandleFree(MemPtrRecoverHandle(name2));
            break;
		
        default:
            break;
    }
}


/***********************************************************************
 *
 * FUNCTION:    ListFormDrawRecord
 *
 * DESCRIPTION: This callback routine draws a book record in the List
 *              view table.  The table object calls this function to
 *              draw each record in the table.
 *
 * PARAMETERS:  table  - pointer to the list view table
 *              row    - row number, in the table, of the item to draw
 *              column - column number, in the table, of the item to draw
 *              bounds - bounds of the draw region
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 ***********************************************************************/
static void ListFormDrawRecord(MemPtr table, Int16 row, Int16 column, 
                               RectangleType *bounds)
{
    UInt16  recordNum;
    Err     error;
    MemHandle  recordH;
    char    noteChar;
    FontID  curFont;
    LibDBRecordType  record;
    char    statusChr;
    Int16   x;
    UInt8   showInList;

   
#ifdef __GNUC__
    // CALLBACK_PROLOGUE;
#endif

    curFont = FntSetFont(gListFont);
	
    // Get the record number that corresponds to the table item to draw.
    recordNum = TblGetRowID(table, row);
	
    // Retrieve a locked handle to the record.  Remember to unlock
    // recordH later when finished with the record.
    error = LibGetRecord(gLibDB, recordNum, &record, &recordH);
    ErrNonFatalDisplayIf((error), "Record not found");
    if (error) {
        MemHandleUnlock(recordH);
        return;
    }
	
    switch (column) {
        case titleColumn:
            showInList = LibGetSortOrder(gLibDB);
            DrawRecordName(&record, bounds, showInList,
                           &gNoAuthorRecordString, &gNoTitleRecordString);
            break;

        case bookStatusColumn:
            switch (record.status.bookStatus) {
                case bookStatusHave:
                    statusChr = libHaveStatusChr;
                    break;
					
                case bookStatusWant:
                    statusChr = libWantStatusChr;
                    break;
				
                case bookStatusOnOrder:
                    statusChr = libOnOrderStatusChr;
                    break;
					
                case bookStatusLoaned:
                    statusChr = libLoanedStatusChr;
                    break;
					
                default:
                    break;
            }
            x = bounds->topLeft.x + (bounds->extent.x / 2) -
                (FntCharWidth(statusChr) / 2);
            WinDrawChars(&statusChr, 1, x, bounds->topLeft.y);
            break;
			
        case printStatusColumn:
            switch (record.status.printStatus) {
                case printStatusInPrint:
                    statusChr = libInPrintStatusChr;
                    break;
					
                case printStatusOutOfPrint:
                    statusChr = libOutOfPrintStatusChr;
                    break;
					
                case printStatusNotPublished:
                    statusChr = libNotPublishedStatusChr;
                    break;
					
                default:
                    break;
            }
            x = bounds->topLeft.x + (bounds->extent.x / 2) -
                (FntCharWidth(statusChr) / 2);
            WinDrawChars(&statusChr, 1, x, bounds->topLeft.y);
            break;
			
        case formatColumn:
            switch (record.status.format) {
                case formatHardcover:
                    statusChr = libHardcoverStatusChr;
                    break;
				
                case formatPaperback:
                    statusChr = libPaperbackStatusChr;
                    break;
				
                case formatTradePaperback:
                    statusChr = libTradePaperStatusChr;
                    break;
				
                case formatOther:
                    statusChr = libOtherStatusChr;
                    break;
				
                default:
                    break;
            }
            x = bounds->topLeft.x + (bounds->extent.x / 2) -
                (FntCharWidth(statusChr) / 2);
            WinDrawChars(&statusChr, 1, x, bounds->topLeft.y);
            break;
		
        case readColumn:
            if (record.status.read)
                statusChr = libReadStatusChr;
            else
                statusChr = libUnreadStatusChr;
            x = bounds->topLeft.x + (bounds->extent.x / 2) -
                (FntCharWidth(statusChr) / 2);
            WinDrawChars(&statusChr, 1, x, bounds->topLeft.y);
            break;
		
        case noteColumn:
            // Draw a note symbol if the field has a note.
            if (record.fields[libFieldNote]) {
                FntSetFont(symbolFont);
                noteChar = symbolNote;
                WinDrawChars (&noteChar, 1, bounds->topLeft.x,
                              bounds->topLeft.y);
                FntSetFont(gListFont);
            }
            break;
		
        default:
            break;
    }
	
    // Since the handle returned from LibGetRecord (recordH) is no longer
    // needed, unlock it.
    MemHandleUnlock(recordH);
	
    FntSetFont(curFont);

#ifdef __GNUC__
    // CALLBACK_EPILOGUE;
#endif
}


/***********************************************************************
 *
 * FUNCTION:    ListFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command - menu item ID
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean ListFormDoCommand(UInt16 command)
{
    Boolean handled = false;


    if (HandleCommonMenus(command))
        return true;
	
    switch (command) {
			
        default:
            break;
    }

    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    ListFormHandleEvent
 *
 * DESCRIPTION: Event handler for the List form.
 *
 * PARAMETERS:  event - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/02/26  Changed code to handle 3.5 masked records.
 *
 ***********************************************************************/
static Boolean ListFormHandleEvent(EventPtr event)
{
    Boolean  handled = false;
    FormPtr  form;


    switch (event->eType) {
        case menuEvent:
            return ListFormDoCommand(event->data.menu.itemID);

        case menuCmdBarOpenEvent:
            MenuCmdBarAddButton(menuCmdBarOnLeft, BarSecureBitmap,
                                menuCmdBarResultMenuItem, OptionsV35Security, 0);
			
            // Prevent the field manager from adding its own buttons;
            // this command bar already contains all the buttons it
            // needs.
            event->data.menuCmdBarOpen.preventFieldButtons = true;
			
            // Leave event unhandled so the system can catch it.
            break;
		
        case frmOpenEvent:
            form = FrmGetActiveForm();
            ListFormInit(form);
			
            if (gListFormSelectThisRecord != noRecord)
                ListFormSelectRecord(gListFormSelectThisRecord);
			
            FrmDrawForm(form);
			
            if (gListFormSelectThisRecord != noRecord) {
                ListFormSelectRecord(gListFormSelectThisRecord);
                gListFormSelectThisRecord = noRecord;
            }
			
            handled = true;
            break;

        case tblSelectEvent:
            if (gROMVersion >=
                sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {
                if (TblRowMasked(event->data.tblSelect.pTable, 
                                 event->data.tblSelect.row)) {
                    if (SecVerifyPW(showPrivateRecords) == true) {
                        gPrivateRecordStatus = showPrivateRecords;
                        // Force taps in status and note columns to behave
                        // the same as taps in the title column.
                        event->data.tblSelect.column = titleColumn;
                    }
                    else
                        break;
                }
            }
			
            gCurrentRecord = TblGetRowID(event->data.tblSelect.pTable,
                                         event->data.tblSelect.row);

            ListFormItemSelected(event);
            handled = true;
            break;
		
        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID) {
                case ListNewButton:
                    EditFormNewRecord();
                    handled = true;
                    break;
		    		
                case ListShowButton:
                    FrmPopupForm(PrefsForm);
                    handled = true;
                    break;
		    		
                case ListCategoryPopTrigger:
                    ListFormSelectCategory();
                    handled = true;
                    break;
		    		
                default:
                    break;
            }
	    	
            break;
		
        case ctlRepeatEvent:
            switch (event->data.ctlEnter.controlID) {
                case ListScrollUpRepeating:
                    ListFormScroll(winUp, 1, false);
                    // Leave unhandled so button can repeat.
                    break;
				
                case ListScrollDownRepeating:
                    ListFormScroll(winDown, 1, false);
                    // Leave unhandled so button can repeat.
                    break;
					
                default:
                    break;
            }
            break;
			
        case keyDownEvent:
            if (TxtCharIsHardKey(event->data.keyDown.modifiers,
                                 event->data.keyDown.chr)) {
                if (! (event->data.keyDown.modifiers & poweredOnKeyMask)) {
                    ListFormNextCategory();
                    handled = true;
                }
            }
            else {
                switch (event->data.keyDown.chr) {
                    case pageUpChr:
                        ListFormScroll(winUp, 1, false);
                        handled = true;
                        break;
					
                    case pageDownChr:
                        ListFormScroll(winDown, 1, false);
                        handled = true;
                        break;
					
                    case linefeedChr:
                        if (gCurrentRecord != noRecord)
                            FrmGotoForm(RecordForm);
                        handled = true;
                        break;
					
                    default:
                        ListFormLookup(event);
                        handled = true;
                        break;
                }
            }
            break;
				
        case frmUpdateEvent:
            ListFormUpdateDisplay(event->data.frmUpdate.updateCode);
            handled = true;
            break;
		
        case frmCloseEvent:
            // Unlock resource strings for unnamed items.
            if (gNoAuthorRecordString) {
                MemPtrUnlock(gNoAuthorRecordString);
                gNoAuthorRecordString = NULL;
            }
            if (gNoTitleRecordString) {
                MemPtrUnlock(gNoTitleRecordString);
                gNoTitleRecordString = NULL;
            }

            // Keep track of the last form so the Note view knows where
            // to return to.
            gPriorFormID = ListForm;
            break;
			
        default:
            break;
		
    }
	
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    ListFormInit
 *
 * DESCRIPTION: Initializes the List view form.
 *
 * PARAMETERS:  form - pointer to the ListForm form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ListFormInit(FormPtr form)
{
    UInt16      row;
    UInt16      rowsInTable;
    TablePtr    table;
    ControlPtr  ctl;
    Int16       statusWidth;
    FontID      curFont;
    char        noteChar;
    Boolean     statusExists;
	

    curFont = FntSetFont(gListFont);
	
    if (gShowAllCategories)
        gCurrentCategory = dmAllCategories;

    // Initialize the book list table.
    table = FrmGetObjectPtr(form, FrmGetObjectIndex(form, ListTable));
    rowsInTable = TblGetNumberOfRows(table);
    for (row = 0; row < rowsInTable; row++) {
        TblSetItemStyle(table, row, titleColumn, customTableItem);
        TblSetItemStyle(table, row, bookStatusColumn, customTableItem);
        TblSetItemStyle(table, row, printStatusColumn, customTableItem);
        TblSetItemStyle(table, row, formatColumn, customTableItem);
        TblSetItemStyle(table, row, readColumn, customTableItem);
        TblSetItemStyle(table, row, noteColumn, customTableItem);
		
        if (gROMVersion >= sysMakeROMVersion(3,0,0,sysROMStageRelease,0)) {
            TblSetItemFont(table, row, titleColumn, gListFont);
            TblSetItemFont(table, row, bookStatusColumn, gListFont);
            TblSetItemFont(table, row, printStatusColumn, gListFont);
            TblSetItemFont(table, row, formatColumn, gListFont);
            TblSetItemFont(table, row, readColumn, gListFont);
        }
				
        TblSetRowUsable(table, row, false);
    }

    TblSetColumnUsable(table, titleColumn, true);
    TblSetColumnUsable(table, bookStatusColumn, gShowBookStatus);
    TblSetColumnUsable(table, printStatusColumn, gShowPrintStatus);
    TblSetColumnUsable(table, formatColumn, gShowFormat);
    TblSetColumnUsable(table, readColumn, gShowReadUnread);
    TblSetColumnUsable(table, noteColumn, true);

    // If Librarian is running on Palm OS 3.5 or later, set up the
    // columns to allow masked records.
    if (gROMVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {
        TblSetColumnMasked(table, titleColumn, true);
        TblSetColumnMasked(table, bookStatusColumn, true);
        TblSetColumnMasked(table, printStatusColumn, true);
        TblSetColumnMasked(table, formatColumn, true);
        TblSetColumnMasked(table, readColumn, true);
        TblSetColumnMasked(table, noteColumn, true);
    }

    // Set the width of the book status column.
    if (gShowBookStatus) {
        TblSetColumnWidth(table, bookStatusColumn,
                          FntCharWidth(libWidestBookStatusChr));
        if (gShowPrintStatus || gShowFormat || gShowReadUnread)
            TblSetColumnSpacing(table, bookStatusColumn, 0);
        else
            TblSetColumnSpacing(table, bookStatusColumn, 1);
    }
	
    // Set the width of the print status column.
    if (gShowPrintStatus) {
        TblSetColumnWidth(table, printStatusColumn,
                          FntCharWidth(libWidestPrintStatusChr));
        if (gShowFormat || gShowReadUnread)
            TblSetColumnSpacing(table, printStatusColumn, 0);
        else
            TblSetColumnSpacing(table, printStatusColumn, 1);
    }
	
    // Set the width of the format column.
    if (gShowFormat) {
        TblSetColumnWidth(table, formatColumn,
                          FntCharWidth(libWidestFormatStatusChr));
        if (gShowReadUnread)
            TblSetColumnSpacing(table, formatColumn, 0);
        else
            TblSetColumnSpacing(table, formatColumn, 1);
    }
	
    // Set the width of the read column.
    if (gShowReadUnread) {
        TblSetColumnWidth(table, readColumn,
                          FntCharWidth(libWidestReadUnreadChr));
        TblSetColumnSpacing(table, readColumn, 1);
    }
	
    // Set the width of the note column.
    FntSetFont(symbolFont);
    noteChar = symbolNote;
    TblSetColumnWidth(table, noteColumn, FntCharWidth(noteChar));
    FntSetFont(gListFont);
	
    statusExists = (gShowBookStatus || gShowPrintStatus ||
                    gShowFormat || gShowReadUnread);
	
    // Set the width and column spacing of the title column.
    statusWidth = ( (statusExists ? spaceBeforeStatus + 1 : 1) +
                    (gShowBookStatus ? TblGetColumnWidth(table,
                                                         bookStatusColumn) : 0) +
                    (gShowPrintStatus ? TblGetColumnWidth(table,
                                                          printStatusColumn) : 0) +
                    (gShowFormat ? TblGetColumnWidth(table,
                                                     formatColumn) : 0) +
                    (gShowReadUnread ? TblGetColumnWidth(table,
                                                         readColumn) : 0) +
                    TblGetColumnWidth(table, noteColumn) + 1);
	
    TblSetColumnWidth(table, titleColumn, table->bounds.extent.x -
                      statusWidth);
    if (statusExists)
        TblSetColumnSpacing(table, titleColumn, spaceBeforeStatus);
    else
        TblSetColumnSpacing(table, titleColumn, 1);
	
    // Set the callback routine that will draw the records.
    TblSetCustomDrawProcedure(table, titleColumn, ListFormDrawRecord);
    TblSetCustomDrawProcedure(table, bookStatusColumn, ListFormDrawRecord);
    TblSetCustomDrawProcedure(table, printStatusColumn, ListFormDrawRecord);
    TblSetCustomDrawProcedure(table, formatColumn, ListFormDrawRecord);
    TblSetCustomDrawProcedure(table, readColumn, ListFormDrawRecord);
    TblSetCustomDrawProcedure(table, noteColumn, ListFormDrawRecord);
	
    // Load records into the book list.
    ListFormLoadTable();
	
    // Set the label of the category trigger.
    ctl = GetObjectPtr(ListCategoryPopTrigger);
    CategoryGetName(gLibDB, gCurrentCategory, gCategoryName);
    CategorySetTriggerLabel(ctl, gCategoryName);
	
    FntSetFont(curFont);
	
    // If Librarian is running on Palm OS 2.0, change the menus to the V20
    // versions.  Likewise, if running on 3.5, use the V35 menu.
    if (gROMVersion < sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
        FrmSetMenu(form, ListV20MenuBar);
    if (gROMVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0))
        FrmSetMenu(form, ListV35MenuBar);
}


/***********************************************************************
 *
 * FUNCTION:    ListFormItemSelected
 *
 * DESCRIPTION: Handles selection of an item in the List view table.
 *
 * PARAMETERS:  event - pointer to the tblSelectEvent
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ListFormItemSelected(EventPtr event)
{
    switch (event->data.tblSelect.column) {
        case titleColumn:
            // Set global variables to force the Edit view to display the
            // first field in the selected record.  Also reset the field
            // position in that field to zero so the insertion point does
            // not appear in a seemingly random position as the user
            // switches between records.
            gTopVisibleFieldIndex = 0;
            gEditRowIDWhichHadFocus = editFirstFieldIndex;
            gEditFieldPosition = 0;
		         
            FrmGotoForm(RecordForm);
            break;
			
        case bookStatusColumn:
        case printStatusColumn:
        case formatColumn:
        case readColumn:
            ListFormSelectFromList(event->data.tblSelect.pTable,
                                   event->data.tblSelect.row,
                                   event->data.tblSelect.column);
            break;
			
        case noteColumn:
            if (CreateNote())
                FrmGotoForm(gNoteFormID);
            break;
			
        default:
            break;
    }
}


/***********************************************************************
 *
 * FUNCTION:    ListFormLoadTable
 *
 * DESCRIPTION: Loads Librarian database records into the List view's
 *              table.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/02/26  Changed code to handle 3.5 masked records.
 *
 ***********************************************************************/
static void ListFormLoadTable(void)
{
    UInt16    row, numRows, visibleRows;
    UInt16	  lineHeight;
    UInt16	  recordNum;
    FontID	  curFont;
    TablePtr  table;
    UInt16    attr;

   
    table = GetObjectPtr(ListTable);
	
    TblUnhighlightSelection(table);
	
    // Try going forward to the last record that should be visible.
    visibleRows = ListFormNumberOfRows(table);
    recordNum = gTopVisibleRecord;
    if (! SeekRecord(&recordNum, visibleRows - 1, dmSeekForward)) {
        // At least one line has no record.  Fix it.
        // Try going backwards one page from the last record.
        gTopVisibleRecord = dmMaxRecordIndex;
        if (! SeekRecord(&gTopVisibleRecord, visibleRows - 1,
                         dmSeekBackward)) {
            // Not enough records to fill one page.  Start with the first
            // record.
            gTopVisibleRecord = 0;
            SeekRecord(&gTopVisibleRecord, 0, dmSeekForward);
        }
    }

    curFont = FntSetFont(gListFont);
    lineHeight = FntLineHeight();
    FntSetFont(curFont);

    recordNum = gTopVisibleRecord;
	
    for (row = 0; row < visibleRows; row++) {
        if (! SeekRecord(&recordNum, 0, dmSeekForward))
            break;

        // Make the row usable.
        TblSetRowUsable(table, row, true);
		
        if (gROMVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {
            Boolean  masked;
			
			
            DmRecordInfo (gLibDB, recordNum, &attr, NULL, NULL);
            masked = (((attr & dmRecAttrSecret) &&
                       gPrivateRecordStatus == maskPrivateRecords));
            TblSetRowMasked(table, row, masked);
        }

		
        // Mark the row invalid so that it will draw when we call the 
        // draw routine.
        TblMarkRowInvalid(table, row);
		
        // Store the record number as the row ID.
        TblSetRowID(table, row, recordNum);
		
        TblSetRowHeight(table, row, lineHeight);
		
        recordNum++;
    }

    // Hide the items that don't have any data.
    numRows = TblGetNumberOfRows(table);
    while (row < numRows) {      
        TblSetRowUsable(table, row, false);
        row++;
    }

    // Update the List view's scroll buttons.
    ListFormUpdateScrollButtons();
}


/***********************************************************************
 *
 * FUNCTION:    ListFormLookup
 *
 * DESCRIPTION: Looks up a record beginning with a Graffiti character
 *              entered by the user and scrolls the List view to the
 *              first matching record. 
 *
 * PARAMETERS:  event - pointer to the event containing the lookup
 *              character
 *                      
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ListFormLookup(EventPtr event)
{
    UInt16  foundRecord = noRecord;
	
	        
    if (LibLookupChar(gLibDB, event->data.keyDown.chr, gCurrentCategory,
                      gShowInList, &foundRecord))
        // Select the first record beginning with the entered character.
        ListFormSelectRecord(foundRecord);
    else
        // Play the error sound if no record beginning with the entered
        // character exists.
        SndPlaySystemSound(sndError);
}


/***********************************************************************
 *
 * FUNCTION:    ListFormNextCategory
 *
 * DESCRIPTION: Displays the next category.  If the last category is
 *              currently displayed, displays the first category in
 *              the list.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ListFormNextCategory(void)
{
    UInt16        category;
    TablePtr    table;
    ControlPtr  ctl;   
	
	
    category = CategoryGetNext(gLibDB, gCurrentCategory);
	
    if (category != gCurrentCategory) {
        if (category == dmAllCategories)
            gShowAllCategories = true;
        else
            gShowAllCategories = false;
		
        ChangeCategory(category);
		
        // Set the label of the category trigger.
        ctl = GetObjectPtr(ListCategoryPopTrigger);
        CategoryGetName(gLibDB, gCurrentCategory, gCategoryName);
        CategorySetTriggerLabel(ctl, gCategoryName);
		
        // Display the new category.
        ListFormLoadTable();
        table = GetObjectPtr(ListTable);
        TblEraseTable(table);
        TblDrawTable(table);
		
        // By changing the category the current record is lost.
        gCurrentRecord = noRecord;
    }
}


/***********************************************************************
 *
 * FUNCTION:    ListFormNumberOfRows
 *
 * DESCRIPTION: Returns the maximun number of visible rows for the
 *              current list view font setting.
 *
 * PARAMETERS:  table - List View table
 *
 * RETURNED:    maximun number of displayable rows
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt16 ListFormNumberOfRows(TablePtr table)
{
    UInt16  rows, rowsInTable;
    UInt16  tableHeight;
    FontID  curFont;
    RectangleType  r;


    rowsInTable = TblGetNumberOfRows(table);

    TblGetBounds(table, &r);
    tableHeight = r.extent.y;

    curFont = FntSetFont(gListFont);
    rows = tableHeight / FntLineHeight();
    FntSetFont(curFont);

    if (rows <= rowsInTable)
        return (rows);
    else
        return (rowsInTable);
}


/***********************************************************************
 *
 * FUNCTION:    ListFormScroll
 *
 * DESCRIPTION: Scrolls the List view table in the direction specified.
 *
 * PARAMETERS:  direction - up or dowm
 *              units	  - unit amount to scroll
 *              byLine	  - If true, list scrolls in line units.
 *							If false, list scrolls in page units.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ListFormScroll(WinDirectionType direction, UInt16 units,
                           Boolean byLine)
{
    TablePtr  table;
    UInt16      rowsInPage;
    UInt16      newTopVisibleRecord;
	
    table = GetObjectPtr(ListTable);
    rowsInPage = ListFormNumberOfRows(table) - 1;
    newTopVisibleRecord = gTopVisibleRecord;

    // Scroll the table down.
    if (direction == winDown) {
        // Scroll down by line units.
        if (byLine) {
            // Scroll down by the requested number of lines
            if (! SeekRecord(&newTopVisibleRecord, units,
                             dmSeekForward)) {
				// Tried to scroll past bottom; go to the last record.
                newTopVisibleRecord = dmMaxRecordIndex;
                SeekRecord(&newTopVisibleRecord, 1, dmSeekBackward);
            }
        }
        // Scroll in page units.
        else {
            // Try scrolling down by the requested number of pages.
            if (! SeekRecord(&newTopVisibleRecord, units * rowsInPage,
                             dmSeekForward)) {
				// Hit bottom; try going backwards one page from the last
				// record.
                newTopVisibleRecord = dmMaxRecordIndex;
                if (! SeekRecord(&newTopVisibleRecord, rowsInPage,
                                 dmSeekBackward)) {
                    // Not enough records to fill one page; go to the
                    // first record.
                    newTopVisibleRecord = 0;
                    SeekRecord(&newTopVisibleRecord, 0, dmSeekForward);
                }
            }
        }
    }
    // Scroll the table up.
    else {
        // Scroll up by line units.
        if (byLine) {
            // Scroll up by the requested number of lines
            if (! SeekRecord(&newTopVisibleRecord, units,
                             dmSeekBackward)) {
				// Tried to scroll past top; go to the first record.
                newTopVisibleRecord = 0;
                SeekRecord(&newTopVisibleRecord, 0, dmSeekForward);
            }
        }
        // Scroll in page units.
        else {
            // Try scrolling up by the requested number of pages.
            if (! SeekRecord(&newTopVisibleRecord, units * rowsInPage,
                             dmSeekBackward)) {
				// Hit top; go to the first record.
                newTopVisibleRecord = 0;
                SeekRecord(&newTopVisibleRecord, 0, dmSeekForward);
            }
        }
    }

    // Redraw the table, but only if there has been a change.
    if (gTopVisibleRecord != newTopVisibleRecord) {
        gTopVisibleRecord = newTopVisibleRecord;
        ListFormLoadTable();
        TblRedrawTable(table);
    }
}


/***********************************************************************
 *
 * FUNCTION:    ListFormSelectCategory
 *
 * DESCRIPTION: Handles selection, creation and deletion of
 *              categories in the List view form.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    The index of the newly selected category.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt16 ListFormSelectCategory(void)
{
    FormPtr   form;
    TablePtr  table;
    UInt16      category;
    Boolean   categoryEdited;
	
    // Process the category popup list.  
    category = gCurrentCategory;
	
    form = FrmGetActiveForm();
    categoryEdited = CategorySelect(gLibDB, form, ListCategoryPopTrigger,
                                    ListCategoryList, true, &category, gCategoryName, 1, 0);
	
    if (category == dmAllCategories)
        gShowAllCategories = true;
    else
        gShowAllCategories = false;
	  
    if (categoryEdited || (category != gCurrentCategory)) {
        ChangeCategory(category);
		
        // Display the new category.
        ListFormLoadTable();
        table = GetObjectPtr(ListTable);
        TblEraseTable(table);
        TblDrawTable(table);
		
        // By changing the category the current record is lost.
        gCurrentRecord = noRecord;
    }
	
    return (category);
}


/***********************************************************************
 *
 * FUNCTION:    ListFormSelectFromList
 *
 * DESCRIPTION: Handles display of and selection from a popup list.
 *              ListFormItemSelected calls this function when the user
 *              selects the book status, print status, or read columns
 *              in the List view table.
 *
 * PARAMETERS:  table - pointer to the List view table
 *              row - row selected in the table
 *              column - column selected in the table
 *              
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ListFormSelectFromList(TablePtr table, UInt16 row, UInt16 column)
{
    ListPtr  list;
    Int16      index, newIndex;
    RectangleType r;
    MemHandle   recordH;
    LibDBRecordType  record;
    LibDBRecordFlags  bits;
	

    // Retrieve the current record from Librarian's database.
    LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
	
    // Figure out which status column to handle.
    switch (column) {
        case bookStatusColumn:
            list = GetObjectPtr(ListBookStatusList);
            index = record.status.bookStatus;
            break;
			
        case printStatusColumn:
            list = GetObjectPtr(ListPrintStatusList);
            index = record.status.printStatus;
            break;
			
        case formatColumn:
            list = GetObjectPtr(ListFormatStatusList);
            index = record.status.format;
            break;
			
        case readColumn:
            list = GetObjectPtr(ListReadStatusList);
            index = record.status.read;
            break;
			
        default:
            break;
    }

    // Unhighlight the current table selection.
    TblUnhighlightSelection(table);

    // Set the list's selection to the appropriate value.
    LstSetSelection(list, index);
	
    // Position the list.
    TblGetItemBounds(table, row, column, &r);
    LstSetPosition(list, r.topLeft.x, r.topLeft.y);
	
    newIndex = LstPopupList(list);

    // A value of -1 indicates the user dismissed the popup list without
    // making a selection.  In this case, unlock the record and return.
    if ((newIndex == -1) || (newIndex == index) ) {
        MemHandleUnlock(recordH);
        return;
    }
	
    // Update the database record.
    switch (column) {
        case bookStatusColumn:
            record.status.bookStatus = newIndex;
            break;
		
        case printStatusColumn:
            record.status.printStatus = newIndex;
            break;
			
        case formatColumn:
            record.status.format = newIndex;
            break;
		
        case readColumn:
            record.status.read = newIndex;
            break;
			
        default:
            break;
    }
    DirtyRecord(gCurrentRecord);
    bits.allBits = 0;
    // LibChangeRecord unlocks the handle locked in LibGetRecord.
    LibChangeRecord(gLibDB, &gCurrentRecord, &record, bits);
	
    // Make sure the row is redrawn.
    TblMarkRowInvalid(table, row);

    // Send an event that will cause the view to be redrawn.
    FrmUpdateForm(ListForm, updatePopupListChanged);
}


/***********************************************************************
 *
 * FUNCTION:    ListFormSelectRecord
 *
 * DESCRIPTION: Selects (highlights) a record on the table, scrolling to
 *              the record if neccessary.  Also sets the gCurrentRecord.
 *
 * PARAMETERS:  recordNum - record to select
 *                      
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/02/26  Changed code to handle 3.5 masked records.
 *
 ***********************************************************************/
static void ListFormSelectRecord(UInt16 recordNum)
{
    Int16     row, column;
    TablePtr  table;
    UInt16    attr;
	
	
    ErrFatalDisplayIf(recordNum >= DmNumRecords(gLibDB),
                      "Record outside gLibDB");
	
    table = GetObjectPtr(ListTable);
	
    // If a masked record was selected, don't bother showing it.
    if (gPrivateRecordStatus > showPrivateRecords) {
        if ( (! DmRecordInfo(gLibDB, recordNum, &attr, NULL, NULL)) &&
             (attr & dmRecAttrSecret) ) {
            gCurrentRecord = noRecord;
            TblUnhighlightSelection(table);
            return;
        }
    }

    // Don't change anything if the same record is selected
    if (TblGetSelection(table, &row, &column) &&
        recordNum == TblGetRowID(table, row)) {
        return;
    }
	  
    // See if the record is currently visible in the table.
    // A while is used because if TblFindRowID fails we need to
    // call it again to find the row in the reloaded table.
    while (! TblFindRowID(table, recordNum, &row)) {
        if (gPrivateRecordStatus > showPrivateRecords) {
            // If the record is hidden stop trying to show it.
            DmRecordInfo(gLibDB, recordNum, &attr, NULL, NULL);
            if (attr & dmRecAttrSecret) {
                return;
            }
        }
	        
        // Scroll the view down, placing the item on the top row.
        gTopVisibleRecord = recordNum;
	
        // Make sure that gTopVisibleRecord is visible in gCurrentCategory.
        if (gCurrentCategory != dmAllCategories) {
            // Get the category and the secret attribute of the current
            // record.
            DmRecordInfo(gLibDB, gTopVisibleRecord, &attr, NULL, NULL);   
            if ((attr & dmRecAttrCategoryMask) != gCurrentCategory) {
                ErrNonFatalDisplay("Record not in gCurrentCategory");
                gCurrentCategory = (attr & dmRecAttrCategoryMask);
            }
        }
	
        ListFormLoadTable();
        TblRedrawTable(table);
    }
	
    // Select the item
    TblSelectItem(table, row, titleColumn);
    gCurrentRecord = recordNum;
}   
   

/***********************************************************************
 *
 * FUNCTION:    ListFormUpdateDisplay
 *
 * DESCRIPTION: Updates the display of the List view.
 *
 * PARAMETERS:  updateCode - a code that indicates what changes have been
 *                           made to the List view form.
 *                      
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ListFormUpdateDisplay(UInt16 updateCode)
{
    TablePtr table;
    FormPtr  form;
	
	
    table = GetObjectPtr(ListTable);
	
    if (updateCode == frmRedrawUpdateCode) {
        FrmDrawForm(FrmGetActiveForm());
    }
	
    if ( (updateCode & updateRedrawAll) ||
         (updateCode & updatePopupListChanged) ) {
        ListFormLoadTable();
        TblRedrawTable(table);
    }
	
    if ( (updateCode & updateFontChanged) ||
         (updateCode & updateListStatusChanged) ) {
        form = FrmGetActiveForm();
        ListFormInit(form);
        TblRedrawTable(table);
        if (gCurrentRecord != noRecord)
            ListFormSelectRecord(gCurrentRecord);
    }
	
    if (updateCode & updateSelectCurrentRecord &&
        (gCurrentRecord != noRecord) ) {
        ListFormSelectRecord(gCurrentRecord);
    }
	
}


/***********************************************************************
 *
 * FUNCTION:    ListFormUpdateScrollButtons
 *
 * DESCRIPTION: Show or hide the list view scroll buttons, according to
 *              the current contents of the List view's table.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ListFormUpdateScrollButtons(void)
{
    Int16     row;
    UInt16      upIndex, downIndex;
    UInt16      recordNum;
    Boolean   scrollableUp, scrollableDown;
    FormPtr   form;
    TablePtr  table;


    form = FrmGetActiveForm ();
	
    // If the first record displayed is not the first record in the
    // category, enable the scroll up button.
    recordNum = gTopVisibleRecord;
    scrollableUp = SeekRecord(&recordNum, 1, dmSeekBackward);
	
    // Find the record in the last row of the table.
    table = GetObjectPtr(ListTable);
    row = TblGetLastUsableRow(table);
    if (row != -1)
        recordNum = TblGetRowID(table, row);

    // If the last record displayed is not the last record in the
    // category, enable the scroll down button.
    scrollableDown = SeekRecord(&recordNum, 1, dmSeekForward);


    // Update the scroll button.
    upIndex = FrmGetObjectIndex(form, ListScrollUpRepeating);
    downIndex = FrmGetObjectIndex (form, ListScrollDownRepeating);
    FrmUpdateScrollers(form, upIndex, downIndex, scrollableUp,
                       scrollableDown);
}


#pragma mark ----------------

/***********************************************************************
 * A NOTE ON THE NOTE VIEW
 *
 * The Note view in Librarian does not use a form resource contained in
 * Librarian's resources.  Instead, the Note view is based on a shared
 * system resource.  The header file UICommon.h defines constants for
 * the shared Note view form and its constituent UI elements.  The
 * Palm built-in applications also make use of the shared Note form.
 ***********************************************************************/

/***********************************************************************
 *
 * FUNCTION:    CreateNote
 *
 * DESCRIPTION: Make sure there is a note field to edit.  If one doesn't
 *              exist, make one.  
 *
 * PARAMETERS:  none
 *
 * RETURNED:    true if a note field exists to edit, false if not
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean CreateNote(void)
{
    LibDBRecordType  record;
    LibDBRecordFlags  bits;
    MemHandle  recordH;
    Err     error;
	
	
    LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
	
    // Since Librarian edits the note field in place, add a note field
    // if one does not already exist.
    if (! record.fields[libFieldNote]) {
        record.fields[libFieldNote] = "";
        bits.allBits = (UInt32)1 << libFieldNote;
        error = LibChangeRecord(gLibDB, &gCurrentRecord, &record, bits);
        if (error) {
            // The device is full, so a new note field cannot be created.
            MemHandleUnlock(recordH);
            FrmAlert(DeviceFullAlert);
            return false;
        }
    }
    else {
        MemHandleUnlock(recordH);
    }
      
    return true;
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
 *
 *
 ***********************************************************************/
static void DeleteNote(void)
{
    LibDBRecordType  record;
    MemHandle  recordH;
    LibDBRecordFlags  changedField;
    Err     error;
	
	  
    LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
    record.fields[libFieldNote] = NULL;
    changedField.allBits = (UInt32)1 << libFieldNote;
    error = LibChangeRecord(gLibDB, &gCurrentRecord, &record,
                            changedField);
    if (error) {
        MemHandleUnlock(recordH);
        FrmAlert(DeviceFullAlert);
        return;
    }

    // Mark the record dirty.   
    DirtyRecord(gCurrentRecord);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewChangeFont
 *
 * DESCRIPTION: Changes the font in the Note view as a result of the
 *              user selecting a font with the font push buttons.  This
 *              function is only used when running under Palm OS 2.0,
 *              since under 3.0, Librarian uses a menu-activated font
 *              selection dialog.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void NoteViewChangeFont(void)
{
    FormPtr  form;
    UInt16   index;
    FontID   newFont;
	
	
    form = FrmGetActiveForm();
    index = FrmGetControlGroupSelection(form, NoteFontGroup);
	
    // Determine which push button is currently selected.
    if (FrmGetObjectId(form, index) == NoteSmallFontButton)
        newFont = stdFont;
    else
        newFont = largeFont;
		
    // If the font has changed, update the Note view.
    if (newFont != gNoteFont) {
        gNoteFont = newFont;
        FrmUpdateForm(gNoteFormID, updateFontChanged);
    }
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewDeleteNote
 *
 * DESCRIPTION: Prompts the user to delete a note.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    true if the note was deleted, false otherwise
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean NoteViewDeleteNote(void)
{
    FieldPtr field;
	
	
    if (FrmAlert(DeleteNoteAlert) != DeleteNoteYes)
        return (false);
	  
    // Unlock the handle that contains the text of the memo.
    field = GetObjectPtr(NoteField);
    ErrFatalDisplayIf((! field), "Bad field");
	
    // Clear the handle value in the field, otherwise the handle
    // will be free when the form is disposed of.  This call also 
    // unlocks the handle the contains the note string.
    FldCompactText(field);
    FldSetTextHandle(field, 0);   
	
    DeleteNote();
	
    return (true);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewDoCommand
 *
 * DESCRIPTION: Handles menu commands in the Note view.
 *
 * PARAMETERS:  command - menu item id
 *
 * RETURNED:    true if the command was handled, false if not
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean NoteViewDoCommand(UInt16 command)
{
    FieldPtr  field;
    Boolean   handled = true;
	
	
    // The Note view menus are slightly different depending on the
    // version of the OS; version 2.0 does not have a font selection
    // menu item, and 3.5 does away with the top of page and bottom of
    // page commands.  The constants for these are all slightly different,
    // too, requiring different case statements.
	
    // Version 3.5+ menus
    if (gROMVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {
        switch (command) {
            case newNoteFontCmd:
                gNoteFont = SelectFont(gNoteFont);
                break;
				
            case newNotePhoneLookupCmd:
                field = GetObjectPtr(NoteField);
                PhoneNumberLookup(field);
                break;
				
            default:
                handled = false;
        }
    }
	
    // Version 3.0-3.3 menus
    else if (gROMVersion >= sysMakeROMVersion(3,0,0,sysROMStageRelease,0)) {
        switch (command) {
            case noteFontCmd:
                gNoteFont = SelectFont(gNoteFont);
                break;
			
            case noteTopOfPageCmd:
                field = GetObjectPtr(NoteField);
                FldSetScrollPosition(field, 0);
                NoteViewUpdateScrollBar();
                break;
	
            case noteBottomOfPageCmd:
                field = GetObjectPtr(NoteField);
                FldSetScrollPosition(field, FldGetTextLength(field));
                NoteViewUpdateScrollBar();
                break;
	
            case notePhoneLookupCmd:
                field = GetObjectPtr(NoteField);
                PhoneNumberLookup(field);
                break;
				
            default:
                handled = false;
        }
    }
	
    // Version 2.0 menus
    else {
        switch (command) {
            case noteTopOfPageCmdV20:
                field = GetObjectPtr(NoteField);
                FldSetScrollPosition(field, 0);
                NoteViewUpdateScrollBar();
                break;
				
            case noteBottomOfPageCmdV20:
                field = GetObjectPtr(NoteField);
                FldSetScrollPosition(field, FldGetTextLength(field));
                NoteViewUpdateScrollBar();
                break;
	
            case notePhoneLookupCmdV20:
                field = GetObjectPtr(NoteField);
                PhoneNumberLookup(field);
                break;
				
            default:
                handled = false;
        }
    }
	
    return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewDrawTitle
 *
 * DESCRIPTION: Draw the title of the note view.  The title should be
 *              the same as the title field of the book record.
 *
 * PARAMETERS:  form - pointer to the note view form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void NoteViewDrawTitle(FormPtr form)
{
    Coord    x, y;
    Err      error;
    LibDBRecordType  record;
    MemHandle  recordH;
    Char     *name;
    Coord    length, width;
    Boolean  ignored;
    Coord    nameExtent;
    Coord    formWidth;
    IndexedColorType  curForeColor, curBackColor, curTextColor;
    FontID   curFont;
    RectangleType  r, eraseRect, drawRect;
    UInt8    *lockedWindow = NULL;
	
	
    // The distinctive look of the note view in the ROM applications is
    // not a property of the NoteView or NewNoteView form resources
    // stored in ROM.  Rather, it is achieved by application code,
    // drawing an inverted rounded rectangular title bar over the top
    // of the regular-shaped title bar.

    // Different action is required depending on the version of the Palm
    // OS Librarian is running on.  Prior to 3.5, drawing the note
    // view's title bar is very simple, since color is only one bit.
    // With the introduction of up to 8-bit color in Palm OS 3.5, this
    // routine must make sure to draw the title so it matches the
    // current Form title colors, whatever they may be.
    if (gROMVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {
        // Lock the screen during drawing to avoid the mess that occurs
        // when the system draws the form's title, then Librarian draws
        // the special note view title over that.
        lockedWindow = WinScreenLock(winLockCopy);
    }
	
    FrmDrawForm(form);
	
    FrmGetFormBounds(form, &r);
    formWidth = r.extent.x;
    x = 2;
    y = 1;
    nameExtent = formWidth - 4;
   
    RctSetRectangle(&eraseRect, 0, 0, formWidth, (FntLineHeight() + 4) );
    RctSetRectangle(&drawRect, 0, 0, formWidth, (FntLineHeight() + 2) );
	
    // On 3.5, Save and set the window colors.  This is necessary
    // because the FrmDrawForm call above sets the foreground and
    // background colors to the colors used for drawing the form's UI,
    // but NoteViewDrawTitle needs to draw in the colors used for the
    // form's title bar.
    if (gROMVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {
        curForeColor = WinSetForeColor(UIColorGetTableEntryIndex(UIFormFrame));
        curBackColor = WinSetBackColor(UIColorGetTableEntryIndex(UIFormFill));
        curTextColor = WinSetTextColor(UIColorGetTableEntryIndex(UIFormFrame));
    }

    // Save the current font and set the font to bold for drawing the
    // title.
    curFont = FntSetFont(boldFont);
	
    // Clear the title area and draw the note view title bar.
    WinEraseRectangle(&eraseRect, 0);
    WinDrawRectangle(&drawRect, 3);

    error = LibGetRecord(gLibDB, gCurrentRecord, &record, &recordH);
    ErrNonFatalDisplayIf((error), "Record not found");
    if (error) return;

    // Retrieve record title, or the untitled string if the record has
    // no title.
    GetTitleString(&record, &name, &length, &width, &gNoTitleRecordString);

    // Find out how much of the title string will fit.  If all of the
    // string fits, center it before drawing.  Since FntCharsInWidthName
    // changes the width value it receives to the actual width allowed
    // in a given space, pass a copy of nameExtent instead of the real
    // thing, since we're interested in using nameExtent later.
    width = nameExtent;
    FntCharsInWidth(name, &width, &length, &ignored);
    if (nameExtent > width)
        x += (nameExtent - width) / 2;
	
    // Draw the portion of the title string that fits.
    WinDrawInvertedChars(name, length, x, y);

    // Unlock the record that LibGetRecord locked.
    MemHandleUnlock(recordH);
	
    // Everything is drawn, so on OS 3.5, it is now time to unlock the
    // form and toss the whole mess back onto the screen.
    if (lockedWindow)
        WinScreenUnlock();
	
    // Restore the colors to their original settings.
    if (gROMVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {
        WinSetForeColor(curForeColor);
        WinSetBackColor(curBackColor);
        WinSetTextColor(curTextColor);
    }
	
    // Restore the font.
    FntSetFont(curFont);
}
 

/***********************************************************************
 *
 * FUNCTION:    NoteViewHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the Librarian's
 *              Note view.
 *
 * PARAMETERS:  event - event to be handled
 *
 * RETURNED:    true if the event was handled, false otherwise
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean NoteViewHandleEvent(EventPtr event)
{
    FormPtr   form;
    Boolean   handled = false;
    FieldPtr  field;
	
	
    switch (event->eType) {
        case keyDownEvent:
            if (TxtCharIsHardKey(event->data.keyDown.modifiers,
                                 event->data.keyDown.chr)) {
                NoteViewSave();
                FrmGotoForm(ListForm);
                return true;
            } 

            switch (event->data.keyDown.chr) {
                case pageUpChr:
                    NoteViewPageScroll(winUp);
                    handled = true;
                    break;
					
                case pageDownChr:
                    NoteViewPageScroll(winDown);
                    handled = true;
                    break;
					
                default:
                    break;
            }
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID) {
                case NoteDoneButton:
                    NoteViewSave();
               
                    // When returning to the List view, highlight this
                    // record.
                    if (gPriorFormID == ListForm)
                        gListFormSelectThisRecord = gCurrentRecord;
					
                    FrmGotoForm(gPriorFormID);
                    handled = true;
                    break;

                case NoteDeleteButton:
                    if (NoteViewDeleteNote())
                        FrmGotoForm(gPriorFormID);
               
                    gListFormSelectThisRecord = noRecord;
                    handled = true;
                    break;
               
                case NoteSmallFontButton:
                case NoteLargeFontButton:
                    NoteViewChangeFont();
                    handled = true;
                    break;
				
                default:
                    break;
            }
            break;

        case fldChangedEvent:
            NoteViewUpdateScrollBar();
            handled = true;
            break;
      
        case menuEvent:
            return NoteViewDoCommand(event->data.menu.itemID);
      
        case frmOpenEvent:
            form = FrmGetActiveForm ();
            NoteViewInit(form);
            // Draw the note view's title bar.  NoteViewDrawTitle
            // includes a call to FrmDrawForm, so it's not needed here.
            NoteViewDrawTitle(form);
            NoteViewUpdateScrollBar();
            FrmSetFocus(form, FrmGetObjectIndex(form, NoteField));
            handled = true;
            break;

        case frmGotoEvent:
            form = FrmGetActiveForm();
            gCurrentRecord = event->data.frmGoto.recordNum;
            NoteViewInit(form);
            field = GetObjectPtr(NoteField);
            FldSetScrollPosition(field, event->data.frmGoto.matchPos);
            FldSetSelection(field, event->data.frmGoto.matchPos, 
                            event->data.frmGoto.matchPos +
                            event->data.frmGoto.matchLen);
            NoteViewDrawTitle(form);
            NoteViewUpdateScrollBar();
            FrmSetFocus(form, FrmGetObjectIndex(form, NoteField));
            handled = true;
            break;
      
        case frmUpdateEvent:
            if (event->data.frmUpdate.updateCode & updateFontChanged) {
                field = GetObjectPtr(NoteField);
                FldSetFont(field, gNoteFont);
                NoteViewUpdateScrollBar();
                handled = true;
            }
            else {
                form = FrmGetActiveForm();
                NoteViewDrawTitle(form);
                handled = true;
            }
            break;

        case frmCloseEvent:
            // Unlock handle to untitled record string.
            if (gNoTitleRecordString) {
                MemPtrUnlock(gNoTitleRecordString);
                gNoTitleRecordString = NULL;
            }

            if (FldGetTextHandle(GetObjectPtr(NoteField)))
                NoteViewSave();
            break;

        case sclRepeatEvent:
            NoteViewScroll(event->data.sclRepeat.newValue - 
                           event->data.sclRepeat.value);
            break;
      
        default:
            break;
    }

    return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewInit
 *
 * DESCRIPTION: Initializes the Note view form.
 *
 * PARAMETERS:  form - pointer to the Note view form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void NoteViewInit(FormPtr form)
{
    FieldPtr       field;
    FieldAttrType  attr;
	
	
    NoteViewLoadRecord();
	
    // The NoteForm resource comes with push buttons for font selection,
    // both stdFont and boldFont.  Since Librarian can also use the
    // largeBoldFont in Palm OS 3.0 and later, and the application also
    // has its own font selection dialog available from a menu item,
    // NoteViewInit hides the push buttons when running on version 3.0
    // or later.  On 2.0, NoteViewInit initializes the push buttons to
    // display the current Note view font.  However, starting with version
    // 3.5, Librarian uses the NewNoteView form, which does not have any
    // font push buttons.
    if (gROMVersion < sysMakeROMVersion(3,0,0,sysROMStageRelease,0)) {
        if (gNoteFont == stdFont)
            FrmSetControlGroupSelection(form, NoteFontGroup,
                                        NoteSmallFontButton);
        else
            FrmSetControlGroupSelection(form, NoteFontGroup,
                                        NoteLargeFontButton);
    }
	
    else if (gROMVersion < sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {
        FrmHideObject(form, FrmGetObjectIndex(form,
                                              NoteSmallFontButton));
        FrmHideObject(form, FrmGetObjectIndex(form,
                                              NoteLargeFontButton));
    }
	
    // Set the field to send events to maintain the scroll bar.
    field = GetObjectPtr(NoteField);
    FldGetAttributes(field, &attr);
    attr.hasScrollBar = true;
    FldSetAttributes(field, &attr);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewLoadRecord
 *
 * DESCRIPTION: Load the record's note field into the Note view's field
 *              object for editing in place.  The note field is too big
 *              (4K) to edit in the heap.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void NoteViewLoadRecord(void)
{
    FieldPtr   field;
    LibPackedDBRecord  *packed;
    MemHandle  packedH;
    Char       *ptr;
    UInt16     offset;
	
	
    // Get a pointer to the memo field.
    field = GetObjectPtr(NoteField);
	
    // Set the font used in the memo field.
    FldSetFont(field, gNoteFont);
	
    // Retrieve the note field from the current database record.
    // Librarian calls CreateNote before getting to NoteViewLoadRecord,
    // which guarantees that the note field already exists.
    packedH = DmQueryRecord(gLibDB, gCurrentRecord);
    ErrFatalDisplayIf((! packedH), "Bad record");
    packed = MemHandleLock(packedH);
	
    // Set a pointer to the location of the note field, using the note
    // field offset stored in the packed database record.
    ptr = &packed->firstField;
    ptr += packed->noteOffset;
	
    // Calculate the offset of the note field from the front of the
    // packed database record, not from the beginning of the first
    // field, since FldSetText wants the offset from the start of the
    // record's memory chunk.
    offset = ptr - (char *)packed;
	
    // Set the note field text to the contents of the note field.
    FldSetText(field, packedH, offset, StrLen(ptr) + 1);
	
    MemHandleUnlock(packedH);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewPageScroll
 *
 * DESCRIPTION: This routine scrolls the note field a page up or down.
 *
 * PARAMETERS:  direction - direction to scroll; up or down
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void NoteViewPageScroll(WinDirectionType direction)
{
    Int16     value;
    Int16     min, max;
    Int16     pageSize;
    UInt16    linesToScroll;
    FieldPtr  field;
    ScrollBarPtr  bar;
	
    field = GetObjectPtr (NoteField);
	
    if (FldScrollable(field, direction))  {
        linesToScroll = FldGetVisibleLines(field) - 1;
        FldScrollField(field, linesToScroll, direction);
		
        // Update the scroll bar.
        bar = GetObjectPtr(NoteScrollBar);
        SclGetScrollBar(bar, &value, &min, &max, &pageSize);
		
        if (direction == winUp)
            value -= linesToScroll;
        else
            value += linesToScroll;
      
        SclSetScrollBar(bar, value, min, max, pageSize);
        return;
    }
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewSave
 *
 * DESCRIPTION: Saves the note field to a database record if the user
 *              changed the contents of the field.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void NoteViewSave(void)
{
    FieldPtr  field;
    UInt16    length;
	
	
    field = GetObjectPtr(NoteField);
	
	
    // If the field wasn't modified then don't do anything.
    if (FldDirty(field)) {      
        // Release any free space in the note field.
        FldCompactText(field);
        DirtyRecord(gCurrentRecord);
    }

    length = FldGetTextLength(field);
	
    // Clear the handle value in the field, otherwise the handle
    // will be free when the form is disposed of.  This call also unlocks
    // the handle that contains the note string.
    FldSetTextHandle(field, 0);
	
    // Empty fields are not allowed because they cause problems.
    if (length == 0)
        DeleteNote();
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewScroll
 *
 * DESCRIPTION: Scrolls the Note view's text field a number of lines up
 *              or down.
 *
 * PARAMETERS:  linesToScroll - number of lines to scroll; positive
 *              values indicate scrolling down, negative values scrolling
 *              up.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void NoteViewScroll(Int16 linesToScroll)
{
    UInt16    blankLines;
    Int16     min, max;
    Int16     value;
    Int16     pageSize;
    FieldPtr  field;
    ScrollBarPtr  bar;
	
    field = GetObjectPtr(NoteField);
	
    // Negative value; scroll up.
    if (linesToScroll < 0) {
        blankLines = FldGetNumberOfBlankLines(field);
        FldScrollField(field, -linesToScroll, winUp);
		
        // If there were blank lines visible at the end of the field
        // then we need to update the scroll bar.
        if (blankLines) {
            // Update the scroll bar.
            bar = GetObjectPtr(NoteScrollBar);
            SclGetScrollBar(bar, &value, &min, &max, &pageSize);
            if (blankLines > -linesToScroll)
                max += linesToScroll;
            else
                max -= blankLines;
            SclSetScrollBar(bar, value, min, max, pageSize);
        }
    }

    // Positive value; scroll down.
    else if (linesToScroll > 0)
        FldScrollField(field, linesToScroll, winDown);
}


/***********************************************************************
 *
 * FUNCTION:    NoteViewUpdateScrollBar
 *
 * DESCRIPTION: Updates the scroll bar in the Note view.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void NoteViewUpdateScrollBar(void)
{
    UInt16      scrollPos;
    UInt16      textHeight;
    UInt16      fieldHeight;
    Int16     maxValue;
    FieldPtr  field;
    ScrollBarPtr  bar;
	
    field = GetObjectPtr(NoteField);
    bar = GetObjectPtr(NoteScrollBar);
	
    FldGetScrollValues(field, &scrollPos, &textHeight, &fieldHeight);
	
    if (textHeight > fieldHeight)
        maxValue = textHeight - fieldHeight;
    else if (scrollPos)
        maxValue = scrollPos;
    else
        maxValue = 0;
	
    SclSetScrollBar(bar, scrollPos, 0, maxValue, fieldHeight - 1);
}


#pragma mark ----------------

/***********************************************************************
 *
 * FUNCTION:    PrefsFormHandleEvent
 *
 * DESCRIPTION: Event handler for the Preferences dialog.
 *
 * PARAMETERS:  event - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean PrefsFormHandleEvent(EventPtr event)
{
    Boolean handled = false;
    FormPtr form;


    switch (event->eType) {
        case frmOpenEvent:
            form = FrmGetActiveForm();
            PrefsFormInit(form);
            FrmDrawForm (form);
            handled = true;
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID) {
                case PrefsOKButton:
                    form = FrmGetActiveForm();
                    PrefsFormSave(form);
                    FrmEraseForm(form);
                    FrmDeleteForm(form);
                    FrmSetActiveForm(FrmGetFirstForm());
                    //FrmReturnToForm(0);
                    handled = true;
                    break;
			
                case PrefsCancelButton:
                    form = FrmGetActiveForm();
                    FrmEraseForm(form);
                    FrmDeleteForm(form);
                    FrmSetActiveForm(FrmGetFirstForm());
                    //FrmReturnToForm(0);
                    handled = true;
                    break;
					
                default:
                    break;
            }
            break;
		
        default:
            break;
    }
	
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    PrefsFormInit
 *
 * DESCRIPTION: This routine initializes the Preferences form.
 *
 * PARAMETERS:  frm - pointer to the PrefsForm form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void PrefsFormInit(FormPtr form)
{
    UInt16    selected;


    // Set the push buttons to show the current sort order.
    switch (gShowInList) {
        case libShowAuthorTitle:
            selected = PrefsShowAuthorTitlePushButton;
            break;
		
        case libShowTitleAuthor:
            selected = PrefsShowTitleAuthorPushButton;
            break;
			
        case libShowTitleOnly:
            selected = PrefsShowTitleOnlyPushButton;
            break;
			
        default:
            break;
    }
    FrmSetControlGroupSelection(form, PrefsShowInListGroup, selected);
	
    // Set the check boxes.
    CtlSetValue(FrmGetObjectPtr(form, FrmGetObjectIndex(form,
                                                        PrefsShowBookStatusCheckbox)), gShowBookStatus);
    CtlSetValue(FrmGetObjectPtr(form, FrmGetObjectIndex(form,
                                                        PrefsShowPrintStatusCheckbox)), gShowPrintStatus);
    CtlSetValue(FrmGetObjectPtr(form, FrmGetObjectIndex(form,
                                                        PrefsShowFormatCheckbox)), gShowFormat);
    CtlSetValue(FrmGetObjectPtr(form, FrmGetObjectIndex(form,
                                                        PrefsShowReadUnreadCheckbox)), gShowReadUnread);

}


/***********************************************************************
 *
 * FUNCTION:    PrefsFormSave
 *
 * DESCRIPTION: Saves the values entered in the Preferences dialog.
 *
 * PARAMETERS:  form - pointer to the PrefsForm form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void PrefsFormSave(FormPtr form)
{
    FormPtr  curForm, sortForm;
    UInt8     index;
    UInt8     newShowInList;
    Boolean  oldShowBookStatus, oldShowPrintStatus, oldShowFormat,
        oldShowReadUnread;
    Boolean  sortChanged = false;
    Boolean  statusChanged = false;
	
	
    // Save original global values.
    oldShowBookStatus = gShowBookStatus;
    oldShowPrintStatus = gShowPrintStatus;
    oldShowFormat = gShowFormat;
    oldShowReadUnread = gShowReadUnread;
	
    // Save checkbox values.
    gShowBookStatus = CtlGetValue(FrmGetObjectPtr(form,
                                                  FrmGetObjectIndex(form, PrefsShowBookStatusCheckbox)));
    gShowPrintStatus = CtlGetValue(FrmGetObjectPtr(form,
                                                   FrmGetObjectIndex(form, PrefsShowPrintStatusCheckbox)));
    gShowFormat = CtlGetValue(FrmGetObjectPtr(form,
                                              FrmGetObjectIndex(form, PrefsShowFormatCheckbox)));
    gShowReadUnread = CtlGetValue(FrmGetObjectPtr(form,
                                                  FrmGetObjectIndex(form, PrefsShowReadUnreadCheckbox)));
		
    // Save database sort order, and if necessary, resort the database.
    index = FrmGetControlGroupSelection(form, PrefsShowInListGroup);
    switch (FrmGetObjectId(form, index)) {
        case PrefsShowAuthorTitlePushButton:
            newShowInList = libShowAuthorTitle;
            break;
			
        case PrefsShowTitleAuthorPushButton:
            newShowInList = libShowTitleAuthor;
            break;
			
        case PrefsShowTitleOnlyPushButton:
            newShowInList = libShowTitleOnly;
            break;
			
        default:
            break;
    }
	
    // If the user has changed the Show In List push button selection,
    // resort the database.
    if (newShowInList != gShowInList) {
        // Display a "Sorting..." dialog so the user knows why the
        // device is locked up while Librarian resorts its database.
        curForm = form;
        sortForm = FrmInitForm(SortForm);
        FrmSetActiveForm(sortForm);
        FrmDrawForm(sortForm);
		
        // Sort the database.
        LibChangeSortOrder(gLibDB, newShowInList);
        gShowInList = newShowInList;
        gCurrentRecord = noRecord;
        gTopVisibleRecord = 0;
        sortChanged = true;
		
        // Remove the "Sorting..." dialog.
        FrmEraseForm(sortForm);
        FrmDeleteForm(sortForm);
        FrmSetActiveForm(curForm);
    }
	
    // Update the display if the status check boxes have been altered.
    if ( (gShowBookStatus != oldShowBookStatus) ||
         (gShowPrintStatus != oldShowPrintStatus) ||
         (gShowFormat != oldShowFormat) ||
         (gShowReadUnread != oldShowReadUnread) ) {
        statusChanged = true;
    }
	
    // Send an appropriate frmUpdateEvent to the List form if there were
    // any changes.
    if (statusChanged)
        FrmUpdateForm(ListForm, updateListStatusChanged);
    else if (sortChanged)
        FrmUpdateForm(ListForm, updateRedrawAll);
}


#pragma mark ----------------

/***********************************************************************
 *
 * FUNCTION:    RecordFormAddField
 *
 * DESCRIPTION: Adds a field to the gRecordFormLines info. 
 *
 * PARAMETERS:  fieldNum  -> field to add
 *              width    <-> width already occupied on the line; set to
 *                           the width of the last line added
 *              maxWidth  -> maximum width allowed per line
 *
 * RETURNED:    width is set to the width of the last line added
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormAddField(UInt16 fieldNum, UInt16 *width, UInt16 maxWidth)
{
    UInt16     length;
    UInt16     offset = 0;
    UInt16     newOffset;
    char * str;
    LibAppInfoType  *appInfo;
	
	
    // Don't add this field if already past the last line allowed.
    if (gRecordFormLastLine > recordFormLinesMax)
        return;
	
    // If handling the title or author fields, or the status
    // strings, continue, since these fields always contain data.
    // Otherwise, check to see if the field exists; fields that
    // do not exist in the current record should be skipped.  The title
    // and author fields, as well as the status strings, will always
    // contain something (even if just the "-Untitled-" or "-No author-"
    // strings), so they should always be added.
    switch (fieldNum) {
        case libFieldTitle:
        case libFieldLastName:
        case recordFormBookStatusLine:
        case recordFormPrintStatusLine:
        case recordFormFormatStatusLine:
        case recordFormReadStatusLine:
            break;
			
        default:
            if (gRecordFormRecord.fields[fieldNum] == NULL)
                return;
            break;
    }
	
    // Retrieve Librarian's application info block.
    appInfo = MemHandleLock(LibGetAppInfo(gLibDB));
	
    // If we're past the maxWidth already then start at the beginning
    // of the next line.
    if (*width >= maxWidth)
        *width = 0;
	  
    // Check to see if the field to add is the title, author, or printing
    // field, or one of the status strings.  Since the text drawn in the
    // Record view for these strings is not necessarily the text stored in
    // the record (title might be "-Untitled-"; author is either a
    // concatenation of the record's lastName and firstName fields, or
    // "-No author-"; printing might contain "th printing" at the end of
    // its string; the status strings are in the application info block,
    // not the record), these fields require special handling.
    switch (fieldNum) {
        case libFieldTitle:
            str = gRecordFormTitleString;
            break;
			
        case libFieldLastName:
            str = gRecordFormAuthorString;
            break;
			
        case libFieldPrinting:
            str = gRecordFormPrintingString;
            break;
		
        case recordFormBookStatusLine:
            str = appInfo->bookStatusStrings[
                gRecordFormRecord.status.bookStatus];
            break;
			
        case recordFormPrintStatusLine:
            str = appInfo->printStatusStrings[
                gRecordFormRecord.status.printStatus];
            break;
			
        case recordFormFormatStatusLine:
            str = appInfo->formatStatusStrings[
                gRecordFormRecord.status.format];
            break;
		
        case recordFormReadStatusLine:
            str = appInfo->readStatusStrings[
                gRecordFormRecord.status.read];
            break;
		
        default:
            str = gRecordFormRecord.fields[fieldNum];
            break;
    }
	
    do {
        // Don't exceed the maximum number of lines.
        if (gRecordFormLastLine >= recordFormLinesMax)
            break;
	  
        // Check if the field word wrapped in the middle of a word that
        // could fit on the next line.  The criteria that must be met are:
        //   1. The field must have enough text left to wrap.  The first
        //      part of the if statement makes sure we haven't stopped in
        //      the middle of the line by running out of text in the field.
        //   2. The last character in the text that fits on this line must
        //      not be whitespace.  If it is, we're stopped after a word
        //      break.  The second part of the if statement checks for a
        //      whitespace character at the end of the text that fits this
        //      line.
        //   3. There must not be any width remaining on this line.  The
        //      third part of the if checks if this line wasn't as wide as
        //      it could be because some other text used up the space.
        length = FldWordWrap(&str[offset], maxWidth - *width);
        if (str[offset + length] != '\0'
            && ! TxtCharIsSpace(str[offset + length - 1])
            && (*width > 0)) {
            length = 0;  // Don't word wrap; try the next line.
        }
	
        // Lines returned from FldUInt16Wrap may include a '\n' at the
        // end.  If '\n' is present, remove it to keep it from being drawn.
        // The alternative is to not draw linefeeds at draw time.  That
        // seems more complex (there are many calls to WinDrawChars), and
        // slower as well.  This way is faster but makes catching word
        // wrapping problems less obvious, since length == 0 also happens
        // when word wrap fails.
        newOffset = offset + length;
        if (newOffset > 0 && str[newOffset - 1] == linefeedChr)
            length--;
	    
        gRecordFormLines[gRecordFormLastLine].fieldNum = fieldNum;
        gRecordFormLines[gRecordFormLastLine].offset = offset;
        gRecordFormLines[gRecordFormLastLine].x = *width;
        gRecordFormLines[gRecordFormLastLine].length = length;
        gRecordFormLastLine++;
        offset = newOffset;
	        
        // If there is still more text to draw, we've run out of room on
        // the current line.  Wrap to the start of the next line.  This
        // also wraps if we've encountered a linefeed character.
        if (str[offset] != '\0')
            *width = 0;
        else
            break;
    } while (true);
	
    // If the last character was a new line then there is no width.
    // Otherwise the width is the width of the characters on the last line.
    if (str[offset - 1] == linefeedChr)
        *width = 0;
    else
        *width += FntCharsWidth(&str[gRecordFormLines[
            gRecordFormLastLine - 1].offset], 
                                gRecordFormLines[gRecordFormLastLine - 1].length);
	
    MemPtrUnlock(appInfo);
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordFormAddSpaceForText
 *
 * DESCRIPTION: Adds space for text to the RecordFormLines info. 
 *
 * PARAMETERS:  string  -> pointer to text to leave space for
 *              width  <-> width already occupied on the line; set to the
 *                         new width occupied
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormAddSpaceForText(char *string, UInt16 *width)
{
    *width += FntCharsWidth(string, StrLen(string));
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormCalcNextLine
 *
 * DESCRIPTION: Calculate how far to advance to the next line.  A blank
 *              line or text which begins to the left of text on the
 *              current line advance the line down.  Multiple blank lines
 *              in succession advance the line down half a line at a time.
 *
 * PARAMETERS:  i - the line to base how far to advance
 *              oneLine - the amount which advance one line down
 *
 * RETURNED:    the amount to advance; typically oneLine or oneLine / 2.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt16 RecordFormCalcNextLine(UInt16 i, UInt16 oneLine)
{
    // Advance down if the text starts before the text of the current line.
    if (gRecordFormLines[i].x == 0 || 
        (i > 0 && (gRecordFormLines[i].x <= gRecordFormLines[i - 1].x ||
                   gRecordFormLines[i - 1].fieldNum == recordFormBlankLine
                   )
         )
        ) {
        // A non blank line moves down a full line.
        if (gRecordFormLines[i].fieldNum != recordFormBlankLine)
            return oneLine;
        else
            // A recordFormBlankLine is half-height.
            return oneLine / 2;
    }
	
    return 0;  // Stay on the same line.
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordFormCleanup
 *
 * DESCRIPTION: Cleans up global variables in Record view prior to
 *              closing the Record form or firing up a beam operation.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormCleanup(void)
{
    // Dispose of the Record view's copy of the current record.
    if (gRecordFormRecordH) {
        MemHandleUnlock(gRecordFormRecordH);
        gRecordFormRecordH = NULL;
    }
	
    if (gRecordFormLines) {
        MemHandleFree(MemPtrRecoverHandle(gRecordFormLines));
        gRecordFormLines = NULL;
    }
	
    // If there is still a handle to a concatenated author string,
    // unlock it and reset the pointer.
    if (gRecordFormAuthorConcatenated) {
        MemHandleFree(MemPtrRecoverHandle(gRecordFormAuthorString));
        gRecordFormAuthorString = NULL;
        gRecordFormAuthorConcatenated = false;
    }
	
    // If new memory was allocated to hold the printing string,
    // unlock it.
    if (gRecordFormPrintingLocked) {
        MemHandleFree(MemPtrRecoverHandle(
            gRecordFormPrintingString));
        gRecordFormPrintingString = NULL;
        gRecordFormPrintingLocked = false;
    }
	
    // Unlock resource strings for unnamed items.
    if (gNoTitleRecordString) {
        MemPtrUnlock(gNoTitleRecordString);
        gNoTitleRecordString = NULL;
    }
    if (gNoAuthorRecordString) {
        MemPtrUnlock(gNoAuthorRecordString);
        gNoAuthorRecordString = NULL;
    }
	
    // Reset pointers to the title and author strings.
    gRecordFormTitleString = NULL;
    gRecordFormAuthorString = NULL;
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormDoCommand
 *
 * DESCRIPTION: Performs the specified menu command.
 *
 * PARAMETERS:  command - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/02/20  Menu code moved to HandleCommonMenus to support
 *             pilrc-generated shared menu resources.
 *
 ***********************************************************************/
static Boolean RecordFormDoCommand(UInt16 command)
{
    Boolean  handled = false;


    if (HandleCommonMenus(command))
        handled = true;

    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormDraw
 *
 * DESCRIPTION: Draws the Record view, selecting text if necessary as
 *              the result of a find operation.  Pass in zero (0) for
 *              all three parameters if selected text is not desired.
 *              Also updates the Record view scroll buttons.
 *
 * PARAMETERS:  selectFieldNum - field to show selected text
 *              selectPos - offset into field for start of selected text
 *              selectLen - length of selected text.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormDraw (UInt16 selectFieldNum, UInt16 selectPos,
                            UInt16 selectLen)
{
    LibAppInfoType  *appInfo;
    UInt16     y;
    UInt16     i;
    FontID   curFont;
    FormPtr  form;
    UInt16     upIndex;
    UInt16     downIndex;
    Boolean  scrollableUp, scrollableDown;
    RectangleType  r;
    int      bottomOfRecordFormDisplay;
    char * str = NULL;
	
	
    appInfo = MemHandleLock(LibGetAppInfo(gLibDB));
	
    form = FrmGetActiveForm();
    FrmGetObjectBounds(form, FrmGetObjectIndex(form, RecordDisplayGadget),
                       &r);
    bottomOfRecordFormDisplay = r.topLeft.y + r.extent.y;
	
    if (gTopRecordFormLine < gRecordFormFirstPlainLine) {
        // Use largeBoldFont on Palm OS 3.0 and later; use largeFont on 2.0.
        if (gROMVersion >= sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
            curFont = FntSetFont(largeBoldFont);
        else
            curFont = FntSetFont(largeFont);
    }
    else
        curFont = FntSetFont(gRecordFont);
	
    y = r.topLeft.y - RecordFormCalcNextLine(gTopRecordFormLine,
                                             FntLineHeight());
	
    for (i = gTopRecordFormLine; i < gRecordFormLastLine; i++) {
        // This must be done before the font shrinks or else
        // we move down less to the next row and overwrite the 
        // descenders of the last row that used a large font.
        y += RecordFormCalcNextLine(i, FntLineHeight());
		
        if (i == gRecordFormFirstPlainLine)
            FntSetFont(gRecordFont);
		
        // If we are past the bottom stop drawing.
        if (y > bottomOfRecordFormDisplay - FntLineHeight())
            break;
		
        ErrNonFatalDisplayIf(y < r.topLeft.y, "Drawing record out of gadget");
	     
        // If the offset for this field is 0, retrieve the string
        // containing this field's text.  Set the string to draw to the
        // appropriate value, depending on what kind of field should be
        // drawn.
        if (gRecordFormLines[i].offset == 0 || str == NULL) {
            switch (gRecordFormLines[i].fieldNum) {
                case recordFormBlankLine:
                    break;
		
                case libFieldTitle:
                    str = gRecordFormTitleString;
                    break;
		        	
                case libFieldLastName:
                    str = gRecordFormAuthorString;
                    break;
		        	
                case libFieldYear:
                    WinDrawChars("©", 1, gRecordFormLines[i].x -
                                 FntCharsWidth("©", 1), y);
                    str = gRecordFormRecord.fields[
                        gRecordFormLines[i].fieldNum];
                    break;
		        	
                case libFieldPublisher:
                    if (gRecordFormLines[i].x > 0)
                        WinDrawChars(", ", 2, gRecordFormLines[i].x -
                                     FntCharsWidth(", ", 2), y);
                    str = gRecordFormRecord.fields[
                        gRecordFormLines[i].fieldNum];
                    break;
					
                case libFieldPrinting:
                    str = gRecordFormPrintingString;
                    break;
		        
                case recordFormBookStatusLine:
                    str = appInfo->bookStatusStrings[
                        gRecordFormRecord.status.bookStatus];
                    break;
		        
                case recordFormPrintStatusLine:
                    str = appInfo->printStatusStrings[
                        gRecordFormRecord.status.printStatus];
                    break;
		        
                case recordFormFormatStatusLine:
                    str = appInfo->formatStatusStrings[
                        gRecordFormRecord.status.format];
                    break;
		        
                case recordFormReadStatusLine:
                    str = appInfo->readStatusStrings[
                        gRecordFormRecord.status.read];
                    break;
		        
                default:
                    str = gRecordFormRecord.fields[
                        gRecordFormLines[i].fieldNum];
                    break;
            }
        }
		
        // Draw the text for this particular line.  If this line is blank,
        // don't draw anything.
        if (gRecordFormLines[i].fieldNum != recordFormBlankLine) {
            WinDrawChars(&str[gRecordFormLines[i].offset],
                         gRecordFormLines[i].length, gRecordFormLines[i].x, y);
        }
		
        // Highlight text if it is within the selection bounds.  This is
        // used to select found text.
        if (gRecordFormLines[i].fieldNum == selectFieldNum &&
            selectLen > 0) {
            RecordFormDrawSelectedText(i, selectPos, selectLen, y);
        }
    }
	           
    MemPtrUnlock(appInfo);
    FntSetFont(curFont);
	
    // Update the scroll buttons.
    form = FrmGetActiveForm();
	
    scrollableUp = gTopRecordFormLine != 0;
    scrollableDown = i < gRecordFormLastLine; 
	
    upIndex = FrmGetObjectIndex(form, RecordScrollUpRepeating);
    downIndex = FrmGetObjectIndex(form, RecordScrollDownRepeating);
    FrmUpdateScrollers(form, upIndex, downIndex, scrollableUp,
                       scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormDrawSelectedText
 *
 * DESCRIPTION: Inverts text that is considered selected.
 *
 * PARAMETERS:  currentField - field containing the selected text
 *              selectPos - offset into field for start of selected text
 *              selectLen - length of selected text
 *              textY - where on the screen the text was drawn
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormDrawSelectedText(UInt16 currentField, UInt16 selectPos,
                                       UInt16 selectLen, UInt16 textY)
{
    UInt16  selectXLeft = 0;
    UInt16  selectXRight = 0;
    RectangleType  invertRect;
	
	
    // If the start of the selected region is on this line calculate a
    // value for the starting x position.
    if (gRecordFormLines[currentField].offset <= selectPos && 
        selectPos < gRecordFormLines[currentField].offset + 
        gRecordFormLines[currentField].length) {
        selectXLeft = FntCharsWidth(&gRecordFormRecord.fields[
            gRecordFormLines[currentField].fieldNum][
                gRecordFormLines[currentField].offset], 
                                    selectPos -
                                    gRecordFormLines[currentField].offset);
    }

    // If the end of the selected region is on this line calculate a
    // value for the ending x position.
    if (gRecordFormLines[currentField].offset <= selectPos + selectLen && 
        selectPos + selectLen <= gRecordFormLines[currentField].offset + 
        gRecordFormLines[currentField].length) {
        selectXRight = FntCharsWidth(&gRecordFormRecord.fields[
            gRecordFormLines[currentField].fieldNum][
                gRecordFormLines[currentField].offset], 
                                     selectPos + selectLen -
                                     gRecordFormLines[currentField].offset);
    }
	
    // If either the left or right have been set then some text needs to
    // be selected.
    if (selectXLeft | selectXRight) {
        if (! selectXRight)
            selectXRight = gRecordFormLines[currentField].x + 
                FntCharsWidth(&gRecordFormRecord.fields[
                    gRecordFormLines[currentField].fieldNum][
                        gRecordFormLines[currentField].offset], 
                              gRecordFormLines[currentField].length);

        // Now add in the left x of the text
        selectXLeft += gRecordFormLines[currentField].x;
        selectXRight += gRecordFormLines[currentField].x;
	  
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
        WinInvertRectangle(&invertRect, 0);
		
        // Reset in case text needs inversion on the next line.
        selectXLeft = 0;
        selectXRight = 0;
    }
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormErase
 *
 * DESCRIPTION: Erases the Record view display area.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormErase()
{
    FormPtr  form;
    RectangleType  r;
	
	
    form = FrmGetActiveForm();
    FrmGetObjectBounds(form, FrmGetObjectIndex(form, RecordDisplayGadget),
                       &r);
    WinEraseRectangle(&r, 0);
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordFormHandleEvent
 *
 * DESCRIPTION: Event handler for the Record view.
 *
 * PARAMETERS:  event - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/03/16  Moved cleanup code in frmCloseEvent to RecordFormCleanup.
 *
 ***********************************************************************/
static Boolean RecordFormHandleEvent(EventPtr event)
{
    Boolean   handled = false;
    FormPtr   form;

    switch (event->eType) {
        case menuEvent:
            return RecordFormDoCommand(event->data.menu.itemID);
            break;

        case menuCmdBarOpenEvent:
            MenuCmdBarAddButton(menuCmdBarOnLeft, BarDeleteBitmap,
				menuCmdBarResultMenuItem, RecordDeleteBook, NULL);
            MenuCmdBarAddButton(menuCmdBarOnLeft, BarBeamBitmap,
                                menuCmdBarResultMenuItem, RecordBeamBook, NULL);
			
            // Prevent the field manager from adding its own buttons;
            // this command bar already contains all the buttons it
            // needs.
            event->data.menuCmdBarOpen.preventFieldButtons = true;
			
            // Leave event unhandled so the system can catch it.
            break;
		
        case frmOpenEvent:
            form = FrmGetActiveForm();
            RecordFormInit(form);
            FrmDrawForm(form);
            RecordFormDraw(0, 0, 0);
            handled = true;
            break;

        case penDownEvent:
            handled = RecordFormHandlePen(event);
            break;
		
        case keyDownEvent:
            if (TxtCharIsHardKey(event->data.keyDown.modifiers,
                                 event->data.keyDown.chr)) {
                FrmGotoForm(ListForm);
                return true;
            }
			
            switch(event->data.keyDown.chr) {
                case pageUpChr:
                    RecordFormScroll(winUp);
                    handled = true;
                    break;
					
                case pageDownChr:
                    RecordFormScroll(winDown);
                    handled = true;
                    break;
					
                default:
                    break;
            }
            break;
		
        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID) {
                case RecordDoneButton:
                    // Highlight this record when returning to the List
                    // view.
                    gListFormSelectThisRecord = gCurrentRecord;
                    FrmGotoForm(ListForm);
                    handled = true;
                    break;
					
                case RecordEditButton:
                    FrmGotoForm(EditForm);
                    handled = true;
                    break;
					
                case RecordNewButton:
                    EditFormNewRecord();
                    handled = true;
                    break;
				
                default:
                    break;
            }
            break;
					
        case ctlRepeatEvent:
            switch (event->data.ctlRepeat.controlID) {
                case RecordScrollUpRepeating:
                    RecordFormScroll(winUp);
                    // Leave unhandled so button repeats.
                    break;
					
                case RecordScrollDownRepeating:
                    RecordFormScroll(winDown);
                    // Leave unhandled so button repeats.
                    break;
					
                default:
                    break;
            }
            break;
			
        case frmGotoEvent:
            form = FrmGetActiveForm();
            gCurrentRecord = event->data.frmGoto.recordNum;
            RecordFormInit(form);
            RecordFormMakeVisible(event->data.frmGoto.matchFieldNum, 
                                  event->data.frmGoto.matchPos,
                                  event->data.frmGoto.matchLen);
            FrmDrawForm(form);
            RecordFormDraw(event->data.frmGoto.matchFieldNum, 
                           event->data.frmGoto.matchPos,
                           event->data.frmGoto.matchLen);
            gPriorFormID = FrmGetFormId(form);
            handled = true;
            break;
		
        case frmUpdateEvent:
            form = FrmGetActiveForm();
            FrmDrawForm(form);
            RecordFormUpdate();
            handled = true;
            break;

        case frmCloseEvent:
            RecordFormCleanup();

            // Keep track of the last form displayed so the Note view knows
            // what it should return to.
            gPriorFormID = FrmGetActiveFormID();
            break;
		
        default:
            break;
		
    }
	
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormHandlePen
 *
 * DESCRIPTION: MemHandle the penDownEvent in the Record view. 
 *
 * PARAMETERS:  event - pointer to the penDown event
 *
 * RETURNED:    true if handled, false if not
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean RecordFormHandlePen(EventPtr event)
{
    Boolean  handled = false;
    FormPtr  form = FrmGetActiveForm();
    RectangleType  r;
    short    x, y;
    Boolean  penDown;
   
   
    // If the user taps and releases in the Record view display area,
    // go to the Edit view.
    FrmGetObjectBounds(form, FrmGetObjectIndex(form,
                                               RecordDisplayGadget), &r);
    if (RctPtInRectangle(event->screenX, event->screenY, &r)) {
        do {
            PenGetPoint (&x, &y, &penDown);
        } while (penDown);
      
        if (RctPtInRectangle (x, y, &r))
            FrmGotoForm(EditForm);
         
        handled = true;
    }
      
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormInit
 *
 * DESCRIPTION: Initializes the Record view form.  This function also
 *              formats the record for display on the screen.
 *
 * PARAMETERS:  form - pointer to the Record view form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormInit(FormPtr form)
{
    UInt16    attr;
    UInt16    index;
    UInt16    category;
    LibAppInfoType  *appInfo;
    MemHandle recordFormLinesH;
    UInt16    width = 0;
    UInt16    maxWidth;
    FontID    curFont;
    Int16     ignoreWidth, ignoreLength;
    RectangleType  r;
	
	
    // Set the category label.
    if (gCurrentCategory == dmAllCategories) {
        DmRecordInfo(gLibDB, gCurrentRecord, &attr, NULL, NULL);   
        category = attr & dmRecAttrCategoryMask;
    }
    else
        category = gCurrentCategory;
	
    CategoryGetName(gLibDB, category, gCategoryName);
    index = FrmGetObjectIndex(form, RecordCategoryLabel);
    FrmSetCategoryLabel(form, index, gCategoryName);
	
    // Allocate the record view lines.
    recordFormLinesH = MemHandleNew(sizeof(RecordFormLineType) *
                                    recordFormLinesMax);
    ErrFatalDisplayIf(! recordFormLinesH, "Out of memory");
	
    gRecordFormLines = MemHandleLock(recordFormLinesH);
    gRecordFormLastLine = 0;
    gTopRecordFormLine = 0;
	
    FrmGetFormBounds(form, &r);
    maxWidth = r.extent.x;
	
    // 000202 Removed to prevent bus error.
    //WinGetWindowExtent((short *) &maxWidth, (short *) &winHeight);
	
    appInfo = MemHandleLock(LibGetAppInfo(gLibDB));
	
    // Get the record to display.  Since the title and author strings may
    // actually be "-Untitled-" or "-No author-" if the record is missing
    // a title or author, fill in global variables to hold the title and
    // author strings.  
    LibGetRecord(gLibDB, gCurrentRecord, &gRecordFormRecord,
                 &gRecordFormRecordH);
    GetTitleString(&gRecordFormRecord, &gRecordFormTitleString,
                   &ignoreLength, &ignoreWidth, &gNoTitleRecordString);
    gRecordFormAuthorConcatenated = GetAuthorString(&gRecordFormRecord,
                                                    &gRecordFormAuthorString, &ignoreLength, &ignoreWidth, true,
                                                    &gNoAuthorRecordString);
    gRecordFormPrintingLocked = GetPrintingString(&gRecordFormRecord,
                                                  &gRecordFormPrintingString);
	
    // Construct the recordFormLines info.  The title should always be in
    // a large font, regardless of the font setting for the Record view.
    // On Palm OS 3.0 or later, this is largeBoldFont; on 2.0, use
    // largeFont.
    if (gROMVersion >= sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
        curFont = FntSetFont(largeBoldFont);
    else
        curFont = FntSetFont(largeFont);
	
    // Add the title.
    RecordFormAddField(libFieldTitle, &width, maxWidth);
    RecordFormNewLine(&width);
	
    // After the title appears in a large font, everything else on the page
    // should be drawn in the current Record view font.
    gRecordFormFirstPlainLine = gRecordFormLastLine;
    FntSetFont(gRecordFont);
	
    // Add the author, followed by a blank line.
    RecordFormAddField(libFieldLastName, &width, maxWidth);
    RecordFormNewLine(&width);
    RecordFormNewLine(&width);
	
    // Add the year, which has a copyright symbol in front of it, followed
    // by the publisher.  If the year and publisher would end up on the
    // same line, separate them with a comma and space, but make sure not
    // to add a comma and space if there is no publisher.
    if (gRecordFormRecord.fields[libFieldYear]) {
        RecordFormAddSpaceForText("©", &width);
        RecordFormAddField(libFieldYear, &width, maxWidth);
        if (gRecordFormRecord.fields[libFieldPublisher] && width > 0)
            RecordFormAddSpaceForText(", ", &width);
    }
    if (gRecordFormRecord.fields[libFieldPublisher])
        RecordFormAddField(libFieldPublisher, &width, maxWidth);
    if (gRecordFormRecord.fields[libFieldYear] ||
        gRecordFormRecord.fields[libFieldPublisher])
        RecordFormNewLine(&width);
		
    // Add the printing.
    if (gRecordFormPrintingString) {
        RecordFormAddField(libFieldPrinting, &width, maxWidth);
        RecordFormNewLine(&width);
    }
	
    // Add the ISBN.
    if (gRecordFormRecord.fields[libFieldIsbn]) {
        RecordFormAddField(libFieldIsbn, &width, maxWidth);
        RecordFormNewLine(&width);
    }
	
    // Add the cover price.
    if (gRecordFormRecord.fields[libFieldPrice]) {
        RecordFormAddField(libFieldPrice, &width, maxWidth);
        RecordFormNewLine(&width);
    }
	
    // Add a blank line if the previous line is not already blank.
    if (gRecordFormLines[gRecordFormLastLine - 1].fieldNum !=
        recordFormBlankLine)
        RecordFormNewLine(&width);
		
    // Add the book status, print status, format, and read/unread status,
    // followed by a blank line.
    RecordFormAddField(recordFormBookStatusLine, &width, maxWidth);
    RecordFormNewLine(&width);
    RecordFormAddField(recordFormPrintStatusLine, &width, maxWidth);
    RecordFormNewLine(&width);
    RecordFormAddField(recordFormFormatStatusLine, &width, maxWidth);
    RecordFormNewLine(&width);
    RecordFormAddField(recordFormReadStatusLine, &width, maxWidth);
    RecordFormNewLine(&width);
    RecordFormNewLine(&width);
	
    // Add the note field.
    if (gRecordFormRecord.fields[libFieldNote])
        RecordFormAddField(libFieldNote, &width, maxWidth);
	
    // Remove trailing blank lines.  These might occur if the record has
    // no note field.
    while (gRecordFormLastLine > 0 &&
           gRecordFormLines[gRecordFormLastLine - 1].fieldNum ==
           recordFormBlankLine) {
        gRecordFormLastLine--;
    }
	
    MemPtrUnlock(appInfo);
    FntSetFont(curFont);
	
    // If Librarian is running on Palm OS 2.0, change the menus to the V20
    // versions.
    if (gROMVersion < sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
        FrmSetMenu(form, RecordV20MenuBar);
	
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormMakeVisible
 *
 * DESCRIPTION: Make a selection range visible.  Used to properly
 *              position the Record view when Librarian displays a found
 *              record after a system global find.
 *
 * PARAMETERS:  selectFieldNum - field to show selected text
 *              selectPos - offset into field for start of selected text
 *              selectLen - length of selected text
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormMakeVisible(UInt16 selectFieldNum, UInt16 selectPos,
                                  UInt16 selectLen)
{
    UInt16 newTopRecordFormLine;
    UInt16 i;
	
	
    newTopRecordFormLine = gRecordFormLastLine;
    for (i = 0; i < gRecordFormLastLine; i++) {
        // Does the selected range end here?
        if (gRecordFormLines[i].fieldNum == selectFieldNum &&
            gRecordFormLines[i].offset <= selectPos + selectLen && 
            selectPos + selectLen <= gRecordFormLines[i].offset + 
            gRecordFormLines[i].length) {
            newTopRecordFormLine = i;
        }
    }
	
    // If the selected range doesn't seem to exist then don't scroll
    // the view.
    if (newTopRecordFormLine == gRecordFormLastLine)
        return;
	
    // Display as much before the selected text as possible.
    newTopRecordFormLine = RecordFormScrollOnePage(newTopRecordFormLine, winUp);
	
    if (newTopRecordFormLine != gTopRecordFormLine)
        gTopRecordFormLine = newTopRecordFormLine;
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormNewLine
 *
 * DESCRIPTION: Adds the next field at the start of a new line.
 *
 * PARAMETERS:  width <-> width already occupied on the line; set to
 *                        the new width occupied
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormNewLine(UInt16 *width)
{
    // Return if we've exceeded the maximum number of lines.
    if (gRecordFormLastLine >= recordFormLinesMax)
        return;
	  
	// If the current line is empty, add a blank line.
    if (*width == 0) {
        gRecordFormLines[gRecordFormLastLine].fieldNum = recordFormBlankLine;
        gRecordFormLines[gRecordFormLastLine].x = 0;
        gRecordFormLastLine++;
    }
    // Reset the occupied width to 0 to indicate a new line
    else
        *width = 0;
}


/***********************************************************************
 *
 * FUNCTION:    RecordFormScroll
 *
 * DESCRIPTION: Scrolls the record view.
 *
 * PARAMETERS:  direction - up or dowm
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/02/26  Changed code to handle 3.5 masked records.
 *
 ***********************************************************************/
static void RecordFormScroll(WinDirectionType direction)
{
    Int16   lastRecordFormLine;
    UInt16  newTopRecordFormLine;
    UInt16  category;
    UInt16  recordNum;
    Int16   seekDirection;
    UInt16  attr;
	
	
    newTopRecordFormLine = gTopRecordFormLine;
    if (direction == winUp) {
        newTopRecordFormLine = RecordFormScrollOnePage(newTopRecordFormLine,
                                                       direction);
    }
    else {
        // Simple two part algorithm:
        // 1) Scroll down one page.
        // 2) Scroll up one page from the bottom.
        // Use the higher of the two positions.
        // Find the line one page down.
        newTopRecordFormLine = RecordFormScrollOnePage(newTopRecordFormLine,
                                                       direction);
        // Find the line at the top of the last page.
        lastRecordFormLine = RecordFormScrollOnePage(gRecordFormLastLine,
                                                     winUp);
	
        // We shouldn't be past the top line of the last page.
        if (newTopRecordFormLine > lastRecordFormLine)
            newTopRecordFormLine = lastRecordFormLine;
    }
	
	
    if (newTopRecordFormLine != gTopRecordFormLine) {
        gTopRecordFormLine = newTopRecordFormLine;
		
        RecordFormErase();
        RecordFormDraw(0, 0, 0);
    }
	
    // If scrolling out of this record, scroll to the next or previous
    // record.
    else {
        if (direction == winUp)
            seekDirection = dmSeekBackward;
        else
            seekDirection = dmSeekForward;
		
        if (gShowAllCategories)
            category = dmAllCategories;
        else
            category = gCurrentCategory;
	
        recordNum = gCurrentRecord;
		
        // Skip masked records.
        while ( (! DmSeekRecordInCategory(gLibDB, &recordNum, 1,
                                          seekDirection, category)) &&
                (! DmRecordInfo(gLibDB, recordNum, &attr, NULL, NULL)) &&
                ( (attr & dmRecAttrSecret) &&
                  gPrivateRecordStatus == maskPrivateRecords) ) {
        }
        if (recordNum == gCurrentRecord) return;
		
        // Give the user an audio cue that Librarian is now displaying a
        // different record instead of a continuation of the current
        // record.
        SndPlaySystemSound(sndInfo);
		
        gCurrentRecord = recordNum;
        RecordFormUpdate();
    }
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordFormScrollOnePage
 *
 * DESCRIPTION: Scrolls the record view by one page, less one line to
 *              preserve context.
 *
 * PARAMETERS:  newTopRecordFormLine - top line of the display
 *              direction - up or dowm
 *
 * RETURNED:    new top line of the display
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt16 RecordFormScrollOnePage(UInt16 newTopRecordFormLine, 
                                      WinDirectionType direction)
{
    Int16      offset;
    FontID   curFont;
    FormPtr  form;
    Int16      largeFontLineHeight;
    Int16      stdFontLineHeight;
    Int16      currentLineHeight;
    RectangleType  r;
    Int16      recordFormDisplayHeight;
	
	
    // If running on Palm OS 3.0 or later, use largeBoldFont for the title;
    // on 2.0, use largeFont.
    if (gROMVersion >= sysMakeROMVersion(3,0,0,sysROMStageRelease,0))
        curFont = FntSetFont(largeBoldFont);
    else
        curFont = FntSetFont(largeFont);
		
    largeFontLineHeight = FntLineHeight();
    FntSetFont(gRecordFont);
    stdFontLineHeight = FntLineHeight();
    FntSetFont(curFont);
	
    form = FrmGetActiveForm();
    FrmGetObjectBounds(form, FrmGetObjectIndex(form, RecordDisplayGadget),
                       &r);
    recordFormDisplayHeight = r.extent.y;
    if (newTopRecordFormLine != gRecordFormLastLine)
        recordFormDisplayHeight -= stdFontLineHeight;  // less one line
	
    if (direction == winUp)
        offset = -1;
    else
        offset = 1;
	
    while (recordFormDisplayHeight >= 0 && 
           (newTopRecordFormLine > 0 || direction == winDown) && 
           (newTopRecordFormLine < (gRecordFormLastLine - 1) ||
            direction == winUp)) {
        newTopRecordFormLine += offset;
        if (gRecordFormLines[newTopRecordFormLine].fieldNum ==
            libFieldTitle)
            currentLineHeight = largeFontLineHeight;
        else
            currentLineHeight = stdFontLineHeight;
		
        recordFormDisplayHeight -=
            RecordFormCalcNextLine(newTopRecordFormLine, currentLineHeight);
    }
	  
    // Did we go too far?
    if (recordFormDisplayHeight < 0) {
        // The last line was too much so remove it.
        newTopRecordFormLine -= offset;
		
        // Also remove any lines which don't have a height
        while (RecordFormCalcNextLine(newTopRecordFormLine, 2) == 0) {
            newTopRecordFormLine -= offset;  // skip it
        }
    }
	
    return newTopRecordFormLine;
}
   

/***********************************************************************
 *
 * FUNCTION:    RecordFormUpdate
 *
 * DESCRIPTION: Updates the Record view and redraws it.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void RecordFormUpdate()
{
    FormPtr form;
	
	
    if (gRecordFormLines)
        MemHandleFree(MemPtrRecoverHandle(gRecordFormLines));
    gRecordFormLines = 0;
    form = FrmGetActiveForm ();
    RecordFormInit(form);
    RecordFormErase();
    RecordFormDraw(0, 0, 0);
}


#pragma mark ----------------

/***********************************************************************
 *
 * FUNCTION:    ApplicationHandleEvent
 *
 * DESCRIPTION: Loads form resources and sets the event handler for the
 *              current form.
 *
 * PARAMETERS:  event - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean ApplicationHandleEvent(EventPtr event)
{
    UInt16 formId;
    FormPtr form;


    if (event->eType == frmLoadEvent) {
        // Load the form resource.
        formId = event->data.frmLoad.formID;
        form = FrmInitForm(formId);
        FrmSetActiveForm(form);

        // Set the event handler for the form.  The handler of the currently
        // active form is called by FrmHandleEvent each time is receives an
        // event.
        switch (formId) {
            case ListForm:
                FrmSetEventHandler(form, ListFormHandleEvent);
                break;
				
            case RecordForm:
                FrmSetEventHandler(form, RecordFormHandleEvent);
                break;
			    
            case EditForm:
                FrmSetEventHandler(form, EditFormHandleEvent);
                break;
				
            case NoteView:
            case NewNoteView:
                FrmSetEventHandler(form, NoteViewHandleEvent);
                break;
            
            case PrefsForm:
                FrmSetEventHandler(form, PrefsFormHandleEvent);
                break;
				
            case DetailsForm:
                FrmSetEventHandler(form, DetailsFormHandleEvent);
                break;
				
            case AboutForm:
                FrmSetEventHandler(form, AboutFormHandleEvent);
                break;

            default:
//				ErrFatalDisplay("Invalid Form Load Event");
                break;
        }
        return true;
    }
	
    return false;
}


/***********************************************************************
 *
 * FUNCTION:    EventLoop
 *
 * DESCRIPTION: Application event loop.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EventLoop(void)
{
    UInt16 error;
    EventType event;


    do {
        EvtGetEvent(&event, evtWaitForever);
		
		
        if (! SysHandleEvent(&event))
            if (! MenuHandleEvent(0, &event, &error))
                if (! ApplicationHandleEvent(&event))
                    FrmDispatchEvent(&event);

    } while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: Main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with
 *                      the launch code. 
 *              launchFlags - word value providing extra information
 *                            about the launch.
 *
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    Err        error;
    Boolean    launched;
    DmOpenRef  db;

	
    // Check to see that Librarian is running on a supported version of
    // Palm OS.  Librarian works on Palm OS 2.0 and above.
    error = RomVersionCompatible(libMinVersion, launchFlags);
    if (error) return (error);
	
    switch (cmd) {
        case sysAppLaunchCmdNormalLaunch:
            error = StartApplication();
            if (error) 
                return error;
				
            FrmGotoForm(ListForm);
            EventLoop();
            StopApplication();
            break;

        case sysAppLaunchCmdFind:
            Search( (FindParamsType *) cmdPBP);
            break;
		
        // This launch code could be sent to the application when it's
        // already running.  If the application is not already running,
        // fire up the routines to start the app, run the event loop,
        // and stop the app.  The launched variable is true if this
        // launch code launched the app (i.e., the app was *not*
        // already running).
        case sysAppLaunchCmdGoTo:
            launched = launchFlags & sysAppLaunchFlagNewGlobals;
		
            if (launched)  {
                error = StartApplication();
                if (error) 
                    return (error);
            }
			
            GoToItem( (GoToParamsType *) cmdPBP, launched);
		
            if (launched) {
                EventLoop();
                StopApplication();   
            }      
            break;
		
        case sysAppLaunchCmdSyncNotify:
            LibRegisterData();
            break;
		
        case sysAppLaunchCmdExgAskUser:
            // If Librarian is not already running, open its database.
            if (! (launchFlags & sysAppLaunchFlagSubCall)) {
                error = LibGetDatabase(&db, dmModeReadWrite);
            } else {
                db = gLibDB;
            }
			
            if (db != NULL) {
                CustomBeamDialog(db, (ExgAskParamType *) cmdPBP);
                if (! (launchFlags & sysAppLaunchFlagSubCall))
                    error = DmCloseDatabase(db);
            }
            break;
		
        case sysAppLaunchCmdExgReceiveData:
            // If Librarian is not already running, open its database.
            if (! (launchFlags & sysAppLaunchFlagSubCall)) {
                error = LibGetDatabase(&db, dmModeReadWrite);
            } else {
                db = gLibDB;
                FrmSaveAllForms();
            }
			
            if (db != NULL) {
                error = LibReceiveData(db, (ExgSocketType *) cmdPBP);
                if (! (launchFlags & sysAppLaunchFlagSubCall))
                    error = DmCloseDatabase(db);
            }
            break;
		
        default:
            break;
    }
	
    return 0;
}


/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  Retrieves the application's preferences and creates
 *               the application database if it doesn't already exist.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err value 0 if nothing went wrong
 *
 * REVISION HISTORY:
 *   Date      Description
 *   ----      -----------
 *   00/02/26  Changed code to handle 3.5 masked records.
 *
 ***********************************************************************/
static Err StartApplication(void)
{
    LibPreferenceType  prefs;
    UInt16   prefsSize;
    Err      error = errNone;
    UInt16   mode;


    // Figure out what version Librarian is running on and set
    // gROMVersion for later reference throughout Librarian.
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &gROMVersion);
	
    // Set the FormID of the note form that is available in the ROM.
    if (gROMVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0))
        gNoteFormID = NewNoteView;
    else
        gNoteFormID = NoteView;

    // Determine whether private records should be hidden.  Palm OS 3.5
    // stores this preference differently from earlier versions, on
    // account of the 3.5 OS supporting masked records as well as
    // completely hidden records.
    if (gROMVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0))
        gPrivateRecordStatus =
            (privateRecordViewEnum) PrefGetPreference(prefShowPrivateRecords);
    else {
        if ( (Boolean) PrefGetPreference(prefHidePrivateRecordsV33) )
            gPrivateRecordStatus = hidePrivateRecords;
        else
            gPrivateRecordStatus = showPrivateRecords;
    }
	
    if (gPrivateRecordStatus == hidePrivateRecords)
        mode = dmModeReadWrite;
    else
        mode = dmModeReadWrite | dmModeShowSecret;
	
    // Find Librarian's database.  If it doesn't exist, create it.
    error = LibGetDatabase(&gLibDB, mode);
    if (error)
        return error;
		
    // Retrieve sort order from the application info block.
    gShowInList = LibGetSortOrder(gLibDB);
	
    // Read Librarian's saved preferences.
    prefsSize = sizeof(LibPreferenceType);
    if (PrefGetAppPreferences(libCreatorID, libPrefID, &prefs, &prefsSize,
                              true) != noPreferenceFound) {
        gCurrentCategory = prefs.currentCategory;
        gShowAllCategories = prefs.showAllCategories;
        gShowBookStatus = prefs.showBookStatus;
        gShowPrintStatus = prefs.showPrintStatus;
        gShowFormat = prefs.showFormat;
        gShowReadUnread = prefs.showReadUnread;
        gSaveBackup = prefs.saveBackup;
        gListFont = prefs.listFont;
        gRecordFont = prefs.recordFont;
        gEditFont = prefs.editFont;
        gNoteFont = prefs.noteFont;
		
    } else {
        // No preferences exist yet, so set the defaults for the global
        // font variables.
        gListFont = stdFont;
        gNoteFont = stdFont;
		
        // If Librarian is running on Palm OS 2.0, the largeBoldFont
        // is invalid.  In that case, substitute stdFont.
        if (gROMVersion <
            sysMakeROMVersion(3,0,0,sysROMStageRelease,0)) {
            gRecordFont = stdFont;
            gEditFont = stdFont;
        }
        else {
            gRecordFont = largeBoldFont;
            gEditFont = largeBoldFont;
        }
    }
		
    return (error);
}


/***********************************************************************
 *
 * FUNCTION:    StopApplication
 *
 * DESCRIPTION: Saves the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void StopApplication(void)
{
    LibPreferenceType  prefs;
   
	
    // Save Librarian's preferences.
    prefs.currentCategory = gCurrentCategory;
    prefs.showAllCategories = gShowAllCategories;
    prefs.showBookStatus = gShowBookStatus;
    prefs.showPrintStatus = gShowPrintStatus;
    prefs.showFormat = gShowFormat;
    prefs.showReadUnread = gShowReadUnread;
    prefs.saveBackup = gSaveBackup;
    prefs.listFont = gListFont;
    prefs.recordFont = gRecordFont;
    prefs.editFont = gEditFont;
    prefs.noteFont = gNoteFont;
	
    PrefSetAppPreferences(libCreatorID, libPrefID, libPrefVersionNum, 
                          &prefs, sizeof(prefs), true);
		                   
    // Send a frmSave event to all the open forms.
    FrmSaveAllForms();

    // Close all the open forms.
    FrmCloseAllForms();

    // Close the application's data file.
    DmCloseDatabase(gLibDB);
}
