/******************************************************************************
 *
 * Copyright (c) 1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FormatsPanel.h
 *
 *****************************************************************************/

/***********************************************************************
 *	Internal Constants
 ***********************************************************************/

#define defaultTimeButton	timeHourButton

#define countriesSelectable	4
#define defaultCountryItem		2				//	default to U.S.

#define timeFormatsSelectable 5
#define defaultTimeFormatItem	0				//	default to tfColonAMPM

#define dateFormatsSelectable	7
#define defaultDateFormatItem	0				//	default to M/D/Y

#define autoOffSettings			3
#define defaultAutoOffSetting	1				//	default to 2 minutes

#define numSoundLevels			2				//	Use defaults from preferences.h


/***********************************************************************
 *	Type Definitions
 ***********************************************************************/

typedef struct {
	DateFormatType			dateFormat;			// Format to display date in
	DateFormatType			longDateFormat;	// Format to display date in
	UInt8						weekStartDay;		// Sunday or Monday
	TimeFormatType			timeFormat;			// Format to display time in
	NumberFormatType		numberFormat;		// Format to display numbers in
	UInt8						reserved;
	} CountrySettingsType;	


/***********************************************************************
 *	Global variables
 ***********************************************************************/
extern UInt16 CurrentRecord;
extern SystemPreferencesType SystemPreferences;


// Mappings between system preferences and the ui values
extern TimeFormatType TimeFormatMappings[timeFormatsSelectable];

//	Map ui list positions to Int16 date formats
extern DateFormatType DateFormatMappings[dateFormatsSelectable];

//	Map ui list positions to long date formats
extern DateFormatType LongDateFormatMappings[dateFormatsSelectable];
extern CountrySettingsType CountrySettings[countriesSelectable];



/***********************************************************************
 *	Global functions
 ***********************************************************************/
// Functions used by multiple Preference files.
extern UInt16 MapToPositionWord (UInt16 *mappingArray, UInt16 value,
										 UInt16 mappings, UInt16 defaultItem);
extern Boolean GotoForm (UInt16 formId);
extern void * GetObjectPtr (UInt16 objectID);



//	Formats Form
#define formatsForm					1200

#define formatPanelPickTrigger	1218
#define formatPanelPickList		1219
#define formatPanelNameLabel		1234

#define countryTrigger				1226
#define countryList					1227

#define timeFormatTrigger			1222
#define timeFormatList				1223

#define dateFormatTrigger			1215
#define dateFormatList				1216

#define weekStartTrigger			1204
#define weekStartList				1206

#define numberFormatTrigger		1230
#define numberFormatList			1231

#define formatsDoneButton			1233

