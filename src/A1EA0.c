/*
 * main_code's A1EA0, IDO -O3; it matches in us, eu and jp: the two
 * D_802B0260 entries and the task that updates them. Its .data
 * (D_802A1B50, the D_802A1B54 table) and .bss (D_802B0260, D_802B0300,
 * D_802B0308) are its own, placed in data_split.txt. func_802876A0's
 * task table is a static of its own (as A4CE0's func_8028A87C has): a
 * function with a static isn't inlined, so the registration stays out of
 * line in func_80287FD4 (dead calls stood in for it).
 */
#include "common.h"
#include "slots.h"
#include "412F0.h"
#include "4DE50.h"
#include "4E030.h"
#include "4ED50.h"
#include "54E40.h"
#include "7D840.h"
#include "A1EA0.h"
#include "404C0.h"
#include "530C0.h"

extern UnkStruct_800BA8E8 D_800BA8E8[6];
extern UnkStruct_800BA900 D_800BA900[16];
extern UnkStruct_800BB340 D_800BB340[16];
extern s32 D_800BB480;
extern UnkStruct_80288040 *D_800BB484;

s32 func_80238A10(void *, void *, PORT_RET(s32, void *), s32, s32);
void func_80239094(void);
f32 sinf(f32);

extern u8 D_800BA90C[];
extern u8 D_800BB488[20];
extern s32 D_800BC520;
extern s32 D_802A1AF0[][4];
extern UnkStruct_802B0260 D_802B0260[2];
extern UnkStruct_802B0300 *D_802B0300[2];
extern UnkStruct_802B0308 *D_802B0308[2];

void func_802876C8(UnkStruct_802B0260 *arg0);
void func_8028773C(UnkStruct_802B0260 *arg0);
void func_802878BC(UnkStruct_802B0260 *arg0, s32 arg1);
void func_802878E0(UnkStruct_802B0260 *arg0, s32 arg1);
static void func_80287904(void);
UnkStruct_802B0260 *func_80287DC0(s32 arg0);

#define LOAD_80285A7C(a_, i_) \
    { \
        void *f; \
        f = func_80226604(0xDD, 1); \
        func_80225CCC((void *) f, D_800BA8E8[(i_)].unk0 * 20, 0); \
        func_8022616C(D_800BB488, 1, 20, f); \
        func_80226368(f); \
        (a_)->unk1F = D_800BB488[0]; \
        (a_)->unk3E = D_800BB488[1]; \
        (a_)->unk3C = D_800BB488[2]; \
        (a_)->unk3D = D_800BB488[3]; \
        (a_)->unk50.b[0] = D_800BB488[4]; \
        (a_)->unk1E = D_800BB488[5]; \
        (a_)->unk24 = (D_800BB488[6] << 24) + (D_800BB488[7] << 16) + (D_800BB488[8] << 8) + D_800BB488[9]; \
        (a_)->unk2C = 2.0f * (s16) ((D_800BB488[10] << 8) + D_800BB488[11]) * 0.01f; \
        (a_)->unk1D = D_800BB488[12]; \
        (a_)->unk44 = D_800BB488[13]; \
        (a_)->unk6C = D_800BB488[14]; \
        (a_)->unk6D = D_800BB488[15]; \
        (a_)->unk70 = (u32) D_800BB488[16]; \
        (a_)->unk74 = (u32) D_800BB488[17]; \
        (a_)->unk7C = D_800BB488[18]; \
        (a_)->unk7E = D_800BB488[19]; \
    }

#define HARD_80285A84(a_) \
    if (func_8023A840()->unk10 == 0) { \
        (a_)->unk2C *= 0.5f; \
        if ((a_)->unk6D >= 2) { \
            (a_)->unk6D--; \
        } \
        (a_)->unk3E *= 2; \
    }

#define FREESLOT_802867CC(r_) \
    { \
        UnkStruct_800BA900 *s_; \
        s32 i_; \
        (r_) = -1; \
        s_ = D_800BA900; \
        for (i_ = 0; i_ < 16; i_++, s_++) { \
            if (s_->unk0 == 0) { \
                (r_) = i_; \
                break; \
            } \
        } \
    }

#define FREETASK_802867D4(r_) \
    { \
        UnkStruct_800BB340 *t_; \
        s32 i_; \
        s32 n_; \
        n_ = 0; \
        (r_) = -1; \
        t_ = D_800BB340; \
        for (i_ = 1; i_ < 16; i_++, t_++) { \
            if (t_->unk4 != 0) { \
                n_++; \
            } \
        } \
        if (n_ < D_800BB480) { \
            t_ = D_800BB340; \
            for (i_ = 0; i_ < 16; i_++, t_++) { \
                if (t_->unk4 == 0) { \
                    (r_) = i_; \
                    break; \
                } \
            } \
        } \
    }

#define DEC_80284CA4(a_) ((--(a_)->unk6D == 0) ? 1 : 0)

s32 D_802A1B50 = -1;
UnkStruct_802B0260 D_802B0260[2];
UnkStruct_802B0300 *D_802B0300[2];
UnkStruct_802B0308 *D_802B0308[2];

void func_80287FD4(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        D_802B0260[i].unk0 = i;
        func_802876C8(&D_802B0260[i]);
    }
    func_802876A0();
    D_802A1B50 = -1;
}

UnkStruct_802B0260 *func_80287DC0(s32 arg0) {
    UnkStruct_802B0260 *e;
    s32 i;

    e = D_802B0260;
    for (i = 0; i < 2; i++, e++) {
        if (e->unk4 == 0) {
            e->unk0 = i;
            e->unk8 = arg0;
            e->unk4 |= 0x80000000;
            e->unkC = 0x400;
            e->unk10 = 0x40;
            D_802B0300[e->unk0] = func_8026CE28(arg0);
            D_802B0308[e->unk0] = func_8022A6C4(D_802B0300[e->unk0], e->unk10);
            e->unkC |= D_802B0308[e->unk0]->unkC;
            e->unk48 = D_802B0300[e->unk0]->unk10;
            e->unk4C = D_802B0300[e->unk0]->unk12;
            D_802B0308[e->unk0]->unk10 = e->unk28;
            D_802B0308[e->unk0]->unk14 = e->unk2C;
            D_802B0308[e->unk0]->unk18 = e->unk30;
            D_802B0308[e->unk0]->unk1C = e->unk34;
            D_802B0308[e->unk0]->unk20 = 0.0f;
            D_802B0308[e->unk0]->unk24 = 0.0f;
            if (D_802A1B50 == -1) {
                D_802A1B50 = func_80238A10(func_80287904, 0, 0, 0, 0x403);
                func_802276FC(0);
            }
            return e;
        }
    }
    return NULL;
}

static void func_80287904(void) {
    s32 j;
    f32 u[2];
    f32 v[2];
    f32 w;
    int t1;
    f32 t2;
    f32 t3;
    f32 rot[3];
    f32 cam[3];
    f32 h;
    f32 du;
    f32 dv;
    UnkStruct_802B0260 *e;
    s32 i;

    func_802348B4(&cam[0], &cam[1], &cam[2]);
    for (j = 0; j < 2; j++) {
        u[j] = 0.0f;
        v[j] = 0.0f;
    }
    for (;;) {
        e = D_802B0260;
        for (i = 0; i < 2; i++, e++) {
            if (!(e->unk4 & 0x80000000)) {
                continue;
            }
            if (e->unk4 & 0x20000000) {
                e->unkC &= ~1;
            } else {
                e->unkC |= 1;
            }
            D_802B0308[i]->unkC = e->unkC;
            D_802B0308[i]->unk8 = e->unk10;
            D_802B0308[i]->unk36 = e->unk14;
            D_802B0308[i]->unk10 = e->unk28;
            D_802B0308[i]->unk14 = e->unk2C;
            D_802B0308[i]->unk18 = e->unk30;
            D_802B0308[i]->unk1C = e->unk34;
            if (!(e->unk4 & 0x20000000)) {
                continue;
            }
            D_802B0308[i]->unk28 = e->unk48 * e->unk40;
            D_802B0308[i]->unk2C = e->unk4C * e->unk44;
            w = e->unk48;
            h = e->unk4C;
            switch (e->unk4 & 0xF) {
            case 0:
                func_80234858(&rot[0], &rot[1], &rot[2]);
                t1 = func_802336E4(rot[0]);
                if (t1 != 0) {
                    du = 0.0f;
                } else if (func_80234768() == 1) {
                    du = w - (w / (360.0f / rot[0]));
                } else {
                    du = w / (360.0f / rot[0]);
                }
                dv = func_80234434((rot[1] >= 360.0f) ? rot[1] - 360.0f : ((rot[1] < 0.0f) ? rot[1] + 360.0f : rot[1])) * (h * e->unk44 * 0.5f);
                break;
            case 1:
                e->unk20 += e->unk18;
                e->unk24 += e->unk1C;
                if (e->unk20 < 0.0f) {
                    e->unk20 += e->unk48;
                }
                if (e->unk48 < e->unk20) {
                    e->unk20 -= e->unk48;
                }
                if (e->unk24 < 0.0f) {
                    e->unk24 += e->unk4C;
                }
                if (e->unk4C < e->unk24) {
                    e->unk24 -= e->unk4C;
                }
                du = e->unk20;
                dv = e->unk24;
                break;
            default:
                for (;;) {
                }
            }
            u[i] = e->unk38 + du;
            t2 = func_80263DC8(u[i], w);
            u[i] = t2;
            v[i] = e->unk3C + dv;
            t3 = func_80263DC8(v[i], h);
            v[i] = t3;
            D_802B0308[e->unk0]->unk20 = u[i];
            D_802B0308[e->unk0]->unk24 = v[i];
        }
        func_80239094();
    }
}

void func_802878E0(UnkStruct_802B0260 *arg0, s32 arg1) {
    if (!(arg0->unk4 & 0x80000000)) {
        for (;;) {
        }
    }
    arg0->unk4 |= arg1;
}

void func_802878BC(UnkStruct_802B0260 *arg0, s32 arg1) {
    if (!(arg0->unk4 & 0x80000000)) {
        for (;;) {
        }
    }
    arg0->unk4 &= ~arg1;
}

void func_8028785C(void) {
    UnkStruct_802B0260 *e;
    s32 i;

    e = D_802B0260;
    for (i = 0; i < 2; i++, e++) {
        if (e->unk4 != 0) {
            func_8028773C(e);
        }
    }
}

void func_8028773C(UnkStruct_802B0260 *arg0) {
    UnkStruct_802B0260 *e;
    s32 last;
    s32 i;
    s32 n;

    if (!(arg0->unk4 & 0x80000000)) {
        for (;;) {
        }
    }
    func_8022A1E8(D_802B0308[arg0->unk0]);
    last = 1;
    e = D_802B0260;
    for (i = 0; i < 2; i++, e++) {
        if ((arg0->unk0 != e->unk0) && (arg0->unk8 == e->unk8)) {
            last = 0;
        }
    }
    if (last != 0) {
        func_8026CCBC(arg0->unk8);
    }
    func_802876C8(arg0);
    n = 0;
    arg0 = D_802B0260;
    for (i = 0; i < 2; i++, arg0++) {
        if (arg0->unk4 != 0) {
            n++;
        }
    }
    if (n == 0) {
        if (D_802A1B50 != -1) {
            func_80238EA8(D_802A1B50);
            D_802A1B50 = -1;
            func_802276FC(1);
        }
    }
}

void func_802876C8(UnkStruct_802B0260 *arg0) {
    arg0->unk4 = 0;
    arg0->unk8 = -1;
    arg0->unkC = 0;
    arg0->unk10 = 0;
    arg0->unk14 = 0xFF;
    arg0->unk18 = 0.0f;
    arg0->unk1C = 0.0f;
    arg0->unk20 = 0.0f;
    arg0->unk24 = 0.0f;
    arg0->unk28 = 0.0f;
    arg0->unk2C = 0.0f;
    arg0->unk38 = 0.0f;
    arg0->unk3C = 0.0f;
    arg0->unk48 = 0.0f;
    arg0->unk4C = 0.0f;
    arg0->unk40 = 1.0f;
    arg0->unk44 = 1.0f;
    arg0->unk30 = 320.0f;
    arg0->unk34 = 240.0f;
}

void func_802876A0(void) {
    static void *D_802A1B54[] = { func_80287DC0, func_8028773C, func_802878E0, func_802878BC };

    func_80297D30(0x1A, D_802A1B54);
}

