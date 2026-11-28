#ifndef GRID_H
#define GRID_H

/* The grid: func_8026DCAC's cell, and the tables of rows of cells. */

#include "common.h"

/* func_8026DCAC's result: the grid cell and the position within it */
typedef struct {
    /* 0x00 */ s32 i[3];
    /* 0x0C */ f32 f[3];
} Unk8026DCAC;

/* A row of cells (D_800B15D8's and D_800B5F20's). */
typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u16 unk2[64];
} UnkCell_800B15D8; /* size 0x82 */

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s8 unkC;
    /* 0x0D */ u8 unkD;
    /* 0x0E */ u8 unkE;
    /* 0x0F */ u8 unkF;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
    /* 0x12 */ UnkCell_800B15D8 unk12[23];
    /* 0xBC0 */ u8 unkBC0[0x74];
} UnkStruct_800B15D8; /* size 0xC34 */

/* 99F40.c's blocks of cells, which follow D_800B15D8 on the N64 */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ u8 *unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ UnkCell_800B15D8 unk10[24];
} UnkStruct_800B5F20; /* size 0xC40 */

#endif
