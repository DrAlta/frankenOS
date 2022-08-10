/**********************************************************************/
/*                                                                    */
/* comp.h:  LISP to SECD compiler                                     */
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
/* 04.03.1998 New                                                FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* prototypes                                                         */
/**********************************************************************/
void InitCompiler(void)           SEC(COMP);
PTR  compile(PTR expr, PTR names) SEC(COMP);
