/************************************************************************
* COPYRIGHT:   Copyright  ©  1999 Symbol Technologies, Inc. 
*
* FILE:        info.c
*
* SYSTEM:      Symbol Print API for Palm III.
* 
*
* DESCRIPTION: Provides a sample application for the Symbol Print API.*
*
* HISTORY:     03/22/99    MS   Created
*              ...
*************************************************************************/

#include <Pilot.h>
#include "Print Sample.h"
#include "Print SampleRsc.h"
#include "info.h"


#pragma mark -- Externs --
extern CharPtr	gInfoPtr;
extern CharPtr	gInfoLabelPtr;

#pragma mark -- Static Prototypes --
static void	InfoScroll( Short linesToScroll, Int FieldID, Int BarID );
static void InfoUpdateScrollBar (Int FieldID, Int BarID);
static void	InfoHandleScroll( EventPtr event );
static Boolean SetFieldText( UInt nFieldID, const CharPtr pSrcText, 
							 ULong nMaxSize, Boolean bRedraw );
static void FreeFieldHandle (int fieldID);
static void SetFldEditable (Boolean bAttr, int fieldID);

/***********************************************************************
 *
 * FUNCTION:		InfoFormHandleEvent
 *
 * DESCRIPTION:	Handles processing of events 
 *
 * PARAMETERS:		event		- the most recent event.
 *
 * RETURNED:		True if the event is handled, false otherwise.
 *
 ***********************************************************************/
Boolean InfoHandleEvent(EventPtr event)
{
	FormPtr		frm;
	Boolean		handled = false;
	
	switch (event->eType)
	{
		case fldHeightChangedEvent:
		{
			InfoUpdateScrollBar( InfoInfoField, InfoInfoScrollBar );
			FldSetScrollPosition( GetObjectPtr( InfoInfoField ),
								event->data.fldHeightChanged.currentPos );
								
			handled = true;
			break;
		}
		case sclRepeatEvent:  //  the scroll bar was pressed
	   	{
	   		InfoHandleScroll( event );
	   		handled = true;
			break;
		}
		
	   	case ctlSelectEvent:  // A control button was pressed and released.
	   	{	
	   		switch ( event->data.ctlEnter.controlID ) {
				case InfoOkButton:
				{	
					FreeFieldHandle( InfoInfoField );
					FreeFieldHandle( InfoLabelField );
					FrmReturnToForm(0);
					handled = true;
					break;
				}
			}
			
			break;
		}
  	
  		case frmOpenEvent:	
  		{
			frm = FrmGetActiveForm();	
			FrmDrawForm (frm);
			
			
			SetFldEditable( false, InfoLabelField );
			SetFieldText( InfoLabelField, gInfoLabelPtr, StrLen(gInfoLabelPtr) + 1, true );
			
			SetFldEditable( false, InfoInfoField ); 
			SetFieldText( InfoInfoField, gInfoPtr, StrLen(gInfoPtr) + 1, true );
			
			if ( 5 < FldCalcFieldHeight( gInfoPtr, 134 ) ) {
				InfoUpdateScrollBar( InfoInfoField, InfoInfoScrollBar );
			}
			
			handled = true;
			break;
		}		
	}
	return(handled);
}


static void InfoHandleScroll( EventPtr event )
{
	switch ( event->data.ctlEnter.controlID ) {
	
		case InfoInfoScrollBar:
		{
			InfoScroll( (event->data.sclRepeat.newValue - event->data.sclRepeat.value),
					   	   InfoInfoField, InfoInfoScrollBar );
			break;
		}	
	}
}

static void InfoScroll( Short linesToScroll, Int FieldID, Int BarID )
{
	
	FieldPtr		fld;
	
	fld = GetObjectPtr (FieldID);

	if( (int)linesToScroll < 0) {
		FldScrollField( fld, -linesToScroll, up);
	}
	else {
		FldScrollField(fld, linesToScroll, down);
	}
	
	InfoUpdateScrollBar( FieldID, BarID );
}

static void InfoUpdateScrollBar (Int FieldID, Int BarID)
{
	Word scrollPos;
	Word textHeight;
	Word fieldHeight;
	Short maxValue;
	FieldPtr fld;
	ScrollBarPtr bar;

	fld = GetObjectPtr (FieldID);
	bar = GetObjectPtr (BarID);
	
	// get the values necessary to update the scroll bar.
	FldGetScrollValues (fld, &scrollPos, &textHeight,  &fieldHeight);
	SclSetScrollBar( bar, 0,  0, 0, 0 );
	
	if ( textHeight > fieldHeight ) {
		maxValue = textHeight - fieldHeight + 1;
	}
	else if ( scrollPos ) {
		maxValue = scrollPos;
	}
	else {
		maxValue = 0;
	}

	SclSetScrollBar (bar, scrollPos, 0, maxValue, fieldHeight - 1 );
}


static Boolean SetFieldText( UInt nFieldID, const CharPtr pSrcText, ULong nMaxSize, Boolean bRedraw )
{
	VoidHand hFieldText;
	CharPtr  pFieldText;
	FieldPtr pField;

	pField = GetObjectPtr( nFieldID );
	if( !pField ) {
		return( false );
	}
		
	hFieldText = FldGetTextHandle( pField );
	if( !hFieldText ) {
		hFieldText = MemHandleNew( nMaxSize );
		if ( ! hFieldText ) {
			MemHeapCompact( 0 );
			hFieldText = MemHandleNew( nMaxSize );
			if ( ! hFieldText ) {
				return( false );
			}
		}
	}
	else	{
		// If already allocated, make sure it can handle nMaxSize already.
		// If not, realloc that buffer
	
		ULong curSize = MemHandleSize( hFieldText );
		if( curSize < nMaxSize ) {
			FreeFieldHandle (nFieldID);
			hFieldText = MemHandleNew( nMaxSize );
			if ( ! hFieldText ) {
				MemHeapCompact( 0 );
				hFieldText = MemHandleNew( nMaxSize );
				if ( ! hFieldText ) {
					return( false );
				}
			}
		}
	}

	if( hFieldText ) {
	
		int len = StrLen(pSrcText);
		
		pFieldText = MemHandleLock( hFieldText );
		
		MemSet( pFieldText, nMaxSize, 0 );
		
		if( len > nMaxSize ) {
			StrNCopy( pFieldText, pSrcText, nMaxSize-1);
		}
		else {
			StrCopy( pFieldText, pSrcText );
		}

		MemHandleUnlock( hFieldText );

		FldSetTextHandle( pField, hFieldText );
		FldSetTextAllocatedSize( pField, nMaxSize );
		FldSetMaxChars( pField, nMaxSize-1 );
		FldRecalculateField( pField, true );
		
		if( bRedraw ) {
			FldDrawField( pField );
		}
	}
	
	return( true );
}

 
/***********************************************************************
 *
 * FUNCTION:    FreeFieldHandle
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:  
 ***********************************************************************/
static void FreeFieldHandle (int fieldID)
{
	FieldPtr pField;
	VoidHand handle;
	
	pField = GetObjectPtr(fieldID);
	handle = FldGetTextHandle(pField);
	
	if (handle)
	{
		FldSetTextHandle(pField,0);	
		MemHandleFree(handle);
	}	
}


static void SetFldEditable (Boolean bAttr, int fieldID)
{
	FieldAttrType attr;
	FieldPtr fld;
	
	fld = GetObjectPtr(fieldID);
	// Set the field to support editable.
	if (fld) {
		FldGetAttributes (fld, &attr);
		attr.editable = bAttr;
		FldSetAttributes (fld, &attr);
	}		
		
 }
 