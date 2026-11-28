/*
 * ovl_0E_13 matches in us, eu and jp. Much of it is ovl_0E_09's code.
 *
 * - func_ovl0E_13_80043110: the D80 position set-up is the static it
 *   inlines, func_ovl0E_13_80043108 (the stub before it), with x, z and the
 *   count as arguments, as ovl_0E_04's func_ovl0E_04_8004399C. It is
 *   defined in another file (statics.h): the `.file` ugen emits where the
 *   inlined body starts ends an as1 block right after the hoisted
 *   invariants (&D80 in s1, 100.0f in fs0, the count in s2), in 80043110
 *   and where it is inlined in 8004439C and 80043444. The macro it was
 *   and its `p += 0` fake go.
 * - func_ovl0E_13_80043940 ends with the model removal 800437B8, which is
 *   in statics.h too with the model placement 800437C0 (as ovl_0D_07's
 *   pair): the block boundary before the epilogue makes it restore s0
 *   before s1.
 * IDO's scheduling follows source lines: 80043444 matches only with its
 * two camera pans on one line (UNK_MACRO_ovl0E_13_80043444), the second
 * reusing the first's a and y.
 *
 * 2026-11-13: 1 `#line` that turned out redundant was removed (each tried away
 * against the sha1 in us, eu and jp). Where the notes above name a `#line`,
 * region or renumbering that is gone, it is no longer needed.
 */
#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_0E_13's game object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse).
 */

/* a switch: its flag and where it is */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
} UnkStruct_ovl0E_13_800469F8;

#define ZJ_0C00 func_ovl0E_13_800454A8
#define ZJ_0D00 func_ovl0E_13_800454C0
#define ZJ_0D02 func_ovl0E_13_800454D0
#define ZJ_0D05 func_ovl0E_13_800454E8
#define ZJ_0D0A func_ovl0E_13_80045510
#define ZJ_0D0E func_ovl0E_13_80045530
#define ZJ_0E03 func_ovl0E_13_80045590
#define ZJ_0F00 func_ovl0E_13_800455A0
#define ZJ_0F05 func_ovl0E_13_800455C8
#define ZJ_0F08 func_ovl0E_13_800455E0
#define ZJ_0F0E func_ovl0E_13_80045610
#define ZJ_0F0F func_ovl0E_13_80045618
#define ZJ_1000 func_ovl0E_13_80045658
#define ZJ_1001 func_ovl0E_13_80045660
#define ZJ_1002 func_ovl0E_13_80045668
#define ZJ_1003 func_ovl0E_13_80045670
#define ZJ_1006 func_ovl0E_13_80045688
#define ZJ_1007 func_ovl0E_13_80045690
#define ZJ_1009 func_ovl0E_13_800456A0
#define ZJ_100A func_ovl0E_13_800456A8
#define ZJ_100C func_ovl0E_13_800456B8
#define ZJ_1013 func_ovl0E_13_800456F0
#define ZJ_150C func_ovl0E_13_80045BE8
#define ZJ_150D func_ovl0E_13_80045BF0
#define ZJ_150E func_ovl0E_13_80045BF8
#define ZJ_1704 func_ovl0E_13_80045CE8
#define ZJ_1708 func_ovl0E_13_80045D08
#define ZJ_1709 func_ovl0E_13_80045D10
#define ZJ_1710 func_ovl0E_13_80045D48
#define ZJ_1711 func_ovl0E_13_80045D50
#define ZJ_1722 func_ovl0E_13_80045DD8
#define ZJ_1728 func_ovl0E_13_80045E08
#define ZJ_1729 func_ovl0E_13_80045E10
f32 func_ovl0E_13_80045E50(f32 *, void *); /* 0x1806, kept: its callers pass u8[16] (the header's f32 *) */
#define ZJ_1807 func_ovl0E_13_80045E58
#define ZJ_1811 func_ovl0E_13_80045EA8
#define ZJ_1812 func_ovl0E_13_80045EB0
#define ZJ_1814 func_ovl0E_13_80045EC0
#define ZJ_181A func_ovl0E_13_80045EF0
#define ZJ_1821 func_ovl0E_13_80045F28
#define ZJ_1833 func_ovl0E_13_80045FB8
#define ZJ_1900 func_ovl0E_13_80045FF8
#define ZJ_1901 func_ovl0E_13_80046000
#define ZJ_1902 func_ovl0E_13_80046008
#define ZJ_1904 func_ovl0E_13_80046018
#define ZJ_1905 func_ovl0E_13_80046020
#define ZJ_1909 func_ovl0E_13_80046040
#define ZJ_1E06 func_ovl0E_13_80046270
#define ZJ_1E07 func_ovl0E_13_80046278
#define ZJ_1E08 func_ovl0E_13_80046280
#define ZJ_2104 func_ovl0E_13_80046338
#define ZJ_2201 func_ovl0E_13_80046348
#define ZJ_2300 func_ovl0E_13_80046360
#define ZJ_2600 func_ovl0E_13_80046448
#define ZJ_2601 func_ovl0E_13_80046450
#define ZJ_2602 func_ovl0E_13_80046458
#define ZJ_2A00 func_ovl0E_13_80046508
#define ZJ_2A0B func_ovl0E_13_80046560
#define ZJ_2B00 func_ovl0E_13_80046580
#define ZJ_2B04 func_ovl0E_13_800465A0
#define ZJ_2B05 func_ovl0E_13_800465A8
#include "overlays/common/zerojump.h"

void func_ovl0E_13_80043010(void);
void func_ovl0E_13_80043018(void);
void func_ovl0E_13_800430A8(void);
void func_ovl0E_13_800430C8(void);
void func_ovl0E_13_800430E8(void);
void func_ovl0E_13_80043110(void);
void func_ovl0E_13_800431E4(void);
void func_ovl0E_13_80043244(void);
void func_ovl0E_13_80043444(void);
s32 func_ovl0E_13_80043754(void);
void func_ovl0E_13_80043940(s16);
void func_ovl0E_13_80043A30(void);
void func_ovl0E_13_80043BD8(void);
void func_ovl0E_13_80043BE0(void);
void func_ovl0E_13_80043C28(void);
void func_ovl0E_13_80043C90(void);
void func_ovl0E_13_80043CD4(void);
void func_ovl0E_13_80043D78(void);
void func_ovl0E_13_80043DF0(void);
void func_ovl0E_13_80043E2C(void);
void func_ovl0E_13_80043E54(void);
void func_ovl0E_13_80043EA8(f32);
void func_ovl0E_13_80043EF0(void);
void func_ovl0E_13_80044008(void);
void func_ovl0E_13_80044048(s32, s32, s32, f32, s32);
void func_ovl0E_13_80044070(void);
void func_ovl0E_13_800440C0(void);
void func_ovl0E_13_80044108(void);
void func_ovl0E_13_80044158(void);
void func_ovl0E_13_8004424C(void);
void func_ovl0E_13_80044294(void);
void func_ovl0E_13_8004431C(void);
void func_ovl0E_13_80044324(void);
void func_ovl0E_13_80044364(void);
void func_ovl0E_13_8004439C(void);
void func_ovl0E_13_80044590(s32);
void func_ovl0E_13_800446F0(void);
void func_ovl0E_13_80044780(void);
void func_ovl0E_13_80044848(UnkStruct_ovl0E_09_80045DCC *, UnkStructA *);
void func_ovl0E_13_80044894(UnkStruct_ovl0E_09_80045DCC *);
void func_ovl0E_13_800449CC(void);
void func_ovl0E_13_80044F30(UnkStructA *);
void func_ovl0E_13_80044FF4(UnkStructA *);
s32 func_ovl0E_13_8004508C(s32, void *);
void func_ovl0E_13_800450C0(f32, f32, f32);
void func_ovl0E_13_8004511C(void);
void func_ovl0E_13_80045134(void);

static void func_ovl0E_13_800437B8(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0E_13_800437C0(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0E_13_80043CCC(void);
static void func_ovl0E_13_80043C78(void);
static void func_ovl0E_13_80043C80(void);
static void func_ovl0E_13_80043C88(void);
static void func_ovl0E_13_80043108(f32, s32, s32);

/* The three statements are on one line, a macro: only then does IDO
 * schedule across them (the store into the first call's delay slot). */
#define UNK_MACRO_ovl0E_13_80043BE0() \
    D_ovl0E_13_80046D6C = func_ovl0E_13_80045688(); func_ovl0E_13_80045668(&D_ovl0E_13_80046D60); func_ovl0E_13_80045690(D_ovl0E_13_80046D6C, D_ovl0E_13_80046D50)

/* The two camera pans of func_ovl0E_13_80043444, a macro (one line): IDO's
 * scheduling follows source lines, and only this matches. */
#define UNK_MACRO_ovl0E_13_80043444()                                                \
    {                                                                                \
        a = 135.0f;                                                                  \
        y = 1600.0f;                                                                 \
        z = 2800.0f;                                                                 \
        c = 3500.0f;                                                                 \
        b = 30.0f;                                                                   \
        for (i = 60; i > 0; i--) {                                                   \
            func_ovl0E_13_80045CE8(2250.0f, y, z);                                   \
            func_ovl0E_13_80045D08(a, b, c);                                         \
            func_ovl0E_13_80046040();                                                \
            a += 0.75f;                                                              \
            y += -3.3333333f;                                                        \
            z += 16.666666f;                                                         \
            c += -41.666668f;                                                        \
            b += 0.5f;                                                               \
        }                                                                            \
        func_ovl0E_13_80046018(30);                                                  \
        a = 3800.0f;                                                                 \
        y = 1000.0f;                                                                 \
        for (i = 60; i > 0; i--) {                                                   \
            func_ovl0E_13_80045CE8(2250.0f, 1400.0f, a);                             \
            func_ovl0E_13_80045D08(180.0f, 60.0f, y);                                \
            func_ovl0E_13_80046040();                                                \
            a += -13.333333f;                                                        \
            y += 16.666666f;                                                         \
        }                                                                            \
    }

/* .bss */
u8 D_ovl0E_13_80046D50[0x10];
Vec3f D_ovl0E_13_80046D60;
s32 D_ovl0E_13_80046D6C;
s32 D_ovl0E_13_80046D70;
s32 D_ovl0E_13_80046D74;
s32 D_ovl0E_13_80046D78;
u8 D_ovl0E_13_80046D7C[0x4]; /* unused (func_ovl0E_13_80045154) */
Vec3f D_ovl0E_13_80046D80;
UnkStruct_ovl0C_01_8004A9C8 D_ovl0E_13_80046D90;
UnkStructA *D_ovl0E_13_80046DA4;

/* .data */
/* passed to func_ovl0E_13_80046360: three 12-byte records */
u32 D_ovl0E_13_800467A0[][3] = {
    { 0xA20002, 0xE, 4 },
    { 0x9F0002, 0xE, 4 },
    { 0, 0, 4 },
};
/* five 12-byte records, each passed to func_ovl0E_13_80046508 */
u32 D_ovl0E_13_800467C4[][3] = {
    { 0, 0x22, 0xE0 },
    { 1, 0xE, 0xEC },
    { 2, 0, 0xDE },
    { 3, 0xD, 0xEB },
    { 4, 0xF, 0xED },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0E_13_80046800[1] = {
    { NULL, -1, 0x1B8, -1, -1, -1, -1, 200.0f, 210.0f, 260.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0E_13_80046838[4] = {
    { NULL, -1, 0x1B9, -1, -1, -1, -1, 190.0f, 160.0f, 260.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x1B9, -1, -1, -1, -1, 190.0f, 160.0f, 290.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x1B9, -1, -1, -1, -1, 260.0f, 160.0f, 260.0f, 0.0f, 180.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x1B9, -1, -1, -1, -1, 260.0f, 160.0f, 290.0f, 0.0f, 180.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0E_13_80046918[4] = {
    { NULL, -1, 0x1BA, -1, -1, -1, -1, 190.0f, 160.0f, 260.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x1BA, -1, -1, -1, -1, 190.0f, 160.0f, 290.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x1BA, -1, -1, -1, -1, 260.0f, 160.0f, 260.0f, 0.0f, 180.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x1BA, -1, -1, -1, -1, 260.0f, 160.0f, 290.0f, 0.0f, 180.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0E_13_800469F8 D_ovl0E_13_800469F8[4] = {
    { 0x40F, 190.0f, 160.0f, 260.0f },
    { 0x410, 190.0f, 160.0f, 290.0f },
    { 0x411, 260.0f, 160.0f, 260.0f },
    { 0x412, 260.0f, 160.0f, 290.0f },
};
s32 D_ovl0E_13_80046A38 = -1;
UnkStruct_ovl0E_09_80045DCC D_ovl0E_13_80046A3C[9] = {
    { 0, -1, NULL, 0, 0, 0, 550.0f, 2000.0f, 2449.0f },
    { 1, -1, NULL, 0, 0, 0, 550.0f, 2000.0f, 2449.0f },
    { 2, -1, NULL, 0, 0, 0, 550.0f, 2000.0f, 2449.0f },
    { 3, -1, NULL, 0, 0, 0, 550.0f, 2000.0f, 2449.0f },
    { 4, -1, NULL, 0, 0, 0, 550.0f, 2000.0f, 2449.0f },
    { 5, -1, NULL, 0, 0, 0, 550.0f, 2000.0f, 2449.0f },
    { 6, -1, NULL, 0, 0, 0, 550.0f, 2000.0f, 2449.0f },
    { 7, -1, NULL, 0, 0, 0, 550.0f, 2000.0f, 2449.0f },
    { 8, -1, NULL, 0, 0, 0, 550.0f, 2000.0f, 2449.0f },
};
UnkStruct_ovl0E_09_80045E0C D_ovl0E_13_80046B5C[15] = {
    { 0.0f, -10.0f, 180.0f, 550.0f, 1200.0f, 0, 2 },
    { 7.071068f, -7.071068f, 135.0f, 1000.0f, 750.0f, 1, 2 },
    { 10.0f, 0.0f, 90.0f, 4200.0f, 750.0f, 1, 0 },
    { 7.071068f, 7.071068f, 45.0f, 4550.0f, 1100.0f, 1, 1 },
    { 0.0f, 10.0f, 0.0f, 4550.0f, 2300.0f, 0, 1 },
    { -7.071068f, 7.071068f, 315.0f, 4100.0f, 2750.0f, 2, 1 },
    { -10.0f, 0.0f, 270.0f, 1000.0f, 2750.0f, 2, 0 },
    { -7.071068f, -7.071068f, 225.0f, 550.0f, 2300.0f, 2, 2 },
    { 0.0f, -10.0f, 180.0f, 550.0f, 1200.0f, 0, 2 },
    { 7.071068f, -7.071068f, 135.0f, 1000.0f, 750.0f, 1, 2 },
    { 10.0f, 0.0f, 90.0f, 4200.0f, 750.0f, 1, 0 },
    { 7.071068f, 7.071068f, 45.0f, 4550.0f, 1100.0f, 1, 1 },
    { 0.0f, 10.0f, 0.0f, 4550.0f, 2300.0f, 0, 1 },
    { -7.071068f, 7.071068f, 315.0f, 4100.0f, 2750.0f, 2, 1 },
    { -10.0f, 0.0f, 270.0f, 2800.0f, 2750.0f, 2, 0 },
};

#include "statics.h"

/* unused: the stubs IDO -O3 leaves at the end of the object; one of them
 * refers to the .bss nothing else uses, which would otherwise be common */
static void func_ovl0E_13_80045154(void) {
    D_ovl0E_13_80046D7C[0] = 0;
}

static void func_ovl0E_13_8004514C(void) {
}

static void func_ovl0E_13_80045144(void) {
}

static void func_ovl0E_13_8004513C(void) {
}

void func_ovl0E_13_80045134(void) {
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl0E_13_8004511C
#define CORE_FUNC_B func_ovl0E_13_800450C0
#define CORE_FUNC_C func_ovl0E_13_80043EA8
#define CORE_FUNC_D func_ovl0E_13_8004508C
#define CORE_BSS_A0 D_ovl0E_13_80046D74
#define CORE_BSS_A1 D_ovl0E_13_80046D78
#define CORE_BSS_B D_ovl0E_13_80046D80
#include "overlays/common/core.h"

void func_ovl0E_13_80044FF4(UnkStructA *arg0) {
    f32 pos[3];

    pos[0] = arg0->unk40->unk10;
    pos[1] = arg0->unk40->unk14 + 100.0f;
    pos[2] = arg0->unk40->unk18;
    func_ovl0E_13_80045EB0(arg0, 0x01000000);
    func_ovl0E_13_80046450(7, pos, 0);
    pos[1] += 100.0f;
    func_ovl0E_13_80046450(13, pos, 0);
    func_ovl0E_13_800465A8(180, pos);
}

void func_ovl0E_13_80044F30(UnkStructA *arg0) {
    func_ovl0E_13_80045EB0(arg0, 0x04000000);
    func_ovl0E_13_80046018(10);
    func_ovl0E_13_80045530(arg0->unk40, &arg0->unk40->unk1C, &arg0->unk40->unk20);
    func_ovl0E_13_80046580(30);
    arg0->unk40->unk28 *= 2.0f;
    arg0->unk40->unk30 *= 2.0f;
    arg0->unk40->unk2C *= 0.1f;
    arg0->unk40->unk14 += 20.0f;
    arg0->unk6C = 0.0f;
    arg0->unk70 = 0.0f;
    func_ovl0E_13_80046018(60);
}

void func_ovl0E_13_800449CC(void) {
    s16 moving;
    f32 h; /* fake: a held value filling the frame word between moving and rec (which is a guess: any float held here fills it, 1200.0f, 4.0f too) */
    UnkStruct_ovl0E_09_80045DCC *rec;
    UnkStructA *obj;
    s16 doneX;
    s16 doneZ;
    s32 flags;
    f32 pos[3];

    h = 100.0f;
    moving = 0;
    rec = func_ovl0E_13_80046020();
    rec->unk8 = func_ovl0E_13_80045EF0(0x1BB, 0x30, rec->unk14, rec->unk18, rec->unk1C);
    obj = rec->unk8;
    func_ovl0E_13_80045EA8(obj, 0x0F000020);
    obj->unk30 = 1;
    obj->unk34 = 70.0f;
    obj->unk68 = 0.0f;
    obj->unk38 = 50.0f;
    obj->unk70 = 1.6333334f;
    rec->unk10 = 0;
    /* the set-up in the loop's header: on lines of their own, IDO schedules
     * the table's address after it */
    for (doneZ = 0, doneX = 0, rec->unkE = rec->unkC;;) {
        if (D_ovl0E_13_80046B5C[rec->unkE].unk14 == 0) {
            doneX = 1;
        }
        if (D_ovl0E_13_80046B5C[rec->unkE].unk14 == 1 && D_ovl0E_13_80046B5C[rec->unkE].unkC <= obj->unk40->unk10) {
            obj->unk40->unk10 = D_ovl0E_13_80046B5C[rec->unkE].unkC;
            doneX = 1;
        }
        if (D_ovl0E_13_80046B5C[rec->unkE].unk14 == 2 && obj->unk40->unk10 <= D_ovl0E_13_80046B5C[rec->unkE].unkC) {
            obj->unk40->unk10 = D_ovl0E_13_80046B5C[rec->unkE].unkC;
            doneX = 1;
        }
        if (D_ovl0E_13_80046B5C[rec->unkE].unk16 == 0) {
            doneZ = 1;
        }
        if (D_ovl0E_13_80046B5C[rec->unkE].unk16 == 1 && D_ovl0E_13_80046B5C[rec->unkE].unk10 <= obj->unk40->unk18) {
            obj->unk40->unk18 = D_ovl0E_13_80046B5C[rec->unkE].unk10;
            doneZ = 1;
        }
        if (D_ovl0E_13_80046B5C[rec->unkE].unk16 == 2 && obj->unk40->unk18 <= D_ovl0E_13_80046B5C[rec->unkE].unk10) {
            obj->unk40->unk18 = D_ovl0E_13_80046B5C[rec->unkE].unk10;
            doneZ = 1;
        }
        if (doneX == 1 && doneZ != 0) {
            rec->unkE++;
            if (rec->unkE >= 15) {
                rec->unkE = 0;
            }
        }
        obj->unk40->unk20 = D_ovl0E_13_80046B5C[rec->unkE].unk8;
        if (moving == 1) {
            obj->unk40->unk10 += D_ovl0E_13_80046B5C[rec->unkE].unk0;
            obj->unk40->unk18 += D_ovl0E_13_80046B5C[rec->unkE].unk4;
        }
        func_ovl0E_13_80045F28(obj);
        if (obj->unk6C == 0.0f && obj->unk68 == 0.0f) {
            moving = 1;
        }
        if (rec->unk10 > 0) {
            rec->unk10--;
        }
        if (rec->unk10 <= 0) {
            pos[0] = obj->unk40->unk10;
            pos[1] = obj->unk40->unk14;
            pos[2] = obj->unk40->unk18;
            if (func_ovl0E_13_80045E50(pos, D_ovl0E_13_80046D50) < 1200.0f) {
                func_ovl0E_13_800465A8(0x77, pos);
                rec->unk10 = 30;
            }
        }
        func_ovl0E_13_80045FB8(obj);
        flags = func_ovl0E_13_80045E58(obj, 0xC0);
        if (flags != 0) {
            if (flags & 0x6000) {
                func_ovl0E_13_80046448(0x14, obj->unk40->unk10, obj->unk40->unk14 + h, obj->unk40->unk18, 0);
                func_ovl0E_13_80046448(0x12, obj->unk40->unk10, obj->unk40->unk14 + h, obj->unk40->unk18, 0);
                func_ovl0E_13_800465A0(0x168, obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18);
                pos[0] = obj->unk40->unk10;
                pos[1] = obj->unk40->unk14;
                pos[2] = obj->unk40->unk18;
                func_ovl0E_13_80046560(pos);
                func_ovl0E_13_80044848(rec, obj);
            } else {
                func_ovl0E_13_800465A0(0x26, obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18);
            }
        }
        if (obj->unk40->unk14 <= func_ovl0E_13_800454D0()) {
            func_ovl0E_13_80044894(rec);
        }
        doneZ = 0;
        doneX = 0;
        func_ovl0E_13_80046040();
    }
}

void func_ovl0E_13_80044894(UnkStruct_ovl0E_09_80045DCC *arg0) {
    UnkStructA *obj;
    f32 floor;

    floor = func_ovl0E_13_800454D0();
    obj = arg0->unk8;
    while (TRUE) {
        obj->unk40->unk14 -= 4.0f;
        if (obj->unk40->unk14 <= floor - 200.0f || floor == -25500.0f) {
            func_ovl0E_13_80044848(arg0, obj);
        }
        func_ovl0E_13_80046040();
    }
}

void func_ovl0E_13_80044848(UnkStruct_ovl0E_09_80045DCC *arg0, UnkStructA *arg1) {
    func_ovl0E_13_80045EB0(arg1, 0x01000000);
    func_ovl0E_13_80045EC0(arg1);
    arg0->unk4 = -1;
    arg0->unk8 = NULL;
    func_ovl0E_13_80046008();
}

void func_ovl0E_13_8004439C(void) {
    UnkStructA *obj;
    u32 (*p)[3];
    s32 i;
    s32 count;

    func_ovl0E_13_80044324();
    func_ovl0E_13_80045590(func_ovl0E_13_80044FF4);
    func_ovl0E_13_800455E0(func_ovl0E_13_80044F30);
    func_ovl0E_13_80046360(D_ovl0E_13_800467A0);
    func_ovl0E_13_800454A8(0x1A2);
    obj = func_ovl0E_13_80045EF0(0x1A3, 0x50, 0.0f, 0.0f, 0.0f);
    func_ovl0E_13_80045EA8(obj, 0x01000000);
    p = D_ovl0E_13_800467C4;
    count = 5;
    for (i = 0; i < count; i++) {
        func_ovl0E_13_80046508((*p)[0], (*p)[1], (*p)[2]);
        p++;
    }
    func_ovl0E_13_800454C0(0x1A9);
    func_ovl0E_13_80045658();
    func_ovl0E_13_80044294();
    func_ovl0E_13_8004424C();
    if (func_ovl0E_13_80046280(0x40E) == 0) {
        func_ovl0E_13_80044158();
    } else {
        func_ovl0E_13_80043110();
    }
    func_ovl0E_13_80045BF0(obj->unk40, 0, 0, 0, 2, 0x400);
    func_ovl0E_13_80046338(0x37A, 0x3DE, 0xFF, 0x87, 0x1F);
    func_ovl0E_13_80046348(0xFF, 0xA1, 0x1D);
}

void func_ovl0E_13_80044780(void) {
    UnkStruct_ovl0E_09_80045DCC *rec;
    s16 i;

    while (TRUE) {
        rec = D_ovl0E_13_80046A3C;
        for (i = 0; i < 9; i++, rec++) {
            if (rec->unk8 == NULL) {
                break;
            }
        }
        if (i < 9) {
            rec->unk4 = func_ovl0E_13_80045FF8(func_ovl0E_13_800449CC, rec, 0, 0, 0x110);
            func_ovl0E_13_80046018(300);
        }
        func_ovl0E_13_80046040();
    }
}

void func_ovl0E_13_800446F0(void) {
    UnkStruct_ovl0E_09_80045DCC *rec;
    s16 i;

    func_ovl0E_13_8004424C();
    D_ovl0E_13_80046A38 = -1;
    for (i = 0, rec = D_ovl0E_13_80046A3C; i < 9; i++, rec++) {
        if (rec->unk8 == NULL) {
            rec->unk4 = func_ovl0E_13_80045FF8(func_ovl0E_13_800449CC, rec, 0, 0, 0x110);
        }
    }
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl0E_13_80044590(s32 arg0) {
    switch (arg0) {
        case 2:
            func_ovl0E_13_8004439C();
            break;
        case 17:
            func_ovl0E_13_800440C0();
            break;
        case 18:
            func_ovl0E_13_80044070();
            break;
        case 4:
            func_ovl0E_13_80044008();
            break;
        case 5:
            func_ovl0E_13_80043E2C();
            break;
        case 0x50:
            func_ovl0E_13_80043CD4();
            break;
        case 3:
            func_ovl0E_13_80043C28();
            break;
        case 12:
            func_ovl0E_13_800430E8();
            break;
        case 1:
            func_ovl0E_13_800430A8();
            break;
        case 0x52:
            break;
        case 0x53:
            break;
    }
}

/* the names src/overlays/common/flags.h's code has here */
#define FLAGS_80046FDC func_ovl0E_13_80044324
#define FLAGS_8004701C func_ovl0E_13_80044364
#define FLAGS_80048694 func_ovl0E_13_80044F30
#define FLAGS_80048754 func_ovl0E_13_80044FF4
#define FLAGS_80048EA0 func_ovl0E_13_80045590
#define FLAGS_80048EF0 func_ovl0E_13_800455E0
#define FLAGS_80049B88 func_ovl0E_13_80046278
#include "overlays/common/flags.h"

void func_ovl0E_13_8004431C(void) {
}

void func_ovl0E_13_80044294(void) {
    s32 i;

    for (i = 0; i < 1; i++) {
        func_ovl0E_13_800437C0(&D_ovl0E_13_80046800[i]);
        func_ovl0E_13_80045EA8(D_ovl0E_13_80046800[i].unk0, 0x02000000);
        func_ovl0E_13_80045EB0(D_ovl0E_13_80046800[i].unk0, 0x80000);
    }
}

void func_ovl0E_13_8004424C(void) {
    s16 i;

    D_ovl0E_13_80046A38 = -1;
    for (i = 0; i < 9; i++) {
        D_ovl0E_13_80046A3C[i].unk0 = i;
        D_ovl0E_13_80046A3C[i].unk4 = -1;
        D_ovl0E_13_80046A3C[i].unk8 = NULL;
    }
}

void func_ovl0E_13_80044158(void) {
    s32 i;

    func_ovl0E_13_80044108();
    for (i = 0; i < 4; i++) {
        if (func_ovl0E_13_80046280(D_ovl0E_13_800469F8[i].unk0) != 0) {
            func_ovl0E_13_800437C0(&D_ovl0E_13_80046918[i]);
            func_ovl0E_13_80045EA8(D_ovl0E_13_80046918[i].unk0, 0x02000000);
            func_ovl0E_13_80045EB0(D_ovl0E_13_80046918[i].unk0, 0x80000);
        } else {
            func_ovl0E_13_800437C0(&D_ovl0E_13_80046838[i]);
            func_ovl0E_13_80045EA8(D_ovl0E_13_80046838[i].unk0, 0x02000000);
            func_ovl0E_13_80045EB0(D_ovl0E_13_80046838[i].unk0, 0x80000);
        }
    }
}

void func_ovl0E_13_80044108(void) {
    if (func_ovl0E_13_80046280(0x400) == 0) {
        D_ovl0E_13_80046DA4 = func_ovl0E_13_80045EF0(-1, 0x80, 0.0f, 0.0f, 0.0f);
        func_ovl0E_13_80046270(0x400);
    }
}

void func_ovl0E_13_800440C0(void) {
    func_ovl0E_13_80045660(0x11, 4, 0x1B);
    func_ovl0E_13_80044048(1, 0, 0, 0.0f, 0);
}

void func_ovl0E_13_80044070(void) {
    func_ovl0E_13_80045660(0x2B, 0, 0x1E);
    func_ovl0E_13_80044048(2, 0, 0x80, 0.0f, 3);
}

void func_ovl0E_13_80044048(s32 arg0, s32 arg1, s32 arg2, f32 arg3, s32 arg4) {
    D_ovl0E_13_80046D90.unk0 = arg0;
    D_ovl0E_13_80046D90.unk4 = arg1;
    D_ovl0E_13_80046D90.unk8 = arg2;
    D_ovl0E_13_80046D90.unkC = arg3;
    D_ovl0E_13_80046D90.unk10 = arg4;
}

void func_ovl0E_13_80044008(void) {
    func_ovl0E_13_80043EF0();
    if (func_ovl0E_13_80046280(0x40E) != 0) {
        func_ovl0E_13_80043E54();
        func_ovl0E_13_800431E4();
    }
}

/* the names src/overlays/common/angle.h's code has here */
#define ANGLE_80043A30 func_ovl0E_13_80043EA8
#define ANGLE_80043A78 func_ovl0E_13_80043EF0
#define ANGLE_800458B0 func_ovl0E_13_800456A0
#define ANGLE_800458C8 func_ovl0E_13_800456B8
#define ANGLE_80045F18 func_ovl0E_13_80045D08
#define ANGLE_D_80046D28 D_ovl0E_13_80046D90
#include "overlays/common/angle.h"

void func_ovl0E_13_80043E54(void) {
    ((UnkStructA_40 *) D_ovl0E_13_80046800[0].unk0->unk40)->unk48->unk4->unk4 =
        func_ovl0E_13_80045BE8(D_ovl0E_13_80046800[0].unk0->unk40, 0) - 0.001f;
}

void func_ovl0E_13_80043E2C(void) {
    func_ovl0E_13_80043DF0();
    func_ovl0E_13_80043D78();
}

void func_ovl0E_13_80043DF0(void) {
    D_ovl0E_13_80046A38 = func_ovl0E_13_80045FF8(func_ovl0E_13_80044780, NULL, 0, 0, 0x110);
}

void func_ovl0E_13_80043D78(void) {
    UNK_MACRO_ovl0E_13_80043BE0();
    func_ovl0E_13_800456A8(D_ovl0E_13_80046D90.unk4, 3, 4.0f, D_ovl0E_13_80046D90.unk10, 0, 1, 1);
}

void func_ovl0E_13_80043C90(void) {
    if (func_ovl0E_13_80046280(0x40E) != 0) {
        func_ovl0E_13_80046458(D_ovl0E_13_80046D70);
    }
    func_ovl0E_13_800455C8();
}

void func_ovl0E_13_80043CD4(void) {
    func_ovl0E_13_80043CCC();
    if (func_ovl0E_13_80045670(0x2B, 0x1E, 0x2C, 0x1E) != 0) {
        D_ovl0E_13_80046D74 = 0x50;
        D_ovl0E_13_80046D78 = 0x15;
        func_ovl0E_13_800455A0(0x50, 0x15, 0);
    }
    if (func_ovl0E_13_80045670(0x16, 0x1E, 0x17, 0x1F) != 0) {
        func_ovl0E_13_80045EB0(D_ovl0E_13_80046800[0].unk0, 0x02000000);
        func_ovl0E_13_80043C90();
    }
    func_ovl0E_13_80043C88();
    func_ovl0E_13_80043C80();
    func_ovl0E_13_80043C78();
}

static void func_ovl0E_13_80043CCC(void) {
}

static void func_ovl0E_13_80043C88(void) {
}

static void func_ovl0E_13_80043C80(void) {
}

static void func_ovl0E_13_80043C78(void) {
}

void func_ovl0E_13_80043C28(void) {
    UNK_MACRO_ovl0E_13_80043BE0();
    func_ovl0E_13_80043A30();
}

void func_ovl0E_13_80043BE0(void) {
    UNK_MACRO_ovl0E_13_80043BE0();
}

void func_ovl0E_13_80043BD8(void) {
}

void func_ovl0E_13_80043A30(void) {
    UnkStruct_ovl0E_13_800469F8 *sw;
    s16 i;

    D_ovl0E_13_80046DA4->unk30 = 1;
    D_ovl0E_13_80046DA4->unk34 = 50.0f;
    D_ovl0E_13_80046DA4->unk38 = 50.0f;
    for (i = 0, sw = D_ovl0E_13_800469F8; i < 4; i++, sw++) {
        if (func_ovl0E_13_80046280(sw->unk0) == 0) {
            D_ovl0E_13_80046DA4->unk40->unk10 = sw->unk4 * 10.0f;
            D_ovl0E_13_80046DA4->unk40->unk14 = sw->unk8 * 10.0f;
            D_ovl0E_13_80046DA4->unk40->unk18 = sw->unkC * 10.0f;
            if (func_ovl0E_13_80045E58(D_ovl0E_13_80046DA4, 0x40) != 0) {
                func_ovl0E_13_800465A0(0x49, D_ovl0E_13_80046DA4->unk40->unk10, D_ovl0E_13_80046DA4->unk40->unk14,
                                       D_ovl0E_13_80046DA4->unk40->unk18);
                func_ovl0E_13_80046448(0xF, sw->unk4 * 10.0f, sw->unk8 * 10.0f + 10, sw->unkC * 10.0f, 0);
                func_ovl0E_13_80043940(i);
                func_ovl0E_13_80046270(sw->unk0);
                if (func_ovl0E_13_80043754() != 0) {
                    func_ovl0E_13_80043444();
                }
            }
        }
    }
}

void func_ovl0E_13_80043940(s16 arg0) {
    UnkStruct_ovl0C_01_8004A54C *e;
    UnkStruct_ovl0C_01_8004A54C *f;

    e = &D_ovl0E_13_80046918[arg0];
    f = &D_ovl0E_13_80046838[arg0];
    func_ovl0E_13_800437C0(e);
    func_ovl0E_13_80045EA8(e->unk0, 0x02000000);
    func_ovl0E_13_80046040();
    func_ovl0E_13_800437B8(f);
    /* the brace in "panels.c": the epilogue restores s0 before s1 */
}

s32 func_ovl0E_13_80043754(void) {
    if (func_ovl0E_13_80046280(0x40F) == 0 || func_ovl0E_13_80046280(0x410) == 0 ||
        func_ovl0E_13_80046280(0x411) == 0 || func_ovl0E_13_80046280(0x412) == 0) {
        return 0;
    }
    return 1;
}

void func_ovl0E_13_80043444(void) {
    f32 a;
    f32 y;
    f32 z;
    f32 c;
    f32 b;
    s16 i;

    func_ovl0E_13_80046018(30);
    func_ovl0E_13_80045618(1);
    func_ovl0E_13_80045610(0);
    func_ovl0E_13_80046018(30);
    func_ovl0E_13_80045E08();
    func_ovl0E_13_80045D50(0x7F);
    func_ovl0E_13_80045CE8(2200.0f, 1600.0f, 2800.0f);
    func_ovl0E_13_80045D08(135.0f, 30.0f, 3500.0f);
    func_ovl0E_13_80046018(30);
    func_ovl0E_13_80043244();
    func_ovl0E_13_80046018(30);
    UNK_MACRO_ovl0E_13_80043444();
    func_ovl0E_13_80046018(10);
    func_ovl0E_13_80046580(0x2C);
    func_ovl0E_13_80043110();
    func_ovl0E_13_80046018(120);
    func_ovl0E_13_80045E10();
    func_ovl0E_13_80045DD8(D_ovl0E_13_80046D50);
    func_ovl0E_13_80045D48(0x1F);
    func_ovl0E_13_80045618(0);
    func_ovl0E_13_80045610(1);
}

void func_ovl0E_13_80043244(void) {
    s16 i;
    s16 alpha;

    func_ovl0E_13_80046580(0x7A);
    func_ovl0E_13_80045EA8(D_ovl0E_13_80046800[0].unk0, 0x04000000);
    func_ovl0E_13_80046040();
    for (i = 0; i < 4; i++) {
        func_ovl0E_13_80045EA8(D_ovl0E_13_80046918[i].unk0, 0x80000);
        func_ovl0E_13_80046448(0xF, D_ovl0E_13_80046918[i].unk14 * 10.0f, D_ovl0E_13_80046918[i].unk18 * 10.0f + 10,
                               D_ovl0E_13_80046918[i].unk1C * 10.0f, 0);
    }
    for (alpha = 0xFF; alpha >= 0; alpha -= 30) {
        for (i = 0; i < 4; i++) {
            D_ovl0E_13_80046918[i].unk0->unk40->unk36 = alpha;
            func_ovl0E_13_80046040();
        }
    }
    func_ovl0E_13_80046018(10);
    func_ovl0E_13_800465A0(0x82, D_ovl0E_13_80046800[0].unk14 * 10.0f, D_ovl0E_13_80046800[0].unk18 * 10.0f,
                           D_ovl0E_13_80046800[0].unk1C * 10.0f);
    while (!(D_ovl0E_13_80046800[0].unk0->unk40->unkC & 0x40)) {
        func_ovl0E_13_80046040();
    }
    func_ovl0E_13_800431E4();
    func_ovl0E_13_80046270(0x40E);
}

void func_ovl0E_13_800431E4(void) {
    func_ovl0E_13_800454E8(0x13, 3, 0x1A, 0x1A, 0x1D, 0x11);
    func_ovl0E_13_800454E8(0x13, 4, 0x19, 0x1A, 0x1C, 0x10);
}

void func_ovl0E_13_80043110(void) {
    func_ovl0E_13_80043108(22.0f, 30, 3);
}

void func_ovl0E_13_800430E8(void) {
    func_ovl0E_13_800456F0(1);
}

void func_ovl0E_13_800430C8(void) {
    func_ovl0E_13_800456F0(1);
}

void func_ovl0E_13_800430A8(void) {
    func_ovl0E_13_80043018();
}

void func_ovl0E_13_80043018(void) {
    s16 i;

    /* one line, as in func_ovl0E_13_800449CC's loop */
    i = 0; do {
        if (D_ovl0E_13_80046A3C[i].unk4 != -1) {
            func_ovl0E_13_80046000(D_ovl0E_13_80046A3C[i].unk4);
        }
        i++;
    } while (i < 9);
    if (D_ovl0E_13_80046A38 != -1) {
        func_ovl0E_13_80046000(D_ovl0E_13_80046A38);
    }
}

void func_ovl0E_13_80043010(void) {
}
