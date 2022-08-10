/**********************************************************************/
/*                                                                    */
/* sess.h: LISPME session subsystem definitions                       */
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

#ifndef INC_SESS_H
#define INC_SESS_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* Exported data                                                      */
/**********************************************************************/
extern UInt16 startPanel;
extern int    lastSess;

/**********************************************************************/
/* Prototypes                                                         */
/**********************************************************************/
Boolean SessHandleEvent(EventType *e);
Boolean NewSessHandleEvent(EventType *e);

Boolean InitDB(char* startSess);
void    ShutdownDB(void);
Boolean ConnectSession(char* sessName, Boolean newDB);
void    DisConnectSession(void);
void    LockMem(void);
void    UnlockMem(void);

#endif
