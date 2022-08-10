/**********************************************************************/
/*                                                                    */
/* store.h: Structures for storage management                         */
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
/* 14.07.1997 New                                                FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

#ifndef INC_STORE_H
#define INC_STORE_H

/**********************************************************************/
/* includes                                                           */
/**********************************************************************/
#include <PalmOS.h>
#include <CoreTraps.h>

/**********************************************************************/
/* Put a function into a specific section                             */
/**********************************************************************/
#define SEC(s) __attribute__((section(#s)))

/**********************************************************************/
/*                                                                    */
/* Memory organization of LispMe:                                     */
/*                                                                    */
/* 1. Symbol store                                                    */
/*                                                                    */
/*    All atoms are stored contiguously, separated by 0x00 bytes, so  */
/*    they can directly accessed as C strings. After last used atom   */
/*    all storage is initialized with 0x01                            */
/*                                                                    */
/* 2. Cons storage                                                    */
/*                                                                    */
/*    uniform 32 bit cells, two PTRs each                             */
/*                                                                    */
/* 3. PTR format                                                      */
/*                                                                    */
/*    16 bit binary, lower 3 bits determine type:                     */
/*                                                                    */
/*    sddd dddd dddd ddd1    15 bit signed integer (-16384 .. 16383)  */
/*    sddd dddd dddd dd00    16 bit signed offset into heap           */
/*                           (no scale neccessary)                    */
/*                                                                    */
/*    dddd dddd dddd 0010    12 bit index into atom table (char*)     */
/*    dddd dddd dddd 0110    12 bit index into double table (double*) */
/*                                                                    */
/*    cccc cccc 0000 1010    8 bit ASCII char                         */
/*    uuuu uuuu 0100 1110    vector in heap, upper 8 bit of UID,      */
/*                           cdr field of this cell contains low      */
/*                           16 bit _untagged_                        */
/*                                                                    */
/*    uuuu uuuu 0101 1110    string in heap, upper 8 bit of UID,      */
/*                           cdr field of this cell contains low      */
/*                           16 bit _untagged_                        */
/*                                                                    */
/*    xxxx xxxx 0011 1110    reserved for special values              */
/*    xxxx xxxx 0111 1110    special value tags in car of list        */
/*    0000 0000 1000 1110    "lambda" used while compiling macro      */
/*                                                                    */
/*                                                                    */
/* 4. Internal compund data                                           */
/*                                                                    */
/*    Complex:       (CPLX_TAG real . imag)                           */
/*    Closures:      (CLOS_TAG arity code . env)                      */
/*    Macros:        (MACR_TAG . expander)                            */
/*    Continuations: (CONT_TAG stack env code . dump)                 */
/*    Promises:      (RCPD_TAG code . env)                            */
/*                   (RCPF_TAG . value)                               */
/*    Vectors:       (uidhi(8) . uidlo(16)) with uidhi tagged as above*/
/*    Strings:       (uidhi(8) . uidlo(16)) with uidhi tagged as above*/
/*    Ports:         (PRTI_TAG pos uidhi . uidlo)                     */
/*                   (PRTO_TAG uidhi . uidlo)                         */
/*                   uidhi and uidlo are 12 bit integers, building    */
/*                   the 24 bit unique DB record ID                   */
/*    Macros:        (MACR_TAG CLOS_TAG 1 code . env)                 */
/*                                                                    */
/* 5. Vector storage                                                  */
/*                                                                    */
/*    A DB record of size 2 * (vector-length v) of pointers.          */
/*    Each vector is a single DB record identified by its UID, which  */
/*    is held in its header structure in the heap. To disinguish a    */
/*    vector from a string, the vector's record is put into           */
/*    category 0.                                                     */
/*                                                                    */
/* 6. String storage                                                  */
/*                                                                    */
/*    A DB record of size (string-length s) of chars (no trailing 0   */
/*    byte) Each string is a single DB record identified by its UID,  */
/*    which is held in its header structure in the heap. To disinguish*/
/*    a vector from a string, the string's record is put into         */
/*    category 1.                                                     */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* defines                                                            */
/**********************************************************************/
#define MAX_HEAP_SIZE     64000L
#define MAX_REAL_SIZE     32768L
#define MSG_OUTPUT_SIZE      64
#define MAX_PROTECT         120
#define OS2_STACK_AVAIL    1980
#define OS3_STACK_AVAIL    3800

/*--------------------------------------------------------------------*/
/* Magic numbers identifying heap versions                            */
/*--------------------------------------------------------------------*/
#define SESAME 0x2702 /* version 2.7 heap tag */

/*--------------------------------------------------------------------*/
/* Record numbers in session database                                 */
/*--------------------------------------------------------------------*/
#define REC_GLOB   0
#define REC_ATOM   1
#define REC_REAL   2
#define REC_HEAP   3
#define REC_INP    4
#define REC_OUT    5
#define REC_VECTOR 6

/*--------------------------------------------------------------------*/
/* Automaton states for last loaded memo                              */
/*--------------------------------------------------------------------*/
#define LS_INIT    0
#define LS_LOADED  1
#define LS_ERROR   2

/**********************************************************************/
/* typedefs                                                           */
/**********************************************************************/
typedef Int16 PTR;

struct MemGlobal {
  UInt16  magic;
  PTR     tlVals;
  PTR     tlNames;
  PTR     firstFree;
  PTR     firstFreeReal;
  PTR     graphState;
  PTR     S,E,C,D,W;
  PTR     newTopLevelVals;
  PTR     newTopLevelNames;
  Int16   outPos;
  UInt32  numStep;
  UInt32  numGC;
  UInt32  tickStart;
  Boolean running;
  Boolean waitEvent;
  Boolean getEvent;
  Boolean fileLoad;
  Boolean printBufOverflow;
  Boolean ownGUI;
  Boolean caseSens;
  Int16   symCnt;
  UInt16  lastMemo;
  UInt8   loadState;
  char    resDBName[dmDBNameLength];
  char    msg[MSG_OUTPUT_SIZE+1];
};

struct LispMeGlobPrefs {
  UInt8   printDepth;
  Boolean noAutoOff;
  Boolean printQuotes;
  Boolean lefty;
  Boolean bigMemo;
  UInt8   watchDogSel;
  char    sessDB[dmDBNameLength];
};

struct Session {
  Boolean icon;
  UInt16  size;
  UInt16  card;
  LocalID id;
  char    name[dmDBNameLength];
};

/**********************************************************************/
/* macros                                                             */
/**********************************************************************/
#define IS_INT(ptr)     ((ptr) & 1)
#define IS_REAL(ptr)    (((ptr) & 0x0f) == 6)
#define IS_ATOM(ptr)    (((ptr) & 0x0f) == 2)
#define IS_CONS(ptr)    (((ptr) & 0x03) == 0)
#define IS_SPECIAL(ptr) (((ptr) & 0xff) == 0x3e)
#define IS_CHAR(ptr)    (((ptr) & 0xff) == 0x0a)
#define IS_STAG(ptr)    (((ptr) & 0x3f) == 0x3e)
#define IS_NE_VEC(ptr)  (IS_CONS(ptr) && (((car(ptr)) & 0xff) == 0x4e))
#define IS_VEC(ptr)     ((ptr)==EMPTY_VEC || IS_NE_VEC(ptr))
#define IS_STRING(ptr)  ((ptr)==EMPTY_STR ||\
                        (IS_CONS(ptr) && (((car(ptr)) & 0xff) == 0x5e)))
#define IS_VECFAST(ptr) (((car(ptr)) & 0xff) == 0x4e)
#define IS_STRFAST(ptr) (((car(ptr)) & 0xff) == 0x5e)
#define IS_VECSTR(ptr)  (((car(ptr)) & 0xef) == 0x4e)
#define IS_COMPLEX(ptr) (IS_CONS(ptr) && car(ptr) == CPLX_TAG)
#define IS_OPORT(ptr)   (IS_CONS(ptr) && car(ptr) == PRTO_TAG)
#define IS_IPORT(ptr)   (IS_CONS(ptr) && car(ptr) == PRTI_TAG)
#define IS_MACRO(ptr)   (IS_CONS(ptr) && car(ptr) == MACR_TAG)
#define IS_PAIR(ptr)    (IS_CONS(ptr) && (((car(ptr)) & 0x4f) != 0x4e))

#define BLACK_HOLE  ((PTR)0x003e)
#define NIL         ((PTR)0x013e)
#define FALSE       ((PTR)0x023e)
#define TRUE        ((PTR)0x033e)
#define NOPRINT     ((PTR)0x043e)
#define END_OF_FILE ((PTR)0x053e)
#define EMPTY_VEC   ((PTR)0x063e)
#define EMPTY_STR   ((PTR)0x073e)

#define CLOS_TAG    ((PTR)0x007e)
#define RCPD_TAG    ((PTR)0x017e)
#define RCPF_TAG    ((PTR)0x027e)
#define CONT_TAG    ((PTR)0x037e)
#define PRTI_TAG    ((PTR)0x047e)
#define PRTO_TAG    ((PTR)0x057e)
#define CPLX_TAG    ((PTR)0x067e)
#define MACR_TAG    ((PTR)0x077e)

#define ATOMVAL(ptr)  (((UInt16)(ptr))>>4)
#define MKATOM(off)   (((off)<<4) | 0x02)
#define REALVAL(ptr)  ((((UInt16)(ptr))>>4)<<3)
#define MKREAL(off)   (((off)<<1) | 0x06)
#define INTVAL(ptr)   ((ptr) >> 1)
#define MKINT(i)      ((PTR)((i)<<1) | 1)
#define CHARVAL(ptr)  (((UInt16)(ptr))>>8)
#define MKCHAR(c)     ((PTR)(((UInt8)(c))<<8) | 0x0a)
#define MKVEC_HI(uid) ((PTR)((((uid)>>8) & 0xff00) | 0x4e))
#define MKVEC_LO(uid) ((PTR)((uid) & 0xffff))
#define MKSTR_HI(uid) ((PTR)((((uid)>>8) & 0xff00) | 0x5e))
#define MKSTR_LO(uid) MKVEC_LO(uid)

#define MLAMBDA     ((PTR)0x008e)

#define car(ptr)    (*((PTR*)(heap+ptr)))
#define cdr(ptr)    (*((PTR*)(heap+ptr+sizeof(PTR))))

#define caar(ptr)   car(car(ptr))
#define cadr(ptr)   car(cdr(ptr))
#define cdar(ptr)   cdr(car(ptr))
#define cddr(ptr)   cdr(cdr(ptr))
#define caaar(ptr)  car(car(car(ptr)))
#define caadr(ptr)  car(car(cdr(ptr)))
#define cadar(ptr)  car(cdr(car(ptr)))
#define caddr(ptr)  car(cdr(cdr(ptr)))
#define cdaar(ptr)  cdr(car(car(ptr)))
#define cdadr(ptr)  cdr(car(cdr(ptr)))
#define cddar(ptr)  cdr(cdr(car(ptr)))
#define cdddr(ptr)  cdr(cdr(cdr(ptr)))
#define cadddr(ptr) car(cdr(cdr(cdr(ptr))))
#define cddddr(ptr) cdr(cdr(cdr(cdr(ptr))))

#define UID_PORT(ptr) (((UInt32)INTVAL(car(ptr)))<<12 | INTVAL(cdr(ptr)))
#define UID_VEC(ptr)  (((UInt32)CHARVAL(car(ptr)))<<16 | (UInt16)cdr(ptr))
#define UID_STR(ptr)  UID_VEC(ptr)

/**********************************************************************/
/* Aliases for keyword PTRs                                           */
/**********************************************************************/
#define QUOTE           keyWords[0]
#define QUASIQUOTE      keyWords[1]
#define UNQUOTE         keyWords[2]
#define UNQUOTESPLICING keyWords[3]
#define DEFINE          keyWords[4]
#define LET             keyWords[5]
#define LETREC          keyWords[6]
#define LAMBDA          keyWords[7]
#define BEGIN           keyWords[8]
#define ELSE            keyWords[9]
#define COND            keyWords[10]
#define IF              keyWords[11]
#define AND             keyWords[12]
#define OR              keyWords[13]
#define LIST            keyWords[14]
#define VECTOR          keyWords[15]
#define SET             keyWords[16]
#define DELAY           keyWords[17]
#define CALLCC          keyWords[18]
#define APPLY           keyWords[19]
#define IT              keyWords[20]
#define CASE            keyWords[21]
#define EVAL            keyWords[22]
#define MACRO           keyWords[23]

/**********************************************************************/
/* Aliases for event symbol PTRs                                      */
/**********************************************************************/
#define PENDOWN         keyWords[24]
#define PENMOVE         keyWords[25]
#define PENUP           keyWords[26]
#define KEYDOWN         keyWords[27]
#define CTLENTER        keyWords[28]
#define CTLSELECT       keyWords[29]
#define CTLREPEAT       keyWords[30]
#define LSTENTER        keyWords[31]
#define LSTSELECT       keyWords[32]
#define POPSELECT       keyWords[33]
#define FLDENTER        keyWords[34]
#define FLDCHANGED      keyWords[35]
#define MENU            keyWords[36]
#define FRMOPEN         keyWords[37]
#define FRMCLOSE        keyWords[38]

/**********************************************************************/
/* Protect for GC                                                     */
/**********************************************************************/
#define PROTECT(p)   {protPtr[protIdx++] = &p;}
#define UNPROTECT(p) {--protIdx;}

/**********************************************************************/
/* Check stack overflow                                               */
/**********************************************************************/
#define CHECKSTACK(p) {if (((char*)&(p))<stackLimit) \
                         ErrThrow(ERR_O7_STACK_OVER);}

/**********************************************************************/
/* external data                                                      */
/**********************************************************************/
extern LocalID   dbId;
extern DmOpenRef dbRef;
extern UInt16    memoDBMode;
extern LocalID   memoId;
extern DmOpenRef memoRef;

extern struct MemGlobal* pMemGlobal;
extern PTR S,E,C,D,W;
extern PTR firstFree;

extern char*    heap;
extern char*    strStore;

extern Int32    atomSize;
extern Int32    heapSize;
extern char*    markBit;

extern MemHandle realHandle;
extern Int32     realSize;
extern char*     reals;
extern char*     markRealBit;
extern PTR       firstFreeReal;

extern Int32   numGC;
extern Int32   numStep;
extern UInt32  tickStart;
extern UInt32  tickStop;
extern Boolean caseSens;

extern Int16    depth;              /* stack depth for printing or GC */
extern Boolean  running;
extern Int16    outPos;

extern MemHandle atomHandle;
extern MemHandle heapHandle;
extern MemHandle globHandle;
extern MemHandle outHandle;
extern MemHandle inHandle;
extern FieldPtr  inField;
extern FieldPtr  outField;
extern ScrollBarPtr scrollBar;
extern FormPtr      mainForm;

extern struct LispMeGlobPrefs LispMePrefs;
extern PTR keyWords[];

extern PTR*  protPtr[MAX_PROTECT];
extern int   protIdx;
extern char* stackLimit;

extern char  charEllipsis;
extern char  charNumSpace;
extern Boolean palmIII;

extern int     actContext;

extern Boolean   quitHandler;
extern Boolean   changeHandler;

/**********************************************************************/
/* prototypes                                                         */
/**********************************************************************/
void    InitStore(void);

PTR     findAtom(char* str) SEC(VM);
char*   getAtom(PTR p)      SEC(VM);

PTR     allocReal(double d) SEC(VM);
double  getReal(PTR p)      SEC(VM);

PTR     cons(PTR a, PTR b)  SEC(VM);
void    gc(PTR a, PTR b)    SEC(VM);

void    memStat(Int32* heapUse, Int32* realUse, Int32* atomUse,
                Int32* vecSize, Int32* strSize);
void    formatRight(char* buf, Int32 n, Int16 pl);
void    formatMemStat(char* buf, Int32 use, Int32 total);

void    initHeap(Boolean clear, Boolean redraw);
PTR     makeVector(Int16 len, PTR fill, Boolean advance) SEC(VM);
PTR     makeString(Int16 len, char* buf, Int16 len2, char* buf2,
                   PTR l)                          SEC(VM);
PTR     appendStrings(PTR a, PTR b)                SEC(VM);
PTR*    listCopy(PTR* d, PTR l, Int16 nc)          SEC(VM);
PTR     vector2List(PTR v)                         SEC(VM);
PTR     string2List(PTR str)                       SEC(VM);
short   stringLength(PTR s)                        SEC(VM);
PTR     substring(PTR str, Int16 start, Int16 end) SEC(VM);
PTR     gensym(void)                               SEC(VM);
char**  allSymbols(UInt16* num);

#endif
