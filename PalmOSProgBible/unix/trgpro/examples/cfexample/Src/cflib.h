/****************************************************************************
 *
 *      Copyright (c) 1999, TRG, All Rights Reserved
 *
 *---------------------------------------------------------------------------
 * FileName:
 *              CFLib.h
 *
 * Description:
 *              CF library API definitions.
 *
 *
 ****************************************************************************/


#ifndef __CF_LIB_H__
#define __CF_LIB_H__


/*---------------------------------------------------------------------------
 * If we're actually compiling the library code, then we need to
 * eliminate the trap glue that would otherwise be generated from
 * this header file in order to prevent compiler errors in CW Pro 2.
 *--------------------------------------------------------------------------*/
#ifdef BUILDING_CF_LIB
        #define CF_LIB_TRAP(trapNum)
#else
        #define CF_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif


/****************************************************************************
 * Type and creator of Sample Library database -- must match project defs!
 ****************************************************************************/
#define CfLibCreatorID  'CfL '       // Ffs Library database creator
#define CfLibTypeID     'libr'       // Standard library database type


/***************************************************************************
 * Internal library name which can be passed to SysLibFind()
 ***************************************************************************/
#define CfLibName       "CF.lib"     


/***************************************************************************
 * Defines for Ffs library calls
 ***************************************************************************/

/*--------------------------------------------------------------------------
 * CF Library result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *
 *-------------------------------------------------------------------------*/
#define CfErrorClass               (appErrorClass | 0x500)

#define CF_ERR_BAD_PARAM           (CfErrorClass | 1)  // invalid parameter
#define CF_ERR_LIB_NOT_OPEN        (CfErrorClass | 2)  // library is not open
#define CF_ERR_LIB_IN_USE          (CfErrorClass | 3)  // library still in used
#define CF_ERR_NO_MEMORY           (CfErrorClass | 4)  // memory error occurred
#define CF_ERR_CARD_DISABLED       (CfErrorClass | 5)  // card data is not enabled

/*---------------------------------------------------------------------------
 * CF power type, used by CfPowerSlot
 *--------------------------------------------------------------------------*/
typedef enum {CF_POWER_ON, CF_POWER_OFF} cf_pwr_type;

/*--------------------------------------------------------------------------
 * Card data bus with (8 bit or 16 bit), used by CfGetDataWidth,
 * CfSetDataWidth
 *--------------------------------------------------------------------------*/
typedef enum {CF_WIDTH_NONE, CF_8_BIT, CF_16_BIT} data_width_type;


/***************************************************************************
 * CF library function trap ID's. Each library call gets a trap number:
 *   CFLibTrapXXXX which serves as an index into the library's dispatch
 *   table. The constant sysLibTrapCustom is the first available trap number
 *   after the system predefined library traps Open,Close,Sleep & Wake.
 *
 * WARNING!!! The order of these traps MUST match the order of the dispatch
 *  table in CFLibDispatch.c!!!
 ****************************************************************************/
typedef enum {
    CfLibTrapGetLibAPIVersion = sysLibTrapCustom,
    CfLibTrapPowerSlot,
    CfLibTrapCardInserted,
    CfLibTrapReset,
    CfLibTrapBaseAddress,
    CfLibTrapGetDataWidth,
    CfLibTrapSetDataWidth,
    CfLibTrapSlotPowered,
    CfLibTrapSetDebuggingOff,
    CfLibTrapSetDebuggingOn,
    CfLibTrapGetIRQLevel,
    CfLibTrapEnableIRQLine,
    CfLibTrapDisableIRQLine,
    CfLibTrapSetSwapping,
    CfLibTrapLast
} CfLibTrapNumberEnum;


/********************************************************************
 *              CF Library API Prototypes
 ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * Standard library open, close, sleep and wake functions
 *-------------------------------------------------------------------------*/

/* open the library */
extern Err CfLibOpen(UInt16 libRef)
                                CF_LIB_TRAP(sysLibTrapOpen);
				
/* close the library */
extern Err CfLibClose(UInt16 libRef)
                                CF_LIB_TRAP(sysLibTrapClose);

/* library sleep */
extern Err CfLibSleep(UInt16 libRef)
                                CF_LIB_TRAP(sysLibTrapSleep);

/* library wakeup */
extern Err CfLibWake(UInt16 libRef)
                                CF_LIB_TRAP(sysLibTrapWake);

/*--------------------------------------------------------------------------
 * Custom library API functions
 *--------------------------------------------------------------------------*/

/* Get our library API version */
extern Err CfGetLibAPIVersion(UInt16 libRef, UInt32 *dwVerP)
                                CF_LIB_TRAP(CfLibTrapGetLibAPIVersion);
	
/* apply/remove power to the CF slot */
extern Err CfPowerSlot(UInt16 libRef, UInt8 slot_num, cf_pwr_type on_off,
                                      data_width_type width)
                                CF_LIB_TRAP(CfLibTrapPowerSlot);

/* check for card inserted */
extern Err CfCardInserted(UInt16 libRef, UInt8 slot_num, Boolean *inserted)
                                CF_LIB_TRAP(CfLibTrapCardInserted);

/* hardware reset the card */
extern Err CfReset(UInt16 libRef, UInt8 slot_num)
                                CF_LIB_TRAP(CfLibTrapReset);

/* get the base address of the card */
extern Err CfBaseAddress(UInt16 libRef, UInt8 slot_num, UInt32 *attribute,
                                        UInt32 *common, UInt32 *io)
                                CF_LIB_TRAP(CfLibTrapBaseAddress);

/* get the current data bus width */
extern Err CfGetDataWidth(UInt16 libRef, UInt8 slot_num, data_width_type *data_width)
                                CF_LIB_TRAP(CfLibTrapGetDataWidth);

/* set the data bus width */
extern Err CfSetDataWidth(UInt16 libRef, UInt8 slot_num, data_width_type data_width)
                                CF_LIB_TRAP(CfLibTrapSetDataWidth);

/* check if the card is powered */
extern Err CfSlotPowered(UInt16 libRef, UInt8 slot_num, Boolean *powered,
                                 data_width_type *data_width,
                                 Boolean *card_in_use)
                                CF_LIB_TRAP(CfLibTrapSlotPowered);

/* turn off serial debugging */
extern void CfSetDebuggingOff(UInt16 libRef)
                                CF_LIB_TRAP(CfLibTrapSetDebuggingOff);

/* turn on serial debugging */
extern void CfSetDebuggingOn(UInt16 libRef, UInt16 ser_port_id)
                                CF_LIB_TRAP(CfLibTrapSetDebuggingOn);

/* get the IRQ priority level for the slot */
extern Err CfGetIRQLevel(UInt16 libRef, UInt8 slot_num, UInt8 *level)
                                CF_LIB_TRAP(CfLibTrapGetIRQLevel);

/* enable the hardware IRQ line attached to the slot */
extern Err CfEnableIRQLine(UInt16 libRef, UInt8 slot_num)
                                CF_LIB_TRAP(CfLibTrapEnableIRQLine);

/* disable the hardware IRQ line attached to the slot */
extern Err CfDisableIRQLine(UInt16 libRef, UInt8 slot_num)
                                CF_LIB_TRAP(CfLibTrapDisableIRQLine);

/* turn on/off byte swapping */
extern Err CfSetSwapping(UInt16 libRef, UInt8 slot_num, Boolean swap_bytes)
                                CF_LIB_TRAP(CfLibTrapSetSwapping);

/*---------------------------------------------------------------------------
 * For loading the library in PalmPilot Mac emulation mode
 *--------------------------------------------------------------------------*/

extern Err CfLibInstall(UInt16 libRef, SysLibTblEntryPtr entryP);


#ifdef __cplusplus 
}
#endif


#endif  // __CF_LIB_H__
