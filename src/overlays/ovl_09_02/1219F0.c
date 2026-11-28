#include "common.h"

/*
 * IDO -O3, like the other overlays: the functions are written last to first
 * (it emits a file's functions in reverse).  At -O2 the common code's
 * CORE_FUNC_D needed an `if (1)` fake to keep arg1 out of its stack home.
 */

#define ZJ_0100 func_ovl09_02_800433E8
#define ZJ_0C00 func_ovl09_02_80043728
#define ZJ_0D00 func_ovl09_02_80043740
void func_ovl09_02_80043808(void (*)(PORT_IP(s32)), s32); /* 0x0E02, kept: its callers pass a void (*)(PORT_IP(s32)) (the header's void (*)(UnkStructA *)) */
#define ZJ_1000 func_ovl09_02_800438D8
#define ZJ_1001 func_ovl09_02_800438E0
#define ZJ_100C func_ovl09_02_80043938
#define ZJ_171B func_ovl09_02_80044020
#define ZJ_1D04 func_ovl09_02_80044440
void func_ovl09_02_80044778(PORT_IP(s32), s32, f32); /* 0x2901, kept: its callers pass PORT_IP(s32) (the header's UnkStructA *) */
s32 func_ovl09_02_800440D8(PORT_IP(s32), s32); /* 0x1807, kept: its callers pass PORT_IP(s32) (the header's UnkStructA *) */
#define ZJ_2B04 func_ovl09_02_80044820
#define ZJ_1709 func_ovl09_02_80043F90
#define ZJ_1708 func_ovl09_02_80043F88
#define ZJ_1704 func_ovl09_02_80043F68
#define ZJ_1904 func_ovl09_02_80044298
#include "overlays/common/zerojump.h"

/* .bss: IDO lays it out in the order it first sees each variable, so the
 * definitions come before the code (docs/DECOMPILING.md, "Data"). */
u8 D_ovl09_02_80044AD0[0x24]; /* unused (func_ovl09_02_8004337C) */
s32 D_ovl09_02_80044AF4;
s32 D_ovl09_02_80044AF8;
Vec3f D_ovl09_02_80044B00;
f32 D_ovl09_02_80044B0C;
f32 D_ovl09_02_80044B10;

void func_ovl09_02_80043010(PORT_IP(s32) arg0); /* (an object, as 80043058's) */
void func_ovl09_02_80043058(PORT_IP(s32) arg0); /* (an object: the callback 5E3F0.c's func_8024E088 passes it) */
s32 func_ovl09_02_8004328C(s32, void *);
void func_ovl09_02_800432C0(f32);
void func_ovl09_02_80043308(f32, f32, f32);
void func_ovl09_02_80043364(void);

static void func_ovl09_02_800433D4(void) {
}

static void func_ovl09_02_800433CC(void) {
}

static void func_ovl09_02_800433C4(void) {
}

static void func_ovl09_02_800433BC(void) {
}

static void func_ovl09_02_800433B4(void) {
}

static void func_ovl09_02_800433AC(void) {
}

static void func_ovl09_02_800433A4(void) {
}

static void func_ovl09_02_8004339C(void) {
}

static void func_ovl09_02_80043394(void) {
}

static void func_ovl09_02_8004338C(void) {
}

static void func_ovl09_02_80043384(void) {
}

/* unused (its body is dropped): the reference keeps the .bss nothing else
 * uses out of common */
static void func_ovl09_02_8004337C(void) {
    D_ovl09_02_80044AD0[0] = 0;
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl09_02_80043364
#define CORE_FUNC_B func_ovl09_02_80043308
#define CORE_FUNC_C func_ovl09_02_800432C0
#define CORE_FUNC_D func_ovl09_02_8004328C
#define CORE_BSS_A0 D_ovl09_02_80044AF4
#define CORE_BSS_A1 D_ovl09_02_80044AF8
#define CORE_BSS_B D_ovl09_02_80044B00
#include "overlays/common/core.h"

/* The overlay's handler: the entry returns it to the loader.  Cases 1 and
 * 17 are empty but set the jump table's range (.rodata). */
void func_ovl09_02_800430A0(s32 arg0) {
    f32 angle;
    f32 wrapped;
    s32 pad[5]; /* fake: unused, the original frame has 0x14 more bytes of locals */

    switch (arg0) {
        case 1:
            break;
        case 2:
            func_ovl09_02_80043728(0x134);
            func_ovl09_02_80043740(0x135);
            func_ovl09_02_800438D8();
            func_ovl09_02_800438E0(9, 0, 0xC);
            func_ovl09_02_80043938();
            D_ovl09_02_80044B0C = 45.0f;
            D_ovl09_02_80044B10 = 45.0f;
            func_ovl09_02_80043808(func_ovl09_02_80043058, 1);
            func_ovl09_02_80043808(func_ovl09_02_80043058, 5);
            func_ovl09_02_80043808(func_ovl09_02_80043010, 7);
            func_ovl09_02_80044020(-45.0f, -70.0f);
            break;
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            break;
        case 17:
            break;
        case 0x50:
            func_ovl09_02_800433E8("debmap0 : Mapchange happened!\n");
            break;
        case 0x51:
            func_ovl09_02_800433E8("debmap0 : Playerout happened!\n");
            break;
        case 0x52:
            break;
        case 0x53:
            func_ovl09_02_800433E8("debmap0 : Evlevel2 happened!\n");
            break;
        case 3:
            angle = D_ovl09_02_80044B0C + 1.0f;
            if (angle >= 360.0f) {
                D_ovl09_02_80044B0C = angle - 360.0f;
            } else {
                if (angle < 0.0f) {
                    wrapped = angle + 360.0f;
                } else {
                    wrapped = angle;
                }
                D_ovl09_02_80044B0C = wrapped;
            }
            func_ovl09_02_80044440(D_ovl09_02_80044B0C, D_ovl09_02_80044B10);
            break;
    }
}

void func_ovl09_02_80043058(PORT_IP(s32) arg0) {
    func_ovl09_02_80044778(arg0, 0x40, 4.0f);
    if (func_ovl09_02_800440D8(arg0, 0x10) != 0) {
        func_ovl09_02_80044778(arg0, 0xC0, 4.0f);
    }
}

void func_ovl09_02_80043010(PORT_IP(s32) arg0) {
    func_ovl09_02_80044778(arg0, 0x40, 4.0f);
    if (func_ovl09_02_800440D8(arg0, 0x10) != 0) {
        func_ovl09_02_80044778(arg0, 0xC0, 4.0f);
    }
}
