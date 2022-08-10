/************************************************************************
* COPYRIGHT:   Copyright  ©  1999, 2000, & 2001 Symbol Technologies, Inc. 
*
* FILE:        Utils.c
*
* SYSTEM:      Symbol barcode scanner for Palm III.
*
* MODULE:      Utility functions.
*
* DESCRIPTION: Commonly used utility functions.
*
* FUNCTIONS: 	GetObjectPtr		// Get a UI element contained in a Form.
*				      SetFieldText		// Set the text in a field.
*				      FreeFieldText		// Free the field's allocated text handle.
*			
*
* REVISION HISTORY:
*         Name   Date       Description
*         -----  ---------- -----------
*         dcat   06/14/1999 Initial Release
*              
*************************************************************************/
#include "Pilot.h"
#include "Utils.h"
#include "FindMdbRsc.h"
#include "ScanMgrDef.h"

int LookUp (char *label);
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
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
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
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
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
		Err	err = 0;
		ULong curSize = MemHandleSize( hFieldText );
		if( curSize < nMaxSize )
			err = MemHandleResize(hFieldText, nMaxSize ) ;
		
		ErrNonFatalDisplayIf(err, "Unable to resize field!");
		
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
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
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

