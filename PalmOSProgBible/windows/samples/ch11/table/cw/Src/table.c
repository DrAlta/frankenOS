/******************************************************************************
 *
 * Table Example application
 * Copyright (c) 2000 Lonnon R. Foster
 * All rights reserved
 *
 * From Chapter 11 of The Palm OS Programming Bible
 * Copyright (c) 2000 IDG Books Worldwide, Inc.
 *
 * File: table.c
 *
 *****************************************************************************/
#include <PalmOS.h>
#include "tableRsc.h"


/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator			  'LFtb'
#define appVersionNum              0x01
#define appPrefID                  0x00
#define appPrefVersionNum          0x01

// Define the minimum OS version we support (2.0 for now).
#define ourMinVersion	sysMakeROMVersion(2,0,0,sysROMStageRelease,0)

// Table constants
#define numTextColumns   3
#define numTableColumns  9
#define numTableRows     11

/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/


/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
static Char *  gLabels[] = {"00", "01", "02", "03", "04", "05", "06",
						    "07", "08", "09", "10"};

MemHandle  gTextHandles[numTextColumns][numTableRows];
Boolean    gHideRows = true;
Boolean    gHideColumns = true;


/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/


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
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	// See if we're on in minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
		{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
			{
			FrmAlert(RomIncompatibleAlert);
		
			// Palm OS 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < ourMinVersion)
				{
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
				}
			}
		
		return sysErrRomIncompatible;
		}

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    GetTextColumn
 *
 * DESCRIPTION: Returns the text column corresponding to a specific
 *              table column.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    void *
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Int16 GetTextColumn(Int16 column)
{
	Int16  result;


	switch (column) {
		case 3:
			result = 0;
			break;
			
		case 5:
			result = 1;
			break;
			
		case 7:
			result = 2;
			break;
			
		default:
			ErrFatalDisplay("Invalid text column");
			break;
	}
	
	return result;			
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
 * RETURNED:    void *
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void * GetObjectPtr(UInt16 objectID)
{
	FormPtr frmP;

	frmP = FrmGetActiveForm();
	return FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID));
}


/***********************************************************************
 *
 * FUNCTION:    LoadTextTableItem
 *
 * DESCRIPTION: Callback routine to load the text of a table item.
 *
 * PARAMETERS:  table - pointer to the table object
 *              row - row in the table to load
 *              column - column in the table to load
 *              editable - true if a text field in the table is currently
 *                         being edited, false otherwise
 *              dataH - unlocked handle of a block containing a null-
 *                      terminated text string
 *              dataOffset - offset from the start of the block to the
 *                           start of the actual string
 *              dataSize - allocated size of the text string, NOT the
 *                         string length
 *              field - pointer to the text field in this table cell
 *              
 *
 * RETURNED:    0 upon success, or an error code if unsuccessful
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err LoadTextTableItem(void *table, Int16 row, Int16 column,
							 Boolean editable, MemHandle *dataH,
							 Int16 *dataOffset, Int16 *dataSize,
							 FieldPtr field)
{
#ifdef __GNUC__
	CALLBACK_PROLOGUE;
#endif

	*dataH = gTextHandles[GetTextColumn(column)][row];
	*dataOffset = 0;
	*dataSize = MemHandleSize(*dataH);
	
#ifdef __GNUC__
	CALLBACK_EPILOGUE;
#endif

	return 0;

}


/***********************************************************************
 *
 * FUNCTION:    SaveTextTableItem
 *
 * DESCRIPTION: Callback routine to save the text of a table item.
 *              Converts the text in the table field to uppercase.
 *
 * PARAMETERS:  table - pointer to a table object
 *              row - row in the table to save
 *              column - column in the table to save
 *
 * RETURNED:    true if the table should be redrawn, false otherwise
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean SaveTextTableItem(void *table, Int16 row, Int16 column)
{
	Boolean   result = false;
	FieldPtr  field;
	
	
#ifdef __GNUC__
	CALLBACK_PROLOGUE;
#endif
	
	field = TblGetCurrentField(table);

	// If the field has been changed, uppercase its text.
	if (field && FldDirty(field)) {
		MemHandle  textH = gTextHandles[GetTextColumn(column)][row];
		Char       *str;
		Int16      i;
		
		str = MemHandleLock(textH);
		for (i = 0; str[i] != '\0'; i++) {
			if (str[i] >= 'a' && str[i] <= 'z') {
				str[i] -= 'a' - 'A';
			}
		}
		
		MemHandleUnlock(textH);
		TblMarkRowInvalid(table, row);
		result = true;
	}
	
#ifdef __GNUC__
	CALLBACK_EPILOGUE;
#endif
	
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    DrawCustomTableItem
 *
 * DESCRIPTION: Callback function to draw the custom table items.
 *
 * PARAMETERS:  table - pointer to the table object
 *              row - row of the table to draw
 *              column - column of the table to draw
 *              bounds - pointer to a rectange defining the region in
 *                       which to draw
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void DrawCustomTableItem(void *table, Int16 row, Int16 column,
                                RectangleType *bounds)
{
	FontID  curFont;
	Char    symbol[2];
	Int16   i;
	
	
#ifdef __GNUC__
	CALLBACK_PROLOGUE;
#endif

	for (i = 0; i < 2; i++)
		symbol[i] = '\0';
		
	switch(TblGetItemInt(table, row, column)) {
		case 1:
			symbol[0] = symbolAlarm;
			break;
			
		case 2:
			symbol[0] = symbolRepeat;
			break;
			
		default:
			break;
	}

	if (symbol[0] != '\0') {
		curFont = FntSetFont(symbolFont);
		WinDrawChars(&symbol[0], 1, bounds->topLeft.x + 1,
		             bounds->topLeft.y);
		FntSetFont(curFont);
	}
	
#ifdef __GNUC__
	CALLBACK_EPILOGUE;
#endif
}



/***********************************************************************
 *
 * FUNCTION:    DrawNarrowTextTableItem
 *
 * DESCRIPTION: Callback function to draw the narrow text table items.
 *
 * PARAMETERS:  table - pointer to the table object
 *              row - row of the table to draw
 *              column - column of the table to draw
 *              bounds - pointer to a rectange defining the region in
 *                       which to draw
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void DrawNarrowTextTableItem(void *table, Int16 row, Int16 column,
                                    RectangleType *bounds)
{
	Char    symbol[3];
	Int16   i;
	Int16   length = 0;
	
#ifdef __GNUC__
	CALLBACK_PROLOGUE;
#endif

	for (i = 0; i < 3; i++)
		symbol[i] = '\0';
		
	switch(TblGetItemInt(table, row, column)) {
		case 13:
			symbol[0] = symbolAlarm;
			length = 1;
			break;
			
		case 20:
			symbol[0] = symbolAlarm;
			symbol[1] = symbolRepeat;
			length = 2;
			break;
			
		default:
			break;
	}

	if (symbol[0] != '\0') {
		FontID  curFont = FntSetFont(symbolFont);
		Coord   x;
		
		x = (bounds->topLeft.x + bounds->extent.x) - ((length * 7) + 6);
		WinDrawChars(&symbol[0], length, x, bounds->topLeft.y);
		FntSetFont(curFont);
	}
	
#ifdef __GNUC__
	CALLBACK_EPILOGUE;
#endif
}



/***********************************************************************
 *
 * FUNCTION:    MainFormInit
 *
 * DESCRIPTION: This routine initializes the Main form.
 *
 * PARAMETERS:  form - pointer to the Main form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormInit(FormPtr form)
{
	TablePtr  table;
	Int16     numRows;
	Int16     i;
	DateType  dates[11], today;
	UInt32    now;
	UInt32    curDate;
	ListPtr   list;
	
	
	// Initialize the dates.  The first date in the table will be set
	// to noTime, so it will display as a hypen.  The rest of the dates
	// will range from four days ago to five days ahead of the current
	// date on the handheld.
	* ((Int16 *) &dates[0]) = noTime;  // The first date should display
	                                   // as a hyphen.
	DateSecondsToDate(TimGetSeconds(), &today);
	now = DateToDays(today);
	for (i = 1; i < sizeof(dates) / sizeof(*dates); i++) {
		curDate = now - 5 + i;
		DateDaysToDate(curDate, &(dates[i]));
	}
	
	table = FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainTable));
	list = FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainList));
	
	numRows = TblGetNumberOfRows(table);
	for (i = 0; i < numRows; i++) {
		TblSetItemStyle(table, i, 0, labelTableItem);
		TblSetItemPtr(table, i, 0, gLabels[i]);
		
		TblSetItemStyle(table, i, 1, dateTableItem);
		TblSetItemInt(table, i, 1, DateToInt(dates[i]));
		
		TblSetItemStyle(table, i, 2, numericTableItem);
		TblSetItemInt(table, i, 2, i);
		
		TblSetItemStyle(table, i, 3, textTableItem);
		
		TblSetItemStyle(table, i, 4, checkboxTableItem);
		TblSetItemInt(table, i, 4, i % 2);
		
		TblSetItemStyle(table, i, 5, narrowTextTableItem);
		TblSetItemInt(table, i, 5, ((i % 3) * 7) + 6);
		
		TblSetItemStyle(table, i, 6, popupTriggerTableItem);
		TblSetItemInt(table, i, 6, i % 3);
		TblSetItemPtr(table, i, 6, list);
		
		TblSetItemStyle(table, i, 7, textWithNoteTableItem);
		
		TblSetItemStyle(table, i, 8, customTableItem);
		TblSetItemInt(table, i, 8, i % 3);
		
		TblSetRowStaticHeight(table, i, true);
	}

	for (i = 0; i < numTableColumns; i++) {
		TblSetColumnUsable(table, i, true);
		switch (i) {
			case 2:
				TblSetColumnSpacing(table, i, 2);
				break;
				
			default:
				TblSetColumnSpacing(table, i, 0);
				break;
		}
	}
		
	TblSetLoadDataProcedure(table, 3, LoadTextTableItem);
	TblSetLoadDataProcedure(table, 5, LoadTextTableItem);
	TblSetLoadDataProcedure(table, 7, LoadTextTableItem);
	
	TblSetSaveDataProcedure(table, 3, SaveTextTableItem);
	
	TblSetCustomDrawProcedure(table, 5, DrawNarrowTextTableItem);
	TblSetCustomDrawProcedure(table, 8, DrawCustomTableItem);
	
	FrmDrawForm(form);
}


/***********************************************************************
 *
 * FUNCTION:    MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean MainFormDoCommand(UInt16 command)
{
	Boolean handled = false;
	FormPtr form;

	switch (command)
		{
		case MainOptionsAboutTableExample:
			MenuEraseStatus(0);				// Clear the menu status.
			form = FrmInitForm(AboutForm);
			FrmDoDialog(form);				// Display the About Box.
			FrmDeleteForm(form);
			handled = true;
			break;

		}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    ToggleRows
 *
 * DESCRIPTION: Hides or displays every other table row.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ToggleRows(void)
{
	FormPtr   form;
	TablePtr  table;
	ControlPtr  ctl;
	Int16     i;
	
	
	form = FrmGetActiveForm();
	table = FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainTable));
	ctl = FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainHideRowsButton));
	
	
	if (gHideRows) {
		for (i = 0; i < numTableRows; i++) {
			if (i % 2)
				TblSetRowUsable(table, i, false);
		}
		CtlSetLabel(ctl, "Show Rows");
	} else {
		for (i = 0; i < numTableRows; i++) {
			if (i % 2)
				TblSetRowUsable(table, i, true);
		}
		CtlSetLabel(ctl, "Hide Rows");
	}
	
	for (i = 0; i < numTableRows; i++)
		TblMarkRowInvalid(table, i);
	TblRedrawTable(table);
	gHideRows = !gHideRows;
}


/***********************************************************************
 *
 * FUNCTION:    ToggleColumns
 *
 * DESCRIPTION: Hides or displays every other table row.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ToggleColumns(void)
{
	FormPtr   form;
	TablePtr  table;
	ControlPtr  ctl;
	Int16     i;
	
	
	form = FrmGetActiveForm();
	table = FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainTable));
	ctl = FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainHideColumnsButton));
	
	
	if (gHideColumns) {
		for (i = 0; i < numTableColumns; i++) {
			if (i % 2)
				TblSetColumnUsable(table, i, false);
		}
		CtlSetLabel(ctl, "Show Columns");
	} else {
		for (i = 0; i < numTableColumns; i++) {
			if (i % 2)
				TblSetColumnUsable(table, i, true);
		}
		CtlSetLabel(ctl, "Hide Columns");
	}
	
	for (i = 0; i < numTableRows; i++)
		TblMarkRowInvalid(table, i);
	TblRedrawTable(table);
	gHideColumns = !gHideColumns;
}


/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "MainForm" of this application.
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
static Boolean MainFormHandleEvent(EventPtr event)
{
	Boolean  handled = false;
	FormPtr  form;

	switch (event->eType) {
		case menuEvent:
			return MainFormDoCommand(event->data.menu.itemID);

		case frmOpenEvent:
			form = FrmGetActiveForm();
			MainFormInit(form);
			FrmDrawForm(form);
			handled = true;
			break;

		case tblEnterEvent:
			{
				Int16  row = event->data.tblEnter.row;
				Int16  column = event->data.tblEnter.column;
				
				if (column == 8) {
					TablePtr  table = event->data.tblEnter.pTable;
					Int16     oldValue = TblGetItemInt(table, row, column);
					
					TblSetItemInt(table, row, column, (oldValue + 1) % 3);
					TblMarkRowInvalid(table, row);
					TblRedrawTable(table);
					handled = true;
				}
			}
			break;

		case ctlSelectEvent:
			switch (event->data.ctlSelect.controlID) {
				case MainHideRowsButton:
					ToggleRows();
					handled = true;
					break;
					
				case MainHideColumnsButton:
					ToggleColumns();
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
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
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
	UInt16   formId;
	FormPtr  form;

	if (event->eType == frmLoadEvent) {
		// Load the form resource.
		formId = event->data.frmLoad.formID;
		form = FrmInitForm(formId);
		FrmSetActiveForm(form);

		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId) {
			case MainForm:
				FrmSetEventHandler(form, MainFormHandleEvent);
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
 * DESCRIPTION: This routine is the event loop for the application.  
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
	UInt16     error;
	EventType  event;

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
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  Set up globals at program start.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err value 0 if nothing went wrong
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err StartApplication(void)
{
	Int16  i, j;
	
	
	for (i = 0; i < numTextColumns; i++) {
		for (j = 0; j < numTableRows; j++) {
			Char  *str;
			
			gTextHandles[i][j] = MemHandleNew(1);
			str = MemHandleLock(gTextHandles[i][j]);
			*str = '\0';
			MemHandleUnlock(gTextHandles[i][j]);
		}
	}
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    StopApplication
 *
 * DESCRIPTION: Save the current state of the application.
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
	Int16  i, j;
	
	
	for (i = 0; i < numTextColumns; i++) {
		for (j = 0; j < numTableRows; j++) {
			MemHandleFree(gTextHandles[i][j]);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) return (error);

	switch (cmd) {
		case sysAppLaunchCmdNormalLaunch:
			error = StartApplication();
			if (error) 
				return error;
				
			FrmGotoForm(MainForm);
			EventLoop();
			StopApplication();
			break;

		default:
			break;

	}
	
	return errNone;
}

