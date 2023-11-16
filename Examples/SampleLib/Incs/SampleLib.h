/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SampleLib.h
 *
 * Description:
 *		Sample library API definitions.  The Sample library serves as an example
 *		of creating Palm OS shared libraries.
 *
 * History:
 *   	 5/20/97	vmk
 *   	12/18/97	scl Added BUILDING_SAMPLE_LIB to fix building in CW Pro 2
 *
 *****************************************************************************/

#ifndef __SAMPLE_LIB_H__
#define __SAMPLE_LIB_H__


// If we're actually compiling the library code, then we need to
// eliminate the trap glue that would otherwise be generated from
// this header file in order to prevent compiler errors in CW Pro 2.
#ifdef BUILDING_SAMPLE_LIB
	#define SAMPLE_LIB_TRAP(trapNum)
#else
	#define SAMPLE_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif


// Palm OS common definitions
#include <PalmTypes.h>
#include <SystemResources.h>


/********************************************************************
 * Type and creator of Sample Library database
 ********************************************************************/
#define		sampleLibCreatorID	'SL??'				// Sample Library database creator
#define		sampleLibTypeID		'libr'				// Standard library database type


/********************************************************************
 * Internal library name which can be passed to SysLibFind()
 ********************************************************************/
#define		sampleLibName			"Sample.lib"		


/************************************************************
 * Sample Library result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *************************************************************/

#define sampleErrParam			(appErrorClass | 1)		// invalid parameter
#define sampleErrNotOpen		(appErrorClass | 2)		// library is not open
#define sampleErrStillOpen		(appErrorClass | 3)		// returned from SampleLibClose() if
																		// the library is still open by others
#define sampleErrMemory			(appErrorClass | 4)		// memory error occurred


//-----------------------------------------------------------------------------
// Sample library function trap ID's. Each library call gets a trap number:
//   sampleLibTrapXXXX which serves as an index into the library's dispatch table.
//   The constant sysLibTrapCustom is the first available trap number after
//   the system predefined library traps Open,Close,Sleep & Wake.
//
// WARNING!!! The order of these traps MUST match the order of the dispatch
//  table in SampleLibDispatch.c!!!
//-----------------------------------------------------------------------------

typedef enum {
	sampleLibTrapGetLibAPIVersion = sysLibTrapCustom,
	sampleLibTrapSetCornerDiameter,
	sampleLibTrapGetCornerDiameter,
	sampleLibTrapDrawRectangle,
	
	sampleLibTrapLast
	} SampleLibTrapNumberEnum;



/********************************************************************
 * Public Structures
 ********************************************************************/
 



/********************************************************************
 * API Prototypes
 ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


//--------------------------------------------------
// Standard library open, close, sleep and wake functions
//--------------------------------------------------

extern Err SampleLibOpen(UInt16 refNum, UInt32 * clientContextP)
				SAMPLE_LIB_TRAP(sysLibTrapOpen);
				
extern Err SampleLibClose(UInt16 refNum, UInt32 clientContext)
				SAMPLE_LIB_TRAP(sysLibTrapClose);

extern Err SampleLibSleep(UInt16 refNum)
				SAMPLE_LIB_TRAP(sysLibTrapSleep);

extern Err SampleLibWake(UInt16 refNum)
				SAMPLE_LIB_TRAP(sysLibTrapWake);


//--------------------------------------------------
// Custom library API functions
//--------------------------------------------------
	
// Get our library API version
extern Err SampleLibGetLibAPIVersion(UInt16 refNum, UInt32 * dwVerP)
				SAMPLE_LIB_TRAP(sampleLibTrapGetLibAPIVersion);
	
// Set the corner diameter
extern Err SampleLibSetCornerDiameter(UInt16 refNum, UInt32 clientContext, Int16 cornerDiam)
				SAMPLE_LIB_TRAP(sampleLibTrapSetCornerDiameter);
				
// Get the corner diameter
extern Err SampleLibGetCornerDiameter(UInt16 refNum, UInt32 clientContext, Int16 * cornerDiamP)
				SAMPLE_LIB_TRAP(sampleLibTrapGetCornerDiameter);
				
// Draw a rectangle to the current draw window
extern Err SampleLibDrawRectangle(UInt16 refNum, UInt32 clientContext, Int16	x, Int16 y, Int16 width, Int16 height)
				SAMPLE_LIB_TRAP(sampleLibTrapDrawRectangle);



// For loading the library in Palm OS Mac emulation mode
extern Err SampleLibInstall(UInt16 refNum, SysLibTblEntryPtr entryP);


#ifdef __cplusplus 
}
#endif


/********************************************************************
 * Public Macros
 ********************************************************************/




#endif	// __SAMPLE_LIB_H__

