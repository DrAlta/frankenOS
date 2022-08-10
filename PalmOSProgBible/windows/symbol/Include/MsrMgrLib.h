/***********************************************************************
 *
 Copyright (c) 1999 Symbol Technologies.  
 All rights reserved.
   
 * FileName:
 *		MsrMgrLib.h
 *
 * Description:
 *		MSR Manager library API definitions.  
 *
 * History:
 *   	 5/19/99	H.Z.
 *
 *******************************************************************/


#ifndef __MSR_MGR_LIB_H__
#define __MSR_MGR_LIB_H__

// PalmPilot common definitions
#include <Common.h>
#include <SystemMgr.rh>


/********************************************************************
 * Type and creator of Sample Library database
 ********************************************************************/
#define		MsrMgrLibCreatorID	'MSR1'				// MSR Library database creator
#define		MsrMgrLibTypeID		'libr'				// Standard library database type


/********************************************************************
 * Internal library name which can be passed to SysLibFind()
 ********************************************************************/
#define		MsrMgrLibName		"MsrMgr.lib"		


/********************************************************************
 * Internal definition
 ********************************************************************/
// Maximum string length in a card information (include Raw Data)
#define		MAX_CARD_DATA			400
// maximum characters for preamble and postamble
#define		MAX_PRE_POST_SIZE		10	
// maximum added field number
#define		MAX_AFLD_NUM			6	
// maximum added field string length
#define		MAX_AFLD_LEN			6	
// maximum data edit send command number
#define		MAX_SCMD_NUM			4	
// maximum length in a data edit send command 
#define		MAX_SCMD_LEN			40
// maximum characters in whole data edit send command
#define		MAX_SCMD_CHAR			110	
// maximum flexible field number
#define		MAX_FFLD_NUM			16	
// maximum length in a flexible field setting command
#define		MAX_FFLD_LEN			20
// maximum characters in whole flexible field setting command
#define		MAX_FFLD_CHAR			60	
// Baud rate for command communication
#define		COMM_BAUDRATE			9600
// maximum reserved character to define
#define		MAX_RES_CHAR_NUM		6
// maximum track number
#define		MAX_TRACK_NUM			3
// characters for track format
#define		TRACK_FORMAT_LEN		5

// Key event received when data is ready from MSR 3000
#define msrDataReadyKey	0x15af

/************************************************************
 * MSR Manager Library result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *************************************************************/

#define	MsrMgrNormal			0						// normal 
#define MsrMgrErrGlobal			(appErrorClass | 1)		// glbal parameter error
#define MsrMgrErrParam			(appErrorClass | 2)		// invalid parameter
#define MsrMgrErrNotOpen		(appErrorClass | 3)		// library is not open
#define MsrMgrErrStillOpen		(appErrorClass | 4)		// returned from MSRLibClose() if
														// the library is still open by others
#define MsrMgrErrMemory			(appErrorClass | 5)		// memory error occurred
#define MsrMgrErrSize			(appErrorClass | 6)		// card information overflow
#define MsrMgrErrNAK			(appErrorClass | 7)		// firmware NAK answer
#define MsrMgrErrTimeout		(appErrorClass | 8)		// waiting timeout
#define MsrMgrErrROM			(appErrorClass | 9)		// ROM check error
#define MsrMgrErrRAM			(appErrorClass | 10)	// RAM check error
#define MsrMgrErrEEPROM			(appErrorClass | 11)	// EEPROM check error
#define MsrMgrErrRes			(appErrorClass | 12)	// error response
#define MsrMgrErrChecksum		(appErrorClass | 13)	// check sum error
#define MsrMgrBadRead			(appErrorClass | 14)	// Bad read for buffered mode
#define MsrMgrNoData			(appErrorClass | 15)	// No data for buffered mode
#define	MsrMgrLowBattery		(appErrorClass | 16)	// No enough battery for MSR 3000


//-----------------------------------------------------------------------------
// MSR library function trap ID's. Each library call gets a trap number:
//   MsrMgrLibTrapXXXX which serves as an index into the library's dispatch table.
//   The constant sysLibTrapCustom is the first available trap number after
//   the system predefined library traps Open,Close,Sleep & Wake.
//-----------------------------------------------------------------------------

typedef	enum {
	msrLibTrapGetSetting = sysLibTrapCustom,
	msrLibTrapSendSetting,
	msrLibTrapGetVersion,
	msrLibTrapGetStatus,
	msrLibTrapSelfDiagnose,
	msrLibTrapSetBufferMode,
	msrLibTrapArmtoRead,
	msrLibTrapSetTerminator,
	msrLibTrapSetPreamble,
	msrLibTrapSetPostamble,
	msrLibTrapSetTrackSelection,
	msrLibTrapSetTrackSeparator,
	msrLibTrapSetLRC,
	msrLibTrapSetDataEdit,
	msrLibTrapSetAddedField,
	msrLibTrapSetDataEditSend,
	msrLibTrapSetFlexibleField,
	msrLibTrapGetDataBuffer,
	msrLibTrapReadMSRBuffer,
	msrLibTrapReadMSRUnbuffer,
	msrLibTrapSetDefault,
	msrLibTrapSetDecoderMode,
	msrLibTrapSetTrackFormat,
	msrLibTrapSetReservedChar
	} MSRLibTrapNumberEnum;
	

/********************************************************************
 * Public Structures
 ********************************************************************/
 
// Library globals
typedef	struct	ReservedChar {
	Byte		format;
	char		SR_Bits;
	char		SR_Chars;
	} ReservedChar;

typedef	struct	MSRSetting {
	Byte		Buffer_mode;
	Byte		Terminator;
	char		Preamble[MAX_PRE_POST_SIZE+1];
	char		Postamble[MAX_PRE_POST_SIZE+1];
	Byte		Track_selection;
	Byte		Track_separator;
	Byte		LRC_setting;
	Byte		Data_edit_setting;
	Byte		Decoder_mode;
	Byte		Track_format[MAX_TRACK_NUM][TRACK_FORMAT_LEN];
	ReservedChar	Reserved_chars[MAX_RES_CHAR_NUM];
	char		Added_field[MAX_AFLD_NUM][MAX_AFLD_LEN+1];
	char		Send_cmd[MAX_SCMD_NUM][MAX_SCMD_LEN];
	char		Flexible_field[MAX_FFLD_NUM][MAX_FFLD_LEN];
	} MSRSetting;

typedef MSRSetting*				MSRSetting_Ptr;
typedef char *					MSRCardInfo_Ptr;
typedef	ReservedChar*			ReservedChar_Ptr;


/********************************************************************
 * API Prototypes
 ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


//--------------------------------------------------
// Standard library open, close, sleep and wake functions
//--------------------------------------------------

extern Err MsrOpen(UInt refNum, unsigned long *msrVerP, unsigned long *libVerP)
				SYS_TRAP(sysLibTrapOpen);
				
extern Err MsrClose(UInt refNum)
				SYS_TRAP(sysLibTrapClose);

extern Err MsrSleep(UInt refNum)
				SYS_TRAP(sysLibTrapSleep);

extern Err MsrWake(UInt refNum)
				SYS_TRAP(sysLibTrapWake);

extern Err MsrGetSetting(UInt refNum, MSRSetting *userMSRP)
				SYS_TRAP(msrLibTrapGetSetting);

extern Err MsrSendSetting(UInt refNum, MSRSetting userMSR)
				SYS_TRAP(msrLibTrapSendSetting);

extern Err MsrGetVersion(UInt refNum, unsigned long *msrVerP, unsigned long *libVerP)
				SYS_TRAP(msrLibTrapGetVersion);

extern Err MsrGetStatus(UInt refNum, BytePtr statusP)
				SYS_TRAP(msrLibTrapGetStatus);

extern Err MsrSelfDiagnose(UInt refNum)
				SYS_TRAP(msrLibTrapSelfDiagnose);

extern Err MsrSetBufferMode(UInt refNum, Byte mode)
				SYS_TRAP(msrLibTrapSetBufferMode);

extern Err MsrArmtoRead(UInt refNum)
				SYS_TRAP(msrLibTrapArmtoRead);

extern Err MsrSetTerminator(UInt refNum, Byte setting)
				SYS_TRAP(msrLibTrapSetTerminator);

extern Err MsrSetPreamble(UInt refNum, char *setting)
				SYS_TRAP(msrLibTrapSetPreamble);

extern Err MsrSetPostamble(UInt refNum, char *setting)
				SYS_TRAP(msrLibTrapSetPostamble);

extern Err MsrSetTrackSelection(UInt refNum, Byte setting)
				SYS_TRAP(msrLibTrapSetTrackSelection);

extern Err MsrSetTrackSeparator(UInt refNum, Byte setting)
				SYS_TRAP(msrLibTrapSetTrackSeparator);

extern Err MsrSetLRC(UInt refNum, Byte setting)
				SYS_TRAP(msrLibTrapSetLRC);

extern Err MsrSetDataEdit(UInt refNum, Byte setting)
				SYS_TRAP(msrLibTrapSetDataEdit);

extern Err MsrSetAddedField(UInt refNum, char setting[MAX_AFLD_NUM][MAX_AFLD_LEN+1])
				SYS_TRAP(msrLibTrapSetAddedField);

extern Err MsrSetDataEditSend(UInt refNum, char setting[MAX_SCMD_NUM][MAX_SCMD_LEN])
				SYS_TRAP(msrLibTrapSetDataEditSend);

extern Err MsrSetFlexibleField(UInt refNum, char setting[MAX_FFLD_NUM][MAX_FFLD_LEN])
				SYS_TRAP(msrLibTrapSetFlexibleField);

extern Err MsrGetDataBuffer(UInt refNum, MSRCardInfo_Ptr userCardInfoP, Byte get_Type)
				SYS_TRAP(msrLibTrapGetDataBuffer);

extern Err MsrReadMSRBuffer(UInt refNum, MSRCardInfo_Ptr userCardInfoP, UInt waitTime)
				SYS_TRAP(msrLibTrapReadMSRBuffer);

extern Err MsrReadMSRUnbuffer(UInt refNum, MSRCardInfo_Ptr cardInfo)
				SYS_TRAP(msrLibTrapReadMSRUnbuffer);

extern Err MsrSetDefault(UInt refNum)
				SYS_TRAP(msrLibTrapSetDefault);

extern	Err MsrSetDecoderMode(UInt refNum, Byte setting)
				SYS_TRAP(msrLibTrapSetDecoderMode);

extern	Err MsrSetTrackFormat(UInt refNum, Byte track_ID, Byte format, 
		Byte SS_Bits, Byte SS_ASCII, Byte ES_Bits, Byte ES_ASCII)
				SYS_TRAP(msrLibTrapSetTrackFormat);

extern	Err MsrSetReservedChar(UInt refNum, ReservedChar_Ptr setting)
				SYS_TRAP(msrLibTrapSetReservedChar);


#ifdef __cplusplus 
}
#endif

// character setting definition
#define	BS				0x08
#define	STX				0x02
#define	ETX				0x03
#define	ACK				0x06
#define NAK				0x0F
#define WAKE			0x10

#define	LF				0x0A
#define	CR				0x0D
#define	DC1				0X11
#define	DC3				0X13

#define	MsrSendCmd		'S'
#define MsrReceiveCmd	'R'

// setting constant
#define	MsrUnbufferedMode	'0'
#define	MsrBufferedMode		'1'

#define	MsrArmtoReadMode	'0'
#define	MsrClearBufferMode	'1'

#define	MsrTerminatorCRLF	'0'
#define	MsrTerminatorCR		'1'
#define	MsrTerminatorLF		'2'
#define	MsrTerminatorNone	'3'

#define MsrAnyTrack			'0'
#define	MsrTrack1Only		'1'
#define	MsrTrack2Only		'2'
#define	MsrTrack1Track2		'3'
#define	MsrTrack3Only		'4'
#define	MsrTrack1Track3		'5'
#define	MsrTrack2Track3		'6'
#define	MsrAllThreeTracks	'7'

#define	MsrNoLRC			'0'
#define	MsrSendLRC			'1'

#define	MsrDisableDataEdit	'0'
#define	MsrDataEditMatch	'1'
#define	MsrDataEditUnmatch	'3'

#define	MsrGetAllTracks		'0'
#define	MsrGetTrack1		'1'
#define	MsrGetTrack2		'2'
#define	MsrGetTrack3		'3'

#define	MsrStatus			'0'
#define MsrDiagnose			'1'
#define	MsrVersionNo		'2'
#define	MsrBufferMode		'3'

#define	MsrNormalDecoder	'0'
#define MsrGenericDecoder	'1'
#define	MsrRawDataDecoder	'2'

#define	MsrTrack1Format		'1'
#define MsrTrack2Format		'2'
#define	MsrTrack3Format		'3'

#define	Msr5BitsFormat		'0'
#define Msr7BitsFormat		'1'

// track ID for raw data
#define	MsrTrack1ID			1
#define	MsrTrack2ID			2
#define	MsrTrack3ID			3

// function id definition
#define	MsrSetTrackSelectionFunID	0x13
#define	MsrSetTrackSeparatorFunID	0x17
#define	MsrSetDefaultFunID			0x18
#define	MsrSetBufferModeFunID		0x19
#define	MsrArmtoReadFunID			0x1A
#define	MsrSetDataEditFunID			0x1B
#define	MsrSetLRCFunID				0x1C
#define	MsrGetSettingFunID			0x1F
#define	MsrSetTerminatorFunID		0x21
#define	MsrGetDataFunID				0x22
#define	MsrGetStatusFunID			0x23
#define	MsrTrack1FormatFunID		0x24
#define	MsrTrack2FormatFunID		0x25
#define	MsrTrack3FormatFunID		0x26
#define	MsrSpecialCharFunID			0x27
#define	MsrSetDecoderModeFunID		0x28
#define	MsrSetPreambleFunID			0xD2
#define	MsrSetPostambleFunID		0xD3
#define	MsrSetFlexFieldFunID		0xF1
#define	MsrDataEditSendFunID		0xF2
#define	MsrSetAddedFieldFunID		0xF3

#define	MsrCCSMDFunID				0xE0
#define	MsrDMVSMDFunID				0xE1
#define	MsrAAMVASMDFunID			0xE2
#define	MsrFLEXSMDFunID				0xE3
#define	MsrLengthMatchFunID			0xF0
#define	MsrStringMatchFunID			0xF1
#define	MsrSearchBeforeFunID		0xF2
#define	MsrSearchBetweenFunID		0xF3
#define	MsrSearchAfterFunID			0xF4

#define	MsrStatusMask				0xC0	//11000000b
#define	MsrROMStatusERR				0X00	//00000000b
#define	MsrRAMStatusERR				0X40	//01000000b
#define	MsrEEPROMStatusERR			0X80	//10000000b
#define	MsrStatusOK					0XC0	//11000000b

#endif	// __MSR_MGR_LIB_H__
