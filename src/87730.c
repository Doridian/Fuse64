#include "common.h"
#include "426E0.h"
#include "54E40.h"
#include "87190.h"
#include "87730.h"
#include "53D70.h"
#include "86680.h"

typedef struct {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ s32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ f32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
} UnkStruct_802AFC44;

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ void *unk4;
} UnkStruct_800BCDE8; /* size 0x8 */

#include "gfx.h"

void *func_8022A328(void *, s32, s32, s32, s32);
PORT_RET(void, s32) func_8023903C(s32);
void func_80239094(void);
void func_8026D2C4(UnkStructA_40 *, Gfx_80225840 **);

extern u8 D_1000000[];
extern UnkStruct_800BCDE8 D_800BCDE8[64];
void func_8026D518(s32 arg0, s32 arg1);
void func_8026D354(void *arg0);
PORT_RET(void, PORT_IP(s32)) func_8026D4B0(void (*arg0)(UnkStructA_40 *, Gfx_80225840 **), s32 arg1);
void func_8026D2A4(s32 arg0, s32 arg1);
void func_8026CFF4(s32 arg0, s32 arg1, s32 arg2);
/* 426E0's (in its table only here; UnkObj is 426E0.c's) */
void func_8022984C(void *, s32);
void func_80229F74(void *, s32);
s32 func_80229C90(void *, s32, s16, s16, s16, s16);
void func_8022979C(void *, s32, s32);

void *D_802A13E0 = NULL;
/* ZeroJump module 21's table (func_8026CF30); the zeros end it, as in
   87190's */
void *D_802A13E4[] = {
    func_8026D518, func_8026D3AC, func_8026D354, func_8022946C, func_8022A19C, func_8022984C,
    func_802297B8, func_80229560, func_80229F74, func_80229E38, func_8022A0D8, func_80229D68,
    func_80229D3C, func_80229C90, func_80229BF4, func_8022979C, func_8026D4B0, func_8026D2A4,
    func_802280F4, func_80228008, func_80227EE0, func_80227FE8, func_8026CFF4, func_80229BB4,
    NULL,          NULL,          NULL,
};

s32 D_802AFC40;
UnkStruct_802AFC44 *D_802AFC44;
UnkStruct_802AFC44 *D_802AFC48;

void func_8026CF30(void) {
    s32 i;

    func_80297D30(21, D_802A13E4);
    for (i = 0; i < 64; i++) {
        D_800BCDE8[i].unk4 = NULL;
        D_800BCDE8[i].unk0 = 0;
    }
}

void func_8026CF90(void) {
    if (D_802A13E0 != NULL) {
        D_802AFC40 = 1;
        func_8022A1E8(D_802AFC44);
        func_8022A1E8(D_802AFC48);
        func_8026CBD4(D_802A13E0);
        D_802A13E0 = NULL;
    }
}

void func_8026CFF4(s32 arg0, s32 arg1, s32 arg2) {
    s32 i;

    D_802AFC40 = 0;
    if (!(func_8023A840()->unk0 & 6)) {
        func_8026C660(0x28);
    }
    func_8023A864(0);
    func_80243698(1);
    D_802AFC48 = func_8022A328(func_8026D2C4, 0x40, 1, 0, 0);
    D_802AFC48->unkC |= 0x90;
    D_802A13E0 = func_8026CE28(arg0);
    D_802AFC44 = func_8022A6C4(D_802A13E0, 0x40);
    D_802AFC44->unk10 = 32.0f;
    D_802AFC44->unk14 = 174.0f;
    D_802AFC44->unk18 = 288.0f;
    D_802AFC44->unk1C = 222.0f;
    D_802AFC44->unk20 = 0.0f;
    D_802AFC44->unk24 = 0.0f;
    D_802AFC44->unk28 = 256.0f;
    D_802AFC44->unk2C = 48.0f;
    D_802AFC44->unkC |= 0x280;
    if (arg2 == 0) {
        func_8023903C(2);
    }
    while (TRUE) {
        if (arg2 != 0) {
            for (i = 0; i < arg2; i++) {
                if (D_802AFC40 != 0) {
                    return;
                }
                func_80239094();
            }
        } else {
            while (!(func_802399AC(0) & 0xC000)) {
                func_80239094();
            }
        }
        if (--arg1 == 0) {
            break;
        }
        if (D_802A13E0 == NULL || D_802AFC44 == NULL) {
            DEBUG_PRINT(("D_802A13E0 or D_802AFC44 == NULL\n"));
        }
        for (i = 0; i < 48; i++) {
            if (D_802AFC40 != 0) {
                return;
            }
            D_802AFC44->unk24 += 1.0f;
            func_80239094();
        }
    }
    func_8022A1E8(D_802AFC44);
    func_8022A1E8(D_802AFC48);
    func_8026CBD4(D_802A13E0);
    D_802A13E0 = NULL;
    if (D_802AFC40 != 0) {
        return;
    }
    func_80239094();
    func_8023A864(1);
    func_80243698(0);
}

void func_8026D2A4(s32 arg0, s32 arg1) {
    func_8026CFF4(arg0, arg1, 0);
}

void func_8026D2C4(UnkStructA_40 *arg0, Gfx_80225840 **arg1) {
    Gfx_80225840 *gfx = *arg1;

    /* one line per command, as gbi.h's macros expand: IDO allocates
     * temporaries per source line */
    { Gfx_80225840 *g = gfx++; g->w0 = 0xFA000000; g->w1 = 0x80; }
    { Gfx_80225840 *g = gfx++; g->w0 = 0xFCFFFFFF; g->w1 = 0xFFFDF6FB; }
    { Gfx_80225840 *g = gfx++; g->w0 = 0xFF10013F; g->w1 = (PORT_UP(u32)) D_1000000; }
    { Gfx_80225840 *g = gfx++; g->w0 = 0xF7000000; g->w1 = 0x10001; }
    { Gfx_80225840 *g = gfx++; g->w0 = 0xF648C384; g->w1 = 0x702A8; }
    *arg1 = gfx;
}

void func_8026D354(void *arg0) {
    s32 i;

    func_8022A1E8(arg0);
    for (i = 0; i < 64; i++) {
        if (arg0 == D_800BCDE8[i].unk4) {
            D_800BCDE8[i].unk4 = NULL;
            break;
        }
    }
}

void func_8026D3AC(void *arg0) {
    s32 i;

    func_8022A1E8(arg0);
    for (i = 0; i < 64; i++) {
        if (arg0 == D_800BCDE8[i].unk4) {
            if (D_800BCDE8[i].unk0 != -1) {
                func_8026CCBC(D_800BCDE8[i].unk0);
            }
            D_800BCDE8[i].unk4 = NULL;
            D_800BCDE8[i].unk0 = 0;
            break;
        }
    }
}

s32 func_8026D428(void) {
    s32 i;

    for (i = 0; i < 64; i++) {
        if (D_800BCDE8[i].unk4 == NULL) {
            return i;
        }
    }
    return -1;
}

PORT_RET(void, PORT_IP(s32)) func_8026D4B0(void (*arg0)(UnkStructA_40 *, Gfx_80225840 **), s32 arg1) {
    s32 i = func_8026D428();

    D_800BCDE8[i].unk0 = -1;
    PORT_V0(D_800BCDE8[i].unk4 = func_8022A328((void *) arg0, arg1 + 0x40, 1, 0, 0));
PORT_ONLY(return (intptr_t)port_v0;) }

void func_8026D518(s32 arg0, s32 arg1) {
    s32 i;
    void *data;

    data = func_8026CE28(arg0);
    i = func_8026D428();

    D_800BCDE8[i].unk0 = arg0;
    PORT_V0(D_800BCDE8[i].unk4 = func_8022A6C4(data, arg1 + 0x40));
}
