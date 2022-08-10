/**********************************************************************/
/*                                                                    */
/* util.h:  LISPME utility functions needed everywhere                */
/*                                                                    */
/* LispMe System (c) FBI Fred Bayer Informatics                       */
/*                                                                    */
/* Distributed under the GNU General Public License;                  */
/* see the README file. This code comes with NO WARRANTY.             */
/*                                                                    */
/* Modification history                                               */
/*                                                                    */
/* When?      What?                                              Who? */
/* -------------------------------------------------------------------*/
/* 10.01.1999 New                                                FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

#ifndef INC_UTIL_H
#define INC_UTIL_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* Exported data                                                      */
/**********************************************************************/
extern  char* errInfo;
extern  Int16 expectedLen;

/**********************************************************************/
/* prototypes                                                         */
/**********************************************************************/
void    checkInt(PTR p)                SEC(VM);
void    checkString(PTR p)             SEC(VM);
void*   ptrFromObjID(UInt16 obj);
void    GrabMem(void)                  SEC(VM);
void    ReleaseMem(void)               SEC(VM);
PTR     str2Lisp(char* cp)             SEC(VM);
void    typeError(PTR p, char* type)   SEC(VM);
void    parmError(PTR p, char* func)   SEC(VM);
void    error1(UInt16 err, PTR p)      SEC(VM);
short   listLength(PTR l)              SEC(VM);
UInt16  makeFileName(PTR fileName);
PTR     getTime(void);
void    enable(UInt16 id);
void    disable(UInt16 id);
void    disableButtons(void);
void    enableButtons(void);
void    displayError(UInt32 err);                 
void    enableCtls(Boolean enable);
void    updateScrollBar(void);                       

#endif
