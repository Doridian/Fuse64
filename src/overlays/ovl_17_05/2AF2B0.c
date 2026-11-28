#include "common.h"
#include "overlays/common/types.h"

void func_ovl17_05_800433A8(void);
#define ZJ_0D06 func_ovl17_05_80043AD0
#define ZJ_0F11 func_ovl17_05_80043C08
#define ZJ_1012 func_ovl17_05_80043CC8
#define ZJ_1704 func_ovl17_05_800442C8
#define ZJ_1708 func_ovl17_05_800442E8
#define ZJ_1709 func_ovl17_05_800442F0
#define ZJ_1811 func_ovl17_05_80044488
#define ZJ_181A func_ovl17_05_800444D0
#define ZJ_1900 func_ovl17_05_800445D8
#define ZJ_1902 func_ovl17_05_800445E8
#define ZJ_1904 func_ovl17_05_800445F8
#define ZJ_1905 func_ovl17_05_80044600
#define ZJ_1909 func_ovl17_05_80044620
#define ZJ_2600 func_ovl17_05_80044A28
#define ZJ_2708 func_ovl17_05_80044A88
#define ZJ_2709 func_ovl17_05_80044A90
#define ZJ_270A func_ovl17_05_80044A98
#define ZJ_2B04 func_ovl17_05_80044B80

void func_ovl17_05_800432B0(UnkStructA *);
#define ZJ_0C00 func_ovl17_05_80043A88
#define ZJ_0D00 func_ovl17_05_80043AA0
#define ZJ_0E02 func_ovl17_05_80043B68
#define ZJ_0F10 func_ovl17_05_80043C00
#define ZJ_1000 func_ovl17_05_80043C38
#define ZJ_1700 func_ovl17_05_800442A8
#define ZJ_1702 func_ovl17_05_800442B8
#define ZJ_1711 func_ovl17_05_80044330
#define ZJ_1816 func_ovl17_05_800444B0
#define ZJ_2004 func_ovl17_05_800448D8
#define ZJ_2300 func_ovl17_05_80044940
#include "overlays/common/zerojump.h"

/* .bss */
u8 D_ovl17_05_80044E50[0x24];
s32 D_ovl17_05_80044E74;
s32 D_ovl17_05_80044E78;
Vec3f D_ovl17_05_80044E80;

/* .data */
/* passed to func_ovl17_05_80044940: three 12-byte records */
u32 D_ovl17_05_80044D80[][3] = {
    { 0xD40000, 0x5000C, 4 },
    { 0, 0, 4 },
    { 0, 0, 4 },
};
UnkStruct_ovl17_04_80044D84 D_ovl17_05_80044DA4[] = {
    { NULL, 0x20E, 0x40, 0.0f, 0.0f, 0.0f, 700.0f, 0x10, 9, 0x15 },
    { NULL, 0x20F, 0xC0, 0.0f, 0.0f, 0.0f, -700.0f, 0x1E, 9, 0x15 },
};

void func_ovl17_05_80043010(void) {
    UnkStruct_8023A834 *b;
    s32 i;

    b = func_ovl17_05_80043C08();
    if (b->unk38 == 1 && b->unk0 == 0) {
        for (i = 0; i < 2; i++) {
            func_ovl17_05_800445D8(func_ovl17_05_800433A8, &D_ovl17_05_80044DA4[i], 0, 0, 0x110);
        }
    }
}

void func_ovl17_05_80043098(void) {
    UnkStructA *obj;
    s32 i;

    for (i = 0; i < 2; i++) {
        obj = func_ovl17_05_800444D0(D_ovl17_05_80044DA4[i].unk4, 0xD0, D_ovl17_05_80044DA4[i].unkC,
                                     D_ovl17_05_80044DA4[i].unk10, D_ovl17_05_80044DA4[i].unk14);
        func_ovl17_05_80044488(obj, 0x01000000);
        D_ovl17_05_80044DA4[i].unk0 = obj;
    }
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl17_05_80043118(s32 arg0) {
    UnkStructA *obj;

    switch (arg0) {
        case 2:
            func_ovl17_05_80044940(D_ovl17_05_80044D80);
            func_ovl17_05_80043A88(0x20D);
            obj = func_ovl17_05_800444B0(0x210, 0x01080000, 0xD0, 0);
            obj->unk40->unk10 = 0.0f;
            obj->unk40->unk14 = 0.0f;
            obj->unk40->unk18 = 0.0f;
            obj->unk40->unk36 = 0xE6;
            func_ovl17_05_80043AA0(0x21D);
            func_ovl17_05_80043C38();
            func_ovl17_05_80043C00(0);
            func_ovl17_05_80044330(0x7F);
            func_ovl17_05_800442A8(1);
            func_ovl17_05_800442B8(1);
            func_ovl17_05_800442C8(2300.0f, 448.0f, 1630.0f);
            func_ovl17_05_800442E8(0.0f, 55.0f, 2450.0f);
            func_ovl17_05_80043B68(func_ovl17_05_800432B0, 5);
            func_ovl17_05_80043098();
            break;
        case 3:
        case 4:
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
            break;
        case 5:
            func_ovl17_05_80043010();
            break;
        case 1:
            func_ovl17_05_800448D8(0, 0, 0, 0.0f);
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

void func_ovl17_05_800432B0(UnkStructA *arg0) {
    s32 i;
    s32 r;
    s16 k;

    func_ovl17_05_80044A28(0xF, arg0->unk40->unk10, arg0->unk40->unk14, arg0->unk40->unk18, 0);
    if (arg0->unk8 & 1) {
        for (i = 0; i < 4; i++) {
            if (func_ovl17_05_80043CC8(i) == arg0) {
                break;
            }
        }
        if (i < 4) {
            k = i;
            if (func_ovl17_05_80044A90(k) != 0) {
                func_ovl17_05_80044A98(k, 0);
            }
        }
    } else {
        r = func_ovl17_05_80044A88(arg0);
        if (r != -1) {
            func_ovl17_05_80044A98(r, 0);
        }
    }
    arg0->unk5C = -600.0f;
    arg0->unk40->unk14 = -600.0f;
}

void func_ovl17_05_800433A8(void) {
    UnkStruct_ovl17_04_80044D84 *rec;
    UnkStruct_8023A834 *b;
    f32 acc;
    f32 step;
    s32 n;
    s32 flag;

    rec = func_ovl17_05_80044600();
    b = func_ovl17_05_80043C08();
    while (TRUE) {
        if (b->unk2C <= 0) {
            break;
        }
        func_ovl17_05_80044620();
    }
    acc = 0;
    n = rec->unk1C;
    flag = FALSE;
    step = (rec->unk18 - rec->unkC) / 3600.0f;
    while (TRUE) {
        if (ABS(acc) > 80.0f && !flag) {
            flag = TRUE;
            func_ovl17_05_80043AD0(n, 0, rec->unk20, n, 5, rec->unk24, 0x141F);
        }
        if (ABS(acc) >= 100.0f) {
            if (acc > 0.0f) {
                acc -= 100.0f;
                n++;
                flag = FALSE;
            } else {
                acc += 100.0f;
                n--;
                flag = FALSE;
            }
        }
        if (ABS(rec->unk0->unk40->unk10 - rec->unkC) > ABS(rec->unk18 - rec->unkC)) {
            func_ovl17_05_800445E8();
        }
        acc += step;
        rec->unk0->unk40->unk10 += step;
        func_ovl17_05_80044620();
    }
}

#define CORE_FUNC_D func_ovl17_05_800435F0
#define CORE_ZJ_2B04 func_ovl17_05_80044B80
#include "overlays/common/core_d.h"

void func_ovl17_05_80043624(f32 arg0) {
    f32 sp1C;

    func_ovl17_05_800442F0(&sp1C, NULL, NULL);
    func_ovl17_05_800442E8(sp1C + arg0, 32768.0f, 32768.0f);
}

void func_ovl17_05_8004366C(f32 arg0, f32 arg1, f32 arg2) {
    func_ovl17_05_800442C8(D_ovl17_05_80044E80.x, D_ovl17_05_80044E80.y, D_ovl17_05_80044E80.z);
    func_ovl17_05_800442E8(arg0, arg1, arg2);
    func_ovl17_05_800445F8(180);
}

void func_ovl17_05_800436C8(void) {
    D_ovl17_05_80044E74 = -1;
    D_ovl17_05_80044E78 = -1;
}

void func_ovl17_05_800436E0(void) {
}

void func_ovl17_05_800436E8(void) {
}

void func_ovl17_05_800436F0(void) {
}

void func_ovl17_05_800436F8(void) {
}

void func_ovl17_05_80043700(void) {
}

void func_ovl17_05_80043708(void) {
}

void func_ovl17_05_80043710(void) {
}

void func_ovl17_05_80043718(void) {
}

void func_ovl17_05_80043720(void) {
}

void func_ovl17_05_80043728(void) {
}

void func_ovl17_05_80043730(void) {
}

void func_ovl17_05_80043738(void) {
}
