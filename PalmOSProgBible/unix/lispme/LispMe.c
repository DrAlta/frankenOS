/**********************************************************************/
/*                                                                    */
/* LispMe.c: Entire program                                           */
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
/* 27.07.1997 New                                                FBI  */
/* 02.08.1997 Combined all sorce modules, as linker fucks up!!   FBI  */
/* 09.10.1997 PalmOS2 only version                               FBI  */
/* 01.03.1998 Use DB mem as heap via MemSemaphoreReserve()       FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 21.11.1999 'Reload' button added                              FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* includes                                                           */
/**********************************************************************/
#include "store.h"
#include <limits.h>

#include "LispMe.h"
#include "io.h"
#include "vm.h"
#include "comp.h"
#include "fpstuff.h"
#include "util.h"
#include "gui.h"
#include "file.h"
#include "sess.h"
#include "setup.h"
#include "hbase.h"
#include "graphic.h"
#include "callback.h"

/**********************************************************************/
/* local defines                                                      */
/**********************************************************************/
#define MAX_LINE_LEN 32
#define OUTPUT_LINES  8
#define PEDIT_APPID  'pn10'


/*--------------------------------------------------------------------*/
/* Size of the interbal editor                                        */
/*--------------------------------------------------------------------*/
#define BIG_EDIT 32768 
#define STD_EDIT  4096

/**********************************************************************/
/* predeclare static functions                                        */
/**********************************************************************/
static Boolean appHandleEvent(EventType*);
static Boolean MainFrameHandleEvent(EventType*);
static Boolean LoadHandleEvent(EventType*);
static Boolean EditFrameHandleEvent(EventType*);
static Boolean LispHandleEvent(EventType*);
static Boolean StartApp(char*);
static void    StopApp(void);

/**********************************************************************/
/* predeclare global functions                                        */
/**********************************************************************/
void    handleScrollEvents(EventType* e);
PTR     MakeLispEvent(EventType *e);
void    DoLMEvent(EventType *e);

/**********************************************************************/
/* global data                                                        */
/**********************************************************************/
DmOpenRef dbRef;
struct MemGlobal* pMemGlobal;
PTR       S,E,C,D,W;
PTR       firstFree;
char*     heap;
char*     strStore;

Int32     heapSize;
Int32     atomSize;
char*     markBit;
MemHandle realHandle;
Int32     realSize;
char*     reals;
char*     markRealBit;
PTR       firstFreeReal;

Int32  numGC;
Int32  numStep;
UInt32 tickStart;
UInt32 tickStop;

short    depth;              /* stack depth for printing or GC */
Boolean  running;
Boolean  quitHandler;
Boolean  changeHandler;
Int16    outPos;

MemHandle    outHandle;
MemHandle    inHandle;
FieldPtr     inField;
FieldPtr     outField;
FormPtr      mainForm;
ScrollBarPtr scrollBar;

struct LispMeGlobPrefs LispMePrefs;

UInt16  memoDBMode = dmModeReadOnly;

/**********************************************************************/
/* static data                                                        */
/**********************************************************************/
static Boolean   returning;
static Boolean   launchedExternally = true;
static UInt16    selMemo;

/**********************************************************************/
/* Switch to MemoPad to edit record (on error position)               */
/**********************************************************************/
static void startMemoPad(UInt16 recNum, UInt32 creator)
{
  GoToParamsPtr     goPtr;
  DmSearchStateType state;
  UInt16            card;
  LocalID           memoPad;

  goPtr = MemPtrNew(sizeof(GoToParamsType));
  MemPtrSetOwner(goPtr, 0);
  DmGetNextDatabaseByTypeCreator(true, &state, sysFileTApplication,
                                 creator, true, &card, &memoPad);
  goPtr->searchStrLen  = startPtr ? 1 : 0;
  goPtr->dbCardNo      = card;
  goPtr->dbID          = memoId;
  goPtr->recordNum     = recNum;
  goPtr->matchPos      = startPtr ? currPtr-startPtr-1 : 0;
  goPtr->matchFieldNum = 0;
  goPtr->matchCustom   = 0;

  SysUIAppSwitch(card, memoPad, sysAppLaunchCmdGoTo, (MemPtr)goPtr);
}

/**********************************************************************/
/* Init MathLib library                                               */
/**********************************************************************/
static void InitMathLib(void)
{
  Err error;

  error = SysLibFind(MathLibName, &MathLibRef);
  if (error)
    error = SysLibLoad(LibType, MathLibCreator, &MathLibRef);
  if (!error)
    mathLibOK = !MathLibOpen(MathLibRef, MathLibVersion);
}

/**********************************************************************/
/* Close MathLib library                                              */
/**********************************************************************/
static void CloseMathLib(void)
{
  Err    error;
  UInt16 useCount;

  if (mathLibOK)
  {
    error = MathLibClose(MathLibRef, &useCount);
    ErrFatalDisplayIf(error, "Can't close MathLib");
    if (useCount == 0)
      SysLibRemove(MathLibRef);
  }
}

/**********************************************************************/
/* Main entry                                                         */
/**********************************************************************/
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
  EventType e;
  short err;

  switch (cmd)
  {
    case sysAppLaunchCmdNormalLaunch:
      cmdPBP = NULL;
      launchedExternally = false;

    case sysAppLaunchCmdCustomBase:
      if (!StartApp((char*)cmdPBP))
        return 0;

      /*--------------------------------------------------------------*/
      /* Determine stack limit                                        */
      /*--------------------------------------------------------------*/
      stackLimit = ((char*)&e) - (palmIII ? OS3_STACK_AVAIL :
                                            OS2_STACK_AVAIL);
      FrmGotoForm(startPanel);
      do {
        EvtGetEvent(&e, running ? 0 : -1);
        if (!SysHandleEvent(&e))
          if (!MenuHandleEvent(0, &e, &err))
            if (!appHandleEvent(&e))
              FrmDispatchEvent(&e);
      } while (e.eType != appStopEvent);
      StopApp();
  }
  return 0;
}

/**********************************************************************/
/* Application event handling                                         */
/**********************************************************************/
static Boolean appHandleEvent(EventType* e)
{
  if (e->eType == frmLoadEvent)
  {
    Int16   formId = e->data.frmLoad.formID;
    FormPtr form   = FrmInitForm(formId);
    FrmSetActiveForm(form);

    switch(formId)
    {
      case IDD_MainFrame:
        FrmSetEventHandler(form, MainFrameHandleEvent);
        break;

      case IDD_Edit:
        FrmSetEventHandler(form, EditFrameHandleEvent);
        break;

      case IDD_SetupGlob:
        FrmSetEventHandler(form, SetupGlobHandleEvent);
        break;

      case IDD_SetupSess:
        FrmSetEventHandler(form, SetupSessHandleEvent);
        break;

      case IDD_Load:
        FrmSetEventHandler(form, LoadHandleEvent);
        break;

      case IDD_Sess:
        FrmSetEventHandler(form, SessHandleEvent);
        break;

      case IDD_NewSess:
        FrmSetEventHandler(form, NewSessHandleEvent);
        break;

      default:
        /*------------------------------------------------------------*/
        /* This is an ext.form, use special handler for Lisp callback */
        /* The LispMe handler function has already been installed     */
        /*------------------------------------------------------------*/
        returning = false;
        FrmSetEventHandler(form, LispHandleEvent);
        break;
    }
    return true;
  }
  return false;
}

/**********************************************************************/
/* Handle all events dealing with scrollbar/entryfield sync           */
/**********************************************************************/
void handleScrollEvents(EventType* e)
{
  FieldPtr fld;
  Int16    lines;
  switch (e->eType)
  {
    case keyDownEvent:
    {
      fld   = ptrFromObjID(IDC_EF_OUTPUT);
      lines = FldGetVisibleLines(fld)-1;
      switch (e->data.keyDown.chr)
      {
        case pageUpChr:
          if (FldScrollable(fld, winUp))
          {
            FldScrollField(fld, lines, winUp);
            updateScrollBar();
          }
          break;

        case pageDownChr:
          if (FldScrollable(fld, winDown))
          {
            FldScrollField(fld, lines, winDown);
            updateScrollBar();
          }
          break;
      }
      break;
    }
    case sclRepeatEvent:
    case sclExitEvent:
    {
      fld   = ptrFromObjID(IDC_EF_OUTPUT);
      lines = e->data.sclRepeat.newValue-e->data.sclRepeat.value;
      if (lines > 0)
        FldScrollField(fld, lines, winDown);
      else
        FldScrollField(fld, -lines, winUp);
      break;
    }

    case fldChangedEvent:
      updateScrollBar();
      break;
    default:
  }
}

/**********************************************************************/
/* Set all up for an evaluation                                       */
/**********************************************************************/
static void setUpEval(char* input, Boolean addBEGIN)
{
  PTR res;

  tickStart = TimGetTicks();
  GrabMem();
  S = E = C = D = W = NIL;
  pMemGlobal->fileLoad = addBEGIN;
  protIdx = 0;
  if (addBEGIN)
    res = loadMemo(input);
  else
    res = readSEXP(input);
  C = compile(res,NIL);
  S = D = W = NIL;
  E = pMemGlobal->tlVals;
  numGC = numStep = 0L;
  outPos = 0;
  running = true;
  evalMacro = false;
  actContext = -1;
}

/**********************************************************************/
/* Clean up after evaluation                                          */
/**********************************************************************/
static void cleanUpEval(Boolean buttons)
{
  tickStop  = TimGetTicks();
  running   = false;
  GrabMem();
  pMemGlobal->waitEvent = false;
  pMemGlobal->getEvent  = false;

  /*------------------------------------------------------------------*/
  /* Cleanup in case name and value lists are out of sync             */
  /*------------------------------------------------------------------*/
  if (listLength(pMemGlobal->tlNames) > listLength(pMemGlobal->tlVals))
      pMemGlobal->tlNames = cdr(pMemGlobal->tlNames);
  ReleaseMem();
  if (buttons)
    enableButtons();
  GUIFreeList();
  actContext = -1;
}

/**********************************************************************/
/* Convert PalmOS events to LispMe events                             */
/**********************************************************************/
PTR MakeLispEvent(EventType *e)
{
  PTR res;
  PROTECT(res)

  switch (e->eType)
  {
    case nilEvent:
      res = cons(FALSE,NIL);
      break;
 
    case penDownEvent:
      res = PENDOWN; goto PenEvent;
    case penUpEvent:
      res = PENUP; goto PenEvent;
    case penMoveEvent:
      res = PENMOVE;
    PenEvent:
      res = cons(res,
              cons(MKINT(e->screenX),
                cons(MKINT(e->screenY),NIL)));
      break;
  
    case keyDownEvent:
      res = cons(KEYDOWN,
              cons(MKCHAR(e->data.keyDown.chr),NIL));
      break;

    case ctlEnterEvent:
      res = cons(CTLENTER,
              cons(MKINT(e->data.ctlEnter.controlID),NIL));
      break;

    case ctlSelectEvent:
      res = cons(CTLSELECT,
              cons(MKINT(e->data.ctlSelect.controlID),
                cons(e->data.ctlSelect.on ? TRUE : FALSE,NIL)));
      break;

    case ctlRepeatEvent:
      res = cons(CTLREPEAT,
              cons(MKINT(e->data.ctlRepeat.controlID),NIL));
      break;

    case lstEnterEvent:
      res = cons(LSTENTER,
              cons(MKINT(e->data.lstEnter.listID),
                cons(MKINT(e->data.lstEnter.selection),NIL)));
      break;

    case lstSelectEvent:
      res = cons(LSTSELECT,
              cons(MKINT(e->data.lstSelect.listID),
                cons(MKINT(e->data.lstSelect.selection),NIL)));
      break;

    case popSelectEvent:
      res = cons(POPSELECT,
              cons(MKINT(e->data.popSelect.controlID),
                cons(MKINT(e->data.popSelect.listID),
                  cons(MKINT(e->data.popSelect.selection),
                    cons(MKINT(e->data.popSelect.priorSelection),NIL)))));
      break;

    case fldEnterEvent:
      res = cons(FLDENTER,
              cons(MKINT(e->data.fldEnter.fieldID),NIL));
      break;

    case fldChangedEvent:
      res = cons(FLDCHANGED,
              cons(MKINT(e->data.fldChanged.fieldID),NIL));
      break;

    case menuEvent:
      res = cons(MENU,
              cons(MKINT(e->data.menu.itemID),NIL));
      break;

    case frmOpenEvent:
      res = cons(FRMOPEN,
              cons(MKINT(e->data.frmOpen.formID),NIL));
      break;

    case frmCloseEvent:
      res = cons(FRMCLOSE,
              cons(MKINT(e->data.frmClose.formID),NIL));
      break;

    default:
      res = cons(FALSE,NIL);
  }

  UNPROTECT(res)
  return res;
}

/**********************************************************************/
/* Low level LispMe hook to event handler                             */
/**********************************************************************/
void DoLMEvent(EventType *e)
{
  if (running && pMemGlobal->getEvent)
    if (e->eType != nilEvent || !pMemGlobal->waitEvent)
    {
      GrabMem();
      S = cons(MakeLispEvent(e), S);
      pMemGlobal->getEvent = false;
      pMemGlobal->waitEvent = false;
      ReleaseMem();
    }
}

static void PopEnvFrame(void)
{
  if (IS_CONS(pMemGlobal->tlNames) &&
      IS_CONS(cdr(pMemGlobal->tlNames)))
  {
    GrabMem();
    pMemGlobal->tlNames   = cdr(pMemGlobal->tlNames);
    pMemGlobal->tlVals    = cdr(pMemGlobal->tlVals);
    pMemGlobal->loadState = LS_INIT;
    ReleaseMem();
  }
  else
    displayError(ERR_O1_STACK_EMPTY);
}

/**********************************************************************/
/* Build list of known symbols                                        */
/**********************************************************************/
static char** symbols;
static void fillSymbolList(void)
{
  static char* emergency[] = {"*** Not enough memory ***"};
  UInt16 num;
  ListPtr list = (ListPtr)ptrFromObjID(IDC_PL_SYMS);
  if (symbols)
    MemPtrFree(symbols);
  if ((symbols = allSymbols(&num)))
    LstSetListChoices(list, symbols, num);
  else
    LstSetListChoices(list, emergency,1);
  list->attr.search = true;
}

/**********************************************************************/
/* Event handler for main form                                        */
/**********************************************************************/
static Boolean MainFrameHandleEvent(EventType *e)
{
  PTR           res;
  static Char   buf[100];
  static Char   buf1[12];
  static Char   buf2[12];
  static Char   buf3[12];
  Boolean handled = false;
  CALLBACK_PROLOGUE

  DoLMEvent(e);
  handleScrollEvents(e);

  switch (e->eType)
  {
    case nilEvent:
      if (running && !pMemGlobal->waitEvent)
      {
        /*------------------------------------------------------------*/
        /* Run machine some steps                                     */
        /*------------------------------------------------------------*/
        ErrTry {
          GrabMem();
          res = exec();
          if (!running)
          {
            cleanUpEval(true);
            GrabMem();
            printSEXP(res, PRT_OUTFIELD | PRT_ESCAPE | PRT_AUTOLF);
          }
        }
        ErrCatch(err) {
          cleanUpEval(true);
          displayError(err);
        } ErrEndCatch
        ReleaseMem();
      }
      handled = true;
      break;

    case menuEvent:
      switch (e->data.menu.itemID)
      {
        case IDM_ViewStat:
        {
          /*----------------------------------------------------------*/
          /* Execution statistics                                     */
          /*----------------------------------------------------------*/
          UInt16 len;
          if (tickStop <= tickStart)
            StrCopy(buf1,"0.00");
          else
          {
            StrIToA(buf,tickStop-tickStart);
            switch (len = StrLen(buf))
            {
              case 1:
                StrCopy(buf1,"0.0"); StrCat(buf1,buf);
                break;

              case 2:
                StrCopy(buf1,"0."); StrCat(buf1,buf);
                break;

              default:
                StrCopy(buf1,buf); MemMove(buf1+len-1,buf1+len-2,3);
                buf1[len-2] = '.';
                break;
            }
          }
          StrIToA(buf2,numStep);
          StrIToA(buf3,numGC);
          FrmCustomAlert(IDA_STAT_EXE,buf1,buf2,buf3);
          handled = true;
        }
        break;

        case IDM_ViewMemory:
        {
          /*----------------------------------------------------------*/
          /* Memory statistics                                        */
          /*----------------------------------------------------------*/
          Int32 heapUse, atomUse, realUse;
          Int32 vecSize, strSize;
          memStat(&heapUse, &realUse, &atomUse, &vecSize, &strSize);
          formatMemStat(buf, heapUse, heapSize);
          StrCat(buf,"\nAtoms:\t");
          formatMemStat(buf1, atomUse, atomSize);
          StrCat(buf,buf1);
          StrCat(buf,"\nReals:\t");
          formatMemStat(buf1, realUse, realSize);
          StrCat(buf,buf1);
          formatRight(buf2,strSize,11);
          formatRight(buf3,vecSize,11);
          FrmCustomAlert(IDA_STAT_MEM,buf,buf2,buf3);
          handled = true;
        }
        break;

        case IDM_ViewGUI:
          /*----------------------------------------------------------*/
          /* View standard LispMe GUI elements                        */
          /*----------------------------------------------------------*/
          GrabMem();
          pMemGlobal->ownGUI = false;
          ReleaseMem();
          enableCtls(true);
          handled = true;
          break;

        case IDM_EditClrIn:
          /*----------------------------------------------------------*/
          /* Clear input field                                        */
          /*----------------------------------------------------------*/
          FldDelete(inField, 0, FldGetTextLength(inField));
          handled = true;
          break;

        case IDM_EditClrOut:
          /*----------------------------------------------------------*/
          /* Clear output field                                       */
          /*----------------------------------------------------------*/
          FldDelete(outField, 0, FldGetTextLength(outField));
          handled = true;
          break;

        case IDM_OptGlob:
          /*----------------------------------------------------------*/
          /* Display global setup dialog                              */
          /*----------------------------------------------------------*/
          FrmPopupForm(IDD_SetupGlob);
          handled = true;
          break;

        case IDM_OptSess:
          /*----------------------------------------------------------*/
          /* Display session setup dialog                             */
          /*----------------------------------------------------------*/
          if (running)
          {
            cleanUpEval(true);
            displayError(ERR_O2_INTERRUPT);
          }
          FrmPopupForm(IDD_SetupSess);
          handled = true;
          break;

        case IDM_OptReset:
          /*----------------------------------------------------------*/
          /* Reset memory                                             */
          /*----------------------------------------------------------*/
          if (FrmAlert(ERR_O6_CONFIRM) == 0)
          {
            if (running)
            {
              cleanUpEval(true);
              displayError(ERR_O2_INTERRUPT);
            }
            initHeap(true,true);
          }
          handled = true;
          break;

        case IDC_PB_LOAD:
        case IDC_PB_RELOAD:
        case IDC_PB_POP:
        case IDC_PB_NAMES:
        case IDC_PB_EVAL:
        {
          /*----------------------------------------------------------*/
          /* Map these commands to corresponding buttons              */
          /*----------------------------------------------------------*/
          EventType ev; 
          ev.eType = ctlSelectEvent; 
          ev.data.ctlSelect.controlID = e->data.menu.itemID;
          EvtAddEventToQueue(&ev);
          handled = true;
          break;
        }  

        case IDM_HelpAbout:
          /*----------------------------------------------------------*/
          /* Show GNU GPL                                             */
          /*----------------------------------------------------------*/
          FrmAlert(IDA_ABOUT);
          handled = true;
          break;

        case IDM_HelpForm:
          /*----------------------------------------------------------*/
          /* Help for special forms                                   */
          /*----------------------------------------------------------*/
          FrmHelp(HLP_LANG_FORM);
          handled = true;
          break;

        case IDM_HelpFunc:
          /*----------------------------------------------------------*/
          /* Help for functions                                       */
          /*----------------------------------------------------------*/
          FrmHelp(HLP_LANG_FUNC);
          handled = true;
          break;

        case IDM_HelpEvent:
          /*----------------------------------------------------------*/
          /* Help for events                                          */
          /*----------------------------------------------------------*/
          FrmHelp(HLP_LANG_EVENT);
          handled = true;
          break;
      }
      break;

    case frmOpenEvent:
    {
      MemHandle oldHandle;
      startPanel = IDD_MainFrame;
      mainForm   = FrmGetActiveForm();
      inField    = ptrFromObjID(IDC_EF_INPUT);
      outField   = ptrFromObjID(IDC_EF_OUTPUT);
      scrollBar  = ptrFromObjID(IDC_SB_OUTPUT);

      /*--------------------------------------------------------------*/
      /* Init handle for input field                                  */
      /*--------------------------------------------------------------*/
      oldHandle = FldGetTextHandle(inField);
      FldSetTextHandle(inField, inHandle);
      if (oldHandle)
        MemHandleFree(oldHandle);

      /*--------------------------------------------------------------*/
      /* Init handle for output field                                 */
      /*--------------------------------------------------------------*/
      oldHandle = FldGetTextHandle(outField);
      FldSetTextHandle(outField, outHandle);
      if (oldHandle)
        MemHandleFree(oldHandle);

      /*--------------------------------------------------------------*/
      /* Init other controls                                          */
      /*--------------------------------------------------------------*/
      CtlSetLabel(ptrFromObjID(IDC_ST_SESSION), LispMePrefs.sessDB);
      handleLefty(mainForm);
      if (running)
        disableButtons();
      enableCtls(!pMemGlobal->ownGUI);

      if (!running && launchedExternally)
      {
        /*------------------------------------------------------------*/
        /* Started externally => simulate pressing EVAL               */
        /*------------------------------------------------------------*/
        launchedExternally = false;
        CtlHitControl(ptrFromObjID(IDC_PB_EVAL));
      }
      handled = true;
      break;
    }

    case frmCloseEvent:
      FldCompactText(inField); 
      FldSetTextHandle(inField, NULL);
      FldSetTextHandle(outField, NULL);
      break;
 
    case ctlSelectEvent:
      switch (e->data.ctlSelect.controlID)
      {
        case IDC_PB_EVAL:
          /*----------------------------------------------------------*/
          /* Evaluate entered expression:                             */
          /*   Parse and compile and set state to running             */
          /*----------------------------------------------------------*/
          ErrTry {
            disableButtons();
            setUpEval(FldGetTextPtr(inField), false);
          }
          ErrCatch(err) {
            cleanUpEval(true);
            displayError(err);
          } ErrEndCatch
          ReleaseMem();
          handled = true;
          break;

        case IDC_PB_RELOAD:
          /*----------------------------------------------------------*/
          /* Pop current and reload last loaded memo                  */
          /*----------------------------------------------------------*/
          if (pMemGlobal->loadState != LS_INIT)
          {
            UInt16    recIdx = pMemGlobal->lastMemo;
            MemHandle recHand; 
            if (pMemGlobal->loadState == LS_LOADED)
              PopEnvFrame();
            memoRef= DmOpenDatabase(0,memoId,memoDBMode);
            startPtr = 0;
            recHand = DmQueryRecord(memoRef, recIdx);
            GrabMem(); 
            pMemGlobal->loadState = LS_LOADED;
            ReleaseMem();
            disableButtons();
            ErrTry {
              setUpEval(MemHandleLock(recHand),true);
            }
            ErrCatch(err) {
              pMemGlobal->loadState = LS_ERROR;
              cleanUpEval(true);
              displayError(err);
              MemHandleUnlock(recHand);
              DmCloseDatabase(memoRef);
              handled = true;
              break;
            } ErrEndCatch
            ReleaseMem();
            MemHandleUnlock(recHand);
            DmCloseDatabase(memoRef);
          }
          else
            displayError(ERR_L5_LAST_NOT_MEMO);
          handled = true;
          break;

        case IDC_PB_BREAK:
          /*----------------------------------------------------------*/
          /* Break excution                                           */
          /*----------------------------------------------------------*/
          cleanUpEval(true);
          displayError(ERR_O2_INTERRUPT);
          handled = true;
          break;

        case IDC_PB_LOAD:
          /*----------------------------------------------------------*/
          /* Load a source memo                                       */
          /*----------------------------------------------------------*/
          FrmPopupForm(IDD_Load);
          handled = true;
          break;

        case IDC_PB_POP:
          /*----------------------------------------------------------*/
          /* Pop last loaded source from stack                        */
          /*----------------------------------------------------------*/
          PopEnvFrame();
          handled = true;
          break;

        case IDC_PB_NAMES:
          /*----------------------------------------------------------*/
          /* Display all available names                              */
          /*----------------------------------------------------------*/
          outPos = 0;
          GrabMem();
          printSEXP(pMemGlobal->tlNames, PRT_OUTFIELD);
          ReleaseMem();
          handled = true;
          break;

        case IDC_PT_SYMS:
          /*----------------------------------------------------------*/
          /* Build list of known symbols                              */
          /*----------------------------------------------------------*/
          fillSymbolList();
          break;

        case IDC_ST_SESSION:
          /*----------------------------------------------------------*/
          /* Display session form                                     */
          /*----------------------------------------------------------*/
          FrmGotoForm(IDD_Sess);
          handled = true;
          break;

        default:
      }
      break;

    case popSelectEvent:
      /*--------------------------------------------------------------*/
      /* Insert selected symbol into input field                      */
      /*--------------------------------------------------------------*/
      if (symbols)
      {
        UInt16 sel = e->data.popSelect.selection;
        char*  sym = LstGetSelectionText((ListPtr)e->data.popSelect.listP,sel);
        FldInsert(inField, sym, StrLen(sym));
      }
      handled = true;
      break;

    default:
  }
  CALLBACK_EPILOGUE
  return handled;
}

/**********************************************************************/
/* Event handler for load form                                        */
/**********************************************************************/
static Boolean LoadHandleEvent(EventType *e)
{
  static char**   ppSrc  = (char**) NULL;
  static UInt16*  recIdx = (UInt16*) NULL;
  static UInt16   numSrc;
  Boolean handled = false;
  ListPtr srcList = ptrFromObjID(IDC_LIST_SOURCE);
  CALLBACK_PROLOGUE

  switch (e->eType)
  {
    case lstSelectEvent:
      startPtr = 0;
      break;

    case frmOpenEvent:
    {
      /*--------------------------------------------------------------*/
      /* Fill list from Memo DB                                       */
      /*--------------------------------------------------------------*/
      UInt16    lispCat;
      UInt16    recId   = 0;
      MemHandle recHand;
      Int16     i;

      memoRef= DmOpenDatabase(0,memoId,memoDBMode);
      startPtr = 0;

#ifdef OBSOLETE_10_HACK
      /*--------------------------------------------------------------*/
      /* Search memo category 'LISP' (either case)                    */
      /* Have to code it this way, as CategoryFind aborts, when       */
      /* it finds nothing :-(                                         */
      /*--------------------------------------------------------------*/
      /* This hacks seems only necessary with OS 1.0, later versions  */
      /* work. Additionally, starting with OS 3.3 on IIIx or V, this  */
      /* code no longer works, so now's a good chance to get rid of   */
      /* it entirely. What we loose is case-insensitivity of category */
      /* names; as we loose this now, let's do the long awaited change*/
      /* and relabel LispMe's category to "LispMe"                    */
      /*--------------------------------------------------------------*/
      CategoryCreateListV10(memoRef, &catLst, 0, false);
      for (i=0; i<catLst.numItems; ++i)
        if (!StrCaselessCompare(catLst.itemsText[i],"lisp"))
        {
          lispCat = CategoryFind(memoRef, catLst.itemsText[i]);
          break;
        }
      CategoryFreeListV10(memoRef, &catLst);
#endif

      lispCat = CategoryFind(memoRef, "LispMe");
      if (lispCat == dmAllCategories)
      {
        displayError(ERR_L1_NO_LISP);
        DmCloseDatabase(memoRef);
        FrmReturnToForm(IDD_MainFrame);
        handled = true;
        break;
      }

      /*--------------------------------------------------------------*/
      /* Go thru all records of category LISP                         */
      /*--------------------------------------------------------------*/
      if ((numSrc = DmNumRecordsInCategory(memoRef, lispCat)) == 0)
      {
        displayError(ERR_L2_NO_MEMO);
        DmCloseDatabase(memoRef);
        FrmReturnToForm(IDD_MainFrame);
        handled = true;
        break;
      }
 
      ppSrc   = MemPtrNew(numSrc * sizeof(char*));
      recIdx  = MemPtrNew(numSrc * sizeof(UInt16));
      for (i=0; i<numSrc; ++i)
      {
        char  *recPtr, *s, *d;
        recHand   = DmQueryNextInCategory(memoRef, &recId, lispCat);
        recPtr    = MemHandleLock(recHand);
        ppSrc[i]  = MemPtrNew(MAX_LINE_LEN+2);
        recIdx[i] = recId;

        /*------------------------------------------------------------*/
        /* Copy beginning of memo text upto first \n or MAX_LINE_LEN  */
        /*------------------------------------------------------------*/
        for (s = recPtr, d = ppSrc[i];
             s-recPtr < MAX_LINE_LEN && *s && *s != '\n';
             ++s, ++d)
          *d = *s;
        if (s-recPtr == MAX_LINE_LEN)
          *d++ = charEllipsis;
        *d = '\0';
        MemHandleUnlock(recHand);
        recId++;
      }
      LstSetListChoices(srcList, ppSrc, numSrc);

      /*--------------------------------------------------------------*/
      /* Disable 'PEdit' button if not installed                      */
      /*--------------------------------------------------------------*/
      { 
        DmSearchStateType state;
        UInt16 card;
        LocalID pedit;
        if (DmGetNextDatabaseByTypeCreator(true, &state, sysFileTApplication,
                                         PEDIT_APPID, true, &card, &pedit))
          CtlHideControl((ControlPtr)ptrFromObjID(IDC_PB_LOAD_PEDIT));
      }

      FrmDrawForm(FrmGetActiveForm());
      handled = true;
      break;
    }

    case keyDownEvent:
    {
      int lines = LstGetVisibleItems(srcList)-1; 
      switch (e->data.keyDown.chr)
      {
        case pageUpChr:
          LstScrollList(srcList, winUp, lines);
          handled = true;
          break;

        case pageDownChr:
          LstScrollList(srcList, winDown, lines);
          handled = true;
          break;
      }
      break;
    }  

    case ctlSelectEvent:
    {
      UInt16 sel = LstGetSelection(srcList);
      switch (e->data.ctlSelect.controlID)
      {
        case IDC_PB_LOAD_OK:
        {
          /*----------------------------------------------------------*/
          /* Load selected memo                                       */
          /*----------------------------------------------------------*/
          if (sel != -1)
          {
            MemHandle recHand = DmQueryRecord(memoRef, recIdx[sel]);
            GrabMem();
            pMemGlobal->lastMemo  = recIdx[sel];
            pMemGlobal->loadState = LS_LOADED;
            ReleaseMem();
            ErrTry {
              setUpEval(MemHandleLock(recHand),true);
            }
            ErrCatch(err) {
              pMemGlobal->loadState = LS_ERROR;
              cleanUpEval(false);
              displayError(err);
              MemHandleUnlock(recHand);
              handled = true;
              break;
            } ErrEndCatch
            ReleaseMem();
            MemHandleUnlock(recHand);
          } // anything selected
        } // case PB_OK
        disableButtons();

        /*------------------------------------------------------------*/
        /* Load was OK, now clean up dialog (=fall thru to cancel)    */
        /*------------------------------------------------------------*/
        case IDC_PB_LOAD_CANCEL:
        {
          Int16 i;
          for (i=0; i<numSrc; ++i)
            MemPtrFree(ppSrc[i]);
          MemPtrFree(ppSrc);
          MemPtrFree(recIdx);
          DmCloseDatabase(memoRef);
          FrmReturnToForm(IDD_MainFrame);
          FrmEraseForm(FrmGetActiveForm());
          FrmDrawForm(FrmGetActiveForm());
          handled = true;
          break;
        }

        case IDC_PB_LOAD_MEMO:
          /*----------------------------------------------------------*/
          /* Call MemoPad with selected memo, jump to error position, */
          /* if known                                                 */
          /*----------------------------------------------------------*/
          if (sel != -1)
            startMemoPad(recIdx[sel],sysFileCMemo);
          handled = true;
          break;

        case IDC_PB_LOAD_PEDIT:
          /*----------------------------------------------------------*/
          /* Call PEDIT with selected memo, jump to error position,   */
          /* if known                                                 */
          /*----------------------------------------------------------*/
          if (sel != -1)
            startMemoPad(recIdx[sel],PEDIT_APPID);
          handled = true;
          break;

        case IDC_PB_LOAD_EDIT:
          /*----------------------------------------------------------*/
          /* Call internal editor with selected memo                  */
          /*----------------------------------------------------------*/
          if (sel != -1)
          {
            selMemo = recIdx[sel]; 
            FrmPopupForm(IDD_Edit);
          }
          handled = true;
          break;
      }
    }
    default:
  }
  CALLBACK_EPILOGUE
  return handled;
}

/**********************************************************************/
/* Event handler for edit form                                        */
/**********************************************************************/
static Boolean EditFrameHandleEvent(EventType *e)
{
  static FieldPtr editField;
  Boolean handled = false;
  CALLBACK_PROLOGUE

  handleScrollEvents(e);
  switch (e->eType)
  {
    case frmOpenEvent:
    {
      /*--------------------------------------------------------------*/
      /* Set field handle to memo DB record                           */
      /*--------------------------------------------------------------*/
      MemHandle oldHandle;
      MemHandle recHand = DmQueryRecord(memoRef, selMemo);
      FormPtr frm = FrmGetActiveForm();
      handleLefty(frm);
      editField = ptrFromObjID(IDC_EF_OUTPUT);

      /*--------------------------------------------------------------*/
      /* Set field size according to preferences                      */
      /*--------------------------------------------------------------*/
      FldSetMaxChars(editField,
                     LispMePrefs.bigMemo ? BIG_EDIT : STD_EDIT);

      oldHandle = FldGetTextHandle(editField);
      FldSetTextHandle(editField, recHand);
      if (oldHandle)
        MemHandleFree(oldHandle);

      /*--------------------------------------------------------------*/
      /* Scroll to error position if known                            */
      /*--------------------------------------------------------------*/
      if (startPtr)
      {
        FldSetSelection(editField, currPtr-startPtr-1, currPtr-startPtr);
        FldSetInsPtPosition(editField, currPtr-startPtr-1);
      }  
      else 
        FldSetInsPtPosition(editField, 0);
      updateScrollBar();
      FrmDrawForm(frm);
      FrmSetFocus(frm, FrmGetObjectIndex(frm,IDC_EF_OUTPUT));
      handled = true;
      break;
    }

    case frmCloseEvent:
      FldSetTextHandle(editField, NULL);
      handled = true;
      break;

    case ctlSelectEvent:
      switch (e->data.ctlSelect.controlID)
      {
        case IDC_PB_EDIT_DONE:
          /*----------------------------------------------------------*/
          /* Clean up                                                 */
          /*----------------------------------------------------------*/
          FldSetTextHandle(editField, NULL);
          FrmReturnToForm(IDD_Load);
          handled = true;
          break;

        case IDC_PB_EDIT_EVAL:
        {
          /*----------------------------------------------------------*/
          /* Copy selection to input field and press EVAL             */
          /*----------------------------------------------------------*/
          UInt16 start, end;
          FldGetSelection(editField, &start, &end);
          FldSetSelection(inField, 0, FldGetTextLength(inField));
          FldInsert(inField, FldGetTextPtr(editField)+start, end-start);
          FldSetTextHandle(editField, NULL);
          FrmReturnToForm(IDD_Load);
          CtlHitControl(ptrFromObjID(IDC_PB_LOAD_CANCEL));
          CtlHitControl(FrmGetObjectPtr(mainForm,
                          FrmGetObjectIndex(mainForm, IDC_PB_EVAL)));
          handled = true;
          break;
        }  

        case IDC_PT_SYMS:
          /*----------------------------------------------------------*/
          /* Build list of known symbols                              */
          /*----------------------------------------------------------*/
          fillSymbolList();
          break;
      }
      break; 

    case popSelectEvent:
      /*--------------------------------------------------------------*/
      /* Insert selected symbol into field                            */
      /*--------------------------------------------------------------*/
      if (symbols)
      {
        UInt16 sel = e->data.popSelect.selection;
        char*  sym = LstGetSelectionText((ListPtr)e->data.popSelect.listP,sel);
        FldInsert(editField, sym, StrLen(sym));
      }
      handled = true;
      break;

    default:
  }
  CALLBACK_EPILOGUE
  return handled;
}


/**********************************************************************/
/* Stub for Scheme event handler                                      */
/**********************************************************************/
static Boolean LispHandleEvent(EventType *e)
{
  static  UInt32 wdog[] = {1000ul,3000ul,6000ul,30000ul};
  Boolean handled = true;
  UInt32  timeLimit;

  CALLBACK_PROLOGUE

  if (LispMePrefs.watchDogSel == 4)
    timeLimit = ULONG_MAX;
  else
    timeLimit = TimGetTicks()+wdog[LispMePrefs.watchDogSel];

  if (e->eType == frmOpenEvent)
    FrmDrawForm(FrmGetActiveForm());

  GrabMem();
  if (!returning)
  {
    S = cons(contexts[actContext].handler,cons(MakeLispEvent(e),NIL));
    C = cons(MKINT(APC),cons(MKINT(STOP),NIL));
    E = pMemGlobal->tlVals;
    W = NIL;
  }

  running = true;
again:
  ErrTry {
    quitHandler = changeHandler = false;
    evalMacro = true;
    handled = exec() != FALSE;
    evalMacro = false;
    ReleaseMem();
  }
  ErrCatch(err) {
    displayError(err);
    goto cleanup;
  } ErrEndCatch

  returning = false;
  if (quitHandler)
  {
    /*----------------------------------------------------------------*/
    /* frm-return was called, restore context as before frm-popup     */
    /*----------------------------------------------------------------*/
    GrabMem();
    W = contexts[actContext--].prevCont;
    S = cons(car(S),car(W));
    E = cadr(W);
    C = caddr(W);
    D = cdddr(W);
    returning = true;
    ReleaseMem();
    FrmReturnToForm(0);
    GUIFreeList();
  }
  else if (changeHandler)
  {
    /*----------------------------------------------------------------*/
    /* frm-goto was called, leave this handler but don't change stack */
    /*----------------------------------------------------------------*/
  }
  else if (running)
  {
    if (TimGetTicks() <= timeLimit)
    {
      GrabMem();
      goto again;
    }
    /*----------------------------------------------------------------*/
    /* Too many steps !                                               */
    /*----------------------------------------------------------------*/
    displayError(ERR_U3_WATCHDOG);
  cleanup:
    FrmReturnToForm(IDD_MainFrame);
    cleanUpEval(true);
    enableCtls(true);
    quitHandler = true;
  }

  CALLBACK_EPILOGUE
  return handled;
}

/**********************************************************************/
/* init all subsystems                                                */
/**********************************************************************/
static Boolean StartApp(char* startSess)
{
  SystemPreferencesType sysp;

  Err     error;
  UInt32  ver;

  /*------------------------------------------------------------------*/
  /* Check for PalmOS2                                                */
  /*------------------------------------------------------------------*/
  error = FtrGet(sysFtrCreator, sysFtrNumROMVersion, &ver);
  if (error || ver < 0x02000000)
  {
    FrmAlert(ERR_O4_OS_VERSION);
    return false;
  }

  /*------------------------------------------------------------------*/
  /* Version-specific settings                                        */
  /*------------------------------------------------------------------*/
  palmIII      = ver >= 0x03000000;
  charEllipsis = ver >= 0x03100000 ? 0x18 : 0x85;
  charNumSpace = ver >= 0x03100000 ? 0x19 : 0x80;
  newGraphic   = ver >= 0x03500000;

  /*------------------------------------------------------------------*/
  /* Open DB and init memory                                          */
  /*------------------------------------------------------------------*/
  startPanel = InitDB(startSess) ? IDD_MainFrame : IDD_Sess;
  if (startSess && startPanel == IDD_Sess)
  {
    FrmCustomAlert(ERR_OX_SESSION_MISS,startSess,0,0);
    return false;
  }
  InitMathLib();

  /*------------------------------------------------------------------*/
  /* Search memo DB and set access mode acc. to secret pref.          */
  /*------------------------------------------------------------------*/
  memoId  = DmFindDatabase(0, "MemoDB");
  ErrFatalDisplayIf(!memoId,"No memoDB?");
  PrefGetPreferences(&sysp);
  if (!sysp.hideSecretRecords)
    memoDBMode |= dmModeShowSecret;

  /*------------------------------------------------------------------*/
  /* Search HanDBase application                                      */
  /*------------------------------------------------------------------*/
  handBaseID = DmFindDatabase(0, "HanDBase");
  return true;
}

/**********************************************************************/
/* shutdown all subsystems                                            */
/**********************************************************************/
static void StopApp(void)
{
  FrmCloseAllForms();
  CloseMathLib();
  ShutdownDB();
}

