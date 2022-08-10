/****************************************************************************
 * File        : hardware.c
 * Date        : 10-15-98
 * Description : 
 ****************************************************************************/
#define PILOT_PRECOMPILED_HEADERS_OFF

#include <Pilot.h>

#include "hdw.h"

/*---------------------------------------------------------------------------
 *  Function    : HdwDisableInts
 *  Date        : 10/8/99
 *  Description : Disable interrupts at SR, saving the old SR value
 *  Params      : none
 *  Returns     : Previous SR value
 *---------------------------------------------------------------------------*/
asm UInt16 HdwDisableInts(void)
{
    move.w sr, d0
    ori.w  #0x0700, sr
    rts
}

/*---------------------------------------------------------------------------
 *  Function    : HdwRestoreInts
 *  Date        : 10/8/99
 *  Description : Restore SR from previously saved value
 *  Params      : saved_ints -- SR value to restore
 *  Returns     : nothing
 *---------------------------------------------------------------------------*/
asm void HdwRestoreInts(UInt16 saved_ints)
{
    move.w 4(a7), sr
    rts
}    

