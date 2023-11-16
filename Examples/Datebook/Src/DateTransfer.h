/******************************************************************************
 *
 * Copyright (c) 1997-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: DateTransfer.h
 *
 * Description:
 *	  This file defines the datebook transfer functions.
 *
 * History:
 *		September 12, 1997	Created by Roger Flores
 *
 *****************************************************************************/

typedef Boolean ImportVToDoF(DmOpenRef dbP, void * inputStream, GetCharF inputFunc, 
	Boolean obeyUniqueIDs, Boolean beginAlreadyRead);
 

/************************************************************
 * Function Prototypes
 *************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

extern void DateSendRecord (DmOpenRef dbP, UInt16 recordNum);

extern void DateSendCategory (DmOpenRef dbP, UInt16 categoryNum);

extern Err DateReceiveData(DmOpenRef dbP, ExgSocketPtr exgSocketP);

extern void DateExportVCal(DmOpenRef dbP, Int16 index, ApptDBRecordPtr recordP, 
	void * outputStream, PutStringF outputFunc, Boolean writeUniqueIDs);

extern Boolean DateImportVCal(DmOpenRef dbP, void * inputStream, GetCharF inputFunc, 
	Boolean obeyUniqueIDs, Boolean beginAlreadyRead, ImportVToDoF vToDoFunc);


#ifdef __cplusplus 
}
#endif

