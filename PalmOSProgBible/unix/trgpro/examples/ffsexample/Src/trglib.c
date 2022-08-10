/***********************************************************************
 *
 * Copyright (c) 1999, TRG, All Rights Reserved
 *
 * PROJECT:         TRG palm common files
 *
 * FILE:            trglib.c
 *
 * DESCRIPTION:     Rewritten C-library functions.
 *                  The following standard C library functions have been replaced:
 *                      strcat(), strcpy(), strlen(), strcmp()
 *                      isprint(), toupper(), tolower()       
 *                      vsprintf(), sprintf()
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

#include "print.h"
#include "trglib.h"


/*--------------------------------------------------------------------------*
 * Name		: isprint()
 * Description	: Returns true if the character is printable ASCII.
 * Params	: chr -- character to check
 * Returns	: True if printable, false otherwise.
 *--------------------------------------------------------------------------*/
Boolean isprint(char chr)
{
    if (chr >= ' ' && chr <= '~')
        return(true);
            
    return(false);
}

/*--------------------------------------------------------------------------*
 * Name		: toupper()
 * Description	: Converts a lowercase ASCII letter to uppercase.
 * Params	: chr -- character to convert
 * Returns	: Converted character
 *--------------------------------------------------------------------------*/
char toupper(char chr)
{
    if (chr >= 'a' && chr <= 'z')
        chr -= 'a' - 'A';
    
    return(chr);
}

/*--------------------------------------------------------------------------*
 * Name         : strcat()
 * Description  : Concatentate two strings.
 * Params       : dest -- string to concat to
 *                src -- string to add to dest
 * Returns      : Pointer to destination
 *--------------------------------------------------------------------------*/
char *strcat(char *dest, char *src)
{
    char *retval;

    retval = dest + strlen(dest);

    for(;*src;)
        *retval++ = *src++;
    *retval = *src;

    return(dest);
}


/*--------------------------------------------------------------------------*
 * Name		: strcpy()
 * Description  : Copy a string from src to dest.
 * Params       : dest -- destination string
 *                src -- source string
 * Returns      : Pointer to destination
 *--------------------------------------------------------------------------*/
char *strcpy(char *dest, char *src)
{
    char *retval;

    for (retval = dest; *src;)
        *dest++ = *src++;

    *dest = *src;

    return(dest);
}    

/*--------------------------------------------------------------------------*
 * Name         : tolower()
 * Description  : Convert an alphabetic character to lower case
 * Params       : c -- character to convert
 * Returns      : Converted character
 *--------------------------------------------------------------------------*/
char tolower(char c)
{
    if ((c >= 'A') && (c <= 'Z'))
        return((char)(c | 0x20));

    return(c);
}    

/*--------------------------------------------------------------------------*
 * Name		: strlen()
 * Description	: Finds the length of a string
 * Params	: str -- string to process
 * Returns	: Length of str
 *--------------------------------------------------------------------------*/
int strlen(const char *str)
{
    int len = 0;

    while(*str++ != 0)
      	len++;

    return(len);
}


/*--------------------------------------------------------------------------*
 * Name        : strcmp()
 * Description : Compare two strings. Case sensitive
 * Params      : str1, str2 -- strings to compare
 * Returns     : 0 if strings identical, non-zero otherwise
 *--------------------------------------------------------------------------*/
int strcmp(const char *str1, const char *str2)
{
    for (; *str1 && *str2; str1++, str2++)
    {
        if (*str1 == *str2)
            continue;
        if (*str1 < *str2)
            return(-1);
        else
            return(1);
    }

    if (*str1 == *str2)
        return(0);
    if (*str1)
        return(1);
    return(-1);    
}


/*---------------------------------------------------------------------------
 *  Function	: vsprintf()
 *  Description : Replacement for C library function of same name. Necessary
 *                because original function was not reentrant. 
 *  Params      : see vsprintf() in C library documentation
 *  Returns     : see vsprintf() in C library documentation
 *---------------------------------------------------------------------------*/
int vsprintf(char *buffer, const char *format, va_list arglist)
{
     _doprnt(buffer,(char *)format,(int *)arglist);
     return(strlen(buffer));
}

/*---------------------------------------------------------------------------
 *  Function    : sprintf()
 *  Description : Replacement for C library function of same name. Necessary
 *                because original function was not reentrant. 
 *  Params      : see sprintf() in C library documentation
 *  Returns     : see sprintf() in C library documentation
 *---------------------------------------------------------------------------*/
int sprintf(char *buffer, const char *format,...)
{
     _doprnt(buffer,(char *)format,(int *)((&format)+1));
     return(strlen(buffer));
}

