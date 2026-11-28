#include "common.h"

/*
 * IDO -O3, like the other overlays: the functions are written last to first
 * (it emits a file's functions in reverse).  At -O2 the common code's
 * CORE_FUNC_D needed an `if (1)` fake to keep arg1 out of its stack home.
 */

#define ZJ_0C00 func_ovl09_01_80043668
#define ZJ_0D00 func_ovl09_01_80043680
#define ZJ_1000 func_ovl09_01_80043818
#define ZJ_1001 func_ovl09_01_80043820
#define ZJ_1009 func_ovl09_01_80043860
#define ZJ_100A func_ovl09_01_80043868
#define ZJ_100C func_ovl09_01_80043878
#define ZJ_100F func_ovl09_01_80043890
#define ZJ_1704 func_ovl09_01_80043EA8
#define ZJ_1708 func_ovl09_01_80043EC8
#define ZJ_1709 func_ovl09_01_80043ED0
#define ZJ_1902 func_ovl09_01_800441C8
#define ZJ_1904 func_ovl09_01_800441D8
#define ZJ_2300 func_ovl09_01_80044520
#define ZJ_2A00 func_ovl09_01_800446C8
#define ZJ_2B04 func_ovl09_01_80044760
#include "overlays/common/zerojump.h"

/* .bss: IDO lays it out in the order it first sees each variable, so the
 * definitions come before the code (docs/DECOMPILING.md, "Data"). */
u8 D_ovl09_01_800449E0[0x24]; /* unused (func_ovl09_01_800432B8) */
s32 D_ovl09_01_80044A04;
s32 D_ovl09_01_80044A08;
Vec3f D_ovl09_01_80044A10;
f32 D_ovl09_01_80044A1C;
f32 D_ovl09_01_80044A20;

/* .data: passed to func_ovl09_01_80044520; three 12-byte records and a
 * zero one (or three and the section's padding: the same bytes). */
u32 D_ovl09_01_80044960[][3] = {
    { 0x008C0000, 0, 4 },
    { 0x008D0002, 0, 4 },
    { 0x008E0002, 0, 4 },
    { 0, 0, 0 },
};

static void func_ovl09_01_80043310(void) {
}

static void func_ovl09_01_80043308(void) {
}

static void func_ovl09_01_80043300(void) {
}

static void func_ovl09_01_800432F8(void) {
}

static void func_ovl09_01_800432F0(void) {
}

static void func_ovl09_01_800432E8(void) {
}

static void func_ovl09_01_800432E0(void) {
}

static void func_ovl09_01_800432D8(void) {
}

static void func_ovl09_01_800432D0(void) {
}

static void func_ovl09_01_800432C8(void) {
}

static void func_ovl09_01_800432C0(void) {
}

/* unused (its body is dropped): the reference keeps the .bss nothing else
 * uses out of common */
static void func_ovl09_01_800432B8(void) {
    D_ovl09_01_800449E0[0] = 0;
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl09_01_800432A0
#define CORE_FUNC_B func_ovl09_01_80043244
#define CORE_FUNC_C func_ovl09_01_800431FC
#define CORE_FUNC_D func_ovl09_01_800431C8
#define CORE_BSS_A0 D_ovl09_01_80044A04
#define CORE_BSS_A1 D_ovl09_01_80044A08
#define CORE_BSS_B D_ovl09_01_80044A10
#define CORE_ZJ_1704 func_ovl09_01_80043EA8
#define CORE_ZJ_1708 func_ovl09_01_80043EC8
#define CORE_ZJ_1709 func_ovl09_01_80043ED0
#define CORE_ZJ_1904 func_ovl09_01_800441D8
#define CORE_ZJ_2B04 func_ovl09_01_80044760
#include "overlays/common/core.h"

void func_ovl09_01_8004316C(void) {
    func_ovl09_01_800441D8(0xA);
    func_ovl09_01_80043868(0, 3, 4.0f, 0, 1, 0, 0);
    func_ovl09_01_800441D8(0xA);
    func_ovl09_01_80043890(0x4000000);
    func_ovl09_01_800441C8();
}

/* The overlay's handler: the entry returns it to the loader.  Cases 1 and
 * 17 are empty but set the jump table's range (.rodata). */
void func_ovl09_01_80043010(s32 arg0) {
    switch (arg0) {
        case 1:
            break;
        case 2:
            func_ovl09_01_80044520(D_ovl09_01_80044960);
            func_ovl09_01_800446C8(0, 0, 0xDE);
            func_ovl09_01_800446C8(1, 0, 0xDE);
            func_ovl09_01_800446C8(2, 0, 0xDE);
            func_ovl09_01_800446C8(3, 0, 0xDE);
            func_ovl09_01_800446C8(4, 0, 0xDE);
            func_ovl09_01_800446C8(5, 0, 0xDE);
            func_ovl09_01_80043668(0x132);
            func_ovl09_01_80043680(0x133);
            func_ovl09_01_80043820(9, 0, 0xC);
            func_ovl09_01_80043818();
            func_ovl09_01_80043878();
            D_ovl09_01_80044A1C = 45.0f;
            D_ovl09_01_80044A20 = 45.0f;
            break;
        case 3:
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
            break;
        case 16:
            func_ovl09_01_80043860(0x40);
            break;
        case 17:
            break;
        case 0x50:
            break;
        case 0x51:
            break;
        case 0x52:
            break;
        case 0x53:
            break;
    }
}
