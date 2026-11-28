/*
 * ovl_0C_08's game object, IDO -O3; it matches in us, eu and jp. The
 * functions are written last to first (IDO -O3 emits a file's functions
 * in reverse). func_ovl0C_08_80043654 ends with the model placement, the
 * static func_ovl0C_08_800435D4, defined in another file (statics.h): the
 * change of file back at the closing brace (ugen's `.file`) ends an as1
 * block there, and the epilogue then restores s0, s1, s2 in order, as the
 * original does (without it, s2, s1, s0).
 */
#include "common.h"
#include "overlays/common/types.h"

/* an object placed in the stage (as ovl_17_01's) */
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
} UnkStruct_ovl0C_08_800454FC; /* size 0x38 */

void func_ovl0C_08_800441C8(s32);
void func_ovl0C_08_800441E0(s32);
void func_ovl0C_08_80044250(UnkStructA_40 *, f32 *, f32 *);
void func_ovl0C_08_800442B0(void (*)(UnkStructA *));
void func_ovl0C_08_800442C0(s32, s32, s32);
void func_ovl0C_08_80044300(void (*)(UnkStructA *));
void func_ovl0C_08_80044350(s32, s32);
void func_ovl0C_08_80044368(s32);
void func_ovl0C_08_80044378(void);
void func_ovl0C_08_80044380(s32, s32, s32);
void func_ovl0C_08_80044388(void *);
s32 func_ovl0C_08_80044390(s32, s32, s32, s32);
s32 func_ovl0C_08_800443A8(void);
void func_ovl0C_08_800443B0(s32, void *);
void func_ovl0C_08_800443B8(s32);
void func_ovl0C_08_800443C0(s32);
void func_ovl0C_08_800443C8(s32, s32, f32, s32, s32, s32, s32);
void func_ovl0C_08_800443D8(void);
UnkStructA *func_ovl0C_08_80044408(s32);
void func_ovl0C_08_80044410(s32);
s32 func_ovl0C_08_80044910(UnkStructA_40 *, s32, s32, s32, s32, s32);
void func_ovl0C_08_80044A08(f32, f32, f32);
void func_ovl0C_08_80044A28(f32, f32, f32);
void func_ovl0C_08_80044A30(f32 *, f32 *, f32 *);
void func_ovl0C_08_80044AC0(f32, f32);
void func_ovl0C_08_80044BC8(UnkStructA *, s32);
void func_ovl0C_08_80044BD0(UnkStructA *, s32);
void func_ovl0C_08_80044BE0(UnkStructA *);
UnkStructA *func_ovl0C_08_80044BF0(s32, s32, s32, PORT_IP(s32));
UnkStructA *func_ovl0C_08_80044C10(s32, s32, f32, f32, f32);
void func_ovl0C_08_80044D18(void (*)(void), void *, PORT_IP(s32), s32, s32);
void func_ovl0C_08_80044D28(void);
void func_ovl0C_08_80044D38(s32);
UnkStructA *func_ovl0C_08_80044D40(void);
void func_ovl0C_08_80044D60(void);
f32 func_ovl0C_08_80044EC0(f32);
void func_ovl0C_08_80044F98(s32);
void func_ovl0C_08_80045080(u32 (*)[3]);
void func_ovl0C_08_80045228(s32, s32, s32);
void func_ovl0C_08_800452A0(s32);
s32 func_ovl0C_08_800452C0(s32, f32, f32, f32);

void func_ovl0C_08_80043010(void);
void func_ovl0C_08_80043020(void);
void func_ovl0C_08_80043040(void);
void func_ovl0C_08_80043068(void);
void func_ovl0C_08_800430B0(void);
void func_ovl0C_08_80043108(void);
void func_ovl0C_08_80043170(void);
void func_ovl0C_08_800431B8(void);
void func_ovl0C_08_80043244(void);
void func_ovl0C_08_800432A8(void);
void func_ovl0C_08_80043300(f32);
void func_ovl0C_08_80043348(void);
void func_ovl0C_08_80043478(void);
void func_ovl0C_08_800434B4(s32 arg0, s32 arg1, s32 arg2, f32 arg3, s32 arg4);
void func_ovl0C_08_800434DC(void);
void func_ovl0C_08_80043528(void);
void func_ovl0C_08_80043580(void);
void func_ovl0C_08_800435CC(void);
void func_ovl0C_08_800435DC(void);
void func_ovl0C_08_8004361C(void);
void func_ovl0C_08_80043888(s32 arg0);
void func_ovl0C_08_80043A20(void);
void func_ovl0C_08_80043AA4(UnkStructA *);
void func_ovl0C_08_80043B64(UnkStructA *);
void func_ovl0C_08_80043BA8(UnkStructA *arg0, s32 arg1, s32 arg2, f32 arg3, f32 arg4);
void func_ovl0C_08_80043C80(void);
s32 func_ovl0C_08_80043D90(s32, void *);
void func_ovl0C_08_80043DC4(f32, f32, f32);
void func_ovl0C_08_80043E20(void);
void func_ovl0C_08_80043E38(void);

static void func_ovl0C_08_80043018(void);
static void func_ovl0C_08_80043060(void);
static void func_ovl0C_08_800430F8(void);
static void func_ovl0C_08_80043100(void);
static void func_ovl0C_08_800435D4(UnkStruct_ovl0C_08_800454FC *);

/* The three statements are on one line, a macro: only then does IDO
 * schedule across them (the store into the first call's delay slot). */
#define UNK_MACRO_ovl0C_08_800430B0() \
    D_ovl0C_08_800455BC = func_ovl0C_08_800443A8(); func_ovl0C_08_80044388(&D_ovl0C_08_800455B0); func_ovl0C_08_800443B0(D_ovl0C_08_800455BC, D_ovl0C_08_800455A0)

/* empty statics: each leaves a `jr ra; nop` just before its caller */

/* .bss */
u8 D_ovl0C_08_800455A0[0x10];
Vec3f D_ovl0C_08_800455B0;
s32 D_ovl0C_08_800455BC;
u8 D_ovl0C_08_800455C0[0x4]; /* unused (func_ovl0C_08_80043E78) */
s32 D_ovl0C_08_800455C4;
s32 D_ovl0C_08_800455C8;
Vec3f D_ovl0C_08_800455D0;
UnkStructA *D_ovl0C_08_800455DC;
UnkStruct_ovl0C_01_8004A9C8 D_ovl0C_08_800455E0;

/* .data */
/* passed to func_ovl0C_08_80045080: three 12-byte records */
u32 D_ovl0C_08_800454C0[][3] = {
    { 0x940002, 0xE, 4 },
    { 0, 0, 4 },
    { 0, 0, 4 },
};
/* two 12-byte records, each passed to func_ovl0C_08_80045228 */
u32 D_ovl0C_08_800454E4[][3] = {
    { 0, 0, 0xDE },
    { 1, 0x22, 0xE0 },
};
/* an object placed in the stage (func_ovl0C_08_800435D4) */
UnkStruct_ovl0C_08_800454FC D_ovl0C_08_800454FC = {
    NULL, -1, 0x2BA, -5, -0x400, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
};

#include "statics.h"

/* unused: the stubs IDO -O3 leaves at the end of the object; one of them
 * refers to the .bss nothing else uses, which would otherwise be common */
static void func_ovl0C_08_80043E78(void) {
    D_ovl0C_08_800455C0[0] = 0;
}

static void func_ovl0C_08_80043E70(void) {
}

static void func_ovl0C_08_80043E68(void) {
}

static void func_ovl0C_08_80043E60(void) {
}

static void func_ovl0C_08_80043E58(void) {
}

static void func_ovl0C_08_80043E50(void) {
}

static void func_ovl0C_08_80043E48(void) {
}

static void func_ovl0C_08_80043E40(void) {
}

void func_ovl0C_08_80043E38(void) {
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl0C_08_80043E20
#define CORE_FUNC_B func_ovl0C_08_80043DC4
#define CORE_FUNC_C func_ovl0C_08_80043300
#define CORE_FUNC_D func_ovl0C_08_80043D90
#define CORE_BSS_A0 D_ovl0C_08_800455C4
#define CORE_BSS_A1 D_ovl0C_08_800455C8
#define CORE_BSS_B D_ovl0C_08_800455D0
#define CORE_ZJ_1704 func_ovl0C_08_80044A08
#define CORE_ZJ_1708 func_ovl0C_08_80044A28
#define CORE_ZJ_1709 func_ovl0C_08_80044A30
#define CORE_ZJ_1904 func_ovl0C_08_80044D38
#define CORE_ZJ_2B04 func_ovl0C_08_800452C0
#include "overlays/common/core.h"

/* the names src/overlays/common/core_0c.h's code has here */
#define CORE0C_80048694 func_ovl0C_08_80043AA4
#define CORE0C_80048754 func_ovl0C_08_80043B64
#define CORE0C_80048798 func_ovl0C_08_80043BA8
#define CORE0C_80048870 func_ovl0C_08_80043C80
#define CORE0C_80048E40 func_ovl0C_08_80044250
#define CORE0C_800497C0 func_ovl0C_08_80044BD0
#define CORE0C_800497D0 func_ovl0C_08_80044BE0
#define CORE0C_800497E0 func_ovl0C_08_80044BF0
#define CORE0C_80049908 func_ovl0C_08_80044D18
#define CORE0C_80049918 func_ovl0C_08_80044D28
#define CORE0C_80049930 func_ovl0C_08_80044D40
#define CORE0C_80049950 func_ovl0C_08_80044D60
#define CORE0C_80049AB0 func_ovl0C_08_80044EC0
#include "overlays/common/core_0c.h"

void func_ovl0C_08_80043A20(void) {
    UnkStructA *obj;
    s32 alpha;
    s32 step;

    func_ovl0C_08_800443B8(1);
    func_ovl0C_08_80044D38(10);
    obj = func_ovl0C_08_80044408(func_ovl0C_08_800443A8());
    func_ovl0C_08_800452A0(0x44);
    func_ovl0C_08_80044BC8(obj, 0x80000);
    step = 0x11;
    for (alpha = 0xFF; alpha > 0; alpha -= step) {
        obj->unk40->unk36 = alpha;
        func_ovl0C_08_80044D60();
    }
    obj->unk40->unk36 = 0;
    func_ovl0C_08_80044D28();
}

/* the names src/overlays/common/flags.h's code has here */
#define FLAGS_80046FDC func_ovl0C_08_800435DC
#define FLAGS_8004701C func_ovl0C_08_8004361C
#define FLAGS_80048694 func_ovl0C_08_80043AA4
#define FLAGS_80048754 func_ovl0C_08_80043B64
#define FLAGS_80048EA0 func_ovl0C_08_800442B0
#define FLAGS_80048EF0 func_ovl0C_08_80044300
#define FLAGS_80049B88 func_ovl0C_08_80044F98
#include "overlays/common/flags.h"

void func_ovl0C_08_80043654(void) {
    u32 (*p)[3];
    s32 i;
    s32 count;

    func_ovl0C_08_8004361C();
    func_ovl0C_08_80045080(D_ovl0C_08_800454C0);
    func_ovl0C_08_800441C8(0x249);
    p = D_ovl0C_08_800454E4;
    count = 2;
    for (i = 0; i < count; i++) {
        func_ovl0C_08_80045228((*p)[0], (*p)[1], (*p)[2]);
        p++;
    }
    func_ovl0C_08_800441E0(0x253);
    func_ovl0C_08_80044378();
    func_ovl0C_08_800435D4(&D_ovl0C_08_800454FC);
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl0C_08_80043888(s32 arg0) {
    switch (arg0) {
        case 1:
            break;
        case 2:
            func_ovl0C_08_80043654();
            break;
        case 17:
            func_ovl0C_08_80043580();
            break;
        case 18:
            func_ovl0C_08_80043528();
            break;
        case 19:
            func_ovl0C_08_800434DC();
            break;
        case 4:
            func_ovl0C_08_80043348();
            if (D_ovl0C_08_800455E0.unk0 == 2) {
                func_ovl0C_08_800432A8();
            }
            break;
        case 5:
            if (D_ovl0C_08_800455E0.unk0 == 2) {
                func_ovl0C_08_800431B8();
            }
            func_ovl0C_08_800443C8(D_ovl0C_08_800455E0.unk4, 3, 4.0f, D_ovl0C_08_800455E0.unk10, 0, 1, 1);
            break;
        case 0x50:
            func_ovl0C_08_80043108();
            break;
        case 3:
            UNK_MACRO_ovl0C_08_800430B0();
            break;
        case 12:
            func_ovl0C_08_80044410(1);
            break;
        case 0x52:
            break;
        case 0x53:
            break;
    }
}

void func_ovl0C_08_800435CC(void) {
}

void func_ovl0C_08_80043580(void) {
    func_ovl0C_08_80044380(0x17, 0, 0x19);
    func_ovl0C_08_800434B4(1, 0, 0, 0.0f, 2);
}

void func_ovl0C_08_80043528(void) {
    func_ovl0C_08_80044380(0x17, 0, 0x19);
    func_ovl0C_08_800434B4(2, 0, 0, 0.0f, 2);
    func_ovl0C_08_80044350(0x2F, 0x13);
}

void func_ovl0C_08_800434DC(void) {
    func_ovl0C_08_80044380(0x17, 0, 0x19);
    func_ovl0C_08_800434B4(3, 0, 0, 0.0f, 2);
}

void func_ovl0C_08_800434B4(s32 arg0, s32 arg1, s32 arg2, f32 arg3, s32 arg4) {
    D_ovl0C_08_800455E0.unk0 = arg0;
    D_ovl0C_08_800455E0.unk4 = arg1;
    D_ovl0C_08_800455E0.unk8 = arg2;
    D_ovl0C_08_800455E0.unkC = arg3;
    D_ovl0C_08_800455E0.unk10 = arg4;
}

void func_ovl0C_08_80043478(void) {
    func_ovl0C_08_80043348();
    if (D_ovl0C_08_800455E0.unk0 == 2) {
        func_ovl0C_08_800432A8();
    }
}

void func_ovl0C_08_80043348(void) {
    f32 angle;

    func_ovl0C_08_800443D8();
    switch (D_ovl0C_08_800455E0.unk8) {
        case 0:
            angle = 0.0f;
            break;
        case 0x80:
            angle = 180.0f;
            break;
        case 0xC0:
            angle = 90.0f;
            break;
        case 0x40:
            angle = 270.0f;
            break;
        case 1:
        default:
            angle = 360.0f;
            break;
    }
    if (angle != 360.0f) {
        func_ovl0C_08_80044A28(angle, 32768.0f, 32768.0f);
    } else {
        func_ovl0C_08_80043300(D_ovl0C_08_800455E0.unkC);
    }
    func_ovl0C_08_800443C0(D_ovl0C_08_800455E0.unk4);
    func_ovl0C_08_80044AC0(45.0f, 45.1f);
}

void func_ovl0C_08_800432A8(void) {
    D_ovl0C_08_800455DC = func_ovl0C_08_80044408(func_ovl0C_08_800443A8());
    func_ovl0C_08_80044BC8(D_ovl0C_08_800455DC, 0x80000);
    D_ovl0C_08_800455DC->unk40->unk36 = 0;
}

void func_ovl0C_08_80043244(void) {
    if (D_ovl0C_08_800455E0.unk0 == 2) {
        func_ovl0C_08_800431B8();
    }
    func_ovl0C_08_800443C8(D_ovl0C_08_800455E0.unk4, 3, 4.0f, D_ovl0C_08_800455E0.unk10, 0, 1, 1);
}

void func_ovl0C_08_800431B8(void) {
    s32 alpha;
    s32 step;

    func_ovl0C_08_800443B8(1);
    func_ovl0C_08_800452A0(0x45);
    step = 0x11;
    for (alpha = 0; alpha < 0xFF; alpha += step) {
        D_ovl0C_08_800455DC->unk40->unk36 = alpha;
        func_ovl0C_08_80044D60();
    }
    D_ovl0C_08_800455DC->unk40->unk36 = 0xFF;
    func_ovl0C_08_80044BD0(D_ovl0C_08_800455DC, 0x80000);
    func_ovl0C_08_800443B8(0);
}

void func_ovl0C_08_80043170(void) {
    func_ovl0C_08_800443C8(D_ovl0C_08_800455E0.unk4, 3, 4.0f, D_ovl0C_08_800455E0.unk10, 0, 1, 1);
}

void func_ovl0C_08_80043108(void) {
    func_ovl0C_08_800430F8();
    func_ovl0C_08_80043100();
    if (func_ovl0C_08_80044390(0x17, 0x18, 0x18, 0x19) != 0) {
        func_ovl0C_08_80044D18(func_ovl0C_08_80043A20, 0, 0, 0, 0x130);
        func_ovl0C_08_80044368(60);
        func_ovl0C_08_800442C0(0x2D, 0x13, 4);
    }
}

static void func_ovl0C_08_80043100(void) {
}

static void func_ovl0C_08_800430F8(void) {
}

void func_ovl0C_08_800430B0(void) {
    UNK_MACRO_ovl0C_08_800430B0();
}

void func_ovl0C_08_80043068(void) {
    func_ovl0C_08_80043060();
    D_ovl0C_08_800455BC = func_ovl0C_08_800443A8();
    func_ovl0C_08_80044388(&D_ovl0C_08_800455B0);
    func_ovl0C_08_800443B0(D_ovl0C_08_800455BC, D_ovl0C_08_800455A0);
}

static void func_ovl0C_08_80043060(void) {
}

void func_ovl0C_08_80043040(void) {
    func_ovl0C_08_80044410(1);
}

void func_ovl0C_08_80043020(void) {
    func_ovl0C_08_80043018();
    func_ovl0C_08_80044410(1);
}

static void func_ovl0C_08_80043018(void) {
}

void func_ovl0C_08_80043010(void) {
}

