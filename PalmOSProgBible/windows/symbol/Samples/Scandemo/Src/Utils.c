/************************************************************************
* COPYRIGHT: 	Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE: 		Utils.c
*
* SYSTEM: 		Symbol barcode scanner for Palm III.
*
* MODULE: 		Utility functions.
*
* DESCRIPTION: 	Commonly used utility functions.
*
* FUNCTIONS: 	GetObjectPtr		// Get a UI element contained in a Form
*				SetFieldText		// Set the text in a field
*				FreeFieldText		// Free the field's allocated text handle
*				ScanGetBarTypeS...	// Get barCode type string from resource
*
* HISTORY:     3/2/97    SS   Created
*              ...
*************************************************************************/
#include "Pilot.h"
#include "Utils.h"
#include "ScanDemoRsc.h"
#include "ScanMgrDef.h"

/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  objectID (in) - The resource ID of the control to get.
 *
 * RETURNED:    Pointer to the control.
 *
 ***********************************************************************/
VoidPtr GetObjectPtr (Word objectID)
{
	FormPtr frm;
	
	frm = FrmGetActiveForm ();
	if( frm )
		return (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objectID)));
	else
		return NULL;
}

/***********************************************************************
 *
 * FUNCTION:    SetFieldText
 *
 * DESCRIPTION:Perform all necessary actions to set a Field control's
 *					text and redraw, if necesary.  Allocates a text handle.
 *              
 *
 * PARAMETERS: nFieldID(in) - The resource ID of the field.
 *					pSrcText(in) - The text to copy into field.
 *					nMaxSize(in) - Max size that the field can grow.
 *					bRedraw(in)  - Should the text be redrawn now? 
 *
 * RETURNED:   None
 *
 ***********************************************************************/
void SetFieldText( UInt nFieldID, const CharPtr pSrcText, Int nMaxSize, Boolean bRedraw )
{
	VoidHand hFieldText;
	CharPtr  pFieldText;
	FieldPtr pField;

	pField = GetObjectPtr( nFieldID );
	if( !pField )
		return;
		
	hFieldText = FldGetTextHandle( pField );
	if( !hFieldText )
		hFieldText = MemHandleNew( nMaxSize );

	// If already allocated, make sure it can handle nMaxSize already.
	// If not, realloc that buffer
	else
	{
		ULong curSize = MemHandleSize( hFieldText );
		if( curSize < nMaxSize )
			MemHandleResize(hFieldText, nMaxSize ) ;
	}

	if( hFieldText )
	{
		int len = StrLen(pSrcText);
		
		pFieldText = MemHandleLock( hFieldText );
		
		if (len > nMaxSize)
		{
			StrNCopy( pFieldText, pSrcText, nMaxSize-1);
			pFieldText[nMaxSize-1] = '\0';
		}
		else
			StrCopy( pFieldText, pSrcText );

		MemHandleUnlock( hFieldText );

		FldSetTextHandle( pField, hFieldText );
		FldSetTextAllocatedSize( pField, nMaxSize );
		
		FldSetMaxChars( pField, nMaxSize-1);

		FldRecalculateField( pField, true );
		if( bRedraw )
			FldDrawField( pField );
	}
}

/***********************************************************************
 *
 * FUNCTION:    FreeFieldHandle
 *
 * DESCRIPTION: Free text handle for field
 *
 * PARAMETERS:  field id
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
void FreeFieldHandle( int ID )
{
	FieldPtr pField;
	VoidHand handle;
	
	pField = GetObjectPtr( ID );
	handle = FldGetTextHandle( pField );
	if (handle)
	{
		FldSetTextHandle( pField, 0 );
		MemHandleFree( handle );
	}
}

/***********************************************************************
 *
 * FUNCTION:    ScanGetBarTypeStr
 *
 * DESCRIPTION: Converts a barType value (a byte) into a string, using 
 * 				 the defined barcode type values from ScanMgrDef.h.
 *
 * PARAMETERS:  barType(in) - The type of barcode.
 *					 pszBarType(out) - Pointer to storage that will be filled 
 * 										 in with the barcode type string.
 *              nStrLen(in) - Max length of pszBarType string.
 *
 * RETURNED:    Boolean - true = success. false otherwise.
 *
 ***********************************************************************/
Boolean ScanGetBarTypeStr( Byte barType, CharPtr pszBarType, UInt nStrLen )
{
	Boolean bResult = true;
	switch (barType)
	{
		case BCTYPE_NOT_APPLICABLE: // 0x00
			SysCopyStringResource( pszBarType, BCTypeNotApplicableString);
			break;	
		case BCTYPE_CODE39: // 0x01
			SysCopyStringResource( pszBarType, BCTypeCode39String);
			break;	
		case BCTYPE_CODABAR: // 0x02
			SysCopyStringResource( pszBarType, BCTypeCodabarString);
			break;	
		case BCTYPE_CODE128: // 0x03
			SysCopyStringResource( pszBarType, BCTypeCode128String);
			break;	
		case BCTYPE_D2OF5: // 0x04
			SysCopyStringResource( pszBarType, BCTypeDiscrete2Of5String);
			break;	
		case BCTYPE_IATA2OF5: // 0x05
			SysCopyStringResource( pszBarType, BCTypeIATA2of5String);
			break;	
		case BCTYPE_I2OF5: // 0x06
			SysCopyStringResource( pszBarType, BCTypeInterleaved2Of5String);
			break;	
		case BCTYPE_CODE93: // 0x07
			SysCopyStringResource( pszBarType, BCTypeCode93String);
			break;	
		case BCTYPE_UPCA: // 0x08
			SysCopyStringResource( pszBarType, BCTypeUPCAString);
			break;	
		case BCTYPE_UPCE0: // 0x09
			SysCopyStringResource( pszBarType, BCTypeUPCEOString);
			break;	
		case BCTYPE_EAN8: // 0x0A
			SysCopyStringResource( pszBarType, BCTypeEAN8String);
			break;	
		case BCTYPE_EAN13: // 0x0B
			SysCopyStringResource( pszBarType, BCTypeEAN13String);
			break;	
		case BCTYPE_MSI_PLESSEY: // 0x0E
			SysCopyStringResource( pszBarType, BCTypeMSIPlesseyString);
			break;	
		case BCTYPE_EAN128: // 0x0F
			SysCopyStringResource( pszBarType, BCTypeEAN128String);
			break;	
		case BCTYPE_UPCE1:  // 0x10
			SysCopyStringResource( pszBarType, BCTypeUPCE1String);
			break;	
//		case BCTYPE_CODE39_FULL_ASCII: // 0x13
//			SysCopyStringResource( pszBarType, BCTypeNotApplicableString);
//			break;	
		case BCTYPE_TRIOPTIC_CODE39: // 0x15
			SysCopyStringResource( pszBarType, BCTypeTriopticCode39String);
			break;	
		case BCTYPE_BOOKLAND_EAN: // 0x16
			SysCopyStringResource( pszBarType, BCTypeBooklandEANString);
			break;	
		case BCTYPE_COUPON_CODE: // 0x17
			SysCopyStringResource( pszBarType, BCTypeCouponCodeString);
			break;	
		case BCTYPE_UPCA_2SUPPLEMENTALS: // 0x48
			SysCopyStringResource( pszBarType, BCTypeUPCA2SuppsString);
			break;	
		case BCTYPE_UPCE0_2SUPPLEMENTALS: // 0x49
			SysCopyStringResource( pszBarType, BCTypeUPCEO2SuppsString);
			break;	
		case BCTYPE_EAN8_2SUPPLEMENTALS:  // 0x4A
			SysCopyStringResource( pszBarType, BCTypeEAN82SuppsString);
			break;	
		case BCTYPE_EAN13_2SUPPLEMENTALS: // 0x4B
			SysCopyStringResource( pszBarType, BCTypeEAN132SuppsString);
			break;	
		case BCTYPE_UPCE1_2SUPPLEMENTALS: // 0x50
			SysCopyStringResource( pszBarType, BCTypeUPCE12SuppsString);
			break;	
		case BCTYPE_EAN8_5SUPPLEMENTALS:  // 0x8A
			SysCopyStringResource( pszBarType, BCTypeEAN85SuppsString);
			break;	
		case BCTYPE_EAN13_5SUPPLEMENTALS: // 0x8B
			SysCopyStringResource( pszBarType, BCTypeEAN135SuppsString);
			break;	
		case BCTYPE_UPCA_5SUPPLEMENTALS:  // 0x88
			SysCopyStringResource( pszBarType, BCTypeUPCA5SuppsString);
			break;	
		case BCTYPE_UPCE0_5SUPPLEMENTALS: // 0x89
			SysCopyStringResource( pszBarType, BCTypeUPCEO5SuppsString);
			break;	
		case BCTYPE_UPCE1_5SUPPLEMENTALS: // 0x90
			SysCopyStringResource( pszBarType, BCTypeUPCE15SuppsString);
			break;	
		default: 
			StrCopy(pszBarType,"UNKNOWN");	
			bResult = false;
			break;
	}		
	return bResult;
}

