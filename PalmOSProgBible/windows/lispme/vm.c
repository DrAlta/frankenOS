/**********************************************************************/
/*                                                                    */
/* vm.c: LISPME Virtual Machine                                       */
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
/* 20.07.1997 New                                                FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "vm.h"
#include "io.h"
#include "comp.h"
#include "LispMe.h"
#include "fpstuff.h"
#include "cplx.h"
#include "file.h"
#include "gui.h"
#include "util.h"
#include "hbase.h"
#include "graphic.h"

/**********************************************************************/
/* Global data                                                        */
/**********************************************************************/
int     stepsInSlice;
Boolean evalMacro;

/**********************************************************************/
/* Static functions                                                   */
/**********************************************************************/
static long addInt(long a, long b) SEC(VM);
static long subInt(long a, long b) SEC(VM);
static long mulInt(long a, long b) SEC(VM);
static PTR  div(PTR a, PTR b)      SEC(VM);
static PTR  genBinOp(PTR a,
                     PTR b,
                     long (*intOp)(long,long),
                     long floatOp,
                     PTR  (*cplOp)(double,double,double,double)) SEC(VM);
static int eqv(PTR a, PTR b)     SEC(VM);
static int strComp(PTR a, PTR b) SEC(VM);
static int leq(PTR a, PTR b)     SEC(VM);
static Boolean member(PTR el, PTR l) SEC(VM);
static double  realVal(PTR p) SEC(VM);
static void    transUnary(void (*fn)(double,double*)) SEC(VM);
static void    transCplUnary(PTR (*fn)(double,double)) SEC(VM);
static void    transBinary(void (*fn)(double,double,double*)) SEC(VM);
static void    application(Boolean copyArgs) SEC(VM);
static void    vectorAcc(PTR* obj, PTR vec, PTR n, Boolean write) SEC(VM);
static void    stringAcc(PTR* c, PTR str, PTR n, Boolean write) SEC(VM);
static short   vectorLength(PTR v) SEC(VM);
static void    sound(Int16 freq, Int16 dur) SEC(VM);


/**********************************************************************/
/* Make Lisp type for a long number                                   */
/**********************************************************************/
PTR makeNum(long n)
{
  if (-16384<=n && n<=16383)
    return MKINT(n);
  else
    return allocReal(longToDouble(n));
}

/**********************************************************************/
/* Integer arithmetic                                                 */
/**********************************************************************/
static long addInt(long a, long b) {return a+b;};
static long subInt(long a, long b) {return a-b;};
static long mulInt(long a, long b) {return a*b;};

/**********************************************************************/
/* generic operaton with two arbitrary numbers                        */
/**********************************************************************/
static PTR genBinOp(PTR a,
                    PTR b,
                    long (*intOp)(long,long),
                    long floatOp,
                    PTR  (*cplOp)(double,double,double,double))
{
  if (IS_INT(a))
  {
    if (IS_INT(b))
      return makeNum(intOp(INTVAL(a),INTVAL(b)));
    else if (IS_REAL(b))
      return allocReal(genericDoubleOp(longToDouble(INTVAL(a)),getReal(b),floatOp));
    else if (IS_COMPLEX(b))
      return cplOp(longToDouble(INTVAL(a)),0.0,getReal(cadr(b)),getReal(cddr(b)));
    else
      typeError(b,"number");
  }
  else if (IS_REAL(a))
  {
    if (IS_INT(b))
      return allocReal(genericDoubleOp(getReal(a),longToDouble(INTVAL(b)),floatOp));
    else if (IS_REAL(b))
      return allocReal(genericDoubleOp(getReal(a),getReal(b),floatOp));
    else if (IS_COMPLEX(b))
      return cplOp(getReal(a),0.0,getReal(cadr(b)),getReal(cddr(b)));
    else
      typeError(b,"number");
  }
  else if (IS_COMPLEX(a))
  {
    if (IS_INT(b))
      return cplOp(getReal(cadr(a)),getReal(cddr(a)),longToDouble(INTVAL(b)),0.0);
    else if (IS_REAL(b))
      return cplOp(getReal(cadr(a)),getReal(cddr(a)),getReal(b),0.0);
    else if (IS_COMPLEX(b))
      return cplOp(getReal(cadr(a)),getReal(cddr(a)),getReal(cadr(b)),getReal(cddr(b)));
    else
      typeError(b,"number");
  }
  else
    typeError(a,"number");
}

/**********************************************************************/
/* divide two arbitrary numbers                                       */
/**********************************************************************/
static PTR div(PTR a, PTR b)
{
  if (IS_INT(a))
  {
    if (IS_INT(b))
    {
      if (INTVAL(b) == 0)
        ErrThrow(ERR_R8_DIV_BY_ZERO);
      {
        long a1 = INTVAL(a);
        long b1 = INTVAL(b);
        if (a1%b1 == 0)
          return makeNum(a1/b1);
        else
          return allocReal(divDouble(longToDouble(a1),longToDouble(b1)));
      }
    }
    else if (IS_REAL(b))
      if (eqDouble(getReal(b), 0.0))
        ErrThrow(ERR_R8_DIV_BY_ZERO);
      else
        return allocReal(divDouble(longToDouble(INTVAL(a)),getReal(b)));
    else if (IS_COMPLEX(b))
      return divCpl(longToDouble(INTVAL(a)),0.0,getReal(cadr(b)),getReal(cddr(b)));
    else
      typeError(b,"number");
  }
  else if (IS_REAL(a))
  {
    if (IS_INT(b))
      if (INTVAL(b) == 0)
        ErrThrow(ERR_R8_DIV_BY_ZERO);
      else
        return allocReal(divDouble(getReal(a),longToDouble(INTVAL(b))));
    else if (IS_REAL(b))
      if (eqDouble(getReal(b), 0.0))
        ErrThrow(ERR_R8_DIV_BY_ZERO);
      else
        return allocReal(divDouble(getReal(a),getReal(b)));
    else if (IS_COMPLEX(b))
      return divCpl(getReal(a),0.0,getReal(cadr(b)),getReal(cddr(b)));
    else
      typeError(b,"number");
  }
  else if (IS_COMPLEX(a))
  {
    if (IS_INT(b))
      if (INTVAL(b) == 0)
        ErrThrow(ERR_R8_DIV_BY_ZERO);
      else
        return divCpl(getReal(cadr(a)),getReal(cddr(a)),longToDouble(INTVAL(b)),0.0);
    else if (IS_REAL(b))
      if (eqDouble(getReal(b), 0.0))
        ErrThrow(ERR_R8_DIV_BY_ZERO);
      else
        return divCpl(getReal(cadr(a)),getReal(cddr(a)),getReal(b),0.0);
    else if (IS_COMPLEX(b))
      return divCpl(getReal(cadr(a)),getReal(cddr(a)),getReal(cadr(b)),getReal(cddr(b)));
    else
      typeError(b,"number");
  }
  else
    typeError(a,"number");
}

/**********************************************************************/
/* compare equivalence (works for all numbers)                        */
/**********************************************************************/
static int eqv(PTR a, PTR b)
{
  if (a == b)
    return 1;
  else if (IS_INT(a))
  {
    if (IS_REAL(b))
      return eqDouble(longToDouble(INTVAL(a)),getReal(b));
    else
      return 0;
  }
  else if (IS_REAL(a))
  {
    if (IS_INT(b))
      return eqDouble(getReal(a), longToDouble(INTVAL(b)));
    else if (IS_REAL(b))
      return eqDouble(getReal(a), getReal(b));
    else
      return 0;
  }
  else if (IS_COMPLEX(a) && IS_COMPLEX(b))
    return eqDouble(getReal(cadr(a)),getReal(cadr(b))) &&
           eqDouble(getReal(cddr(a)),getReal(cddr(b)));
  else
    return 0;
}

/**********************************************************************/
/* compare strings                                                    */
/**********************************************************************/
static int strComp(PTR a, PTR b)
{
  UInt16    index1, index2;
  MemHandle recHand1, recHand2;
  int       len1,len2,i,d;
  char      *p1, *p2;

  if (a==EMPTY_STR && b==EMPTY_STR)  return 0;
  if (a==EMPTY_STR)                  return -1;
  if (b==EMPTY_STR)                  return 1;

  DmFindRecordByID(dbRef, UID_STR(a), &index1);
  recHand1 = DmQueryRecord(dbRef,index1);
  p1 = MemHandleLock(recHand1);
  len1 = MemHandleSize(recHand1);
  DmFindRecordByID(dbRef, UID_STR(b), &index2);
  recHand2 = DmQueryRecord(dbRef,index2);
  p2 = MemHandleLock(recHand2);
  len2 = MemHandleSize(recHand2);
  for (i=0;i<min(len1,len2);++i)
  {
    d = *p1++ - *p2++;
    if (d)
    {
      MemHandleUnlock(recHand1);
      MemHandleUnlock(recHand2);
      return d;
    }
  }
  MemHandleUnlock(recHand1);
  MemHandleUnlock(recHand2);
  return i==len1 ? (i==len2 ? 0 : -1) : 1;
}

/**********************************************************************/
/* compare two arbitrary numbers, chars or strings                    */
/**********************************************************************/
static int leq(PTR a, PTR b)
{
  if (IS_INT(a))
  {
    if (IS_INT(b))
      return INTVAL(a) <= INTVAL(b);
    else if (IS_REAL(b))
      return leqDouble(longToDouble(INTVAL(a)), getReal(b));
    else
      ErrThrow(ERR_R14_INVALID_COMP);
  }
  else if (IS_REAL(a))
  {
    if (IS_INT(b))
      return leqDouble(getReal(a), longToDouble(INTVAL(b)));
    else if (IS_REAL(b))
      return leqDouble(getReal(a), getReal(b));
    else
      ErrThrow(ERR_R14_INVALID_COMP);
  }
  else if (IS_CHAR(a))
  {
    if (IS_CHAR(b))
      return CHARVAL(a) <= CHARVAL(b);
    else
      ErrThrow(ERR_R14_INVALID_COMP);
  }
  else if (IS_STRING(a))
  {
    if (IS_STRING(b))
      return strComp(a,b) <= 0;
    else
      ErrThrow(ERR_R14_INVALID_COMP);
  }
  else
    ErrThrow(ERR_R14_INVALID_COMP);
}

/**********************************************************************/
/* Check, if el is in list l (using eqv?)                             */
/**********************************************************************/
static Boolean member(PTR el, PTR l)
{
  PTR   p = l;
  while (IS_PAIR(p))
  {
    if (eqv(el,car(p))) return true;
    p = cdr(p);
  }
  if (p!=NIL)
    error1(ERR_C6_IMPROPER_ARGS, l);
  return false;
}

/**********************************************************************/
/* Make a real from int or real                                       */
/**********************************************************************/
static double realVal(PTR p)
{
  if (IS_INT(p))
    return longToDouble(INTVAL(p));
  else if (IS_REAL(p))
    return getReal(p);
  else
    typeError(p,"real");
}

/**********************************************************************/
/* Transcendental functions                                           */
/**********************************************************************/
static void transUnary(void (*fn)(double,double*))
{
  /*------------------------------------------------------------------*/
  /* (x.s) e (FKT.c) d --> (fkt(x).s) e c d                           */
  /*------------------------------------------------------------------*/
  double res;

  if (!mathLibOK)
    ErrThrow(ERR_R12_NO_MATHLIB);
  fn(realVal(car(S)),&res);
  S = cons(allocReal(res), cdr(S));
  C = cdr(C);
}

static void transCplUnary(PTR (*fn)(double,double))
{
  /*------------------------------------------------------------------*/
  /* (x.s) e (FKT.c) d --> (fkt(x).s) e c d                           */
  /*------------------------------------------------------------------*/
  W = car(S);
  if (!mathLibOK)
    ErrThrow(ERR_R12_NO_MATHLIB);
  if (IS_INT(W))
    S = cons(fn(longToDouble(INTVAL(W)),0.0), cdr(S));
  else if (IS_REAL(W))
    S = cons(fn(getReal(W),0), cdr(S));
  else if (IS_COMPLEX(W))
    S = cons(fn(getReal(cadr(W)),
                 getReal(cddr(W))),
              cdr(S));
  else
    typeError(W,"number");
  C = cdr(C);
}

static void transBinary(void (*fn)(double,double,double*))
{
  /*------------------------------------------------------------------*/
  /* (x y.s) e (FKT.c) d --> (fkt(x,y).s) e c d                       */
  /*------------------------------------------------------------------*/
  double res;

  if (!mathLibOK)
    ErrThrow(ERR_R12_NO_MATHLIB);
  fn(realVal(car(S)),realVal(cadr(S)),&res);
  S = cons(allocReal(res), cddr(S));
  C = cdr(C);
}

/**********************************************************************/
/* Store a number, reducing it to real if possible                    */
/**********************************************************************/
PTR storeNum(double re, double im)
{
  FlpCompDouble fcd;
  fcd.d = im;
  if (FlpIsZero(fcd))
    return allocReal(re);
  else
    return cons(CPLX_TAG, cons(W=allocReal(re), W=allocReal(im)));
}

/**********************************************************************/
/* Make complex number from polar representation                      */
/**********************************************************************/
PTR makePolar(double mag, double ang)
{
  double x,y;
  if (!mathLibOK)
    ErrThrow(ERR_R12_NO_MATHLIB);
  LMcos(ang,&x);
  LMsin(ang,&y);
  return storeNum(mulDouble(mag,x),mulDouble(mag,y));
}

/**********************************************************************/
/* Play a sound                                                       */
/**********************************************************************/
static void sound(Int16 freq, Int16 dur)
{
  struct SndCommandType cmd;
  if (dur < 0)
    parmError(MKINT(dur),"sound");
  if (freq < 128)
    parmError(MKINT(freq),"sound");
  cmd.cmd    = sndCmdFreqDurationAmp;
  cmd.param1 = freq;
  cmd.param2 = dur;
  cmd.param3 = sndMaxAmp;
  SndDoCmd((void*)0, &cmd, false);
  YIELD();
}

/**********************************************************************/
/* common code for APC and TAPC                                       */
/**********************************************************************/
static void application(Boolean copyArgs)
{
  /*------------------------------------------------------------------*/
  /* apply closure: first check number of arguments                   */
  /*------------------------------------------------------------------*/
  short len = INTVAL(cadar(S));
  short i;

  /*------------------------------------------------------------------*/
  /* have to copy args when applying to avoid destruction of org list */
  /*------------------------------------------------------------------*/
  if (copyArgs)
  {
    W = cadr(S);
    listCopy(&cadr(S),cadr(S),32767);
  }

  /*------------------------------------------------------------------*/
  /* check number of arguments                                        */
  /*------------------------------------------------------------------*/
  if (len >= 0)
  {
    if (len != listLength(cadr(S)))
    {
      expectedLen = len;
      error1(ERR_R3_NUM_ARGS,cadr(S));
    }
  }
  else
  {
    /*----------------------------------------------------------------*/
    /* variable number of args, modify arg list                       */
    /*----------------------------------------------------------------*/
    PTR* ptr;
    if (-len-1 > listLength(cadr(S)))
    {
      expectedLen = len+1;
      error1(ERR_R3_NUM_ARGS,cadr(S));
    }
    for (ptr=&cadr(S),i=0;i<-len-1;i++)
      ptr = &cdr(*ptr);
    *ptr = cons(*ptr,NIL);
  }
}

/**********************************************************************/
/* Virtual machine (assume code is already loaded into C register)    */
/**********************************************************************/
PTR exec(void)
{
  PTR loc;
  int i,k;

  if (LispMePrefs.noAutoOff)
    EvtResetAutoOffTimer();

  for (stepsInSlice=0; stepsInSlice<STEPS_PER_TIMESLICE; ++stepsInSlice)
  {
    ++numStep;
    switch (INTVAL(car(C)))
    {
      case EVA:
        /*------------------------------------------------------------*/
        /* (ex.s) e (EVA na.c) d --> NIL e compile(ex,na) (s e c.d)   */
        /*------------------------------------------------------------*/
        D = cons(cdr(S),cons(E,cons(cddr(C),D)));
        C = compile(car(S),cadr(C));
        S = NIL;
        break;

      case SQRT: transCplUnary(&cplSqrt);  break;
      case EXP:  transCplUnary(&cplExp);   break;
      case LOG:  transCplUnary(&cplLog);   break;
      case SIN:  transCplUnary(&cplSin);   break;
      case COS:  transCplUnary(&cplCos);   break;
      case TAN:  transCplUnary(&cplTan);   break;
      case SINH: transCplUnary(&cplSinh);  break;
      case COSH: transCplUnary(&cplCosh);  break;
      case TANH: transCplUnary(&cplTanh);  break;
      case ASIN: transCplUnary(&cplAsin);  break;
      case ACOS: transCplUnary(&cplAcos);  break;
      case ATAN: transCplUnary(&cplAtan);  break;
      case ASIH: transCplUnary(&cplAsinh); break;
      case ACOH: transCplUnary(&cplAcosh); break;
      case ATAH: transCplUnary(&cplAtanh); break;
      case ATN2: transBinary(&LMatan2);    break;
      case FLOR: transUnary(&LMfloor);     break;
      case CEIL: transUnary(&LMceil);      break;
      case TRUN: transUnary(&LMtrunc);     break;
      case ROUN: transUnary(&LMround);     break;

      case IDIV:
        /*------------------------------------------------------------*/
        /* (a b.s) e (IDIV.c) d --> ([b/a].s) e c d                   */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        checkInt(cadr(S));
        if (INTVAL(cadr(S)) == 0)
          ErrThrow(ERR_R8_DIV_BY_ZERO);
        S = cons(MKINT((INTVAL(car(S))) / (INTVAL(cadr(S)))), cddr(S));
        goto skip1;

      case INTG:
        /*------------------------------------------------------------*/
        /* (x.s) e (INTG.c) d --> (int(x).s) e c d                    */
        /*------------------------------------------------------------*/
        if (IS_INT(car(S)))
        { /* nothing to do */ }
        else if (IS_REAL(car(S)))
        {
          Int32 res = doubleToLong(getReal(car(S)));
          if (res < -16384 || res > 16383)
            typeError(car(S),"integer");
          S = cons(MKINT(res),cdr(S));
        }
        else
          typeError(car(S),"real");
        goto skip1;

      case CPLP:
        /*------------------------------------------------------------*/
        /* (x.s) e (CPLP.c) d --> (#t.s) e c d if x is complex        */
        /* (a.s) e (CPLP.c) d --> (#f.s) e c d                        */
        /*------------------------------------------------------------*/
        S = cons(IS_INT(car(S)) || IS_REAL(car(S)) || IS_COMPLEX(car(S))
                  ? TRUE : FALSE, cdr(S));
        goto skip1;

      case REAP:
        /*------------------------------------------------------------*/
        /* (x.s) e (REAP.c) d --> (#t.s) e c d if x is real           */
        /* (a.s) e (REAP.c) d --> (#f.s) e c d                        */
        /*------------------------------------------------------------*/
        S = cons(IS_INT(car(S)) || IS_REAL(car(S))
                  ? TRUE : FALSE, cdr(S));
        goto skip1;

      case MKRA:
        /*------------------------------------------------------------*/
        /* (a b.s) e (MKRA.c) d --> (complex(a,b).s) e c d            */
        /*------------------------------------------------------------*/
        S = cons(storeNum(realVal(car(S)),realVal(cadr(S))), cddr(S));
        goto skip1;

      case MKPO:
        /*------------------------------------------------------------*/
        /* (a b.s) e (MKPO.c) d --> (polar(a,b).s) e c d              */
        /*------------------------------------------------------------*/
        S = cons(makePolar(realVal(car(S)),realVal(cadr(S))), cddr(S));
        goto skip1;

      case REPA:
        /*------------------------------------------------------------*/
        /* (z.s) e (REPA.c) d --> (re(z).s) e c d                     */
        /*------------------------------------------------------------*/
        if (IS_COMPLEX(car(S)))
          S = cons(cadar(S),cdr(S));
        else if (IS_REAL(car(S)) || IS_INT(car(S)))
          {/* nothing to do! */}
        else
          typeError(car(S),"number");
        goto skip1;

      case IMPA:
        /*------------------------------------------------------------*/
        /* (z.s) e (IMPA.c) d --> (im(z).s) e c d                     */
        /*------------------------------------------------------------*/
        if (IS_COMPLEX(car(S)))
          S = cons(cddar(S),cdr(S));
        else if (IS_REAL(car(S)) || IS_INT(car(S)))
          S = cons(MKINT(0),cdr(S));
        else
          typeError(car(S),"number");
        goto skip1;

      case MAGN:
        /*------------------------------------------------------------*/
        /* (z.s) e (MAGN.c) d --> (mag(z).s) e c d                    */
        /*------------------------------------------------------------*/
        S = cons(magnitude(car(S)),cdr(S));
        goto skip1;

      case ANGL:
        /*------------------------------------------------------------*/
        /* (z.s) e (ANGL.c) d --> (angle(z).s) e c d                  */
        /*------------------------------------------------------------*/
        S = cons(angle(car(S)),cdr(S));
        goto skip1;

      case LD:
        /*------------------------------------------------------------*/
        /* s e (LD i.c) d --> (locate(i,e).s) e c d                   */
        /*------------------------------------------------------------*/
        loc = E; k = INTVAL(caadr(C));
        for (i=0;i<k;i++)
          loc = cdr(loc);
        loc = car(loc); k = INTVAL(cdadr(C));
        if (loc == BLACK_HOLE)
          ErrThrow(ERR_R9_BLACK_HOLE);
        for (i=0;i<k;i++)
          loc = cdr(loc);
        S = cons(car(loc),S);
      skip2:
        C = cddr(C);
        break;

      case ST:
        /*------------------------------------------------------------*/
        /* (a.s) e (ST i.c) d --> (a.s) e' c d                        */
        /* where e' = update e set locate(i) = a                      */
        /*------------------------------------------------------------*/
        loc = E; k = INTVAL(caadr(C));
        for (i=0;i<k;i++)
          loc = cdr(loc);
        loc = car(loc); k = INTVAL(cdadr(C));
        if (loc == BLACK_HOLE)
          ErrThrow(ERR_R9_BLACK_HOLE);
        for (i=0;i<k;i++)
          loc = cdr(loc);
        car(loc) = car(S); C = cddr(C);
        break;

      case LDC:
        /*------------------------------------------------------------*/
        /* s e (LDC x.c) d --> (x.s) e c d                            */
        /*------------------------------------------------------------*/
        S = cons(cadr(C),S);
        goto skip2;

      case LDF:
        /*------------------------------------------------------------*/
        /* s e (LDF c'.c) d --> ((c'.e).s) e c d                      */
        /*------------------------------------------------------------*/
        S = cons(cons(cadr(C),E), S);
        goto skip2;
  
      case AP:
        /*------------------------------------------------------------*/
        /* ((c'.e') v.s) e (AP.c) d --> NIL (v.e') c' (s e c.d)       */
        /*------------------------------------------------------------*/
        D = cons(cddr(S), cons(E, cons(cdr(C), D)));
        E = cons(cadr(S), cdar(S));
        C = caar(S); S = NIL;
        break;
  
      case LDFC:
        /*------------------------------------------------------------*/
        /* s e (LDFC n c'.c) d --> ((#clos n c'.e).s) e c d           */
        /*------------------------------------------------------------*/
        S = cons(cons(CLOS_TAG,
                        cons(cadr(C),
                              cons(caddr(C),E))),
                  S);
        C = cdddr(C);
        break;

      case APC:
      case APY:
        /*------------------------------------------------------------*/
        /* ((#clos n c'.e') v.s) e (APC.c) d -->                      */
        /*                              NIL (v.e') c' (s e c.d)       */
        /*  (special rule for variable number of args)                */
        /* ((#cont s e c.d) (v).s') e' (APC.c') d' --> (v.s) e c d    */
        /*------------------------------------------------------------*/
        if (!IS_CONS(car(S)))
          typeError(car(S),"function");
        if (caar(S) == CLOS_TAG)
        {
          application(INTVAL(car(C)) == APY);
          D = cons(cddr(S),
                    cons(E,
                          cons(cdr(C),D)));
          E = cons(cadr(S), cdddr(car(S)));
          C = caddr(car(S)); S = NIL;
        }
        else if (caar(S) == CONT_TAG)
        {
          /*----------------------------------------------------------*/
          /* apply continuation                                       */
          /*----------------------------------------------------------*/
        applyCont:
          if (listLength(cadr(S)) != 1)
          {
            expectedLen = 1;
            error1(ERR_R3_NUM_ARGS,cadr(S));
          }
          E = cadr(cdar(S));
          C = caddr(cdar(S));
          D = cdddr(cdar(S));
          S = cons(caadr(S), cadar(S));
        }
        else
          typeError(car(S),"function");
        break;

      case TAPC:
      case TAPY:
        /*------------------------------------------------------------*/
        /* ((#clos n c'.e') v.s) e (TAPC) d -->  s (v.e') c' d        */
        /*  (special rule for variable number of args)                */
        /* ((#cont s e c.d) (v).s') e' (TAPC) d' --> (v.s) e c d      */
        /*------------------------------------------------------------*/
        if (!IS_CONS(car(S)))
          typeError(car(S),"function");
        if (caar(S) == CLOS_TAG)
        {
          application(INTVAL(car(C)) == TAPY);
          E = cons(cadr(S), cdddr(car(S)));
          C = caddr(car(S)); S = cddr(S);
        }
        else if (caar(S) == CONT_TAG)
          goto applyCont;
        else
          typeError(car(S),"function");
        break;

      case TAP:
        /*------------------------------------------------------------*/
        /* ((c'.e') v.s) e (TAP.c) d --> s (v.e') c' d                */
        /*------------------------------------------------------------*/
        E = cons(cadr(S), cdar(S));
        C = caar(S);
        S = cddr(S);
        break;
  
      case RTN:
        /*------------------------------------------------------------*/
        /* (a) e' (RTN) (s e c.d) --> (a.s) e c d                     */
        /*------------------------------------------------------------*/

        /*------------------------------------------------------------*/
        /* Store environment, if we load from file                    */
        /*------------------------------------------------------------*/
        if (pMemGlobal->fileLoad)       
          pMemGlobal->newTopLevelVals = car(E);

        S = cons(car(S), car(D));
        E = cadr(D); C = caddr(D); D = cdddr(D);
        break;

      case LDM:
        /*------------------------------------------------------------*/
        /* s e (LDM c'.c) d --> ((#macro #clos 1 c'.e).s) e c d       */
        /*------------------------------------------------------------*/
        S = cons(cons(MACR_TAG,
                      cons(CLOS_TAG,
                        cons(MKINT(1),
                              cons(cadr(C),E)))),
                  S);
        C = cddr(C);
        break;

      case DUM:
        /*------------------------------------------------------------*/
        /* s e (DUM.c) d --> s (?.e) c d                              */
        /*------------------------------------------------------------*/
        E = cons(BLACK_HOLE, E);
      skip1:
        C = cdr(C);
        break;
  
      case RAP:
        /*------------------------------------------------------------*/
        /* ((c'.e') v.s) (?.e) (RAP.c) d -> NIL rpl(e',v) c' (s e c.d)*/
        /*------------------------------------------------------------*/
        D = cons(cddr(S), cons(cdr(E), cons(cdr(C), D)));
        E = cdar(S); car(E) = cadr(S);
        C = caar(S); S = NIL;
        break;

      case RTAP:
        /*------------------------------------------------------------*/
        /* ((c'.e') v.s) (?.e) (RTAP.c) d --> NIL rpl(e',v) c' d      */
        /*------------------------------------------------------------*/
        E = cdar(S); car(E) = cadr(S);
        C = caar(S); S = NIL;
        break;
  
      case SEL:
        /*------------------------------------------------------------*/
        /* (x.s)   e (SEL ct cf.c) d --> s e ct (c.d)                 */
        /* (#f.s)  e (SEL ct cf.c) d --> s e cf (c.d)                 */
        /*------------------------------------------------------------*/
        D = cons(cdddr(C), D);
        /* fall thru! */

      case SELR:
        /*------------------------------------------------------------*/
        /* (x.s)   e (SELR ct cf.c) d --> s e ct d                    */
        /* (#f.s)  e (SELR ct cf.c) d --> s e cf d                    */
        /*------------------------------------------------------------*/
        C = (car(S) == FALSE) ? caddr(C) : cadr(C);
        S = cdr(S);
        break;

      case SEO:
      case SEA:
        /*------------------------------------------------------------*/
        /* (x.s)   e (SEO cf.c) d --> (x.s) e c d                     */
        /* (#f.s)  e (SEA ct.c) d --> (#f.s) e c d                    */
        /* (#f.s)  e (SEO cf.c) d --> s e cf (c.d)                    */
        /* (x.s)   e (SEA ct.c) d --> s e ct (c.d)                    */
        /*------------------------------------------------------------*/
        if ((INTVAL(car(C)) == SEO) == (car(S) == FALSE)) {
          D = cons(cddr(C), D);
          S = cdr(S); C = cadr(C);
        }
        else
          goto skip2;
        break;

      case SEOR:
      case SEAR:
        /*------------------------------------------------------------*/
        /* (x.s)   e (SEOR cf.c) d --> (x.s) e c d                    */
        /* (#f.s)  e (SEAR ct.c) d --> (#f.s) e c d                   */
        /* (#f.s)  e (SEOR cf.c) d --> s e cf d                       */
        /* (x.s)   e (SEAR ct.c) d --> s e ct d                       */
        /*------------------------------------------------------------*/
        if ((INTVAL(car(C)) == SEOR) == (car(S) == FALSE)) {
          S = cdr(S); C = cadr(C);
        }
        else
          goto skip2;
        break;

      case MEM:
        /*------------------------------------------------------------*/
        /* (x.s) e (MEM l ct cf.c) d --> s     e ct (c.d) if x in l   */
        /* (x.s) e (MEM l ct cf.c) d --> (x.s) e cf (c.d) else        */
        /*------------------------------------------------------------*/
        D = cons(cdr(cdddr(C)), D);
        /* fall thru! */

      case MEMR:
        /*------------------------------------------------------------*/
        /* (x.s) e (MEMR l ct cf.c) d --> s     e ct d if x in l      */
        /* (x.s) e (MEMR l ct cf.c) d --> (x.s) e cf d else           */
        /*------------------------------------------------------------*/
        if (member(car(S),cadr(C))) {
          S = cdr(S); C = caddr(C);
        }
        else
          C = car(cdddr(C));
        break;

      case JOIN:
        /*------------------------------------------------------------*/
        /* s e (JOIN) (c.d) -> s e c d                                */
        /*------------------------------------------------------------*/
        C = car(D); D = cdr(D);
        break;
  
      case CAR:
        /*------------------------------------------------------------*/
        /* ((a.b).s) e (CAR.c) d --> (a.s) e c d                      */
        /*------------------------------------------------------------*/
        if (!IS_PAIR(car(S)))
          typeError(car(S),"pair");
        S = cons(caar(S),cdr(S));
        goto skip1;
  
      case CDR:
        /*------------------------------------------------------------*/
        /* ((a.b).s) e (CDR.c) d --> (b.s) e c d                      */
        /*------------------------------------------------------------*/
        if (!IS_PAIR(car(S)))
          typeError(car(S),"pair");
        S = cons(cdar(S),cdr(S));
        goto skip1;

      case SCAR:
        /*------------------------------------------------------------*/
        /* (a' (a.b) .s) e (SCAR.c) d --> ((a'.b) .s) e c d           */
        /*------------------------------------------------------------*/
        if (!IS_PAIR(cadr(S)))
          typeError(cadr(S),"pair");
        caadr(S) = car(S);
        S = cdr(S);
        goto skip1;

      case SCDR:
        /*------------------------------------------------------------*/
        /* (b' (a.b) .s) e (SCDR.c) d --> ((a.b') .s) e c d           */
        /*------------------------------------------------------------*/
        if (!IS_PAIR(cadr(S)))
          typeError(cadr(S),"pair");
        cdadr(S) = car(S);
        S = cdr(S);
        goto skip1;
  
      case PAIR:
        /*------------------------------------------------------------*/
        /* ((a.b).s) e (PAIR.c) d --> (#t.s) e c d                    */
        /* (  x  .s) e (PAIR.c) d --> (#f.s) e c d                    */
        /*------------------------------------------------------------*/
        S = cons(IS_PAIR(car(S)) ? TRUE : FALSE, cdr(S));
        goto skip1;

      case NUL:
        /*------------------------------------------------------------*/
        /* (() .s) e (NUL.c) d --> (#t.s) e c d                       */
        /* (x  .s) e (NUL.c) d --> (#f.s) e c d                       */
        /*------------------------------------------------------------*/
        S = cons((car(S) == NIL) ? TRUE : FALSE, cdr(S));
        goto skip1;

      case CHR:
        /*------------------------------------------------------------*/
        /* (#\? .s) e (CHR.c) d --> (#t.s) e c d                      */
        /* (x   .s) e (CHR.c) d --> (#f.s) e c d                      */
        /*------------------------------------------------------------*/
        S = cons(IS_CHAR(car(S)) ? TRUE : FALSE, cdr(S));
        goto skip1;

      case STRG:
        /*------------------------------------------------------------*/
        /* ("..." .s) e (STRG.c) d --> (#t.s) e c d                   */
        /* (x     .s) e (STRG.c) d --> (#f.s) e c d                   */
        /*------------------------------------------------------------*/
        S = cons(IS_STRING(car(S)) ? TRUE : FALSE, cdr(S));
        goto skip1;

      case PROC:
        /*------------------------------------------------------------*/
        /* ([clos] . s) e (PROC.c) d --> (#t.s) e c d                 */
        /* (x      . s) e (PROC.c) d --> (#f.s) e c d                 */
        /*------------------------------------------------------------*/
        S = cons(IS_CONS(car(S)) &&
                  (caar(S) == CLOS_TAG ||
                   caar(S) == CONT_TAG)  ? TRUE : FALSE, cdr(S));
        goto skip1;

      case DASM:
        /*------------------------------------------------------------*/
        /* ([clos] . s) e (DASM.c) d --> (caddr(clos).s) e c d        */
        /* ([macr] . s) e (DASM.c) d --> (cadddr(macro).s) e c d      */
        /*------------------------------------------------------------*/
        if (!IS_CONS(car(S)) || caar(S) != CLOS_TAG)
          if (!IS_CONS(car(S)) || caar(S) != MACR_TAG)
            typeError(car(S),"procedure");
          else
            S = cons(car(cdddr(car(S))),cdr(S));
        else
          S = cons(caddr(car(S)),cdr(S));
        goto skip1;

      case CONT:
        /*------------------------------------------------------------*/
        /* ([cont] . s) e (PROC.c) d --> (#t.s) e c d                 */
        /* (x      . s) e (PROC.c) d --> (#f.s) e c d                 */
        /*------------------------------------------------------------*/
        S = cons(IS_CONS(car(S)) &&
                  caar(S) == CONT_TAG  ? TRUE : FALSE, cdr(S));
        goto skip1;

      case MACP:
        /*------------------------------------------------------------*/
        /* ([macro]. s) e (MACP.c) d --> (#t.s) e c d                 */
        /* (x      . s) e (MACP.c) d --> (#f.s) e c d                 */
        /*------------------------------------------------------------*/
        S = cons(IS_MACRO(car(S)) ? TRUE : FALSE, cdr(S));
        goto skip1;

      case PROM:
        /*------------------------------------------------------------*/
        /* ([prom] . s) e (PROM.c) d --> (#t.s) e c d                 */
        /* (x      . s) e (PROM.c) d --> (#f.s) e c d                 */
        /*------------------------------------------------------------*/
        S = cons(IS_CONS(car(S)) &&
                 (caar(S) == RCPD_TAG ||
                  caar(S) == RCPF_TAG)  ? TRUE : FALSE, cdr(S));
        goto skip1;

      case SYMB:
        /*------------------------------------------------------------*/
        /* (symbol . s) e (SYMB.c) d --> (#t.s) e c d                 */
        /* (x      . s) e (SYMB.c) d --> (#f.s) e c d                 */
        /*------------------------------------------------------------*/
        S = cons(IS_ATOM(car(S)) ? TRUE : FALSE, cdr(S));
        goto skip1;

      case NONE:
        /*------------------------------------------------------------*/
        /* (#n . s) e (NONE.c) d --> (#t.s) e c d                     */
        /* (x  . s) e (NONE.c) d --> (#f.s) e c d                     */
        /*------------------------------------------------------------*/
        S = cons(car(S) == NOPRINT ? TRUE : FALSE, cdr(S));
        goto skip1;

      case INTP:
        /*------------------------------------------------------------*/
        /* (n.s) e (INTP.c) d --> (#t.s) e c d                        */
        /* (a.s) e (INTP.c) d --> (#f.s) e c d                        */
        /*------------------------------------------------------------*/
        S = cons(IS_INT(car(S)) ? TRUE : FALSE, cdr(S));
        goto skip1;

      case NOT:
        /*------------------------------------------------------------*/
        /* (#f .s) e (NOT.c) d --> (#t.s) e c d                       */
        /* (x  .s) e (NOT.c) d --> (#f.s) e c d                       */
        /*------------------------------------------------------------*/
        S = cons((car(S) == FALSE) ? TRUE : FALSE,
                  cdr(S));
        goto skip1;

      case BOOL:
        /*------------------------------------------------------------*/
        /* (#f .s) e (NUL.c) d --> (#t.s) e c d                       */
        /* (#t .s) e (NUL.c) d --> (#t.s) e c d                       */
        /* (x  .s) e (NUL.c) d --> (#f.s) e c d                       */
        /*------------------------------------------------------------*/
        S = cons((car(S) == TRUE || car(S) == FALSE) ? TRUE : FALSE,
                 cdr(S));
        goto skip1;

      case INPP:
        /*------------------------------------------------------------*/
        /* (inport.s) e (INPP.c) d --> (#t.s) e c d                   */
        /* (a.s) e (INPP.c) d --> (#f.s) e c d                        */
        /*------------------------------------------------------------*/
        S = cons(IS_IPORT(car(S)) ? TRUE : FALSE,
                cdr(S));
        goto skip1;

      case OUPP:
        /*------------------------------------------------------------*/
        /* (outport.s) e (OUPP.c) d --> (#t.s) e c d                  */
        /* (a.s) e (OUPP.c) d --> (#f.s) e c d                        */
        /*------------------------------------------------------------*/
        S = cons(IS_OPORT(car(S)) ? TRUE : FALSE,
                 cdr(S));
        goto skip1;

      case EOFO:
        /*------------------------------------------------------------*/
        /* (#eof.s) e (EOFO.c) d --> (#t.s) e c d                     */
        /* (a.s) e (EOFO.c) d --> (#f.s) e c d                        */
        /*------------------------------------------------------------*/
        S = cons((car(S) == END_OF_FILE) ? TRUE : FALSE,
                 cdr(S));
        goto skip1;

     case VECP:
        /*------------------------------------------------------------*/
        /* (#(...).s) e (VECP.c) d --> (#t.s) e c d                   */
        /* (a.s)      e (VECP.c) d --> (#f.s) e c d                   */
        /*------------------------------------------------------------*/
        S = cons(IS_VEC(car(S)) ? TRUE : FALSE,
                 cdr(S));
        goto skip1;

      case CONS:
        /*------------------------------------------------------------*/
        /* (a b.s) e (CONS.c) d --> ((a.b).s) e c d                   */
        /*------------------------------------------------------------*/
        S = cons(cons(car(S),cadr(S)),cddr(S));
        goto skip1;

      case APND:
        /*------------------------------------------------------------*/
        /* (a b.s) e (APND.c) d --> (append(a,b).s) e c d             */
        /*------------------------------------------------------------*/
        if (car(S) != NIL && !IS_PAIR(car(S)))
          typeError(car(S),"list");
        W = NIL;
        *listCopy(&W,car(S),32767) = cadr(S);
        S = cons(W, cddr(S));
        goto skip1;
  
      case EQ:
        /*------------------------------------------------------------*/
        /* (a a.s) e (EQ.c) d --> (#t.s) e c d                        */
        /* (a b.s) e (EQ.c) d --> (#f.s) e c d                        */
        /*------------------------------------------------------------*/
        S = cons(car(S) == cadr(S) ? TRUE : FALSE, cddr(S));
        goto skip1;
  
      case EQV:
        /*------------------------------------------------------------*/
        /* (a a.s) e (EQ.c) d --> (#t.s) e c d                        */
        /* (a b.s) e (EQ.c) d --> (#f.s) e c d                        */
        /* additionally compare reals                                 */
        /*------------------------------------------------------------*/
        S = cons(eqv(car(S),cadr(S)) ? TRUE : FALSE, cddr(S));
        goto skip1;

      case ADD:
        /*------------------------------------------------------------*/
        /* (a b.s) e (ADD.c) d --> (b+a.s) e c d                      */
        /*------------------------------------------------------------*/
        S = cons(genBinOp(car(S),cadr(S),&addInt,sysFloatEm_d_add,&addCpl),
                 cddr(S));
        goto skip1;
  
      case SUB:
        /*------------------------------------------------------------*/
        /* (a b.s) e (SUB.c) d --> (b-a.s) e c d                      */
        /*------------------------------------------------------------*/
        S = cons(genBinOp(car(S),cadr(S),&subInt,sysFloatEm_d_sub,&subCpl),
                 cddr(S));
        goto skip1;
  
      case MUL:
        /*------------------------------------------------------------*/
        /* (a b.s) e (MUL.c) d --> (b*a.s) e c d                      */
        /*------------------------------------------------------------*/
        S = cons(genBinOp(car(S),cadr(S),&mulInt,sysFloatEm_d_mul,&mulCpl),
                 cddr(S));
        goto skip1;
  
      case DIV:
        /*------------------------------------------------------------*/
        /* (a b.s) e (DIV.c) d --> (b/a.s) e c d                      */
        /*------------------------------------------------------------*/
        S = cons(div(car(S),cadr(S)), cddr(S));
        goto skip1;
  
      case REM:
        /*------------------------------------------------------------*/
        /* (a b.s) e (REM.c) d --> (b%a.s) e c d                      */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        checkInt(cadr(S));
        if (INTVAL(cadr(S)) == 0)
          ErrThrow(ERR_R8_DIV_BY_ZERO);
        S = cons(MKINT((INTVAL(car(S))) % (INTVAL(cadr(S)))), cddr(S));
        goto skip1;

      case LEQ:
        /*------------------------------------------------------------*/
        /* (a b.s) e (LEQ.c) d --> (T.s)   e c d  if b>=a             */
        /* (a b.s) e (LEQ.c) d --> (NIL.s) e c d  if b< a             */
        /*------------------------------------------------------------*/
        S = cons(leq(car(S),cadr(S)) ? TRUE : FALSE, cddr(S));
        goto skip1;
  
      case STOP:
        /*------------------------------------------------------------*/
        /* (v.s) e (STOP) d --> stop returning v                      */
        /*------------------------------------------------------------*/
        if (pMemGlobal->fileLoad && !evalMacro)
        {
          /*----------------------------------------------------------*/
          /* Extend top level environment                             */
          /*----------------------------------------------------------*/
          pMemGlobal->tlVals = cons(pMemGlobal->newTopLevelVals,
                                     pMemGlobal->tlVals);
          pMemGlobal->newTopLevelVals = NIL;
        }
        running = false;
        return car(S);

      case LDE:
        /*------------------------------------------------------------*/
        /* s e (LDE c.c') d --> ((#rcpd c.e).s) e c' d                */
        /*------------------------------------------------------------*/
        S = cons(cons(RCPD_TAG, cons(cadr(C), E)), S);
        goto skip2;
  
      case AP0:
        /*------------------------------------------------------------*/
        /* ((#rcpd c.e).s) e' (AP0.c') d -->                          */
        /*                          NIL e c (((#rcpd c.e).s) e' c'.d) */
        /* ((#rcpf.x).s) e (AP0.c) d --> (x.s) e c d                  */
        /*------------------------------------------------------------*/
        if (!IS_CONS(car(S)))
          typeError(car(S),"promise");
        if (caar(S) == RCPD_TAG)
        {
          D = cons(S, cons(E, cons(cdr(C), D)));
          C = cadar(S); E = cddar(S); S = NIL;
        }
        else if (caar(S) == RCPF_TAG)
        {
          S = cons(cdar(S), cdr(S));
          goto skip1;
        }
        else
          typeError(car(S),"promise");
        break;
  
      case UPD:
        /*------------------------------------------------------------*/
        /* (x) e (UPD) (((#rcpd c.e).s) e' c'.d) --> (x.s) e' c' d    */
        /* and replace (#rcpd c.e) by (#rcpf.x)                       */
        /*------------------------------------------------------------*/
        S = cons(car(S), cdar(D));
        E = cadr(D); C = caddr(D);
        caaar(D) = RCPF_TAG; cdaar(D) = car(S);
        D = cdddr(D);
        break;
  
      case LDCT:
        /*------------------------------------------------------------*/
        /* s e (LDCT c'.c) d --> (((#cont s e c'.d)).s) e c d         */
        /*------------------------------------------------------------*/
        S = cons(cons(cons(CONT_TAG,
                              cons(S,
                                    cons(E,
                                          cons(cadr(C),
                                                D)))),
                        NIL),
                  S);

        goto skip2;

      case UERR:
        /*------------------------------------------------------------*/
        /* (x.s) e (UERR.c) d --> abort interpreter                   */
        /*------------------------------------------------------------*/
        error1(ERR_USER_ERROR, car(S));
        break;

      case CERR:
        /*------------------------------------------------------------*/
        /* Abort uncondionally                                        */
        /*------------------------------------------------------------*/
        ErrThrow(ERR_R10_COND_CLAUSE);
        break;

      case POP:
        /*------------------------------------------------------------*/
        /* (x.s) e (POP.c) d --> s e c d                              */
        /*------------------------------------------------------------*/
        S = cdr(S);
        goto skip1;

      case DSPL:
        /*------------------------------------------------------------*/
        /* (p x.s) e (DSPL.c) d --> (x.s) e c d and display x on port */
        /*------------------------------------------------------------*/
        if (car(S)==MKINT(0))
          printSEXP(cadr(S), PRT_OUTFIELD);
        else
          writeFile(cadr(S), car(S), false);
        S = cdr(S);
        goto skip1;

      case WRIT:
        /*------------------------------------------------------------*/
        /* (p x.s) e (WRIT.c) d --> (x.s) e c d and write x on port   */
        /*------------------------------------------------------------*/
        if (car(S)==MKINT(0))
          printSEXP(cadr(S), PRT_OUTFIELD | PRT_ESCAPE | PRT_SPACE);
        else
          writeFile(cadr(S), car(S), true);
        S = cdr(S);
        goto skip1;

      case SLEN:
        /*------------------------------------------------------------*/
        /* ((#STR . l) . s) e (SLEN.c) d --> (length(l).s) e c d      */
        /*------------------------------------------------------------*/
        S = cons(MKINT(stringLength(car(S))), cdr(S));
        goto skip1;

      case S2L:
        /*------------------------------------------------------------*/
        /* ((#STR . l) . s) e (S2L.c) d --> (copy(l).s) e c d         */
        /*------------------------------------------------------------*/
        checkString(car(S));
        S = cons(string2List(car(S)),cdr(S));
        goto skip1;

      case L2S:
        /*------------------------------------------------------------*/
        /* (l . s) e (L2S.c) d --> ((#STR copy(l)) . s) e c d         */
        /*------------------------------------------------------------*/
        S = cons(makeString(listLength(car(S)),0,0,0,car(S)), cdr(S));
        goto skip1;

      case SAPP:
        /*------------------------------------------------------------*/
        /* ((#STR.l1) (#STR.l2) . s) e (SAPP.c) d -->                 */
        /*      ((#STR copy(l1) copy(l2)) . s) e c d                  */
        /*------------------------------------------------------------*/
        S = cons(appendStrings(car(S),cadr(S)),cddr(S));
        goto skip1;

      case SEQ:
        /*------------------------------------------------------------*/
        /* ((#STR.l1) (#STR.l2) . s) e (SEQ.c) d --> (l1==l2 .s) e c d*/
        /*------------------------------------------------------------*/
        checkString(car(S));
        checkString(cadr(S));
        S = cons(strComp(car(S), cadr(S)) == 0 ? TRUE : FALSE, cddr(S));
        goto skip1;

      case SREF:
        /*------------------------------------------------------------*/
        /* ((#STR.l) n . s) e (SREF.c) d --> (access(l,n) . s) e c d  */
        /*------------------------------------------------------------*/
        stringAcc(&W, car(S), cadr(S), false);
        S = cons(W, cddr(S));
        goto skip1;

      case SSET:
        /*------------------------------------------------------------*/
        /* ((#STR.l) n ch . s) e (SSET.c) d -->                       */
        /*      ((#STR set(l,n,ch)) . s) e c d                        */
        /*------------------------------------------------------------*/
        stringAcc(&caddr(S),car(S),cadr(S),true);
        S = cons(car(S),cdddr(S));
        goto skip1;

      case SUBS:
        /*------------------------------------------------------------*/
        /* ((#STR.l) n k . s) e (SUBS.c) d -->                        */
        /*      ((#STR substr(l,n,k)) . s) e c d                      */
        /*------------------------------------------------------------*/
        checkInt(cadr(S));
        checkInt(caddr(S));
        S = cons(substring(car(S),INTVAL(cadr(S)),INTVAL(caddr(S))),
                 cdddr(S));
        goto skip1;

      case SMAK:
        /*------------------------------------------------------------*/
        /* (n f . s) e (SMAK.c) d --> (mkString(n,f) . s) e c d       */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        if (!IS_CHAR(cadr(S)))
          typeError(cadr(S),"char");   
        if (INTVAL(car(S)) < 0)
          parmError(car(S),"make-string");
        S = cons(makeString(INTVAL(car(S)),0,0,(char*)1,cadr(S)), cddr(S));
        goto skip1;

      case C2I:
        /*------------------------------------------------------------*/
        /* (ch . s) e (C2I.c) d --> (asc(ch) . s) e c d               */
        /*------------------------------------------------------------*/
        if (!IS_CHAR(car(S)))
          typeError(car(S),"char");
        S = cons(MKINT(CHARVAL(car(S))), cdr(S));
        goto skip1;

      case I2C:
        /*------------------------------------------------------------*/
        /* (n . s) e (C2I.c) d --> (chr(n) . s) e c d                 */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        S = cons(MKCHAR(INTVAL(car(S))&0xff), cdr(S));
        goto skip1;

      case O2S:
        /*------------------------------------------------------------*/
        /* (x.s) e (O2S.c) d --> (format(x).s) e c d                  */
        /*------------------------------------------------------------*/
        printSEXP(car(S), PRT_MEMO | PRT_ESCAPE);
        S = cons(str2Lisp(msg), cdr(S));
        goto skip1;

      case S2O:
        /*------------------------------------------------------------*/
        /* (x.s) e (S2O.c) d --> (read(x).s) e c d                    */
        /*------------------------------------------------------------*/
        checkString(car(S));
        printSEXP(car(S), PRT_MEMO);
        S = cons(readSEXP(msg), cdr(S));
        goto skip1;

      case RAND:
        /*------------------------------------------------------------*/
        /* (n . s) e (RAND.c) d --> (rand(n) . s) e c d               */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        if (INTVAL(car(S)) == 0)
          ErrThrow(ERR_R8_DIV_BY_ZERO);
        S = cons(MKINT(SysRandom(0) % INTVAL(car(S))), cdr(S));
        goto skip1;

      case EVT:
        /*------------------------------------------------------------*/
        /* (#f.s) e (EVT . c) d --> (event.s) e c d                   */
        /* (x.s)  e (EVT . c) d --> (event.s) e c d                   */
        /* Waits for event if arg is true                             */
        /*------------------------------------------------------------*/
        pMemGlobal->waitEvent = car(S) != FALSE;
        pMemGlobal->getEvent  = true;
        C = cdr(C);
        S = cdr(S);
        return NIL;

      case GUI:
        /*------------------------------------------------------------*/
        /* (x . s) e (GUI.c) d --> (x . s) e c d                      */
        /* and owns/disowns GUI event handling                        */
        /*------------------------------------------------------------*/
        enableCtls(!(pMemGlobal->ownGUI = car(S) != FALSE));
        goto skip1;

      case READ:
      case REAC:
      case REDL:
      case PEEK:
        /*------------------------------------------------------------*/
        /* (port.s) e (OPxx.c) d --> (ex.s) e c d                     */
        /* where ex is read from port in different ways               */
        /*------------------------------------------------------------*/
        S = cons(readFile(car(S), INTVAL(car(C))), cdr(S));
        goto skip1;

      case DRAW:
        /*------------------------------------------------------------*/
        /* (x y.s) e (DRAW.c) d --> (#n.s) e c d                      */
        /* and draw a line to x,y                                     */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        checkInt(cadr(S));
        drawLine(INTVAL(car(S)),
                 INTVAL(cadr(S)));
        S = cons(NOPRINT, cddr(S));
        goto skip1;

      case RECT:
        /*------------------------------------------------------------*/
        /* (r x y.s) e (RECT.c) d --> (#n.s) e c d                    */
        /* and draw a rectangle to x,y                                */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        checkInt(cadr(S));
        checkInt(caddr(S));
        drawRect(INTVAL(car(S)),
                 INTVAL(cadr(S)),
                 INTVAL(caddr(S)));
        S = cons(NOPRINT, cdddr(S));
        goto skip1;

      case TEXT:
        /*------------------------------------------------------------*/
        /* (a . s) e (TEXT.c) d --> (#n.s) e c d                      */
        /* and display text at graphic coordinates                    */
        /*------------------------------------------------------------*/
        drawText(car(S));
        S = cons(NOPRINT,cdr(S));
        goto skip1;

      case BITM:
        /*------------------------------------------------------------*/
        /* (b . s) e (BITM.c) d --> (#n.s) e c d                      */
        /* and display bitmap at graphic coordinates                  */
        /*------------------------------------------------------------*/
        drawBitmap(car(S));
        S = cons(NOPRINT,cdr(S));
        goto skip1;

      case UINF:
        /*------------------------------------------------------------*/
        /* (x.s) e (UINF.c) d --> (#n.s) e c d and give message x     */
        /*------------------------------------------------------------*/
        printSEXP(car(S), PRT_MESSAGE);
        ReleaseMem();
        FrmCustomAlert(ERR_USER_INFO, msg, "", "");
        GrabMem();
        S = cons(NOPRINT,cdr(S));
        goto skip1;

      case SND:
        /*------------------------------------------------------------*/
        /* (x y.s) e (SND.c) d --> (#n.s) e c d                       */
        /* and play sound freq x, dur y                               */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        checkInt(cadr(S));
        sound(INTVAL(car(S)), INTVAL(cadr(S)));
        S = cons(NOPRINT,cddr(S));
        goto skip1;

      case WAIT:
      {
        /*------------------------------------------------------------*/
        /* (x.s) e (WAIT.c) d --> (#n.s) e c d                        */
        /* and wait x milliseconds                                    */
        /*------------------------------------------------------------*/
        Int32 ticks;
        checkInt(car(S));
        if (INTVAL(car(S)) < 0)
          parmError(car(S),"wait");
        ticks = INTVAL(car(S))*sysTicksPerSecond/1000L;
        if (ticks>0)
          SysTaskDelay(ticks);
        S = cons(NOPRINT,cdr(S));
        YIELD();
        goto skip1;
      }

      case MDIR:
        /*------------------------------------------------------------*/
        /* (cat . s) e (MDIR.c) d --> (dir.s) e c d                   */
        /* where dir is the memo directory of category cat            */
        /*------------------------------------------------------------*/
        S = cons(memoDir(car(S)),cdr(S));
        goto skip1;

      case OOUT:
        /*------------------------------------------------------------*/
        /* (name.s) e (OOUT.c) d --> (port.s) e c d                   */
        /* and create memo <name>                                     */
        /*------------------------------------------------------------*/
        S = cons(createFile(car(S)), cdr(S));
        goto skip1;

      case OINP:
        /*------------------------------------------------------------*/
        /* (name.s) e (OINP.c) d --> (port.s) e c d                   */
        /* and open memo <name>                                       */
        /*------------------------------------------------------------*/
        S = cons(openFile(car(S), false), cdr(S));
        goto skip1;

      case OAPP:
        /*------------------------------------------------------------*/
        /* (name.s) e (OAPP.c) d --> (port.s) e c d                   */
        /* and open memo <name> for append                            */
        /*------------------------------------------------------------*/
        S = cons(openFile(car(S), true), cdr(S));
        goto skip1;

      case DELF:
        /*------------------------------------------------------------*/
        /* (name.s) e (DELF.c) d --> (#n.s) e c d                     */
        /* and delete memo <name>                                     */
        /*------------------------------------------------------------*/
        deleteFile(car(S));
        S = cons(NOPRINT,cdr(S));
        goto skip1;

      case VMAK:
        /*------------------------------------------------------------*/
        /* (n f . s) e (VMAK.c) d --> (#(f...f).s) e c d              */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        if (INTVAL(car(S)) < 0)
          parmError(car(S),"make-vector");
        S = cons(makeVector(INTVAL(car(S)), cadr(S), false), cddr(S));
        goto skip1;

      case VLEN:
        /*------------------------------------------------------------*/
        /* (vec . s) e (VLEN.c) d --> (length(vec).s) e c d           */
        /*------------------------------------------------------------*/
        if (!IS_VEC(car(S)))
          typeError(car(S),"vector");
        S = cons(MKINT(vectorLength(car(S))), cdr(S));
        goto skip1;

      case VREF:
        /*------------------------------------------------------------*/
        /* (vec n . s) e (VREF.c) d --> (access(vec,n) . s) e c d     */
        /*------------------------------------------------------------*/
        vectorAcc(&W, car(S), cadr(S), false);
        S = cons(W, cddr(S));
        goto skip1;

      case VSET:
        /*------------------------------------------------------------*/
        /* (vec n x . s) e (VSET.c) d --> (vec'. s) e c d             */
        /* where vec' is vec updated at index n with x                */
        /*------------------------------------------------------------*/
        vectorAcc(&caddr(S),car(S),cadr(S),true);
        S = cons(car(S),cdddr(S));
        goto skip1;

      case V2L:
        /*------------------------------------------------------------*/
        /* (vec . s) e (V2L.c) d --> (list(vec).s) e c d              */
        /*------------------------------------------------------------*/
        if (!IS_VEC(car(S)))
          typeError(car(S),"vector");
        S = cons(vector2List(car(S)),cdr(S));
        goto skip1;

      case L2V:
        /*------------------------------------------------------------*/
        /* (l . s) e (L2V.c) d --> (vector(l) . s) e c d              */
        /*------------------------------------------------------------*/
        S = cons(makeVector(listLength(car(S)), car(S), true), cdr(S));
        goto skip1;

      case GSYM:
        /*------------------------------------------------------------*/
        /* s e (GSYM.c) d --> (Gxxx.s) e c d                          */
        /*------------------------------------------------------------*/
        S = cons(gensym(),S);
        goto skip1;

      case GTIM:
        /*------------------------------------------------------------*/
        /* s e (GTIM.c) d --> (time . s) e c d                        */
        /*------------------------------------------------------------*/
        S = cons(getTime(),S);
        goto skip1;

      case DBRD:
        /*------------------------------------------------------------*/
        /* (x n . s) e (DBRD.c) d --> (db-rec(n,x) . s) e c d         */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        S = cons(readDB(false,cadr(S),INTVAL(car(S))), cddr(S));
        goto skip1;

      case DBWR:
        /*------------------------------------------------------------*/
        /* (n x da . s) e (DBWR.c) d --> (db-write(n,x,da) . s) e c d */
        /*------------------------------------------------------------*/
        checkInt(cadr(S));
        S = cons(writeDB(car(S),INTVAL(cadr(S)),caddr(S)), cdddr(S));
        goto skip1;

      case RSRD:
        /*------------------------------------------------------------*/
        /* (x t . s) e (RSRD.c) d --> (db-res(t,x) . s) e c d         */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        S = cons(readDB(true,cadr(S),INTVAL(car(S))), cddr(S));
        goto skip1;

      case RDBS:
        /*------------------------------------------------------------*/
        /* (n . s) e (RDBS.c) d --> (n . s) e c d                     */
        /* and set n as active resource DB                            */
        /*------------------------------------------------------------*/
        setResDB(car(S));
        goto skip1;

      case FRMD:
        /*------------------------------------------------------------*/
        /* (x h . s) e (FRMD.c) d --> s e c d                         */
        /* Displays the form and stops running (we're now in event    */
        /* handling mode) and save (s e c d) in context stack         */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        popupForm(INTVAL(car(S)),cadr(S));
        return NIL;

      case FRMG:
        /*------------------------------------------------------------*/
        /* (x h . s) e (FRMG.c) d --> s e c d                         */
        /* Displays the form and stops running (we're now in event    */
        /* handling mode)                                             */
        /*------------------------------------------------------------*/
        checkInt(car(S));
        gotoForm(INTVAL(car(S)),cadr(S));
        return NIL;

      case FRMQ:
        /*------------------------------------------------------------*/
        /* (v) e' (FRMQ) d' --> (v . s) e c d                         */
        /* Event handling mode finished, so restore context of FRMD   */
        /* from continuation stack                                    */
        /*------------------------------------------------------------*/
        if (actContext==-1)
          goto skip1;
        quitHandler = true;
        return NIL;

      case FLDG:
        /*------------------------------------------------------------*/
        /* (id . s) e (FLDG.c) d --> (fld-get-text(id) . s) e c d     */
        /*------------------------------------------------------------*/
        S = cons(GUIfldGetText(car(S)), cdr(S));
        goto skip1;

      case FLDS:
        /*------------------------------------------------------------*/
        /* (id txt . s) e (FLDG.c) d --> (txt . s) e c d              */
        /* and set field text                                         */
        /*------------------------------------------------------------*/
        GUIfldSetText(car(S), cadr(S));
        S = cdr(S);
        goto skip1;

      case CTLG:
        /*------------------------------------------------------------*/
        /* (id . s) e (CTLG.c) d --> (ctl-get-val(id) . s) e c d      */
        /*------------------------------------------------------------*/
        S = cons(GUIctlGetVal(car(S)), cdr(S));
        goto skip1;

      case CTLS:
        /*------------------------------------------------------------*/
        /* (id val . s) e (CTLG.c) d --> (val . s) e c d              */
        /* and set control value                                      */
        /*------------------------------------------------------------*/
        GUIctlSetVal(car(S), cadr(S));
        S = cdr(S);
        goto skip1;

      case LSTG:
        /*------------------------------------------------------------*/
        /* (id . s) e (LSTG.c) d --> (lst-get-sel(id) . s) e c d      */
        /*------------------------------------------------------------*/
        S = cons(GUIlstGetSel(car(S)), cdr(S));
        goto skip1;

      case LSTS:
        /*------------------------------------------------------------*/
        /* (id val . s) e (LSTG.c) d --> (val . s) e c d              */
        /* and set list selection                                     */
        /*------------------------------------------------------------*/
        GUIlstSetSel(car(S), cadr(S));
        S = cdr(S);
        goto skip1;

      case LSTT:
        /*------------------------------------------------------------*/
        /* (id n . s) e (LSTT.c) d --> (lst-get-text(id,n) . s) e c d */
        /*------------------------------------------------------------*/
        S = cons(GUIlstGetText(car(S),cadr(S)), cddr(S));
        goto skip1;

      case LSTL:
        /*------------------------------------------------------------*/
        /* (id items . s) e (LSTL.c) d --> (items . s) e c d          */
        /* and set list choices to items                              */
        /*------------------------------------------------------------*/
        GUIlstSetItems(car(S),cadr(S));
        S = cdr(S);
        goto skip1;

      case FRGF:
        /*------------------------------------------------------------*/
        /* s e (FRGF.c) d --> (frm-get-focus() . s) e c d             */
        /*------------------------------------------------------------*/
        S = cons(GUIfrmGetFocus(), S);
        goto skip1;

      case FRSF:
        /*------------------------------------------------------------*/
        /* (id . s) e (FRSF.c) d --> (id . s) e c d                   */
        /* and sets focus to object id in current form                */
        /*------------------------------------------------------------*/
        GUIfrmSetFocus(car(S));
        goto skip1;

      case FRSH:
        /*------------------------------------------------------------*/
        /* (id on . s) e (FRSH.c) d --> (on . s) e c d                */
        /* and shows/hides object id in current form                  */
        /*------------------------------------------------------------*/
        GUIfrmShow(car(S),cadr(S));
        S = cdr(S);
        goto skip1;

      case HBLD:
        /*------------------------------------------------------------*/
        /* s e (HBLD.c) d --> (hbDir().s) e c d                       */
        /*------------------------------------------------------------*/
        S = cons(listHBDir(),S);
        goto skip1;

      case HBIF:
        /*------------------------------------------------------------*/
        /* (db . s) e (HBIF.c) d --> (hbInfo(db) . s) e c d           */
        /*------------------------------------------------------------*/
        S = cons(getHBInfo(car(S)), cdr(S));
        goto skip1;

      case HBGF:
        /*------------------------------------------------------------*/
        /* (db rn fn . s) e (HBGF.c) d --> (hbGet(db,rn,fn) . s) e c d*/
        /*------------------------------------------------------------*/
        S = cons(getHBFieldVal(car(S),cadr(S),caddr(S)),cdddr(S));
        goto skip1;

      case HBSF:
        /*------------------------------------------------------------*/
        /* (db rn fn v . s) e (HBSF.c) d --> (v . s) e c d            */
        /* and side effect hbSet(db,rn,fn,v)                          */
        /*------------------------------------------------------------*/
        setHBFieldVal(car(S), cadr(S), caddr(S), cadddr(S));
        S = cdddr(S);
        goto skip1;

      case HBGL:
        /*------------------------------------------------------------*/
        /* (db rn fn . s) e (HBGL.c) d --> (hbLnk(db,rn,fn) . s) e c d*/
        /*------------------------------------------------------------*/
        S = cons(getHBLinkList(car(S),cadr(S),caddr(S)),cdddr(S));
        goto skip1;

      case HBAR:
        /*------------------------------------------------------------*/
        /* (db . s) e (HBAR.c) d --> (hbAdd(db) . s) e c d            */
        /*------------------------------------------------------------*/
        S = cons(addHBRecord(car(S)), cdr(S));
        goto skip1;

      case RGBX:
        /*------------------------------------------------------------*/
        /* (r g b . s) e (RGBX.c) d --> (RGB2Index(r,g,b) . s) e c d  */
        /*------------------------------------------------------------*/
        S = cons(MKINT(RGB2Index(car(S), cadr(S), caddr(S))), cdddr(S));
        goto skip1;

      case XRGB:
        /*------------------------------------------------------------*/
        /* (x . s) e (XRGB.c) d --> (Index2RGB(x) . s) e c d          */
        /*------------------------------------------------------------*/
        S = cons(Index2RGB(car(S)), cdr(S));
        goto skip1;

      case DOGC:
        /*------------------------------------------------------------*/
        /* s e (DOGC.c) d --> (#n . s) e c d                          */
        /*------------------------------------------------------------*/
        gc(NIL, NIL); 
        S = cons(NOPRINT,S);
        goto skip1;

      case SPAL:
        /*------------------------------------------------------------*/
        /* (x r g b . s) e (SPAL.c) d --> (x . s) e c d               */
        /* and side effect set-palette entry x to (r,g,b)             */
        /*------------------------------------------------------------*/
        setPalette(car(S), cadr(S), caddr(S), cadddr(S));
        S = cons(car(S),cdddr(S));
        goto skip1;

      default:
        /*------------------------------------------------------------*/
        /* illegal opcode                                             */
        /*------------------------------------------------------------*/
        error1(ERR_R7_ILLEGAL_OP,car(C));
        break;
    } /* switch */
  } /* for */
  return NIL;
}


/**********************************************************************/
/* Access a vector                                                    */
/**********************************************************************/
static void vectorAcc(PTR* obj, PTR vec, PTR n, Boolean write)
{
  Int16     i;
  UInt16    index;
  MemHandle recHand;
  PTR*      pel;

  if (!IS_VEC(vec))
    typeError(vec,"vector");
  checkInt(n);
  if (vec==EMPTY_VEC || (i=INTVAL(n)) < 0)
    error1(ERR_R2_INVALID_INDEX, n);
  DmFindRecordByID(dbRef, UID_VEC(vec), &index);
  recHand = DmQueryRecord(dbRef,index);
  if (i >= MemHandleSize(recHand)/2)
    error1(ERR_R2_INVALID_INDEX, n);
  pel = (PTR*)(MemHandleLock(recHand));
  if (write)
    pel[i] = *obj;
  else
    *obj = pel[i];
  MemHandleUnlock(recHand);
}

/**********************************************************************/
/* Access a string                                                    */
/**********************************************************************/
static void stringAcc(PTR* c, PTR str, PTR n, Boolean write)
{
  Int16     i;
  UInt16    index;
  MemHandle recHand;
  char*     pc;

  checkString(str);
  checkInt(n);
  if (write && !IS_CHAR(*c))
    typeError(*c, "char");
  if (str==EMPTY_STR || (i=INTVAL(n)) < 0)
    error1(ERR_R2_INVALID_INDEX, n);
  DmFindRecordByID(dbRef, UID_STR(str), &index);
  recHand = DmQueryRecord(dbRef,index);
  if (i >= MemHandleSize(recHand))
    error1(ERR_R2_INVALID_INDEX, n);
  pc = MemHandleLock(recHand);
  if (write)
    pc[i] = CHARVAL(*c);
  else
    *c = MKCHAR(pc[i]);
  MemHandleUnlock(recHand);
}

/**********************************************************************/
/* length of a vector                                                 */
/**********************************************************************/
static short vectorLength(PTR v)
{ 
  UInt16    index;
  MemHandle recHand;

  if (v==EMPTY_VEC)
    return 0;
  DmFindRecordByID(dbRef, UID_VEC(v), &index);
  recHand = DmQueryRecord(dbRef,index);
  return MemHandleSize(recHand)/2;
}
