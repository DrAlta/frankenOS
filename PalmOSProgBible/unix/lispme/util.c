/**********************************************************************/
/*                                                                    */
/* util.c:  LISPME utility functions needed everywhere                */
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
/* 10.01.1999 New                                                FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "util.h"
#include "LispMe.h"
#include "gui.h"
#include "file.h"
#include "io.h"

/**********************************************************************/
/* Global data                                                        */
/**********************************************************************/
char* errInfo;
Int16 expectedLen;

/**********************************************************************/
/* check for some types                                               */
/**********************************************************************/
void checkInt(PTR p)
{
  if (!IS_INT(p))
    typeError(p,"integer");
}

void checkString(PTR p)
{
  if (!IS_STRING(p))
    typeError(p,"string");
}

/**********************************************************************/
/* ptr from ID                                                        */
/**********************************************************************/
void* ptrFromObjID(UInt16 obj)
{
  return FrmGetObjectPtr(FrmGetActiveForm(),
           FrmGetObjectIndex(FrmGetActiveForm(),obj));
}

static Boolean grabbed = false;

/**********************************************************************/
/* Convert a C string into LispMe string                              */
/**********************************************************************/
PTR str2Lisp(char* cp)
{
  return makeString(StrLen(cp),cp, 0, 0, NIL);
}

/**********************************************************************/
/* build type error object                                            */
/**********************************************************************/
void typeError(PTR p, char* type) 
{
  errInfo = type;
  ErrThrow((((UInt32)p) << 16) | ERR_R1_WRONG_TYPE);
}

/**********************************************************************/
/* build parm error object                                            */
/**********************************************************************/
void parmError(PTR p, char* func)
{
  errInfo = func;
  ErrThrow((((UInt32)p) << 16) | ERR_R15_INVALID_PARM);
}

/**********************************************************************/
/* length of a list                                                   */
/**********************************************************************/
short listLength(PTR l)
{
  short len = 0;
  PTR   p = l;
  while (IS_PAIR(p))
  {
    p = cdr(p);
    ++len;
  }
  if (p!=NIL)
    error1(ERR_C6_IMPROPER_ARGS, l);
  return len;
}

/**********************************************************************/
/* Get current time                                                   */
/**********************************************************************/
PTR getTime(void)
{
  DateTimeType dt;
  TimSecondsToDateTime(TimGetSeconds(),&dt);
  return cons(MKINT(dt.year),
           cons(MKINT(dt.month),
             cons(MKINT(dt.day),
                cons(MKINT(dt.hour),
                  cons(MKINT(dt.minute),
                    cons(MKINT(dt.second),
                      cons(MKINT(dt.weekDay),NIL)))))));
}

/**********************************************************************/
/* build error object include error resource and object               */
/**********************************************************************/
void error1(UInt16 err, PTR p)
{
  ErrThrow((((UInt32)p) << 16) | err);
}

/**********************************************************************/
/* Make a filename                                                    */
/**********************************************************************/
UInt16 makeFileName(PTR fileName)
{
  UInt16 nameLen;

  if (!IS_STRING(fileName))
    typeError(fileName,"string");
  printSEXP(fileName, PRT_MESSAGE);
  nameLen = StrLen(msg) + 1;
  if (nameLen > MAX_FILENAME_LEN)
  {
    msg[MAX_FILENAME_LEN]   = '\n';
    msg[MAX_FILENAME_LEN+1] = '\0';
    nameLen = MAX_FILENAME_LEN+2;
  }
  else
  {
    StrCat(msg,"\n");
    ++nameLen;
  }
  return nameLen;
}

/**********************************************************************/
/* Grab memory                                                        */
/**********************************************************************/
void GrabMem(void)
{
  if (!grabbed)
  {
    MemSemaphoreReserve(true);
    grabbed = true;
  }
}

/**********************************************************************/
/* Release memory                                                     */
/**********************************************************************/
void ReleaseMem(void)
{
  if (grabbed)
  {
    MemSemaphoreRelease(true);
    grabbed = false;
  }
}

/**********************************************************************/
/* Disable/enable command buttons during execution                    */
/**********************************************************************/
void enable(UInt16 id)
{
  CtlShowControl((ControlPtr)FrmGetObjectPtr(mainForm,
                 FrmGetObjectIndex(mainForm,id)));
}

void disable(UInt16 id)
{
  CtlHideControl((ControlPtr)FrmGetObjectPtr(mainForm,
                 FrmGetObjectIndex(mainForm,id)));
}

/**********************************************************************/
/* Disable/enable command buttons during execution                    */
/**********************************************************************/
void disableButtons(void)
{
  disable(IDC_PB_POP);
  disable(IDC_PB_LOAD);
  disable(IDC_PB_RELOAD);
  disable(IDC_PB_NAMES);
  disable(IDC_PB_EVAL);
  enable(IDC_PB_BREAK);
}

void enableButtons(void)
{
  disable(IDC_PB_BREAK);
  enable(IDC_PB_POP);
  enable(IDC_PB_LOAD);
  enable(IDC_PB_RELOAD);
  enable(IDC_PB_NAMES);
  enable(IDC_PB_EVAL);
}

/**********************************************************************/
/* display error message                                              */
/**********************************************************************/
void displayError(UInt32 err)
{
  UInt16 resId = err & 0Xffff;
  PTR  obj     = err >> 16;
  char msg1[16];
  char msg2[7];

  if (obj)
  {
    GrabMem();
    printSEXP(obj, PRT_MESSAGE | (resId == ERR_USER_ERROR ? 0 : PRT_ESCAPE));
  }
  else if (resId == ERR_S1_INVALID_CHAR)
  {
    msg[0] = errorChar;
    msg[1] = '\0';
  }
  else
    msg[0] = '\0';
  ReleaseMem();

  if (resId == ERR_R3_NUM_ARGS)
  {
    if (expectedLen >= 0)
      StrIToA(msg1,expectedLen);
    else
    {
      StrCopy(msg1,"at least ");
      StrIToA(msg2,-expectedLen);
      StrCat(msg1,msg2);
    }
    StrIToA(msg2, listLength(obj));
    FrmCustomAlert(resId, msg1, msg2, msg);
  }
  else if (resId == ERR_R1_WRONG_TYPE    ||
           resId == ERR_R15_INVALID_PARM ||
           resId == ERR_U5_INVALID_KIND)
    FrmCustomAlert(resId, msg, errInfo, "");
  else if (resId >= ERR_S1_INVALID_CHAR && resId < ERR_C1_UNDEF)
  {
    StrIToA(msg1, lineNr);
    FrmCustomAlert(resId, msg1, msg, "");
  }
  else
    FrmCustomAlert(resId, msg, "", "");
}

/**********************************************************************/
/* Disable/enable main form controls for GUI                          */
/**********************************************************************/
void enableCtls(Boolean enable)
{
  FldSetUsable(outField, enable);
  FldSetUsable(inField, enable);
  CtlSetUsable((ControlPtr)
    FrmGetObjectPtr(mainForm,
                    FrmGetObjectIndex(mainForm,IDC_PT_SYMS)),
    enable);
  CtlSetUsable((ControlPtr)
    FrmGetObjectPtr(mainForm,
                    FrmGetObjectIndex(mainForm,IDC_ST_SESSION)),
    enable);
  if (enable)
  {
    FrmEraseForm(mainForm);
    updateScrollBar();
    FrmUpdateForm(IDD_MainFrame, frmRedrawUpdateCode);
  }
  else
  {
    SclSetScrollBar(scrollBar, 0, 0, 0, 1);
    FldReleaseFocus(outField);
    FldReleaseFocus(inField);
  }
}

/**********************************************************************/
/* Update the scroll bar associated with a field                      */
/**********************************************************************/
void updateScrollBar(void)
{
  FieldPtr     field = ptrFromObjID(IDC_EF_OUTPUT);
  ScrollBarPtr sbar  = ptrFromObjID(IDC_SB_OUTPUT);
  UInt16       scrollPos;
  UInt16       textHeight;
  UInt16       fieldHeight;

  FldGetScrollValues(field, &scrollPos, &textHeight, &fieldHeight);
  if (textHeight > fieldHeight)
    SclSetScrollBar(sbar, scrollPos, 0,textHeight-fieldHeight, fieldHeight);
  else
    SclSetScrollBar(sbar, scrollPos,0, 0, fieldHeight);
}
