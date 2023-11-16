/******************************************************************************
 *
 * Copyright (c) 1996-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: AddressAutoFill.h
 *
 * Description:
 *	  This module defines routine that support the auto-fill feature 
 *   of the address application.
 *
 * History:
 *		January 4, 1996	Created by Art Lamb
 *
 *****************************************************************************/

#define maxLookupEntries  100		// max number of entries per lookup database


typedef struct {
	UInt32		time;						// time the entry was last accessed
	Char		text;						// null-terminated string
	UInt8		reserved;
} LookupRecordType;

typedef LookupRecordType *LookupRecordPtr;




extern Err AutoFillInitDB (UInt32 type, UInt32 creator, Char *dbName, 
	UInt16 initRscID);

extern Boolean LookupStringInDatabase (DmOpenRef dbP, Char *key, 
	UInt16 *indexP);

extern Boolean LookupStringInList (ListPtr lst, Char *key, UInt16 *indexP,
	Boolean *uniqueP);

extern void LookupSave (UInt32 type, UInt32 creator, Char *str);

