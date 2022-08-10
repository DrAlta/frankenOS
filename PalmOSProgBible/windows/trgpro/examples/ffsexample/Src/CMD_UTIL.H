/***********************************************************************
 *
 * Copyright (c) 1999, TRG, All Rights Reserved
 *
 * PROJECT:         Nomad FAT demo
 *
 * FILE:            
 *
 * DESCRIPTION:
 *
 * AUTHOR: Trevor Meyer         
 *
 * DATE: 8/9/99           
 *
 **********************************************************************/
#ifndef _CMD_UTIL_H_
#define _CMD_UTIL_H_

#define MAX_LINE_LENGTH 80

extern char cmdline[];
extern char prevline[];

void       Capitalize(char *str);
void       StripWhiteSpace(char **str);
void       StripUntilWhiteSpace(char **str);
Boolean    GetString(char chr);
UInt16     DecToWord(char *str);
UInt32     DecToDWord(char *str);
Err        GetDecExpression(char **str, UInt16 *val);
Err        GetDecExpressionDW(char **str, UInt32 *val);
char       NibbleToHex(UInt8 nibble);
void       ByteToHex(UInt8 b, char *str);
void       ByteToBinary(UInt8 b, char *str);
void       WordToHex(UInt16 w, char *str);
void       DwordToHex(UInt32 dw, char *str);
UInt8      HexToNibble(char ch);
UInt32     HexToDword(char *str);
UInt16     HexToWord(char *str);
UInt8      HexToByte(char *str);
Err        GetHexExpression(char **str, UInt32 *val);
Err        GetStrExpression(char **str, char *ret_str);

#endif
