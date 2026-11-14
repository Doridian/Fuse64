#include "common.h"

/*
 * IDO -O3, like the other overlays: the functions are written last to first
 * (it emits a file's functions in reverse).  At -O2 the common code's
 * CORE_FUNC_D needed an `if (1)` fake to keep arg1 out of its stack home.
 */

void func_ovl0F_06_80043598(s32);
void func_ovl0F_06_800435B0(s32);
void func_ovl0F_06_80043748(void);
void func_ovl0F_06_80043750(s32, s32, s32);
void func_ovl0F_06_800437A8(void);
void func_ovl0F_06_80043DD8(f32, f32, f32);
void func_ovl0F_06_80043DF8(f32, f32, f32);
void func_ovl0F_06_80043E00(f32 *, f32 *, f32 *);
void func_ovl0F_06_80044108(s32);
void func_ovl0F_06_800445F8(s32, s32, s32);
s32 func_ovl0F_06_80044690(s32, f32, f32, f32);

/* .bss, in first-seen order (docs/DECOMPILING.md, "Data") */
u8 D_ovl0F_06_80044940[0x24]; /* unused (func_ovl0F_06_800431EC) */
s32 D_ovl0F_06_80044964;
s32 D_ovl0F_06_80044968;
Vec3f D_ovl0F_06_80044970;

/* .data: six 12-byte records, each passed to func_ovl0F_06_800445F8 */
u32 D_ovl0F_06_80044890[][3] = {
    { 0, 0x12, 0xF0 },
    { 1, 0x13, 0xF1 },
    { 2, 0x16, 0xF4 },
    { 3, 0, 0xDE },
    { 4, 2, 0xE0 },
    { 5, 0x15, 0xF3 },
};

static void func_ovl0F_06_8004323C(void) {
}

static void func_ovl0F_06_80043234(void) {
}

static void func_ovl0F_06_8004322C(void) {
}

static void func_ovl0F_06_80043224(void) {
}

static void func_ovl0F_06_8004321C(void) {
}

static void func_ovl0F_06_80043214(void) {
}

static void func_ovl0F_06_8004320C(void) {
}

static void func_ovl0F_06_80043204(void) {
}

static void func_ovl0F_06_800431FC(void) {
}

static void func_ovl0F_06_800431F4(void) {
}

/* unused (its body is dropped): the reference keeps the .bss nothing else
 * uses out of common */
static void func_ovl0F_06_800431EC(void) {
    D_ovl0F_06_80044940[0] = 0;
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl0F_06_800431D4
#define CORE_FUNC_B func_ovl0F_06_80043178
#define CORE_FUNC_C func_ovl0F_06_80043130
#define CORE_FUNC_D func_ovl0F_06_800430FC
#define CORE_BSS_A0 D_ovl0F_06_80044964
#define CORE_BSS_A1 D_ovl0F_06_80044968
#define CORE_BSS_B D_ovl0F_06_80044970
#define CORE_ZJ_1704 func_ovl0F_06_80043DD8
#define CORE_ZJ_1708 func_ovl0F_06_80043DF8
#define CORE_ZJ_1709 func_ovl0F_06_80043E00
#define CORE_ZJ_1904 func_ovl0F_06_80044108
#define CORE_ZJ_2B04 func_ovl0F_06_80044690
#include "overlays/common/core.h"

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl0F_06_80043018(s32 arg0) {
    u32 (*p)[3];
    s32 i;
    s32 count;

    switch (arg0) {
        case 1:
            break;
        case 2:
            /* the count in a variable: with a literal 6, IDO compares the
             * pointer against the table's end instead of i with slt */
            p = D_ovl0F_06_80044890;
            count = 6;
            for (i = 0; i < count; i++) {
                func_ovl0F_06_800445F8((*p)[0], (*p)[1], (*p)[2]);
                p++;
            }
            func_ovl0F_06_80043598(0x229);
            func_ovl0F_06_800435B0(0x23C);
            func_ovl0F_06_80043748();
            break;
        case 3:
            break;
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
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
            break;
        case 22:
            func_ovl0F_06_80043750(0x1E, 1, 0xE);
            break;
        case 4:
            func_ovl0F_06_800437A8();
            break;
        case 0x50:
            break;
    }
}

void func_ovl0F_06_80043010(void) {
}
