/************************************************************************
* COPYRIGHT:   Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE:        ScanMgr.h
*
* SYSTEM:      Symbol barcode scanner for Palm III.
*
* HEADER:      Scan Manager Library Header
*
* DESCRIPTION: Provides function declarations and other types for
*					use by a scanner-aware application.
*
* HISTORY:     4/13/98    SS   Created
*              ...
*************************************************************************/
#pragma once
#include "ScanMgrDef.h"

/*******************************************************************
 * 			  Function to check if we're on a PalmSymbol device
 *******************************************************************/
int ScanIsPalmSymbolUnit();

/*******************************************************************
 *   Functions for opening and closing the decoder (required!)     *
 *******************************************************************/
int ScanOpenDecoder();
int ScanCloseDecoder();

/*******************************************************************
 *            Parameters send/retrieve functions                   *
 *******************************************************************/
int ScanCmdSendParams( BeepType beep );
int ScanCmdGetAllParams( BytePtr pbParams, Word max_length );

/*******************************************************************
 *               Versioning functions                              *
 *******************************************************************/
int ScanGetScanManagerVersion( CharPtr pszVer, Word max_length );
int ScanGetScanPortDriverVersion( CharPtr pszVer, Word max_length );
int ScanGetDecoderVersion( CharPtr pszVer, Word max_length);

/*******************************************************************
 *            Decoder Data Retrieval Function                      *
 *******************************************************************/
int ScanGetDecodedData( MESSAGE *ptr);

/*******************************************************************
 * Functions to enable/disable scanning of various barcode types   *
 *******************************************************************/
int ScanSetBarcodeEnabled( BarType barcodeType, Boolean bEnable );
int ScanGetBarcodeEnabled( BarType barcodeType );

/*******************************************************************
 *   Functions for get/set the barcode lengths to be scanned       *
 *******************************************************************/
int ScanSetBarcodeLengths( BarType barcodeType, Word lengthType, Word length1, Word length2 );
int ScanGetBarcodeLengths( BarType barcodeType, WordPtr pLengthType, WordPtr pLength1, WordPtr pLength2 );

/*******************************************************************
 *                 Preamble functions...                           *
 *******************************************************************/
int ScanSetUpcPreamble( BarType barcodeType, int preamble);
int ScanGetUpcPreamble( BarType barcodeType);

/*******************************************************************
 *              Prefix/Suffix functions...                         *
 *******************************************************************/
int ScanSetPrefixSuffixValues( Char prefix, Char suffix_1, Char suffix_2 );
int ScanGetPrefixSuffixValues( CharPtr pPrefix, CharPtr pSuffix_1, CharPtr pSuffix_2 );

int ScanSetCode32Prefix( Boolean bEnable );
int ScanGetCode32Prefix();

/*******************************************************************
 * Functions to get/set which barcode conversions are in effect    *
 *******************************************************************/
int ScanSetConvert( ConvertType conversion, Boolean bEnable);
int ScanGetConvert( ConvertType conversion);


/*******************************************************************
 *           Check Digit setup functions                           *
 *******************************************************************/
int ScanSetTransmitCheckDigit( BarType barType, Word check_digit );
int ScanGetTransmitCheckDigit( BarType barType );

int ScanSetCode39CheckDigitVerification( Word check_digit );
int ScanGetCode39CheckDigitVerification();

int ScanSetI2of5CheckDigitVerification( Word check_digit );
int ScanGetI2of5CheckDigitVerification();

int ScanSetMsiPlesseyCheckDigits( Word check_digits );
int ScanGetMsiPlesseyCheckDigits();

int ScanSetMsiPlesseyCheckDigitAlgorithm( Word algorithm );
int ScanGetMsiPlesseyCheckDigitAlgorithm();

/*******************************************************************
 *              Supplemental/Redundancy Functions...               *
 *******************************************************************/
int ScanSetDecodeUpcEanSupplementals(  Word supplementals );
int ScanGetDecodeUpcEanSupplementals();

int ScanSetDecodeUpcEanRedundancy(  Word supplemental_redundancy );
int ScanGetDecodeUpcEanRedundancy();

/*******************************************************************
 *              Miscellaneous Functions...                         *
 *******************************************************************/

int ScanSetCode39FullAscii( Boolean bEnable );
int ScanGetCode39FullAscii();

int ScanSetClsiEditing( Boolean bEnable );
int ScanGetClsiEditing();

int ScanSetNotisEditing( Boolean bEnable );
int ScanGetNotisEditing();

int ScanSetUpcEanSecurityLevel( Word security_level );
int ScanGetUpcEanSecurityLevel();

int ScanSetEanZeroExtend( Boolean bEnable );
int ScanGetEanZeroExtend();

int ScanSetHostSerialResponseTimeOut( Word time_out );
int ScanGetHostSerialResponseTimeOut();


/*******************************************************************
 *              Decoder Command Functions...                       *
 *******************************************************************/
// Stu 4/16/98: Took out - this has been renamed to ScanGetDecodedData()
// int ScanCmdDecodeData( MESSAGE *ptr );
int ScanCmdParamDefaults();

int ScanCmdScanEnable();
int ScanCmdScanDisable();

int ScanCmdStartDecode();
int ScanCmdStopDecode();

int ScanCmdLedOn();
int ScanCmdLedOff();

int ScanCmdAimOn();
int ScanCmdAimOff();

int ScanGetAimMode();
int ScanGetScanEnabled();
int ScanGetLedState();

/*******************************************************************
 *              Decoder Hardware Functions...                      * 
 *******************************************************************/
int ScanSetLaserOnTime( Word laser_on_time);
int ScanGetLaserOnTime();

int ScanSetDecodeLedOnTime( Word led_on_time);
int ScanGetDecodeLedOnTime();

int ScanSetAngle( Word scan_angle );
int ScanGetAngle();

int ScanSetAimDuration( Word aim_duration);
int ScanGetAimDuration();

int ScanSetTriggeringModes( Word triggering_mode);
int ScanGetTriggeringModes();

int ScanSetTimeOutBetweenSameSymbol( Word time_out );
int ScanGetTimeOutBetweenSameSymbol();

int ScanSetLinearCodeTypeSecurityLevel( Word security_level );
int ScanGetLinearCodeTypeSecurityLevel();

int ScanSetBidirectionalRedundancy( Word redundancy );
int ScanGetBidirectionalRedundancy();

int ScanSetTransmitCodeIdCharacter( Word code_id );
int ScanGetTransmitCodeIdCharacter();

int ScanSetScanDataTransmissionFormat( Word transmission_format );
int ScanGetScanDataTransmissionFormat();

/*******************************************************************
 *                 Beeper Functions...                             * 
 *******************************************************************/
int ScanCmdBeep( BeepType beep );

int ScanSetBeepAfterGoodDecode( Boolean bEnableBeep );
int ScanGetBeepAfterGoodDecode();

int ScanSetBeepFrequency( FrequencyType type, int beep_freq );
int ScanGetBeepFrequency( FrequencyType type );

int ScanSetBeepDuration( DurationType type, int beep_duration );
int ScanGetBeepDuration( DurationType type );
