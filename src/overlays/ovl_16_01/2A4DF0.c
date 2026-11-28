#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_16_01's game object, built -g (as its entry): every value goes through
 * the stack, each function ends in two `jr ra`, and the returns are branches
 * to the epilogue.  It is the other overlays' template with its debug
 * messages kept (func_ovl16_01_80043020).
 */

#define ZJ_0100 func_ovl16_01_80043D38
#define ZJ_0C00 func_ovl16_01_80044068
#define ZJ_0D00 func_ovl16_01_80044080
#define ZJ_0D0A func_ovl16_01_800440D0
#define ZJ_0F00 func_ovl16_01_80044158
#define ZJ_1000 func_ovl16_01_800441F8
#define ZJ_1001 func_ovl16_01_80044200
#define ZJ_1002 func_ovl16_01_80044208
#define ZJ_1006 func_ovl16_01_80044228
#define ZJ_1007 func_ovl16_01_80044230
#define ZJ_1008 func_ovl16_01_80044238
#define ZJ_100C func_ovl16_01_80044258
#define ZJ_150C func_ovl16_01_80044740
#define ZJ_150D func_ovl16_01_80044748
#define ZJ_150E func_ovl16_01_80044750
#define ZJ_1511 func_ovl16_01_80044768
#define ZJ_1704 func_ovl16_01_80044840
#define ZJ_1708 func_ovl16_01_80044860
#define ZJ_1709 func_ovl16_01_80044868
#define ZJ_1713 func_ovl16_01_800448B8
void func_ovl16_01_80044990(PORT_IP(s32), Vec3f *, Vec3f *); /* 0x1803, kept: its callers pass s32 (the header's f32 *) */
#define ZJ_1811 func_ovl16_01_80044A00
#define ZJ_1812 func_ovl16_01_80044A08
#define ZJ_1814 func_ovl16_01_80044A18
#define ZJ_181A func_ovl16_01_80044A48
#define ZJ_1904 func_ovl16_01_80044B68
#define ZJ_1909 func_ovl16_01_80044B90
#define ZJ_1E06 func_ovl16_01_80044D78
#define ZJ_1E07 func_ovl16_01_80044D80
#define ZJ_1E08 func_ovl16_01_80044D88
s32 func_ovl16_01_80044F28(s32, Vec3f *, s32); /* 0x2601, kept: its callers pass Vec3f * (the header's f32 *) */
#define ZJ_2602 func_ovl16_01_80044F30
#define ZJ_2A00 func_ovl16_01_80044F98
#define ZJ_2A02 func_ovl16_01_80044FA8
#define ZJ_2A09 func_ovl16_01_80044FE0
#define ZJ_2B04 func_ovl16_01_80045010
#include "overlays/common/zerojump.h"

void func_ovl16_01_80043504(void);
void func_ovl16_01_80043B7C(void);
void func_ovl16_01_80043BBC(void);
void func_ovl16_01_80043C6C(void);
void func_ovl16_01_80043C98(void);
void func_ovl16_01_80043CE0(void);
void func_ovl16_01_80043D18(void);

/* .bss */
Vec3f D_ovl16_01_800452A0;
u8 D_ovl16_01_800452AC[0x4];
Vec3f D_ovl16_01_800452B0;
s32 D_ovl16_01_800452BC;
s32 D_ovl16_01_800452C0;
s32 D_ovl16_01_800452C4;
s32 D_ovl16_01_800452C8;
u8 D_ovl16_01_800452CC[0x4];
Vec3f D_ovl16_01_800452D0;

void func_ovl16_01_80043020(UnkStruct_ovl0C_01_8004A54C *arg0) {
    arg0->unk0 = func_ovl16_01_80044A48(arg0->unk8, 0xC00, 0.0f, 0.0f, 0.0f);
    func_ovl16_01_80044A00(arg0->unk0, 0x01080000);
    func_ovl16_01_80044A08(arg0->unk0, 0x02000000);
    if (arg0->unkC != -1 && arg0->unkE != -1 && arg0->unk10 != -1 && arg0->unk12 != -1) {
        arg0->unk4 = func_ovl16_01_80044748(arg0->unk0->unk40, 0, arg0->unkC, arg0->unkE, arg0->unk10, arg0->unk12);
        if (arg0->unk4 == -1) {
            func_ovl16_01_80043D38("MapObjEntry : EvObjShiftCreate can't entry\n");
            func_ovl16_01_80043D38("request resource %d\n", arg0->unk8);
            func_ovl16_01_80043D38("p->sstep p->smax : %04x %04x\n", arg0->unkC, arg0->unkE);
            func_ovl16_01_80043D38("p->tstep p->tmax : %04x %04x\n", arg0->unk10, arg0->unk12);
            while (TRUE) {
            }
        }
    }
    arg0->unk0->unk40->unk10 = arg0->unk14 * 10.0f;
    arg0->unk0->unk40->unk14 = arg0->unk18 * 10.0f;
    arg0->unk0->unk40->unk18 = arg0->unk1C * 10.0f;
    arg0->unk0->unk40->unk1C = arg0->unk20;
    arg0->unk0->unk40->unk20 = arg0->unk24;
    arg0->unk0->unk40->unk24 = arg0->unk28;
    arg0->unk0->unk40->unk28 = arg0->unk2C;
    arg0->unk0->unk40->unk2C = arg0->unk30;
    arg0->unk0->unk40->unk30 = arg0->unk34;
}

void func_ovl16_01_8004329C(UnkStruct_ovl0C_01_8004A54C *arg0) {
    func_ovl16_01_80044A18(arg0->unk0);
    /* 0xFFFF where 80043020 has -1: `li at, 0xffff` */
    if (arg0->unkC != 0xFFFF && arg0->unkE != 0xFFFF && arg0->unk10 != 0xFFFF && arg0->unk12 != 0xFFFF) {
        func_ovl16_01_80044750(arg0->unk4);
    }
    arg0->unk0 = NULL;
    arg0->unk4 = -1;
}

s32 func_ovl16_01_80043348(UnkStruct_ovl0C_01_8004A54C *arg0) {
    if (arg0->unk0->unk40->unk20 == 0.0f) {
        return 0;
    }
    if (arg0->unk0->unk40->unk20 == 90.0f) {
        return 1;
    }
    if (arg0->unk0->unk40->unk20 == 180.0f) {
        return 2;
    }
    if (arg0->unk0->unk40->unk20 == 270.0f || arg0->unk0->unk40->unk20 == -90.0f) {
        return 3;
    }
    return -1;
}

void func_ovl16_01_80043448(s32 arg0, s32 *arg1, s32 *arg2) {
    s32 max;

    switch (arg0) {
        case 0:
            max = arg2[0];
            break;
        case 1:
            max = arg2[1];
            break;
        case 2:
            max = arg2[2];
            break;
    }
    if (*arg1 < 0) {
        *arg1 = 0;
    } else if (*arg1 >= max) {
        *arg1 = max - 1;
    }
}

void func_ovl16_01_80043504(void) {
    D_ovl16_01_800452BC = func_ovl16_01_80044228();
    func_ovl16_01_80044208(&D_ovl16_01_800452B0);
    func_ovl16_01_80044230(D_ovl16_01_800452BC, &D_ovl16_01_800452A0);
}

void func_ovl16_01_80043554(s32 arg0, Vec3f *arg1) {
    func_ovl16_01_80044990(arg0, &D_ovl16_01_800452A0, arg1);
    arg1->y = arg1->y + 90.0f >= 360.0f ? arg1->y + 90.0f - 360.0f
            : arg1->y + 90.0f < 0.0f    ? arg1->y + 90.0f + 360.0f
                                        : arg1->y + 90.0f;
}

void func_ovl16_01_8004363C(UnkStructA *arg0) {
    while (!(arg0->unk40->unkC & 0x40)) {
        func_ovl16_01_80044B90();
    }
}

void func_ovl16_01_800436B0(UnkStructA *arg0) {
    arg0->unk40->unk48->unk4->unk4 = func_ovl16_01_80044740(arg0->unk40, 0) - 0.001f;
}

void func_ovl16_01_8004370C(u32 (*arg0)[3], s32 arg1) {
    s32 i;

    for (i = 0; i < arg1; i++, arg0++) {
        func_ovl16_01_80044F98((*arg0)[0], (*arg0)[1], (*arg0)[2]);
    }
}

void func_ovl16_01_80043784(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 i;

    D_ovl16_01_800452D0.x = arg1 * 100.0f;
    D_ovl16_01_800452D0.y = 0.0f;
    for (i = 0; i < arg2; i++) {
        D_ovl16_01_800452D0.y += func_ovl16_01_800440D0(i) * 100.0f;
    }
    D_ovl16_01_800452D0.y += 100.0f;
    D_ovl16_01_800452D0.z = arg3 * 100.0f;
    D_ovl16_01_800452C0 = func_ovl16_01_80044F28(arg0, &D_ovl16_01_800452D0, 0);
}

void func_ovl16_01_8004389C(void) {
    func_ovl16_01_80044F30(D_ovl16_01_800452C0);
}

void func_ovl16_01_800438CC(s32 arg0, s32 arg1, s32 arg2) {
    D_ovl16_01_800452C4 = arg0;
    D_ovl16_01_800452C8 = arg1;
    func_ovl16_01_80044158(arg0, arg1, arg2);
}

void func_ovl16_01_80043924(void) {
    D_ovl16_01_800452C4 = -1;
    D_ovl16_01_800452C8 = -1;
}

void func_ovl16_01_8004394C(f32 arg0, f32 arg1, f32 arg2) {
    func_ovl16_01_80044840(D_ovl16_01_800452D0.x, D_ovl16_01_800452D0.y, D_ovl16_01_800452D0.z);
    func_ovl16_01_80044860(arg0, arg1, arg2);
    func_ovl16_01_80044B68(180);
}

void func_ovl16_01_800439B0(f32 arg0) {
    f32 sp1C;

    func_ovl16_01_80044868(&sp1C, NULL, NULL);
    func_ovl16_01_80044860(sp1C + arg0, 32768.0f, 32768.0f);
}

void func_ovl16_01_80043A00(s32 arg0, UnkStructA *arg1) {
    func_ovl16_01_80045010(arg0, arg1->unk40->unk10, arg1->unk40->unk14, arg1->unk40->unk18);
    return; /* a second branch to the epilogue */
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl16_01_80043A54(s32 arg0) {
    switch (arg0) {
        case 2:
            func_ovl16_01_80043B7C();
            break;
        case 16:
            break;
        case 4:
            func_ovl16_01_80043BBC();
            break;
        case 5:
            func_ovl16_01_80043C6C();
            break;
        case 0x50:
            break;
        case 0x52:
            break;
        case 0x53:
            break;
        case 3:
            func_ovl16_01_80043C98();
            break;
        case 12:
            func_ovl16_01_80043CE0();
            break;
        case 1:
            func_ovl16_01_80043D18();
            break;
    }
}

void func_ovl16_01_80043B7C(void) {
    func_ovl16_01_80044068(0x233);
    func_ovl16_01_80044080(0x234);
    func_ovl16_01_800441F8();
    func_ovl16_01_80044D80(0x400);
}

void func_ovl16_01_80043BBC(void) {
    func_ovl16_01_80044200(0x17, 0, 0x10);
    func_ovl16_01_80044258();
    func_ovl16_01_80044860(0.0f, 32768.0f, 32768.0f);
    func_ovl16_01_80043504();
    func_ovl16_01_80044F98(0, 7, 0xCE);
    func_ovl16_01_80044FA8(0, D_ovl16_01_800452A0.x, D_ovl16_01_800452A0.y, D_ovl16_01_800452A0.z - 500.0f);
    func_ovl16_01_800448B8(0);
    func_ovl16_01_80044860(0.0f, 45.0f, 32768.0f);
}

void func_ovl16_01_80043C6C(void) {
    func_ovl16_01_80044768(0x269, 3);
}

void func_ovl16_01_80043C98(void) {
    if (func_ovl16_01_80044FE0() == 0) {
        func_ovl16_01_80044D78(0x400);
        func_ovl16_01_80044158(0x20, 0x11, 0);
    }
}

void func_ovl16_01_80043CE0(void) {
    if (func_ovl16_01_80044D88(0x400) != 0) {
        func_ovl16_01_80044238(1);
    }
}

void func_ovl16_01_80043D18(void) {
}
