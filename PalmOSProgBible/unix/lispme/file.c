/**********************************************************************/
/*                                                                    */
/* file.c: LISPME "file" (Memo) management                            */
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
/* 10.10.1998 New                                                FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "file.h"
#include "io.h"
#include "lispme.h"
#include "vm.h"
#include "util.h"

/**********************************************************************/
/* Module local data                                                  */
/**********************************************************************/
static DmOpenRef resDBRef;

/**********************************************************************/
/* Read from a database                                               */
/**********************************************************************/
PTR readDB(Boolean isRes, PTR type, int recNum)
{
  LocalID   dbId;
  DmOpenRef locDBRef;
  MemHandle recHand;
  UInt16    attr;
  PTR       res;

  if (!IS_STRING(type))
    typeError(type,"string");
  printSEXP(type, PRT_MESSAGE);

  if (isRes)
    recHand = DmGetResource(*((UInt32*)msg),recNum);
  else
  {
    if (!(dbId = DmFindDatabase(0, msg)))
      return FALSE;
    locDBRef = DmOpenDatabase(0, dbId, dmModeReadOnly);
    DmDatabaseInfo(0,dbId,NULL,&attr,0,0,0,0,0,0,0,0,0);
    if (attr & dmHdrAttrResDB)
      goto cleanUp;
    recHand = DmQueryRecord(locDBRef, recNum);
  }

  if (recHand)
  {
    ErrTry {
      res = makeString(MemHandleSize(recHand), MemHandleLock(recHand),
                       0, 0, NIL);
    } ErrCatch(err) {
      MemHandleUnlock(recHand);
      if (isRes)
        DmReleaseResource(recHand);
      else
        DmCloseDatabase(locDBRef);
      ErrThrow(err);
    } ErrEndCatch
    MemHandleUnlock(recHand);
    if (isRes)
      DmReleaseResource(recHand);
    else
      DmCloseDatabase(locDBRef);
    return res;
  }
  else
  {
  cleanUp:
    if (!isRes)
      DmCloseDatabase(locDBRef);
    return FALSE;
  }
}

/**********************************************************************/
/* Write to a database (or delete rec, if data == #f)                 */
/**********************************************************************/
PTR writeDB(PTR dbName, UInt16 recNum, PTR data)
{
  LocalID   locDBId;
  DmOpenRef locDBRef;
  MemHandle recHand;
  MemHandle srcHand;
  UInt16    attr;
  UInt16    index;
  UInt16    len;

  if (!IS_STRING(dbName))
    typeError(dbName,"string");
  if (!IS_STRING(data) && data != FALSE)
    typeError(data,"string");
  printSEXP(dbName, PRT_MESSAGE);

  if (!(locDBId  = DmFindDatabase(0, msg)) ||
      !(locDBRef = DmOpenDatabase(0, locDBId, dmModeReadWrite)))
    return FALSE;
  DmDatabaseInfo(0,locDBId,NULL,&attr,0,0,0,0,0,0,0,0,0);
  if (attr & dmHdrAttrResDB)
    goto cleanUp;
  if (data == FALSE)
  {
    PTR res = !DmQueryRecord(locDBRef,recNum) ||
              DmRemoveRecord(locDBRef,recNum) ? FALSE : TRUE;
    DmCloseDatabase(locDBRef);
    return res;
  }
  else
  {
    len     = stringLength(data);
    recHand = DmNewRecord(locDBRef, &recNum, len);

    if (recHand)
    {
      DmFindRecordByID(dbRef, UID_STR(data), &index);
      srcHand = DmQueryRecord(dbRef,index);
      DmWrite(MemHandleLock(recHand),0,MemHandleLock(srcHand),len);
      MemHandleUnlock(recHand);
      MemHandleUnlock(srcHand);
      DmReleaseRecord(locDBRef,recNum,true);
      DmCloseDatabase(locDBRef);
      return MKINT(recNum);
    }
    else
    {
    cleanUp:
      DmCloseDatabase(locDBRef);
      return FALSE;
    }
  }
}

/**********************************************************************/
/* Get memodirectory                                                  */
/**********************************************************************/
PTR memoDir(PTR cat)
{
  UInt16    numRec,i;
  MemHandle recHand;
  char      *s,*d,*recPtr;
  PTR       res;
  PTR*      dest = &res;
  UInt16    category = dmAllCategories;
  UInt16    recNum   = 0;


  if (cat!=MKINT(0) && !IS_STRING(cat))
    typeError(cat,"string");

  memoRef = DmOpenDatabase(0, memoId, memoDBMode);

  if (IS_STRING(cat))
  {
    printSEXP(cat, PRT_MESSAGE);
    category = CategoryFind(memoRef, msg);
    if (category == dmAllCategories)
    {
      /*--------------------------------------------------------------*/
      /* Not found => return empty list                               */
      /*--------------------------------------------------------------*/
      DmCloseDatabase(memoRef);
      return NIL;
    }
  }

  res = NIL;
  PROTECT(res);
  numRec = DmNumRecordsInCategory(memoRef, category);
  for (i=0;i<numRec;++i)
  {
    if ((recHand = DmQueryNextInCategory(memoRef, &recNum, category)))
    {
      recPtr = MemHandleLock(recHand);
      /*--------------------------------------------------------------*/
      /* Copy beginning of memo text upto first \n or MAX_LINE_LEN    */
      /*--------------------------------------------------------------*/
      for (s = recPtr, d = msg;
           s-recPtr < MAX_FILENAME_LEN && *s && *s != '\n';
           ++s, ++d)
        *d = *s;
      *d = '\0';
      MemHandleUnlock(recHand);
      *dest = cons(str2Lisp(msg),NIL);
      dest = &cdr(*dest);
    }
    ++recNum;
  }
  DmCloseDatabase(memoRef);
  UNPROTECT(res);
  return res;
}

/**********************************************************************/
/* Make a port from components                                        */
/**********************************************************************/
static PTR makeInPort(Int16 pos, UInt32 uid)
{
  return cons(PRTI_TAG,
               cons(MKINT(pos),
                     cons(MKINT(uid >> 12),
                           MKINT(uid & 0x0fff))));
}

static PTR makeOutPort(UInt32 uid)
{
  return cons(PRTO_TAG,
               cons(MKINT(uid >> 12),
                     MKINT(uid & 0x0fff)));
}

/**********************************************************************/
/* Create a file                                                      */
/**********************************************************************/
PTR createFile(PTR fileName)
{
  UInt16    index;
  MemHandle recHand;
  UInt16    nameLen;
  UInt32    uid;

  nameLen = makeFileName(fileName);
  memoRef = DmOpenDatabase(0, memoId, dmModeReadWrite);
  if (!(recHand = DmNewRecord(memoRef, &index, nameLen)))
  {
    DmCloseDatabase(memoRef);
    error1(ERR_R16_CREATE_FILE, fileName);
  }
  DmWrite(MemHandleLock(recHand), 0, msg, nameLen);
  MemHandleUnlock(recHand);
  DmReleaseRecord(memoRef, index, true);
  DmRecordInfo(memoRef, index, 0, &uid, 0);
  DmCloseDatabase(memoRef);
  return makeOutPort(uid);
}

/**********************************************************************/
/* Open an existing file                                              */
/**********************************************************************/
PTR openFile(PTR fileName, Boolean append)
{
  MemHandle recHand;
  UInt16    nameLen;
  UInt32    uid;
  UInt16    numRec,i;
  char      *s,*d,*recPtr;

  nameLen = makeFileName(fileName);
  msg[nameLen-2] = '\0';
  memoRef = DmOpenDatabase(0, memoId, memoDBMode);
  numRec  = DmNumRecords(memoRef);
  for (i=0;i<numRec;++i)
  {
    if ((recHand = DmQueryRecord(memoRef, i)))
    {
      recPtr = MemHandleLock(recHand);
      /*--------------------------------------------------------------*/
      /* Compare first line with filename                             */
      /*--------------------------------------------------------------*/
      for (s = recPtr, d = msg;
           *s && *s == *d;
           ++s, ++d)
        ;
      if ((*s == '\n' && !*d) || s-recPtr >= MAX_FILENAME_LEN)
      {
        if (append)
        {
          MemHandleUnlock(recHand);
          DmRecordInfo(memoRef, i, 0, &uid, 0);
          W = makeOutPort(uid);
        }
        else
        {
          /*----------------------------------------------------------*/
          /* Establish read position                                  */
          /*----------------------------------------------------------*/
          while (*s && *s != '\n') ++s;
          if (*s) ++s;
          MemHandleUnlock(recHand);
          DmRecordInfo(memoRef, i, 0, &uid, 0);
          W = makeInPort(s-recPtr, uid);
        }
        DmCloseDatabase(memoRef);
        return W;
      }
      MemHandleUnlock(recHand);
    }
  }

  DmCloseDatabase(memoRef);
  if (append)
    return createFile(fileName);
  else
    error1(ERR_R17_OPEN_FILE, fileName);
}

/**********************************************************************/
/* Write to a file                                                    */
/**********************************************************************/
void writeFile(PTR expr, PTR port, Boolean machine)
{
  UInt16    index;
  MemHandle recHand;
  UInt32    uid;
  UInt32    oldSize, newSize;
  UInt16    attr;

  if (!IS_OPORT(port))
    typeError(port,"output port");
  memoRef = DmOpenDatabase(0, memoId, dmModeReadWrite);
  uid = UID_PORT(cdr(port));

  /*--------------------------------------------------------------*/
  /* Check for vanished memo                                      */
  /*--------------------------------------------------------------*/
  if (DmFindRecordByID(memoRef, uid, &index))
  {
  cleanup:
    DmCloseDatabase(memoRef);
    ErrThrow(ERR_R18_LOST_FILE);
  }
  DmRecordInfo(memoRef, index, &attr, 0, 0);
  if (attr & dmRecAttrDelete)
    goto cleanup;

  printSEXP(expr, PRT_MEMO | (machine ? PRT_ESCAPE | PRT_SPACE : 0));
  recHand = DmGetRecord(memoRef, index);
  oldSize = MemHandleSize(recHand);
  if (oldSize < MEMO_OUTPUT_SIZE)
  {
    MemHandle newHand;
    newSize = oldSize+StrLen(msg);
    newHand = DmResizeRecord(memoRef, index, min(newSize,MEMO_OUTPUT_SIZE));
    if (!newHand)
    {
      DmReleaseRecord(memoRef, index, false);
      DmCloseDatabase(memoRef);
      ErrThrow(ERR_R19_WRITE_FILE);
    }
 
    if (newSize > MEMO_OUTPUT_SIZE)
    {
      msg[MEMO_OUTPUT_SIZE-oldSize-1] = charEllipsis;
      msg[MEMO_OUTPUT_SIZE-oldSize] = '\0';
    }
    DmStrCopy(MemHandleLock(newHand), oldSize-1, msg);
    MemHandleUnlock(newHand);
  }
  DmReleaseRecord(memoRef, index, true);
  DmCloseDatabase(memoRef);
}

/**********************************************************************/
/* Read from a file                                                   */
/**********************************************************************/
PTR readFile(PTR port, int opcode)
{
  UInt16    index;
  MemHandle recHand;
  UInt32    uid;
  Int16     pos, size;
  UInt16    attr;
  char      *recPtr, *s, *d;

  if (!IS_IPORT(port))
    typeError(port,"input port");
  memoRef = DmOpenDatabase(0, memoId, memoDBMode);
  uid = UID_PORT(cddr(port));
  pos = INTVAL(cadr(port));

  /*--------------------------------------------------------------*/
  /* Check for vanished memo                                      */
  /*--------------------------------------------------------------*/
  if (DmFindRecordByID(memoRef, uid, &index))
  {
  cleanup:
    DmCloseDatabase(memoRef);
    ErrThrow(ERR_R18_LOST_FILE);
  }
  DmRecordInfo(memoRef, index, &attr, 0, 0);
  if (attr & dmRecAttrDelete)
    goto cleanup;

  recHand = DmQueryRecord(memoRef, index);
  size    = MemHandleSize(recHand);
  if (pos >= size-1)
  {
    DmCloseDatabase(memoRef);
    return END_OF_FILE;
  }
  recPtr  = MemHandleLock(recHand);
  switch (opcode)
  {
    case PEEK:
      W = MKCHAR(recPtr[pos]);
      break;

    case REAC:
      W = MKCHAR(recPtr[pos]);
      cadr(port) = MKINT(pos+1);
      break;

    case REDL:
      for (s=recPtr+pos, d=msg; *s && *s!='\n'; ++pos)
        *d++ = *s++;
      *d = '\0';
      if (*s) ++pos;
      W = str2Lisp(msg);
      cadr(port) = MKINT(pos);
      break;

    case READ:
      ErrTry {
         W = readSEXP(recPtr+pos);
      }
      ErrCatch(err) {
        if (onlyWS)
          W = END_OF_FILE;
        else
        {
          MemHandleUnlock(recHand);
          DmCloseDatabase(memoRef);
          ErrThrow(err);
        }
      } ErrEndCatch
      cadr(port) = MKINT(currPtr-recPtr);
      break;
  }
  MemHandleUnlock(recHand);
  DmCloseDatabase(memoRef);
  return W;
}

/**********************************************************************/
/* Delete an existing file                                            */
/**********************************************************************/
void deleteFile(PTR fileName)
{
  MemHandle recHand;
  UInt16    nameLen;
  UInt16    numRec,i;
  char      *s, *d, *recPtr;

  nameLen = makeFileName(fileName);
  msg[nameLen-2] = '\0';
  memoRef = DmOpenDatabase(0, memoId, dmModeReadWrite);
  numRec  = DmNumRecords(memoRef);
  for (i=0;i<numRec;++i)
  {
    if ((recHand = DmQueryRecord(memoRef, i)))
    {
      recPtr = MemHandleLock(recHand);
      /*--------------------------------------------------------------*/
      /* Compare first line with filename                             */
      /*--------------------------------------------------------------*/
      for (s = recPtr, d = msg;
           *s && *s == *d;
           ++s, ++d)
        ;
      if ((*s == '\n' && !*d) || s-recPtr >= MAX_FILENAME_LEN)
      {
        /*------------------------------------------------------------*/
        /* Memo found, delete it                                      */
        /*------------------------------------------------------------*/
        MemHandleUnlock(recHand);
        DmRemoveRecord(memoRef, i);
        DmCloseDatabase(memoRef);
        return;
      }
      MemHandleUnlock(recHand);
    }
  }

  DmCloseDatabase(memoRef);
  error1(ERR_R17_OPEN_FILE, fileName);
}

/**********************************************************************/
/* Open active resource DB                                            */
/**********************************************************************/
void openResDB(void)
{
  LocalID dbId;

  closeResDB();
  if (pMemGlobal->resDBName[0])
  {
    if (!(dbId = DmFindDatabase(0, pMemGlobal->resDBName)))
    {
      /*--------------------------------------------------------------*/ 
      /* No matching name -> set empty name as indicator              */ 
      /*--------------------------------------------------------------*/ 
      pMemGlobal->resDBName[0] = '\0';
      return; 
    }
    
    resDBRef = DmOpenDatabase(0, dbId, dmModeReadOnly);
    if (!resDBRef)
      pMemGlobal->resDBName[0] = '\0';
  }
}

/**********************************************************************/
/* Close active resource DB                                           */
/**********************************************************************/
void closeResDB(void)
{
  if (resDBRef)
    DmCloseDatabase(resDBRef);
  resDBRef = 0;
}

/**********************************************************************/
/* Set resource DB to be used for GUI                                 */
/**********************************************************************/
void setResDB(PTR name)
{
  LocalID   dbId;
  UInt16    attr;

  if (name == FALSE)
  {
    /*----------------------------------------------------------------*/
    /* Close currently opened resource DB                             */
    /*----------------------------------------------------------------*/
    if (pMemGlobal->resDBName[0])
      closeResDB();
    pMemGlobal->resDBName[0] = '\0';
  }
  else
  {
    /*----------------------------------------------------------------*/
    /* Open a new resource DB                                         */
    /*----------------------------------------------------------------*/
    if (!IS_STRING(name))
      typeError(name,"string");
    printSEXP(name, PRT_MESSAGE);

    if (!StrCompare(msg,pMemGlobal->resDBName))
      return;

    if (!(dbId = DmFindDatabase(0, msg)))
      error1(ERR_U4_INVALID_RESDB, name);

    DmDatabaseInfo(0,dbId,NULL,&attr,NULL,NULL,NULL,NULL,
                   NULL,NULL,NULL,NULL,NULL);
    if (attr & dmHdrAttrResDB)
    {
      closeResDB();
      StrCopy(pMemGlobal->resDBName,msg);
      openResDB();
    }
    else
      error1(ERR_U4_INVALID_RESDB, name);
  }
}

