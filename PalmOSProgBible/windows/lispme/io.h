/**********************************************************************/
/*                                                                    */
/* io.h: LISPME input/output subsystem definitions                    */
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
/* 14.06.1997 New                                                FBI  */
/* 25.10.1999 Prepared for GPL release                           FBI  */
/* 01.04.2000 Prepared for GCC 2.0 and SDK 3.5                   FBI  */
/*                                                                    */
/**********************************************************************/

#ifndef INC_IO_H
#define INC_IO_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* Several buffer sizes                                               */
/**********************************************************************/
#define MEMO_OUTPUT_SIZE   4097
#define MAX_TOKEN_LEN       256

/**********************************************************************/
/* Printing flags                                                     */
/**********************************************************************/
#define PRT_OUTFIELD 0x00
#define PRT_MESSAGE  0x01
#define PRT_MEMO     0x02
#define PRT_DEST     0x03
#define PRT_ESCAPE   0x04
#define PRT_AUTOLF   0x08
#define PRT_SPACE    0x10

/**********************************************************************/
/* Exported data                                                      */
/**********************************************************************/
extern char    msg[];
extern Int32   outpSize;
extern char    errorChar;
extern Int16   lineNr;
extern Boolean onlyWS;
extern char*   startPtr;
extern char*   currPtr;

/**********************************************************************/
/* Prototypes                                                         */
/**********************************************************************/
void printSEXP(PTR p, UInt8 flags) SEC(IO);
PTR  readSEXP(char* src)           SEC(IO);
PTR  loadMemo(char* src)           SEC(IO);

#endif
