#ifndef ENT_H
#define ENT_H

/* The entities of main_code's 87D80 and 8A7A0 (D_8008F438's Ent88, the objects they draw, their controllers) (the players they belong to are player.h's). */

#include "common.h"
#include "player.h"

typedef struct Node87 {
    /* 0x00 */ struct Node87 *next;
    /* 0x04 */ s32 unk4;
} Node87;

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u8 pad2[6];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ PORT_IP(s32) unkC; /* an object */
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u8 pad12[0x1C - 0x12];
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
} Slot30; /* size 0x30 */

typedef struct Ctl87 {
    /* 0x00 */ struct Ctl87 *next;
    /* 0x04 */ UnkStructA *obj;
    /* 0x08 */ UnkStructA *unk8;
    /* 0x0C */ UnkStructA *unkC;
    /* 0x10 */ s32 flags;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
} Ctl87;

/* 926A0's records, D_8008E658[48]: what func_80277FC4 hands out (an
 * Ent88's unk24[]) */
typedef struct Rec926 {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u8 pad2[6];
    /* 0x08 */ PORT_IP(s32) *unk8; /* the caller's slot that holds this record */
    /* 0x0C */ s16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u8 pad12[2];
    /* 0x14 */ UnkStructA *unk14;
    /* 0x18 */ UnkStructA_40 *unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ f32 unk30;
    /* 0x34 */ u8 pad34[2];
    /* 0x36 */ s16 unk36;
    /* 0x38 */ s16 unk38;
    /* 0x3A */ s16 unk3A;
    /* 0x3C */ f32 unk3C;
    /* 0x40 */ f32 unk40;
    /* 0x44 */ s16 unk44;
    /* 0x46 */ s16 unk46;
} Rec926; /* size 0x48 */

typedef struct Ent88 {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u8 pad2[2];
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u8 pad12[2];
    /* 0x14 */ struct Ent88 *next;
    /* 0x18 */ struct Ent88 *prev;
    /* 0x1C */ UnkStructA *obj;
    /* 0x20 */ UnkStructA_40 *xf;
    /* 0x24 */ void *unk24[3];
    /* 0x30 */ UnkStructA *unk30;
    /* 0x34 */ UnkStructA *unk34;
    /* 0x38 */ void *unk38;
    /* 0x3C */ s16 unk3C;
    /* 0x3E */ u8 pad3E[2];
    /* 0x40 */ f32 x;
    /* 0x44 */ f32 y;
    /* 0x48 */ f32 z;
    /* 0x4C */ f32 unk4C;
    /* 0x50 */ f32 unk50;
    /* 0x54 */ f32 unk54;
    /* 0x58 */ u8 pad58[4];
    /* 0x5C */ s16 unk5C;
    /* 0x5E */ s16 unk5E;
    /* 0x60 */ s16 unk60;
    /* 0x62 */ s16 unk62;
    /* 0x64 */ s16 unk64;
    /* 0x66 */ s16 unk66;
    /* 0x68 */ s16 unk68;
    /* 0x6A */ u8 pad6A[0x74 - 0x6A];
    /* 0x74 */ s16 unk74;
    /* 0x76 */ u8 pad76[2];
    /* 0x78 */ f32 unk78;
    /* 0x7C */ f32 unk7C;
    /* 0x80 */ f32 unk80;
    /* 0x84 */ f32 unk84;
} Ent88; /* size 0x88 */

#endif
