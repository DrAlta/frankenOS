/**********************************************************************/
/*                                                                    */
/* hbase.h: LISPME HandBase access functions                          */
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

#ifndef INC_HBASE_H
#define INC_HBASE_H

/**********************************************************************/
/* Includes                                                           */
/**********************************************************************/
#include "store.h"

/**********************************************************************/
/* Exported data                                                      */
/**********************************************************************/
extern LocalID handBaseID;

/**********************************************************************/
/* prototypes                                                         */
/**********************************************************************/
PTR  listHBDir()                                        SEC(IO);
PTR  getHBInfo(PTR name)                                SEC(IO);
PTR  getHBFieldVal(PTR name, PTR rec, PTR fld)          SEC(IO);  
void setHBFieldVal(PTR name, PTR rec, PTR fld, PTR val) SEC(IO);
PTR  getHBLinkList(PTR name, PTR rec, PTR fld)          SEC(IO); 
PTR  addHBRecord(PTR name)                              SEC(IO);

#endif
