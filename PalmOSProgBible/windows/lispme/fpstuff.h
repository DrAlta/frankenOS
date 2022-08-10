/**********************************************************************/
/*                                                                    */
/* fpstuff.h: LISPME floating point operations macros and prototypes  */
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
/* 22.12.1997 New                                                FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

#ifndef INC_FPSTUFF_H
#define INC_FPSTUFF_H

/**********************************************************************/
/* GCC doesn't handle NewFloatMgr.h correctly, so hack around it      */
/* (see news://news.massena.com/342FADFF.C8645E30@netcom.com)         */
/**********************************************************************/
#define _DONT_USE_FP_TRAPS_ 1
#include <FloatMgr.h>
#define PI 3.141592653589793

/**********************************************************************/
/* Use (obsolete type UInt for MathLib                                */
/**********************************************************************/
#define UInt UInt16
#define UIntPtr UInt16*
#define Int Int16
#define Long Int32
#define ULong UInt32
#include "MathLib.h"
#undef UInt
#undef UIntPtr
#undef Int
#undef Long
#undef ULong

extern Boolean mathLibOK;

void  SysTrapFlpLToF(FlpDouble*, Int32) SYS_TRAP(sysTrapFlpEmDispatch);
Int32 SysTrapFlpFToL(FlpDouble) SYS_TRAP(sysTrapFlpEmDispatch);
void  SysTrapBinOp(FlpDouble*, FlpDouble, FlpDouble) SYS_TRAP(sysTrapFlpEmDispatch);
Int32 SysTrapCompare(FlpDouble, FlpDouble) SYS_TRAP(sysTrapFlpEmDispatch);

double longToDouble(long l);
long doubleToLong(double d);
double genericDoubleOp(double a, double b, long opcode);

#define addDouble(a,b) genericDoubleOp(a,b,sysFloatEm_d_add)
#define subDouble(a,b) genericDoubleOp(a,b,sysFloatEm_d_sub)
#define mulDouble(a,b) genericDoubleOp(a,b,sysFloatEm_d_mul)
#define divDouble(a,b) genericDoubleOp(a,b,sysFloatEm_d_div)

Boolean eqDouble(double a, double b);
Boolean leqDouble(double a, double b);

Boolean isInf(FlpCompDouble fcd);
Boolean isNan(FlpCompDouble fcd);

void LMacos(double x, double* result);
void LMasin(double x, double* result);
void LMatan(double x, double* result);
void LMatan2(double y, double x, double* result);
void LMcos(double x, double* result);
void LMsin(double x, double* result);
void LMtan(double x, double* result);
void LMcosh(double x, double* result);
void LMsinh(double x, double* result);
void LMtanh(double x, double* result);
void LMacosh(double x, double* result);
void LMasinh(double x, double* result);
void LMatanh(double x, double* result);
void LMexp(double x, double* result);
void LMlog(double x, double* result);
void LMsqrt(double x, double* result);
void LMceil(double x, double* result);
void LMfloor(double x, double* result);
void LMround(double x, double* result);
void LMtrunc(double x, double* result);

#endif
