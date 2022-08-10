/**********************************************************************/
/*                                                                    */
/* gui.c:   LISPME user interface functions                           */
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
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "gui.h"
#include "io.h"
#include "util.h"
#include "LispMe.h"
#include "vm.h"
#include "util.h"

/**********************************************************************/
/* Static functions                                                   */
/**********************************************************************/
static void* checkedPtrFromObjID(UInt16 obj,
                                 FormObjectKind kind,
                                 char* msg);
static UInt16 checkedIndexFromObjID(UInt16 obj);

/**********************************************************************/
/* Global data                                                        */
/**********************************************************************/
struct UIContext contexts[MAX_GUI_NEST];
int              actContext = -1;
char**           pSlots;
int              numSlots;

/**********************************************************************/
/* Replace current LispMe form by another one                         */
/**********************************************************************/
void gotoForm(int resId, PTR handler)
{
  MemHandle frmHand = DmGetResource('tFRM',resId);
  if (frmHand)
  {
    DmReleaseResource(frmHand);
    if (actContext < 0)
      ErrThrow(ERR_U6_INVALID_GOTO);
    running = false;
    contexts[actContext].handler = handler;
    ReleaseMem();
    FrmGotoForm(resId);
    GrabMem();
    changeHandler = true;
    return;
  }
  error1(ERR_U1_INVALID_FORM, MKINT(resId));
}
/**********************************************************************/
/* Popup a nested LispMe form                                         */
/**********************************************************************/
void popupForm(int resId, PTR handler)
{
  MemHandle frmHand = DmGetResource('tFRM',resId);
  if (frmHand)
  {
    DmReleaseResource(frmHand);
    running = !++actContext;
    if (actContext >= MAX_GUI_NEST-1)
      ErrThrow(ERR_U7_FORM_NEST);
    contexts[actContext].prevCont = cons(cddr(S),cons(E,cons(cdr(C),D)));
    contexts[actContext].handler  = handler;
    ReleaseMem();
    FrmPopupForm(resId);
    GrabMem();
    changeHandler = false;
    return;
  }
  error1(ERR_U1_INVALID_FORM, MKINT(resId));
}

/**********************************************************************/
/* index from ID (check for valid ID)                                 */
/**********************************************************************/
static UInt16 checkedIndexFromObjID(UInt16 obj)
{
  FormPtr frm = FrmGetActiveForm();
  int i;
  for (i=FrmGetNumberOfObjects(frm)-1;i>=0;--i)
    if (FrmGetObjectId(frm,i) == obj)
      return i;
  error1(ERR_U2_INVALID_OBJ, MKINT(obj));
}

/**********************************************************************/
/* Get field text                                                     */
/**********************************************************************/
PTR  GUIfldGetText(PTR id)
{
  char* p;
  checkInt(id);
  p = FldGetTextPtr(checkedPtrFromObjID(INTVAL(id),frmFieldObj,"field"));
  return p ? str2Lisp(p) : EMPTY_STR;
}

/**********************************************************************/
/* Get list text                                                      */
/**********************************************************************/
PTR  GUIlstGetText(PTR id, PTR obj)
{
  char* p;
  checkInt(id);
  checkInt(obj);
  p = LstGetSelectionText(checkedPtrFromObjID(INTVAL(id),frmListObj,"list"),
                           INTVAL(obj));
  return p ? str2Lisp(p) : EMPTY_STR;
}

/**********************************************************************/
/* Set list choices                                                   */
/* This currently allows only one list to avoid leaking               */
/**********************************************************************/
void GUIlstSetItems(PTR id, PTR items)
{
  ListPtr lst = checkedPtrFromObjID(INTVAL(id),frmListObj,"list");
  int i;
 
  GUIFreeList();

  if ((numSlots = listLength(items)))
  {
    pSlots   = MemPtrNew(numSlots*sizeof(char*));
    MemSet(pSlots,numSlots*sizeof(char*),0);
  }
  for (i=0;i<numSlots;++i)
  {
    printSEXP(car(items), PRT_MESSAGE);
    if (!(pSlots[i] = MemPtrNew(StrLen(msg)+1)))
    {
      GUIFreeList();
      ErrThrow(ERR_M5_LIST);
    }
    StrCopy(pSlots[i],msg);
    items = cdr(items);
  }
  LstSetListChoices(lst,pSlots,numSlots);
  LstDrawList(lst);
}

/**********************************************************************/
/* Free list choices                                                  */
/**********************************************************************/
void GUIFreeList(void)
{
  int i;

  if (pSlots)
  {
    for (i=numSlots-1;i>=0;--i)
      if (pSlots[i])
        MemPtrFree(pSlots[i]);
    MemPtrFree(pSlots);
  }
  pSlots = NULL;
  numSlots = 0;
}

/**********************************************************************/
/* ptr from ID (check for valid ID and kind)                          */
/**********************************************************************/
static void* checkedPtrFromObjID(UInt16 obj,
                                 FormObjectKind kind, char* msg)
{
  FormPtr frm = FrmGetActiveForm();
  UInt16  n   = checkedIndexFromObjID(obj);

  if (FrmGetObjectType(frm,n) != kind)
  {
    errInfo = msg;
    error1(ERR_U5_INVALID_KIND, MKINT(obj));
  }
  return FrmGetObjectPtr(frm,n);
}

/**********************************************************************/
/* Set field text                                                     */
/**********************************************************************/
void GUIfldSetText(PTR id, PTR obj)
{
  FieldPtr  fld;
  MemHandle oldH, newH;

  checkInt(id);
  printSEXP(obj, PRT_MESSAGE);
  fld  = checkedPtrFromObjID(INTVAL(id),frmFieldObj,"field");
  oldH = FldGetTextHandle(fld);
  newH = MemHandleNew(StrLen(msg)+1);
  StrCopy(MemHandleLock(newH),msg);
  MemHandleUnlock(newH);
  FldSetTextHandle(fld, newH);
  FldDrawField(fld);
  if (oldH)
    MemHandleFree(oldH);
}

/**********************************************************************/
/* Get control value                                                  */
/**********************************************************************/
PTR  GUIctlGetVal(PTR id)
{
  checkInt(id);
  return CtlGetValue(
           checkedPtrFromObjID(INTVAL(id),frmControlObj,"control"))
         ? TRUE : FALSE;
}

/**********************************************************************/
/* Set control value                                                  */
/**********************************************************************/
void GUIctlSetVal(PTR id, PTR obj)
{
  checkInt(id);
  CtlSetValue(checkedPtrFromObjID(INTVAL(id),frmControlObj,"control"),
              obj!=FALSE);
}

/**********************************************************************/
/* Get list selection                                                 */
/**********************************************************************/
PTR  GUIlstGetSel(PTR id)
{
  UInt16 n;

  checkInt(id);
  n = LstGetSelection(checkedPtrFromObjID(INTVAL(id),frmListObj,"list"));
  return n==-1 ? FALSE : MKINT(n);
}

/**********************************************************************/
/* Set list selection                                                 */
/**********************************************************************/
void GUIlstSetSel(PTR id, PTR obj)
{
  checkInt(id);
  if (obj==FALSE)
    obj=MKINT(-1);
  checkInt(obj);
  LstSetSelection(checkedPtrFromObjID(INTVAL(id),frmListObj,"list"),
                  INTVAL(obj));
}

/**********************************************************************/
/* Set focus                                                          */
/**********************************************************************/
void GUIfrmSetFocus(PTR id)
{
  checkInt(id);
  FrmSetFocus(FrmGetActiveForm(),checkedIndexFromObjID(INTVAL(id)));
}

/**********************************************************************/
/* Get focus                                                          */
/**********************************************************************/
PTR GUIfrmGetFocus(void)
{
  FormPtr frm = FrmGetActiveForm();
  UInt16  n   = FrmGetFocus(frm);
  return n==-1 ? FALSE : MKINT(FrmGetObjectId(frm,n));
}

/**********************************************************************/
/* Show/hide a form object                                            */
/**********************************************************************/
void GUIfrmShow(PTR id, PTR show)
{
  FormPtr frm = FrmGetActiveForm();

  checkInt(id);
  if (show==FALSE)
    FrmHideObject(frm,checkedIndexFromObjID(INTVAL(id)));
  else
    FrmShowObject(frm,checkedIndexFromObjID(INTVAL(id)));
}
