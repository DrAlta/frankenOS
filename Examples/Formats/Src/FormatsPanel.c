/******************************************************************************
 *
 * Copyright (c) 1996-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FormatsPanel.c
 *
 * Description:
 *	  This is the Formats Panel's main module.  This module
 *   starts the panel, dispatches events, and stops
 *   the panel. 
 *
 *   This panel is an SDK example because it shows two things well.
 *   1. It shows how to make a panel properly so it interacts well with
 *   other panels in the preference area and 
 *	  2. It shows how to use the localized information for times, dates, 
 *	  and numbers well.
 *
 * History:
 *		June 3, 1996	Created by Roger Flores
 *
 *****************************************************************************/

#include <PalmOS.h>

#include "FormatsPanel.h"



/***********************************************************************
 *	Internal Constants
 ***********************************************************************/

#define defaultTimeButton	timeHourButton

//	Separator line on Formats form
#define formatsSepLineYPos		34

//	Measurements needed for displaying time & dates on Formats form.
// By using compiled in coordinates the app must be recompiled if someone
// tries to move around the ui.  A different choice would be to use labels. 
// This way trades flexibility for size and speed.
#define lineSpacing				12
#define timeDisplayXPos			89
#define timeDisplayYPos			52
#define dateDisplayXPos			timeDisplayXPos
#define dateDisplayYPos			(timeDisplayYPos + 2 * lineSpacing)

// How far buttons should be from each other when they are moved because
// the Done button needs to appear.
#define ButtonSeparationXDistance	6

/***********************************************************************
 *	Internal typedefs
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			CS		07/16/99	Added daylightSavings & minutesWestOfGMT
 *								to FormatsPreferencesType, even though we don't
 *								allow user to modify them directly, since they
 *								should track country changes as well.
 *
 ***********************************************************************/

typedef struct 
	{
	// system preferences changed by this panel
	CountryType country;					// Country the device is in
	UInt8 weekStartDay;					// Sunday or Monday
	NumberFormatType numberFormat;	// Format to display numbers in
	TimeFormatType timeFormat;			// Format to display time in
	DateFormatType dateFormat;			// Format to display date in
	DateFormatType longDateFormat;	// Format to display date in
	MeasurementSystemType measurementSystem;	// Format for measurements (English, metric)
	// system preferences changed when country changes
	DaylightSavingsTypes	daylightSavings;	// Type of daylight savings correction (can't be set directly)
	UInt32 minutesWestOfGMT;			// minutes west of Greenwich (can't be set directly)
	} FormatsPreferencesType;
	
	
/***********************************************************************
 *	Local function prototypes
 ***********************************************************************/

static void FormatsFormDrawForm (void);
static void FormatsFormForceDrawForm (void);

/***********************************************************************
 *	Global variables
 ***********************************************************************/
static Boolean CalledFromAppMode;
		// True means hide the panel pick list and display a Done button to
		// return to the calling app.

static UInt16 panelCount;
static MemHandle panelIDsH;			// MemHandle to block containing pick list items
static SysDBListItemType *panelIDsP = 0;	// ptr to block containing pick list items

static FormatsPreferencesType FormatsPreferences;
char FormatsFormTimeString[timeStringLength];
char FormatsFormDateString[dateStringLength];
char FormatsFormLongDateString[longDateStrLength];

// Storage for the country information.
MemHandle CountryPreferencesH;
CountryPreferencesType *CountryPreferencesP;
UInt8 CountryCount;
CountryType CountryOrder[countryCount];


TimeFormatType TimeFormatMappings[timeFormatsSelectable] =
	{
	tfColonAMPM, tfColon24h, tfDotAMPM, tfDot24h, tfComma24h
	};

//	Map ui list positions to Int16 date formats
DateFormatType DateFormatMappings[dateFormatsSelectable] =
	{	
	//	All of the Int16 date formats:
	dfMDYWithSlashes, dfDMYWithSlashes, dfDMYWithDots,
	dfDMYWithDashes, dfYMDWithSlashes, dfYMDWithDots,
	dfYMDWithDashes
	};

//	Map ui list positions to long date formats
DateFormatType LongDateFormatMappings[dateFormatsSelectable] =
	{
	dfMDYLongWithComma, dfDMYLong, dfDMYLongWithDot,
	dfDMYLong, dfYMDLongWithSpace, dfYMDLongWithDot,
	dfYMDLongWithSpace
	};

CountrySettingsType CountrySettings[countriesSelectable] =
	{
	{dfMDYWithSlashes, dfMDYLongWithComma, sunday, tfColonAMPM, nfCommaPeriod},
	{dfMDYWithSlashes, dfMDYLongWithComma, sunday, tfColon24h, nfCommaPeriod},
	{dfMDYWithSlashes, dfMDYLongWithComma, sunday, tfColonAMPM, nfCommaPeriod},
	{dfDMYWithSlashes, dfDMYLong, monday, tfColon24h, nfCommaPeriod}
	};


/***********************************************************************
 *
 * FUNCTION:     CompareCountries
 *
 * DESCRIPTION:  Get the current system preferences which we may change.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/5/96	Initial Revision
 *
 ***********************************************************************/
static Int16 CompareCountries(void * a, void * b, Int32 other)
{
	CountryPreferencesType *CountryPreferencesP;
	
	
	CountryPreferencesP = (CountryPreferencesType *) other;
	return StrCompare(CountryPreferencesP[*(CountryType *)a].countryName, 
		CountryPreferencesP[*(CountryType *)b].countryName);
}


/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  Get the current system preferences which we may change.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/4/96	Initial Revision
 *			CS		07/16/99	Get prefDaylightSavings & prefMinutesWestOfGMT
 *								even though we don't allow user to modify them
 *								directly, since they should track country
 *								changes as well.
 *
 ***********************************************************************/
static UInt16 StartApplication (void)
{
	CountryType i;
	
	
	// Get the system preferences that this panel allows changing
	FormatsPreferences.timeFormat = (TimeFormatType) PrefGetPreference(prefTimeFormat);
	FormatsPreferences.dateFormat = (DateFormatType) PrefGetPreference(prefDateFormat);
	FormatsPreferences.longDateFormat = (DateFormatType) PrefGetPreference(prefLongDateFormat);
	FormatsPreferences.weekStartDay = PrefGetPreference(prefWeekStartDay);
	FormatsPreferences.numberFormat = (NumberFormatType) PrefGetPreference(prefNumberFormat);
	FormatsPreferences.country = (CountryType) PrefGetPreference(prefCountry);
	FormatsPreferences.measurementSystem = (MeasurementSystemType)PrefGetPreference(prefMeasurementSystem);

	// Get the system preferences that this panel changes whenever the country
	// changes
	FormatsPreferences.daylightSavings = (DaylightSavingsTypes)PrefGetPreference(prefDaylightSavings);
	FormatsPreferences.minutesWestOfGMT = (UInt32)PrefGetPreference(prefMinutesWestOfGMT);

	CountryPreferencesH = (MemHandle) DmGetResource (sysResTCountries, sysResIDCountries);
	if (CountryPreferencesH)
		{
		// Check that the version numbers match.
		CountryPreferencesP = MemHandleLock(CountryPreferencesH);
		CountryCount = MemPtrSize(CountryPreferencesP) / sizeof(CountryPreferencesType);
		
		// Fill CountryOrder with a country for each possible.  Then sort the list.
		for (i = countryFirst; i <= countryLast; i++)
			{
			CountryOrder[i] = i;
			}
		SysInsertionSort(CountryOrder, countryCount, sizeof(CountryType), 
			CompareCountries, (Int32) CountryPreferencesP);
		}
	else
		{
		CountryPreferencesP = NULL;
		CountryCount = 0;
		}
	
	return 0;		// no error
}

/***********************************************************************
 *
 * FUNCTION:    StopApplication
 *
 * DESCRIPTION: Save the preferences which may have changed.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/4/96	Initial Revision
 *			CS		07/16/99	Set prefDaylightSavings & prefMinutesWestOfGMT
 *								even though we don't allow user to modify them
 *								directly, since they should track country
 *								changes as well.
 *
 ***********************************************************************/
static void StopApplication (void)
{
	FrmCloseAllForms ();

	if (panelIDsP)
		MemPtrFree(panelIDsP);
	
	MemHandleUnlock(CountryPreferencesH);
	DmReleaseResource (CountryPreferencesH);


	// Write the system preferences that this panel allows changing
	PrefSetPreference(prefTimeFormat, FormatsPreferences.timeFormat);
	PrefSetPreference(prefDateFormat, FormatsPreferences.dateFormat);
	PrefSetPreference(prefLongDateFormat, FormatsPreferences.longDateFormat);
	PrefSetPreference(prefWeekStartDay, FormatsPreferences.weekStartDay);
	PrefSetPreference(prefNumberFormat, FormatsPreferences.numberFormat);
	PrefSetPreference(prefMeasurementSystem, FormatsPreferences.measurementSystem);
	
	// Note that although we don't allow user to change prefDaylightSavings
	// or prefMinutesWestOfGMT directly, we change them whenever the country
	// changes, so we need to write them out
	PrefSetPreference(prefDaylightSavings, FormatsPreferences.daylightSavings);
	PrefSetPreference(prefMinutesWestOfGMT, FormatsPreferences.minutesWestOfGMT);
	PrefSetPreference(prefCountry, FormatsPreferences.country);
}


/***********************************************************************
 *
 * FUNCTION:    MapToPositionWord
 *
 * DESCRIPTION:	Map a value to it's position in an array.  If the passed
 *						value is not found in the mappings array, a default
 *						mappings item will be returned.
 *
 * PARAMETERS:  value	- value to look for
 *
 * RETURNED:    position value found in
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf	6/28/95	Initial Revision
 *
 ***********************************************************************/
extern UInt16 MapToPositionWord (UInt16 *mappingArray, UInt16 value,
										 UInt16 mappings, UInt16 defaultItem)
{
	UInt16 i;
	
	i = 0;
	while (mappingArray[i] != value && i < mappings)
		i++;
	if (i >= mappings)
		return defaultItem;

	return i;
}


/***********************************************************************
 *
 * FUNCTION:    MapToPosition
 *
 * DESCRIPTION:	Map a value to it's position in an array.  If the passed
 *						value is not found in the mappings array, a default
 *						mappings item will be returned.
 *
 * PARAMETERS:  value	- value to look for
 *
 * RETURNED:    position value found in
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kcr	9/13/95	Initial Revision
 *
 ***********************************************************************/
static UInt16 MapToPosition (UInt8 *mappingArray, UInt8 value,
									UInt16 mappings, UInt16 defaultItem)
{
	UInt16 i;
	
	i = 0;
	while (mappingArray[i] != value && i < mappings)
		i++;
	if (i >= mappings)
		return defaultItem;

	return i;
}


/***********************************************************************
 *
 * FUNCTION:    PanelPickListDrawItem
 *
 * DEShortcutRIPTION: Draw a navigation list item.
 *
 * PARAMETERS:  itemNum - which shortcut to draw
 *					 bounds - bounds in which to draw the shortcut
 *					 unusedP - pointer to data (not used)
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger 5/29/96	Initial Revision
 *
 ***********************************************************************/

static void PanelPickListDrawItem (Int16 itemNum, RectanglePtr bounds, 
	Char ** /*unusedP*/)
{
	Char * itemText;
	
	
	itemText = panelIDsP[itemNum].name;
	WinDrawChars(itemText, StrLen(itemText), bounds->topLeft.x, bounds->topLeft.y);
}


/***********************************************************************
 *
 * FUNCTION:    CreatePanelPickList
 *
 * DESCRIPTION: Create a list of panels available and select the current one.
 *
 * PARAMETERS:  listP - the list to contain the panel list
 *
 * RETURNED:    panelCount, panelIDsH, and panelIDsP are set
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/30/96	Initial Revision
 *
 ***********************************************************************/
static void CreatePanelPickList (ControlPtr triggerP, ListPtr listP, 
	ListDrawDataFuncPtr func)
{
	UInt16 currentAppCardNo;
	LocalID currentAppDBID;
	int item;
	

	SysCreatePanelList(&panelCount, &panelIDsH);
	
	if (panelCount > 0)
		panelIDsP = MemHandleLock(panelIDsH);
	
	// Now set the list to hold the number of panels found.  There
	// is no array of text to use.
	LstSetListChoices(listP, NULL, panelCount);
	
	// Now resize the list to the number of panel found
	LstSetHeight (listP, panelCount);
		
	
	// Because there is no array of text to use, we need a function
	// to interpret the panelIDsP list and draw the list items.
	LstSetDrawFunction(listP, func);
	
	// Now we should select the item in the list which matches the 
	// current app.
	SysCurAppDatabase(&currentAppCardNo, &currentAppDBID);
	for (item = 0; item < panelCount; item++)
		{
		if (panelIDsP[item].dbID == currentAppDBID &&
			 panelIDsP[item].cardNo == currentAppCardNo)
			 {
			 LstSetSelection(listP, item);
			 CtlSetLabel(triggerP, panelIDsP[item].name);
			 break;
			 }
		}
}


/***********************************************************************
 *
 * FUNCTION:    MoveObject
 *
 * DESCRIPTION:	Move an object within a form by an offset.
 *
 * PARAMETERS:  frm - the form containing the object to move
 *				id - the id of the object to move
 *				deltaX - x distance to move
 *				deltaY - y distance to move
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf 2/28/97	Initial Revision
 *
 ***********************************************************************/
static void MoveObject(FormPtr frm, UInt16 id, Int16 deltaX, Int16 deltaY,
		Boolean redraw)
{
	UInt16 index;
	Int16 x;
	Int16 y;
	
	index = FrmGetObjectIndex(frm, id);
	
	FrmGetObjectPosition(frm, index, &x, &y);
	
	x += deltaX;
	y += deltaY;
	
	if (redraw) 
		FrmHideObject(frm, index);
		
	FrmSetObjectPosition(frm, index, x, y);
	
	if (redraw) 
		FrmShowObject(frm,index);
}	


/***********************************************************************
 *
 * FUNCTION:    SetPreferencesByCountry
 *
 * DESCRIPTION: Given a country set the other international preferences
 *
 * PARAMETERS:  countrySelection	- country to choose set other preferences by
 *
 * RETURNED:    preferences are set to standard values for the country
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf	6/30/95	Initial Revision
 *			CS		07/02/99	Change measurement system to match new country.
 *			CS		07/16/99	Change prefDaylightSavings & prefMinutesWestOfGMT
 *								even though we don't allow user to modify them
 *								directly, since they should track country
 *								changes as well.
 *
 ***********************************************************************/
static void SetPreferencesByCountry(UInt16 countrySelection)
{
	FormatsPreferences.timeFormat =
									CountryPreferencesP[countrySelection].timeFormat;
	FormatsPreferences.dateFormat =
									CountryPreferencesP[countrySelection].dateFormat;
	FormatsPreferences.longDateFormat =
									CountryPreferencesP[countrySelection].longDateFormat;
	FormatsPreferences.weekStartDay =
									CountryPreferencesP[countrySelection].weekStartDay;
	FormatsPreferences.numberFormat =
									CountryPreferencesP[countrySelection].numberFormat;
	FormatsPreferences.measurementSystem =
									CountryPreferencesP[countrySelection].measurementSystem;
	FormatsPreferences.daylightSavings =
									CountryPreferencesP[countrySelection].daylightSavings;
	FormatsPreferences.minutesWestOfGMT =
									CountryPreferencesP[countrySelection].minutesWestOfGMT;
	
	
	// Write out the time format change because the app launcher 
	//  needs the latest value if it is activated.  This can't wait
	//  until the app is exited.
	PrefSetPreference(prefTimeFormat, FormatsPreferences.timeFormat);
}


/***********************************************************************
 *
 * FUNCTION:		RedrawStringIfDifferent
 *
 * DESCRIPTION:	If newString doesn't equal oldString then draw newString 
 * at x, y and replace oldString with newString.
 *
 * PARAMETERS:		oldString - the string already drawn on the screen
 *						newString - a new string to draw if different
 *						x, y - where the old string was drawn.
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf	2/28/96	Initial version
 *
 ***********************************************************************/
static void RedrawStringIfDifferent (Char * oldString, Char * newString, Int16 x, Int16 y)
{
	RectangleType		r;
	Int16					oldWidth;
	Int16					newWidth;
	Int16					winWidth;
	Int16					winHeight;
	
	
	// Redraw the time string only if the new one is different.
	if (StrCompare(newString, oldString) != 0)
		{
		// Draw the new time string
		WinDrawChars (newString, StrLen (newString), x, y);
		
		//	Check if the old string was wider than the new string. If so
		// we must erase the portion not overwritten by the new string.
		newWidth = FntCharsWidth(newString, StrLen(newString));
		if (StrLen(oldString) > 0)
			oldWidth = FntCharsWidth(oldString, StrLen(oldString));
		else
			{
			// handle case where FormatsFormForceDrawForm emptied out old string
			// Assume the string could have extended to the edge of the window.
			WinGetWindowExtent(&winWidth, &winHeight);
			oldWidth = winWidth - x;
			}
		
		if (oldWidth > newWidth)
			{
			r.topLeft.x = x + newWidth;
			r.topLeft.y = y;
			r.extent.x = oldWidth - newWidth;
			r.extent.y = FntLineHeight();
			WinEraseRectangle (&r, 0);
			}
		
		// Remember the newTimeString.
		StrCopy(oldString, newString);
		}
}


/***********************************************************************
 *
 * FUNCTION:		FormatsFormUpdateTime
 *
 * DESCRIPTION:	Update the displayed time string using the current
 *						time format. Nothing is drawn if there is no change.
 *
 * PARAMETERS:		nothing
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kcr	9/14/95	Initial version
 *
 ***********************************************************************/
static void FormatsFormUpdateTime (void)
{
	char					newTimeString[timeStringLength];
	DateTimeType 		today;
	
	
	// Get the time and form a string from it based on the timeFormat.
	TimSecondsToDateTime(TimGetSeconds(), &today);
	TimeToAscii(today.hour, today.minute, FormatsPreferences.timeFormat, 
		newTimeString);
	
	
	// Redraw the time string only if the new one is different.
	RedrawStringIfDifferent(FormatsFormTimeString, newTimeString, 
		timeDisplayXPos, timeDisplayYPos);
}

	
/***********************************************************************
 *
 * FUNCTION:		FormatsFormUpdateDate
 *
 * DESCRIPTION:	Update the displayed date strings using the current
 *						date formats.
 *
 * PARAMETERS:		nothing
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kcr	9/14/95	Initial version
 *
 ***********************************************************************/
static void FormatsFormUpdateDate (void)
{
	char					newDateString[longDateStrLength];
	DateTimeType 		today;
	
	
	// Get the time and form a string from it based on the 'Int16' date format.
	TimSecondsToDateTime(TimGetSeconds(), &today);
	DateToAscii(today.month, today.day, today.year,
					FormatsPreferences.dateFormat, newDateString);
	
	// Redraw the date string only if the new one is different.
	RedrawStringIfDifferent(FormatsFormDateString, newDateString, 
		dateDisplayXPos, dateDisplayYPos);
	
	
	// Get the time and form a string from it based on the 'long' date format.
	DateToAscii(today.month, today.day, today.year,
				FormatsPreferences.longDateFormat, newDateString);

	// Redraw the date string only if the new one is different.
	RedrawStringIfDifferent(FormatsFormLongDateString, newDateString, 
		dateDisplayXPos, dateDisplayYPos + FntLineHeight() + 1);
	
}


/***********************************************************************
 *
 * FUNCTION:    CountryListDrawItem
 *
 * DEShortCutRIPTION: Draw a navigation list item.
 *
 * PARAMETERS:  itemNum - which shortcut to draw
 *					 bounds - bounds in which to draw the shortcut
 *					 unusedP - pointer to data (not used)
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger 8/5/96	Initial Revision
 *
 ***********************************************************************/

static void CountryListDrawItem (Int16 itemNum, RectanglePtr bounds, Char **data)
{
	Char * itemText;
	CountryPreferencesType *CountryPreferencesP;
	
	
	CountryPreferencesP = (CountryPreferencesType *) data;
	itemText = CountryPreferencesP[CountryOrder[itemNum]].countryName;
	WinDrawChars(itemText, StrLen(itemText), bounds->topLeft.x, bounds->topLeft.y);
}


/***********************************************************************
 *
 * FUNCTION:    FormatsFormInit
 *
 * DESCRIPTION:	Initialize the ui in the formats view.  This means setting
 *						the ui to match the system preferences.
 *
 * PARAMETERS:  formP
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kcr	9/12/95	Initial version
 *
 ***********************************************************************/
static void FormatsFormInit (FormPtr formP)
{
	ListPtr listP;
	UInt16 mappedValue;
	UInt16 doneButtonIndex;
	RectangleType r;
	UInt16 distance;


	// Configure the form differently if CalledFromAppMode
	if (CalledFromAppMode)
		{
		FrmHideObject(formP, FrmGetObjectIndex (formP, formatPanelPickTrigger));
		FrmShowObject(formP, FrmGetObjectIndex (formP, formatPanelNameLabel));
		
		// The Done button is supposed to be available for the user to return
		// to the calling app.
		doneButtonIndex = FrmGetObjectIndex (formP, formatsDoneButton);
		FrmShowObject(formP, doneButtonIndex);
		
		// Figure out how much space the Done button plus some separation requires.
		FrmGetObjectBounds(formP, doneButtonIndex, &r);
		distance = r.extent.x + ButtonSeparationXDistance;
		
		// Each button at the bottom of the display (besides the Done button)
		// should be moved over now that the Done button is shown.
		// Comment these lines in when buttons are added to the bottom of the form.
//		MoveObject(formP, FirstButton, distance, 0, false);
//		MoveObject(formP, SecondButton, distance, 0, false);
		}
	
	
	// Set the country trigger and list
	listP = FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, countryList));
	LstSetListChoices(listP, (Char **) CountryPreferencesP, CountryCount);
	LstSetHeight (listP, CountryCount);
	LstSetDrawFunction(listP, CountryListDrawItem);
	mappedValue = MapToPosition ((UInt8 *) CountryOrder,
		FormatsPreferences.country, CountryCount, 0);
	LstSetSelection(listP, mappedValue);
	LstMakeItemVisible(listP, mappedValue);
	CtlSetLabel(FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, countryTrigger)),
		CountryPreferencesP[FormatsPreferences.country].countryName);


	// Set the time format trigger and list
	listP = FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, timeFormatList));
	mappedValue = MapToPosition ((UInt8 *) TimeFormatMappings,
										  FormatsPreferences.timeFormat,
										  timeFormatsSelectable,
										  defaultTimeFormatItem);
	LstSetSelection(listP, mappedValue);
	CtlSetLabel(FrmGetObjectPtr (formP,
										  FrmGetObjectIndex (formP, timeFormatTrigger)),
					LstGetSelectionText(listP, mappedValue));


	// Set the date format trigger and list
	listP = FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, dateFormatList));
	mappedValue = MapToPosition ((UInt8 *) DateFormatMappings,
										  FormatsPreferences.dateFormat,
										  dateFormatsSelectable,
										  defaultDateFormatItem);
	LstSetSelection(listP, mappedValue);
	CtlSetLabel(FrmGetObjectPtr (formP,
										  FrmGetObjectIndex (formP, dateFormatTrigger)),
					LstGetSelectionText(listP, mappedValue));

	//	Set the 'week starts' trigger & list
	listP = FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, weekStartList));
	LstSetSelection(listP, FormatsPreferences.weekStartDay);
	CtlSetLabel(FrmGetObjectPtr (formP,
										  FrmGetObjectIndex (formP, weekStartTrigger)),
					LstGetSelectionText(listP, FormatsPreferences.weekStartDay));

	// Set the number trigger and list
	listP = FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, numberFormatList));
	LstSetSelection(listP, FormatsPreferences.numberFormat);
	CtlSetLabel(FrmGetObjectPtr (formP,
										  FrmGetObjectIndex (formP, numberFormatTrigger)),
					LstGetSelectionText(listP, FormatsPreferences.numberFormat));

}


/***********************************************************************
 *
 * FUNCTION:		FormatsFormDrawForm
 *
 * DESCRIPTION:	Draw the non-resource parts of the Formats form of the
 *						preferences app.
 *
 * PARAMETERS:		nothing
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kcr	9/12/95	Initial version
 *
 ***********************************************************************/

static void FormatsFormDrawForm (void)
{
	Int16 formWidth;
	
	
	WinGetWindowExtent(&formWidth, NULL);
	WinDrawLine (0, formatsSepLineYPos, formWidth, formatsSepLineYPos);

	FormatsFormUpdateTime ();
	FormatsFormUpdateDate ();
}


/***********************************************************************
 *
 * FUNCTION:		FormatsFormForceDrawForm
 *
 * DESCRIPTION:	Draw the non-resource parts of the Formats form of the
 *						preferences app even if they don't appear different.
 *
 * PARAMETERS:		nothing
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kcr	9/12/95	Initial version
 *
 ***********************************************************************/

static void FormatsFormForceDrawForm (void)
{
	FormatsFormTimeString[0] = nullChr;	// dirty the string so it's updated
	FormatsFormDateString[0] = nullChr;	// dirty the string so it's updated
	FormatsFormLongDateString[0] = nullChr;	// dirty the string so it's updated
	FormatsFormDrawForm ();
}


/***********************************************************************
 *
 * FUNCTION:    FormatsFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the Formats form
 *              of the Preferences application.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kcr	9/11/95	Initial version
 *			roger	6/4/96	Added panel pick list
 *
 ***********************************************************************/
static Boolean FormatsFormHandleEvent (EventType * event)
{
	FormPtr 			formP;
	Boolean 			handled = false;
	UInt16	 			currentAppCardNo;
	LocalID			currentAppDBID;
	EventType		newEvent;


	if (event->eType == nilEvent)
		{
		FormatsFormUpdateTime ();
		FormatsFormUpdateDate ();
		
		return true;				// handled
		}
	
	else if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case formatsDoneButton:
				// By simply exiting we return to the prior which called us.
				newEvent.eType = appStopEvent;
				EvtAddEventToQueue(&newEvent);
				handled = true;
				break;
			}
		}

	if (event->eType == popSelectEvent)
		{
		switch (event->data.popSelect.listID)
			{
			case formatPanelPickList:
				// Switch to the panel if it isn't this one
				SysCurAppDatabase(&currentAppCardNo, &currentAppDBID);
				if (panelIDsP[event->data.popSelect.selection].dbID != currentAppDBID ||
					 panelIDsP[event->data.popSelect.selection].cardNo != currentAppCardNo)
					{
					SysUIAppSwitch(panelIDsP[event->data.popSelect.selection].cardNo, 
						panelIDsP[event->data.popSelect.selection].dbID, 
						sysAppLaunchCmdNormalLaunch, 0);
					}
				
				handled = true;
				break;
				
			case countryList:
				//	Do nothing if setting hasn't changed
				if (FormatsPreferences.country !=
									CountryOrder[event->data.popSelect.selection])
					{
					FormatsPreferences.country =
									CountryOrder[event->data.popSelect.selection];
					SetPreferencesByCountry(CountryOrder[event->data.popSelect.selection]);
					formP = FrmGetActiveForm ();
					FormatsFormInit(formP);			//	Re-display new values
					FormatsFormForceDrawForm ();
					}
				handled = true;
				break;

			case timeFormatList:
				//	Do nothing if setting hasn't changed:
				if (FormatsPreferences.timeFormat !=
								TimeFormatMappings[event->data.popSelect.selection])
					{
					FormatsPreferences.timeFormat =
								TimeFormatMappings[event->data.popSelect.selection];
					FormatsFormUpdateTime ();
					
					// Write out the time format change because the app launcher 
					//  needs the latest value if it is activated.
					PrefSetPreference(prefTimeFormat, FormatsPreferences.timeFormat);
					}
				// leave handled false so that the trigger is updated
				break;
				
			case dateFormatList:
				//	Do nothing if setting hasn't changed:
				if (FormatsPreferences.dateFormat !=
								DateFormatMappings[event->data.popSelect.selection])
					{
					FormatsPreferences.dateFormat =
								DateFormatMappings[event->data.popSelect.selection];
					FormatsPreferences.longDateFormat =
								LongDateFormatMappings[event->data.popSelect.selection];
					FormatsFormUpdateDate ();
					}
				// leave handled false so that the trigger is updated
				break;

			case weekStartList:
				FormatsPreferences.weekStartDay =
												(UInt8) event->data.popSelect.selection;
				break;

			case numberFormatList:
				FormatsPreferences.numberFormat =
								(NumberFormatType) event->data.popSelect.selection;
				// leave handled false so that the trigger is updated
				break;
			}
		}

	else if (event->eType == frmOpenEvent)
		{
		formP = FrmGetActiveForm ();
		FormatsFormInit (formP);
		FrmDrawForm (formP);
		FormatsFormForceDrawForm ();


		// Now that the form is displayed, generate the panel pick list.
		// By occuring here after everything is displayed the time required
		// isn't noticed by the user.
		if (!CalledFromAppMode)
			{
			CreatePanelPickList(
				FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, formatPanelPickTrigger)),
				FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, formatPanelPickList)),
				PanelPickListDrawItem);
			}


		handled = true;
		}

	return (handled);
}

	
/***********************************************************************
 *
 * FUNCTION:    ApplicationHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and sets the 
 *						event handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	2/21/95		Initial Revision
 *			kcr	9/11/95		added formats form
 *			kcr	9/22/95		converted from CurrentFormHandleEvent
 *
 ***********************************************************************/
static Boolean ApplicationHandleEvent (EventType * event)
{
	FormPtr formP;
	UInt16 formId;

	if (event->eType == frmLoadEvent)
		{
		// Load the form resource.
		formId = event->data.frmLoad.formID;
		formP = FrmInitForm (formId);
		FrmSetActiveForm (formP);		
		
		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmDispatchEvent each time it receives an
		// event.
		switch (formId)
			{
			case formatsForm:
				FrmSetEventHandler (formP, FormatsFormHandleEvent);
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
 * DESCRIPTION: This routine is the event loop for the Preferences
 *              aplication.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *			kcr	9/22/95	converted from CurrentFormHandleEvent to
 *								ApplicationHandleEvent
 *			kcr	9/27/95	update time/date display periodically
 *
 ***********************************************************************/
static void EventLoop (void)
{
	EventType event;

	do
		{
		// Wait no more than one second.  If more than one second passes 
		// a nilEvent will appear.  When this happpens, update the time.
		EvtGetEvent (&event, sysTicksPerSecond);

		if (! SysHandleEvent (&event))
			if (! ApplicationHandleEvent (&event))
					FrmDispatchEvent (&event);
		}
		while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the Formats Preference
 * Panel.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/21/95	Initial Revision
 *			roger	5/28/95	Broke out from the Preference app into it's own panel
 *			roger	1/9/97	Perform sysAppLaunchCmdSetActivePanel
 *
 ***********************************************************************/
UInt32		PilotMain(UInt16 cmd, void * /*cmdPBP*/, UInt16 /*launchFlags*/)
{
	UInt16 error;
	
	
	// We don't support any action codes
	if (cmd != sysAppLaunchCmdNormalLaunch &&
		cmd != sysAppLaunchCmdPanelCalledFromApp &&
		cmd != sysAppLaunchCmdReturnFromPanel) 
		{
		return sysErrParamErr;
		}

	CalledFromAppMode = (cmd == sysAppLaunchCmdPanelCalledFromApp);
	
#if EMULATION_LEVEL == EMULATION_NONE
	if (!CalledFromAppMode)
		{
		PrefActivePanelParamsType prefs;
		
		// set this panel as the active one next time the prefs app is opened
		prefs.activePanel = sysFileCFormats;
		AppCallWithCommand(sysFileCPreferences, prefAppLaunchCmdSetActivePanel, &prefs);
		}
#endif


	error = StartApplication ();

	FrmGotoForm (formatsForm);

	if (! error)
		EventLoop ();

	StopApplication ();
	return 0;
}

