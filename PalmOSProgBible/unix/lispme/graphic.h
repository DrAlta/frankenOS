/**********************************************************************/
/*                                                                    */
/* graphic.h: LISPME graphic functions                                */
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

#ifndef INC_GRAPHIC_H
#define INC_GRAPHIC_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* Exported data                                                      */
/**********************************************************************/
extern Boolean newGraphic;

/**********************************************************************/
/* prototypes                                                         */
/**********************************************************************/
void drawLine(Int16 x2, Int16 y2)                           SEC(VM);
void drawRect(Int16 x2, Int16 y2, Int16 corner)             SEC(VM);
void drawText(PTR obj)                                      SEC(VM);
void drawBitmap(PTR obj)                                    SEC(VM);
int  RGB2Index(PTR r, PTR g, PTR b)                         SEC(VM);
PTR  Index2RGB(PTR index)                                   SEC(VM);
void setPalette(PTR index, PTR r, PTR g, PTR b)             SEC(VM);

#endif
