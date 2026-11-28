/*
 * ovl_0F_09's game object, IDO -O3; it matches in us, eu and jp.
 * What did it, from 508 words, as in ovl_0F_08/0F_07:
 * - func_ovl0F_09_80043D88 (64 to 0): ovl_0F_08's 80043A28 ported (Vec3f,
 *   `(s32)dx != (s32)vel.x` tests, the pack static 80043CC8 in statics.h
 *   and only on the hit path).
 * - func_ovl0F_09_8004343C (397 to 0): case 0x50's tests both index
 *   D_ovl0F_09_800460F4 ([0..3] and [7..10], the second set past its end),
 *   while what they pass on are separate globals (D_ovl0F_09_80046104..10C,
 *   D_ovl0F_09_80046110..11C, D_ovl0F_09_80046120..128): the tests share
 *   one base in s1 and the rest load by symbol. That freed s1 for
 *   &D_ovl0F_09_80046200 everywhere (case 2 across 80045A30's call, case
 *   20's player back in s2). The loops 8004341C (case 2) and 80043020
 *   (case 1) and the static 80043434 (the `D_ovl0F_09_80046260 = -1;`
 *   store ahead of `lui a0`) are defined in statics.h, and so are case
 *   2's D_ovl0F_09_80046200[2] stores and their `break` in the handler.
 *   func_ovl0F_09_80043200 takes (count, dx, dy, ride): that order of its
 *   register arguments (IDO passes them to the static in s2, $f20, $f22
 *   and s0) gives the original's schedule at its three calls.
 * - func_ovl0F_09_80044528: its padding follows once the rest matches.
 * - Case 2's static 80043390 is defined in another file (statics.h): its
 *   `.file` keeps the next call out of its stores (it replaces a dead
 *   `if (D && D && D) {}` fake; ovl_0F_08's needed neither).
 *
 * - func_ovl0F_09_80043010 (2 to 0): func_ovl0F_09_80043200's loop. The
 *   original tests `slt v1,zero,s2; beqz v1` on entry and `slt v1,s0,s2`
 *   at the end (s0 = i + 1): a compare kept in a variable. The variable c
 *   (`c = i++ < count; while (c) {...}`, with a fake `if (c)` after the
 *   loop to make the entry's test `slt` rather than `blez`) gives the
 *   entry; the end's compare goes to a second variable d (`c = d`), kept
 *   out of v0 by the call's result r, live across it (`if (r) {}`), so
 *   both compares are in v1 (the moves to c in v0 are dead). Without r,
 *   or with `c = d = i++ < count`, the end stays in v0.
 * The `#line N "202690.c"` lines are the physical line numbers (the file
 * named the same whatever path it is compiled by).
 *
 * 2026-11-13: 4 `#line`s that turned out redundant were removed (each tried away
 * against the sha1 in us, eu and jp). Where the notes above name a `#line`,
 * region or renumbering that is gone, it is no longer needed.
 */
#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_0F_09's game object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse). ovl_0F_08 (1FF8F0) is its
 * near twin (all but the handler, func_ovl0F_09_80043200 and the task
 * func_ovl0F_09_80044528 are its code). The statics IDO inlines everywhere
 * are named for the empty stubs they leave.
 */

#define ZJ_0C00 func_ovl0F_09_80044C58
#define ZJ_0D00 func_ovl0F_09_80044C70
#define ZJ_0D05 func_ovl0F_09_80044C98
#define ZJ_0D09 func_ovl0F_09_80044CB8
#define ZJ_0E02 func_ovl0F_09_80044D38
#define ZJ_0E03 func_ovl0F_09_80044D40
#define ZJ_0F00 func_ovl0F_09_80044D50
#define ZJ_0F01 func_ovl0F_09_80044D58
#define ZJ_0F0B func_ovl0F_09_80044DA8
#define ZJ_0F0E func_ovl0F_09_80044DC0
#define ZJ_0F0F func_ovl0F_09_80044DC8
#define ZJ_1000 func_ovl0F_09_80044E08
#define ZJ_1001 func_ovl0F_09_80044E10
#define ZJ_1002 func_ovl0F_09_80044E18
#define ZJ_1003 func_ovl0F_09_80044E20
#define ZJ_1006 func_ovl0F_09_80044E38
#define ZJ_1007 func_ovl0F_09_80044E40
#define ZJ_1008 func_ovl0F_09_80044E48
#define ZJ_1009 func_ovl0F_09_80044E50
#define ZJ_100A func_ovl0F_09_80044E58
#define ZJ_100C func_ovl0F_09_80044E68
#define ZJ_100D func_ovl0F_09_80044E70
#define ZJ_1012 func_ovl0F_09_80044E98
#define ZJ_1013 func_ovl0F_09_80044EA0
#define ZJ_101A func_ovl0F_09_80044ED8
#define ZJ_1503 func_ovl0F_09_80045350
#define ZJ_1704 func_ovl0F_09_80045498
#define ZJ_1708 func_ovl0F_09_800454B8
#define ZJ_1709 func_ovl0F_09_800454C0
#define ZJ_1710 func_ovl0F_09_800454F8
#define ZJ_1711 func_ovl0F_09_80045500
#define ZJ_171B func_ovl0F_09_80045550
#define ZJ_1807 func_ovl0F_09_80045608
#define ZJ_180C func_ovl0F_09_80045630
#define ZJ_180D func_ovl0F_09_80045638
#define ZJ_1811 func_ovl0F_09_80045658
#define ZJ_1812 func_ovl0F_09_80045660
#define ZJ_1814 func_ovl0F_09_80045670
#define ZJ_181F func_ovl0F_09_800456C8
#define ZJ_1821 func_ovl0F_09_800456D8
#define ZJ_1835 func_ovl0F_09_80045778
s32 func_ovl0F_09_800457A8(void (*)(void), s32, PORT_IP(s32), s32, s32); /* 0x1900, kept: its callers pass int (the header's void *) */
#define ZJ_1901 func_ovl0F_09_800457B0
#define ZJ_1902 func_ovl0F_09_800457B8
#define ZJ_1904 func_ovl0F_09_800457C8
PORT_IP(s32) func_ovl0F_09_800457D0(void); /* 0x1905, kept: it keeps the return in PORT_IP(s32) (the header's void *) */
#define ZJ_1909 func_ovl0F_09_800457F0
#define ZJ_1A00 func_ovl0F_09_80045830
#define ZJ_1A02 func_ovl0F_09_80045840
#define ZJ_1E08 func_ovl0F_09_80045A30
#define ZJ_2300 func_ovl0F_09_80045B10
#define ZJ_2600 func_ovl0F_09_80045BF8
#define ZJ_2A00 func_ovl0F_09_80045CB8
#define ZJ_2B00 func_ovl0F_09_80045D30
#define ZJ_2B04 func_ovl0F_09_80045D50
#include "overlays/common/zerojump.h"

void func_ovl0F_09_80043010(void);
void func_ovl0F_09_800432CC(f32);
void func_ovl0F_09_8004343C(s32);
void func_ovl0F_09_80043D88(UnkStructA *);
void func_ovl0F_09_80044408(UnkStructA *);
void func_ovl0F_09_80044528(void);
void func_ovl0F_09_8004460C(void);
s32 func_ovl0F_09_80044810(s32, void *);
void func_ovl0F_09_80044844(f32, f32, f32);
void func_ovl0F_09_800448A0(void);
void func_ovl0F_09_800448B8(void);

static void func_ovl0F_09_80043028(void);
static void func_ovl0F_09_800430C4(void);
static void func_ovl0F_09_800430BC(void);
static void func_ovl0F_09_80043150(s32);
static void func_ovl0F_09_80043200(s32, f32, f32, UnkStructA *);
static void func_ovl0F_09_800432C4(void);
static void func_ovl0F_09_80043314(s32);
static void func_ovl0F_09_80043398(UnkStruct_ovl0F_01_80048264 *);
static s32 func_ovl0F_09_80043CD0(UnkStructA *);
static void func_ovl0F_09_80043CD8(f32);
static void func_ovl0F_09_80043CC8(s32 *, f32, f32);

/* .bss */
UnkStructA *D_ovl0F_09_80046200[7];
u8 D_ovl0F_09_80046220[0x10];
Vec3f D_ovl0F_09_80046230;
s32 D_ovl0F_09_8004623C;
u8 D_ovl0F_09_80046240[0x4];
s32 D_ovl0F_09_80046244;
s32 D_ovl0F_09_80046248;
Vec3f D_ovl0F_09_80046250;
UnkStruct_802B0260 *D_ovl0F_09_8004625C;
s32 D_ovl0F_09_80046260;

/* .data */
f32 D_ovl0F_09_80045F50 = 0.01f;
f32 D_ovl0F_09_80045F54 = 0.1f;
s32 D_ovl0F_09_80045F58 = 0;
s32 D_ovl0F_09_80045F5C = 0;
UnkStructA *D_ovl0F_09_80045F60 = NULL;
s32 D_ovl0F_09_80045F64 = 0;
s32 D_ovl0F_09_80045F68 = 0;
/* passed to func_ovl0F_09_80045B10: three 12-byte records */
u32 D_ovl0F_09_80045F6C[][3] = {
    { 0x25D0001, 0xE, 4 },
    { 0, 0, 4 },
    { 0, 0, 4 },
};
/* four 12-byte records, each passed to func_ovl0F_09_80045CB8 */
u32 D_ovl0F_09_80045F90[][3] = {
    { 0, 0x14, 0xF2 },
    { 1, 0x16, 0xF4 },
    { 2, 0x12, 0xF0 },
    { 3, 0x15, 0xF3 },
};
UnkStruct_ovl0F_01_80048264 D_ovl0F_09_80045FC0[7] = {
    { &D_ovl0F_09_80046200[0], 0x231, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
    { &D_ovl0F_09_80046200[1], 0x232, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0x80000, 0 },
    { &D_ovl0F_09_80046200[2], 0x268, 0, 0.0f, 0.0f, 0.0f, 0.0f, 260.0f, 0.0f, 0, 0 },
    { &D_ovl0F_09_80046200[3], 0x22B, 0, 14.0f, 0.0f, 12.0f, 0.0f, -90.0f, 0.0f, 0, 0 },
    { &D_ovl0F_09_80046200[4], 0x234, 0, 73.0f, 0.0f, 8.0f, 0.0f, -90.0f, 0.0f, 0, 0 },
    { &D_ovl0F_09_80046200[5], 0x235, 0, 73.0f, 0.0f, 8.0f, 0.0f, -90.0f, 0.0f, 0, 0 },
    { &D_ovl0F_09_80046200[6], 0x26C, 0, 41.0f, 0.0f, 18.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
};
/* func_ovl0F_09_8004343C's: two sets of four arguments to test (the handler
 * reads the second through D_ovl0F_09_800460F4[7..10]) and what it passes on */
s32 D_ovl0F_09_800460F4[4] = { 6, 0x12, 6, 0x12 };
s32 D_ovl0F_09_80046104 = 0x5E;
s32 D_ovl0F_09_80046108 = 0x13;
s32 D_ovl0F_09_8004610C = 0;
s32 D_ovl0F_09_80046110 = 0x29;
s32 D_ovl0F_09_80046114 = 0x12;
s32 D_ovl0F_09_80046118 = 0x29;
s32 D_ovl0F_09_8004611C = 0x12;
s32 D_ovl0F_09_80046120 = 0x61;
s32 D_ovl0F_09_80046124 = 0x13;
s32 D_ovl0F_09_80046128 = 0;
UnkStruct_ovl0F_01_80048508 D_ovl0F_09_8004612C[2] = {
    { 0x40, 3, 8.5f, 2, 0, 1, 0 },
    { 0xC0, 3, 8.5f, 3, 0, 1, 0 },
};

#include "statics.h"

/* unused: the stubs IDO -O3 leaves at the end of the object; one of them
 * refers to the .bss nothing else uses, which would otherwise be common */
static void func_ovl0F_09_80044908(void) {
    D_ovl0F_09_80046240[0] = 0;
}

/* Taking the task's address here (as well as in 80043028/800430C4) makes
 * the tasks and callbacks come out at their own turn, after the handler,
 * rather than pulled out ahead of their takers (as in ovl_0F_08). */
static void func_ovl0F_09_80044900(void) {
    func_ovl0F_09_800457A8(func_ovl0F_09_8004460C, 0, 0, 0, 0x130);
}

/* A second caller keeps func_ovl0F_09_80043398 from being inlined into the
 * handler (as in ovl_0F_08). */
static void func_ovl0F_09_800448F8(void) {
    func_ovl0F_09_80043398(D_ovl0F_09_80045FC0);
}

static void func_ovl0F_09_800448F0(void) {
}

static void func_ovl0F_09_800448E8(void) {
}

static void func_ovl0F_09_800448E0(void) {
}

static void func_ovl0F_09_800448D8(void) {
}

static void func_ovl0F_09_800448D0(void) {
}

static void func_ovl0F_09_800448C8(void) {
}

static void func_ovl0F_09_800448C0(void) {
}

void func_ovl0F_09_800448B8(void) {
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl0F_09_800448A0
#define CORE_FUNC_B func_ovl0F_09_80044844
#define CORE_FUNC_C func_ovl0F_09_800432CC
#define CORE_FUNC_D func_ovl0F_09_80044810
#define CORE_BSS_A0 D_ovl0F_09_80046244
#define CORE_BSS_A1 D_ovl0F_09_80046248
#define CORE_BSS_B D_ovl0F_09_80046250
#define CORE_ZJ_1704 func_ovl0F_09_80045498
#define CORE_ZJ_1708 func_ovl0F_09_800454B8
#define CORE_ZJ_1709 func_ovl0F_09_800454C0
#define CORE_ZJ_1904 func_ovl0F_09_800457C8
#define CORE_ZJ_2B04 func_ovl0F_09_80045D50
#include "overlays/common/core.h"




void func_ovl0F_09_8004460C(void) {
    PORT_IP(s32) dir;
    UnkStructA *obj;
    f32 angle;
    f32 step;
    f32 i;

    dir = func_ovl0F_09_800457D0();
    obj = func_ovl0F_09_80044E98(func_ovl0F_09_80044E38());
    D_ovl0F_09_80045F68 = 1;
    angle = 0.0f;
    i = 0.0f;
    func_ovl0F_09_800454C0(NULL, &angle, NULL);
    func_ovl0F_09_80044EA0(1);
    func_ovl0F_09_80044DC8(1);
    func_ovl0F_09_80044DC0(0);
    func_ovl0F_09_80045500(0x7F);
    if (dir == 0) {
        step = -((angle > 180.0f ? 360.0f - angle : angle) / 50.0f);
    } else {
        step = (angle > 180.0f ? 360.0f - angle : angle) / 50.0f;
    }
    step *= 1.1f;
    while (i < 100.0f) {
        func_ovl0F_09_80045498(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18);
        func_ovl0F_09_800454B8(32768.0f, -angle, 32768.0f);
        i += 1.0f;
        angle += step;
        func_ovl0F_09_800457F0();
    }
    func_ovl0F_09_80044DC8(0);
    func_ovl0F_09_80044EA0(0);
    func_ovl0F_09_80044DC0(1);
    func_ovl0F_09_800454F8(0x1F);
    D_ovl0F_09_80045F68 = 0;
    func_ovl0F_09_800457B8();
}

/* a task: turns model 5 for 200 frames, then idles */
void func_ovl0F_09_80044528(void) {
    PORT_IP(s32) speed;
    s32 i;

    i = 0;
    speed = func_ovl0F_09_800457D0();
    func_ovl0F_09_80045350(D_ovl0F_09_80046200[1]->unk40, 0x61);
    func_ovl0F_09_80045658(D_ovl0F_09_80046200[5], 0x80000);
    while (i < 200) {
        D_ovl0F_09_80046200[5]->unk40->unk36 += speed;
        func_ovl0F_09_800457F0();
        i++;
    }
    func_ovl0F_09_80045350(D_ovl0F_09_80046200[1]->unk40, 0x60);
    func_ovl0F_09_80045660(D_ovl0F_09_80046200[5], 0x80000);
    while (1) {
        func_ovl0F_09_800457C8(100);
    }
}

void func_ovl0F_09_80044408(UnkStructA *arg0) {
    f32 dx;
    f32 dy;
    f32 dz;
    s32 i;

    if (arg0->unk8 & 1) {
        func_ovl0F_09_80045500(0x10);
    }
    dx = arg0->unk40->unk28 / 30.0f;
    dy = arg0->unk40->unk2C / 30.0f;
    dz = arg0->unk40->unk30 / 30.0f;
    func_ovl0F_09_80045660(arg0, 0x02000000);
    for (i = 0; i < 30; i++) {
        arg0->unk40->unk28 -= dx;
        arg0->unk40->unk2C -= dy;
        arg0->unk40->unk30 -= dz;
        func_ovl0F_09_800456D8(arg0);
        func_ovl0F_09_800457F0();
    }
    func_ovl0F_09_80045660(arg0, 0x01000000);
}

static void func_ovl0F_09_80043D80(s16 x, s16 z, Vec3f *vel) {
    vel->x = x * 0.002f;
    vel->z = z * 0.002f;
}

void func_ovl0F_09_80043D88(UnkStructA *arg0) {
    Vec3f vel;
    f32 dx;
    f32 dz;
    Vec3f pos;

    dx = arg0->unk40->unk10 - arg0->unk58;
    dz = arg0->unk40->unk18 - arg0->unk60;
    if (arg0->unk8 & 1) {
        D_ovl0F_09_80045F5C = 1;
        D_ovl0F_09_80045F60 = arg0;
    } else {
        D_ovl0F_09_80045F5C = 0;
        D_ovl0F_09_80045F60 = NULL;
    }
    if (arg0->unk68 != 0.0f || arg0->unk6C != 0.0f) {
        arg0->unk64 = 0;
        D_ovl0F_09_80045F58 = 0;
        return;
    }
    pos.x = arg0->unk40->unk10;
    pos.y = arg0->unk40->unk14;
    pos.z = arg0->unk40->unk18;
    func_ovl0F_09_80043D80(arg0->unk64 >> 16, arg0->unk64 & 0xFFFF, &vel);
    if ((s32)dx != (s32)vel.x) {
        arg0->unk40->unk10 = arg0->unk58;
        if ((s32)dx < (s32)vel.x) {
            vel.x -= (dz != 0.0f ? 0.707f : 1.0f) * (dx != 0.0f ? D_ovl0F_09_80045F54 : D_ovl0F_09_80045F50);
            if (vel.x < dx) {
                vel.x = dx;
            }
            func_ovl0F_09_80043CD8(dx);
        } else if ((s32)vel.x < (s32)dx) {
            vel.x += (dz != 0.0f ? 0.707f : 1.0f) * (dx != 0.0f ? D_ovl0F_09_80045F54 : D_ovl0F_09_80045F50);
            if (dx < vel.x) {
                vel.x = dx;
            }
            func_ovl0F_09_80043CD8(dx);
        }
        arg0->unk40->unk10 += vel.x;
    }
    if ((s32)dz != (s32)vel.z) {
        arg0->unk40->unk18 = arg0->unk60;
        if ((s32)dz < (s32)vel.z) {
            vel.z -= (dx != 0.0f ? 0.707f : 1.0f) * (dz != 0.0f ? D_ovl0F_09_80045F54 : D_ovl0F_09_80045F50);
            if (vel.z < dz) {
                vel.z = dz;
            }
            func_ovl0F_09_80043CD8(dz);
        } else if ((s32)vel.z < (s32)dz) {
            vel.z += (dx != 0.0f ? 0.707f : 1.0f) * (dz != 0.0f ? D_ovl0F_09_80045F54 : D_ovl0F_09_80045F50);
            if (dz < vel.z) {
                vel.z = dz;
            }
            func_ovl0F_09_80043CD8(dz);
        }
        arg0->unk40->unk18 += vel.z;
    }
    if (func_ovl0F_09_80043CD0(arg0)) {
        func_ovl0F_09_80043CC8(&arg0->unk64, 0.0f, 0.0f);
        arg0->unk40->unk10 = pos.x;
        arg0->unk40->unk14 = pos.y;
        arg0->unk40->unk18 = pos.z;
    } else {
        arg0->unk64 = ((s16)(vel.x * 500.0f) << 16) | ((s16)(vel.z * 500.0f) & 0xFFFF);
    }
}

#define STATICS_PART 1
#include "overlays/ovl_0F_09/statics.h"
#undef STATICS_PART

static void func_ovl0F_09_800432AC(f32 arg0, f32 arg1) {
    D_ovl0F_09_80045F50 = arg0;
    D_ovl0F_09_80045F54 = arg1;
    D_ovl0F_09_80045F58 = 0;
}

static void func_ovl0F_09_800432B4(void) {
    if (func_ovl0F_09_80044DA8()->unk10 == 0) {
        func_ovl0F_09_800432AC(0.3f, 0.5f);
    } else {
        func_ovl0F_09_800432AC(0.12f, 0.15f);
    }
}

static void func_ovl0F_09_800432BC(void) {
    func_ovl0F_09_80044E68();
    func_ovl0F_09_80044D40(func_ovl0F_09_80044408);
    func_ovl0F_09_800432B4();
    func_ovl0F_09_80044D38(func_ovl0F_09_80043D88, 1);
}

/* ends the task func_ovl0F_09_80044528, if it runs */
static void func_ovl0F_09_800432C4(void) {
    if (D_ovl0F_09_80046260 != -1) {
        func_ovl0F_09_800457B0(D_ovl0F_09_80046260);
        D_ovl0F_09_80046260 = -1;
    }
}

static void func_ovl0F_09_800432A4(void) {
    switch (func_ovl0F_09_80044D58()) {
        case 17:
            func_ovl0F_09_80044E58(D_ovl0F_09_8004612C[0].unk0, D_ovl0F_09_8004612C[0].unk4,
                                   D_ovl0F_09_8004612C[0].unk8, D_ovl0F_09_8004612C[0].unkC,
                                   D_ovl0F_09_8004612C[0].unk10, D_ovl0F_09_8004612C[0].unk14,
                                   D_ovl0F_09_8004612C[0].unk18);
            break;
        case 20:
            func_ovl0F_09_80043200(200, 0.0f, 3.0f, D_ovl0F_09_80046200[6]);
            func_ovl0F_09_80045630(D_ovl0F_09_80046200[6]);
            func_ovl0F_09_80044E50(0xC0);
            func_ovl0F_09_80044E98(func_ovl0F_09_80044E38())->unk40->unk10 += 50.0f;
            func_ovl0F_09_80044E58(D_ovl0F_09_8004612C[1].unk0, D_ovl0F_09_8004612C[1].unk4,
                                   D_ovl0F_09_8004612C[1].unk8, D_ovl0F_09_8004612C[1].unkC,
                                   D_ovl0F_09_8004612C[1].unk10, D_ovl0F_09_8004612C[1].unk14,
                                   D_ovl0F_09_8004612C[1].unk18);
            func_ovl0F_09_80044DC0(1);
            func_ovl0F_09_80044E48(0);
            break;
    }
    func_ovl0F_09_800432C4();
}

static void func_ovl0F_09_800431F8(void) {
    func_ovl0F_09_80044E98(func_ovl0F_09_80044E38())->unk64 = 0;
    D_ovl0F_09_80045F58 = 0;
}

#define STATICS_PART 2
#include "overlays/ovl_0F_09/statics.h"
#undef STATICS_PART

static void func_ovl0F_09_80043018(void) {
    D_ovl0F_09_8004623C = func_ovl0F_09_80044E38();
    func_ovl0F_09_80044E18(&D_ovl0F_09_80046230);
    func_ovl0F_09_80044E40(D_ovl0F_09_8004623C, D_ovl0F_09_80046220);
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl0F_09_8004343C(s32 arg0) {
    UnkStructA *obj;

    switch (arg0) {
        case 2:
            func_ovl0F_09_80043434();
            func_ovl0F_09_8004342C();
            func_ovl0F_09_80043424();
            func_ovl0F_09_8004341C(7, D_ovl0F_09_80045FC0);
            func_ovl0F_09_80043390();
            if (func_ovl0F_09_80045A30(0x405) == 0) {
                func_ovl0F_09_80043314(0x279);
            } else {
                func_ovl0F_09_80043314(0x27A);
            }
#define STATICS_PART 3
#include "overlays/ovl_0F_09/statics.h"
#undef STATICS_PART
        case 17:
            func_ovl0F_09_80044E50(0x40);
            func_ovl0F_09_80044E10(6, 0, 0x12);
            func_ovl0F_09_800432CC(-90.0f);
            break;
        case 19:
            func_ovl0F_09_80044E10(0x29, 0, 0x12);
            break;
        case 20:
            func_ovl0F_09_80044E50(0xC0);
            func_ovl0F_09_80044E10(0x29, 0, 0x12);
            obj = func_ovl0F_09_80044E98(func_ovl0F_09_80044E38());
            func_ovl0F_09_80044DC0(0);
            obj->unk40->unk10 += 50.0f;
            obj->unk40->unk14 -= 600.0f;
            D_ovl0F_09_80046200[6]->unk40->unk14 -= 600.0f;
            D_ovl0F_09_80046200[5]->unk40->unk36 = 50;
            D_ovl0F_09_80046260 = func_ovl0F_09_800457A8(func_ovl0F_09_80044528, 1, 0, 0, 0x110);
            func_ovl0F_09_800432CC(-90.0f);
            break;
        case 4:
            func_ovl0F_09_800432BC();
            break;
        case 5:
            func_ovl0F_09_800432A4();
            break;
        case 0x50:
            if (func_ovl0F_09_80044E20(D_ovl0F_09_800460F4[0], D_ovl0F_09_800460F4[1], D_ovl0F_09_800460F4[2],
                                       D_ovl0F_09_800460F4[3]) != 0) {
                func_ovl0F_09_80044D50(D_ovl0F_09_80046104, D_ovl0F_09_80046108, D_ovl0F_09_8004610C);
            }
            if (func_ovl0F_09_80044E20(PORT_HOST(D_ovl0F_09_800460F4[7], D_ovl0F_09_80046110), PORT_HOST(D_ovl0F_09_800460F4[8], D_ovl0F_09_80046114), PORT_HOST(D_ovl0F_09_800460F4[9], D_ovl0F_09_80046118),
                                       PORT_HOST(D_ovl0F_09_800460F4[10], D_ovl0F_09_8004611C)) != 0) {
                func_ovl0F_09_80044DC0(0);
                func_ovl0F_09_80044E48(1);
                func_ovl0F_09_80044E70(1);
                D_ovl0F_09_80046200[5]->unk40->unk36 = 250;
                D_ovl0F_09_80046260 = func_ovl0F_09_800457A8(func_ovl0F_09_80044528, -1, 0, 0, 0x110);
                func_ovl0F_09_800457C8(10);
                func_ovl0F_09_80043200(26, 2.0f, 0.0f, NULL);
                func_ovl0F_09_80044C98(D_ovl0F_09_80046110, 0, D_ovl0F_09_80046114, D_ovl0F_09_80046118 + 1,
                                       D_ovl0F_09_8004611C, 0x10);
                func_ovl0F_09_80043200(100, 0.0f, -2.5f, D_ovl0F_09_80046200[6]);
                func_ovl0F_09_80044D50(D_ovl0F_09_80046120, D_ovl0F_09_80046124, D_ovl0F_09_80046128);
            }
            break;
        case 0x52:
            func_ovl0F_09_800431F8();
            func_ovl0F_09_80043150(2);
            break;
        case 0x53:
            func_ovl0F_09_80043150(1);
            break;
        case 1:
            func_ovl0F_09_80043020(7, D_ovl0F_09_80045FC0);
            if (D_ovl0F_09_80046260 != -1) {
                func_ovl0F_09_800457B0(D_ovl0F_09_80046260);
                D_ovl0F_09_80046260 = -1;
            }
            break;
        case 12:
            func_ovl0F_09_80044EA0(1);
            break;
        case 3:
            func_ovl0F_09_80043018();
            break;
    }
}

static void func_ovl0F_09_80043398(UnkStruct_ovl0F_01_80048264 *arg0) {
    f32 pos[3];

    func_ovl0F_09_80044CB8(arg0->unkC, arg0->unk10, arg0->unk14, pos, 0);
    (*arg0->unk0)->unk40->unk10 = pos[0];
    (*arg0->unk0)->unk40->unk14 = pos[1];
    (*arg0->unk0)->unk40->unk18 = pos[2];
}

static void func_ovl0F_09_80043314(s32 arg0) {
    D_ovl0F_09_8004625C = func_ovl0F_09_80045830(arg0);
    func_ovl0F_09_80045840(D_ovl0F_09_8004625C, 0x20000000);
    D_ovl0F_09_8004625C->unk40 = 0.6f;
    D_ovl0F_09_8004625C->unk44 = D_ovl0F_09_8004625C->unk40 * 0.75f;
    D_ovl0F_09_8004625C->unk3C = D_ovl0F_09_8004625C->unk4C * 0.3f;
}

/* moves the player (and what it rides, if anything) by (dx, dy) a frame for
 * count frames */
static void func_ovl0F_09_80043200(s32 count, f32 dx, f32 dy, UnkStructA *ride) {
    UnkStructA *obj;
    f32 saved;
    s32 i;
    s32 c;
    s32 r;
    s32 d;

    obj = func_ovl0F_09_80044E98(func_ovl0F_09_80044E38());
    saved = obj->unk70;
    obj->unk70 = 0.0f;
    if (ride != NULL) {
        func_ovl0F_09_80045638(obj, ride);
        func_ovl0F_09_80045658(ride, 0x200000);
    }
    /* fake: c and its use after the loop keep the compares in a register
     * (the original's `slt v1`); r (the call's result, live across the
     * loop's compare) and d move that compare out of v0 */
    i = 0;
    c = i++ < count;
    while (c) {
        obj->unk40->unk10 += dx;
        obj->unk40->unk14 += dy;
        r = func_ovl0F_09_800457F0();
        d = i++ < count;
        if (r) {} /* fake: a dead test, for the register allocation or schedule */
        c = d;
    }
    if (c) {}
    obj->unk70 = saved;
}

static void func_ovl0F_09_80043150(s32 arg0) {
    switch (arg0) {
        case 0:
            if (D_ovl0F_09_80045F64 == 0) {
                func_ovl0F_09_80043028();
            } else {
                func_ovl0F_09_800430C4();
            }
            break;
        case 1:
            if (D_ovl0F_09_80045F64 != 0) {
                func_ovl0F_09_800430C4();
            }
            break;
        case 2:
            if (D_ovl0F_09_80045F64 != 1) {
                func_ovl0F_09_80043028();
            }
            break;
    }
}

static void func_ovl0F_09_800430C4(void) {
    if (D_ovl0F_09_80045F68 == 0) {
        func_ovl0F_09_800457A8(func_ovl0F_09_8004460C, 0, 0, 0, 0x130);
        func_ovl0F_09_800430BC();
    } else {
        func_ovl0F_09_800430BC();
    }
}

static void func_ovl0F_09_800430BC(void) {
    D_ovl0F_09_80045F64 = 0;
    func_ovl0F_09_80045550(70.0f, 45.0f);
}

static void func_ovl0F_09_80043028(void) {
    if (D_ovl0F_09_80045F68 == 0) {
        func_ovl0F_09_800457A8(func_ovl0F_09_8004460C, 1, 0, 0, 0x130);
        D_ovl0F_09_80045F64 = 1;
        func_ovl0F_09_80045550(-50.0f, -60.0f);
    } else {
        D_ovl0F_09_80045F64 = 1;
        func_ovl0F_09_80045550(-50.0f, -60.0f);
    }
}

void func_ovl0F_09_80043010(void) {
}
