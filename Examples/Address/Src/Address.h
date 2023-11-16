/******************************************************************************
 *
 * Copyright (c) 1997-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Address.h
 *
 * Description:
 *		Header for the Address Book.
 *
 * History:
 *   	4/24/97  roger - Created
 *		2/2/99	meg	- added beepOnFail param
 *
 *****************************************************************************/

#include <IMCUtils.h>
#include <ExgMgr.h>

#include "AddressDB.h"



/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
 
#define addrVersionNum                    0x03
#define addrPrefVersionNum						0x03
#define addrPrefID                        0x00
#define addrDBName                        "AddressDB"
#define addrDBType                        'DATA'

#define shortenedFieldString              "..."
#define shortenedFieldLength              3
#define fieldSeparatorString              ", "
#define fieldSeparatorLength              2
#define spaceBetweenNamesAndPhoneNumbers  6

// Address lookup table columns
#define field1Column                      0
#define field2Column                      1



/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/

// The Lookup command is called while another app is running.  Because of
// of this the other app and not the Address Book has global variables.
// All the variables the lookup command needs as globals are therefore
// kept in this structure.

// 2/2/99 meg added the beepOnFail boolean because when a callng app sent in a 
// multi char string and the string did not match, the lookup code would beep
// then remove the last char. It would then repeat this process, beeping for
// each char that was removed. I added this boolean that is initialized to true
// so it beeps the first time, then when it beeps it sets the var to false. 
// Entering a char turns it back on, so a user entering into the lookup field
// will still get a beep per bad char, but it only will beep once for strings
// passed in.

typedef struct 
   {
   AddrLookupParamsPtr params;
   FormPtr frm;
   DmOpenRef dbP;
   Boolean hideSecretRecords;
	UInt8 reserved1;
   UInt16 currentRecord;
   Int16 currentPhone;
   UInt16 topVisibleRecord;
   Int16 topVisibleRecordPhone;
   Boolean sortByCompany;
   Boolean ignoreEmptyLookupField;
   AddressFields lookupFieldMap[addrLookupFieldCount];
   Char phoneLabelLetters[numPhoneLabels];
   Boolean beepOnFail;				
	UInt8 reserved2;
   } LookupVariablesType;

typedef LookupVariablesType *LookupVariablesPtr;





/************************************************************
 * Global Variables
 *************************************************************/

extern Boolean         SortByCompany;


/************************************************************
 * Function Prototypes
 *************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


// From Address.c
extern Boolean DetermineRecordName (AddrDBRecordPtr recordP, 
   Int16 *shortenedFieldWidth, Int16 *fieldSeparatorWidth, Boolean sortByCompany,
   Char **name1, Int16 *name1Length, Int16 *name1Width, 
   Char **name2, Int16 *name2Length, Int16 *name2Width, 
   Char **unnamedRecordStringPtr, Int16 nameExtent);

extern void DrawRecordName (
   Char *name1, Int16 name1Length, Int16 name1Width, 
   Char *name2, Int16 name2Length, Int16 name2Width,
   Int16 nameExtent, Int16 *x, Int16 y, Int16 shortenedFieldWidth, 
   Int16 fieldSeparatorWidth, Boolean center, Boolean priorityIsName1,
	Boolean inTitle);

extern void		InitPhoneLabelLetters(AddrAppInfoPtr appInfoPtr, Char *phoneLabelLetters);
extern void *	GetObjectPtr (UInt16 objectID);
extern void		SetDBAttrBits(DmOpenRef dbP, UInt16 attrBits);


// From AddressLookup.c
extern void Lookup (AddrLookupParamsType *params);


// From AddressTransfer.c
extern void AddrSendRecord (DmOpenRef dbP, Int16 recordNum);
extern void AddrSendCategory (DmOpenRef dbP, UInt16 categoryNum);
extern Err AddrReceiveData(DmOpenRef dbP, ExgSocketPtr obxSocketP);
extern Boolean AddrImportVCard(DmOpenRef dbP, void *inputStream, GetCharF inputFunc,
	Boolean obeyUniqueIDs, Boolean beginAlreadyRead);
extern void AddrExportVCard(DmOpenRef dbP, Int16 index, AddrDBRecordType *recordP, 
	void *outputStream, PutStringF outputFunc, Boolean writeUniqueIDs);
extern void AddrRegisterData ();


#ifdef __cplusplus 
}
#endif

