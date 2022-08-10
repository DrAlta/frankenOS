/**********************************************************************/
/*                                                                    */
/* file.h: LISPME "file" (Memo) management                            */
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

#ifndef INC_FILE_H
#define INC_FILE_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* Defines                                                            */
/**********************************************************************/
#define MAX_FILENAME_LEN 16

/**********************************************************************/
/* prototypes                                                         */
/**********************************************************************/
PTR  memoDir(PTR cat)                               SEC(IO);
PTR  createFile(PTR fileName)                       SEC(IO);
PTR  openFile(PTR fileName, Boolean append)         SEC(IO);
void writeFile(PTR expr, PTR port, Boolean machine) SEC(IO);
PTR  readFile(PTR port, int opcode)                 SEC(IO);
void deleteFile(PTR fileName)                       SEC(IO);
PTR  readDB(Boolean isRes, PTR type, int recNum)    SEC(IO);
PTR  writeDB(PTR dbName, UInt16 recNum, PTR data)   SEC(IO);
void setResDB(PTR name)                             SEC(IO);
void closeResDB(void)                               SEC(IO);
void openResDB(void)                                SEC(IO);

void UnlockMem(void);
void LockMem(void);


#endif
