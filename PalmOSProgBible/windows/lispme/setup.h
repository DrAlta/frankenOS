/**********************************************************************/
/*                                                                    */
/* setup.h: LISPME setup dialogs definitions                          */
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
/* 01.04.2000 New                                                FBI  */
/*                                                                    */
/**********************************************************************/

#ifndef INC_SETUP_H
#define INC_SETUP_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* Exported data                                                      */
/**********************************************************************/

/**********************************************************************/
/* Prototypes                                                         */
/**********************************************************************/
void    handleLefty(FormPtr form);
Boolean SetupGlobHandleEvent(EventType *e);
Boolean SetupSessHandleEvent(EventType *e);

#endif
