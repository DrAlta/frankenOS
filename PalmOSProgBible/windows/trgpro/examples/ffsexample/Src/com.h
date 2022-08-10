/****************************************************************************
 * File        : com.h
 * Date        : 5-21-97
 * Description : Serial routines that use the Palm OS serial routines.
 ****************************************************************************/
#ifndef _COM_H_
#define _COM_H_

Err       COMOpen(void);
Err       COMClose(void);
Err       COMGetC(void *buffer);
Err       COMPutC(char c);
UInt32    COMBytesAvailable(void);
void      COMEnable(void);
void      COMDisable(void);

Err       COMPrintf(char *format, ...);

#endif
