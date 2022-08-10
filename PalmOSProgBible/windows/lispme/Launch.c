/**********************************************************************/
/*                                                                    */
/* Launch.c: Start LispMe with a specific session                     */
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
/* 18.04.1999 New                                                FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 03.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/* There're so few resources, just use 1000 for all of them :-)       */
/**********************************************************************/
#define PROG_ID       1000
#define ERR_NO_LISPME 1000

/**********************************************************************/
/* includes                                                           */
/**********************************************************************/
#include <PalmOS.h>
#include <CoreTraps.h>
#include <SystemMgr.h>

/**********************************************************************/
/* Main entry                                                         */
/**********************************************************************/
UInt32 start()
{
  SysAppInfoPtr appInfo;
  MemPtr        prevGlob, glob;

  if (SysAppStartup(&appInfo, &prevGlob, &glob) != 0)
    return -1;
 
  if (appInfo->cmd == sysAppLaunchCmdNormalLaunch)
  {
    DmSearchStateType state;
    UInt16            card;
    LocalID           LispMeID;
    char*             sess   = MemPtrNew(dmDBNameLength);
    MemHandle         dbName = DmGet1Resource('tAIN',PROG_ID);

    StrCopy(sess,MemHandleLock(dbName));
    MemHandleUnlock(dbName);
    DmReleaseResource(dbName);

    if (DmGetNextDatabaseByTypeCreator(true, &state, sysFileTApplication,
                                       APPID, true, &card, &LispMeID))
      FrmAlert(ERR_NO_LISPME);
    else
    {
      MemPtrSetOwner(sess,0);
      SysUIAppSwitch(card, LispMeID, sysAppLaunchCmdCustomBase, sess);
    }
  }
  SysAppExit(appInfo, prevGlob, glob);
  return 0;
}
