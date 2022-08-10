#define PILOT_PRECOMPILED_HEADERS_OFF

/***********************************************************************
 *
 * Copyright (c) 1999, TRG, All Rights Reserved
 *
 * PROJECT:         Nomad FAT demo
 *
 * FILE:     cmd.c       
 *
 * DESCRIPTION:   Contains the Ffs demo program commands. 
 *
 * AUTHOR: Trevor Meyer         
 *
 * DATE: 8/9/99           
 *
 **********************************************************************/

#include <Pilot.h>
#include <SysEvtMgr.h>

#include "com.h"
#include "cmd_util.h"
#include "trglib.h"
#include "cmd.h"
#include "ffslib.h"


#define DEFAULT_NUM_LINES       12

static int     index = 0;
static char    cwd[40];

static char   *GetStr(void);

extern UInt16  FfsLibRef;


/*--------------------------------------------------------------------------
 * Function    : Help
 * Description : Display the command menu.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void Help(void)
{
    COMPrintf("TRG FAT Library demo V 1.01\r\n\r\n");

    COMPrintf("  ATTRIB <name>                 - Get file/directory attributes\r\n");
    COMPrintf("  CD <dir>                      - Change directory\r\n");
    COMPrintf("  COPY <src> <dest>             - Copy file\r\n");
    COMPrintf("  DEL <filename>                - Delete a file\r\n");
    COMPrintf("  DIR [dir]                     - Print directory\r\n");
    COMPrintf("  FORMAT                        - Format drive\r\n");
    COMPrintf("  FREE                          - Drive free space\r\n");
    COMPrintf("  MKDIR <dir>                   - Make a subdirectory\r\n");
    COMPrintf("  REN <oldname> <newname>       - Rename file or directory\r\n");
    COMPrintf("  RMDIR <dir>                   - Remove a subdirectory\r\n");
    COMPrintf("  SDIR [dir]                    - Display all files with attributes\r\n");
    COMPrintf("  STAT <path>                   - Display statistics on a path\r\n");
    COMPrintf("  TYPE <filename>               - Display file contents\r\n");
    COMPrintf("\r\n");
    COMPrintf("  CTRL-A                       - Previous command\r\n");
    COMPrintf("  ?                            - help\r\n");
}


/*--------------------------------------------------------------------------
 * Function    : GetString
 * Description : Get a complete command string from the user. Non-blocking --
 *               when called, adds the specified character to the current
 *               command string and returns. Allows backspace, and resets
 *               power-off timer each time something is typed.
 * Params      : chr -- character from serial port
 * Returns     : true if a full command string is available, false otherwise
 *--------------------------------------------------------------------------*/
Boolean GetString(char chr)
{
    static Boolean search_string = false;

    /* got user activity -- don't time out */
    EvtResetAutoOffTimer();

    switch (chr)
    {
        case '\b' :
            if (index > 0)
            {
                COMPrintf("\b \b");
                index--;
            }
            break;
        case '\r' :
            cmdline[index] = 0;
            COMPrintf("\r\n");
            search_string = false;
            return(true);
        default :
            if (isprint(chr) && (index < MAX_LINE_LENGTH))
            {
                COMPrintf("%c", chr);

                /* " signifies start of a literal search string */
                if (chr == '\"')
                    search_string = true;

                /* if entering search string, don't capitalize */
                if (!search_string)
                    cmdline[index++] = (char)toupper(chr);
                else
                    cmdline[index++] = chr;
            }
            break;
    }
    return(false);
}


/*--------------------------------------------------------------------------
 * Function    : GetStr
 * Description : Gets a response from the user over the serial port.
 *               Blocking (events are not processed while this is waiting).
 *               Returns when a CR is received.
 * Params      : none
 * Returns     : String entered from user.
 *--------------------------------------------------------------------------*/
static char *GetStr(void)
{
    char chr;
     
    cmdline[0] = '\0';
    index = 0;

    do {
        while(COMBytesAvailable() == 0)    /* wait -- COMGetC() times out */
           ;
        COMGetC(&chr);

    } while (!GetString(chr));

    return(&(cmdline[0]));
}


/*--------------------------------------------------------------------------
 * Function    : CMDStat
 * Description : Get file statistics. Demonstrates the FFsStat() call. Does
 *               not support wildcards.
 * Params      : Command line containing file/path name.
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDStat(char *cmdline)
{
    stat  pstat;

    StripWhiteSpace(&cmdline);

    if (FfsStat(FfsLibRef,cmdline,&pstat) != 0)
    {
        COMPrintf("Path not found\r\n");
        return;
    }

    COMPrintf("Statistics for [%s]:\r\n",cmdline);
    COMPrintf("    drive: %d\r\n",pstat.st_dev);
    COMPrintf("     size: %ld bytes\r\n",pstat.st_size);
    COMPrintf("     date: %02d-%02d-%02d\r\n",
                        (pstat.st_atime.date >> 5) & 0xf,
                        (pstat.st_atime.date) & 0x1f,
                        80 + (pstat.st_atime.date >> 9) & 0xff);
    COMPrintf("     time: %02d:%02d\r\n",
                        (pstat.st_atime.time >> 11) & 0x1f,
                        (pstat.st_atime.time >> 5) & 0x3f);
    COMPrintf("     mode: 0x%08lX\r\n",pstat.st_mode);
    COMPrintf("attribute: 0x%02X\r\n",pstat.st_attr);
}


/*--------------------------------------------------------------------------
 * Function    : CMDAttrib
 * Description : Get the file attributes for a file/path. Demonstrates the
 *               FfsGetfileattr() call. Does not support wildcards.
 * Params      : command line containing file/path
 * Returns     : Nothing
 *--------------------------------------------------------------------------*/
static void CMDAttrib(char *cmdline)
{
    UInt16 attrib;

    StripWhiteSpace(&cmdline);

    if (FfsGetfileattr(FfsLibRef,cmdline,&attrib) != 0)
    {
        COMPrintf("File not found\r\n");
        return;
    }

    if (attrib & FA_ARCH)
        COMPrintf("A ");
    if (attrib & FA_RDONLY)
        COMPrintf("R ");
    if (attrib & FA_HIDDEN)
        COMPrintf("H ");
    if (attrib & FA_SYSTEM)
        COMPrintf("S ");
    if (attrib & FA_LABEL)
        COMPrintf("V ");
    if (attrib & FA_DIREC)
        COMPrintf("D ");

    COMPrintf("     %s\r\n",cmdline);
}


/*--------------------------------------------------------------------------
 * Function    : CMDCopyFile
 * Description : Make a copy of a file. Demonstrates the FfsOpen(), FfsEof(),
 *               FfsRead(), FfsWrite(), FfsClose() calls. Does not support
 *               wildcards. Must specify destination filename.
 * Params      : Command string containing source and destination name/paths.
 * Returns     : Nothing
 *--------------------------------------------------------------------------*/
#define BUF_SIZE        40960L
static void CMDCopyFile(char *cmdline)
{
    UInt16         num_read;
    UInt8	  *buf;
    static char    src[35], dest[35];
    Int16          src_fd, dest_fd;

    if (GetStrExpression(&cmdline, src) != 0)
    {
        COMPrintf("Syntax: COPY <src> <dest>\r\n");
        return;
    }

    if (GetStrExpression(&cmdline, dest) != 0)
    {
        COMPrintf("Syntax: COPY <src> <dest>\r\n");
        return;
    }

    /* allocate a 40K buffer to hold data */
    if ((buf = (UInt8 *)MemPtrNew(BUF_SIZE)) == NULL)
    {
        COMPrintf("Can't allocate buffer\r\n");
        return;
    }

    if ((src_fd = FfsOpen(FfsLibRef,src,O_RDONLY,0)) == -1)
    {
        COMPrintf("Can't open source file\r\n");
        MemPtrFree(buf);
        return;
    }

    if ((dest_fd = FfsOpen(FfsLibRef,dest,O_CREAT|O_WRONLY,S_IWRITE)) == -1)
    {
        COMPrintf("Can't create destination file\r\n");
        FfsClose(FfsLibRef,src_fd);
        MemPtrFree(buf);
        return;
    }

    /* copy the data */
    while(!FfsEof(FfsLibRef, src_fd))
    {
        if ((num_read = FfsRead(FfsLibRef,src_fd,buf,(Int16)BUF_SIZE)) == 0xFFFF)
        {
            COMPrintf("Source file read error\r\n");
            break;
        }

        if (num_read != FfsWrite(FfsLibRef,dest_fd,buf,num_read))
        {
            COMPrintf("Dest write error\r\n");
            break;
        }
    }

    FfsClose(FfsLibRef,src_fd);
    FfsClose(FfsLibRef,dest_fd);
    MemPtrFree(buf);
}


/*--------------------------------------------------------------------------
 * Function    : CMDGetFreeSize
 * Description : Get the free space on the CF card. Demonstrates the
 *               FfsGetdiskfree() call.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDGetFreeSize(void)
{
    diskfree_t  dfree;
    UInt32      bytes_p_cluster;

    FfsGetdiskfree(FfsLibRef,0,&dfree);
    bytes_p_cluster = (UInt32)dfree.bytes_per_sector * (UInt32)dfree.sectors_per_cluster;

    COMPrintf("Free space %ld bytes, total size %ld bytes\r\n",
                dfree.avail_clusters * bytes_p_cluster,
                dfree.total_clusters * bytes_p_cluster);
}


/*--------------------------------------------------------------------------
 * Function    : CMDType
 * Description : Display the contents of a file in ASCII. Demonstrates
 *               the FfsOpen(), FfsRead(), FfsClose() calls.
 * Params      : Command line containing filename.
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDType(char *cmdline)
{
    Int16         fd;
    UInt16        i, num_read;
    Boolean       done = false;
    static UInt8  buf[1000];
 
    StripWhiteSpace(&cmdline);

    if ((fd = FfsOpen(FfsLibRef, cmdline,O_RDONLY,0)) == -1)
    {
        COMPrintf("Can't open file\r\n");
        return;
    }

    while(!done)
    {
        if ((num_read = FfsRead(FfsLibRef,fd,buf,100)) == 0xFFFF)
        {
            COMPrintf("File read error\r\n");
            FfsClose(FfsLibRef,fd);
            return;
        }

        /* reached end of file? */
        if (num_read != 100)
            done = true;

        for(i=0; i<num_read; i++)
            COMPrintf("%c",buf[i]);
    }

    FfsClose(FfsLibRef,fd);
}


/*--------------------------------------------------------------------------
 * Function    : CMDDelete
 * Description : Delete a file. Demonstrates the FfsRemove() call. Does  not
 *               support wildcards.
 * Params      : Command line containing filename.
 * Returns     : Nothing
 *--------------------------------------------------------------------------*/
static void CMDDelete(char *cmdline)
{
    StripWhiteSpace(&cmdline);

    if (FfsRemove(FfsLibRef, cmdline) != 0)
        COMPrintf("File not found or in use\r\n");
}


/*--------------------------------------------------------------------------
 * Function    : CMDRename
 * Description : Rename a file or directory. Demonstrates the FfsRename
 *               call. Does not support wildcards.
 * Params      : Command string with old and new names.
 * Returns     : Nothing
 *--------------------------------------------------------------------------*/
static void CMDRename(char *cmdline)
{
    static char  oldname[100], newname[100];

    if (GetStrExpression(&cmdline, oldname) != 0)
    {
        COMPrintf("Syntax: REN <oldname> <newname>\r\n");
        return;
    }

    if (GetStrExpression(&cmdline, newname) != 0)
    {
        COMPrintf("Syntax: REN <oldname> <newname>\r\n");
        return;
    }

    if (FfsRename(FfsLibRef, oldname, newname) != 0)
        COMPrintf("Path/file not found\r\n");
}


/*--------------------------------------------------------------------------
 * Function    : CMDFormat
 * Description : Format a CF card. Demonstrates the FfsFormat() call. FAT
 *               type (FAT12/FAT16/FAT32) used is based on the size of the
 *               card.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDFormat(void)
{
    char *response;

    COMPrintf("All files in A: will be deleted. Continue (Y/N)?");

    response = GetStr();
    if (strlen(response) > 0)
    {
        if ((*response == 'y') || (*response == 'Y'))
        {
            if (FfsFormat(FfsLibRef,0) == 0)
                COMPrintf("Format successful\r\n");
            else
                COMPrintf("Format failed\r\n");
        }
    }
}


/*--------------------------------------------------------------------------
 * Function    : CMDRemoveDir
 * Description : Delete an empty directory. Demonstrates the FfsRmdir()
 *               call. 
 * Params      : Command string containing directory name.
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDRemoveDir(char *cmdline)
{
    StripWhiteSpace(&cmdline);

    if (FfsRmdir(FfsLibRef, cmdline) != 0)
        COMPrintf("Error deleting directory\r\n");
}


/*--------------------------------------------------------------------------
 * Function    : CMDMakeDir
 * Description : Create a directory. Demonstrates the FfsMkdir() call.
 * Params      : Command line containing directory name.
 * Returns     : Nothing
 *--------------------------------------------------------------------------*/
static void CMDMakeDir(char *cmdline)
{
    StripWhiteSpace(&cmdline);

    if (FfsMkdir(FfsLibRef, cmdline) != 0)
        COMPrintf("Error making directory\r\n");
}


/*--------------------------------------------------------------------------
 * Function    : CMDChangeDir
 * Description : Change the current working directory. Demonstrates the
 *               FfsChdir(), FfsGetcwd() calls.
 * Params      : Command line containing target directory.
 * Returns     : Nothing
 *--------------------------------------------------------------------------*/
static void CMDChangeDir(char *cmdline)
{
    StripWhiteSpace(&cmdline);

    if (FfsChdir(FfsLibRef, cmdline) != 0)
        COMPrintf("Path not found\r\n");
    else
       FfsGetcwd(FfsLibRef, cwd, 39);
}


/*--------------------------------------------------------------------------
 * Function    : CMDPrintDirectory
 * Description : Display a directory of the CF card. Demonstrates FfsIsDir(),
 *               Ffsfindfirst(), FfsFindnext(), FfsFinddone(),
 *               FfsGetdiskfree() calls. Some support for wildcards.
 * Params      : Command string containing path
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDPrintDirectory(char *cmdline)
{
    UInt16       hour, files = 0;
    char         am_pm;
    static char  path[100], usr_dir[100];
    static char  print_path[100];
    UInt32       tot_size = 0L;
    ffblk        stat;
    diskfree_t   dfree;
    Boolean      is_dir;

    /* prepare path for printing and seraching */
    if (GetStrExpression(&cmdline, usr_dir) == 0)
        strcpy(path,usr_dir);
    else
        strcpy(path,cwd);
    strcpy(print_path,path);	/* print without "*.*" */
    FfsIsDir(FfsLibRef, path, &is_dir);
    if (is_dir)
        strcat(path,"\\*.*");
    
    /* print volume label, if any */
    if (FfsFindfirst(FfsLibRef, path, FA_LABEL, &stat) == 0)
        COMPrintf("Volume in drive A is %s\r\n",stat.ff_name);
    FfsFinddone(FfsLibRef, &stat);

    COMPrintf("Directory of A:%s\r\n\r\n",print_path);

    /* loop through all directory entries */
    if (FfsFindfirst(FfsLibRef, path, FA_ALL, &stat) == 0)
    {
        do
        {
            /* don't show the volume labels, hidden or system files */
            if ((stat.ff_attrib & FA_LABEL) ||
                (stat.ff_attrib & FA_HIDDEN) ||
                (stat.ff_attrib & FA_SYSTEM))
            {
                continue;
            }

            /* keep track of total used size */
            files++;
            tot_size += stat.ff_fsize;

            /* convert military to AM/PM time */
            hour = (UInt16)(stat.ff_ftime >> 11 & 0x1f);
            if (hour > 12)
            {
                hour -= 12;
                am_pm = 'p';
            }
            else
                am_pm = 'a';

            /* print the DOS style directory entry */
            COMPrintf("%-12s ",stat.ff_name);
            if (stat.ff_attrib & FA_DIREC)
                COMPrintf("<DIR>         ");
            else
                COMPrintf("      %7ld ",stat.ff_fsize);
            COMPrintf("%02d-%02d-%02d  %2d:%02d%c ",(stat.ff_fdate >> 5) & 0xf,
                        stat.ff_fdate & 0x1f, 80 + (stat.ff_fdate >> 9) & 0xff,
                        hour, (stat.ff_ftime >> 5) & 0x3f, am_pm);
            COMPrintf("   %s\r\n",stat.ff_longname);
        } while (FfsFindnext(FfsLibRef, &stat) == 0);
    }
    FfsFinddone(FfsLibRef, &stat);

    COMPrintf("    %3d file(s)    %9ld bytes\r\n",files,tot_size);
    FfsGetdiskfree(FfsLibRef,0,&dfree);
    COMPrintf("                   %9ld bytes free\r\n",(UInt32)dfree.avail_clusters *
                (UInt32)dfree.sectors_per_cluster * (UInt32)dfree.bytes_per_sector);
}


/*--------------------------------------------------------------------------
 * Function    : CMDPrintSDir
 * Description : Display statistics for all files in a directory.
 *               Demonstrates Ffsfindfirst(), FfsFindnext(), FfsFinddone()
 *               calls. Does not support wildcards.
 * Params      : Command string containing path
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
static void CMDPrintSDir(char *cmdline)
{
    static char  path[100], usr_dir[100];
    ffblk        stat;

    /* prepare path for printing and seraching */
    if (GetStrExpression(&cmdline, usr_dir) == 0)
        strcpy(path,usr_dir);
    else
        strcpy(path,cwd);

    COMPrintf("Directory entries for A:%s\r\n\r\n",path);
    strcat(path,"\\*.*");

    if (FfsFindfirst(FfsLibRef, path, FA_ALL, &stat) == 0)
    {
        do
        {
	    /* print attribute */
            if (stat.ff_attrib & FA_ARCH)
                COMPrintf("A ");
            else
                COMPrintf("  ");
            if (stat.ff_attrib & FA_RDONLY)
                COMPrintf("R ");
            else
                COMPrintf("  ");
            if (stat.ff_attrib & FA_HIDDEN)
                COMPrintf("H ");
            else
                COMPrintf("  ");
            if (stat.ff_attrib & FA_SYSTEM)
                COMPrintf("S ");
            else
                COMPrintf("  ");
            if (stat.ff_attrib & FA_LABEL)
                COMPrintf("V ");
            else
                COMPrintf("  ");
            if (stat.ff_attrib & FA_DIREC)
                COMPrintf("D ");
            else
                COMPrintf("  ");

            /* print name, size, time, date */
            COMPrintf("  %-12s  %7ld ",stat.ff_name,stat.ff_fsize);
            COMPrintf("%02d-%02d-%02d  %2d:%02d",(stat.ff_fdate >> 5) & 0xf,
                        stat.ff_fdate & 0x1f, 80 + (stat.ff_fdate >> 9) & 0xff,
                        (stat.ff_ftime >> 11) & 0x1f, (stat.ff_ftime >> 5) & 0x3f);
            COMPrintf("   %s\r\n",stat.ff_longname);
        } while (FfsFindnext(FfsLibRef, &stat) == 0);
    }
    FfsFinddone(FfsLibRef, &stat);
}


/*--------------------------------------------------------------------------
 * Function    : CMDInit
 * Description : Initializes the command interpreter.
 * Params      : none
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
void CMDInit(void)
{
    cmdline[0] = '\0';
    index = 0;
    strcpy(cwd,"\\");
    COMPrintf("\r\n\r\nA:%s>",cwd);
}    


/*--------------------------------------------------------------------------
 * Function    : CMDProcess
 * Description : Get commands from the user over the serial port and execute
 *               the commands. Called each time a character is typed, and
 *               calls GetString to build up a command string.
 * Params      : chr -- current character
 * Returns     : nothing
 *--------------------------------------------------------------------------*/
void CMDProcess(char chr)
{
    char  cmd[10];
    char  *cmd_ptr;
    
    if ((index == 0) && (chr == 1))   //CTRL-A
    {
        StrCopy(cmdline, prevline);
        index = (int)strlen(cmdline);
        COMPrintf("%s", cmdline);
        return;
    }

    /* is a complete command available? */
    if (!GetString(chr))
        return;

    /* save the command so that CTRL-A can retrieve it */
    strcpy(prevline, cmdline);
    
    cmd_ptr = cmdline;
    GetStrExpression(&cmd_ptr, cmd);
    Capitalize(cmd);

    /* execute the command */
    if (strcmp(cmd,"CD") == 0)
        CMDChangeDir(cmd_ptr);

    else if (strcmp(cmd,"ATTRIB") == 0)
        CMDAttrib(cmd_ptr);

    else if (strcmp(cmd,"STAT") == 0)
        CMDStat(cmd_ptr);

    else if (strcmp(cmd,"TYPE") == 0)
        CMDType(cmd_ptr);

    else if (strcmp(cmd,"COPY") == 0)
        CMDCopyFile(cmd_ptr);

    else if (strcmp(cmd,"REN") == 0)
        CMDRename(cmd_ptr);

    else if (strcmp(cmd,"MKDIR") == 0)
        CMDMakeDir(cmd_ptr);

    else if (strcmp(cmd,"RMDIR") == 0)
        CMDRemoveDir(cmd_ptr);

    else if (strcmp(cmd,"DEL") == 0)
        CMDDelete(cmd_ptr);

    else if (strcmp(cmd,"FORMAT") == 0)
        CMDFormat();

    else if (strcmp(cmd,"FREE") == 0)
        CMDGetFreeSize();

    else if (strcmp(cmd,"DIR") == 0)
        CMDPrintDirectory(cmd_ptr);

    else if (strcmp(cmd,"SDIR") == 0)
        CMDPrintSDir(cmd_ptr);

    else if (strcmp(cmd,"?") == 0)
        Help();

    index = 0;
    cmdline[0] = '\0';
    COMPrintf("A:%s>",cwd);
}
