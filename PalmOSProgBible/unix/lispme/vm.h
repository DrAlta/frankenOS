/**********************************************************************/
/*                                                                    */
/* vm.h: LISPME Virtual Machine interface and opcode definitions      */
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

#ifndef INC_VM_H
#define INC_VM_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* VM opcodes                                                         */
/**********************************************************************/
#define LD     1 /* load variable                                     */
#define LDC    2 /* load constant                                     */
#define LDF    3 /* load function                                     */
#define AP     4 /* apply function                                    */
#define RTN    5 /* return from function                              */
#define DUM    6 /* create dummy environment                          */
#define RAP    7 /* recursively apply (letrec-block)                  */
#define SEL    8 /* select subcontrol list                            */
#define JOIN   9 /* join to main control list                         */
#define CAR   10 /* CAR of TOS object                                 */
#define CDR   11 /* CDR of TOS object                                 */
#define PAIR  12 /* PAIR? of TOS object                               */
#define CONS  13 /* CONS of 2 TOS objects                             */
#define EQ    14 /* EQ of 2 TOS objects                               */
#define ADD   15 /* add 2 TOS objects                                 */
#define SUB   16 /* subtract 2 TOS objects                            */
#define MUL   17 /* multiply 2 TOS objects                            */
#define DIV   18 /* divide 2 TOS objects                              */
#define REM   19 /* remainder of 2 TOS objects                        */
#define LEQ   20 /* <= of 2 TOS objects                               */
#define STOP  21 /* stop execution of VM                              */
#define NUL   22 /* NULL predicate of TOS object                      */
#define ST    23 /* store TOS to variable                             */
#define LDE   24 /* load expression (build recipe)                    */
#define AP0   25 /* apply parameterless function                      */
#define UPD   26 /* update recipe and return                          */
#define LDFC  27 /* load closure including number of args             */
#define APC   28 /* apply closure and check for number of args        */
#define LDCT  29 /* load continuation                                 */
#define UERR  30 /* report user error                                 */
#define INTP  31 /* is TOS an integer                                 */
#define SELR  32 /* select subcontrol without pushing rest            */
#define TAPC  33 /* tail-apply closure and check for number of args   */
#define POP   34 /* pop topmost stack object                          */
#define DSPL  35 /* display topmost stack object                      */
#define CERR  36 /* cond error, no true clause found                  */
#define SCAR  37 /* set CAR of TOS object                             */
#define SCDR  38 /* set CDR of TOS object                             */
#define EQV   39 /* EQV of 2 TOS objects                              */
#define SQRT  40 /* sqrt of TOS object                                */
#define SIN   41 /* sin  of TOS object                                */
#define COS   42 /* cos  of TOS object                                */
#define TAN   43 /* tan  of TOS object                                */
#define ASIN  44 /* asin of TOS object                                */
#define ACOS  45 /* acos of TOS object                                */
#define ATAN  46 /* atan of TOS object                                */
#define EXP   47 /* exp  of TOS object                                */
#define LOG   48 /* log  of TOS object                                */
#define MAGN  49 /* magnitude                                         */
#define ATN2  50 /* atan2 of 2 TOS objects                            */
#define SINH  51 /* sinh  of TOS object                               */
#define COSH  52 /* cosh  of TOS object                               */
#define TANH  53 /* tanh  of TOS object                               */
#define ASIH  54 /* asinh of TOS object                               */
#define ACOH  55 /* acosh of TOS object                               */
#define ATAH  56 /* atanh of TOS object                               */
#define REAP  57 /* is TOS a real                                     */
#define FLOR  58 /* floor of TOS object                               */
#define CEIL  59 /* ceil of TOS object                                */
#define TRUN  60 /* trunc of TOS object                               */
#define ROUN  61 /* round of TOS object                               */
#define INTG  62 /* convert TOS to integer                            */
#define IDIV  63 /* integer division of 2 TOS objects                 */
#define ANGL  64 /* angle                                             */
#define BOOL  65 /* BOOLEAN? of TOS object                            */
#define NOT   66 /* NOT of TOS object                                 */
#define RECT  67 /* Draw a rectangle (filled)                         */
#define DRAW  68 /* Draw a line                                       */
#define CHR   69 /* CHAR? of TOS object                               */
#define STRG  70 /* STRING? of TOS object                             */
#define WRIT  71 /* write topmost stack object                        */
#define REAC  72 /* read-char                                         */
#define SLEN  73 /* string-length                                     */
#define S2L   74 /* string->list                                      */
#define L2S   75 /* list->string                                      */
#define SREF  76 /* string-ref                                        */
#define SAPP  77 /* string-append                                     */
#define SSET  78 /* string-set!                                       */
#define SUBS  79 /* substring                                         */
#define SEQ   80 /* string=?                                          */
#define C2I   81 /* char->integer                                     */
#define I2C   82 /* integer->char                                     */
#define PROC  83 /* procedure?                                        */
#define CONT  84 /* continuation?                                     */
#define PROM  85 /* promise?                                          */
#define SYMB  86 /* symbol?                                           */
#define O2S   87 /* object->string                                    */
#define TEXT  88 /* display graphic text                              */
#define S2O   89 /* string->object                                    */
#define UINF  90 /* report user message                               */
#define EVT   91 /* event                                             */
#define RAND  92 /* random                                            */
#define DASM  93 /* disassemble procedure                             */
#define SND   94 /* play a sound (hertz, milliseconds)                */
#define WAIT  95 /* wait (milliseconds)                               */
#define MDIR  96 /* memopad directory                                 */
#define OOUT  97 /* open-output-file                                  */
#define OINP  98 /* open-input-file                                   */
#define PEEK  99 /* peek-char                                         */
#define DELF 100 /* delete-file                                       */
#define INPP 101 /* input-port?                                       */
#define OUPP 102 /* output-port?                                      */
#define EOFO 103 /* eof-object?                                       */
#define READ 104 /* read                                              */
#define REDL 105 /* read-line                                         */
#define OAPP 106 /* open-append-file                                  */
#define VECP 107 /* vector?                                           */
#define VSET 108 /* vector-set!                                       */
#define VREF 109 /* vector-ref                                        */
#define VMAK 110 /* make-vector                                       */
#define VLEN 111 /* vector-length                                     */
#define V2L  112 /* vector->list                                      */
#define L2V  113 /* list->vector                                      */
#define SEO  114 /* select subcontrol, don't pop if #t                */
#define SEOR 115 /* select subcontrol, don't pop if #t, don't push cdr*/
#define SEA  116 /* select subcontrol, don't pop if #f                */
#define SEAR 117 /* select subcontrol, don't pop if #f, don't push cdr*/
#define MEM  118 /* select subcontrol if TOS is in set                */
#define MEMR 119 /* select subcontrol if TOS is in set, don't push cdr*/
#define MKRA 120 /* make-rectangular                                  */
#define MKPO 121 /* make-polar                                        */
#define CPLP 122 /* is TOS a complex                                  */
#define REPA 123 /* real-part                                         */
#define IMPA 124 /* imag-part                                         */
#define APY  125 /* apply closure and check for number of args,copy   */
#define TAPY 126 /* tail apply clos. and check for number of args,copy*/
#define APND 127 /* append 2 TOS items                                */
#define EVA  128 /* eval                                              */
#define GSYM 129 /* gensym                                            */
#define TAP  130 /* tail-apply let bindings                           */
#define RTAP 131 /* tail-apply letrec bindings                        */
#define NONE 132 /* none?                                             */
#define LDM  133 /* load macro                                        */
#define MACP 134 /* macro?                                            */
#define BITM 135 /* bitmap                                            */
#define GUI  136 /* own-gui                                           */
#define DBRD 137 /* read-record                                       */
#define RSRD 138 /* read-resource                                     */
#define FRMD 139 /* frm-popup                                         */
#define FLDS 140 /* fld-set-text                                      */
#define FLDG 141 /* fld-get-text                                      */
#define CTLS 142 /* ctl-set-val                                       */
#define CTLG 143 /* ctl-get-val                                       */
#define FRMQ 144 /* frm-return                                        */
#define RDBS 145 /* set-resdb                                         */
#define LSTS 146 /* lst-set-sel                                       */
#define LSTG 147 /* lst-get-sel                                       */
#define LSTT 148 /* lst-get-text                                      */
#define LSTL 149 /* lst-set-list                                      */
#define FRMG 150 /* frm-goto                                          */
#define FRGF 151 /* frm-get-focus                                     */
#define FRSF 152 /* frm-set-focus                                     */
#define FRSH 153 /* frm-show                                          */
#define DBWR 154 /* write-record                                      */
#define GTIM 155 /* time                                              */
#define HBLD 156 /* hb-dir                                            */
#define HBIF 157 /* hb-info                                           */
#define HBGF 158 /* hb-setfield                                       */
#define HBSF 159 /* hb-getfield                                       */
#define HBGL 160 /* hb-getlinks                                       */
#define HBAR 161 /* hb-addrecord                                      */
#define RGBX 162 /* rgb->index                                        */
#define XRGB 163 /* index->rgb                                        */
#define DOGC 164 /* gc                                                */
#define SMAK 165 /* make-string                                       */
#define SPAL 166 /* set-palette                                       */

#define NOP 0xff /* marks end of code list in compiler tables         */

/**********************************************************************/
/* Defines                                                            */
/**********************************************************************/
#define STEPS_PER_TIMESLICE   1600
#define YIELD() if (!evalMacro) stepsInSlice = STEPS_PER_TIMESLICE

/**********************************************************************/
/* Exported data                                                      */
/**********************************************************************/
extern int     stepsInSlice;
extern Boolean evalMacro;

/**********************************************************************/
/* prototypes                                                         */
/**********************************************************************/
PTR   makeNum(long n)                   SEC(VM);
PTR   storeNum(double re, double im)    SEC(VM);
PTR   makePolar(double mag, double ang) SEC(VM);
PTR   exec(void)                        SEC(VM);

#endif
