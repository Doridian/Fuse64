/*
 * ovl_12_06's game object, IDO -O3; it matches in us, eu and jp.
 * The functions are written last to first (IDO -O3 emits a file's
 * functions in reverse).  func_ovl12_06_80045708 and func_ovl12_06_800456E8
 * are declared s32: as void, the handler and the task take $v0 where the
 * original takes $v1.
 *
 * How it came to match:
 * - The handler's frame (0xA0): case 11 is a static,
 *   func_ovl12_06_800430C8, inlined into the handler with its own locals.
 *   An inlined static's block of the frame lies below the slots of the
 *   statics inlined before it and above those inlined after it; its locals'
 *   order and `pad` place it.  Its stub is the one at 800430C8 (inlined
 *   statics come out after their callees, in reverse call order), so
 *   func_ovl12_06_800430D0's wait loop is written inline.
 * - Case 2: D_ovl12_06_80045FBC is stored in the delay slot of the static's
 *   first call because the handler stores it, not the static.
 * - func_ovl12_06_800439D4: the a/b/c loop counts in k and its inner loop
 *   in i (40 in $s0, the count in $s3).
 *
 * 2026-11-13: 1 `#line` that turned out redundant was removed (each tried away
 * against the sha1 in us, eu and jp). Where the notes above name a `#line`,
 * region or renumbering that is gone, it is no longer needed.
 */
#include "common.h"

/* What func_ovl12_06_80045770 returns (D_ovl12_06_80045FCC). */
typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ f32 unk3C;
    /* 0x40 */ f32 unk40;
    /* 0x44 */ f32 unk44;
    /* 0x48 */ u8 unk48[4];
    /* 0x4C */ f32 unk4C;
} UnkStruct_ovl12_06_80045FCC;

#define ZJ_0C00 func_ovl12_06_80044B98
#define ZJ_0D00 func_ovl12_06_80044BB0
#define ZJ_0F01 func_ovl12_06_80044C98
#define ZJ_0F12 func_ovl12_06_80044D20
#define ZJ_0F14 func_ovl12_06_80044D30
#define ZJ_0F15 func_ovl12_06_80044D38
#define ZJ_1000 func_ovl12_06_80044D48
#define ZJ_1001 func_ovl12_06_80044D50
#define ZJ_1008 func_ovl12_06_80044D88
#define ZJ_1009 func_ovl12_06_80044D90
#define ZJ_100C func_ovl12_06_80044DA8
#define ZJ_1012 func_ovl12_06_80044DD8
#define ZJ_1018 func_ovl12_06_80044E08
#define ZJ_101C func_ovl12_06_80044E28
#define ZJ_1500 func_ovl12_06_80045278
#define ZJ_1501 func_ovl12_06_80045280
#define ZJ_1509 func_ovl12_06_800452C0
#define ZJ_150C func_ovl12_06_800452D8
#define ZJ_1511 func_ovl12_06_80045300
#define ZJ_1704 func_ovl12_06_800453D8
#define ZJ_1708 func_ovl12_06_800453F8
#define ZJ_1709 func_ovl12_06_80045400
#define ZJ_1710 func_ovl12_06_80045438
#define ZJ_1711 func_ovl12_06_80045440
#define ZJ_1803 func_ovl12_06_80045528
#define ZJ_1806 func_ovl12_06_80045540
#define ZJ_1812 func_ovl12_06_800455A0
#define ZJ_1814 func_ovl12_06_800455B0
#define ZJ_1816 func_ovl12_06_800455C0
#define ZJ_1821 func_ovl12_06_80045618
#define ZJ_1900 func_ovl12_06_800456E8
#define ZJ_1902 func_ovl12_06_800456F8
#define ZJ_1903 func_ovl12_06_80045700
#define ZJ_1904 func_ovl12_06_80045708
#define ZJ_1909 func_ovl12_06_80045730
#define ZJ_190C func_ovl12_06_80045748
#define ZJ_190E func_ovl12_06_80045758
#define ZJ_190F func_ovl12_06_80045760
UnkStruct_ovl12_06_80045FCC *func_ovl12_06_80045770(s32); /* 0x1A00, kept: it keeps the return in UnkStruct_ovl12_06_80045FCC * (the header's UnkStruct_802B0260 *) */
void func_ovl12_06_80045780(UnkStruct_ovl12_06_80045FCC *, s32); /* 0x1A02, kept: its callers pass UnkStruct_ovl12_06_80045FCC * (the header's UnkStruct_802B0260 *) */
#define ZJ_1D00 func_ovl12_06_80045890
#define ZJ_1D01 func_ovl12_06_80045898
#define ZJ_1D0F func_ovl12_06_80045908
#define ZJ_1D10 func_ovl12_06_80045910
#define ZJ_2600 func_ovl12_06_80045B38
#define ZJ_2B00 func_ovl12_06_80045C70
#define ZJ_2B02 func_ovl12_06_80045C80
#define ZJ_2B03 func_ovl12_06_80045C88
#define ZJ_2B04 func_ovl12_06_80045C90
#define ZJ_2D00 func_ovl12_06_80045D18
#define ZJ_2D03 func_ovl12_06_80045D30
#define ZJ_2D04 func_ovl12_06_80045D38
#define ZJ_3000 func_ovl12_06_80045DE8
#define ZJ_3001 func_ovl12_06_80045DF0
#include "overlays/common/zerojump.h"

void func_ovl12_06_80043010(void);
void func_ovl12_06_800430D0(void);
void func_ovl12_06_80043164(s32);
void func_ovl12_06_800439D4(void);
void func_ovl12_06_800446A0(void);
s32 func_ovl12_06_800446F8(s32, void *);
void func_ovl12_06_8004472C(f32);
void func_ovl12_06_80044774(f32, f32, f32);
void func_ovl12_06_800447D0(void);
void func_ovl12_06_800447E8(void);
static f32 func_ovl12_06_80043068(UnkStructA_40 *from, UnkStructA_40 *to);
static void func_ovl12_06_800430C8(void);

/* .bss */
u8 D_ovl12_06_80045F80[0x24];
s32 D_ovl12_06_80045FA4;
s32 D_ovl12_06_80045FA8;
Vec3f D_ovl12_06_80045FB0;
s32 D_ovl12_06_80045FBC;
s32 D_ovl12_06_80045FC0;
s32 D_ovl12_06_80045FC4;
UnkStruct_800BCFE8 *D_ovl12_06_80045FC8;
UnkStruct_ovl12_06_80045FCC *D_ovl12_06_80045FCC;

/* an angle in degrees moved by d and kept in [0, 360) */
#define ANGLE_ADD(a, d) \
    (a) = ((a) + (d) >= 360.0f) ? (a) + (d) - 360.0f : ((a) + (d) < 0.0f) ? (a) + (d) + 360.0f : (a) + (d)
#define ANGLE_SUB(a, d) \
    (a) = ((a) - (d) >= 360.0f) ? (a) - (d) - 360.0f : ((a) < (d)) ? (a) - (d) + 360.0f : (a) - (d)

/* unused: the stubs IDO -O3 leaves at the end of the object; the last one
 * refers to the .bss nothing else uses, which would otherwise be common */
static void func_ovl12_06_80044848(void) {
    D_ovl12_06_80045F80[0] = 0;
}

static void func_ovl12_06_80044840(void) {
}

static void func_ovl12_06_80044838(void) {
}

static void func_ovl12_06_80044830(void) {
}

static void func_ovl12_06_80044828(void) {
}

static void func_ovl12_06_80044820(void) {
}

static void func_ovl12_06_80044818(void) {
}

static void func_ovl12_06_80044810(void) {
}

static void func_ovl12_06_80044808(void) {
}

static void func_ovl12_06_80044800(void) {
}

static void func_ovl12_06_800447F8(void) {
}

void func_ovl12_06_800447E8(void) {
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl12_06_800447D0
#define CORE_FUNC_B func_ovl12_06_80044774
#define CORE_FUNC_C func_ovl12_06_8004472C
#define CORE_FUNC_D func_ovl12_06_800446F8
#define CORE_BSS_A0 D_ovl12_06_80045FA4
#define CORE_BSS_A1 D_ovl12_06_80045FA8
#define CORE_BSS_B D_ovl12_06_80045FB0
#include "overlays/common/core.h"











void func_ovl12_06_800446A0(void) {
    if (D_ovl12_06_80045FBC == 0x10) {
        func_ovl12_06_80045708(0x14F);
        func_ovl12_06_80045C80(10);
    } else {
        func_ovl12_06_80045708(45);
        func_ovl12_06_80045C80(18);
    }
    func_ovl12_06_800456F8();
}

void func_ovl12_06_800439D4(void) {
    UnkStructA *obj;
    UnkStructA *spr;
    s32 i;
    UnkStructA_40 *objs[4];
    s32 j;
    s32 k;
    f32 angle;
    f32 a[3] = { 315.0f, 45.0f, 0.0f };
    f32 b[3] = { 330.0f, 330.0f, 20.0f };
    f32 c[3] = { 315.0f, 0.0f, 45.0f };

    func_ovl12_06_80044DD8(0);
    obj = func_ovl12_06_80044DD8(1);
    func_ovl12_06_80045708(60);
    func_ovl12_06_80045C70(0x23B);
    obj->unk40->unk14 = 1000.0f;
    obj->unk40->unk10 += 10.0f;
    for (i = 0; i < 4; i++) {
        objs[i] = func_ovl12_06_80045278(0x307, 0);
        objs[i]->unkC |= 0x80;
        objs[i]->unk36 = 60;
        objs[i]->unk10 = 2650.0f;
        objs[i]->unk14 = 900.0f;
        objs[i]->unk18 = 500.0f;
        objs[i]->unk1C = 90.0f;
        objs[i]->unk28 = objs[i]->unk2C = objs[i]->unk30 = (i + 1) * 0.5f;
    }
    angle = 45.0f;
    for (i = 0; i < 180; i++) {
        func_ovl12_06_800453F8(32768.0f, angle, 32768.0f);
        ANGLE_SUB(angle, 0.33333334f);
        for (j = 0; j < 4; j++) {
            objs[j]->unk10 -= (j + 1) * 5.0f;
            objs[j]->unk14 -= (j + 1) * 5.0f;
        }
        func_ovl12_06_80045730();
    }
    for (i = 0; i != 4; i++) {
        func_ovl12_06_80045280(objs[i]);
    }
    func_ovl12_06_80045708(30);
    func_ovl12_06_800453D8(obj->unk40->unk10, obj->unk40->unk14 + 50.0f, obj->unk40->unk18);
    for (k = 0; k < 3; k++) {
        angle = 2250.0f;
        for (i = 0; i < 40; i++) {
            if (D_ovl12_06_80045FBC == 0x1B) {
                func_ovl12_06_800453F8(c[k], b[k], angle);
            } else {
                func_ovl12_06_800453F8(a[k], b[k], angle);
            }
            angle -= 42.5f;
            func_ovl12_06_80045730();
        }
    }
    func_ovl12_06_800452C0(obj->unk40, 0x21, 0);
    func_ovl12_06_800452C0(obj->unk40, -1, 1);
    obj->unk40->unk48->unk4[0].unk8 |= 0x20;
    obj->unk40->unk48->unk4[0].unk8 &= ~0x40;
    func_ovl12_06_80045708(17);
    func_ovl12_06_80045C70(0x239);
    func_ovl12_06_80045708(16);
    func_ovl12_06_80045C70(0x239);
    func_ovl12_06_80045708(16);
    func_ovl12_06_80045C70(0x239);
    while (!(obj->unk40->unk48->unk4[0].unk8 & 0x40)) {
        func_ovl12_06_80045730();
    }
    if (D_ovl12_06_80045FBC == 0x1A) {
        func_ovl12_06_80045708(30);
        func_ovl12_06_80045DE8();
        D_ovl12_06_80045FC0 = 0;
        func_ovl12_06_800456F8();
    } else if (D_ovl12_06_80045FBC == 0x1B) {
        func_ovl12_06_80044E28(1, 1);
        obj->unk40->unk48->unk4[0].unk8 |= 0x40;
        k = 0;
        obj->unk40->unk48->unk4[1].unk8 |= 0x40;
        obj->unk68 = 0.0f;
        obj->unk6C = 0.0f;
        obj->unk70 = 0.0f;
        for (i = 0; i < 6; i++) {
            for (angle = 0.0f; angle < func_ovl12_06_800452D8(obj->unk40, 0); angle += 4.0f) {
                obj->unk40->unk48->unk4[0].unk4 = angle;
                obj->unk40->unk48->unk4[1].unk4 = angle;
                obj->unk40->unk1C = func_ovl12_06_80045898(k) * 25.0f;
                ANGLE_ADD(k, 10);
                func_ovl12_06_80045730();
            }
        }
        func_ovl12_06_80044E28(1, 0x13);
        func_ovl12_06_80045B38(0, obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, 0);
        for (i = 0; i < 20; i++) {
            obj->unk40->unk18 += 5.0f;
            ANGLE_SUB(obj->unk40->unk1C, 2.5f);
            func_ovl12_06_80045730();
        }
        func_ovl12_06_80045708(20);
        spr = func_ovl12_06_800455C0(0x2D1, 0x01000000, 0xD0, 0);
        spr->unk40->unk1C = 270.0f;
        spr->unk40->unk20 = 0.0f;
        spr->unk40->unk2C = 0.2f;
        spr->unk40->unk28 = spr->unk40->unk2C;
        spr->unk40->unk30 = 0.1f;
        spr->unk40->unk10 = 2641.0f;
        spr->unk40->unk14 = 1080.0f;
        spr->unk40->unk18 = 415.0f;
        for (i = 0; i < 40; i++) {
            spr->unk40->unk30 += 0.05f;
            spr->unk40->unk14 -= 0.5f;
            func_ovl12_06_80045730();
        }
        func_ovl12_06_80045708(60);
        func_ovl12_06_800455B0(spr);
        for (i = 0; i < 30; i++) {
            obj->unk40->unk14 -= 26.666666f;
            func_ovl12_06_80045730();
        }
        func_ovl12_06_80045708(30);
        func_ovl12_06_80045DE8();
        D_ovl12_06_80045FC0 = 0;
        func_ovl12_06_800456F8();
    }
    angle = 0.0f;
    for (i = 0; i < 120; i++) {
        func_ovl12_06_800453F8(angle, func_ovl12_06_80045890(angle) * 20.0f + 20.0f, 32768.0f);
        ANGLE_ADD(angle, 2.25f);
        func_ovl12_06_80045730();
    }
    func_ovl12_06_80045760(0x70, 0x70);
    func_ovl12_06_800452C0(obj->unk40, 0x16, 0);
    func_ovl12_06_800452C0(obj->unk40, -1, 1);
    obj->unk40->unk48->unk4[0].unk8 |= 0x20;
    obj->unk40->unk48->unk4[0].unk8 &= ~0x40;
    for (i = 0; i < 7; i++) {
        func_ovl12_06_800453F8(angle, func_ovl12_06_80045890(angle) * 20.0f + 20.0f, 32768.0f);
        ANGLE_ADD(angle, 2.25f);
        func_ovl12_06_80045730();
    }
    obj->unk68 = 20.0f;
    obj->unk70 = 1.6333334f;
    for (i = 0; i < 30; i++) {
        if (i == 5) {
            obj->unk40->unk48->unk4[0].unk8 |= 0x40;
        }
        obj->unk40->unk10 -= 8.333333f;
        obj->unk40->unk18 += 20; /* an int: 20.0f shares the $f22 of the camera's 20.0f */
        func_ovl12_06_800453D8(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18);
        func_ovl12_06_800453F8(angle, func_ovl12_06_80045890(angle) * 20.0f + 20.0f, 32768.0f);
        ANGLE_ADD(angle, 2.25f);
        func_ovl12_06_80045730();
    }
    func_ovl12_06_80045438(16);
    func_ovl12_06_80045440(15);
    func_ovl12_06_80045D38(1);
    while (obj->unk68 != 0.0f) {
        func_ovl12_06_80045730();
    }
    func_ovl12_06_80045708(60);
    func_ovl12_06_80045758(0x70, 0x70, 0);
    func_ovl12_06_80044E08(1);
    func_ovl12_06_80044D88(0);
    func_ovl12_06_80044E08(0);
    D_ovl12_06_80045FC0 = 0;
    func_ovl12_06_80045748(D_ovl12_06_80045FC8->unkA0.h.lo);
    func_ovl12_06_800456F8();
}

/* The handler's cases 2 (after its first store, which the handler makes, as
 * ovl_12_05's case 2 before calling 80045B00), 16/26/27, 17 and 4: statics
 * inlined into it, each leaving its `jr ra; nop` stub just before it. */
static void func_ovl12_06_80043144(void) {
    func_ovl12_06_80044B98(0x236);
    func_ovl12_06_80044BB0(0x241);
    func_ovl12_06_80044D48();
    D_ovl12_06_80045FCC = func_ovl12_06_80045770(0x279);
    func_ovl12_06_80045780(D_ovl12_06_80045FCC, 0x20000000);
    D_ovl12_06_80045FCC->unk40 = 0.5f;
    D_ovl12_06_80045FCC->unk44 = D_ovl12_06_80045FCC->unk40 * 0.75f;
    D_ovl12_06_80045FCC->unk3C = D_ovl12_06_80045FCC->unk4C * 0.25f;
    D_ovl12_06_80045FC0 = 1;
    D_ovl12_06_80045FC4 = 0;
    func_ovl12_06_80045908();
}

static void func_ovl12_06_8004314C(void) {
    func_ovl12_06_80044D50(0x17, 0, 0xF);
    func_ovl12_06_80044D90(0);
    func_ovl12_06_80045D18(1, 3, 0x1A, 2, 3);
    D_ovl12_06_80045FC8 = func_ovl12_06_80045D30(1);
    D_ovl12_06_80045FC8->unk90 = 60;
    func_ovl12_06_80045700(D_ovl12_06_80045FC8->unkA0.h.lo, 0);
    func_ovl12_06_80045D38(0);
    func_ovl12_06_80044E08(1);
    func_ovl12_06_80044D88(1);
    func_ovl12_06_80044E08(0);
    func_ovl12_06_80044DD8(0)->unk40->unk10 += 50.0f;
    func_ovl12_06_80044DA8();
    func_ovl12_06_80045440(0x1F);
    func_ovl12_06_800453D8(2350.0f, 529.0f, 1500.0f);
    func_ovl12_06_800453F8(0.0f, 45.0f, 2000.0f);
    func_ovl12_06_80044D20(0x62, 0x11);
    func_ovl12_06_80044D88(1);
}

static void func_ovl12_06_80043154(void) {
    func_ovl12_06_80045D18(1, 3, 0x17, 0, 9);
    D_ovl12_06_80045FC8 = func_ovl12_06_80045D30(1);
    D_ovl12_06_80045FC8->unk90 = 180;
    func_ovl12_06_80044D50(0x17, 0, 0xF);
    func_ovl12_06_80044D90(0);
    func_ovl12_06_80044DD8(0)->unk40->unk10 += 50.0f;
    func_ovl12_06_80044DD8(1)->unk40->unk10 += 50.0f;
    func_ovl12_06_80044DA8();
    func_ovl12_06_80045440(0xF);
}

static void func_ovl12_06_8004315C(void) {
    if (D_ovl12_06_80045FBC != 0x1A && D_ovl12_06_80045FBC != 0x1B) {
        func_ovl12_06_800456E8(func_ovl12_06_800446A0, 0, 0, 0, 0x130);
    }
    /* `0x1A == ...`: the original's `beq v1,s0` (the other way round it's `beq s0,v1`) */
    if (D_ovl12_06_80045FBC == 0x10 || 0x1A == D_ovl12_06_80045FBC || D_ovl12_06_80045FBC == 0x1B) {
        func_ovl12_06_800456E8(func_ovl12_06_800439D4, 0, 0, 0, 0x130);
    }
}

void func_ovl12_06_800430D0(void) {
    if (D_ovl12_06_80045FBC == 0x10 || D_ovl12_06_80045FBC == 0x1A || D_ovl12_06_80045FBC == 0x1B) {
        while (D_ovl12_06_80045FC0 != 0) {
            func_ovl12_06_80045730();
        }
        func_ovl12_06_80044D88(0);
    }
    /* empty: without it the function is small enough to be inlined into
     * the handler (ovl_12_02's and 12_04's case 5 keep the same test) */
}

/* Case 11: a static inlined into the handler; its stub is the one at
 * 800430C8.  Its locals give its block of the handler's frame the
 * original's place (frame 0xA0); angle is a vector, as func_8022FF58
 * (ZeroJump 0x1803) writes three floats through it. */
static void func_ovl12_06_800430C8(void) {
    UnkStructA *obj1;
    UnkStructA *obj0;
    Vec3f pos;
    Vec3f target;
    Vec3f angle;
    s32 i;
    s32 pad[2]; /* fake: unused, frame space the original has below i (no source for it found: hoisted dx/dz steps change the code; with obj1 declared after angle, as m2c had it, a word above obj0 is needed too) */

    func_ovl12_06_80044D88(1);
    func_ovl12_06_80045440(0x1F);
    func_ovl12_06_80045D38(0);
    func_ovl12_06_800453D8(2350.0f, 100.0f, 1720.0f);
    func_ovl12_06_800453F8(0.0f, 45.0f, 2000.0f);
    func_ovl12_06_80045708(2);
    obj1 = func_ovl12_06_80044DD8(1);
    obj0 = func_ovl12_06_80044DD8(0);
    func_ovl12_06_800455A0(obj1, 0x0C000000);
    func_ovl12_06_800452C0(obj1->unk40, 0, 0);
    func_ovl12_06_800452C0(obj1->unk40, -1, 1);
    obj1->unk40->unk20 = func_ovl12_06_80043068(obj1->unk40, obj0->unk40);
    if (func_ovl12_06_80044D30(1) == 3) {
        pos.x = obj0->unk40->unk10;
        pos.y = obj0->unk40->unk14;
        pos.z = obj0->unk40->unk18;
        target.x = 2350.0f;
        target.y = 0.0f;
        target.z = 1550.0f;
        func_ovl12_06_80045528(&pos.x, &target.x, &angle.x);
        target.x = func_ovl12_06_80045890(angle.x) * 800.0f + pos.x;
        target.y = pos.y;
        target.z = func_ovl12_06_80045898(angle.x) * 800.0f + pos.z;
        pos.x = obj1->unk40->unk10;
        pos.y = obj1->unk40->unk14;
        pos.z = obj1->unk40->unk18;
        func_ovl12_06_80045528(&pos.x, &target.x, &angle.x);
        func_ovl12_06_80045540(&pos.x, &target.x);
        obj1->unk68 = 51.0f;
        obj1->unk40->unk14 = 0.1f;
        obj1->unk70 = 0.8166667f;
        obj1->unk6C = 16.333334f;
        for (i = 0; i < 90; i++) {
            obj1->unk40->unk10 += (target.x - pos.x) / 90.0f;
            obj1->unk40->unk18 += (target.z - pos.z) / 90.0f;
            if (obj1->unk40->unk14 < 0.0f) {
                obj1->unk70 = 0.0f;
                obj1->unk68 = 0.0f;
                obj1->unk40->unk14 = 0.0f;
            } else {
                func_ovl12_06_80045618(obj1);
            }
            func_ovl12_06_80045730();
        }
        func_ovl12_06_80045C70(0x259);
    }
    obj1->unk40->unk20 = func_ovl12_06_80043068(obj1->unk40, obj0->unk40);
    obj0->unk40->unk20 = func_ovl12_06_80043068(obj0->unk40, obj1->unk40);
    func_ovl12_06_800453D8(obj0->unk40->unk10, 200.0f, obj0->unk40->unk18);
    func_ovl12_06_800453F8(obj1->unk40->unk20, 20.0f, 800.0f);
    for (i = 0; i < 30; i++) {
        obj1->unk40->unk14 += 2.0f;
        func_ovl12_06_80045730();
    }
    func_ovl12_06_80045708(30);
    func_ovl12_06_80045300(0x334, 1);
    func_ovl12_06_80044D88(1);
    func_ovl12_06_80044D38(50);
    func_ovl12_06_80045C88();
    func_ovl12_06_80045708(60);
    obj1->unk40->unkC |= 0x4000;
    func_ovl12_06_800452C0(obj1->unk40, 0x1C, 0);
    func_ovl12_06_800452C0(obj1->unk40, -1, 1);
    obj1->unk40->unk48->unk4[0].unk8 |= 0x20;
    while (!(obj1->unk40->unk48->unk4[0].unk8 & 0x40)) {
        func_ovl12_06_80045730();
    }
    func_ovl12_06_80045C70(0x20F);
    for (i = 0; i < 40; i++) {
        obj1->unk40->unk14 += 25.0f;
        func_ovl12_06_80045730();
    }
    func_ovl12_06_800455A0(obj1, 0x01000000);
    func_ovl12_06_800453D8(obj0->unk40->unk10, 50.0f, obj0->unk40->unk18);
    func_ovl12_06_800453F8(obj0->unk40->unk20, 20.0f, 800.0f);
    func_ovl12_06_80044D88(0);
    func_ovl12_06_80044D38(32);
    func_ovl12_06_80045910();
    D_ovl12_06_80045FC4 = 1;
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl12_06_80043164(s32 arg0) {
    switch (arg0) {
        case 2:
            D_ovl12_06_80045FBC = func_ovl12_06_80044C98();
            func_ovl12_06_80043144();
            break;
        case 16:
        case 26:
        case 27:
            func_ovl12_06_8004314C();
            break;
        case 17:
            func_ovl12_06_80043154();
            break;
        case 4:
            func_ovl12_06_8004315C();
            break;
        case 5:
            func_ovl12_06_800430D0();
            break;
        case 11:
            func_ovl12_06_800430C8();
            break;
        case 1:
            func_ovl12_06_80043010();
            break;
    }
}

/* A static: its callers pass `from` in a3 and `to` in t0.  Defined below
 * the handler, it still comes out here, ahead of func_ovl12_06_800430D0. */
static f32 func_ovl12_06_80043068(UnkStructA_40 *from, UnkStructA_40 *to) {
    Vec3f a;
    Vec3f b;
    Vec3f angle;

    a.x = from->unk10;
    a.y = from->unk14;
    a.z = from->unk18;
    b.x = to->unk10;
    b.y = to->unk14;
    b.z = to->unk18;
    func_ovl12_06_80045528(&a.x, &b.x, &angle.x);
    return angle.x;
}

void func_ovl12_06_80043010(void) {
    if (D_ovl12_06_80045FBC == 0x1A || D_ovl12_06_80045FBC == 0x1B) {
        func_ovl12_06_80045DF0();
    }
    if (D_ovl12_06_80045FC4 == 0) {
        func_ovl12_06_80045910();
    }
}
