#ifndef ENT_H
#define ENT_H

/* The entities of main_code's 87D80 and 8A7A0 (D_8008F438's Ent88, the objects they draw, their controllers) and the players they belong to (D_800AEDF0 seen as Player87). */

#include "common.h"

typedef struct Node87 {
    /* 0x00 */ struct Node87 *next;
    /* 0x04 */ s32 unk4;
} Node87;

typedef struct {
    /* 0x00 */ u8 pad0[8];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ PORT_RET(u8 pad14[0x30 - 0x14];, u8 pad14[0xC]; void *unk20; void *unk24; u8 unk28[0x8];)
    /* 0x30 */ s32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
    /* 0x3C */ PORT_RET(u8 pad3C[4];, void *unk3C;)
    /* 0x40 */ UnkStructA_40 *unk40;
    /* 0x44 */ PORT_RET(u8 pad44[0x4C - 0x44];, void *unk44; u8 unk48[0x4];)
    /* 0x4C */ f32 unk4C;
    /* 0x50 */ f32 unk50;
    /* 0x54 */ f32 unk54;
    /* 0x58 */ f32 unk58;
    /* 0x5C */ f32 unk5C;
    /* 0x60 */ f32 unk60;
    /* 0x64 */ u8 pad64[4];
    /* 0x68 */ f32 unk68;
    /* 0x6C */ f32 unk6C;
    /* 0x70 */ f32 unk70;
} Obj8E;

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
    /* 0x04 */ Obj8E *obj;
    /* 0x08 */ Obj8E *unk8;
    /* 0x0C */ Obj8E *unkC;
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
    /* 0x1C */ Obj8E *obj;
    /* 0x20 */ UnkStructA_40 *xf;
    /* 0x24 */ void *unk24[3];
    /* 0x30 */ Obj8E *unk30;
    /* 0x34 */ Obj8E *unk34;
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

typedef struct {
    /* 0x00 */ u8 pad0[0x18];
    /* 0x18 */ s32 unk18;
#ifdef TARGET_PC
    /* the bomb count, which func_80277E1C stores as an s32 (Player8E) and
       IDO's code reads the low half of: the word in the host's order */
    /* 0x1C */ s32 unk1E;
#else
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 unk1E;
#endif
    /* 0x20 */ s32 unk20;
    /* 0x24 */ u8 pad24[4];
#ifdef TARGET_PC
    /* (D_800AEDF0's pointers as wide as the host's: 5E3F0.c's
       UnkStruct_800AEDF0) */
    /* 0x28 */ intptr_t unk28;
    /* 0x2C */ intptr_t unk2C;
    /* 0x30 */ intptr_t unk30;
    /* 0x34 */ u8 pad34[0x40 - 0x34];
    /* 0x40 */ intptr_t unk40;
    /* 0x44 */ u8 pad44[0x4C - 0x44];
    /* 0x4C */ intptr_t unk4C;
    /* 0x50 */ u8 pad50[0xB0 - 0x50];
    /* 0xB0 */ intptr_t unkB0;
    /* 0xB4 */ intptr_t unkB4;
    /* 0xB8 */ u8 padB8[0xC8 - 0xB8];
#else
    /* 0x28 */ s32 unk28;
    /* 0x2C */ u8 pad2C[0xC8 - 0x2C];
#endif
} Player87; /* size 0xC8 */

#endif
