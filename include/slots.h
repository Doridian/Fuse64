#ifndef SLOTS_H
#define SLOTS_H

/* The objects in D_800BA900's sixteen slots, the tasks in D_800BB340 that drive them, and D_802B0260's two entries (main_code's 9CAB0, A1EA0 and A2840). */

#include "common.h"

/* The object the tables' functions are handed (A2840's name); it is
 * D_800BA900[i].unk14. */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ u8 unk1C;
    /* 0x1D */ u8 unk1D;
    /* 0x1E */ u8 unk1E;
    /* 0x1F */ u8 unk1F;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
    /* 0x3C */ u8 unk3C;
    /* 0x3D */ u8 unk3D;
    /* 0x3E */ u16 unk3E;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s32 unk48;
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ union {
        f32 f;
        u8 b[4];
    } unk50;
    /* 0x54 */ f32 unk54;
    /* 0x58 */ union {
        f32 f;
        u8 b[4];
    } unk58;
    /* 0x5C */ u8 unk5C;
    /* 0x5D */ u8 unk5D;
    /* 0x5E */ u16 unk5E;
    /* 0x60 */ f32 unk60;
    /* 0x64 */ f32 unk64;
    /* 0x68 */ PORT_IP(s32) unk68;
    /* 0x6C */ u8 unk6C;
    /* 0x6D */ u8 unk6D;
    /* 0x6E */ s16 unk6E;
    /* 0x70 */ f32 unk70;
    /* 0x74 */ f32 unk74;
    /* 0x78 */ u8 unk78;
    /* 0x79 */ s8 unk79;
    /* 0x7A */ u8 unk7A;
    /* 0x7C */ u16 unk7C;
    /* 0x7E */ s16 unk7E;
    /* 0x80 */ s32 unk80;
    /* 0x84 */ s32 unk84;
    /* 0x88 */ s16 unk88;
    /* 0x8C */ PORT_IP(s32) unk8C; /* its task (D_800BB340[i]), written only */
} UnkStruct_80288040; /* size 0x90 */

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
} UnkStruct_800BA8E8;

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ Vec3f unk8;
    /* 0x14 */ UnkStruct_80288040 unk14;
} UnkStruct_800BA900; /* size 0xA4 */

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ UnkStruct_800BA900 *unk10;
} UnkStruct_800BB340; /* size 0x14 */

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ f32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
    /* 0x3C */ f32 unk3C;
    /* 0x40 */ f32 unk40;
    /* 0x44 */ f32 unk44;
    /* 0x48 */ f32 unk48;
    /* 0x4C */ f32 unk4C;
} UnkStruct_802B0260; /* size 0x50 */

typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 unk12;
} UnkStruct_802B0300;

typedef struct {
    /* 0x00 */ u8 unk0[8];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ u8 unk30[6];
    /* 0x36 */ s16 unk36;
} UnkStruct_802B0308;

#endif
