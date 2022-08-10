/**********************************************************************/
/*                                                                    */
/* setup.c: Setup dialogs handling                                    */
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
/* 01.04.2000 New from LispMe.c etc.                             FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* includes                                                           */
/**********************************************************************/
#include "store.h"
#include "setup.h"
#include "sess.h"
#include "util.h"
#include "lispme.h"
#include "io.h"
#include "callback.h"

/**********************************************************************/
/* local defines                                                      */
/**********************************************************************/
#define MAX_PRINT_DEPTH      40

/**********************************************************************/
/* predeclare static functions                                        */
/**********************************************************************/

/**********************************************************************/
/* global data                                                        */
/**********************************************************************/

/**********************************************************************/
/* static data                                                        */
/**********************************************************************/

/**********************************************************************/
/* Set both listbox selection and popup trigger text                  */
/**********************************************************************/
static void setPopupList(UInt16 plID, UInt16 ptID, char selection)
{
  ListPtr lst = ptrFromObjID(plID);
  LstSetSelection(lst, selection);
  CtlSetLabel(ptrFromObjID(ptID), LstGetSelectionText(lst,selection));
}

/**********************************************************************/
/* Find memory size in list box and select it                         */
/**********************************************************************/
static void searchPopupList(UInt16 plID, UInt16 ptID, Int32 val)
{
  ListPtr lst = ptrFromObjID(plID);
  UInt16  num = LstGetNumberOfItems(lst);
  UInt16  i;
  static char buf[12];

  StrIToA(buf, val);
  for (i=0; i<num; ++i)
    if (!StrCompare(buf, LstGetSelectionText(lst,i)))
    {
      LstSetSelection(lst, i);
      CtlSetLabel(ptrFromObjID(ptID), LstGetSelectionText(lst,i));
      return;
    }
  ErrFatalDisplayIf(true, "Value not in list");
}

/**********************************************************************/
/* Retrieve memory block size from list box selection                 */
/**********************************************************************/
static Int32 listSelection(UInt16 obj)
{
  return StrAToI(((ListPtr)ptrFromObjID(obj))->
                  itemsText[LstGetSelection(ptrFromObjID(obj))]);
}

/**********************************************************************/
/* Change size of a memory block                                      */
/**********************************************************************/
static int ChangeBlockSize(UInt16  listId,
                           UInt16  recId,  MemHandle* recHandle,
                           Int32*  size,   char*      what)
{
  Int32 newSize = listSelection(listId);

  if (newSize != *size)
  {
    MemHandle newHand;
    newHand = DmNewRecord(dbRef, &recId, newSize);
    if (newHand)
    {
      DmRemoveRecord(dbRef, recId + 1);
      *recHandle = newHand;
      *size      = newSize;
      return 1;
    }
    else
      FrmCustomAlert(ERR_M6_SIZE_CHANGE, what, NULL, NULL);
  }
  return 0;
}

/**********************************************************************/
/* Handle 'Lefty' preference                                          */
/**********************************************************************/
void handleLefty(FormPtr form)
{
  FrmSetObjectPosition(form, FrmGetObjectIndex(form, IDC_EF_OUTPUT),
                       LispMePrefs.lefty ? 7 : 0, 17);
  FrmSetObjectPosition(form, FrmGetObjectIndex(form, IDC_SB_OUTPUT),
                       LispMePrefs.lefty ? 0 : 153, 17);
}

/**********************************************************************/
/* Event handler for setup form                                       */
/**********************************************************************/
Boolean SetupGlobHandleEvent(EventType *e)
{
  static char newPrintDepth;
  static Boolean oldLefty;
  static char pDepthStr[3];
  Boolean handled = false;
  CALLBACK_PROLOGUE

  switch (e->eType)
  {
    case frmOpenEvent:
    {
      /*--------------------------------------------------------------*/
      /* Set controls according to preferences                        */
      /*--------------------------------------------------------------*/
      setPopupList(IDC_PL_WDOG, IDC_PT_WDOG, LispMePrefs.watchDogSel);
      newPrintDepth = LispMePrefs.printDepth;
      StrIToA(pDepthStr, newPrintDepth);
      FldSetTextPtr(ptrFromObjID(IDC_EF_DEPTH),pDepthStr);
      CtlSetValue(ptrFromObjID(IDC_CB_PRINT_QUOTES), LispMePrefs.printQuotes);
      CtlSetValue(ptrFromObjID(IDC_CB_NO_AUTOOFF), LispMePrefs.noAutoOff);
      CtlSetValue(ptrFromObjID(IDC_CB_LEFTY), oldLefty=LispMePrefs.lefty);
      CtlSetValue(ptrFromObjID(IDC_CB_BIGMEMO), LispMePrefs.bigMemo);
      FrmDrawForm(FrmGetActiveForm());
      handled = true;
      break;
    }

    case ctlRepeatEvent:
    {
      FieldPtr fp = ptrFromObjID(IDC_EF_DEPTH);
      switch (e->data.ctlRepeat.controlID)
      {
        case IDC_DEPTH_DOWN:
          if (newPrintDepth > 1)
          {
            --newPrintDepth;
          displayDepth:
            StrIToA(pDepthStr, newPrintDepth);
            FldSetTextPtr(fp,pDepthStr);
            FldDrawField(fp);
          }
          break;

        case IDC_DEPTH_UP:
          if (newPrintDepth < MAX_PRINT_DEPTH)
          {
            ++newPrintDepth;
            goto displayDepth;
          }
          break;

        default:
      }
      break;
    }

    case ctlSelectEvent:
      switch (e->data.ctlSelect.controlID)
      {
        case IDC_PB_SETGLOB_OK:
        {
          LispMePrefs.printQuotes= CtlGetValue(ptrFromObjID(IDC_CB_PRINT_QUOTES));
          LispMePrefs.noAutoOff  = CtlGetValue(ptrFromObjID(IDC_CB_NO_AUTOOFF));
          LispMePrefs.lefty      = CtlGetValue(ptrFromObjID(IDC_CB_LEFTY));
          LispMePrefs.bigMemo    = CtlGetValue(ptrFromObjID(IDC_CB_BIGMEMO));
          if (LispMePrefs.lefty != oldLefty)
          {
            handleLefty(mainForm);
            FrmEraseForm(mainForm);
            FrmUpdateForm(IDD_MainFrame, frmRedrawUpdateCode);
          }
          LispMePrefs.printDepth = newPrintDepth;
          LispMePrefs.watchDogSel = LstGetSelection(ptrFromObjID(IDC_PL_WDOG));

          /*----------------------------------------------------------*/
          /* Save application preferences                             */
          /*----------------------------------------------------------*/
          PrefSetAppPreferencesV10(APPID, VERSION,
                                   &LispMePrefs, sizeof(LispMePrefs));
          FrmReturnToForm(IDD_MainFrame);
          handled = true;
          break;
        }

        case IDC_PB_SETGLOB_CANCEL:
          FrmReturnToForm(IDD_MainFrame);
          handled = true;
          break;

        default:
      }
      break;
    default:
  }
  CALLBACK_EPILOGUE
  return handled;
}

/**********************************************************************/
/* Event handler for setup form                                       */
/**********************************************************************/
Boolean SetupSessHandleEvent(EventType *e)
{
  Boolean handled = false;
  CALLBACK_PROLOGUE

  switch (e->eType)
  {
    case frmOpenEvent:
    {
      /*--------------------------------------------------------------*/
      /* Set controls according to preferences                        */
      /*--------------------------------------------------------------*/
      searchPopupList(IDC_PL_HEAP, IDC_PT_HEAP, heapSize);
      searchPopupList(IDC_PL_ATOM, IDC_PT_ATOM, atomSize);
      searchPopupList(IDC_PL_REAL, IDC_PT_REAL, realSize);
      searchPopupList(IDC_PL_OUTP, IDC_PT_OUTP, outpSize);
      CtlSetValue(ptrFromObjID(IDC_CB_CASESENS), caseSens);
      FrmDrawForm(FrmGetActiveForm());
      handled = true;
      break;
    }

    case ctlSelectEvent:
    {
      Boolean fromMain = startPanel == IDD_MainFrame;
      switch (e->data.ctlSelect.controlID)
      {
        case IDC_PB_SETSESS_OK:
        {
          Boolean needReinit;
          Boolean outpChanged;

          /*----------------------------------------------------------*/
          /* Try to resize memory blocks                              */
          /*----------------------------------------------------------*/
          if (fromMain)
            FldSetTextHandle(outField, (MemHandle)0);
          UnlockMem();

          if ((outpChanged =
                ChangeBlockSize(IDC_PL_OUTP, REC_OUT, &outHandle,
                                &outpSize, "output")))
          {
            DmStrCopy(MemHandleLock(outHandle),0, "");
            MemHandleUnlock(outHandle);
          }

          if (fromMain)
            FldSetTextHandle(outField, (MemHandle)outHandle);

          needReinit =
              ChangeBlockSize(IDC_PL_HEAP, REC_HEAP, &heapHandle,
                              &heapSize, "heap")
          +   ChangeBlockSize(IDC_PL_ATOM, REC_ATOM, &atomHandle,
                              &atomSize, "atom")
          +   ChangeBlockSize(IDC_PL_REAL, REC_REAL, &realHandle,
                              &realSize, "FP")
          > 0 || 
          (caseSens != CtlGetValue(ptrFromObjID(IDC_CB_CASESENS)));

          caseSens = CtlGetValue(ptrFromObjID(IDC_CB_CASESENS));

          LockMem();
          FrmReturnToForm(startPanel);

          if (needReinit)
            initHeap(true,fromMain);
          else if (outpChanged && fromMain)
          {
            FldDrawField(outField);
            updateScrollBar();
          }
          if (!fromMain)
          {
            DisConnectSession();
            FrmUpdateForm(IDD_Sess,lastSess);
          }
          handled = true;
          break;
        }

        case IDC_PB_SETSESS_CANCEL:
          if (!fromMain)
            DisConnectSession();
          FrmReturnToForm(startPanel);
          handled = true;
          break;
      }
      break;
    }
    default:
  }
  CALLBACK_EPILOGUE
  return handled;
}

