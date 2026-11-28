#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_10_05's game object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse). The task functions
 * (8004329C, 80044288, 80045394-80045488, with the strings "trap task:entry
 * failed" and "task_entry>%d:%s") and the movers (80044B88-80044DD0) are
 * ovl_10_02's. 80043718 is defined first: its initializers come first in
 * .data, and it still comes out where its caller 800438D0 pulls it out.
 * 8004329C is defined above its caller 80045450 (only so does its string
 * come first in .rodata) and 80043314 above its caller 800433A8: both still
 * come out where the original has them, which they don't at their own
 * places. 80044288 is defined just below 800442A4, which inlines it.
 */

#define ZJ_0100 func_ovl10_05_800456C8
#define ZJ_0C00 func_ovl10_05_80045A08
#define ZJ_0C01 func_ovl10_05_80045A10
#define ZJ_0D00 func_ovl10_05_80045A20
#define ZJ_0D15 func_ovl10_05_80045AC8
#define ZJ_0D16 func_ovl10_05_80045AD0
#define ZJ_0E03 func_ovl10_05_80045AF0
#define ZJ_0F00 func_ovl10_05_80045B00
#define ZJ_0F08 func_ovl10_05_80045B40
#define ZJ_1000 func_ovl10_05_80045BB8
#define ZJ_1001 func_ovl10_05_80045BC0
#define ZJ_1003 func_ovl10_05_80045BD0
#define ZJ_1006 func_ovl10_05_80045BE8
#define ZJ_1009 func_ovl10_05_80045C00
#define ZJ_100A func_ovl10_05_80045C08
#define ZJ_100C func_ovl10_05_80045C18
#define ZJ_1012 func_ovl10_05_80045C48
#define ZJ_1013 func_ovl10_05_80045C50
#define ZJ_1017 func_ovl10_05_80045C70
#define ZJ_1023 func_ovl10_05_80045CD0
#define ZJ_1505 func_ovl10_05_80046110
#define ZJ_1506 func_ovl10_05_80046118
#define ZJ_1509 func_ovl10_05_80046130
#define ZJ_150C func_ovl10_05_80046148
#define ZJ_150F func_ovl10_05_80046160
#define ZJ_1704 func_ovl10_05_80046248
#define ZJ_1708 func_ovl10_05_80046268
#define ZJ_1709 func_ovl10_05_80046270
#define ZJ_1711 func_ovl10_05_800462B0
void func_ovl10_05_80046398(Vec3f *, Vec3f *, Vec3f *); /* 0x1803, kept: its callers pass Vec3f * (the header's f32 *) */
void func_ovl10_05_800463A0(Vec3f *, Vec3f *); /* 0x1804, kept: its callers pass Vec3f * (the header's f32 *) */
f32 func_ovl10_05_800463B0(Vec3f *, Vec3f *); /* 0x1806, kept: its callers pass Vec3f * (the header's f32 *) */
#define ZJ_1807 func_ovl10_05_800463B8
#define ZJ_1811 func_ovl10_05_80046408
#define ZJ_1812 func_ovl10_05_80046410
#define ZJ_1814 func_ovl10_05_80046420
#define ZJ_1816 func_ovl10_05_80046430
#define ZJ_1818 func_ovl10_05_80046440
#define ZJ_181C func_ovl10_05_80046460
#define ZJ_1821 func_ovl10_05_80046488
#define ZJ_182B func_ovl10_05_800464D8
#define ZJ_1834 func_ovl10_05_80046520
#define ZJ_1900 func_ovl10_05_80046558
#define ZJ_1904 func_ovl10_05_80046578
#define ZJ_1909 func_ovl10_05_800465A0
#ifdef TARGET_PC
UnkStruct_802B0260 *func_ovl10_05_800465E0(s32 arg0); /* ZeroJump 0x1A00, func_80287DC0 */
#else
UnkStruct_802B0260 *func_ovl10_05_800465E0(void);
#endif
#define ZJ_1A02 func_ovl10_05_800465F0
#define ZJ_1D00 func_ovl10_05_80046700
#define ZJ_1D01 func_ovl10_05_80046708
#define ZJ_1D03 func_ovl10_05_80046718
s32 *func_ovl10_05_800467F0(void); /* 0x1E0A, kept: called with 0 arguments (the definition takes 1) */
#define ZJ_2004 func_ovl10_05_80046858
#define ZJ_2300 func_ovl10_05_800468C0
#define ZJ_2600 func_ovl10_05_800469A8
void func_ovl10_05_800469B0(s32, Vec3f *, s32); /* 0x2601, kept: its callers pass Vec3f * (the header's f32 *) */
#define ZJ_2900 func_ovl10_05_80046A50
#define ZJ_2A00 func_ovl10_05_80046A68
#define ZJ_2B00 func_ovl10_05_80046AE0
#define ZJ_2B04 func_ovl10_05_80046B00
#include "overlays/common/zerojump.h"
void func_ovl10_05_80046B08(s32, Vec3f *); /* 0x2B05, kept: its callers pass Vec3f * (the header's f32 *) */

void func_ovl10_05_80043010(void);
void func_ovl10_05_80043028(void);
void func_ovl10_05_80043084(void);
void func_ovl10_05_80043174(void);
void func_ovl10_05_80043194(void);
void func_ovl10_05_800431EC(void);
void func_ovl10_05_80043244(void);
s32 func_ovl10_05_8004329C(void (*)(void));
void func_ovl10_05_80043314(void);
void func_ovl10_05_800433A8(void);
void func_ovl10_05_800433F0(void);
void func_ovl10_05_80043580(void);
void func_ovl10_05_800435AC(void);
void func_ovl10_05_8004360C(void);
void func_ovl10_05_80043688(); /* its caller passes an argument it doesn't take */
void func_ovl10_05_80043718(void);
void func_ovl10_05_800438D0(void);
void func_ovl10_05_80043A0C(s32);
void func_ovl10_05_80043AC0(s32);
void func_ovl10_05_80043B54(s32, s32);
void func_ovl10_05_80043C18(s32);
void func_ovl10_05_80043CC0(s32);
void func_ovl10_05_80043E00(void);
void func_ovl10_05_80044288(void);
void func_ovl10_05_800442A4(void);
void func_ovl10_05_800446BC(void);
void func_ovl10_05_800447B0(UnkStructA *);
void func_ovl10_05_800447D0(s32);
void func_ovl10_05_80044930(void);
void func_ovl10_05_80044B88(s32);
void func_ovl10_05_80044DD0(void);
void func_ovl10_05_80044F0C(UnkStructA *);
s32 func_ovl10_05_80045250(s32);
void func_ovl10_05_8004526C(UnkStructA *);
void func_ovl10_05_800452EC(UnkStructA *);
void func_ovl10_05_80045358(void);
void func_ovl10_05_80045394(void (*)(void));
void func_ovl10_05_800453B0(s32);
void func_ovl10_05_800453CC(s32);
s32 func_ovl10_05_800453E0(void (*)(void));
void func_ovl10_05_80045424(void (*)(void));
s32 func_ovl10_05_80045450(void (*)(void), const char *);
void func_ovl10_05_80045488(void);
s32 func_ovl10_05_80045578(s32, void *);
void func_ovl10_05_800455AC(f32);
void func_ovl10_05_800455F4(f32, f32, f32);
void func_ovl10_05_80045650(void);
static void func_ovl10_05_80045668(void);

static void func_ovl10_05_80043018(void);
static void func_ovl10_05_80043020(void);
static void func_ovl10_05_8004316C(void);

/* .bss */
u8 D_ovl10_05_80046EE0[0x24];
s32 D_ovl10_05_80046F04;
s32 D_ovl10_05_80046F08;
Vec3f D_ovl10_05_80046F10;
s32 D_ovl10_05_80046F1C;
s32 D_ovl10_05_80046F20;
/* the tasks: a countdown and a function each */
s32 D_ovl10_05_80046F28[64];
s32 D_ovl10_05_80047028; /* the running task */
void (*D_ovl10_05_80047030[64])(void);
UnkStruct_802B0260 *D_ovl10_05_80047130;
UnkStructA *D_ovl10_05_80047138[8];
UnkStructA *D_ovl10_05_80047158[8];
void *D_ovl10_05_80047178[8];
s32 D_ovl10_05_80047198;
s32 D_ovl10_05_800471A0[8];
s32 D_ovl10_05_800471C0[8];
s32 D_ovl10_05_800471E0[8];
UnkStructA *D_ovl10_05_80047200[2];
UnkStructA *D_ovl10_05_80047208[2];
UnkStructA *D_ovl10_05_80047210[2];
Vec3f D_ovl10_05_80047218[2];
s32 D_ovl10_05_80047230[2];
s32 D_ovl10_05_80047238[2];
s32 D_ovl10_05_80047240[2];
s32 D_ovl10_05_80047248[2];
s32 D_ovl10_05_80047250[2];
s32 D_ovl10_05_80047258[64];
f32 D_ovl10_05_80047358[2][10];
f32 D_ovl10_05_800473A8[2][10];

/* the first .data: its initializers */
void func_ovl10_05_80043718(void) {
    s32 i;
    f32 x[8] = { 1150.0f, 2060.0f, 2660.0f, 3550.0f, 1160.0f, 2070.0f, 2660.0f, 3550.0f };
    f32 z[2] = { 770.0f, 2330.0f };

    for (i = 0; i < 8; i++) {
        D_ovl10_05_80047138[i] = func_ovl10_05_80046430(0x1FE, 0x01080000, 0xD0, 0);
        D_ovl10_05_80047138[i]->unk40->unk10 = x[i];
        D_ovl10_05_80047138[i]->unk40->unk14 = 1240.0f;
        D_ovl10_05_80047138[i]->unk40->unk18 = z[i / 4];
        D_ovl10_05_80047138[i]->unk40->unkC |= 0x8000;
        func_ovl10_05_80046408(D_ovl10_05_80047138[i], 0x80000);
        func_ovl10_05_80046110(D_ovl10_05_80047138[i]->unk40, 1);
        func_ovl10_05_80046118(D_ovl10_05_80047138[i]->unk40, 0x1FF, 0, 0);
    }
}

/* .data */
s32 D_ovl10_05_80046D28[8] = { 0, 5, 4, 1, 2, 7, 6, 3 };
s32 D_ovl10_05_80046D48[8] = { 0, 20, 20, 10, 20, 20, 20, 30 };
s32 D_ovl10_05_80046D68[8] = { 90, 190, 310, 350, 480, 580, 780, 940 };
extern u32 D_ovl10_05_80046DF0[][3];

/* unused: the stubs IDO -O3 leaves at the end of the object */
static void func_ovl10_05_800456B8(void) {
}

static void func_ovl10_05_800456B0(void) {
}

static void func_ovl10_05_800456A8(void) {
}

static void func_ovl10_05_800456A0(void) {
}

static void func_ovl10_05_80045698(void) {
}

static void func_ovl10_05_80045690(void) {
}

static void func_ovl10_05_80045688(void) {
}

static void func_ovl10_05_80045680(void) {
}

static void func_ovl10_05_80045678(void) {
}

static void func_ovl10_05_80045670(void) {
}

/* unused (its body is dropped): the reference keeps the unused words in
 * .bss, which would otherwise be common */
static void func_ovl10_05_80045668(void) {
    D_ovl10_05_80046EE0[0] = 0;
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl10_05_80045650
#define CORE_FUNC_B func_ovl10_05_800455F4
#define CORE_FUNC_C func_ovl10_05_800455AC
#define CORE_FUNC_D func_ovl10_05_80045578
#define CORE_BSS_A0 D_ovl10_05_80046F04
#define CORE_BSS_A1 D_ovl10_05_80046F08
#define CORE_BSS_B D_ovl10_05_80046F10
#include "overlays/common/core.h"

/* runs the tasks, one frame each time round */
void func_ovl10_05_80045488(void) {
    s32 i;

    D_ovl10_05_80046F1C = 0;
    D_ovl10_05_80046F20 = 0;
    while (TRUE) {
        for (i = 0; i < 64; i++) {
            if (D_ovl10_05_80047030[i] != NULL) {
                D_ovl10_05_80047028 = i;
                if (D_ovl10_05_80046F28[i] == 0) {
                    D_ovl10_05_80047030[i]();
                } else {
                    D_ovl10_05_80046F28[i]--;
                }
            }
        }
        D_ovl10_05_80046F1C++;
        D_ovl10_05_80046F1C &= 3;
        func_ovl10_05_800465A0();
    }
}

/* defined above its caller 80045450: see the top */
s32 func_ovl10_05_8004329C(void (*arg0)(void)) {
    s32 found;
    s32 i;

    i = 0;
    found = -1;
    do {
        if (D_ovl10_05_80047030[i] == NULL) {
            D_ovl10_05_80047030[i] = arg0;
            found = i;
        }
        i++;
    } while (i < 64 && found == -1);
    if (found == -1) {
        func_ovl10_05_800456C8("trap task:entry failed\n");
    }
    return found;
}

s32 func_ovl10_05_80045450(void (*arg0)(void), const char *arg1) {
    s32 i;

    i = func_ovl10_05_8004329C(arg0);
    func_ovl10_05_800456C8("task_entry>%d:%s\n", i, arg1);
    return i;
}

void func_ovl10_05_80045424(void (*arg0)(void)) {
    D_ovl10_05_80047030[func_ovl10_05_800453E0(arg0)] = NULL;
}

s32 func_ovl10_05_800453E0(void (*arg0)(void)) {
    s32 i;
    s32 found;

    found = -1;
    i = 0;
    do {
        if (arg0 == D_ovl10_05_80047030[i]) {
            found = i;
        }
        i++;
    } while (found == -1 && i < 64);
    return found;
}

void func_ovl10_05_800453CC(s32 arg0) {
    D_ovl10_05_80047030[arg0] = NULL;
}

void func_ovl10_05_800453B0(s32 arg0) {
    D_ovl10_05_80046F28[D_ovl10_05_80047028] = arg0;
}

void func_ovl10_05_80045394(void (*arg0)(void)) {
    D_ovl10_05_80047030[D_ovl10_05_80047028] = arg0;
}

void func_ovl10_05_80045358(void) {
    s32 *p;

    p = func_ovl10_05_800467F0();
    if (*p <= 0 || *p == 0) {
        *p = 1;
    }
}

void func_ovl10_05_800452EC(UnkStructA *arg0) {
    s32 i;

    if (arg0->unk8 & 1) {
        func_ovl10_05_800462B0(16);
    }
    for (i = 0; i < 30; i++) {
        func_ovl10_05_80046488(arg0);
        func_ovl10_05_800465A0();
    }
}

void func_ovl10_05_8004526C(UnkStructA *arg0) {
    Vec3f pos;

    if (arg0->unk8 & 1) {
        func_ovl10_05_80046858(0xFF, 0xFF, 0xFF, 100.0f);
        func_ovl10_05_80046410(arg0, 0x01000000);
        func_ovl10_05_80046578(5);
        func_ovl10_05_80046858(0, 0, 0, 0.0f);
        func_ovl10_05_800464D8(arg0, &pos);
        func_ovl10_05_800469B0(12, &pos, 0);
    }
}

s32 func_ovl10_05_80045250(s32 arg0) {
    s32 ret;

    ret = 0;
    if ((arg0 & 0xF) != 0xF) {
        ret = 1;
    }
    return ret;
}

void func_ovl10_05_80044F0C(UnkStructA *arg0) {
    s32 i;

    func_ovl10_05_800462B0(15);
    i = 0;
    func_ovl10_05_80046130(arg0->unk40, 0x11, 0);
    func_ovl10_05_80046160(arg0->unk40, 4, 12);
    func_ovl10_05_80046160(arg0->unk40, 5, 13);
    func_ovl10_05_800469A8(0, arg0->unk40->unk10, arg0->unk40->unk14 + 100.0f, arg0->unk40->unk18, 0);
    arg0->unk68 = 40.0f;
    arg0->unk70 = 1.6333334f;
    func_ovl10_05_80046AE0(0x1E);
    while (TRUE) {
        func_ovl10_05_80046A50(arg0, 0, 0.0f, func_ovl10_05_80045250);
        if (arg0->unk40->unk14 > 700.0f) {
            arg0->unk40->unk14 = 700.0f;
            arg0->unk68 = 0.0f;
            arg0->unk6C = 0.0f;
        }
        arg0->unk40->unk1C += 20.0f;
        if (arg0->unk40->unk1C >= 360.0f) {
            arg0->unk40->unk1C = 0.0f;
        }
        arg0->unk40->unk10 -= func_ovl10_05_80046700(arg0->unk40->unk1C);
        func_ovl10_05_800465A0();
        if (++i >= 200) {
            break;
        }
        if (arg0->unk40->unk14 < 700.0f && arg0->unk6C == 0.0f) {
            break;
        }
    }
    if (arg0->unk40->unk1C >= 90.0f && arg0->unk40->unk1C < 270.0f) {
        arg0->unk40->unk1C = 180.0f;
        arg0->unk40->unk14 = func_ovl10_05_80045AC8(arg0->unk40) + 70.0f;
    } else {
        arg0->unk40->unk1C = 0.0f;
        arg0->unk40->unk14 = func_ovl10_05_80045AC8(arg0->unk40);
    }
    func_ovl10_05_80046130(arg0->unk40, 0x13, 0);
    func_ovl10_05_80046410(arg0, 0x0C000000);
    arg0->unk40->unk48->unk4[0].unk8 |= 0x20;
    arg0->unk40->unk48->unk4[0].unk8 |= 0x40;
    arg0->unk40->unk48->unk4[0].unk4 = func_ovl10_05_80046148(arg0->unk40, 0) - 0.1f;
    arg0->unk6C = 0.0f;
    arg0->unk70 = 0.0f;
    func_ovl10_05_80046578(0x50);
}

void func_ovl10_05_80044DD0(void) {
    s32 i;

    if (D_ovl10_05_80047198 == 180) {
        for (i = 0; i < 8; i++) {
            if (D_ovl10_05_80046D48[i] != 0) {
                D_ovl10_05_800471A0[i] = func_ovl10_05_80046718(D_ovl10_05_80046D48[i]) + D_ovl10_05_80046D68[i] + 180;
            } else {
                D_ovl10_05_800471A0[i] = D_ovl10_05_80046D68[i] + 180;
            }
        }
    }
    if (D_ovl10_05_80047198 >= 180) {
        for (i = 0; i < 8; i++) {
            if (D_ovl10_05_80047198 == D_ovl10_05_800471A0[i]) {
                func_ovl10_05_80044B88(D_ovl10_05_80046D28[i]);
            }
        }
    }
    func_ovl10_05_80044930();
    D_ovl10_05_80047198++;
    if (D_ovl10_05_80047198 == 1180) {
        D_ovl10_05_80047198 = 180;
    }
}

void func_ovl10_05_80044B88(s32 arg0) {
    s32 i;
    s32 sp54[2] = { 0xAB, 0xAC };
    s32 sp4C[2] = { 0xB8, 0xB9 };
    f32 sp2C[8] = { 700.0f, 1800.0f, 1300.0f, 2400.0f, 1150.0f, 2250.0f, 850.0f, 1950.0f };

    for (i = 0; i < 8; i++) {
        if (D_ovl10_05_80047158[i] == NULL) {
            D_ovl10_05_80047158[i] = func_ovl10_05_80046430(sp54[(s32)(arg0 * 0.25f)], 0x01000070, 0x30, 0);
            D_ovl10_05_80047158[i]->unk40->unk10 = -1600.0f;
            D_ovl10_05_80047158[i]->unk40->unk14 = 100.0f;
            D_ovl10_05_80047158[i]->unk40->unk18 = sp2C[arg0];
            D_ovl10_05_800471C0[i] = arg0;
            if ((s32)(arg0 * 0.25f) == 0) {
                D_ovl10_05_80047158[i]->unk30 = 1;
                D_ovl10_05_80047158[i]->unk34 = 150.0f;
                D_ovl10_05_80047158[i]->unk38 = 120.0f;
            } else {
                D_ovl10_05_80047158[i]->unk30 = 8;
                D_ovl10_05_80047158[i]->unk34 = 300.0f;
                D_ovl10_05_80047158[i]->unk38 = 1600.0f;
            }
            D_ovl10_05_80047178[i] = func_ovl10_05_80046460(sp4C[(s32)(arg0 * 0.25f)], 0x01200000, D_ovl10_05_80047158[i]);
            func_ovl10_05_80046408(D_ovl10_05_80047178[i], 0x80000);
            D_ovl10_05_800471E0[i] = 0;
            break;
        }
    }
}

void func_ovl10_05_80044930(void) {
    s32 i;
    UnkStructA *p;
    f32 lim; /* fake: a guess: any float held for the loop fills this slot (42.5f, 34.0f too) */
    f32 x;

    lim = 5600.0f;
    for (i = 0; i < 8; i++) {
        if (D_ovl10_05_80047158[i] != NULL && D_ovl10_05_80047198 % 2 == i % 2) {
            func_ovl10_05_800447D0(i);
            if (D_ovl10_05_800471C0[i] < 4) {
                x = D_ovl10_05_80047158[i]->unk40->unk10;
                D_ovl10_05_80047158[i]->unk40->unk10 = x + 42.5f;
            } else {
                D_ovl10_05_80047158[i]->unk40->unk10 += 34.0f;
            }
            p = func_ovl10_05_80046520(D_ovl10_05_80047158[i], 7);
            if (p != NULL) {
                if (p->unk8 & 1) {
                    p->unk10 = 0x20;
                }
                if (p->unk8 & 4) {
                    p->unk10 = 0x40;
                }
                if (p->unk8 & 2) {
                    p->unk10 = 0x20;
                }
            }
            if (D_ovl10_05_800471C0[i] < 4 && func_ovl10_05_800463B8(D_ovl10_05_80047158[i], 0x20) != 0) {
                D_ovl10_05_80047158[i]->unk40->unk10 = x;
            }
            if (D_ovl10_05_80047158[i]->unk40->unk10 > lim) {
                func_ovl10_05_800447B0(D_ovl10_05_80047158[i]);
                D_ovl10_05_80047158[i] = NULL;
            }
        }
    }
}

void func_ovl10_05_800447D0(s32 arg0) {
    Vec3f pos;
    Vec3f target;
    f32 dist;

    func_ovl10_05_800464D8(D_ovl10_05_80047158[arg0], &pos);
    if ((s32)(D_ovl10_05_800471C0[arg0] * 0.25f) == 1) {
        pos.x += 1600.0f;
    }
    func_ovl10_05_800464D8(func_ovl10_05_80045C48(func_ovl10_05_80045BE8()), &target);
    dist = func_ovl10_05_800463B0(&pos, &target);
    if (D_ovl10_05_800471E0[arg0] == 0 && dist <= 1000.0f) {
        D_ovl10_05_800471E0[arg0]++;
        if ((s32)(D_ovl10_05_800471C0[arg0] * 0.25f) == 0) {
            func_ovl10_05_80046AE0(0x92);
        } else {
            func_ovl10_05_80046AE0(0x93);
        }
    }
}

void func_ovl10_05_800447B0(UnkStructA *arg0) {
    func_ovl10_05_80046420(arg0);
}

void func_ovl10_05_800446BC(void) {
    s32 i;
    UnkStructA *obj;
    Vec3f angle;
    Vec3f a;
    Vec3f b;

    obj = func_ovl10_05_80045C48(0);
    if (obj->unk40->unk14 == 700.0f) {
        for (i = 0; i < 2; i++) {
            if (D_ovl10_05_80047208[i] != NULL) {
                func_ovl10_05_800464D8(D_ovl10_05_80047200[i], &a);
                func_ovl10_05_800464D8(D_ovl10_05_80047208[i], &b);
                func_ovl10_05_80046398(&a, &b, &angle);
                D_ovl10_05_80047200[i]->unk40->unk20 = angle.x;
            }
        }
    }
}

void func_ovl10_05_800442A4(void) {
    s32 k;
    Vec3f out;
    Vec3f pos;
    Vec3f dir;
    UnkStructA *p;
    f32 x;
    f32 z;

    k = D_ovl10_05_80047258[D_ovl10_05_80047028];
    switch (D_ovl10_05_80047248[k]) {
        case 10:
            if (D_ovl10_05_80047230[k] & 4) {
                dir.z = 70.0f;
                dir.x = D_ovl10_05_80047200[k]->unk40->unk20;
                dir.y = 0.0f;
                func_ovl10_05_800463A0(&dir, &out);
                x = D_ovl10_05_80047200[k]->unk40->unk10 + out.x;
                z = D_ovl10_05_80047200[k]->unk40->unk18 + out.z;
                func_ovl10_05_800469A8(0, x, D_ovl10_05_80047200[k]->unk40->unk14 + 115.0f, z, 0);
                D_ovl10_05_80047210[k] = func_ovl10_05_80046430(0x2D2, 0x11080040, 0xD0, 0);
                D_ovl10_05_80047210[k]->unk40->unk10 = D_ovl10_05_80047358[k][9];
                D_ovl10_05_80047210[k]->unk40->unk14 = D_ovl10_05_80047208[k]->unk40->unk14;
                D_ovl10_05_80047210[k]->unk40->unk18 = D_ovl10_05_800473A8[k][9];
                D_ovl10_05_80047210[k]->unk40->unk28 = 5.0f;
                D_ovl10_05_80047210[k]->unk40->unk2C = 5.0f;
                D_ovl10_05_80047210[k]->unk40->unk30 = 5.0f;
                D_ovl10_05_80047210[k]->unk30 = 1;
                D_ovl10_05_80047210[k]->unk34 = 10.0f;
                D_ovl10_05_80047210[k]->unk38 = 0.0f;
                p = func_ovl10_05_80046520(D_ovl10_05_80047210[k], 7);
                if (p != NULL) {
                    p->unk10 |= 0x40;
                    if (!(func_ovl10_05_80045C70() & 2) && func_ovl10_05_80045CD0(0) == 0) {
                        D_ovl10_05_80046F20 = 1;
                    }
                }
                func_ovl10_05_800464D8(D_ovl10_05_80047210[k], &pos);
                func_ovl10_05_80046B08(0x95, &pos);
            }
            break;
        case 9:
            if (D_ovl10_05_80047210[k] != NULL) {
                D_ovl10_05_80047210[k]->unk40->unk28 = 5.0f;
                D_ovl10_05_80047210[k]->unk40->unk2C = 5.0f;
                D_ovl10_05_80047210[k]->unk40->unk30 = 5.0f;
            }
            break;
        case 8:
            if (D_ovl10_05_80047210[k] != NULL) {
                D_ovl10_05_80047210[k]->unk40->unk28 = 5.0f;
                D_ovl10_05_80047210[k]->unk40->unk2C = 5.0f;
                D_ovl10_05_80047210[k]->unk40->unk30 = 5.0f;
            }
            break;
        case 7:
            if (D_ovl10_05_80047210[k] != NULL) {
                func_ovl10_05_80046420(D_ovl10_05_80047210[k]);
                D_ovl10_05_80047210[k] = NULL;
            }
            break;
        case 0:
            D_ovl10_05_80047248[k] = 11;
            D_ovl10_05_80047240[k]--;
            break;
    }
    D_ovl10_05_80047248[k]--;
    if (D_ovl10_05_80047240[k] == 0 && (D_ovl10_05_80047230[k] & 4)) {
        D_ovl10_05_80047230[k] &= ~4;
        func_ovl10_05_80043B54(k, 2);
    }
    if (D_ovl10_05_80047240[k] == 0) {
        func_ovl10_05_80044288();
    }
}

void func_ovl10_05_80044288(void) {
    D_ovl10_05_80047030[D_ovl10_05_80047028] = NULL;
}

void func_ovl10_05_80043E00(void) {
    s32 i;
    s32 j;
    f32 dist;
    f32 speed;
    Vec3f player;
    Vec3f pos;
    Vec3f spC4;
    Vec3f angle;
    f32 a;
    f32 y; /* the height the positions are flattened to */

    y = 700.0f;
    func_ovl10_05_800464D8(func_ovl10_05_80045C48(0), &player);
    if (player.y == y || D_ovl10_05_80046F20 != 0) {
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 9; j++) {
                D_ovl10_05_80047358[i][10 - j - 1] = D_ovl10_05_80047358[i][10 - j - 2];
                D_ovl10_05_800473A8[i][10 - j - 1] = D_ovl10_05_800473A8[i][10 - j - 2];
            }
#ifdef TARGET_PC
            if (D_ovl10_05_80047208[i] == NULL) {
                /* not spawned yet (func_ovl10_05_80043CC0 below, when the
                   player comes near): the N64 reads ZeroJump's page,
                   mapped at 0 (port_null_word): unk40 is its word 0x40,
                   0, and the two floats its words 0x10 and 0x18 */
                union { u32 w; f32 f; } x, z;
                u32 m = port_null_word(0x40);
                x.w = port_null_word(m + 0x10);
                z.w = port_null_word(m + 0x18);
                D_ovl10_05_80047358[i][0] = x.f;
                D_ovl10_05_800473A8[i][0] = z.f;
            } else {
#endif
            D_ovl10_05_80047358[i][0] = D_ovl10_05_80047208[i]->unk40->unk10;
            D_ovl10_05_800473A8[i][0] = D_ovl10_05_80047208[i]->unk40->unk18;
#ifdef TARGET_PC
            }
#endif
            func_ovl10_05_800464D8(D_ovl10_05_80047200[i], &pos);
            pos.y = y;
            if (func_ovl10_05_800463B0(&player, &pos) < 1500.0f) {
                if (!(D_ovl10_05_80047230[i] & 1)) {
                    func_ovl10_05_80043CC0(i);
                }
                func_ovl10_05_800464D8(D_ovl10_05_80047208[i], &pos);
                pos.y = y;
                func_ovl10_05_80046398(&pos, &player, &angle);
                angle.y -= 90.0f;
                dist = func_ovl10_05_800463B0(&player, &pos);
                if (dist > 6.8f) {
                    speed = 6.8f;
                } else {
                    speed = dist;
                }
                if (dist < 1.0f) {
                    dist = 0.0f;
                }
                a = angle.x;
                D_ovl10_05_80047208[i]->unk40->unk10 += speed * func_ovl10_05_80046700(a);
                D_ovl10_05_80047208[i]->unk40->unk18 += speed * func_ovl10_05_80046708(a);
                if (D_ovl10_05_80047230[i] & 2) {
                    if (D_ovl10_05_80047238[i] > 0) {
                        D_ovl10_05_80047238[i]--;
                    }
                    if (D_ovl10_05_80047238[i] == 0 && !(D_ovl10_05_80047230[i] & 4)) {
                        func_ovl10_05_80043C18(i);
                    }
                }
                if (dist == 0.0f && !(D_ovl10_05_80047230[i] & 2)) {
                    func_ovl10_05_80043B54(i, 1);
                    if (!(D_ovl10_05_80047230[i] & 4)) {
                        D_ovl10_05_80047238[i] = 60;
                        D_ovl10_05_80047230[i] |= 2;
                        func_ovl10_05_800464D8(D_ovl10_05_80047200[i], &spC4);
                        func_ovl10_05_80046B08(0x94, &spC4);
                    }
                }
            } else if (D_ovl10_05_80047230[i] & 1) {
                func_ovl10_05_80043AC0(i);
            }
        }
    } else {
        for (i = 0; i < 2; i++) {
            if (D_ovl10_05_80047230[i] & 1) {
                func_ovl10_05_80043AC0(i);
            }
        }
    }
}

void func_ovl10_05_80043CC0(s32 arg0) {
    s32 i;

    D_ovl10_05_80047208[arg0] = func_ovl10_05_80046430(0xCC, 0x11080000, 0x10, 0);
    D_ovl10_05_80047208[arg0]->unk40->unk10 = D_ovl10_05_80047218[arg0].x;
    D_ovl10_05_80047208[arg0]->unk40->unk14 = D_ovl10_05_80047218[arg0].y;
    D_ovl10_05_80047208[arg0]->unk40->unk18 = D_ovl10_05_80047218[arg0].z;
    D_ovl10_05_80047208[arg0]->unk40->unk36 = 0xC0;
    D_ovl10_05_80047230[arg0] |= 1;
    for (i = 0; i < 10; i++) {
        D_ovl10_05_80047358[arg0][i] = D_ovl10_05_80047218[arg0].x;
        D_ovl10_05_800473A8[arg0][i] = D_ovl10_05_80047218[arg0].z;
    }
}

void func_ovl10_05_80043C18(s32 arg0) {
    D_ovl10_05_80047230[arg0] &= ~2;
    D_ovl10_05_80047240[arg0] = 4;
    D_ovl10_05_80047250[arg0] = 1;
    D_ovl10_05_80047248[arg0] = 10;
    D_ovl10_05_80047230[arg0] &= ~2;
    D_ovl10_05_80047230[arg0] |= 4;
    D_ovl10_05_80047210[arg0] = NULL;
    D_ovl10_05_80047258[func_ovl10_05_8004329C(func_ovl10_05_800442A4)] = arg0;
}

void func_ovl10_05_80043B54(s32 arg0, s32 arg1) {
    Vec3f pos;
    s32 id;

    if (arg1 == 1) {
        id = 0xCB;
    } else {
        id = 0xCC;
    }
    func_ovl10_05_800464D8(D_ovl10_05_80047208[arg0], &pos);
    func_ovl10_05_80046420(D_ovl10_05_80047208[arg0]);
    D_ovl10_05_80047208[arg0] = func_ovl10_05_80046430(id, 0x11080000, 0x10, 0);
    D_ovl10_05_80047208[arg0]->unk40->unk10 = pos.x;
    D_ovl10_05_80047208[arg0]->unk40->unk14 = pos.y;
    D_ovl10_05_80047208[arg0]->unk40->unk18 = pos.z;
    D_ovl10_05_80047208[arg0]->unk40->unk36 = 0xC0;
}

void func_ovl10_05_80043AC0(s32 arg0) {
    D_ovl10_05_80047218[arg0].x = D_ovl10_05_80047208[arg0]->unk40->unk10;
    D_ovl10_05_80047218[arg0].y = D_ovl10_05_80047208[arg0]->unk40->unk14;
    D_ovl10_05_80047218[arg0].z = D_ovl10_05_80047208[arg0]->unk40->unk18;
    func_ovl10_05_80046420(D_ovl10_05_80047208[arg0]);
    D_ovl10_05_80047208[arg0] = NULL;
    D_ovl10_05_80047230[arg0] = 0;
}

void func_ovl10_05_80043A0C(s32 arg0) {
    switch (arg0) {
        case 1:
            break;
        case 2:
            func_ovl10_05_800438D0();
            break;
        case 17:
            func_ovl10_05_80043244();
            break;
        case 18:
            func_ovl10_05_800431EC();
            break;
        case 19:
            func_ovl10_05_80043194();
            break;
        case 4:
            func_ovl10_05_80043174();
            break;
        case 0x50:
            func_ovl10_05_80043084();
            break;
    }
}

void func_ovl10_05_800438D0(void) {
    func_ovl10_05_800468C0(D_ovl10_05_80046DF0);
    func_ovl10_05_80046A68(0, 0, 0xDE);
    func_ovl10_05_80046A68(1, 0x11, 0xEF);
    func_ovl10_05_80046A68(2, 0x1A, 0xF8);
    func_ovl10_05_80046A68(3, 0x1B, 0xF9);
    func_ovl10_05_80045A08(0x1D4);
    func_ovl10_05_80045A10(0x1DE, 0.0f, 0.0f, 0.0f, 0x50, 0);
    func_ovl10_05_80045A10(0x1DF, 0.0f, 0.0f, 0.0f, 0x60, 0);
    func_ovl10_05_80045A10(0x1D5, 7600.0f, 0.0f, 0.0f, 0x50, 0);
    func_ovl10_05_80043718();
    func_ovl10_05_80043688(0x1FD);
    func_ovl10_05_80045A20(0x1E5);
    func_ovl10_05_80045AD0(0xF);
    func_ovl10_05_80045BB8();
    func_ovl10_05_8004360C();
    func_ovl10_05_800435AC();
    func_ovl10_05_80043580();
    func_ovl10_05_800433A8();
    func_ovl10_05_80045AF0(func_ovl10_05_800452EC);
}

/* its caller passes an argument (0x1FD) it hands on in $a0 */
#ifdef TARGET_PC
void func_ovl10_05_80043688(s32 arg0) {
    D_ovl10_05_80047130 = func_ovl10_05_800465E0(arg0);
#else
void func_ovl10_05_80043688() {
    D_ovl10_05_80047130 = func_ovl10_05_800465E0();
#endif
    func_ovl10_05_800465F0(D_ovl10_05_80047130, 0x20000001);
    D_ovl10_05_80047130->unk40 = 0.5f;
    D_ovl10_05_80047130->unk44 = D_ovl10_05_80047130->unk40 * 0.75f;
    D_ovl10_05_80047130->unk18 = 0.1f;
    D_ovl10_05_80047130->unk1C = -0.1f;
}

void func_ovl10_05_8004360C(void) {
    s32 i;

    for (i = 0; i < 64; i++) {
        D_ovl10_05_80047030[i] = NULL;
        D_ovl10_05_80046F28[i] = 0;
    }
    func_ovl10_05_80046558(func_ovl10_05_80045488, 0, 0, 0, 0x110);
}

void func_ovl10_05_800435AC(void) {
    s32 i;

    for (i = 0; i < 8; i++) {
        D_ovl10_05_80047158[i] = NULL;
    }
    D_ovl10_05_80047198 = 0;
    func_ovl10_05_80045B40(func_ovl10_05_80044F0C);
    func_ovl10_05_8004329C(func_ovl10_05_80044DD0);
}

void func_ovl10_05_80043580(void) {
    func_ovl10_05_800433F0();
    func_ovl10_05_8004329C(func_ovl10_05_800446BC);
}

void func_ovl10_05_800433F0(void) {
    f32 x[6] = { 1650.0f, 1650.0f, 1700.0f, 1700.0f, 1700.0f, 1700.0f };
    f32 z[6] = { 400.0f, 2700.0f, 500.0f, 2600.0f, 500.0f, 2600.0f };
    s32 i;

    for (i = 0; i < 2; i++) {
        D_ovl10_05_80047200[i] = func_ovl10_05_80046430(0xB1, 0x01000010, 0x10, 0);
        D_ovl10_05_80047200[i]->unk40->unk10 = x[i];
        D_ovl10_05_80047200[i]->unk40->unk14 = 900.0f;
        D_ovl10_05_80047200[i]->unk40->unk18 = z[i];
        func_ovl10_05_80046440(D_ovl10_05_80047200[i]);
        if (i == 1) {
            D_ovl10_05_80047200[i]->unk40->unk20 = 180.0f;
        }
    }
}

/* defined above its caller 800433A8: see the top */
void func_ovl10_05_80043314(void) {
    s32 i;
    f32 off[2] = { 500.0f, -500.0f };

    for (i = 0; i < 2; i++) {
        D_ovl10_05_80047218[i].x = D_ovl10_05_80047200[i]->unk40->unk10;
        D_ovl10_05_80047218[i].y = 710.0f;
        D_ovl10_05_80047218[i].z = D_ovl10_05_80047200[i]->unk40->unk18 + off[i];
        D_ovl10_05_80047208[i] = NULL;
    }
}

void func_ovl10_05_800433A8(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        D_ovl10_05_80047230[i] = 0;
    }
    func_ovl10_05_80043314();
    func_ovl10_05_8004329C(func_ovl10_05_80043E00);
}

/* passed to func_ovl10_05_800468C0: three 12-byte records and an end */
u32 D_ovl10_05_80046DF0[][3] = {
    { 0xA70001, 0xE, 4 },
    { 0xB70000, 0xE, 4 },
    { 0xA50003, 0xF, 4 },
    { 0, 0, 0 },
};

void func_ovl10_05_80043244(void) {
    func_ovl10_05_80045BC0(0x2C, 1, 10);
    func_ovl10_05_80045C00(0xC0);
    func_ovl10_05_80045C08(0xC0, 3, 8.5f, 4, 0, 1, 1);
}

void func_ovl10_05_800431EC(void) {
    func_ovl10_05_80045BC0(0x2C, 1, 21);
    func_ovl10_05_80045C00(0xC0);
    func_ovl10_05_80045C08(0xC0, 3, 8.5f, 4, 0, 1, 1);
}

void func_ovl10_05_80043194(void) {
    func_ovl10_05_80045BC0(3, 4, 15);
    func_ovl10_05_80045C00(0x40);
    func_ovl10_05_80045C08(0x40, 3, 8.5f, 4, 0, 1, 1);
}

void func_ovl10_05_80043174(void) {
    func_ovl10_05_8004316C();
    func_ovl10_05_80045C18();
}

static void func_ovl10_05_8004316C(void) {
}

void func_ovl10_05_80043084(void) {
    func_ovl10_05_80043028();
    func_ovl10_05_80045C50(1);
    if (func_ovl10_05_80045BD0(0x2C, 6, 0x2E, 0xE) != 0) {
        D_ovl10_05_80046F04 = 0x6B;
        D_ovl10_05_80046F08 = 0x13;
        func_ovl10_05_80045B00(0x6B, 0x13, 0);
    }
    if (func_ovl10_05_80045BD0(0x2C, 0x11, 0x2E, 0x19) != 0) {
        D_ovl10_05_80046F04 = 0x6B;
        D_ovl10_05_80046F08 = 0x14;
        func_ovl10_05_80045B00(0x6B, 0x14, 0);
    }
    if (func_ovl10_05_80045BD0(2, 0xF, 3, 0x10) != 0) {
        D_ovl10_05_80046F04 = 0x6D;
        D_ovl10_05_80046F08 = 0x11;
        func_ovl10_05_80045B00(0x6D, 0x11, 0);
    }
}

void func_ovl10_05_80043028(void) {
    s32 i;

    func_ovl10_05_80043018();
    func_ovl10_05_80043020();
    for (i = 0; i < 8; i++) {
        if (D_ovl10_05_80047138[i] != NULL) {
            func_ovl10_05_80046420(D_ovl10_05_80047138[i]);
            D_ovl10_05_80047138[i] = NULL;
        }
    }
}

static void func_ovl10_05_80043020(void) {
}

static void func_ovl10_05_80043018(void) {
}

void func_ovl10_05_80043010(void) {
}
