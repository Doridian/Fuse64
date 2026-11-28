#ifndef PROTO_85A10_H
#define PROTO_85A10_H

/* src/85A10.c's functions that other files call: their one declaration
 * (docs/DECOMPILING.md, "Prototypes"). */

#include "common.h"

/* error flags (1 heap, 2 DMA, 4 AI underrun, 8 command list, 0x10 DMA
 * buffers) and peaks: DMAs per frame, buffers used, commands */
typedef struct {
    /* 0x00 */ u32 flags;
    /* 0x04 */ u32 maxDMAs;
    /* 0x08 */ u32 maxBuffers;
    /* 0x0C */ u32 maxCmds;
} UnkStruct_802AD680;

/* libaudio.h's ALSynConfig (80580's D_802A1254) */
typedef struct {
    /* 0x00 */ s32 maxVVoices;
    /* 0x04 */ s32 maxPVoices;
    /* 0x08 */ s32 maxUpdates;
    /* 0x0C */ s32 maxFXbusses;
    /* 0x10 */ void *dmaproc;
    /* 0x14 */ void *heap;
    /* 0x18 */ s32 outputRate;
    /* 0x1C */ u8 fxType;
    /* 0x20 */ s32 *params;
} UnkStruct_8026B9CC_0;

/* audiomgr.h's amConfig, Hudson's (80580's D_802A1278) */
typedef struct {
    /* 0x00 */ u16 numDMABuffers;
    /* 0x04 */ s32 dmaBufferLength;
    /* 0x08 */ u32 framesPerField;
    /* 0x0C */ u32 maxACMDSize;
    /* 0x10 */ s32 outputRate;
    /* 0x14 */ void *sched;
    /* 0x18 */ s32 pri;
    /* 0x1C */ s32 id;
} UnkStruct_8026B9CC_1;

void *func_8026B72C(s32 size);
UnkStruct_802AD680 *func_8026B79C(void);
void func_8026B7A8(u32 frames);
void func_8026B7D4(void);
s16 func_8026B9CC(UnkStruct_8026B9CC_0 *c, UnkStruct_8026B9CC_1 *amc);

#endif
