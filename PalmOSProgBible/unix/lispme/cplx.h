/**********************************************************************/
/*                                                                    */
/* cplx.c:  LISPME complex arithmetic                                 */
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
/* 10.10.1998 New                                                FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

#ifndef INC_CPLX_H
#define INC_CPLX_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* prototypes                                                         */
/**********************************************************************/
PTR addCpl(double ar, double ai, double br, double bi) SEC(VM);
PTR subCpl(double ar, double ai, double br, double bi) SEC(VM);
PTR mulCpl(double ar, double ai, double br, double bi) SEC(VM);
PTR divCpl(double ar, double ai, double br, double bi) SEC(VM);

PTR magnitude(PTR p)               SEC(VM);
PTR angle(PTR p)                   SEC(VM);

PTR cplExp(double re, double im)   SEC(VM);
PTR cplLog(double re, double im)   SEC(VM);
PTR cplSqrt(double re, double im)  SEC(VM);

PTR cplSin(double re, double im)   SEC(VM);
PTR cplCos(double re, double im)   SEC(VM);
PTR cplTan(double re, double im)   SEC(VM);

PTR cplSinh(double re, double im)  SEC(VM);
PTR cplCosh(double re, double im)  SEC(VM);
PTR cplTanh(double re, double im)  SEC(VM);

PTR cplAsin(double re, double im)  SEC(VM);
PTR cplAcos(double re, double im)  SEC(VM);
PTR cplAtan(double re, double im)  SEC(VM);

PTR cplAsinh(double re, double im) SEC(VM);
PTR cplAcosh(double re, double im) SEC(VM);
PTR cplAtanh(double re, double im) SEC(VM);

#endif
