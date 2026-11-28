#ifndef STRUCTS_H
#define STRUCTS_H

/*
 * Structures the game's C shares between files.  Nothing about them is
 * understood yet: fields are unk<offset>, and a structure is named only by
 * a letter until the code says more (CLAUDE.md, "Don't guess names").  A
 * field is added where some code is seen to use it; the gaps are padding
 * arrays.
 */

typedef struct {
    f32 x, y, z;
} Vec3f;

/* The structure at UnkStructA's 0x40. */
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
    /* 0x30 */ f32 unk30;
    /* 0x34 */ u8 unk34[2];
    /* 0x36 */ s16 unk36;
#ifdef TARGET_PC
    /* 426E0.c's UnkObj (the pool D_800A0D00's object) has pointers here */
    /* 0x38 */ s32 unk38;
    /* 0x3C */ void *unk3C;
    /* 0x40 */ void *unk40;
    /* 0x44 */ void *unk44;
#else
    /* 0x38 */ u8 unk38[0x10];
#endif
    /* 0x48 */ struct UnkStructA_40_48 *unk48;
#ifdef TARGET_PC
    /* 0x4C */ void *unk4C;
#endif
} UnkStructA_40;

/* The structure at UnkStructA_40's 0x48, and the one at its 0x4. */
typedef struct {
    /* 0x00 */ PORT_RET(u8 unk0[4];, void *unk0;)
    /* 0x04 */ f32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ PORT_RET(u8 unkC[4];, void *unkC;)
    /* 0x10 */ f32 unk10;
    /* 0x14 */ s32 unk14;
} UnkStructA_40_48_4;

typedef struct UnkStructA_40_48 {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ UnkStructA_40_48_4 *unk4;
} UnkStructA_40_48;

/* What the overlays' tasks and callbacks get, and what the main_code
 * function behind the overlays' ZeroJump stub at 0x80044A20 (ovl_0E_06)
 * returns. */
typedef struct {
    /* 0x00 */ u8 unk0[8];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 unk14[4];
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
#ifdef TARGET_PC
    /* 0x20 */ intptr_t unk20;
    /* 0x24 */ intptr_t unk24;
#else
    /* 0x20 */ u8 unk20[8];
#endif
    /* 0x28 */ s32 unk28;
    /* 0x2C */ u8 unk2C[4];
    /* 0x30 */ s32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
#ifdef TARGET_PC
    /* the pointers 48600.c's UnkStructA80 has (the object pool's whole
       object): where pointers are 8 bytes they make room for themselves */
    /* 0x3C */ void *unk3C;
    /* 0x40 */ UnkStructA_40 *unk40;
    /* 0x44 */ void *unk44;
    /* 0x48 */ u8 unk48[4];
#else
    /* 0x3C */ u8 unk3C[4];
    /* 0x40 */ UnkStructA_40 *unk40;
    /* 0x44 */ u8 unk44[8];
#endif
    /* 0x4C */ f32 unk4C;
    /* 0x50 */ f32 unk50;
    /* 0x54 */ f32 unk54;
    /* 0x58 */ f32 unk58;
    /* 0x5C */ f32 unk5C;
    /* 0x60 */ f32 unk60;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ f32 unk68;
    /* 0x6C */ f32 unk6C;
    /* 0x70 */ f32 unk70;
#ifdef TARGET_PC
    /* 0x74 */ f32 unk74;
    /* 0x78 */ void *unk78;
    /* 0x7C */ void *unk7C;
#endif
} UnkStructA;

/* What the main_code function behind the overlays' ZeroJump stub at
 * 0x80043BE8 (ovl_17_04) returns. */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4[4];
    /* 0x14 */ u8 unk14[0x10];
    /* 0x24 */ s32 unk24;
    /* 0x28 */ u8 unk28[4];
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ u8 unk34[4];
    /* 0x38 */ s32 unk38;
    /* 0x3C */ u8 unk3C[4];
    /* 0x40 */ s32 unk40[4];
    /* 0x50 */ s32 unk50[4][4];
    /* 0x90 */ s32 unk90[4];
    /* 0xA0 */ s32 unkA0;
    /* 0xA4 */ s32 unkA4[4];
    /* 0xB4 */ u8 unkB4[0x10];
    /* 0xC4 */ u8 unkC4[4][9];
} UnkStructB;

/* What the main_code function behind the overlays' ZeroJump stub at
 * 0x80044690 (ovl_17_03) returns. */
typedef struct {
    /* 0x00 */ u8 unk0[0x18];
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ u8 unk20[0x1C];
    /* 0x3C */ f32 unk3C;
    /* 0x40 */ f32 unk40;
    /* 0x44 */ f32 unk44;
    /* 0x48 */ u8 unk48[4];
    /* 0x4C */ f32 unk4C;
} UnkStructC;

/* What func_8023A840 returns. */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 unk14[0x4];
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
} UnkStruct_8023A840;

/*
 * A word's low half, as 93850 and 949A0 read UnkStruct_8023A840's unk4,
 * unk8 and unkC: the halfword at +2 on the N64; the port takes the whole
 * word, its own order's (they are small counts).
 */
#ifdef TARGET_PC
#define S16_LO(w) (w)
#else
#define S16_LO(w) (*(s16 *)((u8 *)&(w) + 2))
#endif

#ifdef TARGET_PC
/* the game's objects as the port lays them out where another file's C
   reaches them by offset (port/include/port_types.h) */
#include "port_types.h"
#endif

#endif
