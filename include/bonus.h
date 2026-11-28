#ifndef BONUS_H
#define BONUS_H

/* The bonus items (main_code's 93850 and 949A0: the list at D_800BB4A8 and its thread). */

#include "common.h"

/* A node of the list at D_800BB4A8; the last node is a sentinel */
typedef struct UnkStruct_800BB4A8 {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ struct UnkStruct_800BB4A8 *unk8;
    /* 0x0C */ struct UnkStruct_800BB4A8 *unkC;
    /* 0x10 */ UnkStructA_40 *unk10;
    /* 0x14 */ UnkStructA *unk14;
    /* 0x18 */ PORT_IP(s32) unk18;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ s16 unk20;
    /* 0x22 */ s16 unk22;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ s16 unk30;
    /* 0x32 */ s16 unk32;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
    /* 0x3C */ f32 unk3C;
    /* 0x40 */ f32 unk40;
    /* 0x44 */ f32 unk44;
    /* 0x48 */ s16 unk48;
    /* 0x4A */ u8 unk4A[6];
    /* 0x50 */ f32 unk50;
    /* 0x54 */ s32 unk54;
} UnkStruct_800BB4A8;

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} UnkStruct_802AFED0; /* size 0x1C */

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
} UnkStruct_800BA8A0; /* size 0xE */

#endif
