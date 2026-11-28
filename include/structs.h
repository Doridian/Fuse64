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

/* An object's animation slot: the animation playing, its time and flags
 * (main_code's 426E0 keeps an array of them per model object). */
typedef struct {
    /* 0x00 */ struct UnkAnim *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ s32 unk8;
} UnkStructA_40_48_4; /* size 0xC */

/* The structure at UnkStructA_40's 0x48: its animation slots. */
typedef struct UnkStructA_40_48 {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkStructA_40_48_4 *unk4;
} UnkStructA_40_48;

/* The structure at UnkStructA's 0x40: a model object, of the pool
 * D_800A0D00 (0xA0 of them, linked by priority; main_code's 426E0).  Also
 * what func_8022A6C4 makes for a text slot (9A5F0, AFBF0). unk10-unk30 are
 * its place, rotation and scale (48600's collision shapes). */
typedef struct UnkStructA_40 {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
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
    /* 0x34 */ s16 unk34;
    /* 0x36 */ s16 unk36;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ void *unk3C;
    /* 0x40 */ struct UnkSkel *unk40;
    /* 0x44 */ struct UnkInst *unk44;
    /* 0x48 */ UnkStructA_40_48 *unk48;
    /* 0x4C */ struct UnkTexSlots *unk4C;
} UnkStructA_40; /* size 0x50 */

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

/* What func_8023A834 returns (per controller pak, four values and a name);
 * the overlays call it through ZeroJump id 0xF11 (ovl_17_04's stub at
 * 0x80043BE8). */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4[4];
    /* 0x14 */ u8 unk14[0x10];
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
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
} UnkStruct_8023A834;

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

/* An open file: 8 of them at D_8014B808. */
typedef struct UnkStruct_80225CC0 {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ s32 unk10;
} UnkStruct_80225CC0; /* size 0x14 */

/* A menu cursor position and the entries up, down, left and right of it
 * (995C0's func_8027F658). */
typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 unk2;
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 unk5;
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 unk7;
} UnkStruct_8027F0C4; /* size 0x8 */

/* A screen rectangle (0x40 bytes from func_8022A6C4). */
typedef struct {
    /* 0x00 */ u8 pad0[0x10];
    /* 0x10 */ f32 x0;
    /* 0x14 */ f32 y0;
    /* 0x18 */ f32 x1;
    /* 0x1C */ f32 y1;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
} Rect40;

/* An effect: a task (D_802A1C40[kind]) drawing one model (16 of them, D_800BC528). */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ PORT_IP(s32) unk1C;
    /* 0x20 */ UnkStructA *unk20;
} UnkStruct_800BC528; /* size 0x24 */

/* An object moved by the D_802A1D08 functions (16 of them, D_800BC770). */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkStructA *unk4;
    /* 0x08 */ UnkStructA *unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
} UnkStruct_800BC770; /* size 0x28 */

/* a message queue (os.h's OSMesgQueue) */
typedef struct {
    /* 0x00 */ void *unk0;
    /* 0x04 */ void *unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ void *unk14;
} UnkStruct_802A5330; /* size 0x18 */

#ifdef TARGET_PC
/* the game's objects as the port lays them out where another file's C
   reaches them by offset (port/include/port_types.h) */
#include "port_types.h"
#endif

#endif
