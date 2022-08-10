/**********************************************************************/
/*                                                                    */
/* ParHack.c: Hackmaster hack for easier input of Lisp expressions    */
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
/* 07.06.1998 New                                                FBI  */
/* 01.07.1998 Remove selection on entering closing parenthese    FBI  */
/* 05.08.1998 Send changed notification on 'select other'        FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* includes                                                           */
/**********************************************************************/
#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <CoreTraps.h>
#include <Field.h>
#include "ParHack.h"

/**********************************************************************/
/* local defines                                                      */
/**********************************************************************/
#define RESID   1000
#define APPIDH  'fbPH'

#define asSTART 0
#define asSTRG  1
#define asSTRE  2
#define asCOMM  3
#define asUNKN  4

typedef enum
{
  blinkOther, selectOther, selectRange
} Action;

/*--------------------------------------------------------------------*/
/* Scanner automaton state: What do we know about the role of a par.? */
/*--------------------------------------------------------------------*/
typedef enum
{
  unknown, comment, string, yes
} ParState;

static void searchBackward(FieldPtr field, Short orgPos, Action action);
static void searchForward(FieldPtr field, Short orgPos, Action action);

/**********************************************************************/
/* Main entry                                                         */
/**********************************************************************/
Boolean MyFldHandleEvent(FieldPtr field, EventPtr event)
{
  Boolean (*oldTrap)(FieldPtr,EventPtr);
  DWord temp;
  Boolean handled;
  Word start, end;

  FtrGet(APPIDH,RESID,&temp);
  oldTrap = (Boolean(*)(FieldPtr,EventPtr))temp;
  handled = false;

  if (event->eType == keyDownEvent)
  {
    switch (event->data.keyDown.chr)
    {
      case 'o':
      case 'O':
      case '0':
        /*------------------------------------------------------------*/
        /* If exactly one parenthese is selected, select matching     */
        /*------------------------------------------------------------*/
        FldGetSelection(field,&start,&end);
        if (start+1 == end)
        {
          switch(field->text[start])
          {
            case '(':
              searchForward(field,end,selectOther);
              handled = true;
              break;
            case ')':
              searchBackward(field,start,selectOther);
              handled = true;
              break;
          }
        }
        break;

      case 's':
      case 'S':
      case '5':
        /*------------------------------------------------------------*/
        /* If exactly one parenthese is selected, select range to     */
        /* matching one                                               */
        /*------------------------------------------------------------*/
        FldGetSelection(field,&start,&end);
        if (start+1 == end)
        {
          switch (field->text[start])
          {
            case '(':
              searchForward(field,end,selectRange);
              handled = true;
              break;
            case ')':
              searchBackward(field,start,selectRange);
              handled = true;
              break;
          }
        }
        break;

      case ')':
        /*------------------------------------------------------------*/
        /* Search matching parenthese to the left                     */
        /*------------------------------------------------------------*/
        FldGetSelection(field,&start,&end);
        if (start==end)
          searchBackward(field,FldGetInsPtPosition(field),blinkOther);
        else
        {
          searchBackward(field,start,blinkOther);
          FldSetSelection(field,start,end);
        }
        break;
    }
  }

  if (!handled)
    handled = oldTrap(field, event);
  return handled;
}

/**********************************************************************/
/* Test, if field[pos] is a real parenthesis, or if it's in a comment */
/* or character or string constant                                    */
/* Heuristic: String constants never span more than one line          */
/**********************************************************************/
static Boolean isRealPar(FieldPtr field, Short pos, int* ps)
{
  Short tpos  = pos;

  if (*ps==asUNKN)
  {
    /*----------------------------------------------------------------*/
    /* Search beginning of line                                       */
    /*----------------------------------------------------------------*/
    for (tpos = pos; tpos >= 0 && field->text[tpos] != '\n'; --tpos)
      ;
    ++tpos;

    /*----------------------------------------------------------------*/
    /* Now parse to the right, handling strings and comments only     */
    /*----------------------------------------------------------------*/
    *ps = asSTART;
    while (tpos < pos)
    {
      switch(*ps)
      {
        case asSTART:
          switch (field->text[tpos])
          {
            case '"': *ps = asSTRG; break;
            case ';': *ps = asCOMM; return false;
          }
          break;

        case asSTRG:
          switch (field->text[tpos])
          {
            case '"':  *ps = asSTART; break;
            case '\\': *ps = asSTRE; break;
          }
          break;

        case asSTRE: *ps = asSTRG; break;
      }
      ++tpos;
    }
  }
  /*----------------------------------------------------------------*/
  /* Check for char constant                                        */
  /*----------------------------------------------------------------*/
  if (pos>1 && field->text[pos-1] == '\\' &&
      field->text[pos-2] == '#')
    return false;

  return *ps == asSTART;
}

/**********************************************************************/
/* Search backwards for matching parenthese                           */
/**********************************************************************/
static void searchBackward(FieldPtr field, Short orgPos, Action action)
{
  Short pos, nest;
  int ps = asUNKN;

  if (!isRealPar(field,orgPos,&ps))
  {
    if (action != blinkOther)
      SndPlaySystemSound(sndWarning);
    return;
  }
  pos = orgPos;
  nest = 1;
  while (pos >= 0 && nest)
  {
    switch (field->text[--pos])
    {
      case ')':
        if (isRealPar(field, pos, &ps))
          ++nest;
        break;

      case '(':
        if (isRealPar(field, pos, &ps))
          --nest;
        break;

      case ';':
      case '"':
      case '\n':
        ps = asUNKN;
        break;
    }
  }
  if (pos >= 0)
  {
    switch (action)
    {
      case blinkOther:
        FldSetInsPtPosition(field,pos);
        FldSetSelection(field,pos,pos+1);
        FldDrawField(field);
        SysTaskDelay(30);
        FldSetSelection(field,orgPos,orgPos);
        FldSetInsPtPosition(field,orgPos);
        break;

      case selectOther:
        FldSetInsPtPosition(field,pos);
        FldSetSelection(field,pos,pos+1);
        FldSendChangeNotification(field);
        break;

      case selectRange:
        FldSetSelection(field,pos,orgPos+1);
        break;
    }
    FldDrawField(field);
  }
  else
  {
    /*----------------------------------------------------------------*/
    /* Too many closing parentheses!                                  */
    /*----------------------------------------------------------------*/
    SndPlaySystemSound(sndWarning);
  }
}

/**********************************************************************/
/* Search forwards for matching parenthese                            */
/**********************************************************************/
static void searchForward(FieldPtr field, Short orgPos, Action action)
{
  Short pos, nest;
  int ps = asUNKN;

  if (!isRealPar(field,orgPos,&ps))
  {
    SndPlaySystemSound(sndWarning);
    return;
  }

  pos = orgPos;
  nest = 1;
  while (pos <= field->textLen && nest)
  {
    switch (field->text[pos])
    {
      case ')':
        if (isRealPar(field, pos, &ps))
          --nest;
        break;

      case '(':
        if (isRealPar(field, pos, &ps))
          ++nest;
        break;
 
      case ';':
        ps = asCOMM;
        break;

      case '\n':
        ps = asSTART;
        break;
 
      case '"':
        if (ps!=asCOMM)
          ps = asUNKN;
        break;
    }
    ++pos;
  }
  if (pos <= field->textLen)
  {
    switch (action)
    {
      case selectOther:
        FldSetInsPtPosition(field,pos);
        FldSetSelection(field,pos-1,pos);
        FldSendChangeNotification(field);
        break;

      case selectRange:
        FldSetSelection(field,orgPos-1,pos);
        break;
    }
    FldDrawField(field);
  }
  else
  {
    /*----------------------------------------------------------------*/
    /* Too many opening parentheses!                                  */
    /*----------------------------------------------------------------*/
    SndPlaySystemSound(sndWarning);
  }
}
