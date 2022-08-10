/************************************************************************
* COPYRIGHT:   Copyright  ©  1999, 2000, & 2001 Symbol Technologies, Inc. 
*
* FILE:        ScanTcp.h
*
* SYSTEM:      Symbol barcode scanner for Palm III.
* 
* HEADER:      Scan Demo Utility Functions
*
* DESCRIPTION: Various utility functions.
*
* HISTORY:     05/01/98    dcat  First public release
*              ...
*************************************************************************/
#pragma once

typedef struct {
	char LicId[18];
	char Name1[30];
	char Name2[30];
	char Addr1[30];
	char Addr2[30];
	char City [16];
	char State[4];	
	char Sex  [4];
	char Hght [4];
	char Wght [4];
	char Hair [4];
	char Eyes [4];
} DmvType;

#define ImageSize 3064		// a 160x150x2 .bmp is 3062 or 3064 bytes

