/****************************************************************************
 *
 *      Copyright (c) 1999, TRG, All Rights Reserved
 *
 *---------------------------------------------------------------------------
 * FileName:
 *              FfsLib.h
 *
 * Description:
 *              Ffs library API definitions.
 *
 * Version:     1.00 build 2
 ****************************************************************************/


#ifndef __FFS_LIB_H__
#define __FFS_LIB_H__


/*---------------------------------------------------------------------------
 * If we're actually compiling the library code, then we need to
 * eliminate the trap glue that would otherwise be generated from
 * this header file in order to prevent compiler errors in CW Pro 2.
 *--------------------------------------------------------------------------*/
#ifdef BUILDING_FFS_LIB
        #define FFS_LIB_TRAP(trapNum)
#else
        #define FFS_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif


/****************************************************************************
 * Type and creator of Sample Library database -- must match project defs!
 ****************************************************************************/
#define FfsLibCreatorID  'FfsL'       // Ffs Library database creator
#define FfsLibTypeID     'libr'       // Standard library database type


/***************************************************************************
 * Internal library name which can be passed to SysLibFind()
 ***************************************************************************/
#define FfsLibName       "Ffs.lib"     


/***************************************************************************
 * Defines for Ffs library calls
 ***************************************************************************/

/*--------------------------------------------------------------------------
 * Ffs Library result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *
 * These are for errors specific to loading/opening/closing the library
 *-------------------------------------------------------------------------*/
#define FfsErrorClass              (appErrorClass | 0x300)

#define FFS_ERR_BAD_PARAM          (FfsErrorClass | 1)    // invalid parameter
#define FFS_ERR_LIB_NOT_OPEN       (FfsErrorClass | 2)    // library is not open
#define FFS_ERR_LIB_IN_USE         (FfsErrorClass | 3)    // library still in used
#define FFS_ERR_NO_MEMORY          (FfsErrorClass | 4)    // memory error occurred
#define FFS_ERR_NOT_SUPPORTED      (FfsErrorClass | 5)    // call not supported in this version
#define FFS_ERR_CARD_IN_USE        (FfsErrorClass | 6)    // card in use by another app

/*--------------------------------------------------------------------------
 * Ffs Library call errno codes
 *
 * These are error codes returned by FfsGetErrno() -- they are descriptive
 * error codes set when a call fails. They are stored in a global, and
 * FfsGetErrno() returns the current value (ie. the last error to occur).
 *-------------------------------------------------------------------------*/
#define ENOENT    2   /* File not found or path to file not found                  */
#define ENOMEM    8   /* not enough memory                                         */ 
#define EBADF     9   /* Invalid file descriptor                                   */
#define EACCES    13  /* Attempt to open a read only file or a special (directory) */
#define EINVDRV   15  /* Invalid drive specified                                   */
#define EEXIST    17  /* Exclusive access requested but file already exists.       */
#define EINVAL    22  /* Invalid argument                                          */
#define ENFILE    24  /* No file descriptors available (too many files open)       */
#define ENOSPC    28  /* Write failed. Presumably because of no space              */
#define ESHARE    30  /* Open failed do to sharing                                 */
#define ENODEV    31  /* No valid device found                                     */
#define ERANGE    34  /* Result too large                                          */       
#define EIOERR	  35  /* I/O error                                                 */

/* low level errors during initialization */
#define BUS_ERC_DIAG       101 /* Drive diagnostic failed                   */
#define BUS_ERC_ARGS       102 /* Bad argument during initialization        */
#define BUS_ERC_DRQ        103 /* Drive DRQ is not valid.                   */
#define BUS_ERC_TIMEOUT    104 /* Timeout during an operation               */
#define BUS_ERC_STATUS     105 /* Controller reported an error              */
#define BUS_ERC_ADDR_RANGE 106 /* LBA out of range                          */
#define BUS_ERC_CNTRL_INIT 107 /* Fail to initialize controller             */
#define BUS_ERC_IDDRV      108 /* Identify drive info error                 */
#define BUS_ERC_CMD_MULT   109 /* Read/Write Multiple Command error         */
#define BUS_ERC_BASE_ADDR  110 /* Base Address not valid                    */
#define BUS_ERC_CARD_ATA   111 /* Card is not ATA                           */


/*--------------------------------------------------------------------------
 * MS-DOS file attributes
 *
 * These are the file attributes used by MS-DOS to mark file types in the
 * directory entry. They are set by FfsSetfileattr() and retrieved by
 * FfsGetfileattr(), FfsStat(), and FfsFstat(). They are also used for
 * filtering by FfsFindfirst() and FfsFindnext().
 *
 * Note that Ffsfindfirst() and FfsFindnext() only return items which match
 * the specified attribute exactly (although FA_ARCH is ignored). For this
 * reason, a wildcard attribute FA_ALL is supplied (non-standard) which
 * matches all directory entries. 
 *--------------------------------------------------------------------------*/
#define FA_NORMAL     0x00      /* "normal" file                            */
#define FA_RDONLY     0x01      /* read only                                */
#define FA_HIDDEN     0x02      /* hidden file                              */
#define FA_SYSTEM     0x04      /* system file                              */
#define FA_LABEL      0x08      /* disk volume label                        */
#define FA_DIREC      0x10      /* subdirectory                             */
#define FA_ARCH       0x20      /* archive                                  */
#define FA_ALL        0x8000    /* matches anything for FfsFindfirst()      */

/*---------------------------------------------------------------------------
 * Lseek codes
 *
 * Determine the starting point of an Lseek command.
 *--------------------------------------------------------------------------*/
#define SEEK_SET   0  /* offset from begining of file        */
#define SEEK_CUR   1  /* offset from current file pointer    */
#define SEEK_END   2  /* offset from end of file             */

/*--------------------------------------------------------------------------
 * File mode bits
 *
 * Used by FfsOpen() and FfsCreat() to set the file read/write mode when
 * creating a new file.
 * NOTE: these are in octal
 *--------------------------------------------------------------------------*/
#define S_IREAD    0000200    /* Read permitted. (Always true anyway)   */
#define S_IWRITE   0000400    /* Write permitted                        */

/*--------------------------------------------------------------------------
 * Fstat, stat file type mode bits
 *
 * Current file type mode as returned by FfsFstat() and FfsStat(). One of
 * these mode bits will be OR'd with the read/write permission of the
 * file (S_IREAD, S_IWRITE).
 * NOTE: these are in octal
 *--------------------------------------------------------------------------*/
#define S_IFCHR  0020000 /* character special (unused)                  */
#define S_IFDIR  0040000 /* subdirectory                                */
#define S_IFBLK  0060000 /* block special  (unused)                     */
#define S_IFREG  0100000 /* regular file                                */
#define S_IFMT   0170000 /* type of file mask                           */

/*--------------------------------------------------------------------------
 * File access flags
 *
 * Used by FfsOpen() to set the file access permissions when opening a file.
 *--------------------------------------------------------------------------*/
#define O_RDONLY       0x0000   /* Open for read only                       */
#define O_WRONLY       0x0001   /* Open for write only                      */
#define O_RDWR         0x0002   /* Read/write access allowed.               */
#define O_APPEND       0x0008   /* Seek to eof on each write                */
#define O_CREAT        0x0100   /* Create the file if it does not exist.    */
#define O_TRUNC        0x0200   /* Truncate the file if it already exists   */
#define O_EXCL         0x0400   /* Fail if creating and already exists      */
#define O_TEXT         0x4000   /* Ignored                                  */
#define O_BINARY       0x8000   /* Ignored. All file access is binary       */
#define O_NOSHAREANY   0x0004   /* Wants this open to fail if already open. */
                                /* Other opens will fail while this open    */
                                /* is active                                */
#define O_NOSHAREWRITE 0x0800   /* Wants this opens to fail if already open */
                                /* for write. Other open for write calls    */
                                /* will fail while this open is active.     */

/*---------------------------------------------------------------------------
 * Critical error defines
 *
 * Critical error NOTIFY and RESPONSE types -- NOTIFY is sent to the caller's
 * critical error handler, which should respond with an appropriate response.
 *--------------------------------------------------------------------------*/
#define CRERR_NOTIFY_ABORT_FORMAT       1  /* Abort, Format                */
#define CRERR_NOTIFY_CLEAR_ABORT_RETRY  2  /* Clear+retry, Abort, Retry    */
#define CRERR_NOTIFY_ABORT_RETRY        3  /* Abort, Retry                 */

#define CRERR_RESP_ABORT                1  /* Abort current operation      */
#define CRERR_RESP_RETRY                2  /* Retry current operation      */
#define CRERR_RESP_FORMAT               3  /* Format the card              */
#define CRERR_RESP_CLEAR                4  /* Clear bad sector and retry   */


/***************************************************************************
 * Special types for FFS access
 ***************************************************************************/

/*--------------------------------------------------------------------------
 * diskfree_t structure for FfsGetdiskfree()
 *--------------------------------------------------------------------------*/
typedef struct {
    UInt32  avail_clusters;       /* number of free clusters              */
    UInt32  total_clusters;       /* total number of clusters on drive    */
    UInt16  bytes_per_sector;     /* number bytes per sector              */
    UInt16  sectors_per_cluster;  /* number sectors per cluster           */
} diskfree_t;

/*---------------------------------------------------------------------------
 * ffblk structure for FfsFindfirst(), FfsFindnext()
 *--------------------------------------------------------------------------*/
typedef struct {
    char    ff_reserved[21];   /* used by system -- don't modify!         */
    char    ff_attrib;         /* DOS file attributes                     */
    Int16   ff_ftime;          /* creation time                           */
    Int16   ff_fdate;          /* creation date                           */
    Int32   ff_fsize;          /* file size                               */
    char    ff_name[13];       /* name in 8.3 format                      */
    char    ff_longname[256];  /* long file name                          */
} ffblk;

/*---------------------------------------------------------------------------
 * stat structure used by FfsStat(), FfsFstat(). Structure date_t is used
 * in stat_t (it is defined as a long in some versions).
 *--------------------------------------------------------------------------*/
typedef struct {
    UInt16  date;        
    UInt16  time;
} date_t;

typedef struct {
    Int16   st_dev;       /* drive (always 1)                           */
    Int16   st_ino;       /* not used                                   */
    UInt32  st_mode;      /* file mode information                      */
    Int16   st_nlink;     /* always 1                                   */
    Int16   st_uid;       /* not used                                   */
    Int16   st_gid;       /* not used                                   */
    Int16   st_rdev;      /* same as st_dev                             */
    UInt32  st_size;      /* file size                                  */
    date_t  st_atime;     /* creation date/time                         */
    date_t  st_mtime;     /* same as st_atime                           */
    date_t  st_ctime;     /* same as st_atime                           */
    UInt8   st_attr;      /* file attributes (non-standard)             */
} stat;

/***************************************************************************
 * Ffs library function trap ID's. Each library call gets a trap number:
 *   FfsLibTrapXXXX which serves as an index into the library's dispatch
 *   table. The constant sysLibTrapCustom is the first available trap number
 *   after the system predefined library traps Open,Close,Sleep & Wake.
 *
 * WARNING!!! The order of these traps MUST match the order of the dispatch
 *  table in FfsLibDispatch.c!!!
 ****************************************************************************/
typedef enum {
    FfsLibTrapGetLibAPIVersion = sysLibTrapCustom,
    FfsLibTrapGetdiskfree,
    FfsLibTrapFindfirst,
    FfsLibTrapFindnext,
    FfsLibTrapFinddone,
    FfsLibTrapFileOpen,
    FfsLibTrapFileClose,
    FfsLibTrapRead,
    FfsLibTrapWrite,
    FfsLibTrapRemove,
    FfsLibTrapChdir,
    FfsLibTrapGetcwd,
    FfsLibTrapMkdir,
    FfsLibTrapRmdir,
    FfsLibTrapFormat,
    FfsLibTrapRename,
    FfsLibTrapGetfileattr,
    FfsLibTrapFlush,
    FfsLibTrapFlushDisk,
    FfsLibTrapSetfileattr,
    FfsLibTrapStat,
    FfsLibTrapFstat,
    FfsLibTrapIsDir,
    FfsLibTrapLseek,
    FfsLibTrapGetErrno,
    FfsLibTrapTell,
    FfsLibTrapGetdrive,
    FfsLibTrapSetdrive,
    FfsLibTrapUnlink,
    FfsLibTrapEof,
    FfsLibTrapCreat,
    FfsLibTrapInstErrHandle,
    FfsLibTrapUnInstErrHandle,
    FfsLibTrapSetDebuggingOn,
    FfsLibTrapSetDebuggingOff,
    FfsLibTrapCardIsInserted,
    FfsLibTrapExerciseFAT,
    FfsLibTrapCardIsATA,
    FfsLibTrapLast
} FfsLibTrapNumberEnum;


/********************************************************************
 *              CF FAT Filesystem API Prototypes
 ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * Standard library open, close, sleep and wake functions
 *-------------------------------------------------------------------------*/

/* open the library */
extern Err FfsLibOpen(UInt16 libRef)
                                FFS_LIB_TRAP(sysLibTrapOpen);
				
/* close the library */
extern Err FfsLibClose(UInt16 libRef)
                                FFS_LIB_TRAP(sysLibTrapClose);

/* library sleep */
extern Err FfsLibSleep(UInt16 libRef)
                                FFS_LIB_TRAP(sysLibTrapSleep);

/* library wakeup */
extern Err FfsLibWake(UInt16 libRef)
                                FFS_LIB_TRAP(sysLibTrapWake);

/*--------------------------------------------------------------------------
 * Custom library API functions
 *--------------------------------------------------------------------------*/

/* Get our library API version */
extern Err FfsGetLibAPIVersion(UInt16 libRef, UInt32 *dwVerP)
                                FFS_LIB_TRAP(FfsLibTrapGetLibAPIVersion);
	
/* Get disk free/total size */
extern Err FfsGetdiskfree(UInt16 libRef, UInt8 drive, diskfree_t *dtable)
                                FFS_LIB_TRAP(FfsLibTrapGetdiskfree);

/* get first directory entry */
extern Err FfsFindfirst(UInt16 libRef, char *path, Int16 attrib, ffblk *ff_blk)
                                FFS_LIB_TRAP(FfsLibTrapFindfirst);

/* get next directory entry */
extern Err FfsFindnext(UInt16 libRef, ffblk *ff_blk)
                                FFS_LIB_TRAP(FfsLibTrapFindnext);

/* finish directory scan */
extern Err FfsFinddone(UInt16 libRef, ffblk *ff_blk)
                                FFS_LIB_TRAP(FfsLibTrapFinddone);

/* open a file */
extern Int16 FfsOpen(UInt16 libRef, char *path, Int16 flags, Int16 mode)
                                FFS_LIB_TRAP(FfsLibTrapFileOpen);

/* close a file */
extern Err FfsClose(UInt16 libRef, Int16 handle)
                                FFS_LIB_TRAP(FfsLibTrapFileClose);

/* read from file */
extern Int16 FfsRead(UInt16 libRef, Int16 handle, void *buffer, Int16 num_bytes)
                                FFS_LIB_TRAP(FfsLibTrapRead);

/* write to file */
extern Int16 FfsWrite(UInt16 libRef, Int16 handle, void *buffer, Int16 num_bytes)
                                FFS_LIB_TRAP(FfsLibTrapWrite);

/* delete file */
extern Err FfsRemove(UInt16 libRef, char *path)
                                FFS_LIB_TRAP(FfsLibTrapRemove);

/* set the current working directory */
extern Err FfsChdir(UInt16 libRef, char *path)
                                FFS_LIB_TRAP(FfsLibTrapChdir);

/* ge the current working directory */
extern char *FfsGetcwd(UInt16 libRef, char *path, Int16 numchars)
                                FFS_LIB_TRAP(FfsLibTrapGetcwd);

/* make a new directory */
extern Err FfsMkdir(UInt16 libRef, char *dirname)
                                FFS_LIB_TRAP(FfsLibTrapMkdir);

/* delete a directory */
extern Err FfsRmdir(UInt16 libRef, char *dirname)
                                FFS_LIB_TRAP(FfsLibTrapRmdir);

/* format a drive */
extern Err FfsFormat(UInt16 libRef, UInt16 drive)
                                FFS_LIB_TRAP(FfsLibTrapFormat);

/* rename a file/directory */
extern Err FfsRename(UInt16 libRef, char *path, char *new_name)
                                FFS_LIB_TRAP(FfsLibTrapRename);

/* get file/directory attributes */
extern Err FfsGetfileattr(UInt16 libRef, char *name, UInt16 *attr)
                                FFS_LIB_TRAP(FfsLibTrapGetfileattr);

/* flush a file to disk */
extern Err FfsFlush(UInt16 libRef, Int16 handle)
                                FFS_LIB_TRAP(FfsLibTrapFlush);

/* flush all buffers to disk */
extern Err FfsFlushDisk(UInt16 libRef, UInt16 drive)
                                FFS_LIB_TRAP(FfsLibTrapFlushDisk);

/* set file attributes */
extern Err FfsSetfileattr(UInt16 libRef, char *name, UInt16 attr)
                                FFS_LIB_TRAP(FfsLibTrapSetfileattr);

/* get information about a path */
extern Err FfsStat(UInt16 libRef, char *path, stat *pstat)
                                FFS_LIB_TRAP(FfsLibTrapStat);

/* get information about an open file */
extern Err FfsFstat(UInt16 libRef, Int16 handle, stat *pstat)
                                FFS_LIB_TRAP(FfsLibTrapFstat);

/* test if a path is a directory */
extern Err FfsIsDir(UInt16 libRef, char *path, Boolean *is_dir)
                                FFS_LIB_TRAP(FfsLibTrapIsDir);

/* move file pointer */
extern Int32 FfsLseek(UInt16 libRef, Int16 handle, Int32 offset, Int16 origin)
                                FFS_LIB_TRAP(FfsLibTrapLseek);

/* get the current errno (global error descriptor) value */
extern Int16 FfsGetErrno(UInt16 libRef)
                                FFS_LIB_TRAP(FfsLibTrapGetErrno);

/* get the current file pointer */
extern Int32 FfsTell(UInt16 libRef, Int16 handle)
                                FFS_LIB_TRAP(FfsLibTrapTell);

/* get default drive */
extern void FfsGetdrive(UInt16 libRef, UInt16 *drive)
                                FFS_LIB_TRAP(FfsLibTrapGetdrive);

/* set default drive, and return number of valid drives */
extern void FfsSetdrive(UInt16 libRef, UInt16 drive, UInt16 *ndrives)
                                FFS_LIB_TRAP(FfsLibTrapSetdrive);

/* delete a file (same as FfsRemove) */
extern Err FfsUnlink(UInt16 libRef, char *path)
                                FFS_LIB_TRAP(FfsLibTrapUnlink);

/* determine if end-of_file */
extern Err FfsEof(UInt16 libRef, Int16 handle)
                                FFS_LIB_TRAP(FfsLibTrapEof);

/* create a file */
extern Int16 FfsCreat(UInt16 libRef, char *path, Int16 mode)
                                FFS_LIB_TRAP(FfsLibTrapCreat);

/* install the critical error handler callback function */
extern void FfsInstallErrorHandler(UInt16 libRef, Int16 (*CritErr)(Int16, Int16, char *))
                                FFS_LIB_TRAP(FfsLibTrapInstErrHandle);

/* uninstall the critical error handler callback function */
extern void FfsUnInstallErrorHandler(UInt16 libRef)
                                FFS_LIB_TRAP(FfsLibTrapUnInstErrHandle);

/* turn on serial debugging */
extern void FfsSetDebuggingOn(UInt16 libRef, UInt16 s_port)
                                FFS_LIB_TRAP(FfsLibTrapSetDebuggingOn);

/* turn off serial debugging */
extern void FfsSetDebuggingOff(UInt16 libRef)
                                FFS_LIB_TRAP(FfsLibTrapSetDebuggingOff);

/* check for inserted card */
extern Boolean FfsCardIsInserted(UInt16 libRef, UInt8 drive_num)
                                FFS_LIB_TRAP(FfsLibTrapCardIsInserted);

/* for internal FAT testing only */
extern Err FfsExerciseFAT(UInt16 libRef, UInt8 drive_num)
                                FFS_LIB_TRAP(FfsLibTrapExerciseFAT);

/* check if inserted card is an ATA type card */
extern Boolean FfsCardIsATA(UInt16 libRef, UInt8 drive_num)
                                FFS_LIB_TRAP(FfsLibTrapCardIsATA);

/*---------------------------------------------------------------------------
 * For loading the library in PalmPilot Mac emulation mode
 *--------------------------------------------------------------------------*/

extern Err FfsLibInstall(UInt16 libRef, SysLibTblEntryPtr entryP);


#ifdef __cplusplus 
}
#endif


#endif  // __FFS_LIB_H__
