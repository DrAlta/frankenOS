/************************************************************************
* COPYRIGHT:   Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE:        ptPrint.h
*
* SYSTEM:      Symbol Print API.
*
* HEADER:      Print API Library Header
*
* DESCRIPTION: Provides function declarations and other types for
*			   use by a print application.
*
* HISTORY:     02/19/99    Mitchs   Created
*              
*************************************************************************/
#pragma once

#include <SerialMgr.h>


/*
 *	Return Code Status
 */
typedef enum tagPTStatus
{
	PTStatusOK			=	0,
	PTStatusFail,
	PTStatusRomIncompatible,
	PTStatusBadParmeter,
	PTStatusTransportNotAvail,
	PTStatusErrLine,
	PTStatusNotOpen,
	PTStatusAlreadyOpen,
	PTStatusNoMemory,
	PTStatusAlreadyConnect,
	PTStatusPrinterNotFound,
	PTStatusTimeOut,
	PTStatusPending,
	PTStatusNoPeerAddr,
	PTStatusIrBindFailed,
	PTStatusIrNoDeviceFound,
	PTStatusIrDicoverFail,
	PTStatusIrConnectFailed,
	PTStatusIrConnectLapFailed,
	PTStatusIrQueryFailed,
	PTStatusPrintCapFailed
		
} PTStatus;

/*
 *	Supported Transports
 */
typedef enum tagPTTransports
{
	PTUnknown			= 0,
	PTSerial,
	PTIr
		
} PTTransport;

/*
 *	Communication Settings
 */
typedef struct tagPTConnectSettings {

	ULong			baudRate;			// baud rate
	ULong			timeOut;			// time out in System Ticks
	ULong			flags;				// 
	UInt			recvBufSize;		// receive buffer size
	
} PTConnectSettings;

typedef PTConnectSettings * PTConnectSettingsPtr;

/*
 *	Serial Defaults
 */
#define		PTDefaultSerBaudRate	9600
#define		PTDefaultSerFlags		serDefaultSettings
#define		PTDefaultSerTimeout		(5 * sysTicksPerSecond)

/*
 *	Ir Defaults
 */
#define		PTDefaultIrBaudRate		irOpenOptSpeed57600
#define 	PTDefaultIrTimeout		(4 * sysTicksPerSecond)
#define		PTDefaultSerRecvBuf		512

/*******************************************************************
 *	Low-Level API Functions                                        *
 *	Functions for opening and closing                              *
 *******************************************************************/

PTStatus ptOpenPrinter( CharPtr printerModel, PTTransport transport,
						PTConnectSettingsPtr customSettings );

PTStatus ptClosePrinter( );

PTStatus ptConnectPrinter( CharPtr printerName );

PTStatus ptDisconnectPrinter();

PTStatus ptInitPrinter( VoidPtr initPtr, ULong length );

PTStatus ptResetPrinter( VoidPtr resetPtr, ULong length );

PTStatus ptQueryPrinter( VoidPtr queryPtr, ULong length, 
						 VoidPtr queryResPtr, ULongPtr rtnLength );
 
PTStatus ptWritePrinter( VoidPtr buffer, ULong length );

PTStatus ptQueryPrintCap( VoidPtr query, VoidPtr queryResPtr, 
						  ULongPtr rtnLength );
						  
PTStatus ptPrintApiVersion( CharPtr ptr, Int len );

/*******************************************************************
 *	High-Level API Functions                                       *
 *******************************************************************/
 
PTStatus ptStartPrintBuffer( ULong size );

PTStatus ptSetFont( CharPtr fontBuffPtr );

PTStatus ptTextToBuffer( UInt xStart, UInt yStart, CharPtr pText );

PTStatus ptLineToBuffer( UInt xStart, UInt yStart, 
						 UInt xEnd,   UInt yEnd, UInt lineThickness );

PTStatus ptRectToBuffer( UInt xTopLeft,     UInt yTopLeft,
						 UInt xBottomRight, UInt yBottomRight, 
						 UInt lineThickness );

PTStatus ptResetPrintBuffer();

PTStatus ptPrintPrintBuffer( CharPtr printerName );


/*******************************************************************
 *              end of file                                        *
 *******************************************************************/
