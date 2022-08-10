#ifndef POCKETC_LIB_H
#define POCKETC_LIB_H

#include <Common.h>
#include <SystemMgr.rh>

enum VarType {
   vtInt=0, vtChar, vtFloat, vtString, vtVoid
};

struct Value {
   VarType type;
   union {
      long iVal;
      float fVal;
      char cVal;
      Handle sVal;
   };
};

struct PocketCLibGlobalsType {
	void (* push)(Value&);
	void (* pop)(Value&);
	void (* cleanup)(Value&);
	void (* typeCast)(Value&, VarType);
	void (* typeMatch)(Value&, Value&);
	bool (* UIYield)(bool);
	int  (* addLibFunc)(char* name, int nArgs, VarType arg1 = vtInt, VarType arg2 = vtInt, VarType arg3 = vtInt, VarType arg4 = vtInt, VarType arg5 = vtInt, VarType arg6 = vtInt, VarType arg7 = vtInt, VarType arg8 = vtInt, VarType arg9 = vtInt, VarType arg10 = vtInt);
	void (* callFunc)(int loc);
	Value* retVal;
	Value* (* deref)(int ptr);
	bool (* callBI)(char* name);
	ULong reserved[6];
	
	//
	// TODO: Add your global variables here
	//
	
};
typedef PocketCLibGlobalsType* PocketCLibGlobalsPtr;

#ifdef __cplusplus
extern "C" {
#endif


/********************************************************************
 * Standard library open, close, sleep and wake functions
 ********************************************************************/

extern PocketCLibGlobalsPtr PocketCLibOpen(UInt refNum, DWordPtr);
extern Err PocketCLibClose(UInt refNum, DWord);
extern Err PocketCLibSleep(UInt refNum);
extern Err PocketCLibWake(UInt refNum);

/********************************************************************
 * Custom library API functions
 ********************************************************************/
	
// Add the PocketC library function information
extern Err PocketCLibAddFunctions(UInt refNum);

// Execute a PocketC function
extern Err PocketCLibExecuteFunction(UInt refNum, int funcNum);

// For loading the library in PalmPilot Mac emulation mode
extern Err PocketCLibInstall(UInt refNum, SysLibTblEntryPtr entryP);

#ifdef __cplusplus 
}
#endif


#endif
