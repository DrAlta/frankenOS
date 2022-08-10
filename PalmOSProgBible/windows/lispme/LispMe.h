/**********************************************************************/
/*                                                                    */
/* lispme.h: LISPME general definitions                               */
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

#ifndef LISPME_H
#define LISPME_H 1

//********************************************************************/
// MENURESOURCES                                                     */
//********************************************************************/
#define IDM_OptGlob           9021
#define IDM_OptSess           9022
#define IDM_OptReset          9023
#define IDM_HelpAbout         9031
#define IDM_HelpForm          9032
#define IDM_HelpFunc          9033
#define IDM_HelpEvent         9034
#define IDM_ViewStat          9041
#define IDM_ViewMemory        9042
#define IDM_ViewGUI           9043
#define IDM_EditClrIn         9011
#define IDM_EditClrOut        9012

//********************************************************************/
// DIALOGRESOURCES                                                   */
//********************************************************************/
// MAINFRAME                                                         */
#define IDD_MainFrame         9990
#define IDC_EF_OUTPUT         9001
#define IDC_EF_INPUT          9002
#define IDC_PB_EVAL           9003
#define IDC_PB_LOAD           9004
#define IDC_PB_POP            9005
#define IDC_PB_NAMES          9006
#define IDC_PB_BREAK          9007
#define IDC_SB_OUTPUT         9008
#define IDC_ST_SESSION        9009
#define IDC_PT_SYMS           9010
#define IDC_PL_SYMS           9020
#define IDC_PB_RELOAD         9030

// SETUP Global                                                      */
#define IDD_SetupGlob         9991
#define IDC_PT_WDOG           9114
#define IDC_PL_WDOG           9124
#define IDC_CB_NO_AUTOOFF     9131
#define IDC_CB_LEFTY          9132
#define IDC_CB_PRINT_QUOTES   9133
#define IDC_CB_BIGMEMO        9134
#define IDC_DEPTH_DOWN        9140
#define IDC_DEPTH_UP          9141
#define IDC_EF_DEPTH          9142
#define IDC_PB_SETGLOB_OK     9192
#define IDC_PB_SETGLOB_CANCEL 9193

// LOAD                                                              */
#define IDD_Load              9992
#define IDC_PB_LOAD_OK        9202
#define IDC_PB_LOAD_CANCEL    9203
#define IDC_LIST_SOURCE       9204
#define IDC_PB_LOAD_MEMO      9205
#define IDC_PB_LOAD_EDIT      9206
#define IDC_PB_LOAD_PEDIT     9207

// READ                                                              */
#define IDD_Read              9993
#define IDC_PROMPT            9301
#define IDC_READ              9302
#define IDC_PB_ENTER          9303
#define IDC_PB_BREAK1         9304

// SESSIONS                                                          */
#define IDD_Sess              9994
#define IDC_TAB_SESSION       9402
#define IDC_PB_SESS_USE       9403
#define IDC_PB_SESS_NEW       9404
#define IDC_PB_SESS_DEL       9405
#define IDC_PB_SESS_SIZE      9406

// NEW SESSION                                                       */
#define IDD_NewSess           9995
#define IDC_EF_NSESS_NAME     9501
#define IDC_PB_NSESS_OK       9502
#define IDC_PB_NSESS_CANCEL   9503

// SETUP Session                                                     */
#define IDD_SetupSess         9996
#define IDC_PT_HEAP           9610
#define IDC_PT_ATOM           9611
#define IDC_PT_OUTP           9612
#define IDC_PT_REAL           9613
#define IDC_PL_HEAP           9620
#define IDC_PL_ATOM           9621
#define IDC_PL_OUTP           9622
#define IDC_PL_REAL           9623
#define IDC_CB_CASESENS       9624
#define IDC_PB_SETSESS_OK     9692
#define IDC_PB_SETSESS_CANCEL 9693

// EDITOR                                                            */
#define IDD_Edit              9997
#define IDC_PB_EDIT_DONE      9702
#define IDC_PB_EDIT_EVAL      9703

//********************************************************************/
// ALERTRESOURCES                                                    */
//********************************************************************/
#define IDA_ABOUT             9205
#define IDA_STAT_EXE          9206
#define IDA_STAT_MEM          9207

//********************************************************************/
// STRINGRESOURCES                                                   */
//********************************************************************/
#define HLP_LANG_FORM         9200
#define HLP_LANG_FUNC         9201
#define HLP_LANG_EVENT        9202
#define HLP_CALL_FRED         9203
#define HLP_SESSION           9204
#define HLP_SETGLOB           9205
#define HLP_SETSESS           9206

//********************************************************************/
// Error codes                                                       */
//********************************************************************/
#define ERR_S1_INVALID_CHAR    9000
#define ERR_S2_INVALID_SEXP    9001
#define ERR_S3_MULTI_DOT       9002
#define ERR_S4_TOKEN_LEN       9003
#define ERR_S6_INVALID_REAL    9005
#define ERR_S8_INVALID_HASH    9007
#define ERR_S9_UNTERM_STRING   9008
#define ERR_S10_INVALID_COMP   9009

#define ERR_C1_UNDEF           9021
#define ERR_C2_NUM_ARGS        9022
#define ERR_C3_NOT_SYMBOL      9023
#define ERR_C4_INVALID_LET     9024
#define ERR_C5_INVALID_LAMBDA  9025
#define ERR_C6_IMPROPER_ARGS   9026
#define ERR_C7_DUPLICATE_NAME  9027
#define ERR_C8_INV_LET_LIST    9028
#define ERR_C9_WRONG_DEFINE    9029
#define ERR_C10_EMPTY_SEQUENCE 9030
#define ERR_C11_INVALID_COND   9031
#define ERR_C13_INV_UNQUOTE    9033
#define ERR_C14_SPLICE_NONLIST 9034
#define ERR_C15_INVALID_DEFINE 9035
#define ERR_C16_COMPLEX_MACRO  9036

#define ERR_R1_WRONG_TYPE      9040
#define ERR_R2_INVALID_INDEX   9041
#define ERR_R3_NUM_ARGS        9042
#define ERR_R7_ILLEGAL_OP      9046
#define ERR_R8_DIV_BY_ZERO     9047
#define ERR_R9_BLACK_HOLE      9048
#define ERR_R10_COND_CLAUSE    9049
#define ERR_R12_NO_MATHLIB     9051
#define ERR_R14_INVALID_COMP   9053
#define ERR_R15_INVALID_PARM   9054
#define ERR_R16_CREATE_FILE    9055
#define ERR_R17_OPEN_FILE      9056
#define ERR_R18_LOST_FILE      9057
#define ERR_R19_WRITE_FILE     9058

#define ERR_M1_STRING_FULL     9060
#define ERR_M2_INVALID_PTR     9061
#define ERR_M3_CREATE_SESS     9062
#define ERR_M4_NO_HEAP         9063
#define ERR_M5_LIST            9064
#define ERR_M6_SIZE_CHANGE     9065
#define ERR_M7_NO_VECTOR_MEM   9066
#define ERR_M8_NO_STRING_MEM   9067
#define ERR_M9_NO_REALS        9068

#define ERR_O1_STACK_EMPTY     9080
#define ERR_O2_INTERRUPT       9081
#define ERR_O3_MAX_SESS        9082
#define ERR_O4_OS_VERSION      9083
#define ERR_O5_OUTPUT_TRUNC    9084
#define ERR_O6_CONFIRM         9085
#define ERR_O7_STACK_OVER      9086
#define ERR_O8_DEL_SESS        9087
#define ERR_O9_NO_SESS         9088
#define ERR_O10_ILLEGAL_SESS   9092
#define ERR_OX_SESSION_MISS    9089

#define ERR_USER_ERROR         9090
#define ERR_USER_INFO          9091

#define ERR_L1_NO_LISP         9100
#define ERR_L2_NO_MEMO         9101
#define ERR_L3_NOT_DEFINE      9102
#define ERR_L4_NO_LISPME       9103
#define ERR_L5_LAST_NOT_MEMO   9104
#define ERR_L6_CREATE_ICON     9105

#define ERR_U1_INVALID_FORM    9110
#define ERR_U2_INVALID_OBJ     9111
#define ERR_U3_WATCHDOG        9112
#define ERR_U4_INVALID_RESDB   9113
#define ERR_U5_INVALID_KIND    9114
#define ERR_U6_INVALID_GOTO    9115
#define ERR_U7_FORM_NEST       9116

#define ERR_H1_NO_HANDBASE     9120
#define ERR_H2_ERROR           9121


//********************************************************************/
// VERSION                                                           */
//********************************************************************/
#define VERSION                  27

//********************************************************************/
// SIZE OF INPUT FIELDS                                              */
//********************************************************************/
#define OUTPUT_SIZE           64000
#define INPUT_SIZE             4096
#define READ_SIZE                64

//********************************************************************/
// SIZE OF Session table                                             */
//********************************************************************/
#define SESS_ROWS                10

//********************************************************************/
// Launcher (generated)                                              */
//********************************************************************/
#define IDD_Launcher           9999

#endif
