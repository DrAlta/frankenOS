/***********************************************************************
 *
 * Copyright (c) 1999, TRG, All Rights Reserved
 *
 * PROJECT:         TRG palm common files
 *
 * FILE:            oldcom.c
 *
 * DESCRIPTION:     Functions to print out com port using the old serial mgr
 *
 * AUTHOR:          
 *
 * DATE:            
 *
 **********************************************************************/
#ifndef PILOT_PRECOMPILED_HEADERS_OFF
#define PILOT_PRECOMPILED_HEADERS_OFF
#endif

#include <Pilot.h>
#include <SerialMgr.h>

#include "trglib.h"
#include "com.h"

static UInt16   ser_lib_ref;
static Boolean  port_open = false;


/*---------------------------------------------------------------------------
 *  Function    : COMOpen()
 *  Date        : 10/8/99
 *  Description : Open the cradle serial port, and set to 57600, N-8-1, no
 *                handshaking.
 *  Params      : none
 *  Returns     : 0 on success, otherwise error code
 *---------------------------------------------------------------------------*/
Err COMOpen(void)
{
    Err retval;
    SerSettingsType settings;

    if (port_open)
        return(serErrAlreadyOpen);
    
    if ((retval = SysLibFind("Serial Library", &ser_lib_ref)) != 0)
        return(retval);
    
    if ((retval = SerOpen(ser_lib_ref, 0, 57600)) != 0)
        return(retval);
    
    settings.baudRate   = 57600;
    
    settings.flags      = serSettingsFlagStopBits1 |
                          serSettingsFlagBitsPerChar8;
                          
    if((retval = SerSetSettings(ser_lib_ref, &settings)) != 0)
        return(retval);

    port_open = true;

    return(0);
}

/*---------------------------------------------------------------------------
 *  Function    : COMClose()
 *  Date        : 10/8/99
 *  Description : Close previously opened cradle serial port.
 *  Params      : none
 *  Returns     : 0 on success, otherwise error code
 *---------------------------------------------------------------------------*/
Err COMClose(void)
{
    if (port_open)
    {
        port_open = false;
        return(SerClose(ser_lib_ref));
    }
    return(0);    
}

/*---------------------------------------------------------------------------
 *  Function    : COMSend()
 *  Date        : 9-14-95
 *  Description : Send specified number of bytes out the cradle serial port.
 *  Params      : length -- number bytes to send
 *                buffer -- pointer to buffer containing data
 *  Returns     : 0 on success, otherwise error code
 *---------------------------------------------------------------------------*/
static Err COMSend(UInt16 length, void *buffer)
{
    Err   err;

    if (port_open)
    {
        if ((err = SerSend10(ser_lib_ref, buffer, length)) != 0)
            return(err);

        if ((err = SerSendWait(ser_lib_ref, -1)) != 0)
            return(err);
    }
    
    return(0);
}

/*---------------------------------------------------------------------------
 *  Function    : COMPutC
 *  Date        : 10/8/99
 *  Description : Write a character out the cradle serial port.
 *  Params      : c -- character to write
 *  Returns     : 0 on success, otherwise error code
 *---------------------------------------------------------------------------*/
Err COMPutC(char c)
{
    Err   err;

    if(port_open)
    {
        if ((err = SerSend10(ser_lib_ref, &c, 1)) != 0)
            return(err);

        if ((err = SerSendWait(ser_lib_ref, -1)) != 0)
            return(err);
    }
    return(0);
}    

/*---------------------------------------------------------------------------
 *  Function    : COMGetC
 *  Date        : 10/8/99
 *  Description : Get a character from the cradle serial port.
 *  Params      : buffer -- pointer to character for return of value
 *  Returns     : 0 on success, otherwise error code
 *---------------------------------------------------------------------------*/
Err COMGetC(void *buffer)
{
    Err    err;

    if (port_open)
        if ((err = SerReceive10(ser_lib_ref, buffer, 1,10)) != 0)
            return(err);
    
    return(0);
}

/*---------------------------------------------------------------------------
 *  Function    : COMBytesAvailable
 *  Date        : 10/8/99
 *  Description : See if characters are available in the cradle serial port
 *                receive buffer.
 *  Params      : none
 *  Returns     : number of characters available.
 *---------------------------------------------------------------------------*/
UInt32 COMBytesAvailable(void)
{
    UInt32 value;

    SerClearErr(ser_lib_ref);
    SerReceiveCheck(ser_lib_ref, &value);

    return(value);
}

/*---------------------------------------------------------------------------
 *  Function    : COMPrintf
 *  Date        : 10/8/99
 *  Description : Print a formatted string out the serial port.
 *  Params      : same as printf()
 *  Returns     : 0 on success, otherwise error code.
 *---------------------------------------------------------------------------*/
Err COMPrintf(char *format, ...)
{
    static char  tmp_line[81];
    va_list      marker;

    if (port_open)
    {
        va_start(marker, format);
        vsprintf(tmp_line, format, marker);
        va_end(marker);

        return(COMSend(strlen(tmp_line),tmp_line));
    }    
    return(0);
}

