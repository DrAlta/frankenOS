/**********************************************************************/
/*                                                                    */
/* store.c: LISPME memory management                                  */
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
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/


/**********************************************************************/
/* includes                                                           */
/**********************************************************************/
#include "store.h"
#include "lispme.h"
#include "vm.h"
#include "io.h"
#include "gui.h"
#include "util.h"
#include "comp.h"
#include "file.h"

/**********************************************************************/
/* global data                                                        */
/**********************************************************************/
PTR       keyWords[39]; // see store.h for aliases
PTR*      protPtr[MAX_PROTECT];
int       protIdx;
char*     stackLimit;
char      charEllipsis;
char      charNumSpace;
LocalID   dbId;
MemHandle atomHandle;
MemHandle heapHandle;
MemHandle globHandle;
Boolean   caseSens;
Boolean   palmIII;
LocalID   memoId;
DmOpenRef memoRef;

/**********************************************************************/
/* find atom in store or add it if not found                          */
/**********************************************************************/
PTR findAtom(char* str)
{
  char* store  = strStore;
  char* newstr = str;
 
  for(;;)
  {
    if (*store == *newstr)
    {
      if (*store == '\0')
      {
        /* string found */
        return MKATOM(store-strStore-(newstr-str));
      }
      /* compare next chars */
      ++store;
      ++newstr;
    }
    else
    {
      /* advance to end of string in store */
      while (*store++)
        ;
      if (*store == '\x01')
      {
        /* end of store, check for space and insert string */
        int len = StrLen(str);
        if (store-strStore+len+1 >= atomSize)
          ErrThrow(ERR_M1_STRING_FULL);
        MemMove(store,str,len+1);
        return MKATOM(store-strStore);
      }
      else
        /* continue comparison at start of search string */
        newstr = str;
    }
  }
}

/**********************************************************************/
/* get string for atom from memory                                    */
/**********************************************************************/
char* getAtom(PTR p)
{
  if (IS_ATOM(p))
    return strStore + ATOMVAL(p);
  else
    ErrThrow(ERR_M2_INVALID_PTR);
}

/**********************************************************************/
/* comparison function for quicksort                                  */
/**********************************************************************/
static Int16 compare(void* a, void* b, Int32 ignore)
{
  return StrCompare(*((const char**)a),*((const char**)b));
}

/**********************************************************************/
/* Build list of all available symbols                                */
/**********************************************************************/
char** allSymbols(UInt16* num)
{
  char**  res;
  char*   p;
  UInt16  idx = 0;

  *num = 0;
  for (p = strStore+1; *p != '\x01'; ++p)
    if (*p == '\0') ++*num;
  if ((res = (char**)MemPtrNew(*num*sizeof(char*))))
  {
    for (p = strStore+1; *p != '\x01'; ++p)
    {
      res[idx++] = p;
      while (*++p)
        ;
    }
    SysQSort(res,*num,sizeof(char*),&compare,0L);
  }
  return res;
}

/**********************************************************************/
/* init heap cells                                                    */
/**********************************************************************/
void initHeap(Boolean clear, Boolean redraw)
{
  PTR p;
  MemHandle recHand;

  if (clear)
  {
    /*----------------------------------------------------------------*/
    /* Destroy all vectors and strings                                */
    /*----------------------------------------------------------------*/
    while ((recHand = DmQueryRecord(dbRef, REC_VECTOR)))
      DmRemoveRecord(dbRef, REC_VECTOR);
  }

  GrabMem();
  if (clear)
  {
    pMemGlobal->magic = SESAME;

    /*----------------------------------------------------------------*/
    /* Init string store for atoms                                    */
    /*----------------------------------------------------------------*/
    MemSet(strStore,atomSize,'\x01');
    strStore[0] = '\0';
    firstFree = NIL;
    firstFreeReal = NIL;
  }
  QUOTE           = findAtom("quote");
  DEFINE          = findAtom("define");
  LET             = findAtom("let");
  LETREC          = findAtom("letrec");
  LAMBDA          = findAtom("lambda");
  BEGIN           = findAtom("begin");
  ELSE            = findAtom("else");
  COND            = findAtom("cond");
  IF              = findAtom("if");
  AND             = findAtom("and");
  OR              = findAtom("or");
  LIST            = findAtom("list");
  VECTOR          = findAtom("vector");
  SET             = findAtom("set!");
  DELAY           = findAtom("delay");
  CALLCC          = findAtom("call/cc");
  APPLY           = findAtom("apply");
  IT              = findAtom("it");
  CASE            = findAtom("case");
  QUASIQUOTE      = findAtom("quasiquote");
  UNQUOTE         = findAtom("unquote");
  UNQUOTESPLICING = findAtom("unquote-splicing");
  EVAL            = findAtom("eval");
  MACRO           = findAtom("macro");
  PENDOWN         = findAtom("pen-down");
  PENMOVE         = findAtom("pen-move");
  PENUP           = findAtom("pen-up");
  KEYDOWN         = findAtom("key-down");
  CTLENTER        = findAtom("ctl-enter");
  CTLSELECT       = findAtom("ctl-select");
  CTLREPEAT       = findAtom("ctl-repeat");
  LSTENTER        = findAtom("lst-enter");
  LSTSELECT       = findAtom("lst-select");
  POPSELECT       = findAtom("pop-select");
  FLDENTER        = findAtom("fld-enter");
  FLDCHANGED      = findAtom("fld-changed");
  MENU            = findAtom("menu");
  FRMOPEN         = findAtom("frm-open");
  FRMCLOSE        = findAtom("frm-close");

  if (clear)
  {
    PTR temp;

    /*----------------------------------------------------------------*/
    /* Init machine registers                                         */
    /*----------------------------------------------------------------*/
    S = E = C = D = W = NIL;
    running = false;
    outPos = numStep = numGC = tickStart = 0;
    pMemGlobal->waitEvent = pMemGlobal->getEvent = false;

    /*----------------------------------------------------------------*/
    /* Build linked list of free heap cells                           */
    /*----------------------------------------------------------------*/
    for (p=0x8000; p<heapSize-0x8000; p+=2*sizeof(PTR))
    {
      car(p) = firstFree;
      firstFree = p;
    }

    /*----------------------------------------------------------------*/
    /* Build linked list of free real cells                           */
    /*----------------------------------------------------------------*/
    for (p=0; p<realSize; p+=sizeof(double))
    {
      *((PTR*)(reals+p)) = firstFreeReal;
      firstFreeReal = p;
    }

    /*----------------------------------------------------------------*/
    /* Init global variables it and *gstate*                          */
    /* (no GC protection here, as memory is almost empty!)            */
    /*----------------------------------------------------------------*/
    pMemGlobal->tlNames = 
      cons(cons(IT, cons(findAtom("*gstate*"),NIL)), NIL);


    temp = cons(cons(MKINT(0),MKINT(0)), // point
           cons(MKINT(0),                // font    
           cons(TRUE,                    // pattern 
           cons(MKINT(0),                // mode    
           cons(MKINT(255),              // foreground color
           cons(MKINT(0),                // background color
           cons(MKINT(255), NIL)))))));  // text color
    pMemGlobal->tlVals = 
      cons(cons(FALSE, cons(temp, NIL)), NIL);
    pMemGlobal->graphState = cdar(pMemGlobal->tlVals);

    pMemGlobal->loadState = LS_INIT;
    pMemGlobal->symCnt    = 0;
    setResDB(FALSE);
  }
  pMemGlobal->newTopLevelVals  = NIL;
  InitCompiler();
  ReleaseMem();
  if (clear)
  {
    DmStrCopy(MemHandleLock(outHandle), 0,"\n\n\n\n\t\tWelcome to LispMe");
    MemHandleUnlock(outHandle);
    DmStrCopy(MemHandleLock(inHandle), 0, "");
    MemHandleUnlock(inHandle);
  }
  if (redraw)
  {
    FldSetTextHandle(outField, outHandle);
    FldDrawField(outField);
    FldSetTextHandle(inField, inHandle);
    FldDrawField(inField);
    updateScrollBar();
  }
}

/**********************************************************************/
/* MARK macros for garbage collection                                 */
/**********************************************************************/
#define MARK(n)   markBit[n>>5] |= (1<<((((short)n)>>2)&0x07))
#define MARKED(n) (markBit[n>>5] & (1<<((((short)n)>>2)&0x07)))
#define MARK_REAL(n)   markRealBit[((UInt16)n)>>7] |=\
                       (1<<((((UInt16)n)>>4)&0x07))
#define MARKED_REAL(n) (markRealBit[((UInt16)n)>>6] &\
                       (1<<((((UInt16)n)>>3)&0x07)))

/**********************************************************************/
/* mark phase of garbage collection                                   */
/**********************************************************************/
static void markVector(PTR p);
static void mark(PTR p)
{
  CHECKSTACK(p)
loop:
  if (IS_CONS(p))
  {
    if (!MARKED(p))
    {
      MARK(p);
      if (IS_VECFAST(p))
        markVector(p);
      else if (IS_STRFAST(p))
        return;
      else
      {
        mark(car(p));
        p = cdr(p);
        goto loop;
      }
    }
  }
  else if (IS_REAL(p))
    MARK_REAL(p);
}

/**********************************************************************/
/* mark a vector                                                      */
/**********************************************************************/
static void markVector(PTR p)
{
  UInt16    index;
  MemHandle recHand;
  PTR*      pel;
  Int16     num;

  DmFindRecordByID(dbRef, UID_VEC(p), &index);
  recHand = DmQueryRecord(dbRef,index);
  pel = (PTR*)(MemHandleLock(recHand));
  num = MemHandleSize(recHand)/2;
  ErrTry {
    while (num--)
      mark(*pel++);
  } ErrCatch(err) {
    MemHandleUnlock(recHand);
    ErrThrow(err);
  } ErrEndCatch
  MemHandleUnlock(recHand);
}

/**********************************************************************/
/* collect unused cells                                               */
/**********************************************************************/
static void collect(void)
{
  PTR    p;
  UInt16 index;

  for (p=0x8000; p<heapSize-0x8000; p+=2*sizeof(PTR))
    if (!MARKED(p))
    {
      if (IS_VECSTR(p))
      {
        DmFindRecordByID(dbRef, UID_VEC(p), &index);
        DmRemoveRecord(dbRef,index);
      }
      car(p) = firstFree;
      firstFree = p;
    }

  for (p=0; p<realSize; p+=sizeof(double))
    if (!MARKED_REAL(p))
    {
      *((PTR*)(reals+p)) = firstFreeReal;
      firstFreeReal = p;
    }
}

/**********************************************************************/
/* collect garbage                                                    */
/**********************************************************************/
void gc(PTR a, PTR b)
{
  int i;

  /*------------------------------------------------------------------*/
  /* mark reachable cells and collect                                 */
  /*------------------------------------------------------------------*/
  MemSet(markBit-0x0400, (UInt32)(heapSize >> 5), 0);
  MemSet(markRealBit, max(1ul,(UInt32)(realSize >> 6)), 0);
  mark(pMemGlobal->newTopLevelVals);
  mark(pMemGlobal->tlNames); mark(pMemGlobal->tlVals);
  mark(a); mark(b);
  mark(S); mark(E); mark(C); mark(D); mark(W);

  for (i=0;i<protIdx;++i)
    mark(*(protPtr[i]));

  for (i=0;i<=actContext;++i)
  {
    mark(contexts[i].prevCont);
    mark(contexts[i].handler);
  }

  firstFree = NIL;
  firstFreeReal = NIL;
  collect();
  ++numGC;
  YIELD();
}

/**********************************************************************/
/* memory statistics                                                  */
/**********************************************************************/
void memStat(Int32* heapUse, Int32* realUse, Int32* atomUse,
             Int32* vecSize, Int32* strSize)
{
  PTR    i;
  char*  p;
  Int16  numVec;
  UInt16 attr;

  W = NIL;
  GrabMem();
  if (!running)
  {
    S = E = D = NIL;
    protIdx = 0;
  }
  gc(NIL,NIL);

  *heapUse = *realUse = 0;
  for (i=0x8000; i < heapSize-0x8000; i+=2*sizeof(PTR))
    if (MARKED(i))
      *heapUse += 2*sizeof(PTR);

  for (i=0; i < realSize; i+=sizeof(double))
    if (MARKED_REAL(i))
      *realUse += sizeof(double);

  for (p=strStore; *p != '\x01'; ++p)
    ;
  *atomUse = p-strStore;
  ReleaseMem();
  numVec = DmNumRecords(dbRef) - REC_VECTOR;
  *vecSize = *strSize = 0;

  for (i=0;i<numVec;++i)
  {
    DmRecordInfo(dbRef,REC_VECTOR+i,&attr, 0, 0);
    *(attr & dmRecAttrCategoryMask ? strSize : vecSize)
      += MemHandleSize(DmQueryRecord(dbRef,REC_VECTOR+i));
  }
}

/**********************************************************************/
/* Format a number right-aligned                                      */
/**********************************************************************/
void formatRight(char* buf, Int32 n, Int16 pl)
{
  Int16 l;

  StrIToA(buf, n);
  l = StrLen(buf);
  MemMove(buf+pl-l, buf, l+1);
  MemSet(buf, pl-l, charNumSpace);
}

void formatMemStat(char* buf, Int32 use, Int32 total)
{
  char numBuf[8];
  formatRight(buf, use, 5);
  StrCat(buf,"/");
  formatRight(numBuf, total, 5);
  StrCat(buf, numBuf);
}

/**********************************************************************/
/* Create a vector (either filled with constant or init'ed by a list) */
/**********************************************************************/
PTR makeVector(Int16 len, PTR fill, Boolean advance)
{
  MemHandle recHand;
  char*     recPtr;
  UInt16    index = 65535;
  UInt32    uid;
  int       i;
  PTR       res;

  if (len == 0)
    return EMPTY_VEC;

  /*------------------------------------------------------------------*/
  /* Can't use straightforward cons, as car and cdr of vector cell    */
  /* are not regular pointers that can be protected!                  */
  /*------------------------------------------------------------------*/
  res = cons(NIL,NIL);
  if (!(recHand = DmNewRecord(dbRef, &index, len*2)))
    ErrThrow(ERR_M7_NO_VECTOR_MEM);
  recPtr = MemHandleLock(recHand);
  for (i=0; i<len; ++i)
   if (advance)
   {
     ((PTR*)recPtr)[i] = car(fill);
     fill = cdr(fill);
   }
   else
     ((PTR*)recPtr)[i] = fill;
  MemHandleUnlock(recHand);
  DmReleaseRecord(dbRef, index, true);
  DmRecordInfo(dbRef, index, 0, &uid, 0);
  car(res) = MKVEC_HI(uid);
  cdr(res) = MKVEC_LO(uid);
  return res;
}

/**********************************************************************/
/* length of a string                                                 */
/**********************************************************************/
short stringLength(PTR s)
{
  UInt16      index;
  MemHandle recHand;

  if (!IS_STRING(s))
    typeError(s,"string");
  if (s==EMPTY_STR)
    return 0;
  DmFindRecordByID(dbRef, UID_STR(s), &index);
  recHand = DmQueryRecord(dbRef,index);
  return MemHandleSize(recHand);
}

/**********************************************************************/
/* Append 2 strings                                                   */
/**********************************************************************/
PTR appendStrings(PTR a, PTR b)
{
  UInt16    index1, index2;
  MemHandle recHand1, recHand2;
  PTR       res;

  if (!IS_STRING(a))
    typeError(a,"string");
  if (!IS_STRING(b))
    typeError(b,"string");

  if (a==EMPTY_STR && b==EMPTY_STR)
    return EMPTY_STR;

  if (a == EMPTY_STR) {
    a = b; b = EMPTY_STR;
  }
  DmFindRecordByID(dbRef, UID_STR(a), &index1);
  recHand1 = DmQueryRecord(dbRef,index1);
  if (b != EMPTY_STR)
  {
    DmFindRecordByID(dbRef, UID_STR(b), &index2);
    recHand2 = DmQueryRecord(dbRef,index2);
    res = makeString(MemHandleSize(recHand1),MemHandleLock(recHand1),
                     MemHandleSize(recHand2),MemHandleLock(recHand2), NIL);
    MemHandleUnlock(recHand2);
  }
  else
    res = makeString(MemHandleSize(recHand1),MemHandleLock(recHand1),
                     0, 0, NIL);
  MemHandleUnlock(recHand1);
  return res;
}

/**********************************************************************/
/* Extract substring                                                  */
/**********************************************************************/
PTR substring(PTR str, Int16 start, Int16 end)
{
  int       len;
  UInt16    index;
  MemHandle recHand;
  PTR       res;

  if (!IS_STRING(str))
    typeError(str,"string");

  if (str==EMPTY_STR)
  {
    if (start != 0)
      goto error;
    else
      return EMPTY_STR;
  }
  DmFindRecordByID(dbRef, UID_STR(str), &index);
  recHand = DmQueryRecord(dbRef,index);
  len = MemHandleSize(recHand);
  if (start < 0 || start > len)
  error:
    error1(ERR_R2_INVALID_INDEX, MKINT(start));
  if (end > len)
    end = len;
  res = makeString(max(0,end-start),MemHandleLock(recHand)+start,
                   0, 0, NIL);
  MemHandleUnlock(recHand);
  return res;
}

/**********************************************************************/
/* copy a list (one level) into destination, return last pair         */
/**********************************************************************/
PTR* listCopy(PTR* d, PTR l, Int16 nc)
{
  while (IS_PAIR(l) && --nc >= 0)
  {
    *d = cons(car(l), NIL);
    d = &cdr(*d);
    l = cdr(l);
  }
  return d;
}

/**********************************************************************/
/* create a list from a vector                                        */
/**********************************************************************/
PTR vector2List(PTR v)
{
  Int16    i,num;
  UInt16    index;
  MemHandle recHand;
  PTR*      pel;
  PTR       res = NIL;
  PTR*      d = &res;

  if (v==EMPTY_VEC)
    return NIL;

  PROTECT(res);
  DmFindRecordByID(dbRef, UID_VEC(v), &index);
  recHand = DmQueryRecord(dbRef, index);
  num = MemHandleSize(recHand)/2;
  pel = (PTR*)(MemHandleLock(recHand));
  for (i=0;i<num;++i)
  {
    *d = cons(pel[i],NIL);
    d = &cdr(*d);
  }
  MemHandleUnlock(recHand);
  UNPROTECT(res);
  return res;
}

/**********************************************************************/
/* create a list from a string                                        */
/**********************************************************************/
PTR string2List(PTR str)
{
  Int16     i,num;
  UInt16    index;
  MemHandle recHand;
  char*     pc;
  PTR       res = NIL;
  PTR*      d = &res;

  if (str==EMPTY_STR)
    return NIL;

  PROTECT(res);
  DmFindRecordByID(dbRef, UID_STR(str), &index);
  recHand = DmQueryRecord(dbRef, index);
  num = MemHandleSize(recHand);
  pc = MemHandleLock(recHand);
  for (i=0;i<num;++i)
  {
    *d = cons(MKCHAR(*pc++),NIL);
    d = &cdr(*d);
  }
  MemHandleUnlock(recHand);
  UNPROTECT(res);
  return res;
}

/**********************************************************************/
/* create a new unique symbol                                         */
/**********************************************************************/
PTR gensym(void)
{
  msg[0] = '#';
  StrIToA(msg+1,pMemGlobal->symCnt++);
  return findAtom(msg);
}

/**********************************************************************/
/* Create a string                                                    */
/**********************************************************************/
PTR makeString(Int16 len, char* buf, Int16 len2, char* buf2, PTR l)
{
  MemHandle recHand;
  char*     recPtr;
  UInt16    index = 65535;
  UInt32    uid;
  UInt16    attr;
  PTR       p = l;
  PTR       res;

  if (len+len2 == 0 || (buf == 0 && p == NIL))
    return EMPTY_STR;

  /*------------------------------------------------------------------*/
  /* Can't use straightforward cons, as car and cdr of string cell    */
  /* are not regular pointers that can be protected!                  */
  /*------------------------------------------------------------------*/
  res = cons(NIL,NIL);
  if (len+len2 > 16383 ||
      !(recHand = DmNewRecord(dbRef, &index, len+len2)))
    ErrThrow(ERR_M8_NO_STRING_MEM);
  recPtr = MemHandleLock(recHand);
  if (buf==0)
  {
    if (buf2==0)
    { 
      /*--------------------------------------------------------------*/
      /* Fill by copying characters from list                         */
      /*--------------------------------------------------------------*/
      while (IS_PAIR(p))
      {
        if (!IS_CHAR(car(p)))
        {
          MemHandleUnlock(recHand);
          DmRemoveRecord(dbRef,index);
          typeError(car(p),"char");
        }
        *recPtr++ = CHARVAL(car(p));
        p = cdr(p);
      }
      if (p!=NIL)
      {
        MemHandleUnlock(recHand);
        DmRemoveRecord(dbRef,index);
        typeError(l,"list");
      }
    }
    else
    {
      /*--------------------------------------------------------------*/
      /* Fill with copies of single character                         */
      /*--------------------------------------------------------------*/
      MemSet(recPtr, len, CHARVAL(l));
    }   
  }
  else
  {
    MemMove(recPtr,buf,len);
    if (buf2)
      MemMove(recPtr+len,buf2,len2);
  }
  MemHandleUnlock(recHand);
  DmReleaseRecord(dbRef, index, true);
  DmRecordInfo(dbRef, index, &attr, &uid, 0);
  attr |= 0x01; // category 1
  DmSetRecordInfo(dbRef, index, &attr, 0);
  car(res) = MKSTR_HI(uid);
  cdr(res) = MKSTR_LO(uid);
  return res;
}

/**********************************************************************/
/* allocate a  heap cell protecting both arguments from being GCed    */
/* !!! in the case of consing two newly allocated cells (real or cons)*/
/* !!! both must be assigned to W to avoid GCing the inner cell when  */
/* !!! the other inner cell is allocated, _before_ it's protected by  */
/* !!! the outer cons, e.g.                                           */
/* !!! don't write S = cons(cons(a,b),cons(c,d));                     */
/* !!! but write S = cons(W=cons(a,b),W=cons(c,d));                   */
/**********************************************************************/
PTR cons(PTR a, PTR b)
{
  PTR res;

  if (firstFree == NIL)
  {
    gc(a,b);
    if (firstFree == NIL)
      ErrThrow(ERR_M4_NO_HEAP);
  }
  res = firstFree;
  firstFree = car(firstFree);
  car(res) = a; cdr(res) = b;
  return res;
}

/**********************************************************************/
/* allocate a real in memory                                          */
/**********************************************************************/
PTR allocReal(double d)
{
  PTR res;
  if (firstFreeReal == NIL)
  {
    gc(NIL,NIL);
    if (firstFreeReal == NIL)
      ErrThrow(ERR_M9_NO_REALS);
  }
  res = firstFreeReal;
  firstFreeReal = *((PTR*)(reals+firstFreeReal));
  *((double*)(reals+res)) = d;
  return MKREAL(res);
}

/**********************************************************************/
/* get a real from memory                                             */
/**********************************************************************/
double getReal(PTR p)
{
  if (IS_REAL(p))
    return *((double*)(reals+REALVAL(p)));
  else
    ErrThrow(ERR_M2_INVALID_PTR);
}
