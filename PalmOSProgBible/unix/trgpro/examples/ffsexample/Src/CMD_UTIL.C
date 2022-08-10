#define PILOT_PRECOMPILED_HEADERS_OFF
/***********************************************************************
 *
 * Copyright (c) 1999, TRG, All Rights Reserved
 *
 * PROJECT:         Nomad FAT demo
 *
 * FILE:          cmd_util.c  
 *
 * DESCRIPTION:  Utility functions for cmd.c
 *
 * AUTHOR: Trevor Meyer         
 *
 * DATE: 8/9/99           
 *
 **********************************************************************/

#include <Pilot.h>

#include "cmd_util.h"
#include "trglib.h"


char   cmdline[MAX_LINE_LENGTH];
char   prevline[MAX_LINE_LENGTH];


/*--------------------------------------------------------------------------
 * Convert string to all caps.
 *--------------------------------------------------------------------------*/
void Capitalize(char *str)
{
    for(;*str;)
        *str++ = toupper(*str);
}


/*--------------------------------------------------------------------------
 * Sting handling functions
 *--------------------------------------------------------------------------*/
void StripWhiteSpace(char **str)
{
    while (**str == ' ')
        strcpy(*str,*str + 1);
}

void StripUntilWhiteSpace(char **str)
{
    while ((**str != ' ') && (**str != '\0'))
        strcpy(*str,*str + 1);
}

Err GetStrExpression(char **str, char *ret_str)
{
    char *src;

    StripWhiteSpace(str);
    if (**str == '\0')
        return(1);

    src = *str;
    while((*src != ' ') && *src)
        *ret_str++ = *src++;
    *ret_str = 0;

    StripUntilWhiteSpace(str);
    return(0);
}



/*----------------------------------------------------------------------
 * Decimal conversion functions.                                        
 *----------------------------------------------------------------------*/
UInt16 DecToWord(char *str)
{
   UInt8         i;
   UInt16         value=0;

   while (*str==' ') str++;             /* skip leading spaces */
   for (i=0; i<5 && *str; i++) 
   {
      if (!(*str>='0' && *str<='9')) 
          break;
      value = value*10 + (UInt16)(*str++ - '0');
   }
   return(value);
}

UInt32 DecToDWord(char *str)
{
   UInt8         i;
   UInt32        value = 0L;

   while (*str==' ') str++;             /* skip leading spaces */
   for (i=0; i<10 && *str; i++) 
   {
      if (!(*str>='0' && *str<='9')) 
          break;
      value = value*10L + (UInt32)(*str++ - '0');
   }
   return(value);
}

Err GetDecExpression(char **str, UInt16 *val)
{
    StripWhiteSpace(str);

    if (**str == '\0')
        return(1);

    *val = DecToWord(*str);

    StripUntilWhiteSpace(str);

    return(0);
}

Err GetDecExpressionDW(char **str, UInt32 *val)
{
    UInt16        tmp_val;

    StripWhiteSpace(str);

    if (**str == '\0')
        return(1);

    tmp_val = DecToWord(*str);
    *val = (UInt32)tmp_val;

    StripUntilWhiteSpace(str);

    return(0);
}

/*----------------------------------------------------------------------
 * Hex conversion functions.                                            
 *----------------------------------------------------------------------*/
char NibbleToHex(UInt8 nibble)
{
   nibble &= 0xf;
   nibble += '0';
   if (nibble <= '9') 
       return((char)nibble);
   return((char)(nibble+7));
}

void ByteToHex(UInt8 b, char *str)
{
   str[0] = NibbleToHex((UInt8)(b>>4));
   str[1] = NibbleToHex(b);
   str[2] = 0;
}

void ByteToBinary(UInt8 b, char *str)
{
   UInt8 i, mask;

   for (i=0, mask=0x80; i<8; i++, mask>>=1)
      if (b&mask) 
          str[i] = '1';
      else        
          str[i] = '0';
   str[8] = 0;
}

void WordToHex(UInt16 w, char *str)
{
   ByteToHex((UInt8)(w>>8), str);
   ByteToHex((UInt8)(w),    str+2);
}

void DwordToHex(UInt32 dw, char *str)
{
   WordToHex((UInt16)(dw>>16), str);
   WordToHex((UInt16)(dw),     str+4);
}

UInt8 HexToNibble(char ch)
{
   ch -= '0';
   if (ch<=9) 
       return((UInt8)ch);
   return((UInt8)(ch-7));
}

UInt32 HexToDword(char *str)
{
   UInt8         i;
   UInt32        value=0;

   while (*str==' ') str++;             /* skip leading spaces */
   for (i=0; i<8 && *str; i++) 
   {
      if (*str>='a') 
          *str -= 'a'-'A';
      if (!(*str>='0' && *str<='9' || *str>='A' && *str<='F')) 
          break;
      value <<= 4;
      value += HexToNibble(*str++);
   }
   return(value);
}

UInt16 HexToWord(char *str)
{
   return((UInt16)HexToDword(str));
}

UInt8 HexToByte(char *str)
{
   return((UInt8)HexToDword(str));
}

Err GetHexExpression(char **str, UInt32 *val)
{
    StripWhiteSpace(str);

    if (**str == '\0')
        return(1);

    *val = HexToDword(*str);

    StripUntilWhiteSpace(str);

    return(0);
}
