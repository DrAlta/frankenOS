/************************************************************************
* COPYRIGHT:   Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE:        ScanMgrStruct.h
*
* SYSTEM:      Symbol barcode scanner for Palm III.
*
* HEADER:      Scan Manager structure definitions
*
* DESCRIPTION: Provides the structure definitions used by the internal
*					functions of the decoder shared library.
*
* HISTORY:     4/13/98    SS   Created
*              ...
*************************************************************************/
#pragma once

// The ScanEvent record.
typedef struct 
{
	enum events    eType;
	Boolean        penDown;
	SWord          screenX;
	SWord          screenY;
	union scanData 
	{
		struct scanGen
		{
			Word data1;
			Word data2;
			Word data3;
			Word data4;
			Word data5;
			Word data6;
			Word data7;
			Word data8;
		} scanGen;

		struct 
		{
			UInt batteryLevel;			// The current voltage measured in millivolts
			UInt batteryErrorType;		// not used
		} batteryError;

	} scanData;	// End of union
	
} ScanEventType;
typedef ScanEventType *ScanEventPtr;

/*******************************************************************
 *    Message structure used to hold decoder messages              *
 * Used by ScanGetDecodeData to return barcode type and data       *
 *******************************************************************/
#define MAX_PACKET_LENGTH       258
typedef struct  tagMESSAGE
{
	int length; // length of the data
	int type;	// contains the barcode type when the msg is DecodeData
	int status; // should be STATUS_OK
	unsigned char data[MAX_PACKET_LENGTH]; // the message data
} MESSAGE;


/*******************************************************************
 * BATCH structure used to hold all the decoder parameters to be   *
 * sent to the decoder.  Not used by application programmers.      *
 *******************************************************************/
#define MAX_BATCH_PARAM         247
typedef struct tagBATCH
{
	int length;
	int data[MAX_BATCH_PARAM + 3];
} BATCH;

/*******************************************************************
 * Communication structure COMM_STRUCT is used in the Scan Manager *
 * internal code.                                                  *
 * Note that the communications parameters are preset for this     *
 * version of the decoder and need not be changed by applications. *
 *******************************************************************/
typedef struct  tagCOMM_STRUCT
{
	int port;
	int baud;
	int parity;			
	int stop_bits;
	int host_timeout;
} COMM_STRUCT;

/*******************************************************************
 * Decoder Parameters                                              *
 *******************************************************************/
#define MAX_DECODER_PARMS       1024


