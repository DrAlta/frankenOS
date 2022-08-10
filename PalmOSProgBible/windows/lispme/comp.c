/**********************************************************************/
/*                                                                    */
/* comp.c:  LISP to SECD compiler                                     */
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
/* includes                                                           */
/**********************************************************************/
#include "store.h"
#include "comp.h"
#include "lispme.h"
#include "vm.h"
#include "util.h"

/**********************************************************************/
/* Hacks to fit variable arity and constant into opcode table         */
/**********************************************************************/
#define VARIABLE          42
#define EMPTY_LIST        57
#define EMPTY_STRING      69

/**********************************************************************/
/* Heuristic: More than n macro calls per compile indicate an error   */
/**********************************************************************/
#define MAX_MACRO_PER_COMPILE 200

/**********************************************************************/
/* Static functions                                                   */
/**********************************************************************/
static PTR   location(PTR var, PTR env, Boolean throwErr) SEC(COMP);
static void  comp(PTR expr, PTR names, PTR* code) SEC(COMP);
static void  complist(PTR exlist, PTR names, PTR* code, PTR init, int opc) SEC(COMP);
static int   buildIn(PTR func, PTR args, Int16 argl, PTR names, PTR* code) SEC(COMP);
static void  checkUnique(PTR vars) SEC(COMP);
static PTR   vars(PTR body) SEC(COMP);
static PTR   exprs(PTR body) SEC(COMP);
static short lambdaNames(PTR orgArgs, PTR* newArgs) SEC(COMP);
static PTR   reverse(PTR list) SEC(COMP);
static void  compseq(PTR exlist, PTR names, PTR* code) SEC(COMP);
static void  addDefine(PTR* let, PTR body, Boolean isMacro) SEC(COMP);
static int   resolveDefineList(PTR expr, PTR* newExpr) SEC(COMP);
static int   resolveInnerDefine(PTR expr, PTR* newExpr) SEC(COMP);
static PTR   value(PTR loc, PTR env) SEC(COMP);

/**********************************************************************/
/* local data                                                         */
/**********************************************************************/
static PTR joinCont;
static PTR rtnCont;
static int qqNest;
static int macroCalls;

static struct BuiltIns
{
  char*          func;
  PTR            atom;
  char           arity;
  char           swap;
  unsigned char  opcodes[4];
} builtIns[] =
{
  /*------------------------------------------------------------------*/
  /* First all unary function also available with more or less args   */
  /*------------------------------------------------------------------*/
  {"-",                0, 1, false, SUB,  0,    LDC,  NOP},
  {"/",                0, 1, false, DIV,  1,    LDC,  NOP},
  {"display",          0, 1, false, DSPL, 0,    LDC,  NOP},
  {"write",            0, 1, false, WRIT, 0,    LDC,  NOP},
  {"dir",              0, 1, false, MDIR, NOP},
  {"atan",             0, 1, false, ATAN, NOP},

  /*------------------------------------------------------------------*/
  /* Now the variable arity functions                                 */
  /*------------------------------------------------------------------*/
  {"+",                0, VARIABLE, false, ADD,  0},
  {"*",                0, VARIABLE, false, MUL,  1},
  {"append",           0, VARIABLE, false, APND, EMPTY_LIST},
  {"string-append",    0, VARIABLE, false, SAPP, EMPTY_STRING},

  /*------------------------------------------------------------------*/
  /* And now the fixed arity functions in any order                   */
  /*------------------------------------------------------------------*/
  {"gensym",           0, 0, false, GSYM, NOP},
  {"frm-get-focus",    0, 0, false, FRGF, NOP},
  {"dir",              0, 0, false, MDIR, 0, LDC, NOP},
  {"date-time",        0, 0, false, GTIM, NOP},
  {"hb-dir",           0, 0, false, HBLD, NOP},
  {"gc",               0, 0, false, DOGC, NOP},

  {"boolean?",         0, 1, false, BOOL, NOP},
  {"pair?",            0, 1, false, PAIR, NOP},
  {"null?",            0, 1, false, NUL,  NOP},
  {"number?",          0, 1, false, CPLP, NOP},
  {"complex?",         0, 1, false, CPLP, NOP},
  {"real?",            0, 1, false, REAP, NOP},
  {"integer?",         0, 1, false, INTP, NOP},
  {"char?",            0, 1, false, CHR,  NOP},
  {"string?",          0, 1, false, STRG, NOP},
  {"symbol?",          0, 1, false, SYMB, NOP},
  {"none?",            0, 1, false, NONE, NOP},
  {"macro?",           0, 1, false, MACP, NOP},
  {"not",              0, 1, false, NOT,  NOP},
  {"car",              0, 1, false, CAR,  NOP},
  {"cdr",              0, 1, false, CDR,  NOP},
  {"caar",             0, 1, false, CAR,  CAR,  NOP},
  {"cadr",             0, 1, false, CAR,  CDR,  NOP},
  {"cdar",             0, 1, false, CDR,  CAR,  NOP},
  {"cddr",             0, 1, false, CDR,  CDR,  NOP},
  {"caaar",            0, 1, false, CAR,  CAR,  CAR,  NOP},
  {"caadr",            0, 1, false, CAR,  CAR,  CDR,  NOP},
  {"cadar",            0, 1, false, CAR,  CDR,  CAR,  NOP},
  {"caddr",            0, 1, false, CAR,  CDR,  CDR,  NOP},
  {"cdaar",            0, 1, false, CDR,  CAR,  CAR,  NOP},
  {"cdadr",            0, 1, false, CDR,  CAR,  CDR,  NOP},
  {"cddar",            0, 1, false, CDR,  CDR,  CAR,  NOP},
  {"cdddr",            0, 1, false, CDR,  CDR,  CDR,  NOP},
  {"error",            0, 1, false, UERR, NOP},
  {"force",            0, 1, false, AP0,  NOP},
  {"string-length",    0, 1, false, SLEN, NOP},
  {"string->list",     0, 1, false, S2L,  NOP},
  {"list->string",     0, 1, false, L2S,  NOP},
  {"vector->list",     0, 1, false, V2L,  NOP},
  {"list->vector",     0, 1, false, L2V,  NOP},
  {"char->integer",    0, 1, false, C2I,  NOP},
  {"integer->char",    0, 1, false, I2C,  NOP},
  {"procedure?",       0, 1, false, PROC, NOP},
  {"continuation?",    0, 1, false, CONT, NOP},
  {"promise?",         0, 1, false, PROM, NOP},
  {"object->string",   0, 1, false, O2S,  NOP},
  {"string->object",   0, 1, false, S2O,  NOP},
  {"text",             0, 1, false, TEXT, NOP},
  {"read",             0, 1, false, READ, NOP},
  {"read-char",        0, 1, false, REAC, NOP},
  {"read-line",        0, 1, false, REDL, NOP},
  {"message",          0, 1, false, UINF, NOP},
  {"random",           0, 1, false, RAND, NOP},
  {"sqrt",             0, 1, false, SQRT, NOP},
  {"sin",              0, 1, false, SIN,  NOP},
  {"cos",              0, 1, false, COS,  NOP},
  {"tan",              0, 1, false, TAN,  NOP},
  {"asin",             0, 1, false, ASIN, NOP},
  {"acos",             0, 1, false, ACOS, NOP},
  {"exp",              0, 1, false, EXP,  NOP},
  {"log",              0, 1, false, LOG,  NOP},
  {"sinh",             0, 1, false, SINH, NOP},
  {"cosh",             0, 1, false, COSH, NOP},
  {"tanh",             0, 1, false, TANH, NOP},
  {"asinh",            0, 1, false, ASIH, NOP},
  {"acosh",            0, 1, false, ACOH, NOP},
  {"atanh",            0, 1, false, ATAH, NOP},
  {"floor",            0, 1, false, FLOR, NOP},
  {"ceiling",          0, 1, false, CEIL, NOP},
  {"truncate",         0, 1, false, TRUN, NOP},
  {"round",            0, 1, false, ROUN, NOP},
  {"integer",          0, 1, false, INTG, NOP},
  {"real-part",        0, 1, false, REPA, NOP},
  {"imag-part",        0, 1, false, IMPA, NOP},
  {"magnitude",        0, 1, false, MAGN, NOP},
  {"angle",            0, 1, false, ANGL, NOP},
  {"disasm",           0, 1, false, DASM, NOP},
  {"wait",             0, 1, false, WAIT, NOP},
  {"open-output-file", 0, 1, false, OOUT, NOP},
  {"open-input-file",  0, 1, false, OINP, NOP},
  {"open-append-file", 0, 1, false, OAPP, NOP},
  {"peek-char",        0, 1, false, PEEK, NOP},
  {"input-port?",      0, 1, false, INPP, NOP},
  {"output-port?",     0, 1, false, OUPP, NOP},
  {"eof-object?",      0, 1, false, EOFO, NOP},
  {"delete-file",      0, 1, false, DELF, NOP},
  {"vector?",          0, 1, false, VECP, NOP},
  {"vector-length",    0, 1, false, VLEN, NOP},
  {"event",            0, 1, false, EVT,  NOP},
  {"bitmap",           0, 1, false, BITM, NOP},
  {"own-gui",          0, 1, false, GUI,  NOP},
  {"fld-get-text",     0, 1, false, FLDG, NOP},
  {"ctl-get-val",      0, 1, false, CTLG, NOP},
  {"lst-get-sel",      0, 1, false, LSTG, NOP},
  {"frm-return",       0, 1, false, FRMQ, NOP},
  {"frm-set-focus",    0, 1, false, FRSF, NOP},
  {"set-resdb",        0, 1, false, RDBS, NOP},
  {"hb-info",          0, 1, false, HBIF, NOP},
  {"hb-addrecord",     0, 1, false, HBAR, NOP},
  {"index->rgb",       0, 1, false, XRGB, NOP},

  {"cons",             0, 2, false, CONS, NOP},
  {"-",                0, 2, false, SUB,  NOP},
  {"/",                0, 2, false, DIV,  NOP},
  {"remainder",        0, 2, false, REM,  NOP},
  {"quotient",         0, 2, false, IDIV, NOP},
  {"eq?",              0, 2, false, EQ,   NOP},
  {"<=",               0, 2, false, LEQ,  NOP},
  {">",                0, 2, false, NOT,  LEQ,  NOP},
  {">=",               0, 2, true,  LEQ,  NOP},
  {"<",                0, 2, true,  NOT,  LEQ,  NOP},
  {"set-car!",         0, 2, true,  SCAR, NOP},
  {"set-cdr!",         0, 2, true,  SCDR, NOP},
  {"eqv?",             0, 2, false, EQV,  NOP},
  {"atan",             0, 2, false, ATN2, NOP},
  {"make-rectangular", 0, 2, false, MKRA, NOP},
  {"make-polar",       0, 2, false, MKPO, NOP},
  {"string-ref",       0, 2, false, SREF, NOP},
  {"string=?",         0, 2, false, SEQ,  NOP},
  {"draw",             0, 2, false, DRAW, NOP},
  {"sound",            0, 2, false, SND,  NOP},
  {"display",          0, 2, true,  DSPL, NOP},
  {"write",            0, 2, true,  WRIT, NOP},
  {"vector-ref",       0, 2, false, VREF, NOP},
  {"make-vector",      0, 2, false, VMAK, NOP},
  {"make-string",      0, 2, false, SMAK, NOP},
  {"read-record",      0, 2, true,  DBRD, NOP},
  {"read-resource",    0, 2, true,  RSRD, NOP},
  {"fld-set-text",     0, 2, false, FLDS, NOP},
  {"ctl-set-val",      0, 2, false, CTLS, NOP},
  {"frm-show",         0, 2, false, FRSH, NOP},
  {"lst-set-sel",      0, 2, false, LSTS, NOP},
  {"lst-get-text",     0, 2, false, LSTT, NOP},
  {"lst-set-list",     0, 2, false, LSTL, NOP},
  {"frm-popup",        0, 2, false, FRMD, NOP},
  {"frm-goto",         0, 2, false, FRMG, NOP},

  {"string-set!",      0, 3, false, SSET, NOP},
  {"vector-set!",      0, 3, false, VSET, NOP},
  {"substring",        0, 3, false, SUBS, NOP},
  {"rect",             0, 3, false, RECT, NOP},
  {"write-record",     0, 3, false, DBWR, NOP},
  {"hb-getfield",      0, 3, false, HBGF, NOP},
  {"hb-getlinks",      0, 3, false, HBGL, NOP},
  {"rgb->index",       0, 3, false, RGBX, NOP},

  {"hb-setfield",      0, 4, false, HBSF, NOP},
  {"set-palette",      0, 4, false, SPAL, NOP},
  {0}
};

/**********************************************************************/
/* Initialize compiler tables                                         */
/**********************************************************************/
void InitCompiler(void)
{
  struct BuiltIns* bi;
  for (bi=builtIns; bi->func; bi++)
    bi->atom = findAtom(bi->func);
}

/**********************************************************************/
/* find a variable in an environment                                  */
/**********************************************************************/
static PTR location(PTR var, PTR env, Boolean throwErr)
{
  short depth = 0;
  short ord   = 0;
  PTR   frame;
  PTR   name;

  for (depth = 0, frame = env;
       frame != NIL;
       ++depth, frame = cdr(frame))
    for (ord = 0, name = car(frame);
         name != NIL;
         ++ord, name = cdr(name))
      if (var == car(name))
        return cons(MKINT(depth),MKINT(ord));
  if (throwErr)
    error1(ERR_C1_UNDEF,var);
  else
    return NIL;
}

/**********************************************************************/
/* compile an expression list                                         */
/* using init as neutral element and opc as combiner                  */
/* always use neutral element when building a list or string!         */
/**********************************************************************/
static void complist(PTR exlist, PTR names, PTR* code, 
                     PTR init,   int opc)
{
  PTR e;
  CHECKSTACK(e)
  for (e=exlist; e != NIL; e=cdr(e))
  {
    if (!IS_PAIR(e))
      error1(ERR_C6_IMPROPER_ARGS,exlist);
    if (cdr(e) != NIL || opc == CONS ||
        (init == EMPTY_STR && cdr(exlist) == NIL))
      *code = cons(MKINT(opc),*code);
    comp(car(e), names, code);
  }
  if (exlist == NIL || opc == CONS ||
      (init == EMPTY_STR && cdr(exlist) == NIL))
    *code = cons(MKINT(LDC),cons(init,*code));
}

/**********************************************************************/
/* build code for built-in function                                   */
/**********************************************************************/
static int buildIn(PTR func, PTR args, Int16 argl, PTR names, PTR* code)
{
  int              i,j;
  PTR              temp;
  struct BuiltIns* p;

  CHECKSTACK(p);
  /*------------------------------------------------------------------*/
  /* Search function name in table                                    */
  /*------------------------------------------------------------------*/
  for (p=builtIns, i=0; p->func; ++i,++p)
    if (func == p->atom)
    {
      /*--------------------------------------------------------------*/
      /* Found, now check for variable arity                          */
      /*--------------------------------------------------------------*/
      if (p->arity == VARIABLE)
      {
        if (p->opcodes[1] == EMPTY_LIST)
          temp = NIL;
        else if (p->opcodes[1] == EMPTY_STRING)
          temp = EMPTY_STR;
        else
          temp = MKINT(p->opcodes[1]);
        complist(args, names, code, temp, p->opcodes[0]);
        return 1;
      }
      /*--------------------------------------------------------------*/
      /* Found, now check arity                                       */
      /*--------------------------------------------------------------*/
      if (argl != p->arity)
      {
        if (p->arity == 1 && i < 6)
          continue; /* try non-unary versions, too */
        error1(ERR_C2_NUM_ARGS,func);
      }

      /*--------------------------------------------------------------*/
      /* Build opcode list from table (reversed!)                     */
      /*--------------------------------------------------------------*/
      for (j=0; p->opcodes[j] != NOP; j++)
        *code = cons(MKINT(p->opcodes[j]),*code);

      /*--------------------------------------------------------------*/
      /* Build code for subexpressions, possibly swapped              */
      /*--------------------------------------------------------------*/
      if (argl > 0)
      {
        if (p->swap)
          comp(cadr(args), names, code);
        else
          comp(car(args), names, code);
      }
      if (argl > 1)
      {
        if (p->swap)
          comp(car(args), names, code);
        else
          comp(cadr(args), names, code);
      }
      if (argl > 2)
        comp(caddr(args), names, code);
      if (argl > 3)
        comp(cadddr(args), names, code);
      return 1;
    }

  /*------------------------------------------------------------------*/
  /* No function found                                                */
  /*------------------------------------------------------------------*/
  return 0;
}

/**********************************************************************/
/* check, if name list is unique                                      */
/**********************************************************************/
static void checkUnique(PTR vars)
{
  PTR p1,p2;
  for (p1=vars; p1 != NIL; p1=cdr(p1))
    for (p2=cdr(p1); p2 != NIL; p2=cdr(p2))
      if (car(p1) == car(p2))
        error1(ERR_C7_DUPLICATE_NAME,car(p1));
}

/**********************************************************************/
/* check let-list and  extract variable names from                    */
/* a LET or LETREC-block                                              */
/**********************************************************************/
static PTR vars(PTR body)
{
  PTR e;
  PTR res = NIL;

  PROTECT(res)
  for (e=body; e != NIL; e=cdr(e))
  {
    if (!IS_PAIR(e))
      error1(ERR_C8_INV_LET_LIST,e);
    if (listLength(car(e)) != 2)
      error1(ERR_C4_INVALID_LET,car(e));
    if (!IS_ATOM(caar(e)))
      error1(ERR_C3_NOT_SYMBOL,caar(e));
    res = cons(caar(e),res);
  }
  checkUnique(res);
  UNPROTECT(res)
  return res;
}

/**********************************************************************/
/* extract expressions from a LET or LETREC-block                     */
/**********************************************************************/
static PTR exprs(PTR body)
{
  PTR e;
  PTR res = NIL;
  PROTECT(res)
  for (e=body; e != NIL; e=cdr(e))
    res = cons(cadar(e),res);
  UNPROTECT(res)
  return res;
}

/**********************************************************************/
/* build name list for lambda expression                              */
/**********************************************************************/
static short lambdaNames(PTR orgArgs, PTR* newArgs)
{
  short len = 0;
  PTR   oldP=NIL;

  if (IS_CONS(orgArgs) || orgArgs == NIL)
  {
    PTR p = orgArgs;
    while (IS_CONS(p))
    {
      if (!IS_ATOM(car(p)))
        error1(ERR_C5_INVALID_LAMBDA,orgArgs);
      oldP = p;
      p = cdr(p);
      ++len;
    }
    if (p==NIL)
    {
      *newArgs = orgArgs;
      checkUnique(*newArgs);
      return len;
    }
    else if (IS_ATOM(p))
    {
      cdr(oldP) = cons(p,NIL);
      *newArgs = orgArgs;
      checkUnique(*newArgs);
      return -1-len;
    }
    else
      error1(ERR_C5_INVALID_LAMBDA,orgArgs);
  }
  else if (IS_ATOM(orgArgs))
  {
    *newArgs = cons(orgArgs,NIL);
    return -1;
  }
  else
    error1(ERR_C5_INVALID_LAMBDA,orgArgs);
}

/**********************************************************************/
/* reverse a list                                                     */
/**********************************************************************/
static PTR reverse(PTR list)
{
  PTR acc = NIL;
  PROTECT(acc)
  while (list != NIL) {
    if (!IS_PAIR(list))
      error1(ERR_C6_IMPROPER_ARGS,list);
    acc = cons(car(list),acc);
    list = cdr(list);
  }
  UNPROTECT(acc)
  return acc;
}

/**********************************************************************/
/* compile an expression sequence                                     */
/**********************************************************************/
static void compseq(PTR exlist, PTR names, PTR* code)
{
  PTR e = NIL;

  CHECKSTACK(e)

  if (exlist == NIL)
    ErrThrow(ERR_C10_EMPTY_SEQUENCE);

  PROTECT(e)
  for (e=reverse(exlist); e != NIL; e=cdr(e))
  {
    comp(car(e), names, code);
    if (cdr(e) != NIL) {
      *code = cons(MKINT(POP),*code);
    }
  }
  UNPROTECT(e)
}

/**********************************************************************/
/* add a single definition to a letrec binding list                   */
/**********************************************************************/
static void addDefine(PTR* let, PTR body, Boolean isMacro)
{
  if (!IS_CONS(body) || !IS_CONS(cdr(body)))
    error1(ERR_C2_NUM_ARGS, DEFINE);

  if (IS_CONS(car(body)))
  {
    /*----------------------------------------------------------------*/
    /* function definition:                                           */
    /* ((var . args) expr ...) -> (var (lambda args expr ...))        */
    /*----------------------------------------------------------------*/
    *let = cons(cons(caar(body),
                     cons(cons(isMacro ? MLAMBDA : LAMBDA,
                               cons(cdar(body),cdr(body))),
                          NIL)),
           *let);
  }
  else
  {
    if (isMacro)
      error1(ERR_C15_INVALID_DEFINE, body);
    /*----------------------------------------------------------------*/
    /* simple definition:                                             */
    /* (var expr) -> (var expr)                                       */
    /*----------------------------------------------------------------*/
    *let = cons(body,*let);
  }
}

/**********************************************************************/
/* resolve a definition (define ...) or definition list              */
/* (begin (define ...) ...) to equivalent letrec                     */
/**********************************************************************/
static int resolveDefineList(PTR expr, PTR* newExpr)
{
  PTR temp, def;
  int res;

  *newExpr = expr;
  if (IS_CONS(expr))
  {
    temp = cons(LETREC,cons(NIL,cons(NOPRINT,NIL)));
    PROTECT(temp)

    if (car(expr) == DEFINE || car(expr) == MACRO)
    {
      /*--------------------------------------------------------------*/
      /* single definition                                            */
      /*--------------------------------------------------------------*/
      addDefine(&cadr(temp), cdr(expr), car(expr) == MACRO);
      *newExpr = temp;
      res = 1;
    }
    else if (car(expr) == BEGIN && IS_CONS(cadr(expr)) &&
             (caadr(expr) == DEFINE || caadr(expr) == MACRO))
    {
      /*--------------------------------------------------------------*/
      /* (possible) list of definitions                               */
      /*--------------------------------------------------------------*/
      for (def = cdr(expr); def != NIL; def = cdr(def))
      {
        if (!IS_PAIR(def))
          error1(ERR_C15_INVALID_DEFINE, def);
        if (!IS_CONS(car(def)) ||
              (caar(def) != DEFINE && caar(def) != MACRO))
          error1(ERR_C15_INVALID_DEFINE, car(def));
        addDefine(&cadr(temp), cdar(def), caar(def) == MACRO);
      }
      *newExpr = temp;
      res = 1;
    }
    else
      res = 0;
    UNPROTECT(temp)
    return res;
  }
  else
    return 0;
}

/**********************************************************************/
/* resolve a inner definition ((define1 ...) (define2 ...) exp1 exp2) */
/* to equivalent letrec                                               */
/**********************************************************************/
static int resolveInnerDefine(PTR expr, PTR* newExpr)
{
  PTR temp, def, seq;

  *newExpr = expr;
  if (IS_CONS(expr) && IS_CONS(car(expr)) && caar(expr) == DEFINE)
  {
    /*----------------------------------------------------------------*/
    /* OK, list starts with DEFINE, now find first non-DEFINE         */
    /*----------------------------------------------------------------*/
    for (seq = cdr(expr);
         IS_CONS(seq) && IS_CONS(car(seq)) && caar(seq) == DEFINE;
         seq = cdr(seq))
      ;

    if (seq == NIL)
      ErrThrow(ERR_C10_EMPTY_SEQUENCE);

    /*----------------------------------------------------------------*/
    /* Check that no subsequent expression is a define                */
    /*----------------------------------------------------------------*/
    for (temp=seq; temp != NIL; temp = cdr(temp))
      if (IS_CONS(def) && IS_CONS(car(def)) && caar(def) == DEFINE)
        error1(ERR_C9_WRONG_DEFINE, car(def));

    /*----------------------------------------------------------------*/
    /* Build LETREC with first non-DEFINE as body                     */
    /*----------------------------------------------------------------*/
    temp = cons(LETREC,cons(NIL,seq));
    PROTECT(temp)

    /*----------------------------------------------------------------*/
    /* Add each definition to binding part of LETREC                  */
    /*----------------------------------------------------------------*/
    for (def = expr; def != seq; def = cdr(def))
      addDefine(&cadr(temp), cdar(def), false);
    *newExpr = temp;
    UNPROTECT(temp)
    return 1;
  }
  else
    return 0;
}

/**********************************************************************/
/* get value at specified location in environment                     */
/**********************************************************************/
static PTR value(PTR loc, PTR env)
{
  int i,k;
  k = INTVAL(car(loc));
  for (i=0;i<k;i++)
    env = cdr(env);
  env = car(env);
  k = INTVAL(cdr(loc));
  for (i=0;i<k;i++)
    env = cdr(env);
  return car(env);
}

/**********************************************************************/
/* compile an expression, accumulating the resulted code              */
/**********************************************************************/
static void comp(PTR expr, PTR names, PTR* code)
{
  PTR temp;
  PTR temp1;
  PTR temp2;
  PTR temp3;
  PTR temp4;
  short opc;

  CHECKSTACK(opc)

tailCall:
  if (IS_INT(expr)  || IS_REAL(expr)    || IS_SPECIAL(expr) ||
      IS_CHAR(expr) || IS_STRING(expr)  ||
      (IS_CONS(expr) && IS_STAG(car(expr))))
  {
    /*----------------------------------------------------------------*/
    /* Self-evaluating                                                */
    /*----------------------------------------------------------------*/
  selfEval:
    *code = cons(MKINT(LDC),cons(expr,*code));
  }
  else if (IS_VEC(expr))
  {
    if (qqNest)
    {
      /*--------------------------------------------------------------*/
      /* Handle vector template for quasiquote                        */
      /*--------------------------------------------------------------*/
      *code = cons(MKINT(L2V),*code);
      expr=vector2List(expr);
      goto tailCall;
    }
    else
      goto selfEval;
  }
  else if (IS_ATOM(expr))
  {
    if (qqNest)
      goto selfEval;

    /*----------------------------------------------------------------*/
    /* Variable access                                                */
    /*----------------------------------------------------------------*/
    *code = cons(MKINT(LD),cons(location(expr,names,true),*code));
  }
  else 
  {
    PTR   fn   = car(expr);
    PTR   args = cdr(expr);
    Int16 argl = listLength(args);

    if (qqNest)
    {
      /*--------------------------------------------------------------*/
      /* We're inside a quasiquote template                           */
      /*--------------------------------------------------------------*/
      if (fn == QUASIQUOTE)
      {
        if (argl != 1)
          error1(ERR_C2_NUM_ARGS,fn);
        *code = cons(MKINT(CONS),*code);
        ++qqNest;
        comp(fn, names, code);
        complist(args,names,code,NIL,CONS);
        --qqNest;
      }
      else if (fn == UNQUOTE)
      {
        if (argl != 1)
          error1(ERR_C2_NUM_ARGS,fn);
        if (qqNest == 1)
        {
          --qqNest;
          comp(car(args), names, code);
          ++qqNest;
        }
        else
        {
        compUnquote:
          *code = cons(MKINT(CONS),*code);
          --qqNest;
          comp(fn, names, code);
          complist(args,names,code,NIL,CONS);
          ++qqNest;
        }
      }
      else if (fn == UNQUOTESPLICING)
      {
        if (argl != 1)
          error1(ERR_C2_NUM_ARGS,fn);
        if (qqNest == 1)
        {
          if (car(*code) != MKINT(CONS))
            ErrThrow(ERR_C14_SPLICE_NONLIST);
          /*----------------------------------------------------------*/
          /* Replace latest CONS in continuation by APND              */
          /*----------------------------------------------------------*/
          *code = cons(MKINT(APND), cdr(*code));
          --qqNest;
          comp(car(args), names, code);
          ++qqNest;
        }
        else
          goto compUnquote;
      }
      else
      {
        *code = cons(MKINT(CONS),*code);
        comp(fn,names,code);
        complist(args,names,code,NIL,CONS);
      }
    }
    else
    {
      /*--------------------------------------------------------------*/
      /* normal expression                                            */
      /*--------------------------------------------------------------*/
      if (fn == QUOTE)
      {
        /*------------------------------------------------------------*/
        /* quoted expression                                          */
        /*------------------------------------------------------------*/
        if (argl != 1)
          error1(ERR_C2_NUM_ARGS,fn);
        *code = cons(MKINT(LDC), cons(car(args),*code));
      }
      else if (fn == QUASIQUOTE)
      {
        /*------------------------------------------------------------*/
        /* quasiquote expression                                      */
        /*------------------------------------------------------------*/
        if (argl != 1)
          error1(ERR_C2_NUM_ARGS,fn);
        ++qqNest;
        comp(car(args), names, code);
        --qqNest;
      }
      else if (fn == UNQUOTE || fn == UNQUOTESPLICING)
        error1(ERR_C13_INV_UNQUOTE,fn);
      else if (buildIn(fn, args, argl, names, code))
      {
      }
      else if (fn == COND)
      {
        /*------------------------------------------------------------*/
        /* conditional: if cont = (RET) dont use join, but propagate  */
        /* (RET) to subcontrols and use non-pushing SEL (=SELR)       */
        /*------------------------------------------------------------*/
        temp = temp4 = cons(MKINT(CERR),NIL);
        PROTECT(temp)
        PROTECT(temp4)
        temp3 = reverse(args);
        PROTECT(temp3)
        while (temp3 != NIL)
        {
          if (!IS_PAIR(car(temp3)))
            error1(ERR_C11_INVALID_COND,car(temp3));
   
          /*----------------------------------------------------------*/
          /* Continuation after selection                             */
          /*----------------------------------------------------------*/
          if (*code == rtnCont) {
            temp1 = rtnCont;
            opc = SELR;
          } else {
            temp1 = joinCont;
            opc = SEL;
          }
          PROTECT(temp1)
          temp2 = NIL; PROTECT(temp2)
          if (resolveInnerDefine(cdar(temp3), &temp2))
            comp(temp2, names, &temp1);
          else
            compseq(temp2, names, &temp1);
          UNPROTECT(temp2)
          /*----------------------------------------------------------*/
          /* Omit test, if condition is 'else'                        */
          /*----------------------------------------------------------*/
          if (caar(temp3) == ELSE)
            temp = temp4 = temp1;
          else
          {
            temp4 = cons(MKINT(opc),cons(temp1,cons(temp,NIL)));
            if (opc == SEL)
            {
              if (cdr(temp3) == NIL)
                cdddr(temp4) = *code;
              else
                cdddr(temp4) = joinCont;
            }  

            comp(caar(temp3),names,&temp4);
            temp = temp4;
          }
          UNPROTECT(temp1)
          temp3 = cdr(temp3);
        }
        UNPROTECT(temp3)
        UNPROTECT(temp4)
        UNPROTECT(temp)
        *code = temp4;
      }
      else if (fn == CASE)
      {
        /*------------------------------------------------------------*/
        /* conditional: if cont = (RET) dont use join, but propagate  */
        /* (RET) to subcontrols and use non-pushing SEL (=SELR)       */
        /*------------------------------------------------------------*/
        if (argl < 1)
          error1(ERR_C2_NUM_ARGS,fn);
        temp = temp4 = cons(MKINT(CERR),NIL);
        PROTECT(temp)
        PROTECT(temp4)
        temp3 = reverse(cdr(args));
        PROTECT(temp3)
        while (temp3 != NIL)
        {
          if (!IS_PAIR(car(temp3)))
            error1(ERR_C11_INVALID_COND,car(temp3));
   
          /*----------------------------------------------------------*/
          /* Continuation after selection                             */
          /*----------------------------------------------------------*/
          if (*code == rtnCont) {
            temp1 = rtnCont;
            opc = MEMR;
          } else {
            temp1 = joinCont;
            opc = MEM;
          }
          PROTECT(temp1)
          temp2 = NIL; PROTECT(temp2)
          if (resolveInnerDefine(cdar(temp3), &temp2))
            comp(temp2, names, &temp1);
          else
            compseq(temp2, names, &temp1);
          UNPROTECT(temp2)
          /*----------------------------------------------------------*/
          /* Omit test, if condition is 'else', but have to POP test  */
          /*----------------------------------------------------------*/
          if (caar(temp3) == ELSE)
            temp = temp4 = cons(MKINT(POP),temp1);
          else
          {
            temp4 = cons(MKINT(opc),
                          cons(caar(temp3),
                                cons(temp1,
                                      cons(temp,NIL))));
            if (opc == MEM)
            {
              if (cdr(temp3) == NIL)
                cdr(cdddr(temp4)) = *code;
              else
                cdr(cdddr(temp4)) = joinCont;
            }
            temp = temp4;
          }
          UNPROTECT(temp1)
          temp3 = cdr(temp3);
        }
        UNPROTECT(temp3)
        UNPROTECT(temp4)
        UNPROTECT(temp)
        *code = temp4;
        expr=car(args);
        goto tailCall;
      }
      else if (fn == IF)
      {
        /*------------------------------------------------------------*/
        /* [e1] SEL{R} ([e2] JOIN/RTN) ([e3] JOIN/RTN)                */
        /*------------------------------------------------------------*/
        if (argl != 3)
          error1(ERR_C2_NUM_ARGS,fn);
   
        if (*code == rtnCont) {
          temp2 = temp1 = rtnCont;
          opc = SELR;
        } else {
          temp2 = temp1 = joinCont;
          opc = SEL;
        }
        PROTECT(temp1)
        PROTECT(temp2)
        comp(cadr(args),names,&temp1);
        comp(caddr(args),names,&temp2);
        *code = cons(MKINT(opc), cons(temp1,
                      cons(temp2,opc==SEL?*code:NIL)));
        UNPROTECT(temp2)
        UNPROTECT(temp1)
        expr = car(args);
        goto tailCall;
      }
      else if (fn == OR || fn == AND)
      {
        /*------------------------------------------------------------*/
        /* 0 arg: LDC #f/#t                                           */
        /* 1 arg: comp(e)                                             */
        /* n arg: comp(e1) SE(O/A){R} (comp(e2) ... JOIN/RTN)         */
        /*------------------------------------------------------------*/
        if (argl == 0) {
          expr = fn == OR ? FALSE : TRUE;
          goto selfEval;
        }
        temp = reverse(args); temp2 = NIL;
        PROTECT(temp)
        PROTECT(temp2)
        if (*code == rtnCont)
          opc = fn == OR ? SEOR : SEAR;
        else
          opc = fn == OR ? SEO : SEA;
        while (argl--)
        {
          if (*code == rtnCont)
            temp1 = rtnCont;
          else
            temp1 = (argl==0) ? *code : joinCont;
          if (temp2 == NIL)
            temp2 = temp1;
          else
            temp2 = cons(MKINT(opc),cons(temp2,temp1));
          comp(car(temp),names,&temp2);
          temp = cdr(temp);
        }
        UNPROTECT(temp2)
        UNPROTECT(temp)
        *code = temp2;
      }
      else if (fn == LAMBDA)
      {
        /*------------------------------------------------------------*/
        /* lambda expression                                          */
        /*------------------------------------------------------------*/
        short len;
        if (argl < 1)
          error1(ERR_C2_NUM_ARGS,fn);
        temp = rtnCont;
        PROTECT(temp)
        temp1 = cons(NIL,names);
        PROTECT(temp1)
        len = lambdaNames(car(args), &car(temp1));
        temp2 = NIL; PROTECT(temp2)
        if (resolveInnerDefine(cdr(args), &temp2))
          comp(temp2, temp1, &temp);
        else
          compseq(temp2, temp1, &temp);
        UNPROTECT(temp2)
        UNPROTECT(temp1)
        *code = cons(MKINT(LDFC),cons(MKINT(len),cons(temp,*code)));
        UNPROTECT(temp)
      }
      else if (fn == MLAMBDA)
      {
        /*------------------------------------------------------------*/
        /* lambda expression generated by macro                       */
        /*------------------------------------------------------------*/
        short len;
        if (argl < 1)
          error1(ERR_C2_NUM_ARGS,MACRO);
        temp = rtnCont;
        PROTECT(temp)
        temp1 = cons(NIL,names);
        PROTECT(temp1)
        len = lambdaNames(car(args), &car(temp1));
        if (len != 1)
          error1(ERR_C2_NUM_ARGS,MACRO);
        temp2 = NIL; PROTECT(temp2)
        if (resolveInnerDefine(cdr(args), &temp2))
          comp(temp2, temp1, &temp);
        else
          compseq(temp2, temp1, &temp);
        UNPROTECT(temp2)
        UNPROTECT(temp1)
        *code = cons(MKINT(LDM),cons(temp,*code));
        UNPROTECT(temp)
      }
      else if (fn == LET)
      {
        /*------------------------------------------------------------*/
        /* local block                                                */
        /*------------------------------------------------------------*/
        if (argl < 2)
          error1(ERR_C2_NUM_ARGS,fn);
        temp  = rtnCont;                     PROTECT(temp)
        temp1 = cons(vars(car(args)),names); PROTECT(temp1)
        temp2 = NIL;                         PROTECT(temp2)
         if (resolveInnerDefine(cdr(args), &temp2))
          comp(temp2, temp1, &temp);
        else
          compseq(temp2, temp1, &temp);
        UNPROTECT(temp2)
        UNPROTECT(temp1)
        if (*code == rtnCont)
          *code = cons(MKINT(LDF),cons(temp,cons(MKINT(TAP),NIL)));
        else
          *code = cons(MKINT(LDF),cons(temp,cons(MKINT(AP),*code)));
        temp = exprs(car(args));
        complist(temp, names, code, NIL, CONS);
        UNPROTECT(temp)
      }
      else if (fn == LETREC)
      {
        /*------------------------------------------------------------*/
        /* local recursive block                                      */
        /*------------------------------------------------------------*/
        if (argl < 2)
          error1(ERR_C2_NUM_ARGS,fn);
        temp1 = cons(vars(car(args)),names);  PROTECT(temp1)
        temp  = rtnCont;                      PROTECT(temp)
        temp2 = NIL;                          PROTECT(temp2)
        if (resolveInnerDefine(cdr(args), &temp2))
          comp(temp2, temp1, &temp);
        else
          compseq(temp2, temp1, &temp);
        UNPROTECT(temp2)
        if (*code == rtnCont)
          *code = cons(MKINT(LDF),cons(temp,cons(MKINT(RTAP),NIL)));
        else
          *code = cons(MKINT(LDF),cons(temp,cons(MKINT(RAP),*code)));
        temp = exprs(car(args));
        complist(temp, temp1, code, NIL, CONS);
        UNPROTECT(temp)
        UNPROTECT(temp1)
        *code = cons(MKINT(DUM),*code);
   
        /*------------------------------------------------------------*/
        /* the last LETREC block establishes names in top environment */
        /*------------------------------------------------------------*/
        pMemGlobal->newTopLevelNames = vars(car(args));
      }
      else if (fn == LIST)
      {
        complist(args, names, code, NIL, CONS);
      }
      else if (fn == VECTOR)
      {
        *code = cons(MKINT(L2V),*code);
        complist(args, names, code, NIL, CONS);
      }
      else if (fn == BEGIN)
      {
        temp = NIL;
        PROTECT(temp)
        if (resolveInnerDefine(args, &temp))
          comp(temp, names, code);
        else
          compseq(temp, names, code);
        UNPROTECT(temp)
      }
      else if (fn == SET)
      {
        /*------------------------------------------------------------*/
        /* modify variable                                            */
        /*------------------------------------------------------------*/
        if (argl != 2)
          error1(ERR_C2_NUM_ARGS,fn);
        if (!IS_ATOM(car(args)))
          error1(ERR_C3_NOT_SYMBOL,car(args));
        *code = cons(MKINT(ST),cons(location(car(args),names,true),*code));
        expr = cadr(args);
        goto tailCall;
      }
      else if (fn == DELAY)
      {
        /*------------------------------------------------------------*/
        /* build recipe                                               */
        /*------------------------------------------------------------*/
        if (argl != 1)
          error1(ERR_C2_NUM_ARGS,fn);
        temp = cons(MKINT(UPD),NIL);
        PROTECT(temp)
        comp(car(args), names, &temp);
        *code = cons(MKINT(LDE),cons(temp,*code));
        UNPROTECT(temp)
      }
      else if (fn == CALLCC)
      {
        /*------------------------------------------------------------*/
        /* call with current continuation                             */
        /*------------------------------------------------------------*/
        if (argl != 1)
          error1(ERR_C2_NUM_ARGS,fn);
        if (*code == rtnCont)
          temp = cons(MKINT(TAPC),NIL);
        else
          temp = cons(MKINT(APC),*code);
        PROTECT(temp)
        comp(car(args), names, &temp);
        *code = cons(MKINT(LDCT),cons(*code,temp));
        UNPROTECT(temp)
      }
      else if (fn == DEFINE)
        error1(ERR_C9_WRONG_DEFINE, expr);
      else if (fn == EVAL)
      {
        /*------------------------------------------------------------*/
        /* capture the name list                                      */
        /*------------------------------------------------------------*/
        if (argl != 1)
          error1(ERR_C2_NUM_ARGS,fn);
        *code = cons(MKINT(EVA),cons(names,*code));
        expr = car(args);
        goto tailCall;
      }
      else if (fn == APPLY)
      {
        /*------------------------------------------------------------*/
        /* function application to list                               */
        /*------------------------------------------------------------*/
        if (argl != 2)
          error1(ERR_C2_NUM_ARGS,fn);
        if (*code == rtnCont)
          *code = cons(MKINT(TAPY),NIL);
        else
          *code = cons(MKINT(APY),*code);
        comp(car(args),names,code);
        expr = cadr(args);
        goto tailCall;
      }
      else if (IS_ATOM(fn))
      {
        if ((temp3 = location(fn,pMemGlobal->tlNames,false)) != NIL)
        {
          temp4 = value(temp3,pMemGlobal->tlVals);
          if (IS_MACRO(temp4))
          {
            /*--------------------------------------------------------*/
            /* macro application                                      */
            /*--------------------------------------------------------*/
            PROTECT(temp4)
            D = cons(S,cons(E,cons(C,D)));
            S = cons(cdr(temp4),cons(cons(expr,NIL),NIL));
            E = pMemGlobal->tlVals;
            C = cons(MKINT(APC),cons(MKINT(STOP),NIL));
            PROTECT(temp)
            /*--------------------------------------------------------*/
            /* don't allow macro expansion to be yielded by GC or     */
            /* output; instead abort on exceeding a maximum number    */
            /* of macro calls during a single compilation             */
            /*--------------------------------------------------------*/
            if (++macroCalls >= MAX_MACRO_PER_COMPILE)
              error1(ERR_C16_COMPLEX_MACRO,fn);
            evalMacro = true;
            temp = exec();
            evalMacro = false;
            if (stepsInSlice >= STEPS_PER_TIMESLICE)
              error1(ERR_C16_COMPLEX_MACRO,fn);
            S = car(D); E = cadr(D); C = caddr(D); D = cdddr(D);
            UNPROTECT(temp)
            UNPROTECT(temp4)
            expr = temp;
            goto tailCall;
          }
        }
        goto appl;
      }
      else
      {
        /*------------------------------------------------------------*/
        /* function application                                       */
        /*------------------------------------------------------------*/
      appl:
        if (*code == rtnCont)
          *code = cons(MKINT(TAPC),NIL);
        else
          *code = cons(MKINT(APC),*code);
        comp(fn,names,code);
        complist(args,names,code,NIL,CONS);
      }
    }
  }
}

/**********************************************************************/
/* Compile an SEXPR                                                   */
/**********************************************************************/
PTR compile(PTR expr, PTR names)
{
  PTR res = NIL;
  PTR src = NIL;
  Boolean eval = names != NIL;

  /*------------------------------------------------------------------*/
  /* Protect working registers                                        */
  /*------------------------------------------------------------------*/
  PROTECT(res)
  PROTECT(src)
  joinCont = cons(MKINT(JOIN),NIL);
  PROTECT(joinCont)
  rtnCont  = cons(MKINT(RTN),NIL);
  PROTECT(rtnCont)

  if (eval)
  {
    res = rtnCont;
    src = expr;
  }
  else
  {
    if (pMemGlobal->fileLoad)
    {
      if (!resolveDefineList(expr, &src))
        ErrThrow(ERR_L3_NOT_DEFINE);
    }
    else
    {
      if (resolveDefineList(expr, &src))
      {  
        /*------------------------------------------------------------*/
        /* and interactive definition is compiled, forget last memo   */
        /*------------------------------------------------------------*/
        pMemGlobal->fileLoad  = true;
        pMemGlobal->loadState = LS_INIT;
      }
      else
      {
        /*------------------------------------------------------------*/
        /* add (set! it) before expr. to be able to refer to last res.*/
        /*------------------------------------------------------------*/
        src = cons(SET,cons(IT,cons(expr,NIL)));
      }
    }
    res = cons(MKINT(STOP),NIL);
    names = pMemGlobal->tlNames;
  }
  qqNest     = 0;
  macroCalls = 0;
  comp(src, names, &res);
  if (pMemGlobal->fileLoad && !eval)
  {
    /*----------------------------------------------------------------*/
    /* We loaded a file, so cons the definitions from outermost       */
    /* letrec block in file to current top level environment          */
    /*----------------------------------------------------------------*/
    pMemGlobal->tlNames = cons(pMemGlobal->newTopLevelNames,
                               pMemGlobal->tlNames);
  }
  UNPROTECT(joinCont)
  UNPROTECT(rtnCont)
  UNPROTECT(src)
  UNPROTECT(res)
  return res;
}
