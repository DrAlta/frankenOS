/**********************************************************************/
/*                                                                    */
/* fpstuff.c: LISPME flaoting point operations and MathLib glue code  */
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

#include "store.h"
#include "fpstuff.h"

/**********************************************************************/
/* Global data                                                        */
/**********************************************************************/
UInt16  MathLibRef;
Boolean mathLibOK = false;

/**********************************************************************/
/* Exported functions                                                 */
/**********************************************************************/
double longToDouble(long l)
{
  FlpCompDouble fcd;
  asm("moveq.l %0,%%d2" : : "i" (sysFloatEm_d_itod) : "d2");
  SysTrapFlpLToF(&fcd.fd, l);
  return fcd.d;
}

long doubleToLong(double d)
{
  FlpCompDouble fcd;
  fcd.d = d;
  asm("moveq.l %0,%%d2" : : "i" (sysFloatEm_d_dtoi) : "d2");
  return SysTrapFlpFToL(fcd.fd);
}

double genericDoubleOp(double a, double b, long opcode)
{
  FlpCompDouble fcda, fcdb, fcds;
  fcda.d = a; fcdb.d = b;
  asm("move.l %0,%%d2" : : "g" (opcode) : "d2");
  SysTrapBinOp(&fcds.fd, fcda.fd, fcdb.fd);
  return fcds.d;
}

Boolean eqDouble(double a, double b)
{
  FlpCompDouble fcda, fcdb;
  fcda.d = a; fcdb.d = b;
  asm("moveq.l %0,%%d2" : : "i" (sysFloatEm_d_feq) : "d2");
  return SysTrapCompare(fcda.fd, fcdb.fd);
}

Boolean leqDouble(double a, double b)
{
  FlpCompDouble fcda, fcdb;
  fcda.d = a; fcdb.d = b;
  asm("moveq.l %0,%%d2" : : "i" (sysFloatEm_d_fle) : "d2");
  return SysTrapCompare(fcda.fd, fcdb.fd);
}

Boolean isInf(FlpCompDouble fcd)
{
  return (fcd.ul[0] & 0x7ff00000) == 0x7ff00000 &&
          fcd.fdb.manH == 0 &&
          fcd.fdb.manL == 0;
}

Boolean isNan(FlpCompDouble fcd)
{
  return (fcd.ul[0] & 0x7ff00000) == 0x7ff00000 &&
          (fcd.fdb.manH != 0 ||
           fcd.fdb.manL != 0);
}

/**********************************************************************/
/* Wrapper functions for MathLib to be used as function pointers      */
/**********************************************************************/
void LMacos(double x, double* result)
{
  MathLibACos(MathLibRef, x, result);
}

void LMasin(double x, double* result)
{
  MathLibASin(MathLibRef, x, result);
}

void LMatan(double x, double* result)
{
  MathLibATan(MathLibRef, x, result);
}

void LMatan2(double y, double x, double* result)
{
  MathLibATan2(MathLibRef, y, x, result);
}

void LMcos(double x, double* result)
{
  MathLibCos(MathLibRef, x, result);
}

void LMsin(double x, double* result)
{
  MathLibSin(MathLibRef, x, result);
}

void LMtan(double x, double* result)
{
  MathLibTan(MathLibRef, x, result);
}

void LMcosh(double x, double* result)
{
  MathLibCosH(MathLibRef, x, result);
}

void LMsinh(double x, double* result)
{
  MathLibSinH(MathLibRef, x, result);
}

void LMtanh(double x, double* result)
{
  MathLibTanH(MathLibRef, x, result);
}

void LMacosh(double x, double* result)
{
  MathLibACosH(MathLibRef, x, result);
}

void LMasinh(double x, double* result)
{
  MathLibASinH(MathLibRef, x, result);
}

void LMatanh(double x, double* result)
{
  MathLibATanH(MathLibRef, x, result);
}

void LMexp(double x, double* result)
{
  MathLibExp(MathLibRef, x, result);
}

void LMlog(double x, double* result)
{
  MathLibLog(MathLibRef, x, result);
}

void LMsqrt(double x, double* result)
{
  MathLibSqrt(MathLibRef, x, result);
}

void LMceil(double x, double* result)
{
  MathLibCeil(MathLibRef, x, result);
}

void LMfloor(double x, double* result)
{
  MathLibFloor(MathLibRef, x, result);
}

void LMround(double x, double* result)
{
  MathLibRound(MathLibRef, x, result);
}

void LMtrunc(double x, double* result)
{
  MathLibTrunc(MathLibRef, x, result);
}
