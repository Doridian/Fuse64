#ifndef PROTO_41FD0_H
#define PROTO_41FD0_H

/* src/41FD0.c's functions that other files call: their one declaration
 * (docs/DECOMPILING.md, "Prototypes"). */

#include "common.h"
#include "gfx.h"

/* a viewport, its view matrix and its scissor box (41FD0's D_800BD5B0[4];
 * what func_80227E38 hands out, 54BE0's D_802AC5C0 and D_802AC5C4) */
typedef struct {
    /* 0x00 */ s16 vscale[4];
    /* 0x08 */ s16 vtrans[4];
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ UnkStruct_802A53D8 unk18;
    /* 0x58 */ void *unk58;
    /* 0x5C */ f32 unk5C;
    /* 0x60 */ f32 unk60;
    /* 0x64 */ f32 unk64;
    /* 0x68 */ f32 unk68;
    /* 0x6C */ s32 unk6C;
} UnkStruct_800BD5B0; /* size 0x70 */

void func_802277D0(void);
void func_80227870(void);
void func_8022787C(Gfx_80225840 **arg0);
void func_80227C18(void);
void func_80227C50(f32 fovy, f32 aspect, f32 near, f32 far, f32 scale);
void func_80227C9C(UnkStruct_800BD5B0 *arg0, s32 arg1);
void func_80227CD0(UnkStruct_800BD5B0 *arg0, f32 xEye, f32 yEye, f32 zEye, f32 xAt, f32 yAt, f32 zAt, f32 xUp,
                   f32 yUp, f32 zUp);
void func_80227D50(UnkStruct_800BD5B0 *arg0, u32 x, u32 y, u32 w, u32 h);
UnkStruct_800BD5B0 *func_80227E38(void);

#endif
