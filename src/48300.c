#include "common.h"

/*
 * -O3, so written last function first. Five functions (us
 * 0x8022DB00-0x8022DE00): func_8022DB00 and two tasks that draw screen
 * rectangles. 48600 (src/48600.c) is a separate object:
 * func_80232E60 calls func_8022DB00 with a jal, where one -O3 file inlines
 * it (text_objects.txt).
 */

s32 func_80238A10(void *, PORT_IP(s32), PORT_RET(s32, void *), s32, s32);
PORT_RET(void, s32) func_8023903C(s32);
void func_80239094(void);
PORT_IP(s32) func_802391F0(void);
void func_80239430(void);
s32 *func_8023A840(void);
void func_8026C454(s32, f32, f32, f32);
s32 func_8026C51C(s32, f32, f32, f32);

/* 48600's, for ZeroJump module 0x18's table */
void func_8022E3D4(f32 m[4][4], f32 *v);
f32 func_8022E330(f32 *arg0, f32 *arg1);
void func_8022E364(f32 *arg0, f32 *arg1, f32 *arg2);
void func_8022FF58(f32 *arg0, f32 *arg1, f32 *arg2);
void func_8022FEAC(f32 *arg0, f32 *arg1);
void func_80231D88(u8 *arg0, u8 *arg1, f32 *arg2, f32 *arg3);
f32 func_8022DF68(f32 *arg0, f32 *arg1);
s32 func_80232D18(f32 *arg0, s32 arg1);
s32 func_80232C74(f32 *arg0, s32 arg1);
s32 func_80232BD0(f32 *arg0, s32 arg1);
s32 func_8022E884(UnkStructA *arg0, UnkStructA *arg1);
void func_80232A34(u8 *arg0, s32 arg1);
void func_80232AA8(u8 *arg0);
void func_80232AC8(u8 *arg0, u8 *arg1);
void func_80232B04(u8 *arg0, u8 *arg1, PORT_IP(s32) arg2, s32 arg3);
void func_80231D60(f32 arg0, f32 arg1, f32 arg2, f32 arg3);
void func_8022FC0C(s32 arg0);
void func_80231408(u8 *arg0, s32 arg1);
void func_802313DC(u8 *arg0, s32 arg1);
void func_802313B0(u8 *arg0, s32 arg1);
s32 func_8022FC90(UnkStructA *arg0);
void *func_80230CB4(s32 arg0, s32 arg1, s32 arg2, s32 arg3, u8 *arg4);
PORT_RET(void, PORT_IP(s32)) func_80231274(s32 arg0, s32 arg1, s32 arg2, PORT_IP(s32) arg3);
void func_80231244(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
void *func_802311E8(u8 *arg0);
void *func_8023116C(s32 arg0, s32 arg1, f32 arg2, f32 arg3, f32 arg4);
void *func_802310F4(s32 arg0, s32 arg1, f32 arg2, f32 arg3, f32 arg4);
void func_8023101C(s32 arg0, s32 arg1, s32 arg2, u8 *arg3);
void func_802310BC(s32 arg0, s32 arg1, u8 *arg2);
void *func_80231048(s32 arg0, u8 *arg1, s32 arg2, f32 arg3, f32 arg4, f32 arg5);
void *func_80230E9C(u8 *arg0);
PORT_RET(void, PORT_IP(s32)) func_80230E6C(s32 arg0, PORT_IP(s32) arg1);
void func_80230AA0(UnkStructA *arg0, PORT_IP(s32) arg1, PORT_IP(s32) arg2, PORT_IP(s32) arg3);
void func_80230914(u8 *arg0);
PORT_RET(void, PORT_IP(s32)) func_802308B8(f32 *arg0, s32 arg1);
void func_8023085C(f32 *arg0, s32 arg1);
void func_80230800(f32 *arg0, s32 arg1);
void func_80230790(f32 *arg0, s32 arg1, f32 *arg2);
void func_80230720(f32 *arg0, s32 arg1, f32 *arg2);
void func_802306B0(f32 *arg0, s32 arg1, f32 *arg2);
void func_80230440(f32 *arg0, s32 arg1, u8 *arg2);
void func_802303C0(f32 *arg0, s32 arg1, u8 *arg2);
void func_80230340(f32 *arg0, s32 arg1, u8 *arg2);
void func_8022F808(f32 *arg0, f32 *arg1);
void func_80232BA0(u8 *arg0, u8 *arg1);
void func_8022FEA0(f32 arg0);
void func_8022FE94(s32 arg0);
PORT_RET(void, PORT_IP(s32)) func_8022FA88(f32 *arg0, s32 arg1, f32 *arg2);
void func_8022FA10(f32 *arg0, s32 arg1, f32 *arg2);
void func_8022F998(f32 *arg0, s32 arg1, f32 *arg2);
PORT_IP(s32) func_8022F77C(s32 arg0, PORT_IP(s32) arg1, PORT_IP(s32) arg2);
void func_80232DB4(f32 *arg0);
f32 *func_8022F550(f32 *arg0, s32 arg1);
f32 *func_8022F4AC(f32 *arg0, s32 arg1, f32 *arg2);
PORT_IP(s32) func_8022F698(PORT_IP(s32) arg0, f32 *arg1, f32 *arg2, f32 **arg3, s32 arg4);
void func_8022DE6C(void);
void func_8022DE00(void);

extern UnkStructA_40 *D_800A7F08[8];

void func_8022DB00(void);
static void func_8022DB28(void);
void func_8022DBBC(UnkStruct_8022DBBC *arg0);
static void func_8022DBF4(UnkStructA_40 **arg0, f32 arg1, f32 arg2, s32 *arg3, s32 arg4);
static void func_8022DD6C(void);
void func_8022DDB4(PORT_IP(s32) arg0);

/* ZeroJump module 0x18's table (func_8022DB00): 48600's functions and this
   file's two tasks; the zero word after it is the section's padding */
void *D_8029FD40[] = {
    func_8022E3D4, func_8022E330, func_8022E364, func_8022FF58,
    func_8022FEAC, func_80231D88, func_8022DF68, func_80232D18,
    func_80232C74, func_80232BD0, func_8022E884, func_80232A34,
    func_80232AA8, func_80232AC8, func_80232B04, func_80231D60,
    func_8022FC0C, func_80231408, func_802313DC, func_802313B0,
    func_8022FC90, func_80230CB4, func_80231274, func_80231244,
    func_802311E8, func_8023116C, func_802310F4, func_8023101C,
    func_802310BC, func_80231048, func_80230E9C, func_80230E6C,
    func_80230AA0, func_80230914, func_802308B8, func_8023085C,
    func_80230800, func_80230790, func_80230720, func_802306B0,
    func_80230440, func_802303C0, func_80230340, func_8022F808,
    func_80232BA0, func_8022FEA0, func_8022FE94, func_8022FA88,
    func_8022FA10, func_8022F998, func_8022F77C, func_80232DB4,
    func_8022F550, func_8022F4AC, func_8022F698, func_8022DDB4,
    func_8022DE6C, func_8022DE00, func_8022DBBC,
};

void func_8022DDB4(PORT_IP(s32) arg0) {
    if (!(*func_8023A840() & 2)) {
        func_80238A10(func_8022DD6C, arg0, 0, 0, 0x150);
    }
}

static void func_8022DD6C() {
    func_8022DBF4(D_800A7F08, 291.0f, 56.0f, (s32 *)(func_802391F0()), 0);
    func_80239430();
}

static void func_8022DBF4(UnkStructA_40 **arg0, f32 arg1, f32 arg2, s32 *arg3, s32 arg4) {
    UnkStructA_40 *r;
    s32 cur;
    s32 tgt;
    s32 n;
    f32 f;

    r = *arg0;
    cur = 0;
    while (TRUE) {
        n = *arg3 - arg4;
        if (n < 0) {
            n = 0;
        }
        if (n < 3) {
            tgt = n * 4;
        } else {
            tgt = 8;
        }
        if (cur == tgt && n == 0) {
            break;
        }
        if (cur == tgt && n >= 3) {
            func_8022DBF4(arg0 + 1, arg1, arg2 + 16.0f + 4.0f, arg3, arg4 + 2);
        }
        if (cur < tgt) {
            cur++;
        } else if (tgt < cur) {
            cur--;
        }
        f = cur;
        r->unk10 = arg1 - f;
        r->unk14 = arg2 - f;
        r->unk18 = f + arg1;
        r->unk1C = f + arg2;
        func_8023903C(2);
    }
}

void func_8022DBBC(UnkStruct_8022DBBC *arg0) {
    func_80238A10(func_8022DB28, (PORT_IP(s32)) arg0, 0, 0, 0x150);
}

static void func_8022DB28() {
    UnkStruct_8022DBBC *args;
    UnkStructA_40 *r;
    s32 id;
    s32 n;
    s32 i;

    args = (UnkStruct_8022DBBC *) func_802391F0();
    r = args->unk0;
    id = args->unk4;
    n = args->unk8;
    id = func_8026C51C(id, r->unk10, r->unk14, r->unk18);
    for (i = 0; i < n; i++) {
        func_8026C454(id, r->unk10, r->unk14, r->unk18);
        func_80239094();
    }
    func_80239430();
}

void func_8022DB00(void) {
    func_80297D30(0x18, D_8029FD40);
}
