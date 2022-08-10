/***********************************************************************
 *
 * Copyright (c) 1999, TRG, All Rights Reserved
 *
 * PROJECT:         TRG palm common files
 *
 * FILE:            print.c
 *
 * DESCRIPTION:     printf support
 *
 * AUTHOR:          
 *
 * DATE:            
 *
 **********************************************************************/
#ifndef PILOT_PRECOMPILED_HEADERS_OFF
#define PILOT_PRECOMPILED_HEADERS_OFF
#endif

/****************************************************************************
 * File        : print.c
 * Description : Implements a reentrant routine similar to sprintf.
 ****************************************************************************/
#include <Pilot.h>
#include "trglib.h"
#include "print.h"

/*--------------------------------------------------------------------------*
 * Defines to handle system dependencies:
 *      MAX_INT_DIGITS -- maximum digits in a decimal int
 *      MAX_LONG_DIGITS -- maximum digits in a decimal long
 *      MAX_OCT_DIGITS -- maximum digits in an octal int
 *      MAX_LOCT_DIGITS -- maximum digits in an octal long
 *      MAX_HEX_DIGITS -- maximum digits in a hexidecimal int
 *      MAX_LHEX_DIGITS -- maximum digits in a hexidecimal long
 *--------------------------------------------------------------------------*/
#define MAX_INT_DIGITS    	5
#define MAX_LONG_DIGITS   	10
#define MAX_OCT_DIGITS    	6
#define MAX_LOCT_DIGITS   	11
#define MAX_HEX_DIGITS    	4
#define MAX_LHEX_DIGITS   	8

#define MAXSTR            		80
#define MAX_DIGITS        		30


/* local pototypes */
static void _prt10(unsigned int num, char *str);
static void _prtl10(unsigned long num, char *str);
static void _prt8(unsigned int num, char *str);
static void _prtl8(unsigned long num, char *str);
static void _prt16(unsigned int num, char *str, Boolean use_caps);
static void _prtl16(unsigned long num, char *str, Boolean use_caps);


/*--------------------------------------------------------------------------*
 * Name        	: _doprnt()
 * Description : Similar to sprintf(). Takes a destination string, a format
 *               	  specification string, and a pointer to the first of a list
 *               	  of arguements, and returns a string with the arguements
 *               	  replacing the format specifiers. This routine is reentrant,
 *                	  unlike the C library sprintf. 
 * Params      	: buffer -- pointer to destination string
 *               	  fmt -- format string, like printf()
 *               	  args -- pointer to first argument, cast to (int *)
 * Returns      : Nothing
 *--------------------------------------------------------------------------*/
void _doprnt(char *buffer, char *fmt, int *args)
{
   	int            		c, i, f;
   	char           		*str, string[MAX_DIGITS+2];   /* increased for doubles */
   	int            		length;
   	char           		fill, sign, digit1;
   	int            		leftjust, longflag, leading;
   	int            		fmax, fmin;
        Boolean                 use_caps = false;

   	/* loop until format specifier parsing is finished */
   	for(;;)
   	{
/*--------------------------------------------------------------------------*
 * This first part processes the format string up to the next format
 * specifier, handling the various format modifiers.
 *--------------------------------------------------------------------------*/
      	/* echo characters unil "%" or end of fmt string */
      	while((c = *fmt++) != '%')
      	{
       		if (c == '\0')
         	{
            		*buffer = '\0';
            		return;
         	}   
         	*buffer++ = (char)c;
      	}

      	/* echo "...%%..." as '%' */
      	if (*fmt == '%')
      	{
         	*buffer++ = *fmt++;
         	continue;
      	}

      	/* check for "%-..." == left-justified output */
      	if (*fmt == '-')            /* modified this test slightly  */
      	{                           /*    from that in book so that */
          	leftjust = 1;           /*    the compiler would not    */
          	fmt++;                  /*    complain.                 */
      	}
      	else
          	leftjust = 0;

      	/* allow for zero-filled numeric outputs (%0...") */
      	fill = (char)(((*fmt == '0') ? *fmt++ : ' '));

      	/* allow for minimum field width specifier for %d,u,x,o,c,s */
      	/* also allow %* for variable width (%0* as well)           */
      	fmin = 0;
      	if (*fmt == '*')
      	{
         	fmin = *args++;
         	++fmt;
      	}
      	else 
         	while('0' <= *fmt && *fmt <= '9')
            		fmin = fmin*10 + *fmt++ - '0';

      	/* allow for maximum string width for %s */
      	fmax = 0;
      	if (*fmt == '.')
      	{
         	if (*(++fmt) == '*')
         	{
            		fmax = *args++;
            		++fmt;
         	}
         	else          /* indentation here differs from book ??? */
            		while('0' <= *fmt && *fmt <= '9')
               			fmax = fmax*10 + *fmt++ - '0';
      	}

      	/* check for the 'l' option to force long numeric */
      	if (*fmt == 'l')               	// modified this test slightly
      	{                                  	//     from the one in the book
          	longflag = 1;        	//     so that the compiler would
          	fmt++;                  	//     not complain.
      	}
      	else
          	longflag = 0;

      	str = string;
      	if ((f = *fmt++) == '\0')
      	{
         	*buffer++ = '%';
         	*buffer = '\0';
         	return;
      	}
      	sign = '\0';

/*--------------------------------------------------------------------------*
 * Next, this cascaded if statement handles the format specifier, casting the
 * corresponding argument to the correct value and converting it to a string.
 * A case structure was not used because CodeWarrior generates a jump table
 * which does not work in certain situations.
 *--------------------------------------------------------------------------*/
        /* character */
        if (f == 'c')
        {
            string[0] = (char) *args;
            string[1] = '\0';
            fmax = 0;
            fill = ' ';
        }

        /* string */
        else if (f == 's')
        {
            str = (char *)*((unsigned long *)args);
            args++;
            fill = ' ';
        }

        /* integer */
        else if ((f == 'd') || (f == 'u'))
        {
            /* signed integer */
            if (f == 'd')
            {
                /* long signed integers */
                if (longflag)
                {
/*--------------------------------------------------------------------------*
 * The routine that converts integers expects them to be unsigned, so we
 * remove the sign, save it, then fall through to the unsigned ("u")
 * specifier.
 *--------------------------------------------------------------------------*/
                    if (*(long *)args < 0)
                    {
                        sign = '-';
                        *(long *)args = - *(long *)args;
                    }
                }
                else if (*args < 0)
                {
                    sign = '-';
                    *args = - *args;
                }
            }

            /* signed/unsigned integer */
            if (longflag)
            {
                /* long signed/unsigned integer */
                digit1 = '\0';

                /* "negative" longs in unsigned format can't be computed */
                /* with long division. Convert *args to "positive",      */
                /* digit1 = how much to add back afterwards              */
                while(*(long *)args < 0)
                {
                    *(long *)args -= 1000000000L;
                    digit1 = (char)(digit1 + 1);
                }
                _prtl10(*(unsigned long *)args,str);
                str[0] += digit1;
                args += sizeof(long)/sizeof(int) - 1;
            }
            else
            {
                /* signed/unsigned integer */
                _prt10((unsigned int)*args,str);
            }
            fmax = 0;
        }

        /* octal integer */
        else if (f == 'o')
        {
            if (longflag)
            {
                /* long octal integer */
                _prtl8(*(unsigned long *)args,str);
                args += sizeof(long)/sizeof(int) - 1;
            }
            else
            {
                /* octal integer */
                _prt8((unsigned int)*args,str);
            }
            fmax = 0;
        }

        /* Hexidecimal integer */
        else if ((f == 'X') || (f == 'x'))
        {
            /* print in capital letters */
            if (f == 'X')
                use_caps = true;

            if (longflag)
            {
                /* long hexidecimal integer */
                _prtl16(*(unsigned long *)args,str,use_caps);
                args += sizeof(long)/sizeof(int) - 1;
            }
            else
            {
                /* hexidecimal integer */
                _prt16((unsigned int)*args,str,use_caps);
            }
            fmax = 0;
            use_caps = false;
        }

        /* ??? (unsupported) */
        else
            *buffer++ = (char)f;

/*--------------------------------------------------------------------------*
 * This last part handles formating of the string created by the conversion
 * routines, above. Formating is based on format modifiers parsed in the
 * first part of the loop.
 *--------------------------------------------------------------------------*/
      	args++;
      	for(length = 0; str[length] != '\0'; length++)
         	;
      	if ((fmin > MAXSTR) || (fmin < 0))
         	fmin = 0;
      	if ((fmax > MAXSTR) || (fmax < 0))
         	fmax = 0;
      	leading = 0;
      
      	/* handle field length spcifiers */
      	if ((fmax != 0)||(fmin != 0))
      	{
            	if (fmax != 0)
               		if (length > fmax)
                  		length = fmax;
            	if (fmin != 0)
               		leading = fmin - length;
            	if (sign == '-')
               		--leading;
      	}

      	/* copy fill, sign, number to buffer */
      	if ((sign == '-')&&(fill == '0'))
         	*buffer++ = sign;
      	if (leftjust == 0)
         	for (i=0; i<leading; i++)
            		*buffer++ = fill;
      	if ((sign == '-')&&(fill == ' '))
         	*buffer++ = sign;
      	for(i=0; i<length; i++)
         	*buffer++ = str[i];
      	if (leftjust != 0)
         	for(i=0; i<leading; i++)
            		*buffer++ = fill;
   	}
}


/*--------------------------------------------------------------------------*
 * Name        	: _prt10()
 * Description : Convert an unsigned integer to a string in base 10.
 * Params      	: num -- number to convert
 *               	  str -- pointer to return string
 * Returns     	: Nothing
 *--------------------------------------------------------------------------*/
static void _prt10(unsigned int num, char *str)
{
    	int          	i;
    	char         temp[MAX_INT_DIGITS+1];

    	temp[0] = '\0';

    	/* convert it one digit at a time, starting with the 1s digit */
    	for (i=1; i<=MAX_INT_DIGITS; i++)
    	{
        	temp[i] = (char)((char)(num % 10) + '0');
        	num = (unsigned int)(num/10);
    	}

    	/* get rid of leading 0s */
    	for (i=MAX_INT_DIGITS; temp[i] == '0'; i--)
        	;
    	if (i == 0)
        	i++;

    	/* reverse digit order */
    	while(i >= 0)
        	*str++ = temp[i--];
}

/*--------------------------------------------------------------------------*
 * Name        	: _prtl10()
 * Description : Convert an unsigned long int to a string in base 10.
 * Params      	: num -- number to convert
 *               	  str -- pointer to return string
 * Returns      : Nothing
 *--------------------------------------------------------------------------*/
static void _prtl10(unsigned long num, char *str)
{
    	int          	i;
    	char         temp[MAX_LONG_DIGITS+1];

    	temp[0] = '\0';

    	/* convert to string one digit at a time */
    	for (i=1; i<=MAX_LONG_DIGITS; i++)
    	{
        	temp[i] = (char)((char)(num % 10) + '0');
        	num = num/10;
    	}

    	/* get rid of leading 0s */
    	for (i=MAX_LONG_DIGITS; temp[i] == '0'; i--)
        	;
    	if (i == 0)
        	i++;

    	/* reverse order of digits */
    	while(i >= 0)
        	*str++ = temp[i--];
}

/*--------------------------------------------------------------------------*
 * Name        	: _prt8()
 * Description : Convert an unsigned int to an octal string.
 * Params      	: num -- number to convert
 *               	  str -- pointer to return string
 * Returns     	: Nothing
 *--------------------------------------------------------------------------*/
static void _prt8(unsigned int num, char *str)
{
    	int          	i;
    	char         temp[MAX_OCT_DIGITS+1];

    	temp[0] = '\0';

    	/* convert number one digit at a time */
    	for (i=1; i<=MAX_OCT_DIGITS; i++)
    	{
        	temp[i] = (char)((num & 07) + '0');
        	num = (unsigned int)((num >> 3) & 0037777);
    	}
    	temp[MAX_OCT_DIGITS] &= '1';

    	/* get rid of leading 0s */
    	for(i=MAX_OCT_DIGITS; temp[i] == '0'; i--)
        	;
    	if (i == 0)
        	i++;

    	/* reverse the digit order */
    	while(i >= 0)
        	*str++ = temp[i--];
}

/*--------------------------------------------------------------------------*
 * Name        	: _prtl8()
 * Description : Convert an unsigned long int to an octal string
 * Params      	: num -- number to convert
 *               	  str -- pointer to return string
 * Returns     	: Nothing
 *--------------------------------------------------------------------------*/
static void _prtl8(unsigned long num, char *str)
{
    	int          	i;
    	char         temp[MAX_LOCT_DIGITS+1];

    	temp[0] = '\0';

    	/* convert one digit at a time */
    	for (i=1; i<=MAX_LOCT_DIGITS; i++)
    	{
        	temp[i] = (char)((num & 07) + '0');
        	num = num >> 3;
    	}
    	temp[MAX_LOCT_DIGITS] &= '3';

    	/* get rid of leading 0s */
    	for(i=MAX_LOCT_DIGITS; temp[i] == '0'; i--)
        	;
    	if (i == 0)
        	i++;

    	/* reverse digit order */
    	while(i >= 0)
        	*str++ = temp[i--];
}

/*--------------------------------------------------------------------------*
 * Name        	: _prt16()
 * Description : Convert an unsigned int to a hexidecimal string
 * Params      	: num -- number to convert
 *               	  str -- pointer to return string
 *               	  use_caps -- if TRUE, use capital letters in conversion,
 *                      	otherwise use lower-case
 * Returns     	: Nothing
 *--------------------------------------------------------------------------*/
static void _prt16(unsigned int num, char *str, Boolean use_caps)
{
    	int          	i;
    	char         temp[MAX_HEX_DIGITS+1];

    	temp[0] = '\0';

    	/* convert one digit at a time */
    	for(i=1; i<= MAX_HEX_DIGITS; i++)
    	{   
        	if (use_caps)
            		temp[i] = "0123456789ABCDEF"[num & 0x0f];
        	else
            		temp[i] = "0123456789abcdef"[num & 0x0f];
        	num = (unsigned int)(num >> 4);
    	}

    	/* get rid of leading 0s */
    	for (i=MAX_HEX_DIGITS; temp[i] == '0'; i--)
        	;
    	if (i == 0)
        	i++;

    	/* reverse order of the digits */
    	while(i >= 0)
        	*str++ = temp[i--];
}

/*--------------------------------------------------------------------------*
 * Name        	: _ptrl16()
 * Description : Convert an unsigned long int to a hexidecimal string.
 * Params      	: num -- number to convert
 *               	  str -- pointer to return string
 *                 	  use_caps -- if TRUE, use capital letters in conversion,
 *                      	otherwise use lower-case.
 * Returns     	: Nothing
 *--------------------------------------------------------------------------*/
static void _prtl16(unsigned long num, char *str, Boolean use_caps)
{
    	int          	i;
    	char         temp[MAX_LHEX_DIGITS+1];

    	temp[0] = '\0';

    	/* convert one digit at a time */
    	for(i=1; i<=MAX_LHEX_DIGITS; i++)
    	{
        	if (use_caps)
            		temp[i] = "0123456789ABCDEF"[num & 0x0f];
        	else
            		temp[i] = "0123456789abcdef"[num & 0x0f];
        	num = num >> 4;
    	}

    	/* remove leading 0s */
    	for (i=MAX_LHEX_DIGITS; temp[i] == '0'; i--)
        	;
    	if (i == 0)
        	i++;

    	/* reverse digit order */
    	while(i >= 0)
        	*str++ = temp[i--];
}

