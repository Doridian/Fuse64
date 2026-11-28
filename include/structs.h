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
    /* 0x3C */ struct UnkModel *unk3C;
    /* 0x40 */ struct UnkSkel *unk40;
    /* 0x44 */ struct UnkInst *unk44;
    /* 0x48 */ UnkStructA_40_48 *unk48;
    /* 0x4C */ struct UnkTexSlots *unk4C;
} UnkStructA_40; /* size 0x50 */

/* A path step: flags (0x80000000 wait, 0xF0000 facing, 0xFFFF mode); a list
 * of them ends at flags 0.  UnkStructA's unk3C (48600 follows it), set by
 * func_80232B04 (the overlays' ZeroJump 0x180E: 13_01, 13_02, 14_01's paths). */
typedef struct Step3C {
    /* 0x00 */ s32 flags;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
} Step3C;

/* An object of the pool D_800A3F00 (128 of them, 0x80 bytes; main_code's
 * 48600 links them in the lists D_802A5578, D_802A55F8 and D_802A5678):
 * what the overlays' tasks and callbacks get, and what the main_code
 * function behind the overlays' ZeroJump stub at 0x80044A20 (ovl_0E_06)
 * returns (func_802443F4, a player's).  unk18/unk1C are 48600's nearest
 * hit's distance (0x7FFFFFFF for none) and unk20/unk24 its object; the
 * overlays' ovl_12_01-04 keep floats in unk18/unk1C. */
typedef struct UnkStructA {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ struct UnkStructA *unk20;
    /* 0x24 */ struct UnkStructA *unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
    /* 0x3C */ struct Step3C *unk3C;
    /* 0x40 */ UnkStructA_40 *unk40;
    /* 0x44 */ struct UnkStructA *unk44;
    /* 0x48 */ s32 unk48;
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
    /* 0x74 */ f32 unk74;
    /* 0x78 */ struct UnkStructA *unk78;
    /* 0x7C */ struct UnkStructA *unk7C;
} UnkStructA; /* size 0x80 */

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

/* A sprite slot (D_802B0260, two of them; A1EA0): what func_80287DC0
 * returns, which the overlays call through ZeroJump 0x1A00 (ovl_17_03's
 * stub at 0x80044690) and pass to 0x1A02/0x1A03.  unk28-unk34 go to its
 * model object's (D_802B0308[unk0], UnkStructA_40) unk10-unk1C. */
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

/* D_802AC610 (54E40.c), what func_8023A840 (ZeroJump 0x0F0B) returns. */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s32 unk48;
    /* 0x4C */ s32 unk4C;
} UnkStruct_802AC610; /* size 0x50 */

/* A camera move: a step of a list linked both ways (4ED50's func_80236658
 * runs it as a task: func_802358D0, ZeroJump 0x170B, starts it); the
 * position's step and its change (unkC, unk18), the look-at point's
 * (unk24, unk30). */
typedef struct UnkStruct_802363BC {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ f32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
    /* 0x3C */ struct UnkStruct_802363BC *unk3C;
    /* 0x40 */ struct UnkStruct_802363BC *unk40;
} UnkStruct_802363BC; /* size 0x44 */

/* A file in func_8027BF38's listing. */
typedef struct {
    /* 0x00 */ s32 used;
    /* 0x04 */ u32 size;
    /* 0x08 */ u8 ext[4];
    /* 0x0C */ u8 name[16];
} PakFile; /* size 0x1C */

/* A controller pak's listing: what func_8027BF38 (ZeroJump 0x1C05) fills in. */
typedef struct {
    /* 0x00 */ s32 freeBytes;
    /* 0x04 */ PakFile files[16];
} PakDir; /* size 0x1C4 */

/* A picture of B11F0's (D_802B03C0[5], what func_80296BEC, ZeroJump 0x3106,
 * returns): its pixels, their width and height, the model object that
 * draws it. */
typedef struct {
    /* 0x000 */ void *unk0;
    /* 0x004 */ s32 unk4;
    /* 0x008 */ s32 unk8;
    /* 0x00C */ UnkStructA_40 *unkC;
    /* 0x010 */ u8 unk10[0xA00];
} UnkStruct_802B03C0; /* size 0xA10 */

/*
 * A word's low half, as 93850 and 949A0 read UnkStruct_802AC610's unk4,
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

/* D_802ACF08 (5E3F0's .bss, one per player): what func_80243D7C returns (the
 * overlays' ZeroJump stub 0F_05 80045110) and 6BBE0 keeps in its records'
 * unk9C. */
typedef struct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 unkE;
} UnkStruct_802ACF08;

#ifdef TARGET_PC
/* the game's objects as the port lays them out where another file's C
   reaches them by offset (port/include/port_types.h) */
#include "port_types.h"
#endif

#endif
