/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SampleLibPrv.h
 *
 * Description:
 *		Sample library private definitions.
 *
 * History:
 *   	5/20/97	vmk
 *
 *****************************************************************************/

#ifndef __SAMPLE_LIB_PRV_H__
#define __SAMPLE_LIB_PRV_H__

// Palm OS common definitions
#include <PalmTypes.h>
#include <SampleLib.h>





/********************************************************************
 * Private Structures
 ********************************************************************/
 
// Library globals
typedef struct SampleLibGlobalsType {
	UInt16					thisLibRefNum;			// our library reference number (for convenience and debugging)
	Int16						openCount;				// library open count
	
	// Add other library global fields here
	Int16						contextCount;			// number of context in existence (for debugging)
	
	} SampleLibGlobalsType;

typedef SampleLibGlobalsType*		SampleLibGlobalsPtr;


// Client context structure for storing each client-specific data
typedef struct SampleLibClientContextType {
	UInt16					wSignature;				// signature for validating the context
	
	Int16						cornerDiam;				// rectangle corner diameter
	} SampleLibClientContextType;

typedef SampleLibClientContextType*		SampleLibClientContextPtr;


// The wSignature field of each SampleLibClientContextType will be set to
// this value for debugging
#define sampleLibContextSignature		(0xFEED)


/********************************************************************
 * Private Macros
 ********************************************************************/

#define sampleDefaultCornerDiameter		(0)


#endif	// __SAMPLE_LIB_PRV_H__

