#include "common.h"

/*
 * IDO -O3, like the other overlays: the functions are written last to first
 * (it emits a file's functions in reverse).  At -O2 the common code's
 * CORE_FUNC_D needed an `if (1)` fake to keep arg1 out of its stack home.
 */

#define ZJ_0100 func_ovl09_03_80043338
#define ZJ_0C00 func_ovl09_03_80043678
#define ZJ_0D00 func_ovl09_03_80043690
#define ZJ_0F00 func_ovl09_03_80043770
#define ZJ_0F03 func_ovl09_03_80043788
#define ZJ_1000 func_ovl09_03_80043828
#define ZJ_1001 func_ovl09_03_80043830
#define ZJ_100C func_ovl09_03_80043888
#define ZJ_1D04 func_ovl09_03_80044390
#define ZJ_2300 func_ovl09_03_80044530
#define ZJ_2B04 func_ovl09_03_80044770
#define ZJ_1709 func_ovl09_03_80043EE0
#define ZJ_1708 func_ovl09_03_80043ED8
#define ZJ_1704 func_ovl09_03_80043EB8
#define ZJ_1904 func_ovl09_03_800441E8
#include "overlays/common/zerojump.h"

/* .bss: IDO lays it out in the order it first sees each variable, so the
 * definitions come before the code (docs/DECOMPILING.md, "Data"). */
u8 D_ovl09_03_80044A50[0x24]; /* unused (func_ovl09_03_800432CC) */
s32 D_ovl09_03_80044A74;
s32 D_ovl09_03_80044A78;
Vec3f D_ovl09_03_80044A80;
f32 D_ovl09_03_80044A8C;
f32 D_ovl09_03_80044A90;

/* .data: passed to func_ovl09_03_80044530, as in ovl_09_01; three 12-byte
 * records and a zero one (or three and the section's padding). */
u32 D_ovl09_03_80044970[][3] = {
    { 0x008C0000, 0, 4 },
    { 0x008D0002, 0, 4 },
    { 0x008E0002, 0, 4 },
    { 0, 0, 0 },
};

static void func_ovl09_03_80043324(void) {
}

static void func_ovl09_03_8004331C(void) {
}

static void func_ovl09_03_80043314(void) {
}

static void func_ovl09_03_8004330C(void) {
}

static void func_ovl09_03_80043304(void) {
}

static void func_ovl09_03_800432FC(void) {
}

static void func_ovl09_03_800432F4(void) {
}

static void func_ovl09_03_800432EC(void) {
}

static void func_ovl09_03_800432E4(void) {
}

static void func_ovl09_03_800432DC(void) {
}

static void func_ovl09_03_800432D4(void) {
}

/* unused (its body is dropped): the reference keeps the .bss nothing else
 * uses out of common */
static void func_ovl09_03_800432CC(void) {
    D_ovl09_03_80044A50[0] = 0;
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl09_03_800432B4
#define CORE_FUNC_B func_ovl09_03_80043258
#define CORE_FUNC_C func_ovl09_03_80043210
#define CORE_FUNC_D func_ovl09_03_800431DC
#define CORE_BSS_A0 D_ovl09_03_80044A74
#define CORE_BSS_A1 D_ovl09_03_80044A78
#define CORE_BSS_B D_ovl09_03_80044A80
#define CORE_ZJ_1704 func_ovl09_03_80043EB8
#define CORE_ZJ_1708 func_ovl09_03_80043ED8
#define CORE_ZJ_1709 func_ovl09_03_80043EE0
#define CORE_ZJ_1904 func_ovl09_03_800441E8
#define CORE_ZJ_2B04 func_ovl09_03_80044770
#include "overlays/common/core.h"

/* The overlay's handler: the entry returns it to the loader.  Cases 1 and
 * 17 are empty but set the jump table's range (.rodata). */
void func_ovl09_03_80043010(s32 arg0) {
    f32 angle;
    f32 wrapped;

    switch (arg0) {
        case 1:
            break;
        case 2:
            func_ovl09_03_80044530(D_ovl09_03_80044970);
            func_ovl09_03_80043678(0x343);
            func_ovl09_03_80043690(0x35E);
            func_ovl09_03_80043828();
            func_ovl09_03_80043830(9, 0, 9);
            func_ovl09_03_80043888();
            D_ovl09_03_80044A8C = 45.0f;
            D_ovl09_03_80044A90 = 45.0f;
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
            func_ovl09_03_80043338("debmap0 : Mapchange happened!\n");
            break;
        case 0x51:
            func_ovl09_03_80043338("debmap0 : Playerout happened!\n");
            break;
        case 0x52:
            break;
        case 0x53:
            func_ovl09_03_80043338("debmap0 : Evlevel2 happened!\n");
            break;
        case 3:
            if (func_ovl09_03_80043788() & 0x1000) {
                func_ovl09_03_80043770(0x14, 0x10, 0);
            }
            angle = D_ovl09_03_80044A8C + 1.0f;
            if (angle >= 360.0f) {
                D_ovl09_03_80044A8C = angle - 360.0f;
            } else {
                if (angle < 0.0f) {
                    wrapped = angle + 360.0f;
                } else {
                    wrapped = angle;
                }
                D_ovl09_03_80044A8C = wrapped;
            }
            func_ovl09_03_80044390(D_ovl09_03_80044A8C, D_ovl09_03_80044A90);
            break;
    }
}
