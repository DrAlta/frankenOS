/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FingerMain.c
 *
 * Description:
 *		try fingering:
 *
 *		quake@gldfs.cr.usgs.gov
 *		nasanews@space.mit.edu
 *		copi@oddjob.uchicago.edu
 *
 *		they can return ALOT of stuff...
 *
 * History:
 *		04/24/97		Gregory Toto, created. 
 *		10/04/99		bt,	removed done btn from AppFormHandleEvent
 *
 *****************************************************************************/

#include <PalmOS.h>

#include "FingerDemo.h"



//=======================================================================
//	globals

UInt16		gCurrentFrmID;
DmOpenRef	gFingerDB;	




//=======================================================================
// CreateFingerDatabase
//
// remove the finger database. the routine closes the database before
// removing it (if it is open).
//
// gFingerDB is initialized with the DmOpenRef of the database.
//=======================================================================

Boolean CreateFingerDatabase( void )
{
	gFingerDB = 0;
		

	// remove any existing database (should not exist, but just in case)
	
	ClobberFingerDatabase( false );	
	


	// create a database for the app to use.
		
	if ( DmCreateDatabase( 0, FingerDBNameStr, FingerCreator, FingerDBType, false ) == 0 )
	{
		gFingerDB = DmOpenDatabaseByTypeCreator( FingerDBType, FingerCreator,
															  ( dmModeReadWrite | dmModeExclusive ) );
	}
	


	// error alert if the DB was not created.
		
	if ( gFingerDB == 0 )
	{
		ShowErrorDialog( DBNotCreatedString, NULL );
		return false;				
	}
	else
		return true;

}	// CreateFingerDatabase




//=======================================================================
// ClobberFingerDatabase
//
// remove the finger database. the routine closes the database before
// removing it (if it is open).
//
// gFingerDB is assumed to contain the DmOpenRef of the database.
//=======================================================================

void ClobberFingerDatabase( Boolean showErrDialog )
{
	Err					err = 0;
	DmSearchStateType	searchState;
	UInt16				cardNo;
	LocalID				dbLocalID;

	// if the database is still open, then close it.
	
	if ( gFingerDB != 0 ) 
	{
		if (DmNumRecords( gFingerDB ) > 0)
			DmReleaseRecord( gFingerDB, 0, true);
			
		DmCloseDatabase( gFingerDB );
		gFingerDB = 0;
	};


	// get the card and local ID of the database, and delete the database
	// so we start clean next time...
			
	if ( DmGetNextDatabaseByTypeCreator( true, &searchState,
													 FingerDBType, FingerCreator,
													 false, &cardNo, &dbLocalID ) != dmErrCantFind )
	{
		err = DmDeleteDatabase( cardNo, dbLocalID );
	
	}

	if ( ( err != 0 ) && showErrDialog ) ShowErrorDialog( DBNotFoundOrDelString, NULL );

}	// ClobberFingerDatabase



//=======================================================================
// GetObjectPtr
//
// return a pointer to an object with objectID in the form on formP. use
// the active form if formP is NULL.
//=======================================================================

static void* GetObjectPtr( FormPtr formP, UInt16 objectID)
{
	FormPtr frmP;

	if ( formP != NULL )
		frmP = formP;
	else
		frmP = FrmGetActiveForm();
		
	return FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, objectID ) );

}	// GetObjectPtr



//=======================================================================
// ShowControl
//
// given a formPtr and controlID, show or hide the control based on the
// value of showTheControl. the active form is used if formP is NULL.
// 
//=======================================================================

void ShowControl( FormPtr formP, UInt16 controlID, Boolean showTheControl )
{
	void*	objP;
	

	// get the index of the control
	objP = GetObjectPtr( formP, controlID );


	// show it or hide it.
	if ( showTheControl )
		CtlShowControl( (ControlPtr) objP );
	else
		CtlHideControl( (ControlPtr) objP );
			
}	// ShowControl

	

//=======================================================================
// SetFieldTextFromStr
//
// given a formPtr and fieldID, set the text of the field from the string
// on strP (a copy is made). the active form is used if formP is NULL. works
// even if the field is not editable.
//
// return a pointer to the field.
// 
//=======================================================================

FieldPtr SetFieldTextFromStr( FormPtr formP, UInt16 fieldID, MemPtr strP )
{
	MemHandle	oldTxtH;
	MemHandle	txtH;
	MemPtr		txtP;

	FieldPtr	fldP;
	Boolean		fieldWasEditable;
	

	// get some space in which to stash the string.
	txtH	= MemHandleNew(  StrLen( strP ) + 1 );
	txtP	= MemHandleLock( txtH );


	// copy the string.
	StrCopy( txtP, strP );


	// unlock the string handle.
	MemHandleUnlock( txtH );
	txtP = 0;


	// get the field and the field's current text handle.
	fldP		= GetObjectPtr( formP, fieldID );	
	oldTxtH	= FldGetTextHandle( fldP );
	
	
	// set the field's text to the new text.
	fieldWasEditable		= fldP->attr.editable;
	fldP->attr.editable	= true;
	
	FldSetTextHandle( fldP, txtH );
	FldDrawField( fldP );
	
	fldP->attr.editable	= fieldWasEditable;


	// free the handle AFTER we call FldSetTextHandle().
	if ( oldTxtH != NULL ) MemHandleFree( oldTxtH );
	
	return fldP;

}	// SetFieldTextFromStr



//=======================================================================
// SetFieldTextFromRes
//
// given a formPtr and fieldID, set the text of the field from the string
// resource strID. the active form is used if formP is NULL. works
// even if the field is not editable.
//
// return a pointer to the field.
// 
//=======================================================================

FieldPtr SetFieldTextFromRes( FormPtr formP, UInt16 fieldID, UInt16 strID )
{
	MemHandle	oldTxtH;
	
	MemHandle	strH;
	MemPtr		strP;
	UInt16		strSize;

	MemHandle	txtH;
	MemPtr		txtP;

	FieldPtr	fldP;
	Boolean		fieldWasEditable;
	

	// get a ptr and length of the string.
	
	strH	= DmGetResource( strRsc, strID );
	strP	= MemHandleLock( strH );
	strSize	= StrLen( strP ) + 1;

	// get some space in which to stash the string.
		
	txtH	= MemHandleNew( strSize );
	txtP	= MemHandleLock( txtH );

	// copy the string.
		
	StrCopy( txtP, strP );


	// unlock the handles.
		
	MemHandleUnlock( strH );
	MemHandleUnlock( txtH );

	
	// release the resource.
	
	DmReleaseResource( strH );


	// get the field and the field's current text handle.
	
	fldP		= GetObjectPtr( formP, fieldID );	
	oldTxtH	= FldGetTextHandle( fldP );


	// set the field's text to the new text.
	
	fieldWasEditable		= fldP->attr.editable;
	fldP->attr.editable	= true;
	
	FldSetTextHandle( fldP, txtH );
	FldDrawField( fldP );
	
	fldP->attr.editable	= fieldWasEditable;


	// free the handle AFTER we call FldSetTextHandle().
	
	if ( oldTxtH != NULL ) MemHandleFree( oldTxtH );

	
	return fldP;

}	// SetFieldTextFromRes




//=======================================================================
// ClearFieldText
//
// given a formPtr and fieldID, set the text of the field from the string
// resource strID. the active form is used if formP is NULL.
// 
//=======================================================================

FieldPtr ClearFieldText( FormPtr formP, UInt16 fldID )
{
	FieldPtr	fldP;
	MemHandle	txtH;


	// clear the text from the field specified by fldID in the form on fromPtr.

	fldP = GetObjectPtr( formP, fldID );	
	txtH = FldGetTextHandle( fldP );


	// if there is a text handle, free the text.
	
	if ( txtH != NULL )
	{
		FldSetTextHandle( fldP,NULL );

		// free the handle AFTER we call FldSetTextHandle().

		MemHandleFree( txtH );
	}

	return fldP;
	
}	// ClearFieldText



//=======================================================================
// ShowErrorDialog
// ShowErrorDialog2
//
// display an error dialog with text from the string resource stringID and
// string on aStrPtr, or from two string resources stringID1 and stringID2.
// 
//=======================================================================

#define PUNT_STRING	" "

void ShowErrorDialog( UInt16 stringID, MemPtr aStrPtr )
{
	// get a pointer to the string with ID stringID.
	
	MemHandle	strH = DmGetResource( strRsc, stringID );
	MemPtr		strP = MemHandleLock( strH );

	
	// FrmCustomAlert() really doesn't like to be handed NULL or empty
	// strings, so use " ".
	
	FrmCustomAlert( ErrorNoticeAlert, strP,
									  ( aStrPtr == NULL ) ? PUNT_STRING : aStrPtr,
									  PUNT_STRING );

	// clean up.
	
	MemHandleUnlock( strH );
	DmReleaseResource( strH );

}	// ShowErrorDialog


void ShowErrorDialog2( UInt16 stringID1,  UInt16 stringID2 )
{
	// get a pointer to the string with ID stringID1.
	
	MemHandle	strH1 = DmGetResource( strRsc, stringID1 );
	MemPtr		strP1 = MemHandleLock( strH1 );

	MemHandle	strH2 = DmGetResource( strRsc, stringID2 );
	MemPtr		strP2 = MemHandleLock( strH2 );
	
	// FrmCustomAlert() really doesn't like to be handed NULL or empty
	// strings, so use " ".
	
	FrmCustomAlert( ErrorNoticeAlert, strP1, strP2, PUNT_STRING );


	// clean up.
	
	MemHandleUnlock( strH1 );
	DmReleaseResource( strH1 );

	MemHandleUnlock( strH2 );
	DmReleaseResource( strH2 );

}	// ShowErrorDialog2



//=======================================================================
// UpdateScrollBar
//
// update the scroll bar on barP associated with field fldP.
//=======================================================================

void UpdateScrollBar( FieldPtr fldP, ScrollBarPtr barP )
{
	UInt16	scrollPos;
	UInt16	textHeight;
	UInt16	fieldHeight;
	Int16	maxValue;

	if ( fldP == NULL ) return;
	if ( barP == NULL ) return;
	
	FldGetScrollValues( fldP, &scrollPos, &textHeight,  &fieldHeight );

	if ( textHeight > fieldHeight )
		maxValue = textHeight - fieldHeight;
	else if ( scrollPos != 0 )
		maxValue = scrollPos;
	else
		maxValue = 0;

	SclSetScrollBar( barP, scrollPos, 0, maxValue, fieldHeight - 1 );

}	// UpdateScrollBar



//=======================================================================
// ScrollField
//
// scroll a field on fldP with scroll bar barP, linesToScroll lines.
//=======================================================================

void ScrollField( FieldPtr fldP, ScrollBarPtr barP, Int16 linesToScroll )
{

	if ( fldP == NULL ) return;
	if ( barP == NULL ) return;
	
	
	if ( linesToScroll < 0 )
	{
		UInt16	blankLines;
		Int16	min;
		Int16	max;
		Int16	value;
		Int16	pageSize;

		blankLines = FldGetNumberOfBlankLines( fldP );
		FldScrollField( fldP, -linesToScroll, winUp );
		
		// if there were blank lines visible at the end of the field
		// then we need to update the scroll bar.
		
		if ( blankLines != 0 )
		{
			// update the scroll bar.

			SclGetScrollBar( barP, &value, &min, &max, &pageSize );
			
			if ( blankLines > -linesToScroll )
				max += linesToScroll;
			else
				max -= blankLines;
				
			SclSetScrollBar( barP, value, min, max, pageSize );
		}
	}
	else if ( linesToScroll > 0 )
	{
		FldScrollField( fldP, linesToScroll, winDown );
	}

}	// ScrollField



//=======================================================================
// PageScroll
//
// update the result field scroll bar and any other indicators associated
// with the result field.
//=======================================================================

void PageScroll( FieldPtr fldP, ScrollBarPtr barP, WinDirectionType direction )
{
	if ( fldP == NULL ) return;
	if ( barP == NULL ) return;
	
	if ( FldScrollable( fldP, direction ) )
	{
		UInt16	linesToScroll;
		Int16	value;
		Int16	min;
		Int16	max;
		Int16	pageSize;

		linesToScroll = FldGetVisibleLines( fldP ) - 1;
		FldScrollField( fldP, linesToScroll, direction );

		// update the scroll bar.
		
		SclGetScrollBar( barP, &value, &min, &max, &pageSize );

		if ( direction == winUp )
			value -= linesToScroll;
		else
			value += linesToScroll;
		
		SclSetScrollBar( barP, value, min, max, pageSize );
	}

}	// PageScroll	



//=======================================================================
// DrawBitmap
//
// if drawIt is true, draw a bitmap with resource ID resID, at position x, y
// relative to the current window. otherwise, erase a rectangle enclosing
// the bitmap specified by resID.
//=======================================================================

static void DrawBitMap( Int16 resID, Coord x, Coord y, Boolean drawIt )
{
	MemHandle	resH = DmGetResource( bitmapRsc, resID );
	BitmapPtr	resP = MemHandleLock( resH );


	if ( drawIt )
	{
		// ok, draw it!
		
		WinDrawBitmap( resP, x, y );
	}
	else
	{
		// erase it!
		
		RectangleType	bounds;
		
		bounds.topLeft.x	= x;
		bounds.topLeft.y	= y;
		bounds.extent.x	= resP->width;
		bounds.extent.y	= resP->height;
		
		WinEraseRectangle( &bounds, 0 );
	}
	

	// clean up.
	
	MemPtrUnlock( resP );
	DmReleaseResource( resH );

}	// DrawBitMap




//=======================================================================
// FingerResultUpdate
//
// update the result field scroll bar and any other indicators associated
// with the result field.
//=======================================================================

void FingerResultUpdate( Boolean doInit, UInt32 bytesFetched )
{
	static	Coord	actIndX;
	static	Coord	actIndY;
	static	Int16	updateState;					// drawing state
	const	Int16	netActBitMaps[ ] =				// bitmaps to draw
	{
		NetActBitMap + 0,
		NetActBitMap + 1,
		NetActBitMap + 2,
		NetActBitMap + 3,
		NetActBitMap + 4,
		0
	};
	
	UpdateScrollBar( GetObjectPtr( NULL, FingerResultField ),
					 	  GetObjectPtr( NULL, FingerResultScrollBar ) );


	if ( doInit )
	{
		// get the location of the gadget placeholder for the net activity
		// icons.
		
		FormPtr 	frmP = FrmGetActiveForm();
		FrmGetObjectPosition( frmP, FrmGetObjectIndex( frmP, FingerNetActivityGadget ),
									 &actIndX, &actIndY );

		// erase the activity icon.
		
		DrawBitMap( netActBitMaps[ 0 ], actIndX, actIndY, false );

		updateState = 0;
		
	}
	else
	{
		Char			tmpStr[ 20 ];

		// display the number of bytes received.

		StrPrintF( tmpStr, "%ld Bytes", bytesFetched ); 
		SetFieldTextFromStr( NULL, FingerRetrievedField, tmpStr );		

		
		// draw the activity icon.
		
		DrawBitMap( netActBitMaps[ updateState++ ], actIndX, actIndY, true );


		// update the state.
				
		if ( netActBitMaps[ updateState ] == 0 ) updateState = 0;	
	
	}
	
}	// FingerResultUpdate




//=======================================================================
// AppFormHandleEvent
//
// generic handler for the app's static forms
//	
//	11/10/99	BLT		Added call to FrmDrawForm() at the beginning of
//					 	frmOpenEvent logic since WinDraw is used in this 
//						case.
//=======================================================================

static Boolean AppFormHandleEvent( EventPtr event )
{
	FormPtr 	frmP;
	Boolean 	handled = true;

	// Get the form pointer
	frmP = FrmGetActiveForm();


	//---------------------------------------------------------------
	// Key Events
	//---------------------------------------------------------------
	if (event->eType == keyDownEvent)
	{
		switch( event->data.keyDown.chr )
		{
			case pageUpChr:			// Scroll up key presed?
				PageScroll( GetObjectPtr( frmP, FingerResultField ), 
							GetObjectPtr( frmP, FingerResultScrollBar),
							winUp );
				handled = true;
				break;
				
			case pageDownChr:		// Scroll down key presed?						
				PageScroll( GetObjectPtr( frmP, FingerResultField), 
							GetObjectPtr( frmP, FingerResultScrollBar),
							winDown );
				handled = true;
				break;
				
			default:
				handled = false;
				break;
				
		}	// switch
	}

		
	//---------------------------------------------------------------
	// Controls
	//---------------------------------------------------------------
	else if (event->eType == ctlSelectEvent)
	{
		switch( event->data.ctlSelect.controlID )
		{
			case FingerFingerButton:
				{
					MemHandle		whoStrH   = NULL;
					MemHandle		whereStrH = NULL;
				

					// if we have a valid network address, do the finger. ValidNetAddress()
					// puts up an error dialog if it doesn't like the address string.
					
					if ( ValidNetAddress( frmP, FingerAddressField, &whoStrH, &whereStrH ) )
					{
						// valid network address, make sure the connection is open,
						// dialing if necessary.
						
						if ( OpenNetwork() )
						{
							// the connection is open! do the finger...
							
							FingerIt( whoStrH, whereStrH, GetObjectPtr( frmP, FingerResultField ) );

							
							// show the Disconnect button.
							
							ShowControl( frmP, FingerDisconnectButton, true );
						
						}	// if OpenNetwork( É )
						
						// done with the address, so free the space.
						
						if ( whoStrH   != NULL ) MemHandleFree( whoStrH );
						if ( whereStrH != NULL ) MemHandleFree( whereStrH );
						
					}	// if ValidNetAddress( É )
					
				}
				break;
			
			case FingerDisconnectButton:
			
				// Disconnect button. close the connection and hide the Disconnect
				// button.
				
				CloseNetwork( true );
				ShowControl( frmP, FingerDisconnectButton, false );
				break;
			
			default:
				handled = false;
				break;

		}	// switch
	}



	//---------------------------------------------------------------
	// Menus
	//---------------------------------------------------------------
	else if ( event->eType == menuEvent )
	{
		handled = false;
	}

		
	//---------------------------------------------------------------
	// Pop up lists
	//---------------------------------------------------------------
	else if ( event->eType == popSelectEvent )
	{
		// stub for pop up lists...
		
		switch ( event->data.popSelect.listID )
		{
			default:
				handled = false;
				break;
		}	// switch
			
	}

		
	//---------------------------------------------------------------
	// Form Open
	//---------------------------------------------------------------
	else if ( event->eType == frmOpenEvent )
	{
		// Note that this call has moved to the beginning of this case since
		// we must draw the form *before* we do any WinDraw stuff to it.     
		FrmDrawForm( frmP );
	
		// set the current form ID global
		gCurrentFrmID = frmP->formId;

		// initialize the network globals.
		InitNetworkGlobals();
		
		// update the scroll bar and connection icon.
		FingerResultUpdate( true, 0 );	
		
		// put an initial address in the finger address field.
		SetFieldTextFromRes( frmP, FingerAddressField, TestAddr2String );

		// show the disconnect button if the network connection is live.
		ShowControl( frmP, FingerDisconnectButton, TestNetwork() );

		// clear the result field, freeing the text handle associated with
		// the field. it is important do do this before we do a Finger because
		// we will be putting a handle to a DB record in the field later. from
		// now on, we assume that any non-NULL handle in the result field is
		// to a DB record.
		ClearFieldText( frmP, FingerResultField );

		// set the focus to the address field.		
		/*FrmDrawForm(frmP) should be called *before* any Window.h routines are used*/
		FrmSetFocus( frmP, FrmGetObjectIndex( frmP, FingerAddressField ) );


		// create the finger database. exit the app if we fail to create
		// the DB ( CreateFingerDatabase() will post the alert if we fail ).
		if ( ! CreateFingerDatabase() )
		{
			EventType	newEvent;

			// exit the application by inserting an appStopEvent
			// into the event queue.
	    	MemSet( &newEvent, sizeof( EventType ), 0 );
		   	newEvent.eType = appStopEvent;
		   	EvtAddEventToQueue( &newEvent );
		}
	}

	//---------------------------------------------------------------
	// Form Close
	//---------------------------------------------------------------
	else if ( event->eType == frmCloseEvent )
	{
		handled = false;
	}

	//---------------------------------------------------------------
	// Form Update
	//---------------------------------------------------------------
	else if ( event->eType == frmUpdateEvent )
	{
		handled = false;
	}

	//---------------------------------------------------------------
	// Scroll Bar Update
	//---------------------------------------------------------------

	else if (event->eType == sclRepeatEvent)
	{
		ScrollField( GetObjectPtr( frmP, FingerResultField ),
					 GetObjectPtr( frmP, FingerResultScrollBar ),
					 ( event->data.sclRepeat.newValue - 
					   event->data.sclRepeat.value ) );
		handled = false;
	}

	//---------------------------------------------------------------
	// Everything else...
	//---------------------------------------------------------------
	else
	{
		handled = false;
	}

	return handled;

}	// AppFormHandleEvent



//=======================================================================
// AppHandleEvent
//
// loads form resources and sets the event handler for the form loaded.
//
// returns true if the event was handled and should not be passed
// to a higher level handler.
//=======================================================================

static Boolean AppHandleEvent( EventPtr event )
{
	Boolean	eventHandled = true;

	if ( event->eType == frmLoadEvent )
	{
		// Load the form resource.
		
		UInt16	formID	= event->data.frmLoad.formID;
		FormPtr	frm		= FrmInitForm( formID );

		FrmSetActiveForm( frm );		


		// set the event handler for the form. the handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		//
		// for this app, all forms are handled by the same routine.
			
		switch( formID )
		{
			case FingerForm:
				FrmSetEventHandler( frm, AppFormHandleEvent );
				break;



			// if you add more forms to the app, put a case in for each new
			// form...
		
							
			default:
				eventHandled = false;
				break;
				
		}	// switch
	}
	else
	{
		eventHandled = false;
	}
	
	
	return eventHandled;

}	// AppHandleEvent





//=======================================================================
// AppEventLoop
//
// loop to process application events.
//=======================================================================

static void AppEventLoop(void)
{
	UInt16	 	error;
	EventType 	event;


	do
	{
		// get an event to process.

		EvtGetEvent( &event, evtWaitForever );


		// normal processing of events.
		
		if ( !SysHandleEvent ( &event ) )
		{
			if ( !MenuHandleEvent( NULL /* CurrentMenuP */, &event, &error ) )
			{
				if ( !AppHandleEvent( &event ) ) FrmDispatchEvent( &event );

			}	// Menu
			
		}	// Sys
			
	}
	while( event.eType != appStopEvent );

}	// AppEventLoop




//=======================================================================
// AppStart
//
//=======================================================================

static UInt16 AppStart( void )
{

	return 0;
	
}	// AppStart



//=======================================================================
// StopApplication
//
//	11/10/99	BLT		Added call to FrmCloseAllForms()
//=======================================================================

static void StopApplication( void )
{
	// disassociate the text buffer (a DB chunk) from the result
	// form. if we do not, the form will try to free the space - we
	// would MUCH rather do this by explicitly removing the
	// record or (in our case), deleting the DB.
	
	FldSetTextHandle( GetObjectPtr( FrmGetActiveForm(), FingerResultField ), NULL );


	// close the network, but allow the connection to remain open
	// for other apps (on the device - close the connection under
	// the simulator).


#if EMULATION_LEVEL == EMULATION_NONE
	CloseNetwork( false );
#else
	CloseNetwork( true );
#endif
	
	// cleanup the DB.
	
	ClobberFingerDatabase( false );

	FrmCloseAllForms ();

}	// StopApplication




/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version meets your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *                             
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/15/96	Initial Revision
 *
 ***********************************************************************/
 
static Err RomVersionCompatible (UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	// See if we have at least the minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
		{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
			{
			FrmAlert (RomIncompatibleAlert);
		
			// Pilot 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < 0x02000000)
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			}
		
		return (sysErrRomIncompatible);
		}

	return (0);
}



//=======================================================================
// PilotMain
//
// application entry point.
//=======================================================================

#define version20			0x02000000

UInt32	PilotMain( UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags )
{

	// If normal launch
	
	if ( cmd == sysAppLaunchCmdNormalLaunch )
	{
		UInt16 error;


		// initialize globals.
		
		gCurrentFrmID = 0;
		gFingerDB	  = 0;	


		// check the ROM version - bail if it's not right...
		
		error = RomVersionCompatible( version20, launchFlags );
		if ( error ) return error;


		error = AppStart ();
		if ( error ) return error;


		// first form.
		
		FrmGotoForm( FingerForm );

		
		// run the event loop. returns to exit.
		
		AppEventLoop ();
		StopApplication ();

	}

	return 0;

}	// PilotMain

