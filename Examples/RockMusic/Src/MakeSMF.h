/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MakeSMF.h
 *
 * Description:
 *             	routines to create a SMF in memory
 *
 *****************************************************************************/

MemHandle StartSMF();
MemHandle AppendNote(MemHandle bufH, int note, int dur, int vel, int pause);
MemHandle FinishSMF(MemHandle bufH);

