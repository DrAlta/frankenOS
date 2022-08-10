/*******************************************************************
 * 							 PalmPilot Software
 *
 *	Copyright (c) 1996-1997, USRobotics/Palm Computing., All Rights Reserved
 *
 *-------------------------------------------------------------------
 * FileName:
 *		SampleLibPrv.h
 *
 * Description:
 *		Sample library private definitions.
 *
 * History:
 *   	5/20/97	vmk
 *
 *******************************************************************/


#ifndef __SAMPLE_LIB_PRV_H__
#define __SAMPLE_LIB_PRV_H__

// PalmPilot common definitions
#include <Common.h>
#include <NdkLib.h>

/********************************************************************
 * Private Structures
 ********************************************************************/
 
// Library globals
typedef struct SampleLibGlobalsType {
	UInt	thisLibRefNum;			// our library reference number (for convenience and debugging)
	Int		openCount;				// library open count
	
	// Add other library global fields here
	Int		contextCount;			// number of context in existence (for debugging)
	
	} SampleLibGlobalsType;

typedef SampleLibGlobalsType*		SampleLibGlobalsPtr;


// Client context structure for storing each client-specific data
typedef struct SampleLibClientContextType {
	Word	wSignature;				// signature for validating the context
	
	Int		cornerDiam;				// rectangle corner diameter
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
