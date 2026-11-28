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

/* a gate: its object, its position, the trigger object's position and the
 * arguments of its map changes (0C_07's task 80044B48 takes one; 0C_01/02/
 * 06/07) */
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

/* objects at a position, each a task's (func_ovl0C_01_80047A60,
 * func_ovl0C_04_80043FC0; 0C_01/04) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ s32 unk28;
} UnkStruct_ovl0C_01_8004A17C;

/* a chain of records (unk2 is the next one's index, 0 ends it), each a
 * task's (func_ovl0C_04_80043D3C; 0C_01/04) */
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ UnkStructA *unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
} UnkStruct_ovl0C_01_8004A1A8;

/* a switch: a list of these ends at the one whose unk2 is 0; unk20 and
 * unk22 count, unk24 is its flag (0C_06: the timer of its sparks); the
 * third parameter of 0C_01's 80044878 and its copies (0C_01/02/06) */
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ UnkStructA *unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ s16 unk20;
    /* 0x22 */ s16 unk22;
    /* 0x24 */ s32 unk24;
} UnkStruct_ovl0C_01_8004A22C;

/* D_ovl0C_01_8004A3CC's records (0C_01/02) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s16 unk2C;
    /* 0x2E */ s16 unk2E;
} UnkStruct_ovl0C_01_8004A3CC;

/* D_ovl0C_01_8004A584's records (0C_01/02) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
} UnkStruct_ovl0C_01_8004A584;

/* an object at a position, with three arguments (0C_01/02) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} UnkStruct_ovl0C_01_8004A5B4;

/* a lift, each the task func_ovl0C_07_80045694's (0C_06/07/09) */
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ UnkStructA *unk8;
    /* 0x0C */ UnkStructA *unkC;
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
    /* 0x38 */ s16 unk38;
    /* 0x3A */ s16 unk3A;
    /* 0x3C */ s16 unk3C;
    /* 0x3E */ s16 unk3E;
    /* 0x40 */ s16 unk40;
    /* 0x42 */ s16 unk42;
    /* 0x44 */ s32 unk44;
} UnkStruct_ovl0C_06_80048F88;

/* an object, its position and the limits the movers check it against
 * (0C_06/07/09) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
} UnkStruct_ovl0C_06_80048FD0;

/* a mover: the tasks func_ovl0C_07_80044CE0 and func_ovl0C_07_80044DC8
 * take one (unk0 indexes the overlay's UnkStruct_ovl0C_06_80048FD0s;
 * 0C_06/07/09) */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkStructA *unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ f32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
    /* 0x3C */ f32 unk3C;
    /* 0x40 */ f32 unk40;
} UnkStruct_ovl0C_06_80049030;

/* a mover: each record is passed to its task (0E_01..05) */
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ UnkStructA *unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
} UnkStruct_ovl0E_01_800469F8;

/* what replaces a placed model (UnkStruct_ovl0C_01_8004A54C) once its flag
 * (unk0) is set (0E_01/03/12) */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 unk1E;
} UnkStruct_ovl0E_01_80046B70;

/* a mover with two sets of speeds, chosen by func_ovl0E_02_800451F8()->unk10
 * and its copies (0E_02..05) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ f32 unk30;
    /* 0x34 */ f32 unk34;
} UnkStruct_ovl0E_02_8004648C;

/* a mover: each record is passed to its task (0E_09/13) */
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ UnkStructA *unk8;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
} UnkStruct_ovl0E_09_80045DCC;

/* a mover's path (0E_09: ten points, 0E_13: fifteen) */
typedef struct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
} UnkStruct_ovl0E_09_80045E0C;

/* a mover: each record is passed to its task (0E_11/12) */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkStructA *unk4;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
} UnkStruct_ovl0E_11_800463AC;

/* what func_ovl0F_01_80047508 and its copies return: offsets into itself
 * (0F_01..04, 17_09) */
typedef struct {
    /* 0x00 */ u8 unk0[0x14];
    /* 0x14 */ s32 unk14;
    /* 0x18 */ u8 unk18[8];
    /* 0x20 */ s32 unk20;
} UnkStruct_ovl0F_01_80048130;

/* a mover (D_ovl0F_01_800481D4): direction, speed, acceleration, route
 * (0F_01/02/04) */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
    /* 0x12 */ u16 unk12;
    /* 0x14 */ s32 unk14;
} UnkStruct_ovl0F_01_800481D4; /* size 0x18 */

/* a model the stage places (0F_01..05, 0F_07..10) */
typedef struct {
    /* 0x00 */ UnkStructA **unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
} UnkStruct_ovl0F_01_80048264; /* size 0x2C */

/* seven arguments of func_ovl0F_01_80047038 and its copies (0F_01..04,
 * 0F_07..10) */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} UnkStruct_ovl0F_01_80048508; /* size 0x1C */

/* one of the particles func_ovl0F_01_8004540C and its copies draw
 * (D_ovl0F_01_80048798; 0F_01..04, 17_09) */
typedef struct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ s32 unk10;
} UnkStruct_ovl0F_01_80048798; /* size 0x14 */

/* the movers' script (D_ovl0F_01_80048938; 0F_01/04) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ UnkStructA **unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ u16 (*unk18)[2];
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ void (*unk20)(s32, s32);
} UnkStruct_ovl0F_01_80048938; /* size 0x24 */

/* what func_ovl10_01_80046830 and its copies return (10_01..05) */
typedef struct {
    /* 0x00 */ u8 unk0[8];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ u8 unkC[4];
    /* 0x10 */ s32 unk10;
} UnkStruct_ovl10_01_80046830;

/* what func_ovl10_13_80049728 and func_ovl12_05_800497F8 return */
typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 unk14[0x14];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
} UnkStruct_ovl10_13_80049728;

/* what func_ovl10_13_8004A770 and func_ovl12_05_8004A840 return: a player */
typedef struct {
    /* 0x000 */ s32 unk0;
    /* 0x004 */ PORT_RET(u8 unk4[0x18];, u8 unk4[0x14]; void *unk18;) /* (6BBE0.c's D_800BCFE8[i], Port_800BCFE8: pointers at 0x18, 0x1C, 0x98, 0x9C) */
    /* 0x01C */ UnkStructA *unk1C;
    /* 0x020 */ u8 unk20[0xC];
    /* 0x02C */ Vec3f unk2C;
    /* 0x038 */ u8 unk38[0x50];
    /* 0x088 */ s32 unk88;
    /* 0x08C */ u8 unk8C[4];
    /* 0x090 */ s32 unk90;
    /* 0x094 */ PORT_RET(u8 unk94[0xC];, u8 unk94[4]; void *unk98; void *unk9C;)
    /* 0x0A0 */ union {
        s32 w;
        struct {
            PORT_HALVES(u16 hi, u16 lo)
            /* (the task id's halves in the host's order) */
        } h;
    } unkA0;
    /* 0x0A4 */ u8 unkA4[0x80];
    /* 0x124 */ s32 unk124;
} UnkStruct_ovl10_13_8004A770;

/* an object, its position and an id (D_ovl12_01_80046318; 12_01/03) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ s32 unk10;
} UnkStruct_ovl12_01_80046318;

/* the argument of func_ovl17_01_80043CB4's tasks and its copies' (17_01/06/
 * 08/10) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0C */ f32 unkC;
} UnkStruct_ovl17_01_80045858; /* size 0x10 */

/* D_ovl17_02_80044D54 (17_02/03) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
} UnkStruct_ovl17_02_80044D54;

/* D_ovl17_04_80044D84's records (17_04/05) */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
} UnkStruct_ovl17_04_80044D84;

/* a crumbling block (func_ovl0C_06_8004696C's task): a list of these ends at
 * the one whose unk2 is 0; unk20 points into a list of switches; the second
 * parameter of 0C_01's 80044878 and its copies (0C_01/02/06) */
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ UnkStructA *unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ UnkStruct_ovl0C_01_8004A22C *unk20;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ s16 unk26;
} UnkStruct_ovl0C_01_8004A2A4;

/* the door func_ovl0F_02_80045024 and its copies run as a task: two models
 * and what to call once it is open (0F_02/03/04) */
typedef struct {
    /* 0x00 */ UnkStruct_ovl0F_01_80048264 unk0[2];
    /* 0x58 */ void (*unk58)(void);
    /* 0x5C */ void (*unk5C)(void);
} UnkStruct_ovl0F_02_800489F4; /* size 0x60 */

/* how func_ovl0B_04_80043050 and func_ovl0B_06_80043040 make an object
 * (through ZeroJump 0x1500): its place, model id and flags (D_ovl0B_04_80049C38,
 * D_ovl0B_06_8004A0C4; 0B_04/06) */
typedef struct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
} UnkStruct_ovl0B_04_80049C38; /* size = 0x18 */

/* a model at a place, and its object (what func_ovl0B_03_80047E98 makes;
 * D_ovl0B_02_80049A50; 0B_02/03) */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ UnkStructA_40 *unk10;
} UnkStruct_ovl0B_02_80049A50;

#endif
