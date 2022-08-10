/************************************************************************
* COPYRIGHT:   Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE: 		SetupDlgs.c
*
* SYSTEM: 		Symbol barcode scanner for Palm III.
* 
* MODULE: 		Setup and configuration dialog support functions.
*              
* DESCRIPTION: 	Form-handling code for the scan-parameter forms in the
* 				ScanDemo application.  In general, each of these forms 
* 				displays a group of decoder settings and allows the user
* 				to modify them.
*
* FUNCTIONS: 	Too many to list here.
* 				In general, each form has the following functions:
* 					On<FormName>Setup 		- Issues a FrmGotoForm
* 					<FormName>FormOnInit 	- Initializes controls on form
* 					<FormName>SendToDecoder - Sends users choices to scanner
* 					<FormName>SetupHandleEvent - event handler for the form
*
* HISTORY: 		4/13/98    SS   Created
* 
*************************************************************************/
#include <Pilot.h>
#include "SetupDlgs.h"
#include "ScanDemoRsc.h" 		// include our project's resource ids
#include "ScanMgrStruct.h" 		// include scan manager structure defs
#include "ScanMgrDef.h" 		// include scan manager constant defs
#include "ScanMgr.h" 			// include scan manager API function defs

#include "Utils.h" 			// include utility functions used throughout

static void HardwareSendToDecoder();
static void CodeFormatSendToDecoder();
static void UPCSendToDecoder();
static void UPCMoreSendToDecoder();
static void Code128SendToDecoder();
static void Code39SendToDecoder();
static void MoreCode39SendToDecoder();
static void Code93SendToDecoder();
static void I2of5SendToDecoder();
static void D2of5SendToDecoder();
static void CodabarSendToDecoder();
static void MSIPlesseySendToDecoder();
int ConvertCharToFrequency( CharPtr pChar);
int ConvertCharToDuration( CharPtr pChar);
void DoBeepTest();


/***********************************************************************
 ***********************************************************************
   Functions for the Hardware Setup form
 ***********************************************************************   
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnHardwareSetup
 *
 * DESCRIPTION: 	Makes HardwareForm the active form.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnHardwareSetup()
{
	FrmGotoForm( HardwareForm );
}

/***********************************************************************
 *
 * FUNCTION: 		HardwareFormOnInit
 *
 * DESCRIPTION: 	Initializes controls on the Hardware Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void HardwareFormOnInit()
{
	ControlPtr pCtl;
	ListPtr pList;
	CharPtr label;
	FormPtr pForm = FrmGetActiveForm();
	char chrValue[4];
	int nLaserOnTime = 0;
	int nAimDuration = 0;
	int nDecodeLedOnTime = 0;
	int SecurityLevel = 0;
	int nScanAngle = 0;
	
	MemSet(chrValue, sizeof(chrValue), 0);
		
	// Initialize the controls with current decoder param settings
	pCtl = GetObjectPtr(HardwareBeepOnGoodDecodeCheckbox);
	CtlSetValue( pCtl, ScanGetBeepAfterGoodDecode() );
	pCtl = GetObjectPtr(HardwareRedundancyCheckbox);
	CtlSetValue( pCtl, ScanGetBidirectionalRedundancy() );

	nLaserOnTime = ScanGetLaserOnTime();
	if (nLaserOnTime <= MAX_LASER_ON_TIME)
	{
		StrIToA( chrValue, nLaserOnTime);
		SetFieldText(HardwareLaserOnTimeField, chrValue, 4, false);
	}

	nAimDuration = ScanGetAimDuration();
	if (nAimDuration <= MAX_AIM_DURATION)
	{
		StrIToA( chrValue, nAimDuration);
		SetFieldText(HardwareAimDurationField, chrValue, 4, false);
	}

	nDecodeLedOnTime = ScanGetDecodeLedOnTime();
	if (nDecodeLedOnTime <= MAX_DECODE_LED_ON_TIME)
	{
		StrIToA( chrValue, nDecodeLedOnTime);
		SetFieldText(HardwareDecodeLEDOnTimeField, chrValue, 4, false);
 	}
 	
 	// init the Scan Angle trigger and list
 	nScanAngle = ScanGetAngle();
 	if (nScanAngle == SCAN_ANGLE_NARROW)
 		nScanAngle = 0;
 	else nScanAngle = 1;
 	pList = GetObjectPtr( HardwareScanAngleList );
	LstSetSelection (pList, nScanAngle);
	label = LstGetSelectionText (pList, nScanAngle);
	pCtl = GetObjectPtr (HardwareScanAnglePopTrigger);
	CtlSetLabel (pCtl, label);
	
	// init the LinearSecurity trigger value
	SecurityLevel = ScanGetLinearCodeTypeSecurityLevel(); // 1=Level1,..4=Level 4
	SecurityLevel -= 1; // convert from decoder value to zero-based list value
	pList = GetObjectPtr( HardwareLinearSecurityList );
	LstSetSelection (pList, SecurityLevel);
	label = LstGetSelectionText (pList, SecurityLevel);
	pCtl = GetObjectPtr (HardwareLinearSecurityPopTrigger);
	CtlSetLabel (pCtl, label);

	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION: 		HardwareSendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void HardwareSendToDecoder()
{
	ControlPtr pCtl;
	FieldPtr pFld;
	ListPtr pList;
	char * pChrValue;
	int nValue = 0;
	int SecurityLevel = 0;
	int nScanAngle = 0;
	
	// commit the users selections -- send params to the decoder
	pCtl = GetObjectPtr(HardwareBeepOnGoodDecodeCheckbox);
	ScanSetBeepAfterGoodDecode( CtlGetValue(pCtl) );
	pCtl = GetObjectPtr(HardwareRedundancyCheckbox);
	ScanSetBidirectionalRedundancy(CtlGetValue(pCtl) );
	
	pFld = GetObjectPtr(HardwareLaserOnTimeField);
	pChrValue = FldGetTextPtr(pFld);
	if (pChrValue)
	{
		nValue = StrAToI(pChrValue);
		if (nValue > MAX_LASER_ON_TIME)
			nValue = MAX_LASER_ON_TIME;
		else if (nValue < MIN_LASER_ON_TIME)
			nValue = MIN_LASER_ON_TIME;
		ScanSetLaserOnTime(nValue); 
	}

	pFld = GetObjectPtr(HardwareAimDurationField);
	pChrValue = FldGetTextPtr(pFld);
	if (pChrValue)
	{
		nValue = StrAToI(pChrValue);
		if (nValue > MAX_AIM_DURATION)
			nValue = MAX_AIM_DURATION;
		ScanSetAimDuration(nValue); 
	}
	
	pFld = GetObjectPtr(HardwareDecodeLEDOnTimeField);
	pChrValue = FldGetTextPtr(pFld);
	if (pChrValue)
	{
		nValue = StrAToI(pChrValue);
		if (nValue > MAX_DECODE_LED_ON_TIME)
			nValue = MAX_DECODE_LED_ON_TIME;
		ScanSetDecodeLedOnTime(nValue); 
	}

	// commit the LinearSecurity trigger value
	pList = GetObjectPtr( HardwareLinearSecurityList );
	SecurityLevel = LstGetSelection (pList); // 0=Level1, .., 3=Level4
	SecurityLevel += 1; // convert to value expected by decoder (1=Level1, ...)
	ScanSetLinearCodeTypeSecurityLevel(SecurityLevel);
	
	pList = GetObjectPtr( HardwareScanAngleList );
	nScanAngle = LstGetSelection (pList);
	if (nScanAngle == 0)
		nScanAngle = SCAN_ANGLE_NARROW;
	else nScanAngle = SCAN_ANGLE_WIDE;
	ScanSetAngle(nScanAngle);
	
	ScanCmdSendParams(No_Beep);
}


/***********************************************************************
 *
 * FUNCTION: 		HardwareSetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the Hardware Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean HardwareSetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == HardwareCancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == HardwareOKButton )
			{
				HardwareSendToDecoder();				
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			break;

		case frmOpenEvent:
			HardwareFormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle(HardwareLaserOnTimeField);
			FreeFieldHandle(HardwareAimDurationField);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   Code Format Setup Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnCodeFormatSetup
 *
 * DESCRIPTION: 	Makes CodeFormatForm the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnCodeFormatSetup()
{
	FrmGotoForm( CodeFormatForm );
}

/***********************************************************************
 *
 * FUNCTION: 		CodeFormatFormOnInit
 *
 * DESCRIPTION: 	Initializes controls on the Code Format Setup form by 
 * 					using the Scan Mgr API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void CodeFormatFormOnInit()
{
	int CodeId = 0;
	int TxFormat = 0;
	CharPtr label;
	Byte byPrefix, bySuffix1, bySuffix2;
	char chrPrefix[4];
	char chrSuffix1[4];
	char chrSuffix2[4];
	ListPtr pList;
	ControlPtr pCtl;
	FormPtr pForm = FrmGetActiveForm();
	MemSet(chrPrefix, sizeof(chrPrefix), 0);
	MemSet(chrSuffix1, sizeof(chrSuffix1), 0);
	MemSet(chrSuffix2, sizeof(chrSuffix2), 0);

	//
	// Initialize the controls with current decoder param settings
	//
	
	// init the Code ID Char trigger value
	CodeId = ScanGetTransmitCodeIdCharacter(); // 0=None, 1=Aim, 2=Symbol
	pList = GetObjectPtr( CodeFormatCodeIDCharList );
	LstSetSelection (pList, CodeId);
	label = LstGetSelectionText (pList, CodeId);
	pCtl = GetObjectPtr (CodeFormatCodeIDCharPopTrigger);
	CtlSetLabel (pCtl, label);
	
	// init the TxFormat trigger value
	TxFormat = ScanGetScanDataTransmissionFormat(); // 0= As Is,1=Suffix1, ...7=PrefixDataS1S2
	pList = GetObjectPtr( CodeFormatTxFormatList);
	LstSetSelection (pList, TxFormat);
	label = LstGetSelectionText (pList, TxFormat);
	pCtl = GetObjectPtr (CodeFormatTxFormatPopTrigger);
	CtlSetLabel( pCtl, label);
	
	// init the Prefix, Suffix1 and Suffix2 fields
	ScanGetPrefixSuffixValues((char*)&byPrefix, (char*)&bySuffix1, (char*)&bySuffix2);
	StrIToA( chrPrefix, byPrefix);
	StrIToA( chrSuffix1, bySuffix1);
	StrIToA( chrSuffix2, bySuffix2);
	SetFieldText( CodeFormatPrefixField, chrPrefix, 3, false);  
	SetFieldText( CodeFormatSuffix1Field, chrSuffix1, 3, false);  
	SetFieldText( CodeFormatSuffix2Field, chrSuffix2, 3, false);  

	FrmDrawForm( pForm );	
}


/***********************************************************************
 *
 * FUNCTION: 		CodeFormatSendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void CodeFormatSendToDecoder()
{
	FieldPtr pFld;
	ListPtr pList;
	int CodeId;
	int TxFormat;
	Byte byPrefix = 0, bySuffix1 = 0, bySuffix2 = 0;
	CharPtr pChrValue = NULL;
	
	// commit the users selections -- send params to the decoder
	// set the prefix/suffix characters.
	pFld = GetObjectPtr(CodeFormatPrefixField);
	pChrValue = FldGetTextPtr(pFld);
	if (pChrValue)
		byPrefix = StrAToI(pChrValue);
		
	pFld = GetObjectPtr(CodeFormatSuffix1Field);
	pChrValue = FldGetTextPtr(pFld);
		bySuffix1 = (Byte)StrAToI(pChrValue);
		
	pFld = GetObjectPtr(CodeFormatSuffix2Field);
	pChrValue = FldGetTextPtr(pFld);
	if (pChrValue)
		bySuffix2 = (Byte)StrAToI(pChrValue);
		
	ScanSetPrefixSuffixValues(byPrefix, bySuffix1, bySuffix2);
	
	// set the Code ID Character
	pList = GetObjectPtr( CodeFormatCodeIDCharList );
	CodeId = LstGetSelection(pList);
	ScanSetTransmitCodeIdCharacter(CodeId);

	// set the TxFormat
	pList = GetObjectPtr( CodeFormatTxFormatList );
	TxFormat = LstGetSelection(pList);
	ScanSetScanDataTransmissionFormat(TxFormat);
	
	// send all the params we've set to the decoder
	ScanCmdSendParams( No_Beep);
}


/***********************************************************************
 *
 * FUNCTION: 		CodeFormatSetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the Code Format Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean CodeFormatSetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == CodeFormatCancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == CodeFormatOKButton )
			{
				// commit the users selections -- send params to the decoder
				CodeFormatSendToDecoder();
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			break;

		case frmOpenEvent:
			CodeFormatFormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle(CodeFormatPrefixField);
			FreeFieldHandle(CodeFormatSuffix1Field);
			FreeFieldHandle(CodeFormatSuffix2Field);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}


/***********************************************************************
 ***********************************************************************
   UPC/EAN Setup Dialog
 ***********************************************************************
 ***********************************************************************/

/***********************************************************************
 *
 * FUNCTION: 		OnUPCSetup
 *
 * DESCRIPTION: 	Makes UPCForm the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnUPCSetup()
{
	FrmGotoForm( UPCForm );
}

/***********************************************************************
 *
 * FUNCTION: 		UPCFormOnInit
 *
 * DESCRIPTION: 	Initializes the controls on the UPC Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void UPCFormOnInit()
{
	ListPtr pList;
	ControlPtr pCtl;
	CharPtr label;
	int SecurityLevel;
		
	FormPtr pForm = FrmGetActiveForm();

	//
	// Initialize the controls with current decoder param settings
	//
	// init the Security Level trigger value
	SecurityLevel = ScanGetUpcEanSecurityLevel(); // 0=Level0, 1=Level1, etc..
	pList = GetObjectPtr( UPCSecurityLevelList );
	LstSetSelection (pList, SecurityLevel);
	label = LstGetSelectionText (pList, SecurityLevel);
	pCtl = GetObjectPtr (UPCSecurityLevelPopTrigger);
	CtlSetLabel (pCtl, label);
	
	// init the barcode type checkboxes
	pCtl = GetObjectPtr( UPCUPCACheckbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barUPCA) );
	pCtl = GetObjectPtr( UPCUPCECheckbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barUPCE) );
	pCtl = GetObjectPtr( UPCUPCE1Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barUPCE1) );
	pCtl = GetObjectPtr( UPCEAN13Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barEAN13) );
	pCtl = GetObjectPtr( UPCEAN8Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barEAN8) );
	pCtl = GetObjectPtr( UPCBooklandCheckbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barBOOKLAND_EAN) );
	// init the barcode conversion checkboxes
	pCtl = GetObjectPtr( UPCUPCEACheckbox );
	CtlSetValue(pCtl, ScanGetConvert(convertUpcEtoUpcA) );
	pCtl = GetObjectPtr( UPCUPCE1ACheckbox );
	CtlSetValue(pCtl, ScanGetConvert(convertUpcE1toUpcA) );
	pCtl = GetObjectPtr( UPCEAN0ExtCheckbox );
	CtlSetValue(pCtl, ScanGetEanZeroExtend() );
	pCtl = GetObjectPtr( UPCEAN813Checkbox );
	CtlSetValue(pCtl, ScanGetConvert(convertEan8toEan13) );
	pCtl = GetObjectPtr( UPCCouponCheckbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barCOUPON) );

	// draw the form
	FrmDrawForm( pForm );
	
}

/***********************************************************************
 *
 * FUNCTION: 		UPCSendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void UPCSendToDecoder()
{
	int SecurityLevel = 0;
	ControlPtr pCtl;
	ListPtr pList;
	
	// Set the decoder settings based on users choices... 
	// enable barcode types as requested
	pCtl = GetObjectPtr( UPCUPCACheckbox );
	ScanSetBarcodeEnabled(barUPCA, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCUPCECheckbox );
	ScanSetBarcodeEnabled(barUPCE, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCUPCE1Checkbox );
	ScanSetBarcodeEnabled(barUPCE1, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCEAN13Checkbox );
	ScanSetBarcodeEnabled(barEAN13, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCEAN8Checkbox );
	ScanSetBarcodeEnabled(barEAN8, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCBooklandCheckbox );
	ScanSetBarcodeEnabled(barBOOKLAND_EAN, CtlGetValue(pCtl) );
	// enable conversions as requested
	pCtl = GetObjectPtr( UPCUPCEACheckbox );
	ScanSetConvert( convertUpcEtoUpcA, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCUPCE1ACheckbox );
	ScanSetConvert( convertUpcE1toUpcA, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCEAN0ExtCheckbox );
	ScanSetEanZeroExtend( CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCEAN813Checkbox );
	ScanSetConvert( convertEan8toEan13, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCCouponCheckbox );
	ScanSetBarcodeEnabled(barCOUPON, CtlGetValue(pCtl) );

	// commit the security level
	pList = GetObjectPtr( UPCSecurityLevelList );
	SecurityLevel = LstGetSelection(pList);
	ScanSetUpcEanSecurityLevel(SecurityLevel);

	ScanCmdSendParams( No_Beep );	
}


/***********************************************************************
 *
 * FUNCTION: 		UPCSetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the UPC Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean UPCSetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == UPCCancelButton )
			{
				FrmGotoForm( MainForm );
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == UPCMoreButton )
			{
				UPCSendToDecoder();
				FrmGotoForm( UPCMoreForm );
				bHandled = true;
			}
			break;

		case frmOpenEvent:
			UPCFormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			break;
			
		default:
			break;
	}
	
	return bHandled;
}


/***********************************************************************
 ***********************************************************************
   More UPC/EAN Setup Dialog
 ***********************************************************************
 ***********************************************************************/
/***********************************************************************
 *
 * FUNCTION: 		UPCMoreFormOnInit
 *
 * DESCRIPTION: 	Initialize controls on the MoreUPC Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void UPCMoreFormOnInit()
{
	ControlPtr pCtl;
	ListPtr pList;
	CharPtr label;
	char chrRedundancy[3];
	int nRedundancy; 
	int SupplementalValue = 0;
	int PreambleValue = 0;
	FormPtr pForm = FrmGetActiveForm();
	
	MemSet(chrRedundancy, sizeof(chrRedundancy), 0);
	
	// Initialize the controls with current decoder param settings
	pCtl = GetObjectPtr( UPCMoreUPCACheckbox );
	CtlSetValue(pCtl, ScanGetTransmitCheckDigit(barUPCA) );
	pCtl = GetObjectPtr( UPCMoreUPCECheckbox );
	CtlSetValue(pCtl, ScanGetTransmitCheckDigit(barUPCE) );
	pCtl = GetObjectPtr( UPCMoreUPCE1Checkbox );
	CtlSetValue(pCtl, ScanGetTransmitCheckDigit(barUPCE1) );
	
	nRedundancy = ScanGetDecodeUpcEanRedundancy();
	if (nRedundancy <= MAX_UPCEAN_REDUNDANCY)
	{
		StrIToA( chrRedundancy, nRedundancy);
		SetFieldText(UPCMoreRedundancyField, chrRedundancy, 3, false);  
	}
	
	// initialize Supplementals and Preamble popup lists
	SupplementalValue = ScanGetDecodeUpcEanSupplementals(); // 
	pList = GetObjectPtr( UPCMoreSupplementalsList );
	LstSetSelection (pList, SupplementalValue); // 0=Ignore, 1=Decode, 2=Auto
	label = LstGetSelectionText (pList, SupplementalValue);
	pCtl = GetObjectPtr (UPCMoreSupplementalsPopTrigger);
	CtlSetLabel (pCtl, label);
	
	PreambleValue = ScanGetUpcPreamble( barUPCA); // 
	pList = GetObjectPtr( UPCMoreUPCAPreambleList );
	LstSetSelection (pList, PreambleValue); // 0=No preamble, 1=System, 2=SystemCC
	label = LstGetSelectionText (pList, PreambleValue);
	pCtl = GetObjectPtr (UPCMoreUPCAPreamblePopTrigger);
	CtlSetLabel (pCtl, label);

	PreambleValue = ScanGetUpcPreamble( barUPCE); // 
	pList = GetObjectPtr( UPCMoreUPCEPreambleList );
	LstSetSelection (pList, PreambleValue); // 0=No preamble, 1=System, 2=SystemCC
	label = LstGetSelectionText (pList, PreambleValue);
	pCtl = GetObjectPtr (UPCMoreUPCEPreamblePopTrigger);
	CtlSetLabel (pCtl, label);

	PreambleValue = ScanGetUpcPreamble( barUPCE1); // 
	pList = GetObjectPtr( UPCMoreUPCE1PreambleList );
	LstSetSelection (pList, PreambleValue); // 0=No preamble, 1=System, 2=SystemCC
	label = LstGetSelectionText (pList, PreambleValue);
	pCtl = GetObjectPtr (UPCMoreUPCE1PreamblePopTrigger);
	CtlSetLabel (pCtl, label);
	
	FrmDrawForm( pForm );
}


/***********************************************************************
 *
 * FUNCTION: 		UPCMoreSendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void UPCMoreSendToDecoder()
{
	ControlPtr pCtl;
	FieldPtr pFld;
	ListPtr pList;
	int nRedundancy = 0;
	int SuppValue = 0;
	int PreambleValue = 0;
	CharPtr pChrRedundancy = NULL;
	
	// commit the users selections -- send params to the decoder
	pCtl = GetObjectPtr( UPCMoreUPCACheckbox );
	ScanSetTransmitCheckDigit(barUPCA, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCMoreUPCECheckbox );
	ScanSetTransmitCheckDigit(barUPCE, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( UPCMoreUPCE1Checkbox );
	ScanSetTransmitCheckDigit(barUPCE1, CtlGetValue(pCtl) );

	pFld = GetObjectPtr(UPCMoreRedundancyField);
	pChrRedundancy = FldGetTextPtr(pFld);
	if (pChrRedundancy)
	{
		nRedundancy = StrAToI(pChrRedundancy);
		if (nRedundancy > MAX_UPCEAN_REDUNDANCY)
			nRedundancy = MAX_UPCEAN_REDUNDANCY;
		else if (nRedundancy < MIN_UPCEAN_REDUNDANCY)
			nRedundancy = MIN_UPCEAN_REDUNDANCY;
		ScanSetDecodeUpcEanRedundancy(nRedundancy); 
	}
	
	// commit the Supplementals and Preamble popup lists
	pList = GetObjectPtr( UPCMoreSupplementalsList );
	SuppValue = LstGetSelection(pList);
	ScanSetDecodeUpcEanSupplementals(SuppValue);

	pList = GetObjectPtr( UPCMoreUPCAPreambleList );
	PreambleValue = LstGetSelection(pList);
	ScanSetUpcPreamble(barUPCA, PreambleValue);
	
	pList = GetObjectPtr( UPCMoreUPCEPreambleList );
	PreambleValue = LstGetSelection(pList);
	ScanSetUpcPreamble(barUPCE, PreambleValue);
	
	pList = GetObjectPtr( UPCMoreUPCE1PreambleList );
	PreambleValue = LstGetSelection(pList);
	ScanSetUpcPreamble(barUPCE1, PreambleValue);
	
	ScanCmdSendParams( No_Beep);
}

/***********************************************************************
 *
 * FUNCTION: 		UPCMoreSetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the More UPC Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean UPCMoreSetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == UPCMoreCancelButton )
			{
				FrmGotoForm( MainForm );
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == UPCMoreOKButton )
			{
				UPCMoreSendToDecoder();
				FrmGotoForm( MainForm );
				bHandled = true;
			}
			break;

		case frmOpenEvent:
			UPCMoreFormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle(UPCMoreRedundancyField);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   Code 128 Setup Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnCode128Setup
 *
 * DESCRIPTION: 	Makes Code128Form the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnCode128Setup()
{
	FrmGotoForm( Code128Form );
}

/***********************************************************************
 *
 * FUNCTION: 		Code128FormOnInit
 *
 * DESCRIPTION: 	Initialize controls on the Code128 Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void Code128FormOnInit()
{
	ControlPtr pCtl;
	
	FormPtr pForm = FrmGetActiveForm();

	// Initialize the controls with current decoder param settings
	//   barcode type checkboxes
	pCtl = GetObjectPtr( Code128Code128Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled( barCODE128 ) );
	pCtl = GetObjectPtr( Code128UCCEAN128Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled( barUCC_EAN128 ) );
	pCtl = GetObjectPtr( Code128ISBT128Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled( barISBT128 ) );
	
	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION: 		Code128SendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void Code128SendToDecoder()
{
	ControlPtr pCtl;
	
	// commit the users selections -- send params to the decoder
	// 	enable barcode types as requested
	pCtl = GetObjectPtr( Code128Code128Checkbox );
	ScanSetBarcodeEnabled( barCODE128, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( Code128UCCEAN128Checkbox );
	ScanSetBarcodeEnabled( barUCC_EAN128, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( Code128ISBT128Checkbox );
	ScanSetBarcodeEnabled( barISBT128, CtlGetValue(pCtl) );

	ScanCmdSendParams(No_Beep);
}

/***********************************************************************
 *
 * FUNCTION: 		Code128SetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the XX form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean Code128SetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == Code128CancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == Code128OKButton )
			{
				Code128SendToDecoder();
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			break;

		case frmOpenEvent:
			Code128FormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   Code39 Setup Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnCode39Setup
 *
 * DESCRIPTION: 	Makes Code39Form the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnCode39Setup()
{
	FrmGotoForm( Code39Form );
}

/***********************************************************************
 *
 * FUNCTION: 		Code39FormOnInit
 *
 * DESCRIPTION: 	Initialize controls on the Code39 Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void Code39FormOnInit()
{
	ControlPtr pCtl;
	
	FormPtr pForm = FrmGetActiveForm();
	// Initialize the controls with current decoder param settings
	pCtl = GetObjectPtr( Code39Code39Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barCODE39) );

	// enable conversions as requested
	pCtl = GetObjectPtr( Code39Trioptic39Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barTRIOPTIC39) );
	pCtl = GetObjectPtr( Code39Convert39to32Checkbox );
	CtlSetValue(pCtl, ScanGetConvert(convertCode39toCode32) );
	pCtl = GetObjectPtr( Code39VerifyCheckDigitCheckbox );
	CtlSetValue(pCtl, ScanGetCode39CheckDigitVerification() );
	pCtl = GetObjectPtr( Code39TxCheckDigitCheckbox );
	CtlSetValue(pCtl, ScanGetTransmitCheckDigit( barCODE39 ) );
	pCtl = GetObjectPtr( Code39FullASCIICheckbox );
	CtlSetValue(pCtl, ScanGetCode39FullAscii() );
	pCtl = GetObjectPtr( Code39Code32PrefixCheckbox );
	CtlSetValue(pCtl, ScanGetCode32Prefix() );

	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION: 		Code39SendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void Code39SendToDecoder()
{
	// commit the users selections -- send params to the decoder
	ControlPtr pCtl;
	pCtl = GetObjectPtr( Code39Code39Checkbox );
	ScanSetBarcodeEnabled(barCODE39, CtlGetValue(pCtl) );

	// enable conversions as requested
	pCtl = GetObjectPtr( Code39Trioptic39Checkbox );
	ScanSetBarcodeEnabled(barTRIOPTIC39, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( Code39Convert39to32Checkbox );
	ScanSetConvert(convertCode39toCode32, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( Code39VerifyCheckDigitCheckbox );
	ScanSetCode39CheckDigitVerification( CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( Code39TxCheckDigitCheckbox );
	ScanSetTransmitCheckDigit( barCODE39, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( Code39FullASCIICheckbox );
	ScanSetCode39FullAscii( CtlGetValue(pCtl) ); 
	pCtl = GetObjectPtr( Code39Code32PrefixCheckbox );
	ScanSetCode32Prefix( CtlGetValue(pCtl) );
	
	ScanCmdSendParams(No_Beep);
}


/***********************************************************************
 *
 * FUNCTION: 		Code39SetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the Code 39 Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 						popSelectEvent (for Barcode Length Type)
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean Code39SetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == Code39CancelButton )
			{
				FrmGotoForm( MainForm );
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == Code39MoreButton )
			{
				Code39SendToDecoder();
				FrmGotoForm( MoreCode39Form );
				bHandled = true;
			}
			break;

		case frmOpenEvent:
			Code39FormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			break;
			
		default:
			break;
	}
	
	return bHandled;
}


/***********************************************************************
 ***********************************************************************
   More Code39 Setup Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnMoreCode39Setup
 *
 * DESCRIPTION: 	Makes MoreCode39Form the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnMoreCode39Setup()
{
	FrmGotoForm( MoreCode39Form );
}

/***********************************************************************
 *
 * FUNCTION: 		MoreCode39FormOnInit
 *
 * DESCRIPTION: 	Initialize controls on the More Code39 form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void MoreCode39FormOnInit()
{
	FormPtr pForm = FrmGetActiveForm();
	ListPtr pList;	
	ControlPtr pCtl;
	CharPtr label;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	char chrValue[4];
	MemSet(chrValue, sizeof(chrValue), 0);
	
	// Initialize the controls with current decoder param settings
	ScanGetBarcodeLengths( barCODE39, (WordPtr)&LengthType, (WordPtr)&Length1, 
															(WordPtr)&Length2); 
	
	pList = GetObjectPtr( MoreCode39SetBCLengthsList );
	LstSetSelection (pList, LengthType); // 0=Any, 1=One, 2=Two, 3=Range
	label = LstGetSelectionText (pList, LengthType);
	pCtl = GetObjectPtr (MoreCode39SetBCLengthsPopTrigger);
	CtlSetLabel (pCtl, label);
	
	StrIToA( chrValue, Length1);
	SetFieldText(MoreCode39Length1Field, chrValue, 4, false);
	
	StrIToA( chrValue, Length2);
	SetFieldText(MoreCode39Length2Field, chrValue, 4, false);

	if (LengthType == 1 || LengthType == 0) // "one length" or "any length"
	{
		// hide the length2 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length2Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length2Label));
	}
	if (LengthType == 0) // "any length"
	{
		// hide the length1 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length1Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length1Label));
	}				

	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION: 		MoreCode39SendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void MoreCode39SendToDecoder()
{
	// commit the users selections -- send params to the decoder
	ListPtr pList;
	FieldPtr pFld;
	CharPtr pChrLength;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;

	pList = GetObjectPtr( MoreCode39SetBCLengthsList );
	LengthType = LstGetSelection(pList);
					
	pFld = GetObjectPtr(MoreCode39Length1Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length1 = StrAToI(pChrLength);

	pFld = GetObjectPtr(MoreCode39Length2Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length2 = StrAToI(pChrLength);

	ScanSetBarcodeLengths( barCODE39, LengthType, Length1, Length2);
	
	ScanCmdSendParams( No_Beep );
}


/***********************************************************************
 *
 * FUNCTION: 		MoreCode39SetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the More Code39 Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 						popSelectEvent (for Barcode Length Type)
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean MoreCode39SetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == MoreCode39CancelButton )
			{
				FrmGotoForm( MainForm );
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == MoreCode39OKButton )
			{
				MoreCode39SendToDecoder();
				FrmGotoForm( MainForm );
				bHandled = true;
			}
			break;

		case popSelectEvent:
		{
			if (ev->data.popSelect.listID == MoreCode39SetBCLengthsList)
			{
				FormPtr pForm = FrmGetActiveForm();
				if (ev->data.popSelect.selection == 0) // "any length"
				{
					// hide both
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length1Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length2Label));
				}
				else if (ev->data.popSelect.selection == 1) // "one length"
				{
					// show length1
					// hide length2
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length2Label));
				}
				else  // "two lengths" or "range"
				{
					// show both 
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length1Label));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length2Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MoreCode39Length2Label));
				}
			}
			break;
		}

		case frmOpenEvent:
			MoreCode39FormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle(MoreCode39Length1Field);
			FreeFieldHandle(MoreCode39Length2Field);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   Code 93 Setup Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnCode93Setup
 *
 * DESCRIPTION: 	Makes Code93Form the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnCode93Setup()
{
	FrmGotoForm( Code93Form );
}

/***********************************************************************
 *
 * FUNCTION: 		Code93FormOnInit
 *
 * DESCRIPTION: 	Initialize controls on the Code 93 Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void Code93FormOnInit()
{
	FormPtr pForm = FrmGetActiveForm();
	ListPtr pList;	
	ControlPtr pCtl;
	CharPtr label;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	char chrValue[4];
	MemSet(chrValue, sizeof(chrValue), 0);
	
	// Initialize the controls with current decoder param settings
	pCtl = GetObjectPtr( Code93Code93Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barCODE93) );

	ScanGetBarcodeLengths( barCODE93, (WordPtr)&LengthType, (WordPtr)&Length1, 
																			  (WordPtr)&Length2); 
	pList = GetObjectPtr( Code93SetBCLengthsList );
	LstSetSelection (pList, LengthType); // 0=Any, 1=One, 2=Two, 3=Range
	label = LstGetSelectionText (pList, LengthType);
	pCtl = GetObjectPtr (Code93SetBCLengthsPopTrigger);
	CtlSetLabel (pCtl, label);
	
	StrIToA( chrValue, Length1);
	SetFieldText(Code93Length1Field, chrValue, 4, false);
	
	StrIToA( chrValue, Length2);
	SetFieldText(Code93Length2Field, chrValue, 4, false);

	if (LengthType == 1 || LengthType == 0) // "one length" or "any length"
	{
		// hide the length2 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length2Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length2Label));
	}
	if (LengthType == 0) // "any length"
	{
		// hide the length1 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length1Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length1Label));
	}				
	
	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION: 		Code93SendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void Code93SendToDecoder()
{
	// commit the users selections -- send params to the decoder
	ListPtr pList;
	FieldPtr pFld;
	CharPtr pChrLength;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	ControlPtr pCtl;
	pCtl = GetObjectPtr( Code93Code93Checkbox );
	ScanSetBarcodeEnabled(barCODE93, CtlGetValue(pCtl) );

	pList = GetObjectPtr( Code93SetBCLengthsList );
	LengthType = LstGetSelection(pList);
					
	pFld = GetObjectPtr(Code93Length1Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length1 = StrAToI(pChrLength);

	pFld = GetObjectPtr(Code93Length2Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length2 = StrAToI(pChrLength);

	ScanSetBarcodeLengths( barCODE93, LengthType, Length1, Length2);

	ScanCmdSendParams( No_Beep);				
}

/***********************************************************************
 *
 * FUNCTION: 		Code93SetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the Code 93 Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 						popSelectEvent (for Barcode Length Type)
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean Code93SetupHandleEvent( EventPtr event )
{
	Boolean bHandled = false;
	
	switch( event->eType )
	{
		case ctlSelectEvent:
			if( event->data.ctlSelect.controlID == Code93CancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( event->data.ctlSelect.controlID == Code93OKButton )
			{
				Code93SendToDecoder();
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			break;

		case popSelectEvent:
		{
			if (event->data.popSelect.listID == Code93SetBCLengthsList)
			{
				FormPtr pForm = FrmGetActiveForm();
				if (event->data.popSelect.selection == 0) // "any length"
				{
					// hide both
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length1Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length2Label));
				}
				else if (event->data.popSelect.selection == 1) // "one length"
				{
					// show length1
					// hide length2
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,Code93Length1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,Code93Length1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,Code93Length2Label));
				}
				else // "two lengths" or "range"
				{
					// show both 
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,Code93Length1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,Code93Length1Label));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,Code93Length2Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,Code93Length2Label));
				}
			}
			break;
		}

		case frmOpenEvent:
			Code93FormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle(Code93Length1Field);
			FreeFieldHandle(Code93Length2Field);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   I2of5 Setup Dialog
 ***********************************************************************
 ***********************************************************************/

/***********************************************************************
 *
 * FUNCTION: 		OnI2of5Setup
 *
 * DESCRIPTION: 	Makes I2of5Form the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnI2of5Setup()
{
	FrmGotoForm( I2of5Form );
}

/***********************************************************************
 *
 * FUNCTION: 		I2of5FormOnInit
 *
 * DESCRIPTION: 	Initialize controls on the I2of5 Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void I2of5FormOnInit()
{
	FormPtr pForm = FrmGetActiveForm();
	ListPtr pList;	
	ControlPtr pCtl;
	CharPtr label;
	int VerifyCheckDigit = 0;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	char chrValue[4];
	MemSet(chrValue, sizeof(chrValue), 0);

	// Initialize the controls with current decoder param settings
	pCtl = GetObjectPtr( I2of5I2of5Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barI2OF5) );

	// enable conversions as requested
	pCtl = GetObjectPtr( I2of5ConverttoEAN13Checkbox );
	CtlSetValue(pCtl, ScanGetConvert(convertI2of5toEan13) );
	pCtl = GetObjectPtr( I2of5TxCheckDigitCheckbox );
	CtlSetValue(pCtl, ScanGetTransmitCheckDigit(barI2OF5) );

	// init the VerifyCheckDigit popup list and trigger
	VerifyCheckDigit = ScanGetI2of5CheckDigitVerification(); // 0=Disable, 1=USS, 2=OPCC
	pList = GetObjectPtr( I2of5VerifyCheckDigitList );
	LstSetSelection( pList, VerifyCheckDigit); 
	label = LstGetSelectionText( pList, VerifyCheckDigit);
	pCtl = GetObjectPtr( I2of5VerifyCheckDigitPopTrigger);
	CtlSetLabel (pCtl, label);	

	// init barcode lengths fields
	ScanGetBarcodeLengths( barI2OF5, (WordPtr)&LengthType, (WordPtr)&Length1, 
																			  (WordPtr)&Length2); 
	pList = GetObjectPtr( I2of5SetBCLengthsList );
	LstSetSelection (pList, LengthType); // 0=Any, 1=One, 2=Two, 3=Range
	label = LstGetSelectionText (pList, LengthType);
	pCtl = GetObjectPtr (I2of5SetBCLengthsPopTrigger);
	CtlSetLabel (pCtl, label);
	
	StrIToA( chrValue, Length1);
	SetFieldText(I2of5Length1Field, chrValue, 4, false);
	
	StrIToA( chrValue, Length2);
	SetFieldText(I2of5Length2Field, chrValue, 4, false);

	if (LengthType == 1 || LengthType == 0) // "one length" or "any length"
	{
		// hide the length2 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length2Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length2Label));
	}
	if (LengthType == 0) // "any length"
	{
		// hide the length1 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length1Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length1Label));
	}				

	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION: 		I2of5SendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void I2of5SendToDecoder()
{
	// commit the users selections -- send params to the decoder
	ControlPtr pCtl;
	ListPtr pList;
	FieldPtr pFld;
	CharPtr pChrLength;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	int VerifyCheckDigit = 0;
	
	// commit the users selections 
	pCtl = GetObjectPtr( I2of5I2of5Checkbox );
	ScanSetBarcodeEnabled( barI2OF5, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( I2of5ConverttoEAN13Checkbox );
	ScanSetConvert( convertI2of5toEan13, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( I2of5TxCheckDigitCheckbox );
	ScanSetTransmitCheckDigit(barI2OF5, CtlGetValue(pCtl) );
	
	// commit the VerifyCheckDigit value
	pList = GetObjectPtr( I2of5VerifyCheckDigitList);
	VerifyCheckDigit = LstGetSelection(pList);
	ScanSetI2of5CheckDigitVerification( VerifyCheckDigit);
	
	// commit the BCLengths popup list and field values
	pList = GetObjectPtr( I2of5SetBCLengthsList );
	LengthType = LstGetSelection(pList);
					
	pFld = GetObjectPtr(I2of5Length1Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length1 = StrAToI(pChrLength);

	pFld = GetObjectPtr(I2of5Length2Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length2 = StrAToI(pChrLength);

	ScanSetBarcodeLengths( barI2OF5, LengthType, Length1, Length2);
	

	ScanCmdSendParams( No_Beep ); // Send all our settings to the decoder.
}



/***********************************************************************
 *
 * FUNCTION: 		I2of5SetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the I2of5 Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 						popSelectEvent (for Barcode Length Type)
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean I2of5SetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == I2of5CancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == I2of5OKButton )
			{
				I2of5SendToDecoder();				
				FrmGotoForm( MainForm );
				bHandled = true;
			}
			break;

		case popSelectEvent:
		{
			if (ev->data.popSelect.listID == I2of5SetBCLengthsList)
			{
				FormPtr pForm = FrmGetActiveForm();
				if (ev->data.popSelect.selection == 0) // "any length"
				{
					// hide both
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length1Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length2Label));
				}
				else if (ev->data.popSelect.selection == 1) // "one length"
				{
					// show length1
					// hide length2
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,I2of5Length1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,I2of5Length1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,I2of5Length2Label));
				}
				else // "two lengths" or "range"
				{
					// show both 
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,I2of5Length1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,I2of5Length1Label));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,I2of5Length2Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,I2of5Length2Label));
				}
			}
			break;
		}

		case frmOpenEvent:
			I2of5FormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle(I2of5Length1Field);
			FreeFieldHandle(I2of5Length2Field);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   D2of5 Setup Dialog
 ***********************************************************************
 ***********************************************************************/

/***********************************************************************
 *
 * FUNCTION: 		OnD2of5Setup
 *
 * DESCRIPTION: 	Makes D2of5Form the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnD2of5Setup()
{
	FrmGotoForm( D2of5Form );
}

/***********************************************************************
 *
 * FUNCTION: 		D2of5FormOnInit
 *
 * DESCRIPTION: 	Initialize controls on the D2of5 Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void D2of5FormOnInit()
{
	FormPtr pForm = FrmGetActiveForm();
	ListPtr pList;	
	ControlPtr pCtl;
	CharPtr label;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	char chrValue[4];
	MemSet(chrValue, sizeof(chrValue), 0);
	
	// Initialize the controls with current decoder param settings
	pCtl = GetObjectPtr( D2of5D2of5Checkbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barD25) );
	
	// init the BCLengths fields
	ScanGetBarcodeLengths( barD25, (WordPtr)&LengthType, (WordPtr)&Length1, 
														 (WordPtr)&Length2); 
	
	pList = GetObjectPtr( D2of5SetBCLengthsList );
	LstSetSelection (pList, LengthType); // 0=Any, 1=One, 2=Two, 3=Range
	label = LstGetSelectionText (pList, LengthType);
	pCtl = GetObjectPtr (D2of5SetBCLengthsPopTrigger);
	CtlSetLabel (pCtl, label);
	
	StrIToA( chrValue, Length1);
	SetFieldText(D2of5Length1Field, chrValue, 4, false);
	
	StrIToA( chrValue, Length2);
	SetFieldText(D2of5Length2Field, chrValue, 4, false);

	if (LengthType == 1 || LengthType == 0) // "one length" or "any length"
	{
		// hide the length2 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length2Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length2Label));
	}
	if (LengthType == 0) // "any length"
	{
		// hide the length1 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length1Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length1Label));
	}				

	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION: 		D2of5SendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void D2of5SendToDecoder()
{
	// commit the users selections -- send params to the decoder
	ListPtr pList;
	FieldPtr pFld;
	CharPtr pChrLength;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	ControlPtr pCtl;
	
	// commit the users selections -- send params to the decoder
	pCtl = GetObjectPtr( D2of5D2of5Checkbox );
	ScanSetBarcodeEnabled(barD25, CtlGetValue(pCtl) );

	pList = GetObjectPtr( D2of5SetBCLengthsList );
	LengthType = LstGetSelection(pList);
					
	pFld = GetObjectPtr(D2of5Length1Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length1 = StrAToI(pChrLength);

	pFld = GetObjectPtr(D2of5Length2Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length2 = StrAToI(pChrLength);

	ScanSetBarcodeLengths( barD25, LengthType, Length1, Length2);
	
	ScanCmdSendParams( No_Beep );
}				


/***********************************************************************
 *
 * FUNCTION: 		D2of5SetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the D2of5 Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 						popSelectEvent (for Barcode Length Type)
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean D2of5SetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == D2of5CancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == D2of5OKButton )
			{
				D2of5SendToDecoder();
				FrmGotoForm( MainForm );
				bHandled = true;
			}
			break;

		case popSelectEvent:
		{
			if (ev->data.popSelect.listID == D2of5SetBCLengthsList)
			{
				FormPtr pForm = FrmGetActiveForm();
				if (ev->data.popSelect.selection == 0) // "any length"
				{
					// hide both
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length1Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length2Label));
				}
				else if (ev->data.popSelect.selection == 1) // "one length"
				{
					// show length1
					// hide length2
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,D2of5Length1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,D2of5Length1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,D2of5Length2Label));
				}
				else // "two lengths" or "range"
				{
					// show both 
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,D2of5Length1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,D2of5Length1Label));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,D2of5Length2Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,D2of5Length2Label));
				}
			}
			break;
		}

		case frmOpenEvent:
			D2of5FormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle(D2of5Length1Field);
			FreeFieldHandle(D2of5Length2Field);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   Codabar Setup Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnCodabarSetup
 *
 * DESCRIPTION: 	Makes CodabarForm the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnCodabarSetup()
{
	FrmGotoForm( CodabarForm );
}

/***********************************************************************
 *
 * FUNCTION: 		CodabarFormOnInit
 *
 * DESCRIPTION: 	Initialize controls on the Codabar Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void CodabarFormOnInit()
{
	FormPtr pForm = FrmGetActiveForm();
	ListPtr pList;	
	ControlPtr pCtl;
	CharPtr label;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	char chrValue[4];
	MemSet(chrValue, sizeof(chrValue), 0);
	
	// Initialize the controls with current decoder param settings
	pCtl = GetObjectPtr( CodabarCodabarCheckbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barCODABAR) );

	// enable conversions as requested
	pCtl = GetObjectPtr( CodabarNOTISEditingCheckbox );
	CtlSetValue(pCtl, ScanGetNotisEditing() );
	pCtl = GetObjectPtr( CodabarCLSIEditingCheckbox );
	CtlSetValue(pCtl, ScanGetClsiEditing() );

	// init the SetBCLengths fields.
	ScanGetBarcodeLengths( barCODABAR, (WordPtr)&LengthType, (WordPtr)&Length1, 
																			  (WordPtr)&Length2); 
	
	pList = GetObjectPtr( CodabarSetBCLengthsList );
	LstSetSelection (pList, LengthType); // 0=Any, 1=One, 2=Two, 3=Range
	label = LstGetSelectionText (pList, LengthType);
	pCtl = GetObjectPtr (CodabarSetBCLengthsPopTrigger);
	CtlSetLabel (pCtl, label);
	
	StrIToA( chrValue, Length1);
	SetFieldText(CodabarLength1Field, chrValue, 4, false);
	
	StrIToA( chrValue, Length2);
	SetFieldText(CodabarLength2Field, chrValue, 4, false);

	if (LengthType == 1 || LengthType == 0) // "one length" or "any length"
	{
		// hide the length2 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength2Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength2Label));
	}
	if (LengthType == 0) // "any length"
	{
		// hide the length1 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength1Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength1Label));
	}				

	FrmDrawForm( pForm );	
}


/***********************************************************************
 *
 * FUNCTION: 		CodabarSendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void CodabarSendToDecoder()
{
	// commit the users selections -- send params to the decoder
	ListPtr pList;
	FieldPtr pFld;
	CharPtr pChrLength;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	ControlPtr pCtl;
	
	// commit the users selections -- send params to the decoder
	pCtl = GetObjectPtr( CodabarCodabarCheckbox );
	ScanSetBarcodeEnabled(barCODABAR, CtlGetValue(pCtl) );

	// enable conversions as requested
	pCtl = GetObjectPtr( CodabarNOTISEditingCheckbox );
	ScanSetNotisEditing( CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( CodabarCLSIEditingCheckbox );
	ScanSetClsiEditing( CtlGetValue(pCtl) );

	// set the BCLengths as requested
	pList = GetObjectPtr( CodabarSetBCLengthsList );
	LengthType = LstGetSelection(pList);
					
	pFld = GetObjectPtr(CodabarLength1Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length1 = StrAToI(pChrLength);

	pFld = GetObjectPtr(CodabarLength2Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length2 = StrAToI(pChrLength);

	ScanSetBarcodeLengths( barCODABAR, LengthType, Length1, Length2);
	
	ScanCmdSendParams( No_Beep );
}


/***********************************************************************
 *
 * FUNCTION: 		CodabarSetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the Codabar Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 						popSelectEvent (for Barcode Length Type)
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean CodabarSetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == CodabarCancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == CodabarOKButton )
			{
				CodabarSendToDecoder();
				FrmGotoForm( MainForm );
				bHandled = true;
			}
			break;

		case popSelectEvent:
		{
			if (ev->data.popSelect.listID == CodabarSetBCLengthsList)
			{
				FormPtr pForm = FrmGetActiveForm();
				if (ev->data.popSelect.selection == 0) // "any length"
				{
					// hide both
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength1Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength2Label));
				}
				else if (ev->data.popSelect.selection == 1) // "one length"
				{
					// show length1
					// hide length2
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,CodabarLength1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,CodabarLength1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,CodabarLength2Label));
				}
				else // "two lengths" or "range"
				{
					// show both 
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,CodabarLength1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,CodabarLength1Label));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,CodabarLength2Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,CodabarLength2Label));
				}
			}
			break;
		}

		case frmOpenEvent:
			CodabarFormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle(CodabarLength1Field);
			FreeFieldHandle(CodabarLength2Field);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   MSIPlessey Setup Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnMSIPlesseySetup
 *
 * DESCRIPTION: 	Makes MSIPlesseyForm the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnMSIPlesseySetup()
{
	FrmGotoForm( MSIPlesseyForm );
}

/***********************************************************************
 *
 * FUNCTION: 		MSIPlesseyFormOnInit
 *
 * DESCRIPTION: 	Initialize controls on the MSI Plessey Setup form 
 * 					using the Scan Mgr API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void MSIPlesseyFormOnInit()
{
	FormPtr pForm = FrmGetActiveForm();
	ListPtr pList;	
	ControlPtr pCtl;
	CharPtr label;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	int CheckDigits = 0;
	int CheckDigitType = 0;
	char chrValue[4];
	MemSet(chrValue, sizeof(chrValue), 0);

	// Initialize the controls with current decoder param settings
	pCtl = GetObjectPtr( MSIPlesseyMSIPlesseyCheckbox );
	CtlSetValue(pCtl, ScanGetBarcodeEnabled(barMSI_PLESSEY) );
	pCtl = GetObjectPtr( MSIPlesseyTxCheckDigitCheckbox );
	CtlSetValue(pCtl, ScanGetTransmitCheckDigit( barMSI_PLESSEY ) );

	// init barcode lengths fields
	ScanGetBarcodeLengths( barMSI_PLESSEY, (WordPtr)&LengthType, (WordPtr)&Length1, 
																			  (WordPtr)&Length2); 
	pList = GetObjectPtr( MSIPlesseySetBCLengthsList );
	LstSetSelection (pList, LengthType); // 0=Any, 1=One, 2=Two, 3=Range
	label = LstGetSelectionText (pList, LengthType);
	pCtl = GetObjectPtr (MSIPlesseySetBCLengthsPopTrigger);
	CtlSetLabel (pCtl, label);
	
	StrIToA( chrValue, Length1);
	SetFieldText(MSIPlesseyLength1Field, chrValue, 4, false);
	
	StrIToA( chrValue, Length2);
	SetFieldText(MSIPlesseyLength2Field, chrValue, 4, false);

	if (LengthType == 1 || LengthType == 0) // "one length" or "any length"
	{
		// hide the length2 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength2Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength2Label));
	}
	if (LengthType == 0) // "any length"
	{
		// hide the length1 field and its label
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength1Field));
		FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength1Label));
	}				

	// init the CheckDigits, CheckDigitType
	CheckDigits = ScanGetMsiPlesseyCheckDigits(); // 0=One, 1=Two
	pList = GetObjectPtr( MSIPlesseyCheckDigitsList );
	LstSetSelection( pList, CheckDigits); 
	label = LstGetSelectionText( pList, CheckDigits);
	pCtl = GetObjectPtr( MSIPlesseyCheckDigitsPopTrigger);
	CtlSetLabel (pCtl, label);	
	
	CheckDigitType = ScanGetMsiPlesseyCheckDigitAlgorithm(); // 0=Mod10/11, 1=Mod10/10
	pList = GetObjectPtr( MSIPlesseyCheckDigitTypeList );
	LstSetSelection( pList, CheckDigitType); 
	label = LstGetSelectionText( pList, CheckDigitType);
	pCtl = GetObjectPtr( MSIPlesseyCheckDigitTypePopTrigger);
	CtlSetLabel (pCtl, label);	

	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION: 		MSIPlesseySendToDecoder
 *
 * DESCRIPTION: 	Invoked in response to the OK or More button, this 
 * 					function gets the end-users selections from the form
 * 					and sets the corresponding decoder parameters using 
 * 					the API. After all parameters have been set, calls 
 * 					"ScanCmdSendParams" to send them all to the decoder.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void MSIPlesseySendToDecoder()
{
	ListPtr pList;
	FieldPtr pFld;
	CharPtr pChrLength;
	int LengthType = 0;
	int Length1 = 0;
	int Length2 = 0;
	int CheckDigits = 0;
	int CheckDigitType = 0;
	ControlPtr pCtl;

	// commit the users selections
	pCtl = GetObjectPtr( MSIPlesseyMSIPlesseyCheckbox );
	ScanSetBarcodeEnabled(barMSI_PLESSEY, CtlGetValue(pCtl) );
	pCtl = GetObjectPtr( MSIPlesseyTxCheckDigitCheckbox );
	ScanSetTransmitCheckDigit( barMSI_PLESSEY, CtlGetValue(pCtl) );
	
	// set the BCLengths as requested
	pList = GetObjectPtr( MSIPlesseySetBCLengthsList );
	LengthType = LstGetSelection(pList);
					
	pFld = GetObjectPtr( MSIPlesseyLength1Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length1 = StrAToI(pChrLength);

	pFld = GetObjectPtr( MSIPlesseyLength2Field);
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
		Length2 = StrAToI(pChrLength);
	ScanSetBarcodeLengths( barMSI_PLESSEY, LengthType, Length1, Length2);

	// commit the CheckDigits, CheckDigitType
	pList = GetObjectPtr( MSIPlesseyCheckDigitsList );
	CheckDigits = LstGetSelection( pList); 
	ScanSetMsiPlesseyCheckDigits( CheckDigits); // 0=One, 1=Two
	
	pList = GetObjectPtr( MSIPlesseyCheckDigitTypeList );
	CheckDigitType = LstGetSelection( pList); 
	ScanSetMsiPlesseyCheckDigitAlgorithm(CheckDigitType); // 0=Mod10/11, 1=Mod10/10
	
	// send params to the decoder
	ScanCmdSendParams( No_Beep);
}


/***********************************************************************
 *
 * FUNCTION: 		MSIPlesseySetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the MSI Plessey Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK and Cancel), 
 * 						popSelectEvent (for Barcode Length Type)
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean MSIPlesseySetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == MSIPlesseyCancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == MSIPlesseyOKButton )
			{
				MSIPlesseySendToDecoder();				
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			break;

		case popSelectEvent:
		{
			if (ev->data.popSelect.listID == MSIPlesseySetBCLengthsList)
			{
				FormPtr pForm = FrmGetActiveForm();
				if (ev->data.popSelect.selection == 0) // "any length"
				{
					// hide both
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength1Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength2Label));
				}
				else if (ev->data.popSelect.selection == 1) // "one length"
				{
					// show length1
					// hide length2
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength1Label));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength2Field));
					FrmHideObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength2Label));
				}
				else // "two lengths" or "range"
				{
					// show both 
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength1Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength1Label));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength2Field));
					FrmShowObject( pForm, FrmGetObjectIndex(pForm,MSIPlesseyLength2Label));
				}
			}
			break;
		}

		case frmOpenEvent:
			MSIPlesseyFormOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle(MSIPlesseyLength1Field);
			FreeFieldHandle(MSIPlesseyLength2Field);
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   Beep Frequency Setup Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnBeepFrequencySetup
 *
 * DESCRIPTION: 	Makes BeepFrequenciesForm the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnBeepFrequencySetup()
{
	FrmGotoForm( BeepFrequenciesForm );
}

/***********************************************************************
 *
 * FUNCTION: 		BeepFrequencyOnInit
 *
 * DESCRIPTION: 	Initialize controls on the Beep Frequency Setup form
 * 					using the Scan Mgr API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void BeepFrequencyOnInit( void )
{
	FormPtr pForm = FrmGetActiveForm();
	Char temp[7];
	int frequency;

	frequency = ScanGetBeepFrequency( decodeFrequency );
	SetFieldText( BeepFrequenciesDecodeFreqField, StrIToA( temp, frequency ), 6, false );

	frequency = ScanGetBeepFrequency( lowFrequency );
	SetFieldText( BeepFrequenciesLowFreqField, StrIToA( temp, frequency ), 6, false );

	frequency = ScanGetBeepFrequency( mediumFrequency );
	SetFieldText( BeepFrequenciesMedFreqField, StrIToA( temp, frequency ), 6, false );

	frequency = ScanGetBeepFrequency( highFrequency );
	SetFieldText( BeepFrequenciesHighFreqField, StrIToA( temp, frequency ), 6, false );

	FrmDrawForm( pForm );		
}

/***********************************************************************
 *
 * FUNCTION: 		ConvertCharToFrequency
 *
 * DESCRIPTION: 	Takes the character string input, converts it to a 
 * 					number, ensures that it's in the valid range, and 
 * 					then returns it.
 *
 * PARAMETERS: 		pChar - pointer to char string representing frequency
 *
 * RETURNED: 		integer value in the allowed frequency range
 *
 ***********************************************************************/
int ConvertCharToFrequency( CharPtr pChar)
{
	int	freq = StrAToI(pChar);

	if (freq > MAX_BEEP_FREQUENCY)
		freq = MAX_BEEP_FREQUENCY;
	else if (freq < MIN_BEEP_FREQUENCY)
		freq = MIN_BEEP_FREQUENCY;

	return freq;
}

/***********************************************************************
 *
 * FUNCTION: 		BeepFrequencySend
 *
 * DESCRIPTION: 	Reads the user's chosen frequency values from the form,
 * 					and calls the Scan Manager API to set them.
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void BeepFrequencySend( void )
{
	FieldPtr pFld;
	CharPtr pChrFreq;
	int freq;

	pFld = GetObjectPtr( BeepFrequenciesDecodeFreqField );
	pChrFreq = FldGetTextPtr(pFld);
	if (pChrFreq)
	{
		freq = ConvertCharToFrequency( pChrFreq);
		ScanSetBeepFrequency( decodeFrequency, freq );
	}
		
	pFld = GetObjectPtr( BeepFrequenciesLowFreqField );
	pChrFreq = FldGetTextPtr(pFld);
	if (pChrFreq)
	{
		freq = ConvertCharToFrequency(pChrFreq);
		ScanSetBeepFrequency( lowFrequency, freq );
	}

	pFld = GetObjectPtr( BeepFrequenciesMedFreqField );
	pChrFreq = FldGetTextPtr(pFld);
	if (pChrFreq)
	{
		freq = ConvertCharToFrequency(pChrFreq);
		ScanSetBeepFrequency( mediumFrequency, freq );
	}

	pFld = GetObjectPtr( BeepFrequenciesHighFreqField );
	pChrFreq = FldGetTextPtr(pFld);
	if (pChrFreq)
	{
		freq = ConvertCharToFrequency(pChrFreq);
		ScanSetBeepFrequency( highFrequency, freq );
	}
	
}

/***********************************************************************
 *
 * FUNCTION: 		OnBeep
 *
 * DESCRIPTION: 	Issues a beep of the desired frequency and duration.
 * 
 * PARAMETERS: 		freq - the frequency desired (in Hz)
 * 					duration - how long (in msec) the beep should last.
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
static void OnBeep( int freq, int duration )
{
	SndCommandType sndCmd;

	sndCmd.cmd = sndCmdFreqDurationAmp;
	sndCmd.param1 = freq;
	sndCmd.param2 = duration;
	sndCmd.param3 = sndMaxAmp - 4;	// Actually louder than the max
	SndDoCmd( NULL, &sndCmd, true );
}

/***********************************************************************
 *
 * FUNCTION: 		OnTestFrequency
 *
 * DESCRIPTION: 	Given a text field id, converts the contents of that
 * 					field to an integer frequency value, and then issues
 * 					the beep tone for that frequency (for 0.1 second).
 * 
 * PARAMETERS: 		field - the resource identifier for a text field that
 * 							should contain a frequency string.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void OnTestFrequency( Word field )
{
	FieldPtr pFld;
	CharPtr pChrFreq;
	int freq;

	pFld = GetObjectPtr( field );
	pChrFreq = FldGetTextPtr(pFld);
	if (pChrFreq)
	{
		freq = ConvertCharToFrequency(pChrFreq);
		OnBeep( freq, 100 );
	}
}

/***********************************************************************
 *
 * FUNCTION: 		BeepFrequencySetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the Beep Frequency Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent
 * 						ctlSelectEvent (for OK, Cancel, and Test buttons) 
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean BeepFrequencySetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == BeepFrequenciesCancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == BeepFrequenciesOKButton )
			{
				BeepFrequencySend();				
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if ( ev->data.ctlSelect.controlID == BeepFrequenciesTestDecodeButton )
				OnTestFrequency( BeepFrequenciesDecodeFreqField );
			else if ( ev->data.ctlSelect.controlID == BeepFrequenciesTestLowButton )
				OnTestFrequency( BeepFrequenciesLowFreqField );
			else if ( ev->data.ctlSelect.controlID == BeepFrequenciesTestMedButton )
				OnTestFrequency( BeepFrequenciesMedFreqField );
			else if ( ev->data.ctlSelect.controlID == BeepFrequenciesTestHighButton )
				OnTestFrequency( BeepFrequenciesHighFreqField );
			break;
			
		case frmOpenEvent:
			BeepFrequencyOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle( BeepFrequenciesDecodeFreqField );
			FreeFieldHandle( BeepFrequenciesLowFreqField );
			FreeFieldHandle( BeepFrequenciesMedFreqField );
			FreeFieldHandle( BeepFrequenciesHighFreqField );
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

/***********************************************************************
 ***********************************************************************
   Beep Duration Setup Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnBeepDurationSetup
 *
 * DESCRIPTION: 	Makes BeepDurationsForm the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnBeepDurationSetup()
{
	FrmGotoForm( BeepDurationsForm );
}

/***********************************************************************
 *
 * FUNCTION: 		OnTestFrequency
 *
 * DESCRIPTION: 	Given a text field id, converts the contents of that
 * 					field to an integer duration value, and then issues
 * 					the a beep tone for that duration (in msec).
 * 
 * PARAMETERS: 		field - the resource identifier for a text field that
 * 							should contain a duration string.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void OnTestDuration( Word field )
{
	FieldPtr pFld;
	CharPtr pChrLength;
	int dur;

	pFld = GetObjectPtr( field );
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
	{
		dur = ConvertCharToDuration(pChrLength);
		OnBeep( 3000, dur );
	}
}

/***********************************************************************
 *
 * FUNCTION: 		ConvertCharToDuration
 *
 * DESCRIPTION: 	Takes the character string input, converts it to a 
 * 					number, ensures that it's in the valid range, and 
 * 					then returns it.
 *
 * PARAMETERS: 		pChar - pointer to char string representing duration
 *
 * RETURNED: 		integer value in the allowed duration range
 *
 ***********************************************************************/
int ConvertCharToDuration( CharPtr pChar)
{
	int	freq = StrAToI(pChar);

	if (freq > MAX_BEEP_DURATION)
		freq = MAX_BEEP_DURATION;
	else if (freq < MIN_BEEP_DURATION)
		freq = MIN_BEEP_DURATION;

	return freq;
}

/***********************************************************************
 *
 * FUNCTION: 		BeepDurationSend
 *
 * DESCRIPTION: 	Reads the user's chosen duration values from the form,
 * 					and calls the Scan Manager API to set them.
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void BeepDurationSend( void )
{
	FieldPtr pFld;
	CharPtr pChrLength;
	int dur;

	pFld = GetObjectPtr( BeepDurationsDecodeDurField );
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
	{
		dur = ConvertCharToDuration(pChrLength);
		ScanSetBeepDuration( decodeDuration, dur );
	}
		
	pFld = GetObjectPtr( BeepDurationsLongDurField );
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
	{
		dur = ConvertCharToDuration(pChrLength);
		ScanSetBeepDuration( longDuration, dur );
	}

	pFld = GetObjectPtr( BeepDurationsMedDurField );
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
	{
		dur = ConvertCharToDuration(pChrLength);
		ScanSetBeepDuration( mediumDuration, dur );
	}

	pFld = GetObjectPtr( BeepDurationsShortDurField );
	pChrLength = FldGetTextPtr(pFld);
	if (pChrLength)
	{
		dur = ConvertCharToDuration(pChrLength);
		ScanSetBeepDuration( shortDuration, dur );
	}
	
}

/***********************************************************************
 *
 * FUNCTION: 		BeepDurationSetupOnInit
 *
 * DESCRIPTION: 	Initialize controls on the Duration Setup form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void BeepDurationSetupOnInit( void )
{
	FormPtr pForm = FrmGetActiveForm();
	Char temp[7];
	int duration;

	duration = ScanGetBeepDuration( decodeDuration );
	SetFieldText( BeepDurationsDecodeDurField, StrIToA( temp, duration ), 6, false );

	duration = ScanGetBeepDuration( shortDuration );
	SetFieldText( BeepDurationsShortDurField, StrIToA( temp, duration ), 6, false );

	duration = ScanGetBeepDuration( mediumDuration );
	SetFieldText( BeepDurationsMedDurField, StrIToA( temp, duration ), 6, false );

	duration = ScanGetBeepDuration( longDuration );
	SetFieldText( BeepDurationsLongDurField, StrIToA( temp, duration ), 6, false );


	FrmDrawForm( pForm );	
}

/***********************************************************************
 *
 * FUNCTION: 		BeepDurationSetupHandleEvent
 *
 * DESCRIPTION: 	Event handler for the Beep Duration Setup form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for OK, Cancel and Test buttons) 
 * 						popSelectEvent (for Barcode Length Type)
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean BeepDurationSetupHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == BeepDurationsCancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == BeepDurationsOKButton )
			{
				BeepDurationSend();				
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if ( ev->data.ctlSelect.controlID == BeepDurationsTestDecodeButton )
				OnTestDuration( BeepDurationsDecodeDurField );
			else if ( ev->data.ctlSelect.controlID == BeepDurationsTestShortButton )
				OnTestDuration( BeepDurationsShortDurField );
			else if ( ev->data.ctlSelect.controlID == BeepDurationsTestMedButton )
				OnTestDuration( BeepDurationsMedDurField );
			else if ( ev->data.ctlSelect.controlID == BeepDurationsTestLongButton )
				OnTestDuration( BeepDurationsLongDurField );
			break;

		case frmOpenEvent:
			BeepDurationSetupOnInit();
			bHandled = true;
			break;
			
		
		case frmCloseEvent:
			FreeFieldHandle( BeepDurationsDecodeDurField );
			FreeFieldHandle( BeepDurationsShortDurField );
			FreeFieldHandle( BeepDurationsMedDurField );
			FreeFieldHandle( BeepDurationsLongDurField );
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

void OnBeepTest()
{
	FrmGotoForm( BeepTestForm );
}

/***********************************************************************
 *
 * FUNCTION: 		BeepTestHandleEvent
 *
 * DESCRIPTION: 	Event handler for the Beep Test form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for Cancel and Test buttons) 
 * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean BeepTestHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == BeepTestCancelButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			else if( ev->data.ctlSelect.controlID == BeepTestBeepButton )
			{
				DoBeepTest();				
				bHandled = true;
			}
			break;

		case frmOpenEvent:
			FrmDrawForm( FrmGetActiveForm() );
			bHandled = true;
			break;
			
		default:
			break;
	}
	
	return bHandled;
}

void DoBeepTest()
{
	ListPtr pList = GetObjectPtr( BeepTestBeepTypeList );
	Int nSel = LstGetSelection( pList );
	
	if( nSel >= 0 )
	{
		ScanCmdBeep( (BeepType)(nSel+1) );
	}
}


/***********************************************************************
 ***********************************************************************
   About Dialog
 ***********************************************************************
 ***********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION: 		OnAbout
 *
 * DESCRIPTION: 	Makes AboutForm the active form
 *
 * PARAMETERS: 		None.
 *
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
void OnAbout()
{
	FrmGotoForm( AboutForm );
}

/***********************************************************************
 *
 * FUNCTION: 		AboutOnInit
 *
 * DESCRIPTION: 	Initializes the controls on the About form using 
 * 					the Scan Manager API to get current decoder values.
 * 
 * PARAMETERS: 		None.
 * 
 * RETURNED: 		Nothing.
 *
 ***********************************************************************/
static void AboutOnInit( void )
{
	char szText[MAX_PACKET_LENGTH];
	ControlPtr pControl;
	FormPtr pForm = FrmGetActiveForm();

	// Draw the form - then draw the version information (labels used as place holders only)
	FrmDrawForm( pForm );	

	// Scan Manager Version
	pControl = (ControlPtr)GetObjectPtr( AboutScanMgrVerLabel );
	ScanGetScanManagerVersion( szText, MAX_PACKET_LENGTH );
	WinDrawChars( szText, StrLen(szText), 	pControl->bounds.topLeft.x, pControl->bounds.topLeft.y );

	// Scan Port Driver Version
	pControl = (ControlPtr)GetObjectPtr( AboutScanDrvVerLabel );
	ScanGetScanPortDriverVersion( szText, MAX_PACKET_LENGTH );
	WinDrawChars( szText, StrLen(szText), pControl->bounds.topLeft.x, pControl->bounds.topLeft.y );

	// Decoder Firmware Version
	pControl = (ControlPtr)GetObjectPtr( AboutDecoderVerLabel );
	ScanGetDecoderVersion( szText, MAX_PACKET_LENGTH );
	WinDrawChars( 	szText, StrLen(szText), pControl->bounds.topLeft.x, pControl->bounds.topLeft.y );
}

/***********************************************************************
 *
 * FUNCTION: 		AboutHandleEvent
 *
 * DESCRIPTION: 	Event handler for the About form.
 * 					Handles the following events: 
 * 						frmOpenEvent and frmCloseEvent, 
 * 						ctlSelectEvent (for the OK button) 
  * 
 * PARAMETERS: 		ev - pointer to the event information structure
 * 
 * RETURNED: 		true  - we handled the event
 * 					false - not handled here, so pass it to next handler
 *
 ***********************************************************************/
Boolean AboutHandleEvent( EventPtr ev )
{
	Boolean bHandled = false;
	
	switch( ev->eType )
	{
		case ctlSelectEvent:
			if( ev->data.ctlSelect.controlID == AboutOKButton )
			{
				FrmGotoForm(MainForm);
				bHandled = true;
			}
			break;

		case frmOpenEvent:
			AboutOnInit();
			bHandled = true;
			break;
			
		case frmCloseEvent:
			break;
			
		default:
			break;
	}
	
	return bHandled;
}
