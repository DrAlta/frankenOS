/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SampleLib.c
 *
 * Description:
 *             	Sample library implementation.  The Sample library serves
 *		as an example of creating Palm OS shared libraries.
 *
 * History:
 *		5/20/97	Created by vmk
 *   	12/18/97	scl Added BUILDING_SAMPLE_LIB to fix building in CW Pro 2
 *
 *****************************************************************************/

#include <PalmOS.h>			// Includes all Palm OS headers

#include <Rect.h>				// for RectangleType
#include <Window.h>			// for WinDrawRectangle


// Our library public definitions (library API)
#define BUILDING_SAMPLE_LIB
#include "SampleLib.h"


// Our library private definitions (library globals, etc.)
#include "SampleLibPrv.h"



/********************************************************************
 * LIBRARY GLOBALS:
 *
 * IMPORTANT:
 * ==========
 * Libraries are *not* allowed to have global or static variables.  Instead,
 * they allocate a memory chunk to hold their persistent data, and save
 * a handle to it in the library's system library table entry.  Example
 *	functions below demostrate how the library "globals" chunk is set up, saved,
 * and accessed.
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
static SampleLibGlobalsPtr PrvMakeGlobals(UInt16 refNum);
static void PrvFreeGlobals(UInt16 refNum);
static SampleLibGlobalsPtr PrvLockGlobals(UInt16 refNum);
static Boolean PrvIsLibOpen(UInt16 refNum);
static Err PrvCreateClientContext(SampleLibGlobalsPtr gP, UInt32 * clientContextP);
static Err PrvDestroyClientContext(SampleLibGlobalsPtr gP, UInt32 clientContext);
static SampleLibClientContextPtr PrvLockContext(UInt32 context);


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

#define prvSampleLibVersion	sysMakeROMVersion(1, 0, 0, sysROMStageDevelopment, 1)




/********************************************************************
 * Sample Library API Routines
 ********************************************************************/
 
 
/************************************************************
 *
 *  FUNCTION: SampleLibOpen
 *
 *  DESCRIPTION:	Opens the Sample library, creates and initializes the globals.
 *						This function must be called before any other Sample Library functions,
 *						with the exception of SampleLibGetLibAPIVersion.
 *
 *						If SampleLibOpen fails, do not call any other Sample library API functions.
 *						If SampleLibOpen succeeds, call SampleLibClose when you are done using
 *						the library to enable it to release critical system resources.
 *
 *  LIBRARY DEVELOPER NOTES:
 *
 *						The library's "open" and "close" functions should *not* take an excessive
 *						amount of time to complete.  If the processing time for either of these
 *						is lengthy, consider creating additional library API function(s) to handle
 *						the time-consuming chores.
 *
 *
 *  PARAMETERS:	refNum				-- Sample library reference number returned by SysLibLoad()
 *										   	   or SysLibFind().
 *						clientContextP		-- pointer to variable for returning client context.  The
 *												   The client context is used to maintain client-specific
 *													data for multiple client support.  The
 *												   value returned here will be used as a parameter for
 *												   other Sample library functions which require a client
 *												   context.  
 *
 *  CALLED BY:		anyone who wants to use this library
 *
 *  RETURNS:		0							-- no error
 *						sampleErrMemory		-- not enough memory to initialize
 *
 *						*clientContextP will be set to client context on success, or zero on error.
 *
 *  CREATED:	5/20/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
Err SampleLibOpen(UInt16 refNum, UInt32 * clientContextP)
{
	SampleLibGlobalsPtr	gP;
	Err						err = 0;
	Int16						originalOpenCount = 0;


	// Error-check our parameters
	ErrFatalDisplayIf(clientContextP == NULL, "null context variable pointer");
	
	
	// Initialize return variable
	*clientContextP = 0;

	// Get library globals
	gP = PrvLockGlobals(refNum);										// lock our library globals
	
	// Check if already open
	if (!gP)
		{
		// Allocate and initialize our library globals.
		gP = PrvMakeGlobals(refNum);									// returns locked globals on success
		if ( !gP )
			err = sampleErrMemory;
		}
	
	// If we have globals, create a client context, increment open count, and unlock our globals
	if ( gP )
		{
		originalOpenCount = gP->openCount;							// save original open count for error handling
		
		err = PrvCreateClientContext(gP, clientContextP);		// create a context for client-specific data
		if ( !err )
			gP->openCount++;												// increment open count on success
		
		PrvUnlockGlobals(gP);											// unlock our library globals
		
		// If there was an error creating a client context and there are no other clients,
		// free our globals
		if ( err && (originalOpenCount == 0) )
			PrvFreeGlobals(refNum);
		}

	
	return( err );
}



/************************************************************
 *
 *  FUNCTION: SampleLibClose
 *
 *  DESCRIPTION:	Closes the Sample libary, frees client context and globals.
 *
 *						***IMPORTANT***
 *						May be called only if SampleLibOpen succeeded.
 *
 *						If other applications still have the library open, decrements
 *						the reference count and returns sampleErrStillOpen.
 *
 *
 *  LIBRARY DEVELOPER NOTES:
 *
 *						The library's "open" and "close" functions should *not* take an excessive
 *						amount of time to complete.  If the processing time for either of these
 *						is lengthy, consider creating additional library API function(s) to handle
 *						the time-consuming chores.
 *							
 *
 *  PARAMETERS:	refNum				-- Sample library reference number returned by SysLibLoad()
 *												   or SysLibFind().
 *						clientContext		-- client context
 *
 *  CALLED BY:		Whoever wants to close the Sample library
 *
 *  RETURNS:		0							-- no error
 *						sampleErrStillOpen	-- library is still open by others (no error)
 *
 *  CREATED:	5/20/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
Err SampleLibClose(UInt16 refNum, UInt32 clientContext)
{
	SampleLibGlobalsPtr	gP;
	Int16						openCount;
	Int16						contextCount;
	Err						err = 0;


	gP = PrvLockGlobals(refNum);	// lock our globals
	
	// If not open, return
	if (!gP)
		{
		return 0;						// MUST return zero here to get around a bug in system v1.x that
											// would cause SysLibRemove to fail.
		}

	// Destroy the client context (we ignore the return code in this implementation)
	PrvDestroyClientContext(gP, clientContext);
	
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



/************************************************************
 *
 *  FUNCTION: SampleLibSleep
 *
 *  DESCRIPTION:	Handles system sleep notification.
 *
 *						***IMPORTANT***
 *						This notification function is called from a system interrupt.
 *						It is only allowed to use system services which are interrupt-
 *						safe.  Presently, this is limited to EvtEnqueueKey, SysDisableInts,
 *						SysRestoreStatus.  Because it is called from an interrupt,
 *						it must *not* take a long time to complete to preserve system
 *						integrity.  The intention is to allow system-level libraries
 *						to disable hardware components to conserve power while the system
 *						is asleep.
 *
 *  PARAMETERS:	refNum		-- Sample library reference number
 *
 *  CALLED BY:		System
 *
 *  RETURNS:		0						-- no error
 *
 *  CREATED:	5/20/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
Err SampleLibSleep(UInt16 refNum)
{
	return( 0 );
}



/************************************************************
 *
 *  FUNCTION: SampleLibWake
 *
 *  DESCRIPTION:	Handles system wake notification
 *
 *						***IMPORTANT***
 *						This notification function is called from a system interrupt.
 *						It is only allowed to use system services which are interrupt-
 *						safe.  Presently, this is limited to EvtEnqueueKey, SysDisableInts,
 *						SysRestoreStatus.  Because it is called from an interrupt,
 *						it must *not* take a long time to complete to preserve system
 *						integrity.  The intention is to allow system-level libraries
 *						to enable hardware components which were disabled when the system
 *						went to sleep.
 *
 *  PARAMETERS:	refNum		-- Sample library reference number
 *
 *  CALLED BY:	System
 *
 *  RETURNS:	0						-- no error
 *
 *  CREATED:	5/20/97 
 *
 *  BY: vmk
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
Err SampleLibWake(UInt16 refNum)
{
	return( 0 );
}



/************************************************************
 *
 *  FUNCTION: SampleLibGetLibAPIVersion
 *
 *  DESCRIPTION:	Get our library API version.  The Sample library does not
 *						need to be "opened" to call SampleLibGetLibAPIVersion.
 *
 *  PARAMETERS:	refNum		-- Sample library reference number
 *						dwVerP		-- pointer to variable for storing the version number
 *
 *  CALLED BY:		Anyone wishing to get our library API version
 *
 *  RETURNS:		0						-- no error
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
Err SampleLibGetLibAPIVersion(UInt16 refNum, UInt32 * dwVerP)
{
	
	// Error-check our parameters
	ErrFatalDisplayIf(dwVerP == NULL, "null pointer argument");
	
	
	*dwVerP = prvSampleLibVersion;
	
	return( 0 );
}



/************************************************************
 *
 *  FUNCTION: SampleLibSetCornerDiameter
 *
 *  DESCRIPTION:	Set the corner diameter
 *
 *  PARAMETERS:	refNum				-- Sample library reference number
 *						clientContext		-- client context
 *						cornerDiam			-- corner diameter
 *
 *  CALLED BY:		Anyone wishing to set the rectangle corner diameter
 *
 *  RETURNS:		0						-- no error
 *						sampleErrParam		-- invalid corner diameter value
 *						sampleErrNotOpen	-- the library is not open
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
Err SampleLibSetCornerDiameter(UInt16 refNum, UInt32 clientContext, Int16 cornerDiam)
{
	SampleLibClientContextPtr	contextP;
		
		
	// Validate our parameters
	if ( cornerDiam < 0 )
		return sampleErrParam;
	
	
	// Make sure the library has been opened
	if ( !PrvIsLibOpen(refNum) )
		return( sampleErrNotOpen );

	
	// Lock the client context
	contextP = PrvLockContext(clientContext);
	
	
	// Save the new corner diameter in our library globals
	contextP->cornerDiam = cornerDiam;
	
	
	// Unlock the client context
	PrvUnlockContext(contextP);
	

	return( 0 );
}



/************************************************************
 *
 *  FUNCTION: SampleLibGetCornerDiameter
 *
 *  DESCRIPTION:	Get the corner diameter
 *
 *  PARAMETERS:	refNum				-- Sample library reference number
 *						clientContext		-- client context
 *						cornerDiamP			-- pointer to variable for storing corner diameter
 *
 *  CALLED BY:		Anyone wishing to get the rectangle corner diameter
 *
 *  RETURNS:		0						-- no error
 *						sampleErrNotOpen	-- the library is not open
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
Err SampleLibGetCornerDiameter(UInt16 refNum, UInt32 clientContext, Int16 * cornerDiamP)
{
	SampleLibClientContextPtr	contextP;
	
	
	// Error-check our parameters
	ErrFatalDisplayIf(cornerDiamP == NULL, "null pointer argument"); 


	// Make sure the library has been opened
	if ( !PrvIsLibOpen(refNum) )
		return( sampleErrNotOpen );

	
	// Lock the client context
	contextP = PrvLockContext(clientContext);
	
	
	// Get the corner diameter from the client context
	*cornerDiamP = contextP->cornerDiam;
	
	
	// Unlock the client context
	PrvUnlockContext(contextP);
	

	return( 0 );
}



/************************************************************
 *
 *  FUNCTION: SampleLibDrawRectangle
 *
 *  DESCRIPTION:	Draw a rectangle to the current draw window using the current
 *						corner diameter.
 *
 *  PARAMETERS:	refNum				-- Sample library reference number
 *						clientContext		-- client context
 *						x						-- x coordinate of the rectangle origin
 *						y						-- y coordinate of the rectangle origin
 *						width					-- width of the rectangle
 *						height				-- height of the rectangle
 *
 *  CALLED BY:		Anyone wishing to draw a rectangle
 *
 *  RETURNS:		0						-- no error
 *						sampleErrParam		-- invalid parameter(s)
 *						sampleErrNotOpen	-- the library is not open
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
Err SampleLibDrawRectangle(UInt16 refNum, UInt32 clientContext, Int16	x, Int16 y, Int16 width, Int16 height)
{
	Err						err = 0;
	Int16						cornerDiam;			// corner diameter
	RectangleType			r;						// rectangle
	
	
	// Validate the parameters
	if ( width < 0 || height < 0 )
		return sampleErrParam;

	
	// Get the current corner diameter
	err = SampleLibGetCornerDiameter(refNum, clientContext, &cornerDiam);
	if ( err )
		return( err );
	
	
	// Draw the rectanglge
	r.topLeft.x = x;
	r.topLeft.y = y;
	r.extent.x = width;
	r.extent.y = height;
	WinDrawRectangle(&r, (UInt16)(cornerDiam));
	

	return( 0 );
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
static SampleLibGlobalsPtr PrvMakeGlobals(UInt16 refNum)
{
	SampleLibGlobalsPtr	gP = NULL;							// our globals pointer
	MemHandle				gH;									// our globals handle
	SysLibTblEntryPtr		libEntryP;							// pointer to our library table entry


	// Get library globals
	libEntryP = SysLibTblEntry(refNum);						// get our system library table entry
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
static void PrvFreeGlobals(UInt16 refNum)
{
	MemHandle					gH;									// our globals handle
	SysLibTblEntryPtr		libEntryP;							// pointer to our library table entry


	// Get our library globals handle
	libEntryP = SysLibTblEntry(refNum);						// get our system library table entry
	ErrFatalDisplayIf(libEntryP == NULL, "invalid Sample lib refNum");
	
	gH = (MemHandle)(libEntryP->globalsP);					// get our globals handle from the entry
	
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
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *
 *************************************************************/
static SampleLibGlobalsPtr PrvLockGlobals(UInt16 refNum)
{
	SampleLibGlobalsPtr	gP = NULL;
	MemHandle				gH;
	SysLibTblEntryPtr		libEntryP;


	libEntryP = SysLibTblEntry(refNum);						// get our system library table entry
	if ( libEntryP )
		gH = (MemHandle)(libEntryP->globalsP);				// get our globals handle from the entry
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
static Boolean PrvIsLibOpen(UInt16 refNum)
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
 *						The client context allows the library to support multiple clients.
 *
 *  PARAMETERS:	gP						-- pointer to our locked globals
 *						clientContextP		-- pointer to variable for returning client context
 *
 *  CALLED BY:		Anyone wishing to create a client context
 *
 *  RETURNS:		0						-- no error
 *						sampleErrNotOpen	-- the library is not open
 *						sampleErrMemory	-- insufficient memory
 *
 *						*clientContextP will be set to client context on success, or zero on error.
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
static Err PrvCreateClientContext(SampleLibGlobalsPtr gP, UInt32 * clientContextP)
{
	Err								err = 0;
	MemHandle						contextH;
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
		*clientContextP = (UInt32)contextH;			// save context chunk handle in return variable
		
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
 *  PARAMETERS:	gP						-- pointer to our locked globals
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
static Err PrvDestroyClientContext(SampleLibGlobalsPtr gP, UInt32 clientContext)
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
static SampleLibClientContextPtr PrvLockContext(UInt32 context)
{
	SampleLibClientContextPtr	contextP = NULL;
	
	
	// Error-check our parameters
	ErrFatalDisplayIf(context == 0, "null client context");
	
	// Lock the client context
	contextP = (SampleLibClientContextPtr)MemHandleLock((MemHandle)context);
	ErrFatalDisplayIf(contextP == NULL, "failed to lock client context");	// should not happen
	
	// Validate the client context
	ErrFatalDisplayIf(contextP->wSignature != sampleLibContextSignature, "invalid client context");
	
	
	return( contextP );
}

