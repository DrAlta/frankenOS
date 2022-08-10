/**********************************************************************/
/*                                                                    */
/* gui.h:   LISPME user interface functions                           */
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

#ifndef INC_GUI_H
#define INC_GUI_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* Structures                                                         */
/**********************************************************************/
struct UIContext {
  PTR prevCont; // continuation in previous event handler for frm-return
  PTR handler;  // current event handler procedure
};

#define MAX_GUI_NEST 6

/**********************************************************************/
/* Exported variables                                                 */
/**********************************************************************/
extern struct UIContext contexts[];
extern int actContext;

/**********************************************************************/
/* prototypes                                                         */
/**********************************************************************/
void popupForm(int resId, PTR handler);
void gotoForm(int resId, PTR handler);
PTR  GUIfldGetText(PTR id);
void GUIfldSetText(PTR id, PTR obj);
PTR  GUIctlGetVal(PTR id);
void GUIctlSetVal(PTR id, PTR obj);
PTR  GUIlstGetSel(PTR id);
void GUIlstSetSel(PTR id, PTR obj);
PTR  GUIlstGetText(PTR id, PTR obj);
void GUIlstSetItems(PTR id, PTR items);
void GUIFreeList(void);
void GUIfrmSetFocus(PTR id);
PTR  GUIfrmGetFocus(void);
void GUIfrmShow(PTR id, PTR show);

#endif
