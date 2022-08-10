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
/* 10.10.1997 New                                                FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"
#include "LispMe.h"
#include "fpstuff.h"
#include "vm.h"
#include "cplx.h"
#include "util.h"

/**********************************************************************/
/* Helper functions                                                   */
/**********************************************************************/
static double mag(double re, double im)
{
  double res = addDouble(mulDouble(re,re),mulDouble(im,im));
  double res1;
  LMsqrt(res,&res1);
  return res1;
}

static void divHlp(double ar, double ai, double br, double bi,
                   double* cr, double* ci)
{
  double z = addDouble(mulDouble(br,br),mulDouble(bi,bi));
  *cr = divDouble(addDouble(mulDouble(ar,br),mulDouble(ai,bi)),z);
  *ci = divDouble(subDouble(mulDouble(ai,br),mulDouble(ar,bi)),z);
}

static void logHlp(double ar, double ai, double* br, double* bi)
{
  double res = mag(ar,ai);
  LMlog(res,br);
  LMatan2(ai,ar,bi);
}

static void sqrtHlp(double ar, double ai, double* br, double* bi)
{
  double r1,r2,s,d;
  if (FlpIsZero(ar) && FlpIsZero(ai))
    *br = *bi = ar;
  else
  {
    r1 = ar;
    FlpSetPositive(r1);
    r2 = mulDouble(0.5,addDouble(mag(ar,ai),r1));
    LMsqrt(r2,&s);
    d = mulDouble(0.5,divDouble(ai,s));
    if (!leqDouble(ar,0.0))
    {
      *br = s; *bi = d;
    }
    else
    {
      if (!leqDouble(0.0,ai))
      {
        FlpNegate(d);
        FlpNegate(s);
      }
      *br = d; *bi = s;
    }
  }
}

/**********************************************************************/
/* Basic operations                                                   */
/**********************************************************************/
PTR addCpl(double ar, double ai, double br, double bi)
{
  return storeNum(addDouble(ar,br),addDouble(ai,bi));
}

PTR subCpl(double ar, double ai, double br, double bi)
{
  return storeNum(subDouble(ar,br),subDouble(ai,bi));
}

PTR mulCpl(double ar, double ai, double br, double bi)
{
  return storeNum(subDouble(mulDouble(ar,br),mulDouble(ai,bi)),
                  addDouble(mulDouble(ar,bi),mulDouble(ai,br)));
}

PTR divCpl(double ar, double ai, double br, double bi)
{
  double cr,ci;
  divHlp(ar,ai,br,bi,&cr,&ci);
  return storeNum(cr,ci);
}

/**********************************************************************/
/* Magnitude and angle                                                */
/**********************************************************************/
PTR magnitude(PTR p)
{
  if (IS_INT(p))
    if (INTVAL(p) < 0)
      return makeNum(-INTVAL(p));
    else
      return p;
  else if (IS_REAL(p))
    if (leqDouble(0.0,getReal(p)))
      return p;
    else
      return allocReal(subDouble(0.0,getReal(p)));
  else if (IS_COMPLEX(p))
  {
    double a = getReal(cadr(p));
    double b = getReal(cddr(p));
    if (!mathLibOK)
      ErrThrow(ERR_R12_NO_MATHLIB);
    return allocReal(mag(a,b));
  }
  else
    typeError(p,"number");
}

PTR angle(PTR p)
{
  if (IS_INT(p))
    if (INTVAL(p) < 0)
      return allocReal(PI);
    else
      return MKINT(0);
  else if (IS_REAL(p))
    if (leqDouble(0.0,getReal(p)))
      return MKINT(0);
    else
      return allocReal(PI);
  else if (IS_COMPLEX(p))
  {
    double a = getReal(cadr(p));
    double b = getReal(cddr(p));
    double c;
    if (!mathLibOK)
      ErrThrow(ERR_R12_NO_MATHLIB);
    LMatan2(b,a,&c);
    return allocReal(c);
  }
  else
    typeError(p,"number");
}

/**********************************************************************/
/* exp, log and sqrt                                                  */
/**********************************************************************/
PTR cplExp(double re, double im)
{
  double res;
  LMexp(re,&res);
  if (FlpIsZero(im))
    return allocReal(res);
  else
    return makePolar(res,im);
}

PTR cplLog(double re, double im)
{
  double res,res1;
  if (FlpIsZero(im))
  {
    if (FlpGetSign(re))
    {
      FlpSetPositive(re);
      LMlog(re,&res);
      return storeNum(res,PI);
    }
    else
    {
      LMlog(re,&res);
      return allocReal(res);
    }
  }
  else
  {
    logHlp(re,im,&res1,&res);
    return storeNum(res1,res);
  }
}

PTR cplSqrt(double re, double im)
{
  double res,res1;
  if (FlpIsZero(im))
  {
    if (FlpGetSign(re))
    {
      FlpSetPositive(re);
      LMsqrt(re,&res);
      return storeNum(0.0,res);
    }
    else
    {
      LMsqrt(re,&res);
      return allocReal(res);
    }
  }
  else
  {
    sqrtHlp(re,im,&res1,&res);
    return storeNum(res1,res);
  }
}

/**********************************************************************/
/* sin, cos, sinh, cosh                                               */
/**********************************************************************/
PTR cplSin(double re, double im)
{
  double res,res1,res2,res3;
  LMsin(re,&res);
  if (FlpIsZero(im))
    return allocReal(res);
  else
  {
    LMcosh(im,&res1);
    LMcos(re,&res2);
    LMsinh(im,&res3);
    return storeNum(mulDouble(res,res1),mulDouble(res2,res3));
  }
}

PTR cplCos(double re, double im)
{
  double res,res1,res2,res3;
  LMcos(re,&res);
  if (FlpIsZero(im))
    return allocReal(res);
  else
  {
    LMcosh(im,&res1);
    LMsin(re,&res2);
    LMsinh(im,&res3);
    FlpNegate(res3);
    return storeNum(mulDouble(res,res1),mulDouble(res2,res3));
  }
}

PTR cplSinh(double re, double im)
{
  double res,res1,res2,res3;
  LMsinh(re,&res);
  if (FlpIsZero(im))
    return allocReal(res);
  else
  {
    LMcos(im,&res1);
    LMcosh(re,&res2);
    LMsin(im,&res3);
    return storeNum(mulDouble(res,res1),mulDouble(res2,res3));
  }
}

PTR cplCosh(double re, double im)
{
  double res,res1,res2,res3;
  LMcosh(re,&res);
  if (FlpIsZero(im))
    return allocReal(res);
  else
  {
    LMcos(im,&res1);
    LMsinh(re,&res2);
    LMsin(im,&res3);
    return storeNum(mulDouble(res,res1),mulDouble(res2,res3));
  }
}

/**********************************************************************/
/* asin, asinh, acos, acosh                                           */
/**********************************************************************/
PTR cplAsin(double re, double im)
{
  double res,r1,r2,r3,r4;
  Boolean negate = false;
  if (FlpIsZero(im) && leqDouble(-1.0,re) && leqDouble(re,1.0))
  {
    LMasin(re,&res);
    return allocReal(res);
  }
  else
  {
    /*----------------------------------------------------------------*/
    /* Check dangerous case: z=iy for large y, avoid cancellation by  */
    /* symmetry asin(z) = -asin(-z) (stay out off Q1 and Q2           */
    /*----------------------------------------------------------------*/
    if ((!FlpGetSign(im) && !FlpIsZero(im)) ||
        (FlpIsZero(im)   && FlpGetSign(re)))
    {
      FlpNegate(re);
      FlpNegate(im);
      negate = true;
    }
    r1 = mulDouble(re,re);
    r2 = mulDouble(im,im);
    sqrtHlp(subDouble(1.0,subDouble(r1,r2)),
            mulDouble(-2.0,mulDouble(re,im)),
            &r3,&r4);
    logHlp(subDouble(r3,im),addDouble(re,r4),&r1,&r2);
    if (negate)
      FlpNegate(r2);
    else
      FlpNegate(r1);
    return storeNum(r2,r1);
  }
}

PTR cplAsinh(double re, double im)
{
  double res,r1,r2,r3,r4;
  Boolean negate = false;
  if (FlpIsZero(im))
  {
    LMasinh(re,&res);
    return allocReal(res);
  }
  else
  {
    /*----------------------------------------------------------------*/
    /* Check dangerous case: z=iy for large y, avoid cancellation by  */
    /* symmetry asinh(z) = -asinh(-z) (stay out of Q1 and Q4)         */
    /*----------------------------------------------------------------*/
    if ((!FlpGetSign(re) && !FlpIsZero(re)) ||
        (FlpIsZero(re)   && !FlpGetSign(im)))
    {
      FlpNegate(re);
      FlpNegate(im);
      negate = true;
    }
    r1 = mulDouble(re,re);
    r2 = mulDouble(im,im);
    sqrtHlp(addDouble(subDouble(r1,r2),1.0),
            mulDouble(2.0,mulDouble(re,im)),
            &r3,&r4);
    logHlp(subDouble(r3,re),subDouble(r4,im),&r1,&r2);
    if (!negate)
    {
      FlpNegate(r1);
      FlpNegate(r2);
    }
    return storeNum(r1,r2);
  }
}

PTR cplAcos(double re, double im)
{
  double res,r1,r2,r3,r4;
  Boolean negate = false;
  Boolean dropReal = false;
  if (FlpIsZero(im) && leqDouble(-1.0,re) && leqDouble(re,1.0))
  {
    LMacos(re,&res);
    return allocReal(res);
  }
  else
  {
    if (FlpIsZero(im) && !FlpGetSign(re))
    {
      FlpNegate(re);
      negate = dropReal = true;
    }
    else if (FlpGetSign(im))
    {
      FlpNegate(im);
      negate = true;
    }
    r1 = mulDouble(re,re);
    r2 = mulDouble(im,im);
    sqrtHlp(subDouble(1.0,subDouble(r1,r2)),
            mulDouble(-2.0,mulDouble(re,im)),
            &r3,&r4);
    logHlp(subDouble(re,r4),addDouble(im,r3),&r1,&r2);
    if (!negate)
      FlpNegate(r1);
    return storeNum(dropReal?0.0:r2,r1);
  }
}

PTR cplAcosh(double re, double im)
{
  double res,r1,r2,r3,r4;
  Boolean negate = false;
  if (FlpIsZero(im) && leqDouble(1.0,re))
  {
    LMacosh(re,&res);
    return allocReal(res);
  }
  else
  {
    if (FlpGetSign(im))
    {
      FlpNegate(im);
      negate = true;
    }
    r1 = mulDouble(re,re);
    r2 = mulDouble(im,im);
    sqrtHlp(subDouble(1.0,subDouble(r1,r2)),
            mulDouble(-2.0,mulDouble(re,im)),
            &r3,&r4);
    logHlp(subDouble(re,r4),addDouble(im,r3),&r1,&r2);
    if (negate)
      FlpNegate(r1);
    return storeNum(r1,r2);
  }
}

/**********************************************************************/
/* tan, tanh, atan, atanh                                             */
/**********************************************************************/
PTR cplTan(double re, double im)
{
  double res,res1,res2,res3;
  if (FlpIsZero(im))
  {
    LMtan(re,&res);
    return allocReal(res);
  }
  else
  {
    re = mulDouble(re,2.0);
    im = mulDouble(im,2.0);
    LMsin(re,&res);
    LMcos(re,&res1);
    LMsinh(im,&res2);
    LMcosh(im,&res3);
    re = addDouble(res1,res3);
    return storeNum(divDouble(res,re),divDouble(res2,re));
  }
}

PTR cplTanh(double re, double im)
{
  double res,res1,res2,res3;
  if (FlpIsZero(im))
  {
    LMtanh(re,&res);
    return allocReal(res);
  }
  else
  {
    re = mulDouble(re,2.0);
    im = mulDouble(im,2.0);
    LMsinh(re,&res);
    LMcosh(re,&res1);
    LMsin(im,&res2);
    LMcos(im,&res3);
    re = addDouble(res1,res3);
    return storeNum(divDouble(res,re),divDouble(res2,re));
  }
}

PTR cplAtan(double re, double im)
{
  double res,res1,res2,res3;
  if (FlpIsZero(im))
  {
    LMatan(re,&res);
    return allocReal(res);
  }
  else
  {
    divHlp(re,               addDouble(1.0,im),
           subDouble(0.0,re),subDouble(1.0,im),
           &res,&res1);
    logHlp(res,res1,&res2,&res3);
    return storeNum(mulDouble(res3,-0.5),mulDouble(res2,0.5));
  }
}

PTR cplAtanh(double re, double im)
{
  double res,res1,res2,res3;
  if (FlpIsZero(im) && leqDouble(-1.0,re) && leqDouble(re,1.0))
  {
    LMatanh(re,&res);
    return allocReal(res);
  }
  else
  {
    divHlp(addDouble(1.0,re),im,
           subDouble(1.0,re),subDouble(0.0,im),
           &res,&res1);
    logHlp(res,res1,&res2,&res3);
    return storeNum(mulDouble(res2,0.5),mulDouble(res3,0.5));
  }
}
