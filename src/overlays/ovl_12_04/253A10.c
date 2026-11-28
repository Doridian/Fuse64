#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_12_04's game object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse).
 */

#define ZJ_0C00 func_ovl12_04_80043CC8
#define ZJ_0D00 func_ovl12_04_80043CE0
#define ZJ_0D0E func_ovl12_04_80043D50
#define ZJ_0E03 func_ovl12_04_80043DB0
#define ZJ_0F00 func_ovl12_04_80043DC0
#define ZJ_0F08 func_ovl12_04_80043E00
#define ZJ_1000 func_ovl12_04_80043E78
#define ZJ_1001 func_ovl12_04_80043E80
#define ZJ_1002 func_ovl12_04_80043E88
#define ZJ_1003 func_ovl12_04_80043E90
#define ZJ_1006 func_ovl12_04_80043EA8
#define ZJ_1007 func_ovl12_04_80043EB0
#define ZJ_1009 func_ovl12_04_80043EC0
#define ZJ_100A func_ovl12_04_80043EC8
#define ZJ_100C func_ovl12_04_80043ED8
#define ZJ_1013 func_ovl12_04_80043F10
PORT_IP(s32) func_ovl12_04_80043F48(s32); /* 0x101A, kept: it keeps the return in PORT_IP(s32) (the header's UnkStructA *) */
#define ZJ_1704 func_ovl12_04_80044508
#define ZJ_1708 func_ovl12_04_80044528
#define ZJ_1709 func_ovl12_04_80044530
#define ZJ_1711 func_ovl12_04_80044570
#define ZJ_1807 func_ovl12_04_80044678
#define ZJ_1812 func_ovl12_04_800446D0
#define ZJ_1821 func_ovl12_04_80044748
PORT_IP(s32) func_ovl12_04_800447E8(UnkStructA *, s32, PORT_IP(s32)); /* 0x1835, kept: its callers pass PORT_IP(s32) (the header's UnkStructA *) */
#define ZJ_1904 func_ovl12_04_80044838
#define ZJ_1909 func_ovl12_04_80044860
UnkStructA *func_ovl12_04_800448A0(s32); /* 0x1A00, kept: it keeps the return in UnkStructA * (the header's UnkStruct_802B0260 *) */
void func_ovl12_04_800448B0(UnkStructA *, s32); /* 0x1A02, kept: its callers pass UnkStructA * (the header's UnkStruct_802B0260 *) */
#define ZJ_1E07 func_ovl12_04_80044A98
#define ZJ_2101 func_ovl12_04_80044B40
#define ZJ_2300 func_ovl12_04_80044B80
#define ZJ_2303 func_ovl12_04_80044B98
#define ZJ_2901 func_ovl12_04_80044D18
#define ZJ_2A00 func_ovl12_04_80044D28
#define ZJ_2B04 func_ovl12_04_80044DC0
#include "overlays/common/zerojump.h"

void func_ovl12_04_80043010(void);
void func_ovl12_04_80043020(void);
void func_ovl12_04_80043040(void);
void func_ovl12_04_80043068(void);
void func_ovl12_04_800430B0(void);
void func_ovl12_04_80043108(void);
void func_ovl12_04_80043170(void);
void func_ovl12_04_800431B8(void);
void func_ovl12_04_800431D8(f32);
void func_ovl12_04_80043220(void);
void func_ovl12_04_80043338(void);
void func_ovl12_04_80043358(s32, s32, s32, f32, s32);
void func_ovl12_04_80043380(void);
void func_ovl12_04_800433CC(void);
void func_ovl12_04_80043418(void);
void func_ovl12_04_80043420(void);
void func_ovl12_04_80043460(void);
void func_ovl12_04_800434CC(void);
void func_ovl12_04_80043570(s32);
void func_ovl12_04_80043690(UnkStructA *);
void func_ovl12_04_80043750(UnkStructA *);
void func_ovl12_04_80043810(UnkStructA *);
s32 func_ovl12_04_80043888(s32, void *);
void func_ovl12_04_800438BC(f32, f32, f32);
void func_ovl12_04_80043918(void);
void func_ovl12_04_80043930(void);

/* empty statics: each leaves a `jr ra; nop` just before its caller */
static void func_ovl12_04_80043018(void);
static void func_ovl12_04_80043060(void);
static void func_ovl12_04_800430F8(void);
static void func_ovl12_04_80043100(void);

/* The three statements are on one line, a macro: only then does IDO
 * schedule across them (the store into the first call's delay slot). */
#define UNK_MACRO_ovl12_04_800430B0() \
    D_ovl12_04_8004508C = func_ovl12_04_80043EA8(); func_ovl12_04_80043E88(&D_ovl12_04_80045080); func_ovl12_04_80043EB0(D_ovl12_04_8004508C, D_ovl12_04_80045070)

/* .bss */
u8 D_ovl12_04_80045070[0x10];
Vec3f D_ovl12_04_80045080;
s32 D_ovl12_04_8004508C;
u8 D_ovl12_04_80045090[0x4]; /* unused (func_ovl12_04_80043978) */
s32 D_ovl12_04_80045094;
s32 D_ovl12_04_80045098;
Vec3f D_ovl12_04_800450A0;
UnkStruct_ovl0C_01_8004A9C8 D_ovl12_04_800450B0;

/* .data */
/* passed to func_ovl12_04_80044B80: three 12-byte records */
u32 D_ovl12_04_80044FC0[][3] = {
    { 0x970000, 0xF, 0x2004 },
    { 0xCF0000, 0xE, 4 },
    { 0xD00002, 0xF, 4 },
};
/* passed to func_ovl12_04_80044B98 */
u32 D_ovl12_04_80044FE4[][3] = {
    { 0x980000, 0x84, 0 },
    { 4, 0, 4 },
};
/* two 12-byte records, each passed to func_ovl12_04_80044D28 */
u32 D_ovl12_04_80044FFC[][3] = {
    { 0, 0x21, 0xFF },
    { 1, 0x1F, 0xFD },
};

/* unused: the stubs IDO -O3 leaves at the end of the object; one of them
 * refers to the .bss nothing else uses, which would otherwise be common */
static void func_ovl12_04_80043978(void) {
    D_ovl12_04_80045090[0] = 0;
}

static void func_ovl12_04_80043970(void) {
}

static void func_ovl12_04_80043968(void) {
}

static void func_ovl12_04_80043960(void) {
}

static void func_ovl12_04_80043958(void) {
}

static void func_ovl12_04_80043950(void) {
}

static void func_ovl12_04_80043948(void) {
}

static void func_ovl12_04_80043940(void) {
}

static void func_ovl12_04_80043938(void) {
}

void func_ovl12_04_80043930(void) {
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl12_04_80043918
#define CORE_FUNC_B func_ovl12_04_800438BC
#define CORE_FUNC_C func_ovl12_04_800431D8
#define CORE_FUNC_D func_ovl12_04_80043888
#define CORE_BSS_A0 D_ovl12_04_80045094
#define CORE_BSS_A1 D_ovl12_04_80045098
#define CORE_BSS_B D_ovl12_04_800450A0
#include "overlays/common/core.h"

void func_ovl12_04_80043810(UnkStructA *arg0) {
    s32 i;

    if (arg0->unk8 & 1) {
        func_ovl12_04_80044570(0x10);
    }
    func_ovl12_04_800446D0(arg0, 0x02000000);
    for (i = 0; i < 30; i++) {
        func_ovl12_04_80044748(arg0);
        func_ovl12_04_80044860();
    }
}

void func_ovl12_04_80043750(UnkStructA *arg0) {
    func_ovl12_04_80044838(10);
    func_ovl12_04_80043D50(arg0->unk40, &arg0->unk40->unk1C, &arg0->unk40->unk20);
    arg0->unk40->unk28 *= 2.0f;
    arg0->unk40->unk30 *= 2.0f;
    arg0->unk40->unk2C *= 0.1f;
    arg0->unk40->unk14 += 20.0f;
    arg0->unk6C = 0.0f;
    arg0->unk70 = 0.0f;
    func_ovl12_04_80044838(60);
}

void func_ovl12_04_80043460(void) {
    UnkStructA *obj;

    func_ovl12_04_80043420();
    func_ovl12_04_80043DB0(func_ovl12_04_80043810);
    func_ovl12_04_80043E00(func_ovl12_04_80043750);
    obj = func_ovl12_04_800448A0(0x2E5);
    func_ovl12_04_800448B0(obj, 0x20000001);
    *(f32 *) &obj->unk18 = 0.03f;
    *(f32 *) &obj->unk1C = 0.0f;
}

void func_ovl12_04_80043690(UnkStructA *arg0) {
    s32 hit;
    PORT_IP(s32) t;

    func_ovl12_04_80044D18(arg0, 0, 5.0f);
    hit = FALSE;
    if (arg0->unk8 & 1) {
        t = func_ovl12_04_80043F48(func_ovl12_04_80043EA8());
        if (t != 0) {
            if (func_ovl12_04_800447E8(arg0, 0x10, t) != 0) {
                hit = TRUE;
            }
        } else if (func_ovl12_04_80044678(arg0, 0x10) != 0) {
            hit = TRUE;
        }
    } else if (func_ovl12_04_80044678(arg0, 0x10) != 0) {
        hit = TRUE;
    }
    if (hit) {
        func_ovl12_04_80044D18(arg0, 0x80, 5.0f);
    }
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl12_04_80043570(s32 arg0) {
    switch (arg0) {
        case 1:
            break;
        case 2:
            func_ovl12_04_800434CC();
            break;
        case 17:
            func_ovl12_04_800433CC();
            break;
        case 18:
            func_ovl12_04_80043380();
            break;
        case 4:
            func_ovl12_04_80043220();
            break;
        case 5:
            func_ovl12_04_80043170();
            break;
        case 0x50:
            func_ovl12_04_80043108();
            break;
        case 3:
            UNK_MACRO_ovl12_04_800430B0();
            break;
        case 12:
            func_ovl12_04_80043F10(1);
            break;
        case 0x52:
            break;
        case 0x53:
            break;
    }
}

void func_ovl12_04_800434CC(void) {
    u32 (*p)[3];
    s32 i;
    s32 count;

    func_ovl12_04_80043460();
    func_ovl12_04_80044B80(D_ovl12_04_80044FC0);
    func_ovl12_04_80044B98(D_ovl12_04_80044FE4);
    func_ovl12_04_80043CC8(0x2DA);
    p = D_ovl12_04_80044FFC;
    count = 2;
    for (i = 0; i < count; i++) {
        func_ovl12_04_80044D28((*p)[0], (*p)[1], (*p)[2]);
        p++;
    }
    func_ovl12_04_80043CE0(0x2E2);
    func_ovl12_04_80043E78();
    func_ovl12_04_80044B40(0, 0x72, 0x37);
}

void func_ovl12_04_80043420(void) {
    func_ovl12_04_80044A98(0x400);
    func_ovl12_04_80044A98(0x401);
    func_ovl12_04_80044A98(0x402);
    func_ovl12_04_80044A98(0x403);
    func_ovl12_04_80044A98(0x404);
}

void func_ovl12_04_80043418(void) {
}

void func_ovl12_04_800433CC(void) {
    func_ovl12_04_80043E80(0x13, 4, 7);
    func_ovl12_04_80043358(1, 0x80, 0, 0.0f, 0);
}

void func_ovl12_04_80043380(void) {
    func_ovl12_04_80043E80(0x1D, 4, 2);
    func_ovl12_04_80043358(2, 0x80, 0, 0.0f, 2);
}

void func_ovl12_04_80043358(s32 arg0, s32 arg1, s32 arg2, f32 arg3, s32 arg4) {
    D_ovl12_04_800450B0.unk0 = arg0;
    D_ovl12_04_800450B0.unk4 = arg1;
    D_ovl12_04_800450B0.unk8 = arg2;
    D_ovl12_04_800450B0.unkC = arg3;
    D_ovl12_04_800450B0.unk10 = arg4;
}

void func_ovl12_04_80043338(void) {
    func_ovl12_04_80043220();
}

/* the names src/overlays/common/angle.h's code has here */
#define ANGLE_80043A30 func_ovl12_04_800431D8
#define ANGLE_80043A78 func_ovl12_04_80043220
#define ANGLE_800458B0 func_ovl12_04_80043EC0
#define ANGLE_800458C8 func_ovl12_04_80043ED8
#define ANGLE_80045F18 func_ovl12_04_80044528
#define ANGLE_D_80046D28 D_ovl12_04_800450B0
#include "overlays/common/angle.h"

void func_ovl12_04_800431B8(void) {
    func_ovl12_04_80043170();
}

void func_ovl12_04_80043170(void) {
    /* where the other overlays fade in (UNK_MACRO_..._80043068), a
     * compiled-out debug check: it keeps the function too big to be
     * inlined into its callers, where the original calls it. Any test of
     * the record matches (unk0 == 0, unk0 < 0, unk4 < 0 too); which one
     * is a guess. */
    if (D_ovl12_04_800450B0.unk0 == 2) {
        DEBUG_PRINT(("mode 2\n"));
    }
    func_ovl12_04_80043EC8(D_ovl12_04_800450B0.unk4, 3, 4.0f, D_ovl12_04_800450B0.unk10, 0, 1, 1);
}

void func_ovl12_04_80043108(void) {
    func_ovl12_04_80043100();
    func_ovl12_04_800430F8();
    if (func_ovl12_04_80043E90(0x1D, 1, 0x1E, 2) != 0) {
        func_ovl12_04_80043DC0(0x7B, 0x12, 0);
    }
    if (func_ovl12_04_80043E90(0xC, 0x16, 0xD, 0x17) != 0) {
        func_ovl12_04_80043DC0(0x7B, 0x12, 0);
    }
}

static void func_ovl12_04_80043100(void) {
}

static void func_ovl12_04_800430F8(void) {
}

void func_ovl12_04_800430B0(void) {
    UNK_MACRO_ovl12_04_800430B0();
}

void func_ovl12_04_80043068(void) {
    func_ovl12_04_80043060();
    D_ovl12_04_8004508C = func_ovl12_04_80043EA8();
    func_ovl12_04_80043E88(&D_ovl12_04_80045080);
    func_ovl12_04_80043EB0(D_ovl12_04_8004508C, D_ovl12_04_80045070);
}

static void func_ovl12_04_80043060(void) {
}

void func_ovl12_04_80043040(void) {
    func_ovl12_04_80043F10(1);
}

void func_ovl12_04_80043020(void) {
    func_ovl12_04_80043018();
    func_ovl12_04_80043F10(1);
}

static void func_ovl12_04_80043018(void) {
}

void func_ovl12_04_80043010(void) {
}

