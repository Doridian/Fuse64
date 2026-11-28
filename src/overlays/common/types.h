#ifndef OVERLAYS_COMMON_TYPES_H
#define OVERLAYS_COMMON_TYPES_H

/*
 * The structures several overlays share (docs/DECOMPILING.md, "Types"):
 * each overlay had its own copy, one layout, used the same way (the same
 * shared code, the same object at the same place in each).  Each keeps the
 * name of the first overlay that has it (its address there), as the shared
 * code's headers here do; the comment says which others use it.
 */

/* the handler's state, one in .bss (0C_01..10, 0E_01..15, 12_01..05) */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ s32 unk10;
} UnkStruct_ovl0C_01_8004A9C8;

#endif
