#include "common.h"

void func_80299170(s32, f32, f32, f32); /* 0x2B04 (the overlays' CORE_ZJ_2B04) */
void func_802988E0(f32 *, f32 *, f32 *);
void func_802988D8(f32, f32, f32);
void func_802988B8(f32, f32, f32);
void func_80298BE8(s32);

/* .data: 40040's init clears D_802A1230, D_802A1234 and D_802A123C;
 * nothing names D_802A1238 or D_802A1240 (zeros; at least its first word
 * is the file's, since 80580's .data starts at 0x802A1250 and IDO pads
 * only to 16). */
s32 D_802A1230 = 1;
s32 D_802A1234 = 2;
s32 D_802A1238 = 0;
s32 D_802A123C = 1;
u8 D_802A1240[0x10] = { 0 };

/* .bss: nothing names D_802AD0E0 or D_802AD10C. */
u8 D_802AD0E0[0x24];
s32 D_802AD104;
s32 D_802AD108;
u8 D_802AD10C[4];
Vec3f D_802AD110;

void func_80265C20(s32 arg0, UnkStructA *arg1) {
    UnkStructA_40 *p;

    p = arg1->unk40;
    /* A check around compiled-out debug output: the test's block keeps
     * arg1 from being spilled (any one-variable test of p matches; the
     * condition is a guess). */
    if (p == NULL) {
        DEBUG_PRINT(("p == NULL\n"));
    }
    func_80299170(arg0, p->unk10, p->unk14, p->unk18);
}

void func_80265C54(f32 arg0) {
    f32 sp1C;

    func_802988E0(&sp1C, NULL, NULL);
    func_802988D8(sp1C + arg0, 32768.0f, 32768.0f);
}

void func_80265C9C(f32 arg0, f32 arg1, f32 arg2) {
    func_802988B8(D_802AD110.x, D_802AD110.y, D_802AD110.z);
    func_802988D8(arg0, arg1, arg2);
    func_80298BE8(0xB4);
}

void func_80265CF8(void) {
    D_802AD104 = -1;
    D_802AD108 = -1;
}

void func_80265D10(void) {
}

void func_80265D18(void) {
}

void func_80265D20(void) {
}

void func_80265D28(void) {
}

void func_80265D30(void) {
}

void func_80265D38(void) {
}

void func_80265D40(void) {
}

void func_80265D48(void) {
}

void func_80265D50(void) {
}

void func_80265D58(void) {
}

void func_80265D60(void) {
}

void func_80265D68(void) {
}

void func_80265D70(void) {
}
