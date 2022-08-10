/************************************************************************
* COPYRIGHT:   Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE:        ScanMgrDef.h
*
* SYSTEM:      Symbol barcode scanner for Palm III.
*
* HEADER:      Scan Manager common defines.
*
* DESCRIPTION: Provies all common defines for such things as opcode values,
*              error codes and parameter values for scan manager functions.
*
* HISTORY:     4/13/98    SS   Created
*              ...
*************************************************************************/
#pragma once

/*******************************************************************
 *  Symbol unit hardware token (checked in ScanIsPalmSymbolUnit)
 *******************************************************************/
#define SymbolROMToken 'scnr'

/*******************************************************************
 *                    Range Values
 *******************************************************************/
#define MIN_BEEP_DURATION 			0
#define MAX_BEEP_DURATION 			10000
#define MIN_BEEP_FREQUENCY 			0
#define MAX_BEEP_FREQUENCY 			15000
#define MIN_UPCEAN_REDUNDANCY 		2
#define MAX_UPCEAN_REDUNDANCY 		20
#define MIN_LASER_ON_TIME 			5
#define MAX_LASER_ON_TIME 			99
#define MAX_AIM_DURATION 			99
#define MAX_TIMEOUT_BETWEEN_SYMBOL 	10
#define MAX_DECODE_LED_ON_TIME 		100


/*******************************************************************
 *                    Enumerated Types
 *******************************************************************/

// BarType is used with several API functions to indicate which barcode type
// you wish to operate on. The functions that use BarType are:
// 		ScanSetBarcodeEnabled, SetBarcodeLengths, SetUpcPreamble, SetTransmitCheckDigit
typedef enum tagBarType
{
	barCODE39		= 0x00,
	barUPCA			= 0x01,
	barUPCE 			= 0x02,
	barEAN13			= 0x03,
	barEAN8			= 0x04,
	barD25			= 0x05,
	barI2OF5			= 0x06,
	barCODABAR		= 0x07,
	barCODE128		= 0x08,
	barCODE93		= 0x09,
	barTRIOPTIC39 	= 0x0D,
	barUCC_EAN128	= 0x0E,
	barMSI_PLESSEY = 0x0B,
	barUPCE1			= 0x0C,
	barBOOKLAND_EAN= 0x53,
	barISBT128		= 0x54,
	barCOUPON		= 0x55
} BarType;

// BeepType defines various beep patterns you can sound with the following commands:
// 		ScanCmdSendParams, ScanCmdBeep
typedef enum tagBeepType
{
	No_Beep 			= 0x00,
	One_Short_High,
    Two_Short_High,
    Three_Short_High,
    Four_Short_High,
    Five_Short_High,

    One_Short_Low,
    Two_Short_Low,
    Three_Short_Low,
    Four_Short_Low,
    Five_Short_Low,

    One_Long_High,
    Two_Long_High,
    Three_Long_High,
    Four_Long_High,
    Five_Long_High,

    One_Long_Low,
    Two_Long_Low,
    Three_Long_Low,
    Four_Long_Low,
    Five_Long_Low,

    Fast_Warble,
    Slow_Warble,
    Mix1,
    Mix2,
    Mix3,
    Mix4,

   	Decode_Beep,
   	Bootup_Beep,
   	Parameter_Entry_Error_Beep,
   	Parameter_Defaults_Beep,
   	Parameter_Entered_Beep,
   	Host_Convert_Error_Beep,
   	Transmit_Error_Beep,
   	Parity_Error_Beep,
   	
   	Last_Beep
} BeepType;

// ConvertType specifieds the various barcode conversions that can be 
// enabled/disabled by the function "ScanSetConvert".
typedef enum tagConvertType
{
	convertUpcEtoUpcA,
	convertUpcE1toUpcA,
	convertCode39toCode32,
	convertEan8toEan13,
	convertI2of5toEan13
	
} ConvertType; 

// Events that will be passed to the application.
typedef enum tagScanMgrEvent
{
	scanDecodeEvent	= 0x7fff+0x800,	        // A decode has finished (valid or invalid decode)
	scanBatteryErrorEvent,					// Battery too low to perform scan
	scanTriggerEvent 						// A scan attempt was initiated - hard or soft trigger
} ScanMgrEvent;

// Param Types - internal
#define	SHORT_BEEP_DUR		0x20	
#define	MEDIUM_BEEP_DUR	0x21
#define	LONG_BEEP_DUR		0x22
#define	HIGH_FREQ			0x23
#define	MEDIUM_FREQ			0x24
#define	LOW_FREQ				0x25
#define	DECODE_BEEP_DUR	0x27
#define	DECODE_BEEP_FREQ	0x28

// Types of durations that can be set with ScanSetBeepDuration
typedef enum
{
	decodeDuration = DECODE_BEEP_DUR,
	shortDuration = SHORT_BEEP_DUR,
	mediumDuration = MEDIUM_BEEP_DUR,
	longDuration = LONG_BEEP_DUR
} DurationType;

// Types of frequencies that can be set with ScanSetBeepFrequency
typedef enum
{
	decodeFrequency = DECODE_BEEP_FREQ,
	lowFrequency = LOW_FREQ,
	mediumFrequency = MEDIUM_FREQ,
	highFrequency = HIGH_FREQ
} FrequencyType;

/*******************************************************************
 * Return Codes that come back from the various Scan Mgr API calls
 *******************************************************************/
#define STATUS_OK                                0
#define NOT_SUPPORTED                           -2
#define COMMUNICATIONS_ERROR                    -3
#define BAD_PARAM                               -4
#define BATCH_ERROR                             -5
#define ERROR_UNDEFINED                         -6
                                                

/********************************************************************
 * Parameter values for various Scan Mgr API calls
 ********************************************************************/
// triggering modes
    #define LEVEL                                       0x00
    #define PULSE                                       0x02
    #define HOST                                        0x08

// enable or disable for various params	
    #define DISABLE                                     0x00
    #define ENABLE                                      0x01

// Linear code type security
    #define SECURITY_LEVEL0                             0x00
    #define SECURITY_LEVEL1                             0x01
    #define SECURITY_LEVEL2                             0x02
    #define SECURITY_LEVEL3                             0x03
    #define SECURITY_LEVEL4                             0x04

// UPC/EAN Supplementals
    #define IGNORE_SUPPLEMENTALS                        0x00
    #define DECODE_SUPPLEMENTALS                        0x01
    #define AUTODISCRIMINATE_SUPPLEMENTALS              0x02

// Transmit Check Digit options
    #define DO_NOT_TRANSMIT_CHECK_DIGIT                 0x00
    #define TRANSMIT_CHECK_DIGIT                        0x01

// Preamble options
    #define NO_PREAMBLE                                 0x00
    #define SYSTEM_CHARACTER                            0x01
    #define SYSTEM_CHARACTER_COUNTRY_CODE               0x02

// Length types for the barcode SetLengths calls
    #define ANY_LENGTH                                  0x00
    #define ONE_DISCRETE_LENGTH                         0x01
    #define TWO_DISCRETE_LENGTHS                        0x02
    #define LENGTH_WITHIN_RANGE                         0x03

// CheckDigit verification options
	#define 	DISABLE_CHECK_DIGIT 					0x00
    #define     USS_CHECK_DIGIT                         0x01
    #define     OPCC_CHECK_DIGIT                        0x02

// MSI Plessey checkdigit options
     #define    ONE_CHECK_DIGIT                         0x00
     #define    TWO_CHECK_DIGITS                        0x01

// MSI Plessey check digit algorithms
     #define    MOD10_MOD11                             0x00
     #define    MOD10_MOD10                             0x01

// Transmit Code ID Character options
    #define AIM_CODE_ID_CHARACTER                       0x01    
    #define SYMBOL_CODE_ID_CHARACTER                    0x02

// Prefix/Suffix value options
	#define PREFIX_SUFFIX_VALUES_P                  0x69
	#define PREFIX_SUFFIX_VALUES_S1                 0x68
	#define PREFIX_SUFFIX_VALUES_S2                 0x6A

// Scan data transmission formats
    #define DATA_AS_IS                                  0x00
    #define DATA_SUFFIX1                                0x01
    #define DATA_SUFFIX2                                0x02
    #define DATA_SUFFIX1_SUFFIX2                        0x03
    #define PREFIX_DATA                                 0x04
    #define PREFIX_DATA_SUFFIX1                         0x05
    #define PREFIX_DATA_SUFFIX2                         0x06
    #define PREFIX_DATA_SUFFIX1_SUFFIX2                 0x07

// Scan angle options
	#define SCAN_ANGLE_WIDE 					0xB6
	#define SCAN_ANGLE_NARROW					0xB5

// barcode data types returned along with the decode data...
    #define BCTYPE_NOT_APPLICABLE                       0x00
    #define BCTYPE_CODE39                               0x01
    #define BCTYPE_CODABAR                              0x02
    #define BCTYPE_CODE128                              0x03
    #define BCTYPE_D2OF5                                0x04
    #define BCTYPE_IATA2OF5                             0x05
    #define BCTYPE_I2OF5                                0x06
    #define BCTYPE_CODE93                               0x07
    #define BCTYPE_UPCA                                 0x08
    #define BCTYPE_UPCA_2SUPPLEMENTALS                  0x48
    #define BCTYPE_UPCA_5SUPPLEMENTALS                  0x88
    #define BCTYPE_UPCE0                                0x09
    #define BCTYPE_UPCE0_2SUPPLEMENTALS                 0x49
    #define BCTYPE_UPCE0_5SUPPLEMENTALS                 0x89
    #define BCTYPE_EAN8                                 0x0A
    #define BCTYPE_EAN8_2SUPPLEMENTALS                  0x4A
    #define BCTYPE_EAN13_5SUPPLEMENTALS                 0x8B
    #define BCTYPE_EAN8_5SUPPLEMENTALS                  0x8A
    #define BCTYPE_EAN13                                0x0B
    #define BCTYPE_EAN13_2SUPPLEMENTALS                 0x4B
    #define BCTYPE_MSI_PLESSEY                          0x0E
    #define BCTYPE_EAN128                               0x0F
    #define BCTYPE_UPCE1                                0x10
    #define BCTYPE_UPCE1_2SUPPLEMENTALS                 0x50
    #define BCTYPE_UPCE1_5SUPPLEMENTALS                 0x90
    #define BCTYPE_CODE39_FULL_ASCII                    0x13
    #define BCTYPE_TRIOPTIC_CODE39                      0x15
    #define BCTYPE_BOOKLAND_EAN                         0x16
    #define BCTYPE_COUPON_CODE                          0x17
    #define BCTYPE_ISBT128                              0x19
    #define BCTYPE_CODE32                               0x20

// custom launch codes for the Scan Manager software:
typedef enum {
	myAppCmdBatteryAlert = sysAppLaunchCmdCustomBase, 
	myAppCmdNotUsed
} MyAppCustomActionCodes;


/*******************************************************************
 *              end of file                                        *
 *******************************************************************/
