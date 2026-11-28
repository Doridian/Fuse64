#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_10_04's game object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse). It is ovl_10_02's object
 * (20D660.c) with other numbers: four of its setup functions don't call
 * 80044AC0 (ovl_10_02's 80044AE0), and 80043028 and 80043404 pass other
 * values. The task functions (800432B0-80043388, 80044234-80044344, with the
 * strings "trap task:entry failed" and "task_entry>%d:%s") are in every one
 * of bank 0x10's overlays.
 */

#define ZJ_0100 func_ovl10_04_80044588
#define ZJ_0C00 func_ovl10_04_800448C8
#define ZJ_0C01 func_ovl10_04_800448D0
#define ZJ_0D00 func_ovl10_04_800448E0
#define ZJ_0D15 func_ovl10_04_80044988
#define ZJ_0D16 func_ovl10_04_80044990
#define ZJ_0E03 func_ovl10_04_800449B0
#define ZJ_0F00 func_ovl10_04_800449C0
#define ZJ_0F08 func_ovl10_04_80044A00
#define ZJ_1000 func_ovl10_04_80044A78
#define ZJ_1001 func_ovl10_04_80044A80
#define ZJ_1003 func_ovl10_04_80044A90
#define ZJ_1006 func_ovl10_04_80044AA8
#define ZJ_100A func_ovl10_04_80044AC8
#define ZJ_100C func_ovl10_04_80044AD8
#define ZJ_1012 func_ovl10_04_80044B08
#define ZJ_1013 func_ovl10_04_80044B10
#define ZJ_1509 func_ovl10_04_80044FF0
#define ZJ_150C func_ovl10_04_80045008
#define ZJ_150F func_ovl10_04_80045020
#define ZJ_1704 func_ovl10_04_80045108
#define ZJ_1708 func_ovl10_04_80045128
#define ZJ_1709 func_ovl10_04_80045130
#define ZJ_1711 func_ovl10_04_80045170
f32 func_ovl10_04_80045270(Vec3f *, Vec3f *); /* 0x1806, kept: its callers pass Vec3f * (the header's f32 *) */
#define ZJ_1807 func_ovl10_04_80045278
#define ZJ_1811 func_ovl10_04_800452C8
#define ZJ_1812 func_ovl10_04_800452D0
#define ZJ_1814 func_ovl10_04_800452E0
#define ZJ_1816 func_ovl10_04_800452F0
#define ZJ_181C func_ovl10_04_80045320
#define ZJ_1821 func_ovl10_04_80045348
#define ZJ_182B func_ovl10_04_80045398
#define ZJ_1834 func_ovl10_04_800453E0
#define ZJ_1900 func_ovl10_04_80045418
#define ZJ_1904 func_ovl10_04_80045438
#define ZJ_1909 func_ovl10_04_80045460
UnkStruct_802B0260 *func_ovl10_04_800454A0(void); /* 0x1A00, kept: called with 0 arguments (the definition takes 1) */
#define ZJ_1A02 func_ovl10_04_800454B0
#define ZJ_1D00 func_ovl10_04_800455C0
#define ZJ_1D03 func_ovl10_04_800455D8
s32 *func_ovl10_04_800456B0(void); /* 0x1E0A, kept: called with 0 arguments (the definition takes 1) */
#define ZJ_2004 func_ovl10_04_80045718
#define ZJ_2101 func_ovl10_04_80045740
#define ZJ_2300 func_ovl10_04_80045780
#define ZJ_2600 func_ovl10_04_80045868
void func_ovl10_04_80045870(s32, Vec3f *, s32); /* 0x2601, kept: its callers pass Vec3f * (the header's f32 *) */
#define ZJ_2900 func_ovl10_04_80045910
#define ZJ_2A00 func_ovl10_04_80045928
#define ZJ_2B00 func_ovl10_04_800459A0
#define ZJ_2B04 func_ovl10_04_800459C0
#include "overlays/common/zerojump.h"

void func_ovl10_04_80043010(void);
void func_ovl10_04_80043028(void);
void func_ovl10_04_80043150(void);
void func_ovl10_04_80043170(void);
void func_ovl10_04_800431C0(void);
void func_ovl10_04_80043210(void);
void func_ovl10_04_80043260(void);
s32 func_ovl10_04_800432B0(void (*)(void));
void func_ovl10_04_80043328(void);
void func_ovl10_04_80043388(void);
void func_ovl10_04_80043404(void);
void func_ovl10_04_800434FC(s32);
void func_ovl10_04_800435C0(UnkStructA *);
void func_ovl10_04_800435E0(s32);
void func_ovl10_04_80043740(void);
void func_ovl10_04_80043998(s32);
void func_ovl10_04_80043BE0(void);
void func_ovl10_04_80043D1C(UnkStructA *);
s32 func_ovl10_04_80044060(s32);
void func_ovl10_04_8004407C(UnkStructA *);
void func_ovl10_04_800440FC(UnkStructA *);
void func_ovl10_04_80044168(void);
void func_ovl10_04_800441A4(void);
void func_ovl10_04_80044234(void (*)(void));
void func_ovl10_04_80044250(s32);
void func_ovl10_04_8004426C(s32);
void func_ovl10_04_80044280(void);
s32 func_ovl10_04_8004429C(void (*)(void));
void func_ovl10_04_800442E0(void (*)(void));
s32 func_ovl10_04_8004430C(void (*)(void), const char *);
void func_ovl10_04_80044344(void);
s32 func_ovl10_04_80044438(s32, void *);
void func_ovl10_04_8004446C(f32);
void func_ovl10_04_800444B4(f32, f32, f32);
void func_ovl10_04_80044510(void);
static void func_ovl10_04_80044528(void);

static void func_ovl10_04_80043018(void);
static void func_ovl10_04_80043020(void);
static void func_ovl10_04_80043148(void);

/* .bss */
u8 D_ovl10_04_80045D10[0x24];
s32 D_ovl10_04_80045D34;
s32 D_ovl10_04_80045D38;
Vec3f D_ovl10_04_80045D40;
s32 D_ovl10_04_80045D4C;
s32 D_ovl10_04_80045D50;
/* the tasks: a countdown and a function each */
s32 D_ovl10_04_80045D58[64];
s32 D_ovl10_04_80045E58; /* the running task */
void (*D_ovl10_04_80045E60[64])(void);
UnkStruct_802B0260 *D_ovl10_04_80045F60;
UnkStructA *D_ovl10_04_80045F68[8];
void *D_ovl10_04_80045F88[8];
s32 D_ovl10_04_80045FA8;
s32 D_ovl10_04_80045FB0[8];
s32 D_ovl10_04_80045FD0[8];
s32 D_ovl10_04_80045FF0[4 PORT_MORE(4)]; /* (indexed by the eight movers' slots: on the N64 [4..7] are past the .bss's end, free memory in the overlay slot, each set before it is read) */

/* .data */
s32 D_ovl10_04_80045BC0[8] = { 0, 5, 4, 1, 2, 7, 6, 3 };
s32 D_ovl10_04_80045BE0[8] = { 0, 20, 20, 10, 20, 20, 20, 30 };
s32 D_ovl10_04_80045C00[8] = { 90, 190, 310, 350, 480, 580, 780, 940 };

/* unused: the stubs IDO -O3 leaves at the end of the object */
static void func_ovl10_04_80044578(void) {
}

static void func_ovl10_04_80044570(void) {
}

static void func_ovl10_04_80044568(void) {
}

static void func_ovl10_04_80044560(void) {
}

static void func_ovl10_04_80044558(void) {
}

static void func_ovl10_04_80044550(void) {
}

static void func_ovl10_04_80044548(void) {
}

static void func_ovl10_04_80044540(void) {
}

static void func_ovl10_04_80044538(void) {
}

static void func_ovl10_04_80044530(void) {
}

/* unused (its body is dropped): the reference keeps the unused words in
 * .bss, which would otherwise be common */
static void func_ovl10_04_80044528(void) {
    D_ovl10_04_80045D10[0] = 0;
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl10_04_80044510
#define CORE_FUNC_B func_ovl10_04_800444B4
#define CORE_FUNC_C func_ovl10_04_8004446C
#define CORE_FUNC_D func_ovl10_04_80044438
#define CORE_BSS_A0 D_ovl10_04_80045D34
#define CORE_BSS_A1 D_ovl10_04_80045D38
#define CORE_BSS_B D_ovl10_04_80045D40
#include "overlays/common/core.h"

/* runs the tasks, one frame each time round */
void func_ovl10_04_80044344(void) {
    s32 i;

    D_ovl10_04_80045D4C = 0;
    D_ovl10_04_80045D50 = 0;
    while (TRUE) {
        for (i = 0; i < 64; i++) {
            if (D_ovl10_04_80045E60[i] != NULL) {
                D_ovl10_04_80045E58 = i;
                if (D_ovl10_04_80045D58[i] == 0) {
                    D_ovl10_04_80045E60[i]();
                } else {
                    D_ovl10_04_80045D58[i]--;
                }
            }
        }
        D_ovl10_04_80045D4C++;
        D_ovl10_04_80045D4C &= 3;
        func_ovl10_04_80045460();
    }
}

/* Defined above its caller 8004432C, which pulls it out among the earlier
 * functions all the same: only so does its string come first in .rodata. */
s32 func_ovl10_04_800432B0(void (*arg0)(void)) {
    s32 found;
    s32 i;

    i = 0;
    found = -1;
    do {
        if (D_ovl10_04_80045E60[i] == NULL) {
            D_ovl10_04_80045E60[i] = arg0;
            found = i;
        }
        i++;
    } while (i < 64 && found == -1);
    if (found == -1) {
        func_ovl10_04_80044588("trap task:entry failed\n");
    }
    return found;
}

/* the names src/overlays/common/tasks.h's code has here */
#define TASK_800432D0 func_ovl10_04_800432B0
#define TASK_8004409C func_ovl10_04_8004407C
#define TASK_8004411C func_ovl10_04_800440FC
#define TASK_80044188 func_ovl10_04_80044168
#define TASK_800441C4 func_ovl10_04_800441A4
#define TASK_80044254 func_ovl10_04_80044234
#define TASK_80044270 func_ovl10_04_80044250
#define TASK_8004428C func_ovl10_04_8004426C
#define TASK_800442A0 func_ovl10_04_80044280
#define TASK_800442BC func_ovl10_04_8004429C
#define TASK_80044300 func_ovl10_04_800442E0
#define TASK_8004432C func_ovl10_04_8004430C
#define TASK_800445A8 func_ovl10_04_80044588
#define TASK_80045190 func_ovl10_04_80045170
#define TASK_800452F0 func_ovl10_04_800452D0
#define TASK_80045368 func_ovl10_04_80045348
#define TASK_800453B8 func_ovl10_04_80045398
#define TASK_80045458 func_ovl10_04_80045438
#define TASK_80045480 func_ovl10_04_80045460
#define TASK_800454C0 func_ovl10_04_800454A0
#define TASK_800454D0 func_ovl10_04_800454B0
#define TASK_800456D0 func_ovl10_04_800456B0
#define TASK_80045738 func_ovl10_04_80045718
#define TASK_80045890 func_ovl10_04_80045870
#define TASK_D_80045D78 D_ovl10_04_80045D58
#define TASK_D_80045E78 D_ovl10_04_80045E58
#define TASK_D_80045E80 D_ovl10_04_80045E60
#define TASK_D_80045F80 D_ovl10_04_80045F60
#include "overlays/common/tasks.h"

s32 func_ovl10_04_80044060(s32 arg0) {
    s32 ret;

    ret = 0;
    if ((arg0 & 0xF) != 0xF) {
        ret = 1;
    }
    return ret;
}

void func_ovl10_04_80043D1C(UnkStructA *arg0) {
    s32 i;

    func_ovl10_04_80045170(15);
    i = 0;
    func_ovl10_04_80044FF0(arg0->unk40, 0x11, 0);
    func_ovl10_04_80045020(arg0->unk40, 4, 12);
    func_ovl10_04_80045020(arg0->unk40, 5, 13);
    func_ovl10_04_80045868(0, arg0->unk40->unk10, arg0->unk40->unk14 + 100.0f, arg0->unk40->unk18, 0);
    arg0->unk68 = 40.0f;
    arg0->unk70 = 1.6333334f;
    func_ovl10_04_800459A0(0x1E);
    while (TRUE) {
        func_ovl10_04_80045910(arg0, 0, 0.0f, func_ovl10_04_80044060);
        if (arg0->unk40->unk14 > 700.0f) {
            arg0->unk40->unk14 = 700.0f;
            arg0->unk68 = 0.0f;
            arg0->unk6C = 0.0f;
        }
        arg0->unk40->unk1C += 20.0f;
        if (arg0->unk40->unk1C >= 360.0f) {
            arg0->unk40->unk1C = 0.0f;
        }
        arg0->unk40->unk10 -= func_ovl10_04_800455C0(arg0->unk40->unk1C);
        func_ovl10_04_80045460();
        if (++i >= 200) {
            break;
        }
        if (arg0->unk40->unk14 < 700.0f && arg0->unk6C == 0.0f) {
            break;
        }
    }
    if (arg0->unk40->unk1C >= 90.0f && arg0->unk40->unk1C < 270.0f) {
        arg0->unk40->unk1C = 180.0f;
        arg0->unk40->unk14 = func_ovl10_04_80044988(arg0->unk40) + 70.0f;
    } else {
        arg0->unk40->unk1C = 0.0f;
        arg0->unk40->unk14 = func_ovl10_04_80044988(arg0->unk40);
    }
    func_ovl10_04_80044FF0(arg0->unk40, 0x13, 0);
    func_ovl10_04_800452D0(arg0, 0x0C000000);
    arg0->unk40->unk48->unk4[0].unk8 |= 0x20;
    arg0->unk40->unk48->unk4[0].unk8 |= 0x40;
    arg0->unk40->unk48->unk4[0].unk4 = func_ovl10_04_80045008(arg0->unk40, 0) - 0.1f;
    arg0->unk6C = 0.0f;
    arg0->unk70 = 0.0f;
    func_ovl10_04_80045438(0x50);
}

void func_ovl10_04_80043BE0(void) {
    s32 i;

    if (D_ovl10_04_80045FA8 == 180) {
        for (i = 0; i < 8; i++) {
            if (D_ovl10_04_80045BE0[i] != 0) {
                D_ovl10_04_80045FB0[i] = func_ovl10_04_800455D8(D_ovl10_04_80045BE0[i]) + D_ovl10_04_80045C00[i] + 180;
            } else {
                D_ovl10_04_80045FB0[i] = D_ovl10_04_80045C00[i] + 180;
            }
        }
    }
    if (D_ovl10_04_80045FA8 >= 180) {
        for (i = 0; i < 8; i++) {
            if (D_ovl10_04_80045FA8 == D_ovl10_04_80045FB0[i]) {
                func_ovl10_04_80043998(D_ovl10_04_80045BC0[i]);
            }
        }
    }
    func_ovl10_04_80043740();
    D_ovl10_04_80045FA8++;
    if (D_ovl10_04_80045FA8 == 1180) {
        D_ovl10_04_80045FA8 = 180;
    }
}

void func_ovl10_04_80043998(s32 arg0) {
    s32 i;
    s32 sp54[2] = { 0xAB, 0xAC };
    s32 sp4C[2] = { 0xB8, 0xB9 };
    f32 sp2C[8] = { 700.0f, 1800.0f, 1300.0f, 2400.0f, 1150.0f, 2250.0f, 850.0f, 1950.0f };

    for (i = 0; i < 8; i++) {
        if (D_ovl10_04_80045F68[i] == NULL) {
            D_ovl10_04_80045F68[i] = func_ovl10_04_800452F0(sp54[(s32)(arg0 * 0.25f)], 0x01000070, 0x30, 0);
            D_ovl10_04_80045F68[i]->unk40->unk10 = -1600.0f;
            D_ovl10_04_80045F68[i]->unk40->unk14 = 100.0f;
            D_ovl10_04_80045F68[i]->unk40->unk18 = sp2C[arg0];
            D_ovl10_04_80045FD0[i] = arg0;
            if ((s32)(arg0 * 0.25f) == 0) {
                D_ovl10_04_80045F68[i]->unk30 = 1;
                D_ovl10_04_80045F68[i]->unk34 = 150.0f;
                D_ovl10_04_80045F68[i]->unk38 = 120.0f;
            } else {
                D_ovl10_04_80045F68[i]->unk30 = 8;
                D_ovl10_04_80045F68[i]->unk34 = 300.0f;
                D_ovl10_04_80045F68[i]->unk38 = 1600.0f;
            }
            D_ovl10_04_80045F88[i] = func_ovl10_04_80045320(sp4C[(s32)(arg0 * 0.25f)], 0x01200000, D_ovl10_04_80045F68[i]);
            func_ovl10_04_800452C8(D_ovl10_04_80045F88[i], 0x80000);
            D_ovl10_04_80045FF0[i] = 0;
            break;
        }
    }
}

void func_ovl10_04_80043740(void) {
    s32 i;
    UnkStructA *p;
    f32 lim; /* fake: a guess: any float held for the loop fills this slot (42.5f, 34.0f too) */
    f32 x;

    lim = 5600.0f;
    for (i = 0; i < 8; i++) {
        if (D_ovl10_04_80045F68[i] != NULL && D_ovl10_04_80045FA8 % 2 == i % 2) {
            func_ovl10_04_800435E0(i);
            if (D_ovl10_04_80045FD0[i] < 4) {
                x = D_ovl10_04_80045F68[i]->unk40->unk10;
                D_ovl10_04_80045F68[i]->unk40->unk10 = x + 42.5f;
            } else {
                D_ovl10_04_80045F68[i]->unk40->unk10 += 34.0f;
            }
            p = func_ovl10_04_800453E0(D_ovl10_04_80045F68[i], 7);
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
            if (D_ovl10_04_80045FD0[i] < 4 && func_ovl10_04_80045278(D_ovl10_04_80045F68[i], 0x20) != 0) {
                D_ovl10_04_80045F68[i]->unk40->unk10 = x;
            }
            if (D_ovl10_04_80045F68[i]->unk40->unk10 > lim) {
                func_ovl10_04_800435C0(D_ovl10_04_80045F68[i]);
                D_ovl10_04_80045F68[i] = NULL;
            }
        }
    }
}

void func_ovl10_04_800435E0(s32 arg0) {
    Vec3f pos;
    Vec3f target;
    f32 dist;

    func_ovl10_04_80045398(D_ovl10_04_80045F68[arg0], &pos);
    if ((s32)(D_ovl10_04_80045FD0[arg0] * 0.25f) == 1) {
        pos.x += 1600.0f;
    }
    func_ovl10_04_80045398(func_ovl10_04_80044B08(func_ovl10_04_80044AA8()), &target);
    dist = func_ovl10_04_80045270(&pos, &target);
    if (D_ovl10_04_80045FF0[arg0] == 0 && dist <= 1000.0f) {
        D_ovl10_04_80045FF0[arg0]++;
        if ((s32)(D_ovl10_04_80045FD0[arg0] * 0.25f) == 0) {
            func_ovl10_04_800459A0(0x92);
        } else {
            func_ovl10_04_800459A0(0x93);
        }
    }
}

void func_ovl10_04_800435C0(UnkStructA *arg0) {
    func_ovl10_04_800452E0(arg0);
}

void func_ovl10_04_800434FC(s32 arg0) {
    switch (arg0) {
        case 1:
            break;
        case 2:
            func_ovl10_04_80043404();
            break;
        case 17:
            func_ovl10_04_80043260();
            break;
        case 18:
            func_ovl10_04_80043210();
            break;
        case 19:
            func_ovl10_04_800431C0();
            break;
        case 20:
            func_ovl10_04_80043170();
            break;
        case 4:
            func_ovl10_04_80043150();
            break;
        case 0x50:
            func_ovl10_04_80043028();
            break;
    }
}

/* passed to func_ovl10_04_80045780: three 12-byte records and an end */
u32 D_ovl10_04_80045C50[][3] = {
    { 0xA60002, 0xE, 4 },
    { 0xA70001, 0xE, 4 },
    { 0xB70000, 0xE, 4 },
    { 0, 0, 0 },
};

void func_ovl10_04_80043404(void) {
    func_ovl10_04_80045780(D_ovl10_04_80045C50);
    func_ovl10_04_80045928(0, 0, 0xDE);
    func_ovl10_04_80045928(1, 0x11, 0xEF);
    func_ovl10_04_80045928(2, 0x1A, 0xF8);
    func_ovl10_04_80045928(3, 0x1B, 0xF9);
    func_ovl10_04_800448C8(0x1D8);
    func_ovl10_04_800448D0(0x1D9, 0.0f, 0.0f, 0.0f, 0x50, 0);
    func_ovl10_04_800448D0(0x1DB, 0.0f, 0.0f, 0.0f, 0x60, 0);
    func_ovl10_04_80045740(0xF, 0x7E, 0);
    func_ovl10_04_800448E0(0x1E4);
    func_ovl10_04_80044990(0xF);
    func_ovl10_04_80044A78();
    func_ovl10_04_80043388();
    func_ovl10_04_80043328();
    func_ovl10_04_800449B0(func_ovl10_04_800440FC);
}

void func_ovl10_04_80043388(void) {
    s32 i;

    for (i = 0; i < 64; i++) {
        D_ovl10_04_80045E60[i] = NULL;
        D_ovl10_04_80045D58[i] = 0;
    }
    func_ovl10_04_80045418(func_ovl10_04_80044344, 0, 0, 0, 0x110);
}

void func_ovl10_04_80043328(void) {
    s32 i;

    for (i = 0; i < 8; i++) {
        D_ovl10_04_80045F68[i] = NULL;
    }
    D_ovl10_04_80045FA8 = 0;
    func_ovl10_04_80044A00(func_ovl10_04_80043D1C);
    func_ovl10_04_800432B0(func_ovl10_04_80043BE0);
}

void func_ovl10_04_80043260(void) {
    func_ovl10_04_80044A80(0x2C, 1, 10);
    func_ovl10_04_80044AC8(0xC0, 3, 8.5f, 4, 0, 1, 1);
}

void func_ovl10_04_80043210(void) {
    func_ovl10_04_80044A80(0x2C, 1, 21);
    func_ovl10_04_80044AC8(0xC0, 3, 8.5f, 4, 0, 1, 1);
}

void func_ovl10_04_800431C0(void) {
    func_ovl10_04_80044A80(3, 1, 10);
    func_ovl10_04_80044AC8(0x40, 3, 8.5f, 4, 0, 1, 1);
}

void func_ovl10_04_80043170(void) {
    func_ovl10_04_80044A80(3, 1, 21);
    func_ovl10_04_80044AC8(0x40, 3, 8.5f, 4, 0, 1, 1);
}

void func_ovl10_04_80043150(void) {
    func_ovl10_04_80043148();
    func_ovl10_04_80044AD8();
}

static void func_ovl10_04_80043148(void) {
}

void func_ovl10_04_80043028(void) {
    func_ovl10_04_80043018();
    func_ovl10_04_80043020();
    func_ovl10_04_80044B10(1);
    if (func_ovl10_04_80044A90(0x2C, 6, 0x2E, 0xE) != 0) {
        D_ovl10_04_80045D34 = 0x6A;
        D_ovl10_04_80045D38 = 0x13;
        func_ovl10_04_800449C0(0x6A, 0x13, 0);
    }
    if (func_ovl10_04_80044A90(0x2C, 0x11, 0x2E, 0x19) != 0) {
        D_ovl10_04_80045D34 = 0x6A;
        D_ovl10_04_80045D38 = 0x14;
        func_ovl10_04_800449C0(0x6A, 0x14, 0);
    }
    if (func_ovl10_04_80044A90(1, 6, 3, 0xE) != 0) {
        D_ovl10_04_80045D34 = 0x6C;
        D_ovl10_04_80045D38 = 0x11;
        func_ovl10_04_800449C0(0x6C, 0x11, 0);
    }
    if (func_ovl10_04_80044A90(1, 0x11, 3, 0x19) != 0) {
        D_ovl10_04_80045D34 = 0x6C;
        D_ovl10_04_80045D38 = 0x12;
        func_ovl10_04_800449C0(0x6C, 0x12, 0);
    }
}

static void func_ovl10_04_80043020(void) {
}

static void func_ovl10_04_80043018(void) {
}

void func_ovl10_04_80043010(void) {
}
