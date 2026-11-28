/*
 * ovl_0D_07's stage object, IDO -O3; it matches in us, eu and jp. Its
 * fakes and findings:
 * - (not a fake) the two statics 80043424 and 800432F0 are defined in
 *   another file, statics.h: the `.file` ugen emits where each is inlined
 *   ends an as1 block, which puts 8004342C's `mtc1 $zero` ahead of
 *   `sw $ra` and keeps both statics' stores ahead of the next call (it
 *   replaces the `if (!D && !D && !D) {}` fakes 8004342C and 800432F8 had).
 * - func_ovl0D_07_80043660, event 5: D_ovl0D_07_8004671C read into a
 *   block-local `k`, so it is its own live range and gets v0 (with the
 *   global compared directly it shares event 4's v1).
 * - event 11: the static 80043010 takes player 0 and the `obj` array and
 *   reads player 1's unk40 itself, after player 0's: p0 gets v1 and p1 a3,
 *   and player 1 is reloaded into a temporary, as in the original.
 * - 80043660's `pad[3]` (the frame's size) and the three empty statics
 *   80043030 calls (the stubs' placement).
 */
#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_0D_07's stage object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse; docs/DECOMPILING.md).
 */

void func_ovl0D_07_800452C0(s32, UnkStructA_40 **); /* 0x0D00, kept: called with 2 arguments (the definition takes 1) */
#define ZJ_0D03 func_ovl0D_07_800452D8
#define ZJ_0E03 func_ovl0D_07_80045390
#define ZJ_0F01 func_ovl0D_07_800453A8
#define ZJ_0F12 func_ovl0D_07_80045430
#define ZJ_0F14 func_ovl0D_07_80045440
#define ZJ_0F15 func_ovl0D_07_80045448
#define ZJ_1000 func_ovl0D_07_80045458
#define ZJ_1001 func_ovl0D_07_80045460
#define ZJ_1008 func_ovl0D_07_80045498
#define ZJ_1009 func_ovl0D_07_800454A0
#define ZJ_100C func_ovl0D_07_800454B8
#define ZJ_1012 func_ovl0D_07_800454E8
#define ZJ_1018 func_ovl0D_07_80045518
#define ZJ_101B func_ovl0D_07_80045530
#define ZJ_101C func_ovl0D_07_80045538
#define ZJ_1500 func_ovl0D_07_80045988
#define ZJ_1501 func_ovl0D_07_80045990
#define ZJ_1508 func_ovl0D_07_800459C8
#define ZJ_1509 func_ovl0D_07_800459D0
#define ZJ_150C func_ovl0D_07_800459E8
#define ZJ_150D func_ovl0D_07_800459F0
#define ZJ_150E func_ovl0D_07_800459F8
#define ZJ_1702 func_ovl0D_07_80045AD8
#define ZJ_1704 func_ovl0D_07_80045AE8
#define ZJ_1705 func_ovl0D_07_80045AF0
#define ZJ_1706 func_ovl0D_07_80045AF8
#define ZJ_1708 func_ovl0D_07_80045B08
#define ZJ_1709 func_ovl0D_07_80045B10
#define ZJ_1710 func_ovl0D_07_80045B48
#define ZJ_1711 func_ovl0D_07_80045B50
#define ZJ_1803 func_ovl0D_07_80045C38
f32 func_ovl0D_07_80045C50(Vec3f *, Vec3f *); /* 0x1806, kept: its callers pass Vec3f * (the header's f32 *) */
#define ZJ_1811 func_ovl0D_07_80045CA8
#define ZJ_1812 func_ovl0D_07_80045CB0
#define ZJ_1814 func_ovl0D_07_80045CC0
#define ZJ_1816 func_ovl0D_07_80045CD0
#define ZJ_181A func_ovl0D_07_80045CF0
#define ZJ_1821 func_ovl0D_07_80045D28
#define ZJ_1900 func_ovl0D_07_80045DF8
#define ZJ_1901 func_ovl0D_07_80045E00
#define ZJ_1902 func_ovl0D_07_80045E08
#define ZJ_1903 func_ovl0D_07_80045E10
#define ZJ_1904 func_ovl0D_07_80045E18
#define ZJ_1905 func_ovl0D_07_80045E20
#define ZJ_1909 func_ovl0D_07_80045E40
#define ZJ_190C func_ovl0D_07_80045E58
#define ZJ_190E func_ovl0D_07_80045E68
#define ZJ_190F func_ovl0D_07_80045E70
#define ZJ_1D01 func_ovl0D_07_80045FA8
#define ZJ_1D02 func_ovl0D_07_80045FB0
#define ZJ_1D03 func_ovl0D_07_80045FB8
#define ZJ_1D0F func_ovl0D_07_80046018
#define ZJ_1D10 func_ovl0D_07_80046020
#define ZJ_2201 func_ovl0D_07_80046148
#define ZJ_2600 func_ovl0D_07_80046248
#define ZJ_2B00 func_ovl0D_07_80046380
#define ZJ_2B02 func_ovl0D_07_80046390
#define ZJ_2B03 func_ovl0D_07_80046398
#define ZJ_2B04 func_ovl0D_07_800463A0
#define ZJ_2D00 func_ovl0D_07_80046428
#define ZJ_2D03 func_ovl0D_07_80046440
#define ZJ_2D04 func_ovl0D_07_80046448
#define ZJ_3000 func_ovl0D_07_800464F8
#define ZJ_3001 func_ovl0D_07_80046500
#include "overlays/common/zerojump.h"

void func_ovl0D_07_80043030(void);
void func_ovl0D_07_80043108(void);
void func_ovl0D_07_800432F8(void);
void func_ovl0D_07_8004342C(void);
void func_ovl0D_07_80043660(s32 arg0);
void func_ovl0D_07_80043D80(void);
static void func_ovl0D_07_80043028(void);
static void func_ovl0D_07_80043020(void);
static void func_ovl0D_07_80043018(void);
static f32 func_ovl0D_07_80043010(UnkStructA *obj0, UnkStructA **obj);
static void func_ovl0D_07_80043424(UnkStruct_ovl0C_01_8004A54C *e);
static void func_ovl0D_07_800432F0(UnkStruct_ovl0C_01_8004A54C *e);
void func_ovl0D_07_80044628(void);
void func_ovl0D_07_80044798(void);
void func_ovl0D_07_80044918(void);
void func_ovl0D_07_80044A98(void);
void func_ovl0D_07_80044AF0(void);
void func_ovl0D_07_80044754(UnkStructA *arg0);
void func_ovl0D_07_80044C90(void);

/* .bss */
u8 D_ovl0D_07_800466E0[0x24];
s32 D_ovl0D_07_80046704;
s32 D_ovl0D_07_80046708;
Vec3f D_ovl0D_07_80046710;
s32 D_ovl0D_07_8004671C;
UnkStructA_40 *D_ovl0D_07_80046720[5];
s32 D_ovl0D_07_80046734;
s32 D_ovl0D_07_80046738;
UnkStructA_40 *D_ovl0D_07_8004673C;

/* .data */
UnkStruct_ovl0C_01_8004A54C D_ovl0D_07_800465A0 = {
    NULL, -1, 0x16F, -1, -1, -1, -1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
};

#include "statics.h"

/* unused: the stubs IDO -O3 leaves at the end of the object; one of them
 * refers to the .bss nothing else uses, which would otherwise be common */
static void func_ovl0D_07_80044F4C(void) {
    D_ovl0D_07_800466E0[0] = 0;
}

static void func_ovl0D_07_80044F44(void) {
}

static void func_ovl0D_07_80044F3C(void) {
}

static void func_ovl0D_07_80044F34(void) {
}

static void func_ovl0D_07_80044F2C(void) {
}

static void func_ovl0D_07_80044F24(void) {
}

static void func_ovl0D_07_80044F1C(void) {
}

static void func_ovl0D_07_80044F14(void) {
}

static void func_ovl0D_07_80044F0C(void) {
}

static void func_ovl0D_07_80044F04(void) {
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl0D_07_80044EEC
#define CORE_FUNC_B func_ovl0D_07_80044E90
#define CORE_FUNC_C func_ovl0D_07_80044E48
#define CORE_FUNC_D func_ovl0D_07_80044E14
#define CORE_BSS_A0 D_ovl0D_07_80046704
#define CORE_BSS_A1 D_ovl0D_07_80046708
#define CORE_BSS_B D_ovl0D_07_80046710
#define CORE_ZJ_1704 func_ovl0D_07_80045AE8
#define CORE_ZJ_1708 func_ovl0D_07_80045B08
#define CORE_ZJ_1709 func_ovl0D_07_80045B10
#define CORE_ZJ_1904 func_ovl0D_07_80045E18
#define CORE_ZJ_2B04 func_ovl0D_07_800463A0
#include "overlays/common/core.h"

/* a task: a pulse at the object's place (the callback's argument) */
void func_ovl0D_07_80044C90(void) {
    UnkStructA *obj;
    UnkStructA_40 *p;
    s32 i;
    f32 a;

    obj = func_ovl0D_07_80045E20();
    p = func_ovl0D_07_80045988(0x183, 0);
    p->unkC |= 0x80;
    p->unk36 = 0x80;
    p->unk10 = obj->unk40->unk10;
    p->unk14 = obj->unk40->unk14;
    p->unk18 = obj->unk40->unk18;
    p->unk2C = 0.2f;
    func_ovl0D_07_800463A0(0xB3, p->unk10, p->unk14, p->unk18);
    a = 0.0f;
    for (i = 0; i != 20; i++) {
        p->unk2C = func_ovl0D_07_80045FA8(a) * 1.5f;
        a = a + 9.0f >= 360.0f ? a + 9.0f - 360.0f : a + 9.0f < 0.0f ? a + 9.0f + 360.0f : a + 9.0f;
        func_ovl0D_07_80045E40();
    }
    func_ovl0D_07_80045990(p);
    func_ovl0D_07_80045E08();
}

/* a task: player 2 floats up and down */
void func_ovl0D_07_80044AF0(void) {
    UnkStructA *obj;
    s32 i;
    f32 a;

    obj = func_ovl0D_07_800454E8(1);
    obj->unk68 = obj->unk6C;
    for (i = 0; i < 60; i++) {
        obj->unk40->unk14 -= 6.6666665f;
        func_ovl0D_07_80045E40();
    }
    a = 0.0f;
    for (i = 0; i < 120; i++) {
        obj->unk40->unk14 += 2.0f * func_ovl0D_07_80045FA8(a);
        a = a + 5.0f >= 360.0f ? a + 5.0f - 360.0f : a + 5.0f < 0.0f ? a + 5.0f + 360.0f : a + 5.0f;
        func_ovl0D_07_80045E40();
    }
    for (i = 0; i != 30; i++) {
        obj->unk68 = obj->unk6C;
        obj->unk40->unk14 += 26.666666f;
        func_ovl0D_07_80045E40();
    }
    while (TRUE) {
        obj->unk68 = obj->unk6C;
        func_ovl0D_07_80045E40();
    }
}

void func_ovl0D_07_80044A98(void) {
    if (D_ovl0D_07_8004671C == 0x10) {
        func_ovl0D_07_80045E18(0x280);
        func_ovl0D_07_80046390(10);
    } else {
        func_ovl0D_07_80045E18(45);
        func_ovl0D_07_80046390(18);
    }
    func_ovl0D_07_80045E08();
}

/* a task: a ring growing at the object's place (the task's argument) */
void func_ovl0D_07_80044918(void) {
    UnkStructA *obj;
    UnkStructA_40 *p;
    s32 i;

    obj = func_ovl0D_07_80045E20();
    p = func_ovl0D_07_80045988(7, 0);
    p->unkC |= 0x80;
    p->unk10 = obj->unk40->unk10;
    p->unk14 = obj->unk40->unk14;
    p->unk18 = obj->unk40->unk18;
    p->unk36 = 0xDC;
    p->unk28 = 0.1f;
    p->unk2C = 0.1f;
    p->unk30 = 0.1f;
    for (i = 0; i != 180; i++) {
        p->unk30 += 0.08888889f;
        p->unk28 = p->unk2C = p->unk30;
        p->unk36 = p->unk36 - 1.2222222f;
        if (p->unk36 < 0.0f) {
            p->unkC |= 1;
        }
        func_ovl0D_07_80045E40();
    }
    func_ovl0D_07_80045990(p);
    func_ovl0D_07_80045E08();
}

/* a task: the same, faster */
void func_ovl0D_07_80044798(void) {
    UnkStructA *obj;
    UnkStructA_40 *p;
    s32 i;

    obj = func_ovl0D_07_80045E20();
    p = func_ovl0D_07_80045988(7, 0);
    p->unkC |= 0x80;
    p->unk10 = obj->unk40->unk10;
    p->unk14 = obj->unk40->unk14;
    p->unk18 = obj->unk40->unk18;
    p->unk36 = 0xC8;
    p->unk28 = 0.1f;
    p->unk2C = 0.1f;
    p->unk30 = 0.1f;
    for (i = 0; i != 30; i++) {
        p->unk30 += 0.6666667f;
        p->unk28 = p->unk2C = p->unk30;
        p->unk36 = p->unk36 - 6.6666665f;
        if (p->unk36 < 0.0f) {
            p->unkC |= 1;
        }
        func_ovl0D_07_80045E40();
    }
    func_ovl0D_07_80045990(p);
    func_ovl0D_07_80045E08();
}

/* above its callback 80044754 in the source: defined with the other helpers,
 * the callbacks come out out of order */
void func_ovl0D_07_8004342C(void) {
    func_ovl0D_07_80043424(&D_ovl0D_07_800465A0);
    func_ovl0D_07_80045CB0(D_ovl0D_07_800465A0.unk0, 0x80000);
    D_ovl0D_07_8004673C = func_ovl0D_07_80045988(0x207, 0);
    D_ovl0D_07_8004673C->unkC |= 0x80;
    func_ovl0D_07_800452C0(0x170, &D_ovl0D_07_8004673C);
    func_ovl0D_07_80045458();
    func_ovl0D_07_80045390(func_ovl0D_07_80044754);
    func_ovl0D_07_800452D8(-100.0f);
    func_ovl0D_07_80046148(0, 0x28, 0x46);
    D_ovl0D_07_80046734 = 1;
    D_ovl0D_07_80046738 = 0;
    func_ovl0D_07_80046018();
}

/* the stage's callback (func_ovl0D_07_8004342C) */
void func_ovl0D_07_80044754(UnkStructA *arg0) {
    func_ovl0D_07_80045CB0(arg0, 0x01000000);
    func_ovl0D_07_80045DF8(func_ovl0D_07_80044C90, arg0, 0, 0, 0x150);
}

/* a task: one of the five pieces (D_ovl0D_07_80046720) flies off */
void func_ovl0D_07_80044628(void) {
    UnkStructA_40 *p;
    s32 i;
    f32 dx;
    f32 dz;

    p = func_ovl0D_07_80045E20();
    dx = (f32)func_ovl0D_07_80045FB0() * 10.0f;
    dz = (f32)func_ovl0D_07_80045FB0() * 10.0f;
    p->unkC |= 0x80;
    p->unk36 = 0xFF;
    for (i = -60; i != 20; i++) {
        p->unk14 = i * i * -0.11f + 400.0f;
        p->unk1C += dx;
        p->unk36 -= 3;
        p->unk24 += dz;
        func_ovl0D_07_80045E40();
    }
    p->unkC |= 1;
    func_ovl0D_07_80045E08();
}

/* a task: the stage's ending */
void func_ovl0D_07_80043D80(void) {
    UnkStructA *obj;
    UnkStructA *obj2;
    s32 i;
    s32 task;
    f32 y;

    obj = func_ovl0D_07_800454E8(0);
    func_ovl0D_07_80045538(0, 2);
    for (i = 0; i < 120; i++) {
        obj->unk40->unk18 -= 1.6666666f;
        if (D_ovl0D_07_8004671C != 0x1A && D_ovl0D_07_8004671C != 0x1B) {
            func_ovl0D_07_80045E40();
        }
    }
    task = func_ovl0D_07_80045DF8(func_ovl0D_07_80044AF0, NULL, 0, 0, 0x130);
    func_ovl0D_07_80045538(0, 0);
    func_ovl0D_07_80045E18(210);
    func_ovl0D_07_80045E70(0x70, 0x70);
    func_ovl0D_07_80045E18(120);
    func_ovl0D_07_80046380(0x221);
    func_ovl0D_07_80045CA8(D_ovl0D_07_800465A0.unk0, 0x04000000);
    func_ovl0D_07_80045E18(70);
    func_ovl0D_07_80046380(0x21F);
    for (i = 0; i < 5; i++) {
        func_ovl0D_07_80045DF8(func_ovl0D_07_80044628, D_ovl0D_07_80046720[i], 0, 0, 0x130);
    }
    obj->unk68 = 20.0f;
    obj->unk70 = 0.40833336f;
    while (obj->unk68 != 0.0f) {
        func_ovl0D_07_80045E40();
    }
    obj = func_ovl0D_07_800454E8(1);
    obj->unk40->unk10 = 2350.0f;
    if (D_ovl0D_07_8004671C == 0x1B) {
        obj->unk40->unk14 = 600.0f;
        obj->unk40->unk18 = 1655.0f;
    } else {
        obj->unk40->unk14 = 500.0f;
        obj->unk40->unk18 = 1550.0f;
    }
    obj->unk68 = 0.0f;
    func_ovl0D_07_80045E00(task);
    if (D_ovl0D_07_8004671C == 0x1A) {
        func_ovl0D_07_80045AE8(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18);
        func_ovl0D_07_80045B08(32768.0f, 32768.0f, 630.0f);
        func_ovl0D_07_80045E18(30);
    } else if (D_ovl0D_07_8004671C == 0x1B) {
        obj->unk70 = 0.40833336f;
        obj->unk6C = 8.166667f;
        func_ovl0D_07_80045AE8(obj->unk40->unk10, obj->unk40->unk14 - 50.0f, obj->unk40->unk18);
        func_ovl0D_07_80045B08(350.0f, 32768.0f, 800.0f);
        while (obj->unk40->unk14 > 400.0f) {
            func_ovl0D_07_80045E40();
        }
        func_ovl0D_07_80045538(1, 0x13);
        func_ovl0D_07_80046248(0, obj->unk40->unk10, obj->unk40->unk14 + 50.0f, obj->unk40->unk18, 0);
        func_ovl0D_07_80045530(1, 3);
        for (i = 0; i != 20; i++) {
            obj->unk40->unk18 += 7.5f;
            func_ovl0D_07_80045D28(obj);
            func_ovl0D_07_80045E40();
        }
        func_ovl0D_07_80045CB0(obj, 0x01000000);
        obj2 = func_ovl0D_07_80045CD0(obj->unk28, 0x01000000, 0xC0, 0);
        obj2->unk40->unk10 = obj->unk40->unk10;
        obj2->unk40->unk18 = obj->unk40->unk18;
        obj2->unk40->unk20 = obj->unk40->unk20;
        obj2->unk40->unk24 = obj->unk40->unk24;
        obj2->unk40->unk14 = 50.0f;
        obj2->unk40->unk1C = 90.0f;
        func_ovl0D_07_80045E18(30);
        func_ovl0D_07_800459C8(obj2->unk40, 2);
        func_ovl0D_07_800459D0(obj2->unk40, 1, 0);
        func_ovl0D_07_800459D0(obj2->unk40, 2, 1);
        obj2->unk40->unkC &= ~0x60;
        func_ovl0D_07_80045AE8(obj2->unk40->unk10 + 20.0f, obj2->unk40->unk14, obj2->unk40->unk18 + 80.0f);
        func_ovl0D_07_80045B08(340.0f, 60.0f, 800.0f);
        func_ovl0D_07_80045E18(60);
        func_ovl0D_07_800464F8();
        D_ovl0D_07_80046734 = 0;
        func_ovl0D_07_80045E08();
    } else {
        func_ovl0D_07_80045E18(30);
        func_ovl0D_07_80045AF0(NULL, &y, NULL);
        for (i = 0; i != 180; i++) {
            func_ovl0D_07_80045AE8(32768.0f, i * 1.6666666f + y, 32768.0f);
            func_ovl0D_07_80045E40();
        }
        func_ovl0D_07_80045B08(32768.0f, 32768.0f, 630.0f);
    }
    func_ovl0D_07_800459D0(obj->unk40, 0x1F, 0);
    func_ovl0D_07_800459D0(obj->unk40, -1, 1);
    obj->unk40->unk48->unk4[0].unk8 |= 0x20;
    obj->unk40->unk48->unk4[0].unk8 &= ~0x40;
    func_ovl0D_07_80046380(0x1B2);
    while (!(obj->unk40->unk48->unk4[0].unk8 & 0x40)) {
        func_ovl0D_07_80045E40();
    }
    if (D_ovl0D_07_8004671C == 0x1A) {
        func_ovl0D_07_80045E18(30);
        func_ovl0D_07_800464F8();
        D_ovl0D_07_80046734 = 0;
        func_ovl0D_07_80045E08();
    }
    func_ovl0D_07_80045E18(30);
    func_ovl0D_07_800459D0(obj->unk40, 0x16, 0);
    func_ovl0D_07_800459D0(obj->unk40, -1, 1);
    obj->unk40->unk48->unk4[0].unk8 |= 0x20;
    obj->unk40->unk48->unk4[0].unk8 &= ~0x40;
    func_ovl0D_07_80045E18(7);
    obj->unk68 = 20.0f;
    obj->unk70 = 1.6333334f;
    for (i = 0; i != 30; i++) {
        if (i == 5) {
            obj->unk40->unk48->unk4[0].unk8 |= 0x40;
        }
        obj->unk40->unk18 += 5.0f;
        func_ovl0D_07_80045E40();
    }
    func_ovl0D_07_80046448(1);
    func_ovl0D_07_80045AE8(2350.0f, 100.0f, 2020.0f);
    func_ovl0D_07_80045B08(0.0f, 45.0f, 2220.0f);
    func_ovl0D_07_80045B48(0x10);
    func_ovl0D_07_80045B50(0xF);
    while (obj->unk68 != 0.0f) {
        func_ovl0D_07_80045E40();
    }
    func_ovl0D_07_80045E18(30);
    func_ovl0D_07_80045E68(0x70, 0x70, 0);
    func_ovl0D_07_80045518(1);
    func_ovl0D_07_80045498(0);
    func_ovl0D_07_80045518(0);
    D_ovl0D_07_80046734 = 0;
    func_ovl0D_07_80045430(0x3E, 0x11);
    func_ovl0D_07_80045E58(func_ovl0D_07_80046440(1)->unkA0.h.lo);
    obj = func_ovl0D_07_800454E8(0);
    obj->unk68 = 0.0f;
    obj->unk70 = 1.6333334f;
    obj->unk6C = 16.333334f;
    obj = func_ovl0D_07_800454E8(1);
    obj->unk68 = 0.0f;
    obj->unk70 = 1.6333334f;
    obj->unk6C = 16.333334f;
    func_ovl0D_07_80045E08();
}

void func_ovl0D_07_800432F8(void) {
    s32 i;

    if (D_ovl0D_07_8004671C == 0x1A || D_ovl0D_07_8004671C == 0x1B) {
        func_ovl0D_07_80046500();
    }
    if (D_ovl0D_07_8004671C == 0x10 || D_ovl0D_07_8004671C == 0x1A || D_ovl0D_07_8004671C == 0x1B) {
        for (i = 0; i < 5; i++) {
            func_ovl0D_07_80045990(D_ovl0D_07_80046720[i]);
        }
    }
    func_ovl0D_07_800432F0(&D_ovl0D_07_800465A0);
    func_ovl0D_07_80045990(D_ovl0D_07_8004673C);
    if (D_ovl0D_07_80046738 == 0) {
        func_ovl0D_07_80046020();
    }
}

void func_ovl0D_07_80043108(void) {
    s32 i;
    UnkStructA *obj;

    func_ovl0D_07_80046380(0x224);
    func_ovl0D_07_80045460(0x17, 0, 0x17);
    func_ovl0D_07_800454A0(0);
    func_ovl0D_07_80046428(1, 1, 0x17, 2, 10);
    func_ovl0D_07_80046448(0);
    func_ovl0D_07_80045518(1);
    func_ovl0D_07_80045498(1);
    func_ovl0D_07_80045518(0);
    func_ovl0D_07_800454E8(0)->unk40->unk10 += 50.0f;
    obj = func_ovl0D_07_800454E8(1);
    obj->unk40->unk10 += 50.0f;
    obj->unk40->unk14 += 200.0f;
    func_ovl0D_07_80045E10(func_ovl0D_07_80046440(1)->unkA0.h.lo, 0);
    for (i = 0; i != 5; i++) {
        D_ovl0D_07_80046720[i] = func_ovl0D_07_80045988(0x171, 0);
        D_ovl0D_07_80046720[i]->unk10 = 2350.0f;
        D_ovl0D_07_80046720[i]->unk14 = 0.0f;
        D_ovl0D_07_80046720[i]->unk18 = i * 600.0f + 350.0f;
    }
    func_ovl0D_07_800454B8();
    func_ovl0D_07_80045B50(0x1F);
    func_ovl0D_07_80045AE8(2350.0f, 138.0f, 1694.0f);
    func_ovl0D_07_80045B08(0.0f, 10.0f, 1500.0f);
    func_ovl0D_07_80045498(1);
}

void func_ovl0D_07_80043030(void) {
    UnkStructA_40 *p;

    func_ovl0D_07_80043028(); /* the stubs' placement (above) */
    func_ovl0D_07_80043020();
    func_ovl0D_07_80043018();
    func_ovl0D_07_80045460(0x17, 0, 0x17);
    func_ovl0D_07_800454A0(0);
    D_ovl0D_07_800465A0.unk0->unk40->unk48->unk4[0].unk4 =
        func_ovl0D_07_800459E8(D_ovl0D_07_800465A0.unk0->unk40, 0) - 0.01f;
    func_ovl0D_07_80046428(1, 1, 0x17, 0, 0x11);
    p = func_ovl0D_07_800454E8(0)->unk40;
    p->unk10 += 50.0f;
    p = func_ovl0D_07_800454E8(1)->unk40;
    p->unk10 += 50.0f;
    func_ovl0D_07_800454B8();
    func_ovl0D_07_80045B50(0xF);
}

/* the two players face each other (event 11): the angle for player 1. A
 * static: its locals sit below the caller's and the spill, as in the original.
 * Player 1's unk40 is read here, after player 0's (see the top) */
static f32 func_ovl0D_07_80043010(UnkStructA *obj0, UnkStructA **obj) {
    UnkStructA_40 *p0;
    UnkStructA_40 *p1;
    Vec3f pos0;
    Vec3f pos1;
    Vec3f angle;

    p0 = obj0->unk40;
    p1 = obj[1]->unk40;
    pos0.x = p0->unk10;
    pos0.y = p0->unk14;
    pos0.z = p0->unk18;
    pos1.x = p1->unk10;
    pos1.y = p1->unk14;
    pos1.z = p1->unk18;
    func_ovl0D_07_80045C38(&pos0.x, &pos1.x, &angle.x);
    return angle.x;
}

/* three of the four stubs before func_ovl0D_07_80043030: statics inlined
 * everywhere. Called from 80043030 they come out there; which statics they
 * were, and who called them, isn't known (the first is 80043010's) */
static void func_ovl0D_07_80043018(void) {
}

static void func_ovl0D_07_80043020(void) {
}

static void func_ovl0D_07_80043028(void) {
}

/* the stage object's callback (the entry returns it): what each event does.
 * Last in the source, so it is processed first: it pulls out its callees
 * 80043030-8004342C ahead of itself, in the reverse order of their last calls,
 * which puts 80043010's stub (case 11 calls it last) with 80043030's three */
void func_ovl0D_07_80043660(s32 arg0) {
    UnkStructA *o;
    s32 pad[3]; /* fake: unused; a guess: the frame has three more words here (pass 13: they sit between o and i, so locals, not an inlined call's slots; the empty statics 80043018/20/28 called here instead: the frame unchanged; a third parameter on 80043010: its slots go below; the two face-away blocks as one static: not inlined; f32 x, y, z for the 0x1704/0x2600 arguments: the schedule moves; a dead `task =` store of a 0x1900 result fills one word, a guess) */
    s32 i;
    s32 kind;
    UnkStructA *obj[2];
    Vec3f angle;
    f32 dist;
    Vec3f mid;
    Vec3f pos;

    switch (arg0) {
        case 2:
            D_ovl0D_07_8004671C = func_ovl0D_07_800453A8();
            func_ovl0D_07_8004342C();
            break;
        case 1:
            func_ovl0D_07_800432F8();
            break;
        case 16:
        case 26:
        case 27:
            func_ovl0D_07_80043108();
            break;
        case 17:
            func_ovl0D_07_80043030();
            break;
        case 5:
            {
                s32 k = D_ovl0D_07_8004671C;

                if (k == 0x10 || 0x1A == k || k == 0x1B) {
                    while (D_ovl0D_07_80046734 != 0) {
                        func_ovl0D_07_80045E40();
                    }
                    func_ovl0D_07_80045498(0);
                }
            }
            break;
        case 4:
            if (D_ovl0D_07_8004671C != 0x1A && D_ovl0D_07_8004671C != 0x1B) {
                func_ovl0D_07_80045DF8(func_ovl0D_07_80044A98, NULL, 0, 0, 0x130);
            }
            if (D_ovl0D_07_8004671C == 0x10 || 0x1A == D_ovl0D_07_8004671C || D_ovl0D_07_8004671C == 0x1B) {
                func_ovl0D_07_80045DF8(func_ovl0D_07_80043D80, NULL, 0, 0, 0x130);
            }
            break;
        case 11:
            if (func_ovl0D_07_80045440(1) == 3) {
                break;
            }
            func_ovl0D_07_80045498(1);
            func_ovl0D_07_80045E18(30);
            obj[1] = func_ovl0D_07_800454E8(1);
            obj[0] = o = func_ovl0D_07_800454E8(0);
            obj[0]->unk40->unk20 = func_ovl0D_07_80043010(o, obj);
            func_ovl0D_07_80045AE8(obj[1]->unk40->unk10, obj[1]->unk40->unk14 + 50.0f, obj[1]->unk40->unk18);
            func_ovl0D_07_80045AD8(0);
            func_ovl0D_07_80045AF8(2350.0f, 300.0f, 1550.0f);
            func_ovl0D_07_80045AD8(1);
            mid.x = 2350.0f;
            mid.y = 0.0f;
            mid.z = 1550.0f;
            o = obj[1];
            pos.x = o->unk40->unk10;
            pos.y = o->unk40->unk14;
            pos.z = o->unk40->unk18;
            dist = func_ovl0D_07_80045C50(&mid, &pos);
            func_ovl0D_07_80045B10(&angle.x, NULL, NULL);
            if (dist <= 500.0f) {
                angle.x = angle.x + 180.0f >= 360.0f ? angle.x + 180.0f - 360.0f
                        : angle.x + 180.0f < 0.0f    ? angle.x + 180.0f + 360.0f
                                                     : angle.x + 180.0f;
            }
            obj[1]->unk40->unk20 = angle.x;
            func_ovl0D_07_80045B08(angle.x, 32768.0f, 32768.0f);
            func_ovl0D_07_80045448(50);
            func_ovl0D_07_80046398();
            func_ovl0D_07_80046380(0x21A);
            for (i = 0; i < 32; i++) {
                func_ovl0D_07_80045FB8(0x100);
                kind = (func_ovl0D_07_80045FB8(0x100) & 1) ? 0xC : 0x18;
                func_ovl0D_07_80046248(kind, (obj[1]->unk40->unk10 - 80.0f) + func_ovl0D_07_80045FB0() * 160.0,
                                       obj[1]->unk40->unk14 + func_ovl0D_07_80045FB0() * 50.0 + 50.0,
                                       (obj[1]->unk40->unk18 - 80.0f) + func_ovl0D_07_80045FB0() * 160.0, 0);
                func_ovl0D_07_80045E18(4);
            }
            func_ovl0D_07_80046248(0x14, obj[1]->unk40->unk10, obj[1]->unk40->unk14 + 100.0f, obj[1]->unk40->unk18, 0);
            func_ovl0D_07_80046380(0x1B5);
            func_ovl0D_07_80046380(0x21C);
            func_ovl0D_07_80045DF8(func_ovl0D_07_80044918, obj[1], 0, 0, 0x130);
            func_ovl0D_07_80045DF8(func_ovl0D_07_80044798, obj[1], 0, 0, 0x130);
            func_ovl0D_07_80045E18(10);
            func_ovl0D_07_80045CB0(obj[1], 0x01000000);
            for (i = 0; i != 60; i++) {
                func_ovl0D_07_80045B08(32768.0f, 45.0f, i * 16.666666f + 1500.0f);
                func_ovl0D_07_80045E40();
            }
            func_ovl0D_07_80045E18(60);
            func_ovl0D_07_800454E8(0);
            func_ovl0D_07_80045AE8(obj[0]->unk40->unk10, 50.0f, obj[0]->unk40->unk18);
            func_ovl0D_07_80045AD8(0);
            func_ovl0D_07_80045AF8(2350.0f, 300.0f, 1550.0f);
            func_ovl0D_07_80045AD8(1);
            mid.x = 2350.0f;
            mid.y = 0.0f;
            mid.z = 1550.0f;
            o = obj[0];
            pos.x = o->unk40->unk10;
            pos.y = o->unk40->unk14;
            pos.z = o->unk40->unk18;
            dist = func_ovl0D_07_80045C50(&mid, &pos);
            func_ovl0D_07_80045B10(&angle.x, NULL, NULL);
            if (dist <= 500.0f) {
                angle.x = angle.x + 180.0f >= 360.0f ? angle.x + 180.0f - 360.0f
                        : angle.x + 180.0f < 0.0f    ? angle.x + 180.0f + 360.0f
                                                     : angle.x + 180.0f;
            }
            obj[0]->unk40->unk20 = angle.x;
            func_ovl0D_07_80045B08(angle.x, 32768.0f, 32768.0f);
            func_ovl0D_07_80045498(0);
            func_ovl0D_07_80045448(32);
            func_ovl0D_07_80046020();
            D_ovl0D_07_80046738 = 1;
            break;
    }
}
