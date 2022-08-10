/***********************************************************************
 *
 * PROJECT:  Librarian
 * FILE:     librarianTransfer.h
 * AUTHOR:   Lonnon R. Foster
 *
 * DESCRIPTION:  IR beaming header for Librarian.
 *
 * From Palm OS Programming Bible
 * ©2000 Lonnon R. Foster.  All rights reserved.
 *
 ***********************************************************************/

#define libFileExtension        "lib"
#define libCategoryExtension    "lbc"
#define libFileExtensionLength  3

#define libEntireStream        0xffffffff
#define libImportBufferSize    100
#define libMaxBeamDescription  50

/***********************************************************************
 *
 *   Function Prototypes
 *
 ***********************************************************************/

static Err     BeamData(ExgSocketType *exgSocket, void *buffer,
                        UInt32 bytes);
       Err     CustomBeamDialog(DmOpenRef db, ExgAskParamPtr askInfo);
       void    LibBeamCategory(DmOpenRef db, UInt16 category);
       void    LibBeamRecord(DmOpenRef db, Int16 recordNum);
       Err     LibImportRecord(DmOpenRef db, ExgSocketType *exgSocketP,
                               UInt32 bytes, UInt16 *indexP);
       Err     LibReceiveData(DmOpenRef db, ExgSocketType *exgSocketP);
       void    LibRegisterData(void);
