/******************************************************************************
 *
 * Copyright (c) 1996-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ExpLookup.h
 *
 * Description:
 *	  This module defines routine that support the auto-fill feature 
 *   of the expense application.
 *
 * History:
 *		January 4, 1996	Created by Art Lamb
 *
 *****************************************************************************/

#define maxLookupEntries  25		// max number of entries per lookup database


typedef struct {
	UInt32		time;						// time the entry was last access
	Char		text;						// null-terminated string
	UInt8		reserved;
} LookupRecordType;

typedef LookupRecordType * LookupRecordPtr;




extern Err LookupInitDB (UInt32 type, UInt32 creator, Char * name, 
	UInt16 initRscID);

extern Boolean LookupStringInDatabase (DmOpenRef dbP, Char * key, 
	UInt16 * indexP);

extern Boolean LookupStringInList (ListPtr lst, Char * key, UInt16 * indexP,
	Boolean * uniqueP);

extern void LookupSave (UInt32 type, UInt32 creator, Char * str);

