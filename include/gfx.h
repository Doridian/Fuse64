#ifndef GFX_H
#define GFX_H

/* A frame's buffers and what they hold: display list commands and matrices (main_code's 40040, 412F0, 41FD0, 426E0). */

#include "common.h"

/* A display list command, two words (gbi.h's Gfx, not in include/ yet). */
typedef struct {
    PORT_UP(u32) w0;
    PORT_UP(u32) w1;
} Gfx_80225840;

/* gbi.h's Mtx (8-aligned, as D_802A53D8's place after a u16 shows) */
typedef union {
    /* 0x00 */ s32 m[16];
    long long force_structure_alignment;
} UnkStruct_802A53D8; /* size 0x40 */

/* one frame's buffers (D_8014C8D0[2]): matrices, the areas func_80227678
 * hands out, the display list */
typedef struct {
    /* 0x00000 */ UnkStruct_802A53D8 mtx[0x400];
    /* 0x10000 */ u8 unk10000[4][PORT_GFX_BYTES(0x5000)];
    /* 0x24000 */ Gfx_80225840 gfx[0x600];
} UnkStruct_802A538C; /* size 0x27000 */

#endif
