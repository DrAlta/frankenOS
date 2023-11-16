/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SampleLibDispatch.c
 *
 * Description:
 *  Main entry point and Dispatch table for the Sample Library. This
 *  module gets linked in with the Library source modules. It provides
 *  library dispatch table and the routine for initializing this table
 *  when the library is installed.
 *
 * History:
 *   	5/19/97  vmk - Created by vmk
 *
 *****************************************************************************/

// Define this so we don't use the Palm OS pre-compiled headers.
// We can't use them here because we re-define some build options
#ifndef PILOT_PRECOMPILED_HEADERS_OFF
#define	PILOT_PRECOMPILED_HEADERS_OFF
#endif


// Define EMULATION_LEVEL.  __PALMOS_TRAPS__ was pre-defined by MW C Compiler
// Test it before we blow it away so we can properly define EMULATION_LEVEL.
#if __PALMOS_TRAPS__
		#define EMULATION_LEVEL		EMULATION_NONE		// building Pilot executable
#endif

// Now clear __PALMOS_TRAPS__ and define USE_TRAPS so headers do the right thing
// That is, we need to be able to get the addresses of SampleLibrary routines.
// Other modules will access the routines through traps.
#undef __PALMOS_TRAPS__
#define __PALMOS_TRAPS__ 	0
#define	USE_TRAPS 	0


// Include Pilot headers
#include <PalmOS.h>


// Our library public definitions (library API)
#include "SampleLib.h"


// Our library private definitions (library globals, etc.)
#include "SampleLibPrv.h"



// Private routines

#if EMULATION_LEVEL == EMULATION_NONE
	#define SampleLibInstall		__Startup__
#endif


// Local prototypes

Err SampleLibInstall(UInt16 refNum, SysLibTblEntryPtr entryP);
static MemPtr	asm SampleLibDispatchTable(void);



/***********************************************************************
 *
 *  FUNCTION:     SampleLibInstall
 *
 *  DESCRIPTION:  Entry point for Library installation resource. This
 *						is a 'libr' resource in the "Sample Library" resource database.
 *						This resource contains the Library API entry points. 
 *						
 *						Beginning with PilotOS v2.0, to install the library, the application
 *						or another library calls SysLibLoad().  The library's "open" and "close"
 *						functions should be called to initialize and de-initialize the library.
 *						SysLibRemove() is called to uninstall the library.
 *					
 *						(On PilotOS v1.x, the application
 *						needs to open the libarary database, lock down this library code resource,
 *						close the database without unlocking the resource, and pass its pointer to
 *						SysLibInstall().  SysLibInstall() will jump to this resource to have it
 *						install it's dispatch table into the library entry. The library's "open"
 *						and "close" functions should be called to initialize and de-initialize the
 *						library.  When the application is done with the library, it needs to call
 *						SysLibRemove, and then unlock the library code resource.  On v1.x,
 *						SysLibRemove calls the library's "close" function; for this reason, the
 *						library's close function should be able to deal with this "extra" close call.)
 *
 *  PARAMETERS:   
 *					refNum - refNum of library
 *					entryP - pointer to entry in library table.
 *
 *  RETURNS:   	0 if no error   
 *
 *  CALLED BY:    SysLibInstall()
 *
 *  CREATED:      5/19/97
 *
 *  BY:           vmk
 *
 ***********************************************************************/
Err SampleLibInstall(UInt16 refNum, SysLibTblEntryPtr entryP)
{
	// Install pointer to our dispatch table
	entryP->dispatchTblP = (MemPtr*)SampleLibDispatchTable();
	
	// Initialize globals pointer to zero (we will set up our library
	// globals in the library "open" call).
	entryP->globalsP = 0;

	return 0;
	
}


/************************************************************
 *
 *  FUNCTION: Dispatch Table for Sample Library
 *
 *  DESCRIPTION: This table gets installed into the dispatchTblP
 *		field of the library entry in the library table. It gives
 *		the 16-bit offset of every routine relative to the start of
 *		the dispatch table.
 *
 *  WARNING!!! This table must match the ordering of the library's
 *		trap numbers!!!!!!!!!!
 *
 *  CREATED: 5/19/97
 *
 *  BY: vmk
 *
 *************************************************************/

// First, define the size of the jump instruction
#if EMULATION_LEVEL == EMULATION_NONE
	#define prvJmpSize		4					// Palm OS presently uses short jumps
#elif EMULATION_LEVEL == EMULATION_MAC
	#define prvJmpSize		6					// Our Mac builds use long jumps
#else
	#error unsupported emulation mode
#endif	// EMULATION_LEVEL...

// Now, define a macro for offset list entries
#define libDispatchEntry(index)		(kOffset+((index)*prvJmpSize))


// Finally, define the size of the dispatch table's offset list --
// it is equal to the size of each entry (which is presently 2 bytes) times
// the number of entries in the offset list (***including the @Name entry***).
//
#define	kOffset		(2*9)						// NOTE: This is empirical!!!!!!
														// Will change when table changes!!

static MemPtr	asm SampleLibDispatchTable(void)
{
	LEA		@Table, A0								// table ptr
	RTS													// exit with it

@Table:
	// Offset to library name
	DC.W		@Name
	
	//
	// Library function dispatch entries
	//
	// ***IMPORTANT***
	// The index parameters passed to the macro libDispatchEntry
	// must be numbered consecutively, beginning with zero.
	//
	// The hard-wired values need to be used for offsets because the MW SDK tools do
	// not support label subtraction.
	//
	
	// Standard traps
	DC.W		libDispatchEntry(0)					// Open
	DC.W		libDispatchEntry(1)					// Close
	DC.W		libDispatchEntry(2)					// Sleep
	DC.W		libDispatchEntry(3)					// Wake
	
	// Start of the Custom traps
	DC.W		libDispatchEntry(4)					// GetLibAPIVersion
	DC.W		libDispatchEntry(5)					// SetCornerDiameter
	DC.W		libDispatchEntry(6)					// GetCornerDiameter
	DC.W		libDispatchEntry(7)					// DrawRectangle

// Standard library function handlers
@GotoOpen:
	JMP 		SampleLibOpen
@GotoClose:
	JMP 		SampleLibClose
@GotoSleep:
	JMP 		SampleLibSleep
@GotoWake:
	JMP 		SampleLibWake
	
// Customer library function handlers
@GotoGetLibAPIVersion:
	JMP 		SampleLibGetLibAPIVersion
@GotoSetCornerDiameter:
	JMP 		SampleLibSetCornerDiameter
@GotoGetCornerDiameter:
	JMP 		SampleLibGetCornerDiameter
@GotoDrawRectangle:
	JMP		SampleLibDrawRectangle

	
@Name:
	DC.B		sampleLibName							// This name identifies the library.  Apps
															// can pass it to SysLibFind() to check if the
															// library is already installed
	
}
