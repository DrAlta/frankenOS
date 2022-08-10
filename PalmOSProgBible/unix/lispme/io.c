/**********************************************************************/
/*                                                                    */
/* io.c: LISPME input/output subsystem implementation                 */
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
/* 14.06.1997 New                                                FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "io.h"
#include "LispMe.h"
#include "fpstuff.h"
#include "vm.h"
#include "util.h"

/**********************************************************************/
/* Global data                                                        */
/**********************************************************************/
char    msg[MEMO_OUTPUT_SIZE+1];
Int32   outpSize;
char    errorChar;
Int16   lineNr;
char*   currPtr;
char*   startPtr;
int     errorPos;
Boolean onlyWS;

/**********************************************************************/
/* Character classes for scanner                                      */
/**********************************************************************/
#define ccWS   0    /* white space: ' ', \t */
#define ccNL   1    /* newline \n */
#define ccLXA  2    /* letters but 'e', 'E', 'i', 'I' and extended chars */
#define ccE    3    /* 'e', 'E' */
#define ccD    4    /* decimal digit */
#define ccPM   5    /* +,- */
#define ccAT   6    /* @ */
#define ccBSL  7    /* backslash \ */
#define ccDOT  8    /* dot . */
#define ccPUN  9    /* punctuation ( ) ' ` , */
#define ccSEM  0xa  /* semicolon ; */
#define ccILL  0xb  /* illegal char */
#define ccEOF  0xc  /* end of file \0 */
#define ccHSH  0xd  /* hash sign # */
#define ccDQ   0xe  /* double quote " */
#define ccI    0xf  /* 'i', 'I' */

/**********************************************************************/
/* Token types                                                        */
/**********************************************************************/
#define TT_NAME     1
#define TT_INT      2
#define TT_FLOAT    3
#define TT_PUNCT    4
#define TT_CHAR     5
#define TT_STRING   6
#define TT_TRUE     7
#define TT_FALSE    8
#define TT_NOPRINT  9

/**********************************************************************/
/* Automaton states                                                   */
/**********************************************************************/
#define asSTART 0
#define asID    1
#define asCOMM  2
#define asN1    3
#define asN1D   4
#define asN2    5
#define asN3    6
#define asN4    7
#define asN5    8
#define asN6    9
#define asN7   10
#define asN8   11
#define asN9   12
#define asN10  13
#define asN11  14
#define asDOT  15
#define asHASH 16
#define asCHAR 17
#define asSTRG 18
#define asSTRE 19
#define asSTH1 20
#define asSTH2 21
#define asCHH1 22
#define asCHH2 23

#define rsREAL 0
#define rsIMAG 1
#define rsPOL  2

/**********************************************************************/
/* Add current char to token                                          */
/**********************************************************************/
#define PUSH_CHAR(c) {                                                 \
  if (p-token >= MAX_TOKEN_LEN)                                        \
    ErrThrow(ERR_S4_TOKEN_LEN);                                        \
  *p++ = c;}

/**********************************************************************/
/* Formatting parameters                                              */
/**********************************************************************/
#define NUM_DIGITS   15
#define MIN_FLOAT    4
#define ROUND_FACTOR 1.0000000000000005 /* NUM_DIGITS zeros */

/**********************************************************************/
/* FP conversion constants                                            */
/**********************************************************************/
static double pow1[] =
{
  1e256, 1e128, 1e064,
  1e032, 1e016, 1e008,
  1e004, 1e002, 1e001
};

static double pow2[] =
{
  1e-256, 1e-128, 1e-064,
  1e-032, 1e-016, 1e-008,
  1e-004, 1e-002, 1e-001
};

/**********************************************************************/
/* Pilot chars classified...                                          */
/* V2.6: Sigh, starting with OS3.3 we now have different code tables  */
/* 0x80 was numericSpace char prior to 3.3, but now is the euro sign  */
/* Aceept as a valid symbol now.                                      */
/**********************************************************************/
static unsigned char charClass[] = {
/*         01    23    45    67    89    ab    cd    ef */
/* 0 */  0xcb, 0xbb, 0xbb, 0xbb, 0xb0, 0x1b, 0xbb, 0xbb,
/* 1 */  0xbb, 0xbb, 0xbb, 0xbb, 0xb0, 0xbb, 0xbb, 0xbb,
/* 2 */  0x02, 0xed, 0x22, 0x29, 0x99, 0x25, 0x95, 0x82,
/* 3 */  0x44, 0x44, 0x44, 0x44, 0x44, 0x2a, 0x22, 0x22,
/* 4 */  0x62, 0x22, 0x23, 0x22, 0x2f, 0x22, 0x22, 0x22,
/* 5 */  0x22, 0x22, 0x22, 0x22, 0x22, 0x2b, 0x7b, 0x22,
/* 6 */  0x92, 0x22, 0x23, 0x22, 0x2f, 0x22, 0x22, 0x22,
/* 7 */  0x22, 0x22, 0x22, 0x22, 0x22, 0x2b, 0xbb, 0x2b,
/* 8 */  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
/* 9 */  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
/* a */  0x02, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
/* b */  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
/* c */  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
/* d */  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
/* e */  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
/* f */  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22
};

/**********************************************************************/
/* Static data                                                        */
/**********************************************************************/
static Int16         outLen;
static Int16         gMaxLen;
static char          tt;
static unsigned char token[MAX_TOKEN_LEN+1];
static Int32         longVal;
static int           negative;
static short         stLen;

static double        doubleVal;
static double        doubleRe;
static int           scale;
static int           exponent;
static int           expNegative;
static int           realState;

/**********************************************************************/
/* having these global saves stack space in recursive print/scan fct. */
/**********************************************************************/
static UInt8         prtFlags;
static FlpCompDouble fcdRe;
static FlpCompDouble fcdIm;

/**********************************************************************/
/* output a token                                                     */
/**********************************************************************/
static void outStr(char* p) SEC(IO);
static void outStr(char* p) 
{
  while (*p)
  {
    if (++outLen >= gMaxLen)
      ErrThrow(ERR_O5_OUTPUT_TRUNC);
    else
      *currPtr++ = *p++;
  }
}

/**********************************************************************/
/* print a double (SysTrapFlpFToA is bogus!)                          */
/**********************************************************************/
static void printDouble(double x, Char* s) SEC(IO);
static void printDouble(double x, Char* s)
{
  FlpCompDouble fcd;
  short e,e1,i;
  double *pd, *pd1;
  char sign = '\0';
  short dec = 0;

  /*------------------------------------------------------------------*/
  /* Round to desired precision                                       */
  /*------------------------------------------------------------------*/
  x = mulDouble(x, ROUND_FACTOR);

  /*------------------------------------------------------------------*/
  /* check for NAN, +INF, -INF, 0                                     */
  /*------------------------------------------------------------------*/
  fcd.d = x;
  if ((fcd.ul[0] & 0x7ff00000) == 0x7ff00000)
    if (fcd.fdb.manH == 0 && fcd.fdb.manL == 0)
      if (fcd.fdb.sign)
        StrCopy(s, "[-inf]");
      else
        StrCopy(s, "[inf]");
    else
      StrCopy(s, "[nan]");
  else if (FlpIsZero(fcd))
    StrCopy(s, "0");
  else
  {
    /*----------------------------------------------------------------*/
    /* Make positive and store sign                                   */
    /*----------------------------------------------------------------*/
    if (FlpGetSign(fcd))
    {
      *s++ = '-';
      FlpSetPositive(fcd);
    }

    if ((unsigned)fcd.fdb.exp < 0x3ff) /* meaning x < 1.0 */
    {
      /*--------------------------------------------------------------*/
      /* Build negative exponent                                      */
      /*--------------------------------------------------------------*/
      for (e=1,e1=256,pd=pow1,pd1=pow2; e1; e1>>=1, ++pd, ++pd1)
        if (!leqDouble(*pd1, fcd.d))
        {
          e += e1;
          fcd.d = mulDouble(fcd.d, *pd);
        }
      fcd.d = mulDouble(fcd.d, 10.0);

      /*--------------------------------------------------------------*/
      /* Only print big exponents                                     */
      /*--------------------------------------------------------------*/
      if (e <= MIN_FLOAT)
      {
        *s++ = '0';
        *s++ = '.';
        dec = -1;
        while (--e)
          *s++ = '0';
      }
      else
        sign = '-';
    }
    else
    {
      /*--------------------------------------------------------------*/
      /* Build positive exponent                                      */
      /*--------------------------------------------------------------*/
      for (e=0,e1=256,pd=pow1,pd1=pow2; e1; e1>>=1, ++pd, ++pd1)
        if (leqDouble(*pd, fcd.d))
        {
          e += e1;
          fcd.d = mulDouble(fcd.d, *pd1);
        }
      if (e < NUM_DIGITS)
        dec = e;
      else
        sign = '+';
    }

    /*----------------------------------------------------------------*/
    /* Extract decimal digits of mantissa                             */
    /*----------------------------------------------------------------*/
    for (i=0;i<NUM_DIGITS;++i,--dec)
    {
      Int32 d = doubleToLong(fcd.d);
      *s++ = d + '0';
      if (!dec)
        *s++ = '.';
      fcd.d = subDouble(fcd.d, longToDouble(d));
      fcd.d = mulDouble(fcd.d, 10.0);
    }

    /*----------------------------------------------------------------*/
    /* Remove trailing zeros and decimal point                        */
    /*----------------------------------------------------------------*/
    while (s[-1] == '0')
      *--s = '\0';
    if (s[-1] == '.')
      *--s = '\0';

    /*----------------------------------------------------------------*/
    /* Append exponent                                                */
    /*----------------------------------------------------------------*/
    if (sign)
    {
      *s++ = 'e';
      *s++ = sign;
      StrIToA(s, e);
    }
    else
      *s = '\0';
  }
}

/**********************************************************************/
/* internal helper function for printing                              */
/**********************************************************************/
#define HEXDIGIT(c) ((c) < 10 ? (c)+'0' : (c)+('a'-10))
static void writeVector(PTR p) SEC(IO);
static void writeString(PTR p) SEC(IO);
static void writeSEXP(PTR p)   SEC(IO);
static void writeSEXP(PTR p)
{
  if (p==TRUE)
    outStr("#t");
  else if (p==FALSE)
    outStr("#f");
  else if (p==NOPRINT)
    outStr("#n");
  else if (p==NIL)
    outStr("()");
  else if (p==END_OF_FILE)
    outStr("[eof]");
  else if (p==EMPTY_VEC)
    outStr("#()");
  else if (p==EMPTY_STR)
  {
    if (prtFlags & PRT_ESCAPE)
      outStr("\"\"");
  }
  else if (IS_CHAR(p))
  {
    StrCopy(token,"#\\?");
    token[2] = CHARVAL(p);
    if ((prtFlags & PRT_ESCAPE) && CHARVAL(p) < ' ')
    {
      unsigned char c=CHARVAL(p)>>4;
      token[1] = '#';
      token[2] = HEXDIGIT(c);
      c=CHARVAL(p)&0x0f;
      token[3] = HEXDIGIT(c);
      token[4] = '\0';
    }
    outStr(prtFlags & PRT_ESCAPE ? token : token+2);
  }
  else if (IS_INT(p))
  {
    if (INTVAL(p) >= 0)
      StrIToA(token,INTVAL(p));
    else
    {
      outStr("-");
      StrIToA(token,-INTVAL(p));
    }
    outStr(token);
  }
  else if (IS_REAL(p))
  {
    printDouble(getReal(p), token);
    outStr(token);
  }
  else if (IS_ATOM(p))
    outStr(getAtom(p));
  else if (IS_NE_VEC(p))
  {
    if (depth >= LispMePrefs.printDepth)
    {
      /*--------------------------------------------------------------*/
      /* don't let recursion go to deep                               */
      /*--------------------------------------------------------------*/
      outStr("[deep]");
      return;
    }
    ++depth;
    outStr("#(");
    writeVector(p);
    outStr(")");
    --depth;
  }
  else if (IS_STRING(p))
    writeString(p);
  else if (IS_CONS(p))
  {
    /*----------------------------------------------------------------*/
    /* Special cases                                                  */
    /*----------------------------------------------------------------*/
    switch (car(p))
    {
      case CLOS_TAG:
        outStr("[clos ");
      writeTrail:
        writeSEXP(cadr(p));
        outStr("]");
        break;

      case CONT_TAG:
        outStr("[cont]");
        break;

      case RCPD_TAG:
      case RCPF_TAG:
        outStr("[prom]");
        break;

      case PRTI_TAG:
        outStr("[inport ");
        goto writeTrail;
        break;

      case PRTO_TAG:
        outStr("[outport]");
        break;

      case MACR_TAG:
        outStr("[macro]");
        break;

      case CPLX_TAG:
        /*------------------------------------------------------------*/
        /* Output complex number: <real>(+|-)<imag>i                  */
        /*------------------------------------------------------------*/
        fcdRe.d = getReal(cadr(p));
        fcdIm.d = getReal(cddr(p));
        if (isNan(fcdRe) || isNan(fcdIm))
          outStr("[nan]");
        else if (isInf(fcdRe) || isInf(fcdIm))
          outStr("[cinf]");
        else
        {
          if (!FlpIsZero(fcdRe))
          {
            printDouble(fcdRe.d, token);
            outStr(token);
          }
          outStr(FlpGetSign(fcdIm) ? "-" : "+");
          FlpSetPositive(fcdIm);
          if (!eqDouble(fcdIm.d, 1.0))
          {
            printDouble(fcdIm.d, token);
            outStr(token);
          }
          outStr("i");
        }
        break;

      default:
        if (LispMePrefs.printQuotes && IS_PAIR(cdr(p)) && cddr(p) == NIL)
        {
          if (car(p) == QUOTE) {
            outStr("'");
          writeRest:
            writeSEXP(cadr(p)); return;
          } else if (car(p) == QUASIQUOTE) {
            outStr("`"); goto writeRest;
          } else if (car(p) == UNQUOTE) {
            outStr(","); goto writeRest;
          } else if (car(p) == UNQUOTESPLICING) {
            outStr(",@"); goto writeRest;
          }
        }
        if (depth >= LispMePrefs.printDepth)
        {
          /*----------------------------------------------------------*/
          /* don't let recursion go to deep                           */
          /*----------------------------------------------------------*/
          outStr("[deep]");
          return;
        }
        ++depth;
        outStr("(");
        while (IS_PAIR(p))
        {
          writeSEXP(car(p));

          /*----------------------------------------------------------*/
          /* last list element?                                       */
          /*----------------------------------------------------------*/
          if ((p = cdr(p)) == NIL)
            break;
          outStr(" ");

          /*----------------------------------------------------------*/
          /* dotted pair at end of list                               */
          /*----------------------------------------------------------*/
          if (!IS_PAIR(p))
          {
            outStr(". ");
            writeSEXP(p);
          }
        }
        outStr(")");
        --depth;
    }
  }
  else
    ErrThrow(ERR_M2_INVALID_PTR);
}

/**********************************************************************/
/* helper function to print a vector                                  */
/**********************************************************************/
static void writeVector(PTR p)
{
  UInt16    index;
  MemHandle recHand;
  PTR*      pel;
  Int16     num;

  DmFindRecordByID(dbRef, UID_VEC(p), &index);
  recHand = DmQueryRecord(dbRef,index);
  pel = (PTR*)(MemHandleLock(recHand));

  ErrTry {
    num = MemHandleSize(recHand)/2;
    while (num--)
    {
      writeSEXP(*pel++);
      if (num)
        outStr(" ");
    }
  } ErrCatch(err) {
    /*----------------------------------------------------------------*/
    /* Clean up in case of truncated output                           */
    /*----------------------------------------------------------------*/
    MemHandleUnlock(recHand);
    ErrThrow(err);
  } ErrEndCatch
  MemHandleUnlock(recHand);
}

/**********************************************************************/
/* helper function to print a string                                  */
/**********************************************************************/
static void writeString(PTR p)
{
  UInt16    index;
  MemHandle recHand;
  char*     pc;
  Int16     num;

  DmFindRecordByID(dbRef, UID_STR(p), &index);
  recHand = DmQueryRecord(dbRef,index);
  StrCopy(token,"?");
  StrCopy(token+2,"#xx");
  if (prtFlags & PRT_ESCAPE)
    outStr("\"");

  pc  = MemHandleLock(recHand);
  ErrTry {
    num = MemHandleSize(recHand);
    while (num--)
    {
      token[0] = *pc++;
      if (prtFlags & PRT_ESCAPE)
      {
        if (token[0] < ' ')
        {
          unsigned char c=token[0]>>4;
          token[3] = HEXDIGIT(c);
          c=token[0]&0x0f;
          token[4] = HEXDIGIT(c);
          outStr(token+2);
        }
        else if (token[0] == '\\' || token[0] == '"' || token[0] == '#')
        {
          outStr("\\");
          outStr(token);
        }
        else
          outStr(token);
      }
      else
      {
        if (token[0])
          outStr(token);
        else
        {
          /*----------------------------------------------------------*/
          /* '\0' byte, write ' ' instead and clear afterwards to     */
          /* advance output pointer                                   */
          /*----------------------------------------------------------*/
          outStr(" ");
          currPtr[-1] = '\x00';
        }
      }
    }
    if (prtFlags & PRT_ESCAPE)
      outStr("\"");
  } ErrCatch(err) {
    /*----------------------------------------------------------------*/
    /* Clean up in case of truncated output                           */
    /*----------------------------------------------------------------*/
    MemHandleUnlock(recHand);
    ErrThrow(err);
  } ErrEndCatch
  MemHandleUnlock(recHand);
}

/**********************************************************************/
/* print an SEXPR using flags for destination and options             */
/**********************************************************************/
void printSEXP(PTR p, UInt8 flags)
{
  char* start;
  depth    = 0;
  prtFlags = flags;
  outLen   = 0;
  switch (prtFlags & PRT_DEST)
  {
    case PRT_OUTFIELD:
      /*--------------------------------------------------------------*/
      /* Don't print #n (the non-printing-object) on "stdout"         */
      /*--------------------------------------------------------------*/
      if (p == NOPRINT)
        return;
      outLen  = outPos;
      gMaxLen = outpSize-1;
      if (outPos == 0)
       pMemGlobal->printBufOverflow = false;
      if (!pMemGlobal->printBufOverflow)
      {
        start = (char*)MemHandleLock(outHandle);
        currPtr = start + outPos;
      }
      break;

    case PRT_MESSAGE:
      gMaxLen = MSG_OUTPUT_SIZE;
      currPtr = start = msg;
      break;

    case PRT_MEMO:
      gMaxLen = MEMO_OUTPUT_SIZE;
      currPtr = start = msg;
      break;
  }

  if (!pMemGlobal->printBufOverflow ||
      (flags & PRT_DEST) != PRT_OUTFIELD)
  {
    ErrTry {
      if ((flags & PRT_AUTOLF) && outPos)
        outStr("\n");
      writeSEXP(p);
      YIELD();
      if (flags & PRT_SPACE)
        outStr(" ");
    }
    ErrCatch(err) {
      if (err == ERR_O5_OUTPUT_TRUNC)
      {
        /*------------------------------------------------------------*/
        /* Abbreviate output with ... and terminate it                */
        /*------------------------------------------------------------*/
        *currPtr++ = charEllipsis;
        pMemGlobal->printBufOverflow = true;
      }
      else
        ErrThrow(err);
    } ErrEndCatch

    *currPtr = '\0';

    if ((flags & PRT_DEST) == PRT_OUTFIELD)
    {
      outPos = currPtr - start;
      MemHandleUnlock(outHandle);
      FldSetTextHandle(outField, outHandle);
      if (!pMemGlobal->ownGUI && actContext == -1)
      {
        /*------------------------------------------------------------*/
        /* Update in "stdio" mode only, i.e. no GUI is active         */
        /*------------------------------------------------------------*/
        FldDrawField(outField);
        updateScrollBar();
      }
      if (pMemGlobal->printBufOverflow)
      {
        displayError(ERR_O5_OUTPUT_TRUNC);
        if (running)
          GrabMem();
      }
    }
  }
}

/**********************************************************************/
/* interpret hex digit                                                */
/**********************************************************************/
static short hexVal(char c) SEC(IO);
static short hexVal(char c)
{
  if ('0'<= c && c<='9')
    return c-'0';
  else if ('A'<= c && c<='F')
    return c-('A'-10);
  else if ('a'<= c && c<='f')
    return c-('a'-10);
  else
    ErrThrow(ERR_S8_INVALID_HASH);
}

/**********************************************************************/
/* Build real number from components                                  */
/**********************************************************************/
static void buildReal(void) SEC(IO);
static void buildReal(void)
{
  short e1;
  double* pd;

  if (expNegative)
    exponent = -exponent;
  exponent += scale;

  if (exponent > 308)
    doubleVal = 1e999;
  else if (exponent < -324)
    doubleVal = 0.0;
  else
    for (e1 = 256, pd = (exponent>=0 ? pow1 : pow2), exponent = abs(exponent);
         e1;
         e1>>=1, ++pd)
      if (exponent >= e1)
      {
        exponent -= e1;
        doubleVal = mulDouble(doubleVal, *pd);
      }
  if (negative)
    FlpSetNegative(doubleVal);
}

/**********************************************************************/
/* get a token                                                        */
/**********************************************************************/
static void scan(void) SEC(IO);
static void scan(void)
{
  int            state     = asSTART;
  unsigned char* p         = token;
  char           c, cc;

  negative    = 0;
  longVal     = 0;
  realState   = rsREAL;
  doubleVal   = 0.0;
  expNegative = 0;
  exponent    = 0;
  scale       = 0;

  while (true)
  {
    c = *currPtr++;

    /*----------------------------------------------------------------*/
    /* Classify input char                                            */
    /*----------------------------------------------------------------*/
    cc = (c&1) ? charClass[((unsigned char)c)>>1]&0x0f :
                 charClass[((unsigned char)c)>>1]>>4;

    /*----------------------------------------------------------------*/
    /* Never read beyond EOF! Pushing back other char classes is done */
    /* in the appropriate automaton states                            */
    /*----------------------------------------------------------------*/
    if (cc == ccEOF)
      --currPtr;
    else if (cc == ccNL)
      ++lineNr;

    switch (state)
    {
      case asSTART: /* non-accepting */
        /*------------------------------------------------------------*/
        /* Initial state, all is possible...                          */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccWS:
          case ccNL:
            break;

          case ccLXA:
          case ccE:
          case ccI:
            onlyWS = false;
            tt = TT_NAME;
            PUSH_CHAR(c)
            state = asID;
            break;

          case ccSEM:
            state = asCOMM;
            break;

          case ccPUN:
            onlyWS = false;
            tt = TT_PUNCT;
            if (c==',' && *currPtr=='@')
              *p = *currPtr++;
            else
              *p = c;
            return;

          case ccDOT:
            onlyWS = false;
            state = asDOT;
            *p = c;
            break;

          case ccD:
            onlyWS = false;
            tt = TT_INT;
            longVal = c-'0';
            state = asN1;
            break;

          case ccPM:
            onlyWS = false;
            negative = c=='-';
            PUSH_CHAR(c)
            state = asN2;
            break;

          case ccDQ:
            onlyWS = false;
            tt = TT_STRING;
            state = asSTRG;
            break;

          case ccEOF:
            /*--------------------------------------------------------*/
            /* Close potential open lists                             */
            /*--------------------------------------------------------*/
            tt = TT_PUNCT;
            *p = ')';
            return;

          case ccHSH:
            onlyWS = false;
            state = asHASH;
            break;

          default:
            onlyWS = false;
            errorChar = c;
            ErrThrow(ERR_S1_INVALID_CHAR);
        }
        break;

      case asID: /* accepting */
        /*------------------------------------------------------------*/
        /* Simple identifier, just append valid chars                 */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccLXA:
          case ccE:
          case ccI:
          case ccD:
          case ccPM:
            PUSH_CHAR(c)
            break;

          default:
            /*--------------------------------------------------------*/
            /* Push back read-ahead and fall thru                     */
            /*--------------------------------------------------------*/
            --currPtr;
          case ccEOF:
            *p = '\0';
            return;
        }
        break;

      case asCOMM: /* non-accepting, but ignore */
        /*------------------------------------------------------------*/
        /* Comment: do nothing but on \n or EOF                       */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccNL:
          case ccEOF:
            state = asSTART;
            break;
        }
        break;

      case asN1: /* accepting, if not IMAG */
        /*------------------------------------------------------------*/
        /* Digit string representable as smallint                     */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccD:
            longVal = 10*longVal + c - '0';
            if (longVal-negative > 16383)
            {
              /*------------------------------------------------------*/
              /* Too large! Convert to double                         */
              /*------------------------------------------------------*/
              tt = TT_FLOAT;
              state = asN1D;
              doubleVal = longToDouble(longVal);
            }
            break;

          case ccE:
            tt = TT_FLOAT;
            doubleVal = longToDouble(longVal);
            state = asN4;
            break;

          case ccI:
          case ccPM:
          case ccAT:
            /*--------------------------------------------------------*/
            /* epsilon trans to asN1D                                 */
            /*--------------------------------------------------------*/
            tt = TT_FLOAT;
            doubleVal = longToDouble(longVal);
            state = asN1D;
            --currPtr;
            break;

          case ccDOT:
            tt = TT_FLOAT;
            doubleVal = longToDouble(longVal);
            state = asN3;
            break;
 
          default:
            --currPtr;
          case ccEOF:
            if (realState == rsIMAG)
              ErrThrow(ERR_S10_INVALID_COMP);
            return;
        }
        break;

      case asN1D:
        /*------------------------------------------------------------*/
        /* Digit string to large for smallint                         */
        /*------------------------------------------------------------*/
        switch (cc) /* accepting */
        {
          case ccD:
            doubleVal = mulDouble(doubleVal, 10.0);
            doubleVal = addDouble(doubleVal, longToDouble(c-'0'));
            break;

          case ccE:
            state = asN4;
            break;

          case ccPM:
            state = asN8;
            break;

          case ccI:
            state = asN9;
            break;

          case ccAT:
            state = asN10;
            break;

          case ccDOT:
            state = asN3;
            break;
 
          default:
            --currPtr;
          case ccEOF:
            if (realState == rsIMAG)
              ErrThrow(ERR_S10_INVALID_COMP);
            buildReal();
            return;
        }
        break;

      case asN2: /* accepting */
        /*------------------------------------------------------------*/
        /* Seen + or - so far, could be number or symbol              */
        /*------------------------------------------------------------*/
        switch (cc) /* accepting */
        {
          case ccD:
            /*--------------------------------------------------------*/
            /* OK, it's a number                                      */
            /* fake epsilon trans. to avoid duplicate code            */
            /*--------------------------------------------------------*/
            tt = TT_INT;
            state = asN1;
            --currPtr;
            break;

          case ccDOT:
            /*--------------------------------------------------------*/
            /* This must be a real number                             */
            /*--------------------------------------------------------*/
            tt = TT_FLOAT;
            state = asN5;
            break;

          case ccI:
            /*--------------------------------------------------------*/
            /* +i or -i                                               */
            /*--------------------------------------------------------*/
            doubleRe    = 0.0;
          makeImagUnit:
            doubleVal   = 1.0;
            expNegative = 0;
            exponent    = 0;
            scale       = 0;
            buildReal();
            tt          = TT_FLOAT;
            realState   = rsIMAG;
            return;
            break;

          case ccE:
          case ccLXA:
          case ccPM:
            /*--------------------------------------------------------*/
            /* It's a symbol                                          */
            /*--------------------------------------------------------*/
            tt = TT_NAME;
            PUSH_CHAR(c)
            state = asID;
            break;

          case ccAT:
            ErrThrow(ERR_S10_INVALID_COMP);

          default:
            /*--------------------------------------------------------*/
            /* Single + or - , also a symbol                          */
            /*--------------------------------------------------------*/
            --currPtr;
          case ccEOF:
            tt = TT_NAME;
            *p = '\0';
            return;
        }
        break;

      case asN3: /* accepting, if not IMAG */
        /*------------------------------------------------------------*/
        /* Digit string after decimal point                           */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccD:
            --scale;
            doubleVal = mulDouble(doubleVal, 10.0);
            doubleVal = addDouble(doubleVal, longToDouble(c-'0'));
            break;

          case ccE:
            state = asN4;
            break;

          case ccPM:
            state = asN8;
            break;

          case ccI:
            state = asN9;
            break;

          case ccAT:
            state = asN10;
            break;

          default:
            --currPtr;
          case ccEOF:
            if (realState == rsIMAG)
              ErrThrow(ERR_S10_INVALID_COMP);
            buildReal();
            return;
        }
        break;

      case asN4: /* non-accepting */
        /*------------------------------------------------------------*/
        /* Exponent part of real number, no sign or digit seen yet    */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccPM:
            expNegative = c=='-';
            state = asN6;
            break;

          case ccD:
            /*--------------------------------------------------------*/
            /* Epsilon trans to asN7                                  */
            /*--------------------------------------------------------*/
            --currPtr;
            state = asN7;
            break;

          default:
            ErrThrow(ERR_S6_INVALID_REAL);
        }
        break;

      case asN5: /* non-accepting */
        /*------------------------------------------------------------*/
        /* Seen sign and decimal point, must get a digit              */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccD:
            /*--------------------------------------------------------*/
            /* Epsilon trans to asN3                                  */
            /*--------------------------------------------------------*/
            --currPtr;
            state = asN3;
            break;

          default:
            ErrThrow(ERR_S6_INVALID_REAL);
        }
        break;

      case asN6: /* non-accepting */
        /*------------------------------------------------------------*/
        /* Exponent part of real number incl. sign, but no digits yet */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccD:
            /*--------------------------------------------------------*/
            /* Epsilon trans to asN7                                  */
            /*--------------------------------------------------------*/
            --currPtr;
            state = asN7;
            break;

          default:
            ErrThrow(ERR_S6_INVALID_REAL);
        }
        break;

      case asN7: /* accepting, if not IMAG */
        /*------------------------------------------------------------*/
        /* Digit string of exponent part of real number               */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccD:
            exponent = 10*exponent + c - '0';
            break;

          case ccI:
            state = asN9;
            break;

          case ccPM:
            state = asN8;
            break;

          case ccAT:
            state = asN10;
            break;

          default:
            --currPtr;
          case ccEOF:
            if (realState == rsIMAG)
              ErrThrow(ERR_S10_INVALID_COMP);
            buildReal();
            return;
        }
        break;

      case asN8: /* non-accepting */
        /*------------------------------------------------------------*/
        /* got sign after a real => begin of complex numer            */
        /* store real part and reinit floating point scanner          */
        /*------------------------------------------------------------*/
        if (realState != rsREAL)
          ErrThrow(ERR_S10_INVALID_COMP);
        realState   = rsIMAG;
        buildReal();
        doubleRe    = doubleVal;
        doubleVal   = 0.0;
        longVal     = 0;
        expNegative = 0;
        exponent    = 0;
        scale       = 0;
        negative    = currPtr[-2] == '-'; /* already read + inc of currPtr! */
        switch (cc)
        {
          case ccD:
            --currPtr;
            state = asN1;
            break;

          case ccDOT:
            state = asDOT;
            break;

          case ccI:
            goto makeImagUnit;

          default:
            ErrThrow(ERR_S10_INVALID_COMP);
        }
        break;

      case asN9: /* accepting */
        /*------------------------------------------------------------*/
        /* i after a real number, now check real scanner automaton    */
        /*------------------------------------------------------------*/
        if (cc!=ccEOF)
          --currPtr;
        switch (realState)
        {
          case rsREAL:
            /*--------------------------------------------------------*/
            /* Imaginary with no real part                            */
            /*--------------------------------------------------------*/
            buildReal();
            doubleRe    = 0.0;
            realState   = rsIMAG;
            return;

          case rsIMAG:
            /*--------------------------------------------------------*/
            /* Complex number complete!                               */
            /*--------------------------------------------------------*/
            buildReal();
            realState   = rsIMAG;
            return;

          case rsPOL:
            ErrThrow(ERR_S10_INVALID_COMP);
        }
        break;

      case asN10: /* non-accepting */
        /*------------------------------------------------------------*/
        /* @ after a real number, now check real scanner automaton    */
        /*------------------------------------------------------------*/
        switch (realState)
        {
          case rsREAL:
            /*--------------------------------------------------------*/
            /* Start scanning polar representation                    */
            /*--------------------------------------------------------*/
            buildReal();
            doubleRe    = doubleVal;
            realState   = rsPOL;
            doubleVal   = 0.0;
            longVal     = 0;
            expNegative = 0;
            negative    = 0;
            exponent    = 0;
            scale       = 0;
            switch (cc)
            {
              case ccD:
                doubleVal = longToDouble(c-'0');
                state = asN1D;
                break;
      
              case ccDOT:
                state = asDOT;
                break;
      
              case ccPM:
                negative = c=='-';
                state = asN11;
                break;
      
              default:
                ErrThrow(ERR_S10_INVALID_COMP);
            }
            break;

          case rsIMAG:
          case rsPOL:
            ErrThrow(ERR_S10_INVALID_COMP);
        }
        break;

      case asN11: /* non-accepting */
        /*------------------------------------------------------------*/
        /* Seen <real>@<sign>, must get unsigned number               */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccD:
            doubleVal = longToDouble(c-'0');
            state = asN1D;
            break;

          case ccDOT:
            state = asN5;
            break;

          default:
            ErrThrow(ERR_S10_INVALID_COMP);
        }
        break;

      case asDOT: /* accepting, if not IMAG */
        /*------------------------------------------------------------*/
        /* Could be punctuation or start of number                    */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccD:
            tt = TT_FLOAT;
            --currPtr;
            state = asN3;
            break;

          default:
            --currPtr;
          case ccEOF:
            if (realState != rsREAL)
              ErrThrow(ERR_S10_INVALID_COMP);
            tt = TT_PUNCT;
            return;
        }
        break;

      case asHASH: /* non-accepting */
        /*------------------------------------------------------------*/
        /* #t, #f, #n, #(, #\char or ##xx                             */
        /*------------------------------------------------------------*/
        switch (c)
        {
          case 't': case 'T':
            tt = TT_TRUE;
            return;

          case 'f': case 'F':
            tt = TT_FALSE;
            return;

          case 'n': case 'N':
            tt = TT_NOPRINT;
            return;
 
          case '(':
            tt = TT_PUNCT;
            *p = '['; /* indicates start of vector */
            return;

          case '\\':
            tt = TT_CHAR;
            state = asCHAR;
            break;

          case '#':
            tt = TT_CHAR;
            state = asCHH1;
            break;

          default:
            ErrThrow(ERR_S8_INVALID_HASH);
        }
        break;

      case asCHAR: /* accepting */
        /*------------------------------------------------------------*/
        /* accept any but EOF                                         */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccEOF:
            ErrThrow(ERR_S8_INVALID_HASH);
          default:
            PUSH_CHAR(c);
            return;
        }
        break;

      case asSTRG: /* non-accepting */
        /*------------------------------------------------------------*/
        /* Normal characters in string, accept all but \ " # EOF      */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccBSL:
            state = asSTRE;
            break;

          case ccHSH:
            state = asSTH1;
            break;

          case ccDQ:
            stLen = p-token;
            return;

          case ccEOF: 
            ErrThrow(ERR_S9_UNTERM_STRING);

          default:
            PUSH_CHAR(c)
            break;
        }
        break;

      case asSTRE: /* non-accepting */
        /*------------------------------------------------------------*/
        /* Escape in string                                           */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccEOF: 
            ErrThrow(ERR_S9_UNTERM_STRING);

          default:
            PUSH_CHAR(c)
            state = asSTRG;
            break;
        }
        break;

      case asSTH1: /* non-accepting */
        /*------------------------------------------------------------*/
        /* # in string begins hex sequence                            */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          short val;

          case ccEOF: 
            ErrThrow(ERR_S9_UNTERM_STRING);

          default:
            val = hexVal(c) << 4;
            PUSH_CHAR(val)
            state = asSTH2;
            break;
        }
        break;

      case asSTH2: /* non-accepting */
        /*------------------------------------------------------------*/
        /* second char of hex sequence                                */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccEOF: 
            ErrThrow(ERR_S9_UNTERM_STRING);

          default:
            p[-1] |= hexVal(c);
            state = asSTRG;
            break;
        }
        break;

      case asCHH1: /* non-accepting */
        /*------------------------------------------------------------*/
        /* ## starts hex sequence for single characters               */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          short val;

          case ccEOF: 
            ErrThrow(ERR_S8_INVALID_HASH);

          default:
            val = hexVal(c) << 4;
            PUSH_CHAR(val)
            state = asCHH2;
            break;
        }
        break;

      case asCHH2: /* accepting */
        /*------------------------------------------------------------*/
        /* second char of hex sequence in character                   */
        /*------------------------------------------------------------*/
        switch (cc)
        {
          case ccEOF: 
            ErrThrow(ERR_S8_INVALID_HASH);

          default:
            p[-1] |= hexVal(c);
            return;
        }
        break;
    } /* switch state */
  } /* char loop */
}

/**********************************************************************/
/* Internal helper functions                                          */
/**********************************************************************/
static void getSEXPList(PTR* e) SEC(IO);
static PTR getSEXP(void) SEC(IO);
static PTR getSEXP(void)
{
  PTR res;
  Boolean isVec;
  int     macroNr;
  static char* ptrPos;
  static const char* const macroChars = "'`,@";

  CHECKSTACK(macroNr);

  if (tt == TT_TRUE)
    return TRUE;
  else if (tt == TT_FALSE)
    return FALSE;
  else if (tt == TT_NOPRINT)
    return NOPRINT;
  else if (tt == TT_CHAR)
    return MKCHAR(token[0]);
  else if (tt == TT_STRING)
    return makeString(stLen, token, 0, 0, NIL);
  else if (tt == TT_INT)
    return MKINT(negative?-longVal:longVal);
  else if (tt == TT_FLOAT)
  {
    switch (realState)
    {
      case rsREAL:
        return allocReal(doubleVal);
      case rsIMAG:
        return storeNum(doubleRe,doubleVal);
      case rsPOL:
        return makePolar(doubleRe,doubleVal);
    }
  }
  else if (tt == TT_NAME)
  {
    if (!caseSens)
      StrToLower(token, token);
    return findAtom(token);
  }
  else if (*token == '(' || *token == '[')
  {
    /*----------------------------------------------------------------*/
    /* Start of pair, try reading list or vector                      */
    /*----------------------------------------------------------------*/
    isVec = *token == '[';
    res = NIL;
    PROTECT(res);
    getSEXPList(&res);
    UNPROTECT(res);
    return isVec ? makeVector(listLength(res), res, true) : res;
  }
  else if (tt == TT_PUNCT && (ptrPos = StrChr(macroChars,*token)))
  {
    /*----------------------------------------------------------------*/
    /* quote, quasiquote, unquote or unquote-splicing,                */
    /* build 2-list (macro-expansion expr)                            */
    /*----------------------------------------------------------------*/
    macroNr = ptrPos-macroChars;
    scan();
    return cons(keyWords[macroNr],
                 cons(getSEXP(),NIL));
  }
  else
    ErrThrow(ERR_S2_INVALID_SEXP);
}

static void getSEXPList(PTR* e)
{
  scan();
  if (*token == ')' && tt == TT_PUNCT)
    return;
  else
  {
  loop:
    *e = cons(getSEXP(),NIL);
    scan();
    if (*token == '.' && tt == TT_PUNCT)
    {
      /*--------------------------------------------------------------*/
      /* Dotted pair, read cdr as single SEXPR, assure a closing      */
      /* parenthesis follows                                          */
      /*--------------------------------------------------------------*/
      scan();
      cdr(*e) = getSEXP();
      scan();
      if (*token != ')' || tt != TT_PUNCT)
        ErrThrow(ERR_S3_MULTI_DOT);
    }
    else if (*token == ')' && tt == TT_PUNCT)
      return;
    else
    {
      /*--------------------------------------------------------------*/
      /* List notation                                                */
      /*--------------------------------------------------------------*/
      e = &cdr(*e);
      goto loop;
    }
  }
}

/**********************************************************************/
/* read an SEXPR                                                      */
/**********************************************************************/
PTR readSEXP(char* src)
{
  lineNr  = 1;
  onlyWS = true;
  startPtr = currPtr = src;
  scan();
  W = getSEXP();
  startPtr = 0;
  return W;
}

/**********************************************************************/
/* load a memo (enclose all definitions in BEGIN block)               */
/**********************************************************************/
PTR loadMemo(char* src)
{
  PTR res;
  lineNr  = 1;
  onlyWS = true;
  startPtr = currPtr = src;
  PROTECT(res);
  res = cons(BEGIN,NIL);
  getSEXPList(&cdr(res));
  UNPROTECT(res);
  startPtr = 0;
  return res;
}
