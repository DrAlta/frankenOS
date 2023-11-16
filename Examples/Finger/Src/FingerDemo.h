/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FingerDemo.h
 *
 * History:
 *		01/04/97		Gregory Toto, created. 
 *
 *****************************************************************************/

#include "FingerRsc.h"



// useful debugging tools on the device when the serial port is occupied
// and you can't use the nifty source code debugger...

#define	AlertPrintf1(a)					FrmCustomAlert(PrintfAlert,(a)," "," ")
#define	AlertPrintf2(a,b)					FrmCustomAlert(PrintfAlert,(a),(b)," ")
#define	AlertPrintf3(a,b,c)				FrmCustomAlert(PrintfAlert,(a),(b),(c))

#define AlertPrintf							AlertPrintf2



// resource ID of first network activity bitmap.
// define here because of Constructor b10 bug -
// #defines for bit maps are not generated...

#define NetActBitMap							1100



#define FingerDBNameStr						"FingerDB"
#define FingerCreator						'Fngr'
#define FingerDBType							'DATA'




//=======================================================================
// FingerMain.c

Boolean CreateFingerDatabase( void );
void ClobberFingerDatabase( Boolean showErrDialog );


void ShowControl( FormPtr formP, UInt16 controlID, Boolean showTheControl );

FieldPtr SetFieldTextFromStr( FormPtr formP, UInt16 fieldID, MemPtr strP );
FieldPtr SetFieldTextFromRes( FormPtr formP, UInt16 fieldID, UInt16 strID );

FieldPtr ClearFieldText( FormPtr formP, UInt16 fldID );
void ShowErrorDialog( UInt16 stringID, MemPtr aStrPtr );
void ShowErrorDialog2( UInt16 stringID1,  UInt16 stringID2 );
void UpdateScrollBar( FieldPtr fldP, ScrollBarPtr barP );
void ScrollField( FieldPtr fldP, ScrollBarPtr barP, Int16 linesToScroll );
void PageScroll( FieldPtr fldP, ScrollBarPtr barP, WinDirectionType direction );

void FingerResultUpdate( Boolean doInit, UInt32 bytesFetched );


// Network.c

void InitNetworkGlobals( void );
Boolean ValidNetAddress( FormPtr formP, UInt16 addrFieldID, MemHandle * whoStrHP, MemHandle * whereStrHP );
Boolean FindNetworkLibrary( void );
Boolean OpenNetwork( void );
Boolean CloseNetwork( Boolean forceToClose );
Boolean RefreshNetwork( Boolean reconnectIfDown );
Boolean TestNetwork( void );
void FingerIt( MemHandle whoStrH, MemHandle whereStrH, FieldPtr resFldP );




//=======================================================================

extern UInt16			gCurrentFrmID;
extern DmOpenRef	gFingerDB;	

