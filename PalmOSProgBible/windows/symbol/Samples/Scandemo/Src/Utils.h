/************************************************************************
* COPYRIGHT: 	Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE: 		Utils.h
*
* SYSTEM: 		Symbol barcode scanner for Palm III.
* 
* HEADER: 		Scan Demo Utility Functions
* 
* DESCRIPTION: 	Various utility functions.
*
* HISTORY: 		3/2/97    SS   Created
* 				...
*************************************************************************/
#pragma once

VoidPtr GetObjectPtr (Word objectID);
void SetFieldText( UInt nFieldID, const CharPtr pSrcText, Int nMaxSize, Boolean bRedraw );
void FreeFieldHandle( int ID );
Boolean ScanGetBarTypeStr( Byte barType, CharPtr pszBarType, UInt nStrLen );
