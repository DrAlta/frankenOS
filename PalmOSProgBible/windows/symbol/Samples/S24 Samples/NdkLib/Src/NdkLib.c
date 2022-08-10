/***********************************************************************
 *
 *	Copyright (c) 1996-1997, USRobotics Corp., All Rights Reserved
 *
 * PROJECT:			PalmPilot
 *
 * FILE:			SampleLib.c
 *
 * AUTHOR:			vmk, 5/20/97
 *
 * DECLARER:
 *
 * DESCRIPTION:	Sample library implementation.  The Sample library serves
 *		as an example of creating PalmPilot shared libraries.
 *
 *	  
 * REVISION HISTORY:
 *   	12/18/97  scl Added BUILDING_SAMPLE_LIB to fix building in CW Pro 2
 *    05/14/99  dcat created NDKLIB example for Spectrum24 NDK
 *    06/26/99  dcat now read/write update the struct with #bytes processed
 **********************************************************************/

#include <Pilot.h>			// Includes all PalmPIlot headers
#include <sys_socket.h>

// Our library public definitions (library API)
#define BUILDING_SAMPLE_LIB

// Our library private definitions (library globals, etc.)
#include "NdkLibPrv.h"
#include "NdkLib.h"

/********************************************************************
 * LIBRARY GLOBALS:
 *
 * IMPORTANT:
 * ==========
 * Libraries are *not* allowed to have global or static variables.  Instead,
 * they allocate a memory chunk to hold their persistent data, and save
 * a handle to it in the library's system library table entry.  Example
 * functions below demostrate how the library "globals" chunk is set up,
 * saved, and accessed.
 *
 * We use a movable memory chunk for our library globals to minimize
 * dynamic heap fragmentation.  Our library globals are locked only
 *	when needed and unlocked as soon as possible.  This behavior is
 * critical for healthy system performance.
 *
 ********************************************************************/
/********************************************************************
 * Internally used data structures
 ********************************************************************/
/********************************************************************
 * Internally used routines
 ********************************************************************/
static SampleLibGlobalsPtr PrvMakeGlobals(UInt refNum);
static void PrvFreeGlobals(UInt refNum);
static SampleLibGlobalsPtr PrvLockGlobals(UInt refNum);
static Boolean PrvIsLibOpen(UInt refNum);
static Err PrvCreateClientContext(SampleLibGlobalsPtr gP, DWordPtr clientContextP);
static Err PrvDestroyClientContext(SampleLibGlobalsPtr gP, DWord clientContext);
static SampleLibClientContextPtr PrvLockContext(DWord context);
static Boolean RefreshNetwork (SocketTypeP TcpParmsP);
static int CheckForNetwork (SocketTypeP TcpParmsP);
static int WriteN (SocketTypeP TcpParmsP);
static int ReadN (SocketTypeP TcpParmsP);

/********************************************************************
 * Internally used macros
 ********************************************************************/

// Unlock globals
#define PrvUnlockGlobals(gP)	MemPtrUnlock(gP)

// Unlock the client context
#define PrvUnlockContext(contextP)	MemPtrUnlock(contextP)


 
// LIBRARY VERSION
//
// The library version scheme follows the system versioning scheme.
// See sysMakeROMVersion and friends in SystemMgr.h.
//
// For reference:
//
// 0xMMmfsbbb, where MM is major version, m is minor version
// f is bug fix, s is stage: 3-release,2-beta,1-alpha,0-development,
// bbb is build number for non-releases 
// V1.12b3   would be: 0x01122003
// V2.00a2   would be: 0x02001002
// V1.01     would be: 0x01013000

#define prvSampleLibVersion	sysMakeROMVersion(1, 0, 1, sysROMStageDevelopment, 1)


/********************************************************************
 * Sample Library API Routines
 ********************************************************************/
 
 
/***********************************************************************
 *
 * FUNCTION: NdkLibOpen       
 *
 * DESCRIPTION: 
 *
 *
 * PARAMETERS: 
 *  
 * RETURNED: 
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/
Err NdkLibOpen (UInt refNum)
{
	SampleLibGlobalsPtr	gP;
	Err	err = 0;
	Int	originalOpenCount = 0;


	// Error-check our parameters
	//ErrFatalDisplayIf(clientContextP == NULL, "null context variable pointer");
	
	
	// Initialize return variable
	//*clientContextP = 0;

	// Get library globals
	gP = PrvLockGlobals(refNum);		// lock our library globals
	
	// Check if already open
	if (!gP)
		{
		// Allocate and initialize our library globals.
		gP = PrvMakeGlobals(refNum);		// returns locked globals on success
		if ( !gP )
			err = sampleErrMemory;
		}
	
	// If we have globals, 
	//    create a client context, 
	//    increment open count, 
	//    and unlock our globals
	if ( gP )
		{
		originalOpenCount = gP->openCount;// save original open count for error handling
		
		//err = PrvCreateClientContext(gP, clientContextP);		// create a context for client-specific data
		//if ( !err )
			//gP->openCount++;		// increment open count on success
			
		gP->openCount++;		// increment open count on success
		
		PrvUnlockGlobals(gP);	// unlock our library globals
		
		// If there was an error creating a client context and there are no other clients,
		//   free our globals
		if ( err && (originalOpenCount == 0) )
			PrvFreeGlobals(refNum);
		}
	
	return( err );
}


/***********************************************************************
 *
 * FUNCTION: NdkLibClose       
 *
 * DESCRIPTION: Close the library
 *
 *
 * PARAMETERS: 
 *  refNum-------NdkLib refnum from open of NdkLib
 *
 * RETURNED: 0 if aok else err code
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/
Err NdkLibClose (UInt refNum)
{
	SampleLibGlobalsPtr	gP;
	Int						openCount;
	Int						contextCount;
	Err						err = 0;


	gP = PrvLockGlobals(refNum);	// lock our globals
	
	// If not open, return
	if (!gP)
		{
		return 0;			// MUST return zero here to get around a bug in system v1.x that
									// would cause SysLibRemove to fail.
		}

	// Destroy the client context 
	//  (we ignore the return code in this implementation)
	//PrvDestroyClientContext(gP, clientContext);
	
	// Decrement our library open count
	gP->openCount--;
	
	// Error check for open count underflow
	ErrFatalDisplayIf(gP->openCount < 0, "Sample lib open count underflow");
	
	
	// Save the new open count and the context count
	openCount = gP->openCount;
	contextCount = gP->contextCount;
	
	PrvUnlockGlobals(gP);			// unlock our globals
	
	
	// If open count reached zero, free our library globals
	if ( openCount <= 0 )
		{
		// Error check to make sure that all client contexts were destroyed
		ErrFatalDisplayIf(contextCount != 0, "not all client contexts were destroyed");
		
		// Free our library globals
		PrvFreeGlobals(refNum);
		}
	else
		err = sampleErrStillOpen;	// return this error code to inform the caller
											// that others are still using this library

	return err;
}


/***********************************************************************
 *
 * FUNCTION: NdkLibSleep(       
 *
 * DESCRIPTION: Stub to support the library model
 *
 *
 * PARAMETERS: 
 *  refNum-------NdkLib refnum from open of NdkLib
 *
 * RETURNED: 0
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/
Err NdkLibSleep(UInt refNum)
{
	return( 0 );
}



/***********************************************************************
 *
 * FUNCTION: NdkLibWake       
 *
 * DESCRIPTION: Stub to support the library model
 *
 *
 * PARAMETERS: 
 *  refNum-------NdkLib refnum from open of NdkLib
 *
 * RETURNED: 0
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/
Err NdkLibWake(UInt refNum)
{
	return( 0 );
}


/***********************************************************************
 *
 * FUNCTION: NdkLibGetLibAPIVersion       
 *
 * DESCRIPTION: return the version number of the library
 *
 *
 * PARAMETERS: 
 *  refNum-------NdkLib refnum from open of NdkLib
 *  dwVerP-------pointer to store value
 *
 * RETURNED: 0 if aok else update users version pointer
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/
Err NdkLibGetLibAPIVersion(UInt refNum, DWordPtr dwVerP)
{
	
	// Error-check our parameters
	ErrFatalDisplayIf(dwVerP == NULL, "null pointer argument");
	
	
	*dwVerP = prvSampleLibVersion;
	
	return( 0 );
}

/***********************************************************************
 *
 * FUNCTION:   RefreshNetwork        
 *
 * DESCRIPTION: Called by Echo to make sure we're still up and running.
 * Check the status of the connection and re-establish it (if it's down),
 * if recconnectIfDown is true. returns true if connected when this
 * routine returns, false else.
 *
 * This handles the case where the user powers the unit off/on and expects
 * the socket to maintain its connection.
 *
 * Don't call this unless tcp_open() has been called.
 *
 * PARAMETERS:   
 *
 * RETURNED:        
 *             
 * REVISION HISTORY:
 *			Name	Date		    Description
 *			----	----		    -----------
 *      dcat  02/14/1999 Initial Revision  
 ***********************************************************************/
Boolean RefreshNetwork (SocketTypeP SocketParmsP)
{
	Word	ifErrs		= 0;
	Err		sysError	= 0;
	Boolean	allUp		= false;
	Boolean	isConn		= false;

	// check the status of the connection, and bring it up if 
	// reconnectIfDown is true. allUp will contain the
	// connection status.
	sysError = NetLibConnectionRefresh (SocketParmsP->NetLibRefNum, true,
										&allUp, &ifErrs );
	if (sysError)
		return 1;

	return 0;
			
}	


/***********************************************************************
 *
 * FUNCTION:   CheckForNetwork        
 *
 * DESCRIPTION:Called by GetTcpSocket to open NetLib    
 *
 * PARAMETERS:   
 *
 * RETURNED:   0 if aok, else error code.  Sets LibRefNum value    
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/
int CheckForNetwork (SocketTypeP SocketParmsP)
{
 Word ifErrs;
 Err error;
 
	error = SysLibFind( "Net.lib", &SocketParmsP->NetLibRefNum);
	if (error)
	{
		return error;
	}
		
	error = NetLibOpen(SocketParmsP->NetLibRefNum, &ifErrs);
	if ( error == netErrAlreadyOpen )
		return 0;  // that's ok
	
	if ( error || ifErrs )
	{
		NetLibClose (SocketParmsP->NetLibRefNum, true);
		return 1;
	} 
	
	return 0; // all is well
}

/***********************************************************************
 *
 * FUNCTION: NdkLibGetSocket       
 *
 * DESCRIPTION: Open NetLib and get a socket
 *
 * PARAMETERS:  TcpParmsP pointer to our data structure
 *
 * RETURNED: 0 if aok else an error code
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/
int NdkLibGetSocket (SocketTypeP SocketParmsP, int UDP)
{
	int		fd, result, AddrSize;
	unsigned long	inaddr;			
	
	result = CheckForNetwork (SocketParmsP);
	if (result) // opens NetLib & sets NetLibRefNum
		return -1;

	//___clear out the socket struct
	AddrSize =	sizeof(*SocketParmsP->srv_addrP);  
	MemSet((char*)SocketParmsP->srv_addrP, AddrSize, 0);	

	if (SocketParmsP->port <= 0) 
		return -2; // invalid port
	
	SocketParmsP->srv_addrP->sin_family = AF_INET;
	SocketParmsP->srv_addrP->sin_port = htons(SocketParmsP->port);	

	inaddr = NetLibAddrAToIN (SocketParmsP->NetLibRefNum, SocketParmsP->hostIP);		 
	if (inaddr != INADDR_NONE)
		MemMove((char*)&SocketParmsP->srv_addrP->sin_addr, (char*)&inaddr, sizeof(inaddr));			
	else 
		return -3; // its a name, we don't do names
		
	//___does the user want a Udp socket (the execption)	
	if (UDP)
		fd = NetLibSocketOpen (SocketParmsP->NetLibRefNum, AF_INET, SOCK_DGRAM,
								0, SocketParmsP->NetLibTimeOut,&SocketParmsP->errNo);
	else
		fd = NetLibSocketOpen (SocketParmsP->NetLibRefNum, AF_INET, SOCK_STREAM,
								0, SocketParmsP->NetLibTimeOut,&SocketParmsP->errNo);

	if (fd < 0)
		return -4;  // Can't create TCP socket		
		
	//___the special stuff needed for a UDP socket	see udp_open.c
	if (UDP)
	{
		MemSet((char*)SocketParmsP->cli_addrP, AddrSize, 0);
		SocketParmsP->cli_addrP->sin_family = AF_INET;
		SocketParmsP->cli_addrP->sin_addr.s_addr = htonl(INADDR_ANY);
		SocketParmsP->cli_addrP->sin_port = htons(0);
		result =NetLibSocketBind (SocketParmsP->NetLibRefNum, fd,
					         (NetSocketAddrType*)SocketParmsP->cli_addrP,
					             AddrSize,SocketParmsP->NetLibTimeOut, &SocketParmsP->errNo);
		if (result < 0) 
		{
			//SetFieldText(MainStatusField,"UDP bind error",39, true);
			NetLibSocketClose (SocketParmsP->NetLibRefNum, fd, 
								SocketParmsP->NetLibTimeOut, &SocketParmsP->errNo);
			return -5;
		}
			
	} //___end UDP only
		

	//___Connect to the server
	result = NetLibSocketConnect (SocketParmsP->NetLibRefNum, fd,
	                (NetSocketAddrType*)SocketParmsP->srv_addrP,		                
	                AddrSize, SocketParmsP->NetLibTimeOut, &SocketParmsP->errNo);
 	if (result < 0)
	{
		NetLibSocketClose (SocketParmsP->NetLibRefNum, fd, 
							SocketParmsP->NetLibTimeOut, &SocketParmsP->errNo);
		return -6; // Can't connect to server
	}
	
	//___we need to limit the UDP socket to 1024 or it will fail on the write
	if (UDP)
	{
		if (SocketParmsP->data_size > 1024)
		{
			SocketParmsP->data_size = 1024;
		}
	}	

	return (fd);	// We now have a socket...
}


/***********************************************************************
 *
 * FUNCTION: NdkLibCloseSocket       
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  TcpParmsP pointer to our data structure
 *
 * RETURNED: 0 if aok else an error code
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/

int NdkLibCloseSocket (SocketTypeP SocketParmsP)
{
	if (SocketParmsP->mySocket)
	{ 
		NetLibSocketClose (SocketParmsP->NetLibRefNum, SocketParmsP->mySocket, 
							SocketParmsP->NetLibTimeOut, &SocketParmsP->errNo);
		SocketParmsP->mySocket = 0;
	}

	if (SocketParmsP->NetLibRefNum)
	{ 
    	NetLibClose (SocketParmsP->NetLibRefNum, true);
		SocketParmsP->NetLibRefNum = 0;
	}

	return 0;
}

/***********************************************************************
 *
 * FUNCTION:    WriteN             
 *
 * DESCRIPTION: called by WriteSocket to write "n" bytes to a descriptor.    
 *              
 * PARAMETERS:
 * fd______a socket descriptor  
 * ptr_____pointer to data to be written
 * nbytes__how much to write
 *
 * RETURNED: bytes actually written     
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/

int WriteN (SocketTypeP SocketParmsP)
{
	int	fd, nbytes, nleft, nwritten, status;
	BytePtr ptr; 
	
	fd = SocketParmsP->mySocket;
	ptr = (BytePtr)SocketParmsP->data_buffP;
	nbytes = SocketParmsP->data_size;

	nleft = nbytes;
	status  = RefreshNetwork (SocketParmsP);
	if (status)
		return 0;	
			
	while (nleft > 0) 
	{
		nwritten =NetLibSend (SocketParmsP->NetLibRefNum, fd,(const VoidPtr)ptr,nleft, 
							  0,0,0,SocketParmsP->NetLibTimeOut, &SocketParmsP->errNo);
		if (nwritten <= 0)/* error */
			break;

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return (nbytes - nleft);
}

/***********************************************************************
 *
 * FUNCTION: NdkLibWriteSocket       
 *
 * DESCRIPTION: Write data out to our socket
 *
 * PARAMETERS:  TcpParmsP pointer to our data structure
 *
 * RETURNED: 0 if aok, 1 if error
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/

int NdkLibWriteSocket (SocketTypeP SocketParmsP)
{
	int sendBytes, sentBytes;
	
	sendBytes = SocketParmsP->data_size;		
	sentBytes = WriteN (SocketParmsP);
	
	//___tell the user home many bytes we actual wrote
	SocketParmsP->data_size = sentBytes;
	
	if (sentBytes != sendBytes) 
		return 1;
	
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    ReadN    
 *
 * DESCRIPTION: called by WriteSocket  to read "n" bytes from a descriptor.
 *
 * PARAMETERS:
 * fd______a socket descriptor  
 * ptr_____pointer to store the data received
 * nbytes__how many were expecting to receive
 *
 * RETURNED: bytes actually read     
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/
int ReadN (SocketTypeP SocketParmsP)
{
	int	fd, nleft, nread, nbytes, status;
	BytePtr ptr;
	
	fd = SocketParmsP->mySocket;
	ptr = (BytePtr)SocketParmsP->data_buffP;	
	nbytes = SocketParmsP->data_size;
	

	status  = RefreshNetwork (SocketParmsP);	
	if (status)
		return 0;	
		
	nleft = nbytes;
	while (nleft > 0) 
	{
		nread = NetLibReceive (SocketParmsP->NetLibRefNum, fd, ptr, nleft,  
					0,0,0, SocketParmsP->NetLibTimeOut, &SocketParmsP->errNo);
				
		if (nread < 0) /* error, return < 0 x1212 = timeout*/ 
			break;			
		else if (nread == 0)
			break;			/* EOF */

		nleft -= nread;
		ptr += nread;
	}
	return (nbytes - nleft);		/* return >= 0 */
}

/***********************************************************************
 *
 * FUNCTION: NdkLibReadSocket       
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  TcpParmsP pointer to our data structure
 *
 * RETURNED: 0 if aok, 1 if error
 *             
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 ***********************************************************************/
int NdkLibReadSocket (SocketTypeP SocketParmsP)
{
	int rcvBytes, BytesToRead;	
	
	BytesToRead = SocketParmsP->data_size;
	rcvBytes = ReadN (SocketParmsP);
	
	//___tell the user home many bytes we actual read
	SocketParmsP->data_size = rcvBytes;
				
	if (rcvBytes != BytesToRead) 
		return 1;

	return 0;
}


/************************************************************
 *
 *  FUNCTION: PrvMakeGlobals
 *
 *  DESCRIPTION:	Create our library globals.
 *
 *  PARAMETERS:	refNum		-- Sample library reference number
 *
 *  CALLED BY:		internal
 *
 *  RETURNS:		pointer to our *locked* library globals; NULL if our globals
 *						could not be created.
 *
 *  CREATED: 5/20/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
static SampleLibGlobalsPtr PrvMakeGlobals(UInt refNum)
{
	SampleLibGlobalsPtr	gP = NULL;	// our globals pointer
	VoidHand					gH;						// our globals handle
	SysLibTblEntryPtr		libEntryP;	// pointer to our library table entry


	// Get library globals
	libEntryP = SysLibTblEntry(refNum);	// get our system library table entry
	ErrFatalDisplayIf(libEntryP == NULL, "invalid Sample lib refNum");
	
	// Error check to make sure the globals don't already exist
	ErrFatalDisplayIf(libEntryP->globalsP, "Sample lib globals already exist");	
	
	// Allocate and initialize our library globals.
	gH = MemHandleNew(sizeof(SampleLibGlobalsType));
	if ( !gH )
		return( NULL );											// memory allocation error
	
	// Save the handle of our library globals in the system library table entry so we
	// can later retrieve it using SysLibTblEntry().
	libEntryP->globalsP = (void*)gH;

	// Lock our globals
	gP = PrvLockGlobals(refNum);				// this should not fail
	ErrFatalDisplayIf(gP == NULL, "failed to lock Sample lib globals");
	
		
	// Set the owner of our globals memory chunk to "system" (zero), so it won't get
	// freed automatically by Memory Manager when the first application to call
	// SampleLibOpen exits.  This is important if the library is going to stay open
	// between apps.
	MemPtrSetOwner(gP, 0);
	
	// Initialize our library globals
	MemSet(gP, sizeof(SampleLibGlobalsType), 0);
	gP->thisLibRefNum = refNum;		// for convenience and debugging (althouth not used in this sample library)
	gP->openCount = 0;					// initial open count
	

	return( gP );							// return a pointer to our *locked* globals
}



/************************************************************
 *
 *  FUNCTION: PrvFreeGlobals
 *
 *  DESCRIPTION:	Free our library globals.
 *
 *  PARAMETERS:	refNum		-- Sample library reference number
 *
 *  CALLED BY:		internal
 *
 *  RETURNS:		nothing
 *
 *  CREATED: 5/20/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
static void PrvFreeGlobals(UInt refNum)
{
	VoidHand					gH;									// our globals handle
	SysLibTblEntryPtr		libEntryP;							// pointer to our library table entry


	// Get our library globals handle
	libEntryP = SysLibTblEntry(refNum);						// get our system library table entry
	ErrFatalDisplayIf(libEntryP == NULL, "invalid Sample lib refNum");
	
	gH = (VoidHand)(libEntryP->globalsP);					// get our globals handle from the entry
	
	// Free our library globals
	if ( gH )
		{
		libEntryP->globalsP = NULL;							// clear our globals reference
		MemHandleFree(gH);										// free our globals
		}

}



/************************************************************
 *
 *  FUNCTION: PrvLockGlobals
 *
 *  DESCRIPTION:	Lock our library globals.
 *
 *  PARAMETERS:	refNum		-- Sample library reference number
 *
 *  CALLED BY:		internal
 *
 *  RETURNS:		pointer to our library globals; NULL if our globals
 *						have not been created yet.
 *
 *  CREATED: 5/20/97 
 *
 *  BY: vmk
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   02/14/1999 Initial Release
 *
 *
 *************************************************************/
static SampleLibGlobalsPtr PrvLockGlobals(UInt refNum)
{
	SampleLibGlobalsPtr	gP = NULL;
	VoidHand					gH;
	SysLibTblEntryPtr		libEntryP;


	libEntryP = SysLibTblEntry(refNum);						// get our system library table entry
	if ( libEntryP )
		gH = (VoidHand)(libEntryP->globalsP);				// get our globals handle from the entry
	if ( gH )
		gP = (SampleLibGlobalsPtr)MemHandleLock(gH);		// lock our globals


	return( gP );
}



/************************************************************
 *
 *  FUNCTION: PrvIsLibOpen
 *
 *  DESCRIPTION:	Check if the library has been opened.
 *
 *  PARAMETERS:	refNum		-- Sample library reference number
 *
 *  CALLED BY:		internal
 *
 *  RETURNS:		non-zero if the library has been opened
 *
 *  CREATED: 6/9/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
static Boolean PrvIsLibOpen(UInt refNum)
{
	SampleLibGlobalsPtr	gP;
	Boolean	isOpen = false;
	
	gP = PrvLockGlobals(refNum);
	
	if ( gP )
		{
		isOpen = true;
		PrvUnlockGlobals(gP);
		}
	
	return( isOpen );
}



/************************************************************
 *
 *  FUNCTION: PrvCreateClientContext
 *
 *  DESCRIPTION:	Create a client context for storing client-specific data.
 *	The client context allows the library to support multiple clients.
 *
 *  PARAMETERS:	
 *  gP						-- pointer to our locked globals
 *	clientContextP		-- pointer to variable for returning client context
 *
 *  CALLED BY:		Anyone wishing to create a client context
 *
 *  RETURNS:		0						-- no error
 *						sampleErrNotOpen	-- the library is not open
 *						sampleErrMemory	-- insufficient memory
 *
 *	*clientContextP will be set to client context on success, or zero on error.
 *
 *  CREATED: 6/9/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
static Err PrvCreateClientContext(SampleLibGlobalsPtr gP, DWordPtr clientContextP)
{
	Err								err = 0;
	VoidHand							contextH;
	SampleLibClientContextPtr	contextP;

	
	// Error-check our parameters
	ErrFatalDisplayIf(gP == NULL, "null globals pointer");
	ErrFatalDisplayIf(clientContextP == NULL, "null context variable pointer");
	
	
	// Initialize return variable
	*clientContextP = 0;
	

	// Allocate a new client context structure
	contextH = MemHandleNew(sizeof(SampleLibClientContextType));
	if ( !contextH )
		{
		err = sampleErrMemory;
		}
	else
		{
		*clientContextP = (DWord)contextH;			// save context chunk handle in return variable
		
		// Initialize the context chunk
		contextP = (SampleLibClientContextPtr)MemHandleLock(contextH);		// lock context chunk
		contextP->wSignature = sampleLibContextSignature;
		contextP->cornerDiam = sampleDefaultCornerDiameter;
		PrvUnlockContext(contextP);													// unlock the context

		gP->contextCount++;			// increment context count (for debugging)
		ErrFatalDisplayIf(gP->contextCount == 0, "context count overflow");
		}


	return( err );
}



/************************************************************
 *
 *  FUNCTION: PrvDestroyClientContext
 *
 *  DESCRIPTION:	Destroy a client context which was created by PrvCreateClientContext.
 *
 *  PARAMETERS:	
 *  gP						-- pointer to our locked globals
 *						clientContext		-- client context
 *
 *  CALLED BY:		Anyone wishing to create a client context
 *
 *  RETURNS:		0						-- no error
 *						sampleErrNotOpen	-- the library is not open
 *
 *  CREATED: 6/9/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
static Err PrvDestroyClientContext(SampleLibGlobalsPtr gP, DWord clientContext)
{
	SampleLibClientContextPtr	contextP;
	
	// Error-check our parameters
	ErrFatalDisplayIf(gP == NULL, "null globals pointer");

	// Validate the client context by locking it
	contextP = PrvLockContext(clientContext);
	
	if ( contextP )
		{
		MemPtrFree(contextP);		// freeing a locked chunk is permitted by the system
		gP->contextCount--;			// decrement context count (for debugging)
		ErrFatalDisplayIf(gP->contextCount < 0, "context count underflow");
		}
	
	return( 0 );
}



/************************************************************
 *
 *  FUNCTION: PrvLockContext
 *
 *  DESCRIPTION:	Validate and lock a client context.
 *
 *  PARAMETERS:	context		-- a client context to lock
 *
 *  CALLED BY:		internal
 *
 *  RETURNS:		pointer to the locked client context.
 *
 *  CREATED: 6/9/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
static SampleLibClientContextPtr PrvLockContext(DWord context)
{
	SampleLibClientContextPtr	contextP = NULL;
	
	
	// Error-check our parameters
	ErrFatalDisplayIf(context == 0, "null client context");
	
	// Lock the client context
	contextP = (SampleLibClientContextPtr)MemHandleLock((VoidHand)context);
	ErrFatalDisplayIf(contextP == NULL, "failed to lock client context");	// should not happen
	
	// Validate the client context
	ErrFatalDisplayIf(contextP->wSignature != sampleLibContextSignature, "invalid client context");
	
	
	return( contextP );
}

