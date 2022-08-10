/****************************************************************************
 *
 *      Copyright (c) 1999, TRG, All Rights Reserved
 *
 *---------------------------------------------------------------------------
 * FileName:
 *              AudioLib.h
 *
 * Description:
 *              Audio library API definitions.
 *
 *
 ****************************************************************************/


#ifndef __AUDIO_LIB_H__
#define __AUDIO_LIB_H__


/*---------------------------------------------------------------------------
 * If we're actually compiling the library code, then we need to
 * eliminate the trap glue that would otherwise be generated from
 * this header file in order to prevent compiler errors in CW Pro 2.
 *--------------------------------------------------------------------------*/
#ifdef BUILDING_AUDIO_LIB
        #define AUDIO_LIB_TRAP(trapNum)
#else
        #define AUDIO_LIB_TRAP(trapNum) SYS_TRAP(trapNum)
#endif


/****************************************************************************
 * Type and creator of Sample Library database -- must match project defs!
 ****************************************************************************/
#define AudioLibCreatorID  'trgA'       // Audio Library database creator
#define AudioLibTypeID     'libr'       // Standard library database type


/***************************************************************************
 * Internal library name which can be passed to SysLibFind()
 ***************************************************************************/
#define AudioLibName       "Audio.lib"     


/***************************************************************************
 * Defines for Audio library calls
 ***************************************************************************/

/*--------------------------------------------------------------------------
 * Audio Library result codes
 * (appErrorClass is reserved for 3rd party apps/libraries.
 * It is defined in SystemMgr.h)
 *
 * These are for errors specific to loading/opening/closing the library
 *-------------------------------------------------------------------------*/
#define AudioErrorClass              (appErrorClass | 0x400)

#define AUDIO_ERR_BAD_PARAM          (AudioErrorClass | 1)    // invalid parameter
#define AUDIO_ERR_LIB_NOT_OPEN       (AudioErrorClass | 2)    // library is not open
#define AUDIO_ERR_LIB_IN_USE         (AudioErrorClass | 3)    // library still in used

typedef enum {
    AudioLibTrapGetLibAPIVersion = sysLibTrapCustom,
    AudioLibTrapGetMasterVolume,
    AudioLibTrapSetMasterVolume,
    AudioLibTrapGetMute,
    AudioLibTrapSetMute,
    AudioLibTrapPlayDTMFChar,
    AudioLibTrapPlayDTMFStr,
    AudioLibTrapLast
} AudioLibTrapNumberEnum;


/********************************************************************
 *              
 ********************************************************************/

typedef enum {AUDIO_MUTE_OFF, AUDIO_MUTE_ON} mute_type;

#define AUDIO_VOLUME_MAX      63
#define AUDIO_VOLUME_MIN      0

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * Standard library open, close, sleep and wake functions
 *-------------------------------------------------------------------------*/

/* open the library */
extern Err AudioLibOpen(UInt16 libRef)
                                AUDIO_LIB_TRAP(sysLibTrapOpen);
				
/* close the library */
extern Err AudioLibClose(UInt16 libRef)
                                AUDIO_LIB_TRAP(sysLibTrapClose);

/* library sleep */
extern Err AudioLibSleep(UInt16 libRef)
                                AUDIO_LIB_TRAP(sysLibTrapSleep);

/* library wakeup */
extern Err AudioLibWake(UInt16 libRef)
                                AUDIO_LIB_TRAP(sysLibTrapWake);

/*--------------------------------------------------------------------------
 * Custom library API functions
 *--------------------------------------------------------------------------*/

/* Get our library API version */
extern Err AudioGetLibAPIVersion(UInt16 libRef, UInt32 *dwVerP)
                                 AUDIO_LIB_TRAP(AudioLibTrapGetLibAPIVersion);
	
extern Err AudioGetMasterVolume(UInt16 libRef, UInt8 *volume)
                                 AUDIO_LIB_TRAP(AudioLibTrapGetMasterVolume);
	
extern Err AudioSetMasterVolume(UInt16 libRef, UInt8 volume)
                                 AUDIO_LIB_TRAP(AudioLibTrapSetMasterVolume);

extern Err AudioGetMute(UInt16 libRef, mute_type *mute)
                                 AUDIO_LIB_TRAP(AudioLibTrapGetMute);

extern Err AudioSetMute(UInt16 libRef,mute_type mute)
                                 AUDIO_LIB_TRAP(AudioLibTrapSetMute);

	
extern void AudioPlayDTMFChar(UInt16 libRef,char ascChar, Int16 toneLength)
                                 AUDIO_LIB_TRAP(AudioLibTrapPlayDTMFChar);

extern void AudioPlayDTMFStr(UInt16 libRef,char *ascStr, Int16 toneLength, Int16 toneGap)
                                 AUDIO_LIB_TRAP(AudioLibTrapPlayDTMFStr);

/*---------------------------------------------------------------------------
 * For loading the library in PalmPilot Mac emulation mode
 *--------------------------------------------------------------------------*/

extern Err AudioLibInstall(UInt16 libRef, SysLibTblEntryPtr entryP);


#ifdef __cplusplus 
}
#endif


#endif  // __AUDIO_LIB_H__
