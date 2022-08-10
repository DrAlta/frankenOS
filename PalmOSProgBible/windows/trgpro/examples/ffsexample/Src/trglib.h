/***********************************************************************
 *
 * Copyright (c) 1999, TRG, All Rights Reserved
 *
 * PROJECT:         TRG palm common files
 *
 * FILE:            trglib.h
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
#ifndef _TRGLIB_H_
#define _TRGLIB_H_

/*-------------------------------------------------------------------------
 * Some standard C library stuff not included with the Palm OS
 *------------------------------------------------------------------------*/

/* variable arg list processing */
#define va_list                   void *
#define va_start(marker,format)   (marker =(va_list)((&format)+1))
#define va_end(marker)            /* function not needed, but included for compatibility */

/* character processing */
char    toupper(char chr);
char    tolower(char c);
Boolean isprint(char chr);

/* string processing */
int     strlen(const char *str);
char   *strcpy(char *dest, char *src);
int     strcmp(const char *str1, const char *str2);
char   *strcat(char *dest, char *src);

/* printf support */
int     vsprintf(char *buffer, const char *format, va_list arglist);
int     sprintf(char *buffer, const char *format,...);


#endif
