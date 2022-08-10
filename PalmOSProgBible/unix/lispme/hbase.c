/**********************************************************************/
/*                                                                    */
/* hbase.c: LISPME HandBase access functions                          */
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
/* 04.04.2000 New                                                FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "hbase.h"
#include "hapi.h"
#include "vm.h"
#include "util.h"
#include "io.h"
#include "LispMe.h"

/**********************************************************************/
/* Global data                                                        */
/**********************************************************************/
LocalID handBaseID;

/**********************************************************************/
/* Local macros                                                       */
/**********************************************************************/
#define HB_CALL(cmd,buf) \
  {UInt32 rc1; int rc=SysAppLaunch(0,handBaseID,0,cmd,(char*)buf,&rc1);\
  if(!rc) rc = rc1; if (rc) error1(ERR_H2_ERROR,MKINT(rc));}

/**********************************************************************/
/* List HanDBase directory                                            */
/**********************************************************************/
PTR listHBDir()
{
  HapiDBListType* buf;
  PTR             res = NIL;
  PTR*            d = &res;
  int             i;

  if (!handBaseID)
    ErrThrow(ERR_H1_NO_HANDBASE);

  buf = MemPtrNew(sizeof(HapiDBListType));
  ErrTry {
    HB_CALL(HapiGetDBNames, buf)
    PROTECT(res);
    for (i=0; i<buf->numdbs; ++i)
    {
      *d = cons(str2Lisp(buf->dbnames[i]), NIL);
      d  = &cdr(*d);
    }
    UNPROTECT(res);
  } ErrCatch(err) {
    MemPtrFree(buf);
    ErrThrow(err);
  } ErrEndCatch
  MemPtrFree(buf);
  return res;
}

/**********************************************************************/
/* Get info for HanDBase DB                                           */
/**********************************************************************/
PTR getHBInfo(PTR name)
{
  HapiFieldsDefType* buf;
  PTR                res;
  PTR*               d;
  int                i;
  PTR                temp;

  if (!handBaseID)
    ErrThrow(ERR_H1_NO_HANDBASE);
  checkString(name);

  buf = MemPtrNew(sizeof(HapiDBListType));
  ErrTry {
    printSEXP(name, PRT_MEMO);
    StrCopy(buf->dbname, msg);
    HB_CALL(HapiGetDBInfo, buf)
    res = cons(MKINT(buf->numrecs), NIL);
    d = &cdr(res);
    PROTECT(res);
    for (i=0; i<HAPI_MAX_FIELDS; ++i) 
    {
      HapiFieldDefType* c = &buf->fields[i];

      if (c->fieldtype == HB_FIELD_NOT_USED)
        continue;    

      temp = cons(MKINT(i),
               cons(str2Lisp(c->fieldname),
                 cons(MKINT(c->fieldtype),
                   cons(MKINT(c->maxsize),
                     cons(c->export ? TRUE : FALSE,
                       cons(c->visible ? TRUE : FALSE, NIL))))));
      *d = cons(temp, NIL);
      d  = &cdr(*d);
    }
    UNPROTECT(res);
  } ErrCatch(err) {
    MemPtrFree(buf);
    ErrThrow(err);
  } ErrEndCatch
  MemPtrFree(buf);
  return res;
}

/**********************************************************************/
/* Get field value as text                                            */
/**********************************************************************/
PTR getHBFieldVal(PTR name, PTR rec, PTR fld)
{
  HapiFieldValueType value;

  if (!handBaseID)
    ErrThrow(ERR_H1_NO_HANDBASE);
  checkString(name);
  checkInt(rec);
  checkInt(fld);

  /*------------------------------------------------------------------*/
  /* Use msg (printing buffer) as databuffer for handbase             */
  /*------------------------------------------------------------------*/
  printSEXP(name, PRT_MEMO);
  StrCopy(value.dbname, msg);
  value.recnum         = INTVAL(rec);
  value.fieldnum       = INTVAL(fld);
  value.maxsizeofvalue = MEMO_OUTPUT_SIZE;
  value.outvalue       = msg;
  HB_CALL(HapiGetFieldValue, &value)
  return str2Lisp(msg);
}

/**********************************************************************/
/* Set field value as text                                            */
/**********************************************************************/
void setHBFieldVal(PTR name, PTR rec, PTR fld, PTR val)
{
  HapiFieldValueType value;

  if (!handBaseID)
    ErrThrow(ERR_H1_NO_HANDBASE);
  checkString(name);
  checkInt(rec);
  checkInt(fld);

  /*------------------------------------------------------------------*/
  /* Use msg (printing buffer) as databuffer for handbase             */
  /*------------------------------------------------------------------*/
  printSEXP(name, PRT_MEMO);
  StrCopy(value.dbname, msg);
  printSEXP(val, PRT_MEMO);
  value.recnum         = INTVAL(rec);
  value.fieldnum       = INTVAL(fld);
  value.maxsizeofvalue = MEMO_OUTPUT_SIZE;
  value.outvalue       = msg;
  HB_CALL(HapiSetFieldValue, &value)
}

/**********************************************************************/
/* Add a new (empty) record                                           */
/**********************************************************************/
PTR addHBRecord(PTR name)
{
  HapiRecordValueType rec;
  int i;

  if (!handBaseID)
    ErrThrow(ERR_H1_NO_HANDBASE);
  checkString(name);

  for (i=0; i<HAPI_MAX_FIELDS; ++i)
    rec.fieldvalues[i] = "";

  printSEXP(name, PRT_MEMO);
  StrCopy(rec.dbname, msg);
  HB_CALL(HapiAddRecord, &rec)
  return MKINT(rec.recnum);
}

/**********************************************************************/
/* Get a list of fields this field is linked to                       */
/**********************************************************************/
PTR getHBLinkList(PTR name, PTR rec, PTR fld)
{
  HapiLinkInfoType         info;
  HapiLinkedRecordInfoType lrec;
  PTR  res;
  PTR* d;

  if (!handBaseID)
    ErrThrow(ERR_H1_NO_HANDBASE);
  checkString(name);
  checkInt(rec);
  checkInt(fld);
  
  printSEXP(name, PRT_MEMO);
  StrCopy(info.dbname, msg);
  info.recnum         = INTVAL(rec);
  info.fieldnum       = INTVAL(fld);
  HB_CALL(HapiGetLinkInfo, &info);

  StrCopy(lrec.dbname, info.linkeddatabasename);
  lrec.fieldnum       = info.linkedfieldnum;
  lrec.maxsizeofvalue = MEMO_OUTPUT_SIZE;
  lrec.outvalue       = msg;
  lrec.newsearch      = true;
  MemMove(lrec.linkvalue, info.outvalue, sizeof(lrec.linkvalue));

  res = cons(str2Lisp(lrec.dbname), NIL);
  d   = &cdr(res);
  PROTECT(res);
  
  while (true) {
    UInt32 rc1;
    int rc=SysAppLaunch(0,handBaseID,0,HapiFindNextLinkedRecord,
                        (char*)&lrec,&rc1);
    if (!rc) rc = rc1;

    switch (rc) {
      case 0:
        lrec.newsearch = false;
        *d = cons(MKINT(lrec.recnum), NIL);
        d  = &cdr(*d);
        break; 

      case 7: // no more matches
        UNPROTECT(res);
        return res;
 
      default: error1(ERR_H2_ERROR,MKINT(rc));
    }
  }
}

