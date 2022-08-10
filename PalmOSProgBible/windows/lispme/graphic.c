/**********************************************************************/
/*                                                                    */
/* graphic.c: LISPME graphic functions                                */
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
/* 08.04.2000 New                                                FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "graphic.h"
#include "util.h"
#include "io.h"
#include "LispMe.h"

/**********************************************************************/
/* Global data                                                        */
/**********************************************************************/
Boolean newGraphic;

/**********************************************************************/
/* Local functions                                                    */
/**********************************************************************/
static void checkGState(void)                                SEC(VM);
static void setPattern(void)                                 SEC(VM);
static void setColors(void)                                  SEC(VM);

/**********************************************************************/
/* Local macros                                                       */
/**********************************************************************/
#define GS_COORD   caar(pMemGlobal->graphState)
#define GS_FONT    cadar(pMemGlobal->graphState)
#define GS_PATTERN car(cddar(pMemGlobal->graphState))
#define GS_MODE    cadr(cddar(pMemGlobal->graphState))
#define GS_FORE    caddr(cddar(pMemGlobal->graphState))
#define GS_BACK    car(cdddr(cddar(pMemGlobal->graphState)))
#define GS_TEXT    cadr(cdddr(cddar(pMemGlobal->graphState)))

/**********************************************************************/
/* Set fill pattern                                                   */
/**********************************************************************/
static void setPattern(void)
{
  PTR pat = GS_PATTERN;
  if (IS_STRING(pat))
  {
    if (stringLength(pat) != 8)
      goto patError;
    printSEXP(pat, PRT_MESSAGE);
    WinSetPattern((UInt8*)msg);
  }
  else 
    patError:
      typeError(pat, "valid pattern");
}

/**********************************************************************/
/* Check gstate consistency                                           */
/**********************************************************************/
static void checkGState(void)
{
  PTR p = car(pMemGlobal->graphState);

  if (!IS_PAIR(p) || !IS_PAIR(car(p)) || !IS_INT(caar(p)) || !IS_INT(cdar(p)) ||
      (p=cdr(p), false) || !IS_INT(car(p)) ||
      (p=cdr(p), false) || // check pattern later
      (p=cdr(p), false) || !IS_INT(car(p)) ||    
      (p=cdr(p), false) || !IS_INT(car(p)) ||    
      (p=cdr(p), false) || !IS_INT(car(p)) ||    
      (p=cdr(p), false) || !IS_INT(car(p)))
    typeError(car(pMemGlobal->graphState), "valid graphic state");
}

/**********************************************************************/
/* Set colors and mode                                                */
/**********************************************************************/
static void setColors(void)
{
  WinPushDrawState();
  WinSetForeColor(INTVAL(GS_FORE));
  WinSetBackColor(INTVAL(GS_BACK));
  WinSetTextColor(INTVAL(GS_TEXT));
  WinSetDrawMode(INTVAL(GS_MODE));
  WinSetPatternType(blackPattern);
  switch (GS_PATTERN)
  {
    case FALSE:   // deprecated, only for backward compatibility
      WinSetForeColor(INTVAL(GS_BACK));
      WinSetBackColor(INTVAL(GS_FORE));
      WinSetTextColor(INTVAL(GS_BACK));
      break;

    case NOPRINT: // deprecated, only for backward compatibility
      WinSetDrawMode(winInvert);
      break;

    case TRUE:
      break;
  
    default:
      WinSetPatternType(customPattern); 
      setPattern();
      break;
  }
}

/**********************************************************************/
/* Draw a line                                                        */
/**********************************************************************/
void drawLine(Int16 x2, Int16 y2)
{
  Int16 x1, y1;

  checkGState();
  x1 = INTVAL(car(GS_COORD));
  y1 = INTVAL(cdr(GS_COORD));

  if (newGraphic) 
  {
    setColors();
    WinPaintLine(x1,y1,x2,y2);
    WinPopDrawState();
  }
  else
  {
    switch (GS_PATTERN)
    {
      case TRUE:
        WinDrawLine(x1,y1,x2,y2);
        break;

      case FALSE:
        WinEraseLine(x1,y1,x2,y2);
        break;

      case NOPRINT:
        WinInvertLine(x1,y1,x2,y2);
        break;

      default:
        setPattern();
        WinFillLine(x1,y1,x2,y2);
        break;
    }
  }
  car(GS_COORD) = MKINT(x2);
  cdr(GS_COORD) = MKINT(y2);
}

/**********************************************************************/
/* Draw a text                                                        */
/**********************************************************************/
void drawText(PTR obj)
{
  Int16 x,y;
  int   font;
  int   len;
  FontID oldFont;

  checkGState();
  x    = INTVAL(car(GS_COORD));
  y    = INTVAL(cdr(GS_COORD));
  font = INTVAL(GS_FONT);

  if (newGraphic) 
    setColors();

  printSEXP(obj, PRT_MESSAGE);
  oldFont = FntGetFont();
  FntSetFont(stdFont <= font && font <= (palmIII ? 7 : ledFont)
               ? font : stdFont);
  len = StrLen(msg);  

  if (newGraphic) 
  {
    WinPaintChars(msg,len,x,y);
    WinPopDrawState();
  }
  else 
  {
    switch (GS_PATTERN)
    {
      case FALSE:
        WinDrawInvertedChars(msg,len,x,y);
        break;

      case NOPRINT:
        WinInvertChars(msg,len,x,y);
        break;

      default:
        WinDrawChars(msg,len,x,y);
        break;
    }
  }
  FntSetFont(oldFont);
}

/**********************************************************************/
/* Draw a bitmap                                                      */
/**********************************************************************/
void drawBitmap(PTR obj)
{
  Int16 x,y;

  checkGState();
  x = INTVAL(car(GS_COORD));
  y = INTVAL(cdr(GS_COORD));

  if (!IS_STRING(obj))
    typeError(obj, "bitmap");
  printSEXP(obj, PRT_MEMO);

  /*------------------------------------------------------------------*/
  /* Some sanity checks to avoid a fatal exception - more necessary?  */
  /*------------------------------------------------------------------*/
  if (((BitmapPtr)msg)->rowBytes & 1)
    typeError(obj, "bitmap");

  if (newGraphic)
  {
    setColors();
    WinPaintBitmap((BitmapPtr)msg, x, y);
    WinPopDrawState();
  }
  else
    WinDrawBitmap((BitmapPtr)msg, x, y);
}

/**********************************************************************/
/* Draw a filled rectangle                                            */
/**********************************************************************/
void drawRect(Int16 x2, Int16 y2, Int16 corner)
{
  RectangleType rect;
  Int16 x1, y1;

  checkGState();
  x1 = INTVAL(car(GS_COORD));
  y1 = INTVAL(cdr(GS_COORD));

  rect.topLeft.x = min(x1,x2);
  rect.topLeft.y = min(y1,y2);
  rect.extent.x  = abs(x1-x2);
  rect.extent.y  = abs(y1-y2);

  if (newGraphic) 
  {
    setColors();
    WinPaintRectangle(&rect, corner);
    WinPopDrawState();
  }
  else
  {
    switch (GS_PATTERN)
    {
      case TRUE:
        WinDrawRectangle(&rect, corner);
        break;

      case FALSE:
        WinEraseRectangle(&rect, corner);
        break;

      case NOPRINT:
        WinInvertRectangle(&rect, corner);
        break;

      default:
        setPattern();
        WinFillRectangle(&rect, corner);
        break;
    }
  }

  car(GS_COORD) = MKINT(x2);
  cdr(GS_COORD) = MKINT(y2);
}

/**********************************************************************/
/* Find nearest color index for RGB values                            */
/**********************************************************************/
int RGB2Index(PTR r, PTR g, PTR b)
{
  RGBColorType col;

  checkInt(r); checkInt(g); checkInt(b);
  if (newGraphic)
  {
    col.r = INTVAL(r); col.g = INTVAL(g); col.b = INTVAL(b);
    return WinRGBToIndex(&col); 
  }
  else
    return 0;
}

/**********************************************************************/
/* Find nearest color index for RGB values                            */
/**********************************************************************/
PTR Index2RGB(PTR index)
{
  RGBColorType col;

  checkInt(index);
  col.r = col.g = col.b = 0;
  if (newGraphic)
    WinIndexToRGB(INTVAL(index), &col); 
  return cons(MKINT(col.r),
           cons(MKINT(col.g),
             cons(MKINT(col.b), NIL)));
}

/**********************************************************************/
/* Set palette entry                                                  */
/**********************************************************************/
void setPalette(PTR index, PTR r, PTR g, PTR b)
{
  RGBColorType col;

  checkInt(index); checkInt(r); checkInt(g); checkInt(b);
  if (newGraphic)
  {
    col.r = INTVAL(r); col.g = INTVAL(g); col.b = INTVAL(b);
    WinPalette(winPaletteSet, 0xff & INTVAL(index), 1, &col); 
  }
}
