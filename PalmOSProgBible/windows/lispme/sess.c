/**********************************************************************/
/*                                                                    */
/* sess.c: Session handling                                           */
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
#include "LispMe.h"
#include "sess.h"
#include "util.h"
#include "io.h"
#include "file.h"
#include "callback.h"

/**********************************************************************/
/* local defines                                                      */
/**********************************************************************/
#define MAX_SESS     32

/**********************************************************************/
/* predeclare static functions                                        */
/**********************************************************************/

/**********************************************************************/
/* global data                                                        */
/**********************************************************************/
UInt16 startPanel;
int    lastSess;

/**********************************************************************/
/* static data                                                        */
/**********************************************************************/
static TablePtr         tbl;
static struct Session** pSess = (struct Session**) NULL;
static int              numSess;
static int              topRow;
static Boolean          connected = false;

/**********************************************************************/
/* Static functions                                                   */
/**********************************************************************/
static Err     CreateSession(char* sessName);
static void    DeleteDB(char* dbName);
static Boolean createIcon(char* sessName);
static void    CreateRecord(UInt16 rec, UInt32 size);

/**********************************************************************/
/* Draw data callback procedure for session table                     */
/**********************************************************************/
static void drawSessName(void* tbl, UInt16 row, UInt16 col,
                         RectanglePtr bounds)
{
  char*  sess;
  FontID lastFont;

  CALLBACK_PROLOGUE
  if (pSess[topRow+row])
  {
    lastFont = FntSetFont(stdFont);
    sess = pSess[topRow+row]->name;
    WinDrawChars(sess, StrLen(sess), bounds->topLeft.x, bounds->topLeft.y);
    FntSetFont(lastFont);
  }
  CALLBACK_EPILOGUE
}

/**********************************************************************/
/* Draw session table                                                 */
/**********************************************************************/
static void drawSessTable(void)
{
  int i;

  for (i=0;i<SESS_ROWS;++i)
  {
    if (topRow+i < numSess && pSess[topRow+i])
    {
      TblSetItemInt(tbl,i,0,pSess[topRow+i]->icon);
      TblSetItemInt(tbl,i,1,pSess[topRow+i]->size);
      TblSetRowUsable(tbl,i,true);
    }
    else
      TblSetRowUsable(tbl,i,false);
  }
  TblMarkTableInvalid(tbl);
  TblRedrawTable(tbl);
}

/**********************************************************************/
/* Make parameter top visible line; care for overflows                */
/**********************************************************************/
static void makeSessVisible(int sess)
{
  int maxRow = max(0,numSess-SESS_ROWS);
  TblUnhighlightSelection(tbl);
  topRow = sess < 0 ? 0 : sess > maxRow ? maxRow : sess;
  drawSessTable();
}

/**********************************************************************/
/* Retrieve selected memory size from list box                        */
/**********************************************************************/
static Int32 prefSelection(FormPtr form, UInt16 obj, int sel)
{
  return StrAToI(((ListPtr)FrmGetObjectPtr(form,FrmGetObjectIndex(form,obj)))->
                 itemsText[sel]);
}

/**********************************************************************/
/* Event handler for session form                                     */
/**********************************************************************/
Boolean SessHandleEvent(EventType *e)
{
  Boolean handled = false;
  UInt16  card;
  LocalID id;
  UInt32  totBytes;
  char*   sess;

  CALLBACK_PROLOGUE

  switch (e->eType)
  {
    case frmOpenEvent:
    {
      DmSearchStateType state;
      Boolean           first;
      UInt16            i;

      startPanel = IDD_Sess;

      /*--------------------------------------------------------------*/
      /* Read all available LispMe databases                          */
      /*--------------------------------------------------------------*/
      pSess  = MemPtrNew(MAX_SESS * sizeof(struct Session*));
      MemSet(pSess,MAX_SESS * sizeof(struct Session*), 0);
      for (numSess=0, first=true;
           numSess<MAX_SESS &&
           !DmGetNextDatabaseByTypeCreator(first,&state,
                                           'data',APPID,false,&card,&id);
           ++numSess, first=false)
      {
        pSess[numSess] = MemPtrNew(sizeof(struct Session));
        pSess[numSess]->card = card;
        pSess[numSess]->id   = id;
        sess = pSess[numSess]->name;
        DmDatabaseInfo(card,id,sess,0,0,0,0,0,0,0,0,0,0);
        DmDatabaseSize(card,id,0,&totBytes,0);
        pSess[numSess]->size = totBytes/1024 + 1;
        *sess ^= 0x80;
        pSess[numSess]->icon = !!DmFindDatabase(0,sess);
        *sess ^= 0x80;
      }

      /*--------------------------------------------------------------*/
      /* Init UI table                                                */
      /*--------------------------------------------------------------*/
      tbl  = ptrFromObjID(IDC_TAB_SESSION);
      for (i=0;i<SESS_ROWS;++i)
      {
        TblSetItemStyle(tbl,i,0,checkboxTableItem);
        TblSetItemStyle(tbl,i,1,numericTableItem);
        TblSetItemStyle(tbl,i,2,customTableItem);
      }
      TblSetCustomDrawProcedure(tbl,2,(void*) drawSessName);
      for (i=0;i<3;++i)
        TblSetColumnUsable(tbl,i,true);

      /*--------------------------------------------------------------*/
      /* Select currently active session                              */
      /*--------------------------------------------------------------*/
      makeSessVisible(0);
      for (i=0;i<numSess;++i)
        if (!StrCompare(LispMePrefs.sessDB, pSess[i]->name))
        {
          makeSessVisible(i);
          break;
        }
      FrmDrawForm(FrmGetActiveForm());
      if (i<numSess)
        TblSelectItem(tbl,i-topRow,2);

      /*--------------------------------------------------------------*/
      /* Disconnect session to allow deleting current one             */
      /*--------------------------------------------------------------*/
      DisConnectSession();
      handled = true;
      break;
    }

    case frmUpdateEvent:
    {
      /*--------------------------------------------------------------*/
      /* Re-read DB size                                              */
      /*--------------------------------------------------------------*/
      int idx = e->data.frmUpdate.updateCode;
      if (idx >= 0 && idx < numSess && pSess[idx])
      {
        DmDatabaseSize(pSess[idx]->card,pSess[idx]->id,0,&totBytes,0);
        pSess[idx]->size = totBytes/1024 + 1;
        TblUnhighlightSelection(tbl);
        drawSessTable();
        FrmDrawForm(FrmGetActiveForm());
        TblSelectItem(tbl,idx-topRow,2);
        handled = true;
      }  
      break;
    }

    case keyDownEvent:
      switch (e->data.keyDown.chr)
      {
        case pageUpChr:
          /*----------------------------------------------------------*/
          /* Scroll up one page                                       */
          /*----------------------------------------------------------*/
          makeSessVisible(topRow-SESS_ROWS+1);
          handled = true;
          break;

        case pageDownChr:
          /*----------------------------------------------------------*/
          /* Scroll down one page                                     */
          /*----------------------------------------------------------*/
          makeSessVisible(topRow+SESS_ROWS-1);
          handled = true;
          break;
      }
      break;

    case tblSelectEvent:
      if (e->data.tblSelect.column == 0)
      {
        int   row  = e->data.tblSelect.row;
        char* sess = pSess[topRow+row]->name;

        /*------------------------------------------------------------*/
        /* Icon checkbox tapped                                       */
        /*------------------------------------------------------------*/
        if ((pSess[topRow+row]->icon = TblGetItemInt(tbl,row,0)))
        {
          /*----------------------------------------------------------*/
          /* Create launcher icon                                     */
          /*----------------------------------------------------------*/
          if (!createIcon(sess))
          {
            FrmAlert(ERR_L6_CREATE_ICON);
            TblSetItemInt(tbl,row,0,false);
            pSess[topRow+row]->icon = false;
          }
        }
        else
        {
          /*----------------------------------------------------------*/
          /* Remove launcher icon                                     */
          /*----------------------------------------------------------*/
          *sess ^= 0x80;
          DeleteDB(sess);
          *sess ^= 0x80;
        }
        handled = true;
      }
      break;

    case ctlSelectEvent:
      switch (e->data.ctlSelect.controlID)
      {
        case IDC_PB_SESS_USE:
        {
          /*----------------------------------------------------------*/
          /* Activate selected session                                */
          /*----------------------------------------------------------*/
          UInt16 sel, col;
          handled = true;
          if (TblGetSelection(tbl,&sel,&col))
          {
            int i;
            StrCopy(LispMePrefs.sessDB, pSess[topRow+sel]->name);
            PrefSetAppPreferencesV10(APPID, VERSION,
                                     &LispMePrefs, sizeof(LispMePrefs));
            if (!ConnectSession(LispMePrefs.sessDB, false))
            {
              FrmAlert(ERR_O10_ILLEGAL_SESS);
              handled = true;
              break;
            }  

            /*--------------------------------------------------------*/
            /* Connect OK, free session list and switch to it         */
            /*--------------------------------------------------------*/
            for (i=0; i<numSess; ++i)
              MemPtrFree(pSess[i]);
            MemPtrFree(pSess);
            FrmGotoForm(IDD_MainFrame);
            break;
          }
          else
            FrmAlert(ERR_O9_NO_SESS);
          break;
        }

        case IDC_PB_SESS_NEW:
          /*----------------------------------------------------------*/
          /* Create a new session with default settings               */
          /*----------------------------------------------------------*/
          if (numSess >= MAX_SESS)
            FrmAlert(ERR_O3_MAX_SESS);
          else
            FrmPopupForm(IDD_NewSess);
          handled = true;
          break;

        case IDC_PB_SESS_DEL:
        {
          /*----------------------------------------------------------*/
          /* Delete selected session                                  */
          /*----------------------------------------------------------*/
          int i;
          UInt16 sel, col;
          handled = true;
          if (TblGetSelection(tbl,&sel,&col))
          {
            if (FrmCustomAlert(ERR_O8_DEL_SESS,pSess[topRow+sel]->name,
                               0,0) == 0)
            {
              /*------------------------------------------------------*/
              /* Remove session DB and launcher icon                  */
              /*------------------------------------------------------*/
              char* sess = pSess[topRow+sel]->name;

              DeleteDB(sess);
              if (pSess[topRow+sel]->icon)
              {
                *sess ^= 0x80;
                DeleteDB(sess);
                *sess ^= 0x80;
              }
              MemPtrFree(pSess[topRow+sel]);
              --numSess;
              for (i=topRow+sel; i<numSess; ++i)
                pSess[i] = pSess[i+1];
              pSess[numSess] = NULL;
              makeSessVisible(topRow);
            }
          }
          else
            FrmAlert(ERR_O9_NO_SESS);
          handled = true;
          break;
        }

        case IDC_PB_SESS_SIZE:
        {
          /*----------------------------------------------------------*/
          /* Configure selected session                               */
          /*----------------------------------------------------------*/
          UInt16 sel, col;
          handled = true;
          if (TblGetSelection(tbl,&sel,&col))
          {
            if (!ConnectSession(pSess[topRow+sel]->name,false))
            {  
              FrmAlert(ERR_O10_ILLEGAL_SESS); 
              break;
            }  
            lastSess = topRow+sel;
            FrmPopupForm(IDD_SetupSess);
          }
          else
            FrmAlert(ERR_O9_NO_SESS);
          break;
        }
      }
    default:
  }
  CALLBACK_EPILOGUE
  return handled;
}

/**********************************************************************/
/* Event handler for new session name form                            */
/**********************************************************************/
Boolean NewSessHandleEvent(EventType *e)
{
  Boolean handled = false;

  CALLBACK_PROLOGUE

  switch (e->eType)
  {
    case frmOpenEvent:
    {
      FormPtr form = FrmGetActiveForm();
      FrmDrawForm(form);
      FrmSetFocus(form,FrmGetObjectIndex(form,IDC_EF_NSESS_NAME));
      handled = true;
      break;
    }

    case ctlSelectEvent:
      handled = true;
      switch (e->data.ctlSelect.controlID)
      {
        case IDC_PB_NSESS_OK:
        {
          /*----------------------------------------------------------*/
          /* Accept new name                                          */
          /*----------------------------------------------------------*/
          char* p = FldGetTextPtr((FieldPtr)ptrFromObjID(IDC_EF_NSESS_NAME));
          pSess[numSess] = MemPtrNew(sizeof(struct Session));
          StrCopy(pSess[numSess]->name, p ? p : "");
          pSess[numSess]->icon = false;
          pSess[numSess]->size = 4;
          switch (CreateSession(pSess[numSess]->name))
          {
            case 0:
              pSess[numSess]->card = 0;
              pSess[numSess]->id   = dbId;
              DisConnectSession();
              FrmReturnToForm(IDD_Sess);
              makeSessVisible(numSess++);
              break;

            case dmErrInvalidDatabaseName:
              FrmCustomAlert(ERR_M3_CREATE_SESS,pSess[numSess]->name,
                             "invalid name",0);
              goto cleanUp;

            case dmErrAlreadyExists:
              FrmCustomAlert(ERR_M3_CREATE_SESS,pSess[numSess]->name,
                             "already exists",0);
              goto cleanUp;

            default:
              FrmCustomAlert(ERR_M3_CREATE_SESS,pSess[numSess]->name,
                             "general error",0);
            cleanUp:
              MemPtrFree(pSess[numSess]);
              break;
          }
          break;
        }

        case IDC_PB_NSESS_CANCEL:
          /*----------------------------------------------------------*/
          /* Abort new session creation                               */
          /*----------------------------------------------------------*/
          FrmReturnToForm(IDD_Sess);
          break;
      }
    default:
  }
  CALLBACK_EPILOGUE
  return handled;
}

/**********************************************************************/
/* Copy a resource                                                    */
/**********************************************************************/
static Boolean CopyResource(DmOpenRef db, UInt32 type, UInt16 destID, UInt16 srcId)
{
  MemHandle srcHand  = DmGetResource(type, srcId);
  MemHandle destHand = DmNewResource(db, type, destID, MemHandleSize(srcHand));
  if (!destHand)
    return true;
  DmWrite(MemHandleLock(destHand), 0, MemHandleLock(srcHand),
          MemHandleSize(srcHand));
  MemHandleUnlock(destHand);
  MemHandleUnlock(srcHand);
  DmReleaseResource(destHand);
  return false;
}

/**********************************************************************/
/* Create starter icon for a session                                  */
/**********************************************************************/
static Boolean createIcon(char* sessName)
{
  LocalID           iconDBid;
  DmOpenRef         iconDBRef;
  char              fullName[dmDBNameLength];
  MemHandle         destHand;
  UInt32            appId = 'fbLA';
  UInt16            card;
  DmSearchStateType state;

  /*------------------------------------------------------------------*/
  /* Find an unused APPID starting with 'fbLA'                        */
  /*------------------------------------------------------------------*/
  while (!DmGetNextDatabaseByTypeCreator(true,&state,0,appId,true,
                                         &card,&iconDBid))
    ++appId;

  /*------------------------------------------------------------------*/
  /* Generate name for starter app                                    */
  /*------------------------------------------------------------------*/
  StrCopy(fullName,sessName);
  fullName[0] ^= 0x80;

  if (DmCreateDatabase(0, fullName, appId, 'appl', true))
    return false;
  iconDBid  = DmFindDatabase(0, fullName);
  iconDBRef = DmOpenDatabase(0, iconDBid, dmModeReadWrite);

  /*------------------------------------------------------------------*/
  /* Copy resources from LispMe                                       */
  /*------------------------------------------------------------------*/
  if (CopyResource(iconDBRef, 'code',    1, 9001)             ||
      CopyResource(iconDBRef, 'Talt', 1000, ERR_L4_NO_LISPME) ||
      CopyResource(iconDBRef, 'tAIB', 1000, 9000)             ||
      CopyResource(iconDBRef, 'tAIB', 1001, 9001))
    return false;

  /*------------------------------------------------------------------*/
  /* Create tAIN resource use both for icon name and session DB name  */
  /*------------------------------------------------------------------*/
  destHand = DmNewResource(iconDBRef, 'tAIN', 1000, StrLen(sessName)+1);
  if (!destHand)
    return false;
  DmWrite(MemHandleLock(destHand), 0, sessName, StrLen(sessName)+1);
  MemHandleUnlock(destHand);
  DmReleaseResource(destHand);

  DmCloseDatabase(iconDBRef);
  return true;
}

/**********************************************************************/
/* Create a record in DB                                              */
/**********************************************************************/
static void CreateRecord(UInt16 rec, UInt32 size)
{
  ErrFatalDisplayIf(!DmNewRecord(dbRef, &rec, size),
                    "Can't create DB rec");
  DmReleaseRecord(dbRef, rec, true);
}

/**********************************************************************/
/* Create a new LispMe session database                               */
/**********************************************************************/
static Err CreateSession(char* sessName)
{
  Err     err;
  FormPtr form;

  DisConnectSession();

  form = FrmInitForm(IDD_SetupSess);
  heapSize = prefSelection(form, IDC_PL_HEAP, 0);
  atomSize = prefSelection(form, IDC_PL_ATOM, 0);
  realSize = prefSelection(form, IDC_PL_REAL, 0);
  outpSize = prefSelection(form, IDC_PL_OUTP, 0);
  FrmDeleteForm(form);

  /*------------------------------------------------------------------*/
  /* Create new LispMe DB                                             */
  /*------------------------------------------------------------------*/
  if (!(err = DmCreateDatabase(0,sessName,APPID,'data',false)))
  {
    dbId  = DmFindDatabase(0, sessName);
    dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite);
    CreateRecord(REC_GLOB,sizeof(struct MemGlobal));
    CreateRecord(REC_ATOM,atomSize);
    CreateRecord(REC_REAL,realSize);
    CreateRecord(REC_HEAP,heapSize);
    CreateRecord(REC_OUT, outpSize);
    CreateRecord(REC_INP, 1); // field will resize as necessary      
    DmCloseDatabase(dbRef);
    ErrFatalDisplayIf(!ConnectSession(sessName, true),
                      "Can't connect to new session ?!?!?");
  }
  return err;
}

/**********************************************************************/
/* Delete a LispMe database                                           */
/**********************************************************************/
static void DeleteDB(char* dbName)
{
  LocalID dbId = DmFindDatabase(0, dbName);
  ErrFatalDisplayIf(!dbId,"DB not found");
  ErrFatalDisplayIf(DmDeleteDatabase(0,dbId),"Delete DB");
}

/**********************************************************************/
/* Connect to LispMe session database                                 */
/**********************************************************************/
Boolean ConnectSession(char* sessName, Boolean newDB)
{
  DisConnectSession();

  dbId = DmFindDatabase(0, sessName);
  if (!dbId)
    return false;
  dbRef = DmOpenDatabase(0,dbId,dmModeReadWrite);

  /*------------------------------------------------------------------*/
  /* Retrieve handles                                                 */
  /*------------------------------------------------------------------*/
  globHandle = DmGetRecord(dbRef, REC_GLOB);
  pMemGlobal = MemHandleLock(globHandle);
  if (!newDB && pMemGlobal->magic != SESAME) 
  {
    /*----------------------------------------------------------------*/
    /* Incompatible version, release resources currently acquired     */
    /*----------------------------------------------------------------*/
    MemHandleUnlock(globHandle);
    DmReleaseRecord(dbRef, REC_GLOB, false);
    DmCloseDatabase(dbRef);
    return false;
  } 
  MemHandleUnlock(globHandle);
  
  atomHandle = DmGetRecord(dbRef, REC_ATOM);
  realHandle = DmGetRecord(dbRef, REC_REAL);
  heapHandle = DmGetRecord(dbRef, REC_HEAP);
  outHandle  = DmGetRecord(dbRef, REC_OUT);
  inHandle   = DmGetRecord(dbRef, REC_INP);
  LockMem();

  /*------------------------------------------------------------------*/
  /* Retrieve memory sizes                                            */
  /*------------------------------------------------------------------*/
  if (newDB)
    DmSet(pMemGlobal,0,sizeof(struct MemGlobal), 0);
  else
  {
    heapSize = MemHandleSize(heapHandle);
    atomSize = MemHandleSize(atomHandle);
    realSize = MemHandleSize(realHandle);
    outpSize = MemHandleSize(outHandle);
  }

  /*------------------------------------------------------------------*/
  /* Retrieve globals                                                 */
  /*------------------------------------------------------------------*/
  S = pMemGlobal->S;
  E = pMemGlobal->E;
  C = pMemGlobal->C;
  D = pMemGlobal->D;
  W = pMemGlobal->W;
  firstFree     = pMemGlobal->firstFree;
  firstFreeReal = pMemGlobal->firstFreeReal;
  running       = pMemGlobal->running;
  GrabMem();
  if (!running)
    pMemGlobal->ownGUI = false;
  ReleaseMem();
  outPos    = pMemGlobal->outPos;
  numStep   = pMemGlobal->numStep;
  numGC     = pMemGlobal->numGC;
  tickStart = pMemGlobal->tickStart;
  caseSens  = pMemGlobal->caseSens;
  initHeap(newDB,false);
  MemMove(msg, pMemGlobal->msg, MSG_OUTPUT_SIZE);
  msg[MSG_OUTPUT_SIZE] = '\0';
  GrabMem();
  openResDB();
  ReleaseMem();
  connected = true;
  return true;
}

/**********************************************************************/
/* Disconnect current LispMe session database                         */
/* (but keep current resource DB open to be able to copy an icon out  */
/* of it)                                                             */
/**********************************************************************/
void DisConnectSession(void)
{
  if (connected)
  {
    GrabMem();
    pMemGlobal->S = S;
    pMemGlobal->E = E;
    pMemGlobal->C = C;
    pMemGlobal->D = D;
    pMemGlobal->W = W;
    pMemGlobal->firstFree = firstFree;
    pMemGlobal->firstFreeReal = firstFreeReal;
    pMemGlobal->running   = running;
    pMemGlobal->outPos    = outPos;
    pMemGlobal->numStep   = numStep;
    pMemGlobal->numGC     = numGC;
    pMemGlobal->tickStart = tickStart;
    pMemGlobal->caseSens  = caseSens;
    MemMove(pMemGlobal->msg, msg, MSG_OUTPUT_SIZE);
    pMemGlobal->msg[MSG_OUTPUT_SIZE] = '\0';
    ReleaseMem();
    UnlockMem();
    DmReleaseRecord(dbRef, REC_INP,  true);
    DmReleaseRecord(dbRef, REC_OUT,  true);
    DmReleaseRecord(dbRef, REC_GLOB, true);
    DmReleaseRecord(dbRef, REC_HEAP, true);
    DmReleaseRecord(dbRef, REC_REAL, true);
    DmReleaseRecord(dbRef, REC_ATOM, true);
    DmCloseDatabase(dbRef);
  }
  connected = false;
}

/**********************************************************************/
/* Init database and local memory                                     */
/**********************************************************************/
Boolean InitDB(char* startSess)
{
  /*------------------------------------------------------------------*/
  /* Allocate GC bitvectors                                           */
  /*------------------------------------------------------------------*/
  markBit       = MemPtrNew(MAX_HEAP_SIZE>>5) + 0x0400;
  markRealBit   = MemPtrNew(MAX_REAL_SIZE>>6);

  /*------------------------------------------------------------------*/
  /* Read application preferences                                     */
  /*------------------------------------------------------------------*/
  if (!PrefGetAppPreferencesV10(APPID, VERSION,
                                &LispMePrefs, sizeof(LispMePrefs)))
  {
    /*----------------------------------------------------------------*/
    /* Default preferences                                            */
    /*----------------------------------------------------------------*/
    MemSet(&LispMePrefs,sizeof(LispMePrefs),0);
    LispMePrefs.printDepth = 10;
    return false;
  }
  else
  {
    if (startSess)
      StrCopy(LispMePrefs.sessDB,startSess);
    return ConnectSession(LispMePrefs.sessDB,false);
  }
}

/**********************************************************************/
/* Shutdown database                                                  */
/**********************************************************************/
void ShutdownDB(void)
{
  DisConnectSession();
}

/**********************************************************************/
/* Lock memory                                                        */
/**********************************************************************/
void LockMem(void)
{
  strStore  = MemHandleLock(atomHandle);
  heap      = MemHandleLock(heapHandle) + 0x8000L;
  reals     = MemHandleLock(realHandle);
  pMemGlobal= MemHandleLock(globHandle);
}

/**********************************************************************/
/* Unlock memory                                                      */
/**********************************************************************/
void UnlockMem(void)
{
  MemHandleUnlock(globHandle);
  MemHandleUnlock(realHandle);
  MemHandleUnlock(heapHandle);
  MemHandleUnlock(atomHandle);
}

