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

/* a model placed in the stage at set-up: the object, its collision handle,
 * its model id, its collision box (unkC-unk12, -1 for none), and its
 * position (in tiles), angles and scale (0C_01..08, 0D_07, 0E_01..05/11..13,
 * 16_01, 17_01/06/07/08/10) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ f32 unk30;
    /* 0x34 */ f32 unk34;
} UnkStruct_ovl0C_01_8004A54C;

/* an object and its position (0C_01/02/06/09, 0E_01/06/07/08/10/14,
 * 12_01..03, 17_09) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
} UnkStruct_ovl0C_01_8004A3BC;

/* a gate (ovl_0C_07's UnkStruct_ovl0C_07_80047590) */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkStructA *unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s32 unk48;
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 unk60;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ s32 unk68;
    /* 0x6C */ s32 unk6C;
    /* 0x70 */ s32 unk70;
    /* 0x74 */ s32 unk74;
} UnkStruct_ovl0C_01_8004A104;

#endif
