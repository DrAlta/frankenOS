/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MemoMain.h
 *
 * Description:
 *		Include file the Memo application
 *
 * History:
 *   	9/27/95	Created by Christopher Raff
 *		10/02/99	Externed the SetDBBackupBit() routine.
 *
 *****************************************************************************/

#ifndef 	__MEMOMAIN_H__
#define	__MEMOMAIN_H__

#include <IMCUtils.h>
#include <ExgMgr.h>

#define memoDBName						"MemoDB"
#define memoDBType						'DATA'
#define memoMaxLength					8192		// note: must be same as tFLD 1109 max length!!!


/************************************************************
 * Function Prototypes
 *************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


typedef UInt32 ReadFunctionF (const void * stream, Char * bufferP, UInt32 length);
typedef UInt32 WriteFunctionF (void * stream, const Char * const bufferP, Int32 length);


// From MemoTransfer.c
extern void MemoSendRecord (DmOpenRef dbP, Int16 recordNum);

extern void MemoSendCategory (DmOpenRef dbP, UInt16 categoryNum);

extern Err MemoReceiveData(DmOpenRef dbP, ExgSocketPtr exgSocketP, UInt16 *numRecordsReceived);

extern Boolean MemoImportMime(DmOpenRef dbP, void * inputStream, ReadFunctionF inputFunc,
	Boolean obeyUniqueIDs, Boolean beginAlreadyRead, UInt16 *numRecordsReceived);

extern void MemoExportMime(DmOpenRef dbP, Int16 index, MemoDBRecordType *recordP, 
	void * outputStream, WriteFunctionF outputFunc, 
	Boolean writeUniqueIDs, Boolean outputMimeInfo);
	
extern void SetDBBackupBit(DmOpenRef dbP);

#ifdef __cplusplus 
}
#endif

#endif	//	__MEMOMAIN_H__

