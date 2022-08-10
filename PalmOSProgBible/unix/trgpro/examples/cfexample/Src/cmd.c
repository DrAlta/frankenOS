#define PILOT_PRECOMPILED_HEADERS_OFF

/***********************************************************************
 *
 * Copyright (c) 1999, TRG, All Rights Reserved
 *
 * PROJECT:         Nomad CF demo
 *
 * FILE:     cmd.c       
 *
 * DESCRIPTION:   Contains the Cf demo program commands. 
 *
 * AUTHOR: Trevor Meyer         
 *
 * DATE: 8/10/99           
 *
 **********************************************************************/

#include <Pilot.h>
#include <SysEvtMgr.h>

#include "com.h"
#include "cmd_util.h"
#include "trglib.h"
#include "cmd.h"
#include "cflib.h"


#define DEFAULT_NUM_LINES       12

static int     index = 0;

static char   *GetStr(void);

extern UInt16  cf_lib;


/*--------------------------------------------------------------------------
 * Function    : Help
 * Description : Display the command menu.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void Help(void)
{
    COMPrintf("TRG CF debugger V 1.01 \r\n\r\n");

    COMPrintf("  ADDR                         - Display card base addresses\r\n");
    COMPrintf("  CF_OFF                       - Turn card off\r\n");
    COMPrintf("  CF_ON <width>                - Turn card on\r\n");
    COMPrintf("  CHECK                        - Check for card inserted\r\n");
    COMPrintf("  RESET                        - Reset card\r\n");
    COMPrintf("  SLOT                         - Display card slot status\r\n");
    COMPrintf("  SWAP_ON                      - Enable CF bus byte swapping\r\n");
    COMPrintf("  SWAP_OFF                     - Disable CF bus byte swapping\r\n");
    COMPrintf("  WIDTH <width>                - Set bus width to 8 or 16 bits\r\n");

    COMPrintf("\r\n");
    COMPrintf("  D[<B,W,D>] [address [lines]] - Dump memory\r\n");
    COMPrintf("  E[<B,W,D>] <address>         - Edit memory\r\n");

    COMPrintf("\r\n");
    COMPrintf("  CTRL-A                       - Previous command\r\n");
    COMPrintf("  ?                            - help\r\n");
}


/*--------------------------------------------------------------------------
 * Function    : GetString
 * Description : Get a complete command string from the user. Non-blocking --
 *               when called, adds the specified character to the current
 *               command string and returns. Allows backspace, and resets
 *               power-off timer each time something is typed.
 * Params      : chr -- character from serial port
 * Returns     : true if a full command string is available, false otherwise
 *--------------------------------------------------------------------------*/
Boolean GetString(char chr)
{
    static Boolean search_string = false;

    /* got user activity -- don't time out */
    EvtResetAutoOffTimer();

    switch (chr)
    {
        case '\b' :
            if (index > 0)
            {
                COMPrintf("\b \b");
                index--;
            }
            break;
        case '\r' :
            cmdline[index] = 0;
            COMPrintf("\r\n");
            search_string = false;
            return(true);
        default :
            if (isprint(chr) && (index < MAX_LINE_LENGTH))
            {
                COMPrintf("%c", chr);

                /* " signifies start of a literal search string */
                if (chr == '\"')
                    search_string = true;

                /* if entering search string, don't capitalize */
                if (!search_string)
                    cmdline[index++] = (char)toupper(chr);
                else
                    cmdline[index++] = chr;
            }
            break;
    }
    return(false);
}


/*--------------------------------------------------------------------------
 * Function    : GetStr
 * Description : Gets a response from the user over the serial port.
 *               Blocking (events are not processed while this is waiting).
 *               Returns when a CR is received.
 * Params      : none
 * Returns     : String entered from user.
 *--------------------------------------------------------------------------*/
static char *GetStr(void)
{
    char chr;
     
    cmdline[0] = '\0';
    index = 0;

    do {
        while(COMBytesAvailable() == 0)    /* wait -- COMGetC() times out */
           ;
        COMGetC(&chr);

    } while (!GetString(chr));

    return(&(cmdline[0]));
}


/*--------------------------------------------------------------------------
 * Function    : CMDSlotStatus
 * Description : Display the status of the CF slot. Demonstrates
 *               CfCardInserted(), CfSlotPowered(), CfGetDataWidth(),
 *               CfGetIRQLevel() calls.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDSlotStatus(void)
{
    UInt8           level;
    Boolean         inserted, powered, in_use;
    data_width_type width;

    CfCardInserted(cf_lib, 0, &inserted);
    CfSlotPowered(cf_lib, 0, &powered, NULL, &in_use);
    if (powered)
        CfGetDataWidth(cf_lib, 0, &width);      // redundant, for illustration
    CfGetIRQLevel(cf_lib, 0, &level);

    COMPrintf("Slot 0 status:\r\n");
    if (inserted)
        COMPrintf("    Card detected in slot\r\n");
    else
        COMPrintf("    Slot empty\r\n");
    COMPrintf("    Slot assigned to IRQ %d\r\n",level);
    if (!powered)
        COMPrintf("    Slot not powered\r\n");
    else
    {
        COMPrintf("    Slot powered in ");
        if (width == CF_8_BIT)
            COMPrintf("8-bit mode\r\n");
        else
            COMPrintf("16-bit mode\r\n");
    }
}


/*--------------------------------------------------------------------------
 * Function    : CMDCheckCard
 * Description : Check for presence of card. Demonstrates CfCardInserted()
 *               call.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDCheckCard(void)
{
    Boolean inserted;

    CfCardInserted(cf_lib, 0, &inserted);
    if (inserted)
        COMPrintf("Card inserted in slot\r\n");
    else
        COMPrintf("No card detected in slot\r\n");
}



/*--------------------------------------------------------------------------
 * Function    : CMDSetSwap
 * Description : Enable/disable CF data bus byte swapping. Demonstrates
 *               CfSetSwapping().
 * Params      : swap_bytes -- true to enable, false to disable
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDSetSwap(Boolean swap_bytes)
{
    CfSetSwapping(cf_lib, 0, swap_bytes);
}


/*--------------------------------------------------------------------------
 * Function    : CMDResetSlot
 * Description : Activate reset line on the card slot. Demonstrates CfReset()
 *               call.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDResetSlot(void)
{
    CfReset(cf_lib, 0);
    COMPrintf("Card reset\r\n");
}


/*--------------------------------------------------------------------------
 * Function    : CMDTurnSlotOff
 * Description : Remove power from the card slot. Demonstrates CfPowerSlot()
 *               call.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDTurnSlotOff(void)
{
    CfPowerSlot(cf_lib, 0, CF_POWER_OFF, CF_WIDTH_NONE);
    COMPrintf("Card powered off\r\n");
}


/*--------------------------------------------------------------------------
 * Function    : CMDTurnSlotOn
 * Description : Power up the CF slot. Demonstrates CfPowerSlot() call.
 * Params      : Command string containing bus width (8 or 16).
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDTurnSlotOn(char *cmdline)
{
    UInt16   width;

    if (GetDecExpression(&cmdline, &width) != 0)
    {
        COMPrintf("Must specify bus width (8 or 16)\r\n");
        return;
    }

    if ((width != 8) && (width != 16))
    {
        COMPrintf("Invalid bus width (8 or 16)\r\n");
        return;
    }

    if (width == 8)
        CfPowerSlot(cf_lib, 0, CF_POWER_ON, CF_8_BIT);
    else
        CfPowerSlot(cf_lib, 0, CF_POWER_ON, CF_16_BIT);

    COMPrintf("Slot powered in %d-bit mode\r\n",width);
}


/*--------------------------------------------------------------------------
 * Function    : CMDSetBusWidth
 * Description : Change the CF data bus width to 8 or 16 bit. Demonstrates
 *               CfSlotPowered(), CfSetDataWidth() calls.
 * Params      : Command string containing desired bus width.
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDSetBusWidth(char *cmdline)
{
    UInt16   width;
    Boolean  powered;
    
    if (GetDecExpression(&cmdline, &width) != 0)
    {
        COMPrintf("Must specify bus width (8 or 16)\r\n");
        return;
    }

    if ((width != 8) && (width != 16))
    {
        COMPrintf("Invalid bus width (8 or 16)\r\n");
        return;
    }

    CfSlotPowered(cf_lib, 0, &powered, NULL, NULL);
    if (!powered)
    {
        COMPrintf("Can't set bus width unless card is powered\r\n");
        return;
    }

    COMPrintf("Setting to %d bits\r\n",width);

    if (width == 8)
        CfSetDataWidth(cf_lib, 0, CF_8_BIT);
    else
        CfSetDataWidth(cf_lib, 0, CF_16_BIT);
}


/*--------------------------------------------------------------------------
 * Function    : CMDGetAddr
 * Description : Display base addresses for card. Demonstrates
 *               CfBaseAddress() call.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDGetAddr(void)
{
    UInt32      attribute, common, io;

    COMPrintf("CF card base addresses:\r\n");
    CfBaseAddress(cf_lib, 0, &attribute, &common, &io);
    COMPrintf("    attribute memory: 0x%08lX\r\n",attribute);
    COMPrintf("       common memory: 0x%08lX\r\n",common);
    COMPrintf("           io memory: 0x%08lX\r\n",io);
}


/*--------------------------------------------------------------------------
 * Function    : CMDEditMemory
 * Description : Display and optionally modify a memory location.
 * Params      : cmdline -- Command line containing memory location
 *               data_size -- number nibbles in item
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDEditMemory(char *cmdline, UInt8 data_size)
{
    UInt32   edit_address;
    char    *response;
    int      width;

    if (GetHexExpression(&cmdline, &edit_address) != 0)
        return;

    switch (data_size) 
    {
        case 'B' : width = 2; break;
        case 'W' : width = 4; break;
        case 'D' : width = 8; break;
        default  : width = 2;
    }

    for(;;)
    {
        COMPrintf("%08lX", edit_address);

/*----------------------------------------------------------------------
 * Show the original data.  Start a new line if necessary.
 *----------------------------------------------------------------------*/

        COMPrintf(" ");
        switch(width)
        {
            case 2 : COMPrintf("%02X", *(UInt8 *)edit_address);  break;
            case 4 : COMPrintf("%04X", *(UInt16 *)edit_address);  break;
            case 8 : COMPrintf("%08lX", *(UInt32 *)edit_address); break;
            default :
                break;
        }
        COMPrintf(".");

/*----------------------------------------------------------------------
 * Get a response from the user.
 *----------------------------------------------------------------------*/
        response = GetStr();
        if (strlen(response) > 0)
        {

            HDWUnwriteProtectRAM();
            if (width == 2)      
                *(UInt8*)edit_address  = HexToByte(response);
            else if (width == 4) 
                *(UInt16*)edit_address  = HexToWord(response);
            else if (width == 8) 
                *(UInt32*)edit_address = HexToDword(response);
            HDWWriteProtectRAM();
        }
        else
            return;

        edit_address += (width/2);
    }
}



/*--------------------------------------------------------------------------
 * Function    : CMDDumpMemory
 * Description : Dump a block of memory in byte, word, or dword format.
 *               In byte mode, display printable chars as ASCII.
 * Params      : cmdline -- command line containing starting memory address
 *                      and optionally number of lines to display
 *               size -- item size in bytes.
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDDumpMemory(char *cmdline, UInt8 size)
{
    static Ptr     next_address = 0;
    static UInt16  num_lines = DEFAULT_NUM_LINES;
    Ptr            tmptr, org_address;
    UInt32         value;
    UInt16         lines;
    int            i, j;

    if (GetHexExpression(&cmdline, &value) != 0)
        tmptr = next_address;
    else
    {
        tmptr = (Ptr)value;
        if (GetDecExpression(&cmdline,&lines) == 0)
            num_lines = lines;
    }


    for(j=0; j<num_lines; j++)
    {
        COMPrintf("%08lX: ", tmptr);
        org_address = tmptr;

        for (i=0;i<16/size;i++)
        {
            switch (size)
            {
                case 1 :
                    COMPrintf("%02x", *((UInt8 *)tmptr));
                    break;
                case 2 :
                    COMPrintf("%04x", *((UInt16 *)tmptr));
                    break;
                case 4 :
                    COMPrintf("%08lx", *((UInt32 *)tmptr));
                    break;
            }
            if ((i == 7) && (size == 1))
                COMPrintf(" - ");
            else
                COMPrintf(" ");
        
            tmptr += size;
        }            

        /* if bytes, show also in ASCII */
        if (size == 1)
        {
            COMPrintf("  ");
            tmptr = org_address;
            for (i=0;i<16;i++)
            {
                if (isprint(*tmptr))
                    COMPrintf("%c", *tmptr);
                else
                    COMPrintf(".");
                tmptr++;
            }
        }
        COMPrintf("\r\n");
    }
    next_address = tmptr;
}


/*--------------------------------------------------------------------------
 * Function    : CMDInit
 * Description : Initializes the command interpreter.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
void CMDInit(void)
{
    cmdline[0] = '\0';
    index = 0;
    COMPrintf("\r\n\r\n>");
}    


/*--------------------------------------------------------------------------
 * Function    : CMDProcess
 * Description : Get commands from the user over the serial port and execute
 *               the commands. Called each time a character is typed, and
 *               calls GetString to build up a command string.
 * Params      : chr -- current character
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
void CMDProcess(char chr)
{
    char   cmd[10];
    char  *cmd_ptr;
    
    if ((index == 0) && (chr == 1))   //CTRL-A
    {
        StrCopy(cmdline, prevline);
        index = (int)strlen(cmdline);
        COMPrintf("%s", cmdline);
        return;
    }

    if (!GetString(chr))
        return;

    StrCopy(prevline, cmdline);
    
    cmd_ptr = cmdline;
    GetStrExpression(&cmd_ptr, cmd);
    Capitalize(cmd);

    if ((strcmp(cmd,"EB") == 0) ||
        (strcmp(cmd,"EW") == 0) ||
        (strcmp(cmd,"ED") == 0))
    {
        CMDEditMemory(cmd_ptr,cmd[1]);
    }

    else if (strcmp(cmd, "SLOT") == 0)
        CMDSlotStatus();

    else if (strcmp(cmd, "CHECK") == 0)
        CMDCheckCard();

    else if (strcmp(cmd, "SWAP_ON") == 0)
        CMDSetSwap(true);

    else if (strcmp(cmd, "SWAP_OFF") == 0)
        CMDSetSwap(false);

    else if (strcmp(cmd, "ADDR") == 0)
        CMDGetAddr();

    else if (strcmp(cmd, "WIDTH") == 0)
        CMDSetBusWidth(cmd_ptr);

    else if (strcmp(cmd,"DB") == 0)
        CMDDumpMemory(cmd_ptr, 1);

    else if (strcmp(cmd,"DW") == 0)
        CMDDumpMemory(cmd_ptr, 2);

    else if (strcmp(cmd,"DD") == 0)
        CMDDumpMemory(cmd_ptr, 4);

    else if (strcmp(cmd, "RESET") == 0)
        CMDResetSlot();

    else if (strcmp(cmd, "CF_OFF") == 0)
        CMDTurnSlotOff();

    else if (strcmp(cmd, "CF_ON") == 0)
        CMDTurnSlotOn(cmd_ptr);

    else if (strcmp(cmd,"?") == 0)
        Help();

    index = 0;
    cmdline[0] = '\0';
    COMPrintf(">");
}
