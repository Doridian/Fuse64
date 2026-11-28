/*
 * ovl_0F_05's game object, IDO -O3; it matches in us, eu and jp.
 *
 * From 619 words, as in ovl_0F_08/0F_07:
 * - func_ovl0F_05_800440AC: ovl_0F_08's 80043A28 ported (Vec3f,
 *   `(s32)dx != (s32)vel.x` tests, the pack static 80043FE4 in statics.h
 *   and only on the hit path).
 * - func_ovl0F_05_800430BC: the loops 800430A4 (case 2) and 80043018 (case
 *   1) are defined in statics.h; 800430B4 declares i first; the 80045A30
 *   block is in statics.h, back to "1F7CA0.c" before the D_[2] stores.
 *   80045870 is declared s32, which puts case 1's count in v1.
 * - func_ovl0F_05_80043B74: the 180-frame loop's dead `angle =
 *   D_ovl0F_05_80046550.y; ANGLE_SUB(angle, 1/3)` (ovl_12_06's twin turns
 *   the camera there) is register-allocated and then dropped: it leaves the
 *   original's &D_ovl0F_05_80046550 in s0, 1/3 in $f28 and the saved
 *   $f20-$f31. The wait is `while`, not `do`; objs' unk14 = 0 (an int: its
 *   0.0f is a new $f22 rather than the $f28 kept from the start); the
 *   locals are declared cam, i, task, j, k, angle, objs (every one gets a
 *   slot: task at 0xB4, objs at 0x98).
 *
 * - The stub 8004409C (from 42 words to 3, 2026-11-03): an empty static
 *   defined K&R-style, `static void f() {}`, called at the end of 800440A4.
 *   Inlined, a `(void)` empty static costs its caller 8 bytes of frame
 *   (800440AC was 120, not 112); the K&R one costs none, and its stub lands
 *   right before 800440A4's.  (Before: never inlined, kept by an `if (0)`
 *   address in 80044AF4, it came out among the globals after 80043B74.)
 * - `cam->unkE = 0x4000; cam->unkC = 0x4000;` (3 words to 0): the original
 *   holds 0x4000 in v1 (uopt merged the two constants).  That takes unkC and
 *   unkE of one type (u16: with s16/u16 the constants differ, and the
 *   chained `a = b = 0x4000` gives ugen's t5) and two stores, which put it
 *   in v0; 800459C8 declared s32 (the call before) moves it to v1.
 *
 * The `#line N "1F7CA0.c"` lines are the physical line numbers (the file
 * named the same whatever path it is compiled by).
 *
 * 2026-11-13: 5 `#line`s that turned out redundant were removed (each tried away
 * against the sha1 in us, eu and jp). Where the notes above name a `#line`,
 * region or renumbering that is gone, it is no longer needed.
 */
#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_0F_05's game object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse). ovl_0F_08 (1FF8F0) shares
 * its callback (func_ovl0F_05_800440AC) and the small functions at the end;
 * the handler and the three tasks are its own.
 */

/* as ovl_12_06's */
#define ANGLE_SUB(a, d) \
    (a) = ((a) - (d) >= 360.0f) ? (a) - (d) - 360.0f : ((a) < (d)) ? (a) - (d) + 360.0f : (a) - (d)

#define ZJ_0C00 func_ovl0F_05_80044E58
#define ZJ_0D00 func_ovl0F_05_80044E70
#define ZJ_0D09 func_ovl0F_05_80044EB8
#define ZJ_0E02 func_ovl0F_05_80044F38
#define ZJ_0F00 func_ovl0F_05_80044F50
#define ZJ_0F01 func_ovl0F_05_80044F58
#define ZJ_0F0E func_ovl0F_05_80044FC0
#define ZJ_0F0F func_ovl0F_05_80044FC8
#define ZJ_1000 func_ovl0F_05_80045008
#define ZJ_1001 func_ovl0F_05_80045010
#define ZJ_1006 func_ovl0F_05_80045038
#define ZJ_1008 func_ovl0F_05_80045048
#define ZJ_1012 func_ovl0F_05_80045098
#define ZJ_101A func_ovl0F_05_800450D8
#define ZJ_101B func_ovl0F_05_800450E0
#define ZJ_1021 func_ovl0F_05_80045110
#define ZJ_1500 func_ovl0F_05_80045538
#define ZJ_1501 func_ovl0F_05_80045540
#define ZJ_1503 func_ovl0F_05_80045550
#define ZJ_1704 func_ovl0F_05_80045698
#define ZJ_1706 func_ovl0F_05_800456A8
#define ZJ_1708 func_ovl0F_05_800456B8
#define ZJ_1709 func_ovl0F_05_800456C0
#define ZJ_1711 func_ovl0F_05_80045700
#define ZJ_1728 func_ovl0F_05_800457B8
#define ZJ_1729 func_ovl0F_05_800457C0
#define ZJ_1807 func_ovl0F_05_80045808
#define ZJ_1811 func_ovl0F_05_80045858
#define ZJ_1812 func_ovl0F_05_80045860
#define ZJ_1814 func_ovl0F_05_80045870
#define ZJ_181F func_ovl0F_05_800458C8
#define ZJ_1835 func_ovl0F_05_80045978
#define ZJ_1900 func_ovl0F_05_800459A8
#define ZJ_1901 func_ovl0F_05_800459B0
#define ZJ_1902 func_ovl0F_05_800459B8
#define ZJ_1904 func_ovl0F_05_800459C8
#define ZJ_1909 func_ovl0F_05_800459F0
#define ZJ_1A00 func_ovl0F_05_80045A30
#define ZJ_1A02 func_ovl0F_05_80045A40
#define ZJ_1D01 func_ovl0F_05_80045B58
#define ZJ_2004 func_ovl0F_05_80045CA8
#define ZJ_2300 func_ovl0F_05_80045D10
#define ZJ_2600 func_ovl0F_05_80045DF8
#define ZJ_2601 func_ovl0F_05_80045E00
#define ZJ_2602 func_ovl0F_05_80045E08
#define ZJ_2A00 func_ovl0F_05_80045EB8
#define ZJ_2A04 func_ovl0F_05_80045ED8
#define ZJ_2B00 func_ovl0F_05_80045F30
#define ZJ_2B04 func_ovl0F_05_80045F50
#define ZJ_3000 func_ovl0F_05_800460A8
#define ZJ_3001 func_ovl0F_05_800460B0
#include "overlays/common/zerojump.h"

void func_ovl0F_05_80043010(void);
void func_ovl0F_05_800430BC(s32);
void func_ovl0F_05_8004373C(void);
void func_ovl0F_05_80043B74(void);
void func_ovl0F_05_800440AC(UnkStructA *);
void func_ovl0F_05_8004472C(void);
s32 func_ovl0F_05_80044994(s32, void *);
void func_ovl0F_05_800449C8(f32);
void func_ovl0F_05_80044A10(f32, f32, f32);
void func_ovl0F_05_80044A6C(void);
void func_ovl0F_05_80044A84(void);

static void func_ovl0F_05_80043020(UnkStruct_ovl0F_01_80048264 *);
static void func_ovl0F_05_8004409C();
static void func_ovl0F_05_80043FF4(f32);
static s32 func_ovl0F_05_80043FEC(UnkStructA *);
static void func_ovl0F_05_80043FE4(s32 *, f32, f32);

/* .bss */
UnkStructA *D_ovl0F_05_800464E0[12];
u8 D_ovl0F_05_80046510[0x10];
Vec3f D_ovl0F_05_80046520;
s32 D_ovl0F_05_8004652C;
u8 D_ovl0F_05_80046530[0x4];
s32 D_ovl0F_05_80046534;
s32 D_ovl0F_05_80046538;
Vec3f D_ovl0F_05_80046540;
UnkStruct_802B0260 *D_ovl0F_05_8004654C;
Vec3f D_ovl0F_05_80046550;
s32 D_ovl0F_05_8004655C;

/* .data */
f32 D_ovl0F_05_80046150 = 0.01f;
f32 D_ovl0F_05_80046154 = 0.1f;
s32 D_ovl0F_05_80046158 = 0;
s32 D_ovl0F_05_8004615C = 0;
UnkStructA *D_ovl0F_05_80046160 = NULL;
f32 D_ovl0F_05_80046164 = 0.0f;
f32 D_ovl0F_05_80046168 = 0.0f;
f32 D_ovl0F_05_8004616C = 0.0f;
UnkStructA *D_ovl0F_05_80046170 = NULL;
f32 D_ovl0F_05_80046174 = 0.0f;
f32 D_ovl0F_05_80046178 = 0.0f;
f32 D_ovl0F_05_8004617C = 0.0f;
s32 D_ovl0F_05_80046180 = -1;
f32 D_ovl0F_05_80046184 = 0.0f;
s32 D_ovl0F_05_80046188 = 0;
/* passed to func_ovl0F_05_80045D10: three 12-byte records */
u32 D_ovl0F_05_8004618C[][3] = {
    { 0x25D0001, 0xE, 4 },
    { 2, 0, 4 },
    { 2, 0, 4 },
};
/* four 12-byte records, each passed to func_ovl0F_05_80045EB8 */
u32 D_ovl0F_05_800461B0[][3] = {
    { 0, 0x14, 0xF2 },
    { 1, 0x16, 0xF4 },
    { 2, 0x12, 0xF0 },
    { 3, 0x15, 0xF3 },
};
UnkStruct_ovl0F_01_80048264 D_ovl0F_05_800461E0[12] = {
    { &D_ovl0F_05_800464E0[0], 0x22B, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[1], 0x261, 0, 21.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[2], 0x268, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[3], 0x262, 0, 22.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[4], 0x22E, 0, 58.0f, 0.0f, -33.0f, 0.0f, -90.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[5], 0x22F, 0, 58.0f, 0.0f, -33.0f, 0.0f, -90.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[6], 0x231, 0, -12.0f, 0.0f, 14.0f, 0.0f, 90.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[7], 0x232, 0, -12.0f, 0.0f, 14.0f, 0.0f, 90.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[8], 0x234, 0, -5.0f, 0.0f, -58.0f, 0.0f, 0.0f, 0.0f, 0x80000, 0 },
    { &D_ovl0F_05_800464E0[9], 0x233, 0, -5.0f, 0.0f, -58.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[10], 0x235, 0, -4.0f, 0.0f, -58.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
    { &D_ovl0F_05_800464E0[11], 0x22C, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
};
s32 D_ovl0F_05_800463F0 = 0;
u8 D_ovl0F_05_800463F4[0xC] = { 0 }; /* nothing uses it */

/* unused: the stubs IDO -O3 leaves at the end of the object; one of them
 * refers to the .bss nothing else uses, which would otherwise be common */
static void func_ovl0F_05_80044AFC(void) {
    D_ovl0F_05_80046530[0] = 0;
    D_ovl0F_05_80046510[0] = D_ovl0F_05_80046520.x + D_ovl0F_05_8004652C;
}

/* A second caller keeps func_ovl0F_05_80043020 from being inlined into the
 * handler (it is a call there, with its argument in s0). */
static void func_ovl0F_05_80044AF4(void) {
    func_ovl0F_05_80043020(D_ovl0F_05_800461E0);
}

static void func_ovl0F_05_80044AEC(void) {
}

static void func_ovl0F_05_80044AE4(void) {
}

static void func_ovl0F_05_80044ADC(void) {
}

static void func_ovl0F_05_80044AD4(void) {
}

static void func_ovl0F_05_80044ACC(void) {
}

static void func_ovl0F_05_80044AC4(void) {
}

static void func_ovl0F_05_80044ABC(void) {
}

static void func_ovl0F_05_80044AB4(void) {
}

static void func_ovl0F_05_80044AAC(void) {
}

static void func_ovl0F_05_80044AA4(void) {
}

static void func_ovl0F_05_80044A9C(void) {
}

static void func_ovl0F_05_80044A94(void) {
}

static void func_ovl0F_05_80044A8C(void) {
}

void func_ovl0F_05_80044A84(void) {
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl0F_05_80044A6C
#define CORE_FUNC_B func_ovl0F_05_80044A10
#define CORE_FUNC_C func_ovl0F_05_800449C8
#define CORE_FUNC_D func_ovl0F_05_80044994
#define CORE_BSS_A0 D_ovl0F_05_80046534
#define CORE_BSS_A1 D_ovl0F_05_80046538
#define CORE_BSS_B D_ovl0F_05_80046540
#define CORE_ZJ_1704 func_ovl0F_05_80045698
#define CORE_ZJ_1708 func_ovl0F_05_800456B8
#define CORE_ZJ_1709 func_ovl0F_05_800456C0
#define CORE_ZJ_1904 func_ovl0F_05_800459C8
#define CORE_ZJ_2B04 func_ovl0F_05_80045F50
#include "overlays/common/core.h"











void func_ovl0F_05_8004472C(void) {
    f32 x;
    f32 z;
    s32 i;

    D_ovl0F_05_80046550.x = 35.0f;
    D_ovl0F_05_80046550.y = 38.894f;
    D_ovl0F_05_80046550.z = 4000.0f;
    x = 3929.4f;
    z = 1626.7f;
    for (i = 0; i < 500; i++) {
        x += -4.8027997f;
        z += -0.29177588f;
        D_ovl0F_05_80046550.y -= 0.03f;
        D_ovl0F_05_80046550.z += -4.0f;
        func_ovl0F_05_80045698(x, 32768.0f, z);
        func_ovl0F_05_800456B8(32768.0f, D_ovl0F_05_80046550.y, D_ovl0F_05_80046550.z);
        func_ovl0F_05_800459F0();
    }
    func_ovl0F_05_800459C8(100);
    for (i = 0; i < 600; i++) {
        D_ovl0F_05_80046550.y -= 0.081166685f;
        if (D_ovl0F_05_80046550.y < 0.0f) {
            D_ovl0F_05_80046550.y += 360.0f;
        }
        D_ovl0F_05_80046550.x -= 0.073666684f;
        if (D_ovl0F_05_80046550.x < 0.0f) {
            D_ovl0F_05_80046550.x += 360.0f;
        }
        func_ovl0F_05_800456B8(D_ovl0F_05_80046550.x, D_ovl0F_05_80046550.y, 32768.0f);
        func_ovl0F_05_800459F0();
    }
    func_ovl0F_05_800459C8(160);
    for (i = 0; i < 200; i++) {
        x += 2.8232496f;
        z += 0.4574402f;
        func_ovl0F_05_80045698(x, 32768.0f, z);
        func_ovl0F_05_800459F0();
    }
    while (1) {
        func_ovl0F_05_800459C8(100);
    }
}

static void func_ovl0F_05_800440A4(s16 x, s16 z, Vec3f *vel) {
    vel->x = x * 0.002f;
    vel->z = z * 0.002f;
    func_ovl0F_05_8004409C();
}

void func_ovl0F_05_800440AC(UnkStructA *arg0) {
    Vec3f vel;
    f32 dx;
    f32 dz;
    Vec3f pos;

    dx = arg0->unk40->unk10 - arg0->unk58;
    dz = arg0->unk40->unk18 - arg0->unk60;
    if (arg0->unk8 & 1) {
        D_ovl0F_05_8004615C = 1;
        D_ovl0F_05_80046160 = arg0;
    } else {
        D_ovl0F_05_8004615C = 0;
        D_ovl0F_05_80046160 = NULL;
    }
    if (arg0->unk68 != 0.0f || arg0->unk6C != 0.0f) {
        arg0->unk64 = 0;
        D_ovl0F_05_80046158 = 0;
        return;
    }
    pos.x = arg0->unk40->unk10;
    pos.y = arg0->unk40->unk14;
    pos.z = arg0->unk40->unk18;
    func_ovl0F_05_800440A4(arg0->unk64 >> 16, arg0->unk64 & 0xFFFF, &vel);
    if ((s32)dx != (s32)vel.x) {
        arg0->unk40->unk10 = arg0->unk58;
        if ((s32)dx < (s32)vel.x) {
            vel.x -= (dz != 0.0f ? 0.707f : 1.0f) * (dx != 0.0f ? D_ovl0F_05_80046154 : D_ovl0F_05_80046150);
            if (vel.x < dx) {
                vel.x = dx;
            }
            func_ovl0F_05_80043FF4(dx);
        } else if ((s32)vel.x < (s32)dx) {
            vel.x += (dz != 0.0f ? 0.707f : 1.0f) * (dx != 0.0f ? D_ovl0F_05_80046154 : D_ovl0F_05_80046150);
            if (dx < vel.x) {
                vel.x = dx;
            }
            func_ovl0F_05_80043FF4(dx);
        }
        arg0->unk40->unk10 += vel.x;
    }
    if ((s32)dz != (s32)vel.z) {
        arg0->unk40->unk18 = arg0->unk60;
        if ((s32)dz < (s32)vel.z) {
            vel.z -= (dx != 0.0f ? 0.707f : 1.0f) * (dz != 0.0f ? D_ovl0F_05_80046154 : D_ovl0F_05_80046150);
            if (vel.z < dz) {
                vel.z = dz;
            }
            func_ovl0F_05_80043FF4(dz);
        } else if ((s32)vel.z < (s32)dz) {
            vel.z += (dx != 0.0f ? 0.707f : 1.0f) * (dz != 0.0f ? D_ovl0F_05_80046154 : D_ovl0F_05_80046150);
            if (dz < vel.z) {
                vel.z = dz;
            }
            func_ovl0F_05_80043FF4(dz);
        }
        arg0->unk40->unk18 += vel.z;
    }
    if (func_ovl0F_05_80043FEC(arg0)) {
        func_ovl0F_05_80043FE4(&arg0->unk64, 0.0f, 0.0f);
        arg0->unk40->unk10 = pos.x;
        arg0->unk40->unk14 = pos.y;
        arg0->unk40->unk18 = pos.z;
    } else {
        arg0->unk64 = ((s16)(vel.x * 500.0f) << 16) | ((s16)(vel.z * 500.0f) & 0xFFFF);
    }
}

/* empty, K&R-style (no frame when inlined, unlike `(void)`): inlined in
 * 800440A4, its stub comes right before 800440A4's */
static void func_ovl0F_05_8004409C() {
}

#include "overlays/ovl_0F_05/statics.h"

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl0F_05_800430BC(s32 arg0) {
    /* the empty global: inlined, its call gives this frame the original's
     * 8 extra bytes and leaves no stub (an empty static would); where the
     * call was is placed by those bytes */
    func_ovl0F_05_80043010();
    switch (arg0) {
        case 2:
            D_ovl0F_05_8004655C = func_ovl0F_05_80044F58();
            if (D_ovl0F_05_8004655C == 0x10) {
                D_ovl0F_05_80046188 = 1;
                func_ovl0F_05_80045ED8(0);
            } else {
                D_ovl0F_05_80046188 = 0;
            }
            func_ovl0F_05_80045CA8(0xFF, 0, 0, 120.0f);
            func_ovl0F_05_800430B4();
            func_ovl0F_05_800430AC();
            func_ovl0F_05_800430A4(12, D_ovl0F_05_800461E0);
            func_ovl0F_05_80045550(D_ovl0F_05_800464E0[0]->unk40, 0x61);
            func_ovl0F_05_80045550(D_ovl0F_05_800464E0[5]->unk40, 0x61);
            func_ovl0F_05_80045550(D_ovl0F_05_800464E0[7]->unk40, 0x61);
            func_ovl0F_05_80045550(D_ovl0F_05_800464E0[8]->unk40, 0x61);
            D_ovl0F_05_800464E0[8]->unk40->unk14 -= 3200.0f;
            D_ovl0F_05_800464E0[9]->unk40->unk14 -= 3200.0f;
            D_ovl0F_05_800464E0[10]->unk40->unk14 -= 3200.0f;
#define STATICS_PART 1
#include "overlays/ovl_0F_05/statics.h"
#undef STATICS_PART
            D_ovl0F_05_800464E0[2]->unk40->unk10 = 2400.0f;
            D_ovl0F_05_800464E0[2]->unk40->unk14 = 2600.0f;
            D_ovl0F_05_800464E0[2]->unk40->unk18 = -2000.0f;
            D_ovl0F_05_800464E0[2]->unk40->unk28 = 16.0f;
            D_ovl0F_05_800464E0[2]->unk40->unk2C = 16.0f;
            D_ovl0F_05_800464E0[2]->unk40->unk30 = 16.0f;
            D_ovl0F_05_80046164 = 0.0f;
            if (D_ovl0F_05_80046188 == 0) {
                D_ovl0F_05_80046170 = func_ovl0F_05_80045098(func_ovl0F_05_80045038());
                func_ovl0F_05_80045860(D_ovl0F_05_80046170, 0x01000000);
            }
            D_ovl0F_05_80046180 = -1;
            break;
        case 16:
            func_ovl0F_05_80045010(6, 0, 11);
            break;
        case 4:
            if (D_ovl0F_05_80046188 == 1) {
                func_ovl0F_05_800456B8(35.0f, 38.894f, 4000.0f);
                func_ovl0F_05_800456A8(1259.038f, 1715.864f, 2157.064f);
                func_ovl0F_05_80045698(3929.4f, -132.448f, 1626.7f);
                func_ovl0F_05_800459A8(func_ovl0F_05_80043B74, 0, 0, 0, 0x130);
                func_ovl0F_05_80045700(0x7F);
            } else {
                func_ovl0F_05_80044FC8(1);
                func_ovl0F_05_80045048(1);
                func_ovl0F_05_800457B8();
                func_ovl0F_05_80045700(0x7F);
                D_ovl0F_05_80046168 = 24.1f;
                D_ovl0F_05_8004616C = 0.0f;
                func_ovl0F_05_80045698(2702.9f, 2073.1f, -975.7f);
                func_ovl0F_05_800456B8(D_ovl0F_05_80046168, D_ovl0F_05_8004616C, 6408.0f);
                D_ovl0F_05_80046178 = D_ovl0F_05_800464E0[3]->unk40->unk18;
                D_ovl0F_05_8004617C = D_ovl0F_05_800464E0[3]->unk40->unk14;
                D_ovl0F_05_800464E0[3]->unk40->unk28 = 0.5f;
                D_ovl0F_05_800464E0[3]->unk40->unk2C = 0.5f;
                D_ovl0F_05_800464E0[3]->unk40->unk30 = 0.5f;
                D_ovl0F_05_80046174 = 2200.0f;
                D_ovl0F_05_800464E0[3]->unk40->unk18 = D_ovl0F_05_80046178 - D_ovl0F_05_80046174;
                D_ovl0F_05_800464E0[3]->unk40->unk14 =
                    (1.0f - D_ovl0F_05_800464E0[3]->unk40->unk2C) * 600.0f + (D_ovl0F_05_8004617C + D_ovl0F_05_80046174);
                func_ovl0F_05_800459A8(func_ovl0F_05_8004373C, 0, 0, 0, 0x130);
            }
            break;
        case 1:
            func_ovl0F_05_80043018(12, D_ovl0F_05_800461E0);
            if (D_ovl0F_05_80046180 != -1) {
                func_ovl0F_05_80045E08(D_ovl0F_05_80046180);
            }
            if (D_ovl0F_05_80046188 == 1) {
                func_ovl0F_05_800460B0();
            } else {
                func_ovl0F_05_80045CA8(0, 0, 0, 0.0f);
                func_ovl0F_05_80045858(D_ovl0F_05_80046170, 0x01000000);
                func_ovl0F_05_800457C0();
                func_ovl0F_05_80044FC0(1);
                func_ovl0F_05_80045048(0);
                func_ovl0F_05_80044FC8(0);
            }
            break;
        case 3:
            D_ovl0F_05_800463F0 = 1;
            func_ovl0F_05_80044FC0(0);
            break;
    }
}

static void func_ovl0F_05_80043020(UnkStruct_ovl0F_01_80048264 *arg0) {
    f32 pos[3];

    func_ovl0F_05_80044EB8(arg0->unkC, arg0->unk10, arg0->unk14, pos, 0);
    (*arg0->unk0)->unk40->unk10 = pos[0];
    (*arg0->unk0)->unk40->unk14 = pos[1];
    (*arg0->unk0)->unk40->unk18 = pos[2];
}

void func_ovl0F_05_80043010(void) {
}
