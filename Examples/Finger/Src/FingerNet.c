/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FingerNet.c
 *
 * History:
 *		01/04/97		Gregory Toto, created. 
 *
 *****************************************************************************/

#include <PalmOS.h>
#include <sys_socket.h>



// Application headers

#include "FingerDemo.h"



// From NetSocket.c
//
// Global that is also declard by standard C library, if linked in.
// Rather than declare it here, possibly redundantly, we'll rely on the
//  app to declare it somewhere in it's source code if it doesn't already
//  link in the standard C library.

#ifdef __PALMOS_TRAPS__
Err errno;
#endif




//=======================================================================
//	globals

Boolean	gNetLibOpen;




//=======================================================================
// InitNetworkGlobals
//
// 
//=======================================================================

void InitNetworkGlobals( void )
{
//AppNetRefnum
	AppNetRefnum	= 0;
	gNetLibOpen		= false;
	
}	// InitNetworkGlobals




//=======================================================================
// ValidNetAddress
//
// parse the network address contained in the field with addrFieldID in
// the from on formP. return the address as a two strings: the part before
// the '@' on whoStrHP and the part after the '@' on whereStrHP. 
//=======================================================================

Boolean ValidNetAddress( FormPtr formP, UInt16 addrFieldID, MemHandle * whoStrHP,
																			 MemHandle * whereStrHP )
{

	Boolean	addrIsValid = false;
	FieldPtr	addrFieldP;	
	Char*		addrStr;

	// initialize the return values.
	
	*whoStrHP   = NULL;
	*whereStrHP = NULL;



	// get a reference to the address field.
		
	addrFieldP = (FieldPtr) FrmGetObjectPtr( formP, FrmGetObjectIndex( formP, addrFieldID ) );
	
	if ( addrFieldP != NULL )
	{
		Char* atPtr, *nextAtPtr;

		// get a reference to the field text.
				
		addrStr = FldGetTextPtr( addrFieldP );

		
		// find the '@' in the address.
		
		atPtr   = StrChr( addrStr, '@' );
		
		// then, find the last '@' in the address.
		
		while (atPtr && ( nextAtPtr = StrChr (atPtr+1, '@') ) )
			atPtr = nextAtPtr;
		
		
		
		if  ( atPtr != NULL )
		{
			Char*	tmpStr;
			
			// determine the lengths of the components of the address, before and 
			// after the '@'.
			
			UInt16		whoStrLen	= atPtr - addrStr;
			UInt16		whereStrLen = StrLen( atPtr + 1 );


			// get some space to contain the address strings.
						
			*whoStrHP	= MemHandleNew( whoStrLen   + 3 );	// extra for \r, \n and null
			*whereStrHP = MemHandleNew( whereStrLen + 1 );	// extra for null


			// copy the address strings, terminating the whoStr with \r\n.
			
			tmpStr = MemHandleLock( *whoStrHP );			
			StrNCopy( tmpStr, addrStr, whoStrLen );
			tmpStr[ whoStrLen     ] = '\r';
			tmpStr[ whoStrLen + 1 ] = '\n';
			tmpStr[ whoStrLen + 2 ] = 0;
			MemHandleUnlock( *whoStrHP );

			tmpStr = MemHandleLock( *whereStrHP );			
			StrCopy( tmpStr, ( atPtr + 1 ) );
			MemHandleUnlock( *whereStrHP );

			
			// indicate a valid address. we could do more checking - for example
			// check for a numeric, correctly formed whereStr for instance...
			
			addrIsValid = true;
		
		}
		else
			ShowErrorDialog( BadAddressString, NULL );
	}
	else
		AlertPrintf( "ValidNetAddress", "FrmGetObjectPtr returned NULL" );
	
	return addrIsValid;

}	// ValidNetAddress




//=======================================================================
// FindNetworkLibrary
//
// find the network library. return true on success, false else.
//
// AppNetRefnum is initialized if it has not already been initialized
// the by the app.
//=======================================================================

Boolean FindNetworkLibrary( void )
{
	Boolean	retVal	= true;
	Err		sysError	= 0;
	UInt32	version;
	

	// if we haven't already gotten the net library, check if it
	// is available, and get a reference to it if so.
	
	if ( AppNetRefnum == 0 )
	{
	
		// use the Feature Manager to detemine if the net library is
		// installed.
		
		sysError = FtrGet( netFtrCreator, netFtrNumVersion, &version );
		
		if ( sysError == 0 )
		{
			// get a reference to the Net.lib into AppNetRefnum.

			sysError = SysLibFind( "Net.lib", &AppNetRefnum );

			if ( sysError )
			{
				AppNetRefnum = 0;
				retVal		 = false;
			}
		}
		else
		{
			retVal = false;
		}
	}

	return retVal;

}	// FindNetworkLibrary




//=======================================================================
// OpenNetwork
//
// open a network connection. return true on success, false else.
//
// gNetLibOpen is set true on success.
//
// AppNetRefnum is initialized if it has not already been initialized
// by the app.
//=======================================================================

Boolean OpenNetwork( void )
{

	// if we've already opened the connection, don't do it again...
	 
	if ( !gNetLibOpen )
	{
		UInt16	ifErrs	= 0;
		Err	sysError	= 0;
		
		
		if ( !FindNetworkLibrary() )
		{
			// couldn't find the network library. assume it's not present on
			// the device.
			
			ShowErrorDialog( NetNotAvailErrorString, NULL );
			AppNetRefnum = 0;
		}
		else
		{
			char errStr[ 10 ];

			// ok, got Net.lib. now open it...
			
			sysError = NetLibOpen( AppNetRefnum, &ifErrs );
			
			if ( ifErrs != 0 )
			{
				NetLibClose( AppNetRefnum, true );
				AppNetRefnum = 0;
				StrPrintF( errStr, "%d", ifErrs );
				ShowErrorDialog( ObscureErrorString, errStr );
			}
			else
			{
				switch ( sysError )
				{
					case 0:
					case netErrAlreadyOpen:
						gNetLibOpen = true;
						break;
						
					case netErrOutOfMemory:
						ShowErrorDialog2( OutOfMemErrorString, OpenConnString );
						break;
					
					case netErrNoInterfaces:
					case netErrPrefNotFound:
						ShowErrorDialog( BadSetupErrorString, NULL );
						break;
						
					default:
						sprintf( errStr, "%d", sysError );
						ShowErrorDialog( ObscureErrorString, errStr );
						break;
				}	// switch
			}	// if ( ifErrs != 0 )		
		}	// if ( !FindNetworkLibrary() )
	}	// if ( !gNetLibOpen )
	
	return gNetLibOpen;
			
}	// OpenNetwork



//=======================================================================
// CloseNetwork
//
// close the network connection. return true if the network is closed,
// false else.
//=======================================================================

Boolean CloseNetwork( Boolean forceToClose )
{
	Boolean closeResult = true;


	// test on gNetLibOpen and also on AppNetRefnum because we may not
	// have opened the connection in this app, or in this instantiation
	// of this app (e.g. we may have opened a connection, gone to another
	// app, and returned).
	
	if ( gNetLibOpen || ( AppNetRefnum != 0 ) )
	{

		closeResult = ( NetLibClose( AppNetRefnum, forceToClose ) != netErrStillOpen );
			
		// the NetLib may really still be open - NetLibClose may have
		// returned netErrStillOpen (because we didn't force a close - 
		// forceToClose may have been false).
		
		gNetLibOpen		= false; 		
		AppNetRefnum	= 0;
	}
	
	return closeResult;
	
}	// CloseNetwork





//=======================================================================
// RefreshNetwork
//
// check the status of the connection and reestablish it (if it is down), 
// if recconnectIfDown is true. returns true if connected when this
// routine returns, false else.
//
// AppNetRefnum is initialized if it has not already been initialized
// the by the app.
//
// don't call this unless OpenNetwork() has been called.
//=======================================================================

Boolean RefreshNetwork( Boolean reconnectIfDown )
{

	UInt16		ifErrs	= 0;
	Boolean	allUp		= false;
	Boolean	isConn	= false;
	Err		sysError	= 0;
	

	// check the status of the connection, and bring it up if 
	// reconnectIfDown is true. allUp will contain the
	// connection status.

	sysError	= NetLibConnectionRefresh( AppNetRefnum, reconnectIfDown,
													&allUp, &ifErrs );
	
													
	if ( sysError != 0 )
	{
		ShowErrorDialog( NetRefreshErrorString, "." );		
	}
	else if ( ifErrs != 0 )
	{
		char errStr[ 10 ];
		
		StrPrintF( errStr, ": %d", ifErrs );
		ShowErrorDialog( NetRefreshErrorString, errStr );
	}
	else
		isConn = allUp;

	
	return isConn;
			
}	// RefreshNetwork




//=======================================================================
// TestNetwork
//
// a bit of a hack to check whether we have a connection or not - we
// assume that if the net lib is open, that we have a connection. this
// might not really be true: the library might be closed but the connection
// is still open, or the library might be open but the line has dropped...
//
// we can't use RefreshNetwork() to do this, because we must open the
// connection to use RefreshNetwork() (which will bring up the user
// interface for the connection if the connection is not already up).
//
// this routine can also be used to test whether the device supports
// the net library. it puts up a dialog if there is no network support.
//
// AppNetRefnum is initialized if it has not already been initialized
// the by the app.
//
// can be called before OpenNetwork() has been called.
//=======================================================================

Boolean TestNetwork( void )
{

	UInt16		openCount	= 0;
	Boolean	isConn		= false;
	

	if ( !FindNetworkLibrary() )
	{
		// couldn't find the network library. assume it's not present on
		// the device.
			
		ShowErrorDialog( NetNotAvailErrorString, NULL );
		AppNetRefnum = 0;
	}
	else
	{

		// is the net lib open?
		
		NetLibOpenCount( AppNetRefnum, &openCount );
		
		if ( openCount > 0 ) isConn = true;

	}

	return isConn;
			
}	// TestNetwork



//=======================================================================
// FingerIt
//
// finger the user on whoStrH at server whereStrH and send the result to
// the field on resFldP. the routine grabs data from the server in
// CHUNK_SIZE blocks until all the data is retrieved or an error occurs.
// space for the returned data is allocated outside the dynamic heap.
//=======================================================================

#define CHUNK_SIZE 2048
#define MAX_FIELD_SIZE 32767

void FingerIt( MemHandle whoStrH, MemHandle whereStrH, FieldPtr resFldP )
{

	// get references to the component strings of the address.
	
	MemPtr 			whoP		 = MemHandleLock( whoStrH );
	MemPtr 			whereP	 = MemHandleLock( whereStrH );
	UInt16			whoLen	 = StrLen( whoP );

	UInt16			dbRecIndx = 0;

	MemHandle		rdBufH	 = NULL;		// buffer for NetUWriteN()
	Char*			rdBufP	 = NULL;

	MemHandle		bufH		 = NULL; 	// buffer for finger data
	MemPtr			bufP		 = NULL;
	UInt32			bufLen	 = 0;
	UInt32			bufOffset = 0;

	MemHandle		tmpH		 = NULL;		// tmp buf handle

	NetSocketRef	fd			 = -1;		// socked file descriptor
	int     		port		 = 0;


	// make sure the connection is open!
	
	RefreshNetwork( true );


	// free the text buffer currently in the result field. if there
	// is a handle associated with the field, it belongs to a 
	// DB record  - so free the record. we know the handle belongs to
	// a DB because we freed the default handle associated with the
	// form when our main form opened.
	
	if ( FldGetTextHandle( resFldP ) != NULL )
	{
		FldSetTextHandle( resFldP, NULL );
		DmRemoveRecord( gFingerDB, 0 );
	}
	

	// try to open a finger connection to the server...
	
	if ( ( fd = NetUTCPOpen( whereP, "finger", port ) ) < 0 )
	{
		ShowErrorDialog( CouldNotOpenConnErrorString, NULL );
		goto _FingerExit;
	}
	
	
	// allocate a small buffer for receiving data from the server.
	
	rdBufH = MemHandleNew( CHUNK_SIZE );
	if ( rdBufH == NULL)
	{
		ShowErrorDialog2( OutOfMemErrorString, AllocRxBufString );
		goto _FingerExit;
	}
	rdBufP = MemHandleLock( rdBufH );

	
	// allocate some space outside the dynamic heap but in a DB!. there
	// is a 64K limitation of chunks in the dynamic heap, but not in 
	// a DB. bufH will be associated with form, so we have to make sure
	// the form manager does not free it when the form is closed. instead,
	// we will free it by deleting the DB when the app exits.
	bufLen	 = CHUNK_SIZE + 1;
	bufOffset = 0;
	
	// if the Record exists, use it--don't create another one in its place.
	if (DmNumRecords(gFingerDB) > 0)
	 		bufH = DmResizeRecord( gFingerDB, dbRecIndx, bufLen );
	else
		bufH = DmNewRecord( gFingerDB, &dbRecIndx, bufLen );
		
	if ( bufH == NULL)
	{
		ShowErrorDialog2( OutOfMemErrorString, AllocDBMemString );
		goto _FingerExit;
	}

	
	// null terminate the buffer by writing 0 at the end.
	
	bufP		 = MemHandleLock( bufH );
	DmSet( bufP, bufOffset, 1, 0 );
	MemHandleUnlock( bufH );


	// set the results field to display from the buffer. the
	// the previous display buffer has already been freed.

	FldSetTextHandle( resFldP, bufH );


	// ok cool, the connection is open and we have buffer space for the
	// result - now send the 'fingeree' to the server.
					
	if ( NetUWriteN( fd, (UInt8 *) whoP, (UInt32) whoLen ) != whoLen )
	{
		ShowErrorDialog( WhoWriteErrorString, NULL );
		goto _FingerExit;
	}


	while ( true ) 
	{
		Int16	 i;
		Int32 rdLen = NetUReadN( fd, (UInt8 *) rdBufP, (UInt32) CHUNK_SIZE );

		// exit if no data or error.
		
		if ( rdLen == 0 ) break;
		if ( rdLen  < 0 )
		{

			ShowErrorDialog( NetReadErrorString, NULL );
			break;
		}
		
		// eat any data that is over the max field size
		
	 	if ( ( bufOffset + rdLen + 1 ) > MAX_FIELD_SIZE )
	 		rdLen = MAX_FIELD_SIZE - (bufOffset + 1);
	 	
	 	// if there is no room left, just keep reading until done
	 		
	 	if ( rdLen <= 0 )
	 		break;

		for ( i = 0; i < rdLen; i++ )
		{
			// substitute ' ' for CR because it displays
			// better in the field.
			
			if ( rdBufP[ i ] == '\r' ) rdBufP[ i ] = ' ';
		}	// for
	 		

		// if we need more buffer space to contain the received data
		// then try to allocate it...
		
	 	if ( ( bufOffset + rdLen + 1 ) > bufLen )
	 	{
	 		MemHandle	newBufH;
	 			
	 		//kja MemHandleUnlock( bufH );
	 		bufP = 0;

	 		
	 		// force the result field to release it too
	 		 		  
	 		FldSetTextHandle( resFldP, NULL );


			// attempt to resize the buffer.
				 		
	 		newBufH = DmResizeRecord( gFingerDB, dbRecIndx, bufLen + CHUNK_SIZE );
	 		if ( newBufH == NULL )
	 		{
	 			// the resize failed...let the field keep what we
	 			// have already received.
	 			
	 			bufH = DmGetRecord( gFingerDB, dbRecIndx );
	 				
	 			FldSetTextHandle( resFldP, bufH ); 	
		 		//kja bufP = MemHandleLock( bufH );
	 			break;
	 		}
	 		else
	 			bufH = newBufH;
	 			
	 		//kja bufP    = MemHandleLock( bufH );
	 		bufLen += CHUNK_SIZE;
	 		
	 	}	
	 	

		// save the received data to our display buffer, adding a 
		// null termination at the end.	
		bufP = MemHandleLock( bufH );	 	 
	 	DmWrite( bufP, bufOffset, rdBufP, rdLen );
	 	bufOffset += rdLen;
	 	DmSet( bufP, bufOffset, 1, 0 );
	 	MemHandleUnlock( bufH );


		// a bit of funkyness to update the text in the result field.
		// result field is NOT editable by the user. make it editable
		// to update the text, then put it back to not editable.
		
		resFldP->attr.editable = true;
		FldSetTextHandle( resFldP, bufH );
	 	FldDrawField( resFldP );
	 	resFldP->attr.editable = false;

		FingerResultUpdate( false, bufOffset );

	 }	// while ( true )
	 


	// cleanup the mess...

_FingerExit:

	MemHandleUnlock( whoStrH );
	MemHandleUnlock( whereStrH );
	
	if ( rdBufH != NULL )
	{
		MemHandleUnlock( rdBufH );
		MemHandleFree( rdBufH );
	}			
	if ( bufH != NULL )
	{
		// don't free the handle here. bufH must be freed with a Data Manager
		// operation since it is a chunk in a DB. also, it is now 
		// associated with the result field text. we must make sure that
		// the Form Manager does not free it when the form closes.
		
		//kja MemHandleUnlock( bufH );
	}			
	if ( fd >= 0 ) close( fd );
	
	// clean up the status.
	
	FingerResultUpdate( true, bufOffset );


}	// FingerIt

