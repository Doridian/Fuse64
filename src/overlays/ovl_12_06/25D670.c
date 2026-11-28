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

/* What func_ovl12_06_80045D30 returns (D_ovl12_06_80045FC8). */
typedef struct {
    /* 0x00 */ PORT_RET(u8 unk0[0x90];, u8 unk0[0x18]; void *unk18; void *unk1C; u8 unk20[0x70];) /* (6BBE0.c's D_800BCFE8[i], Port_800BCFE8: pointers at 0x18, 0x1C, 0x98, 0x9C) */
    /* 0x90 */ s32 unk90;
    /* 0x94 */ PORT_RET(u8 unk94[0xE];, u8 unk94[4]; void *unk98; void *unk9C;)
    /* 0xA2 */ PORT_RET(u16 unkA2;, PORT_HALVES(u16 unkA0, u16 unkA2)) /* (the task id's low half) */
} UnkStruct_ovl12_06_80045FC8;

void func_ovl12_06_80044B98(s32);
void func_ovl12_06_80044BB0(s32);
s32 func_ovl12_06_80044C98(void);
void func_ovl12_06_80044D20(s32, s32);
s32 func_ovl12_06_80044D30(s32);
void func_ovl12_06_80044D38(s32);
void func_ovl12_06_80044D48(void);
void func_ovl12_06_80044D50(s32, s32, s32);
void func_ovl12_06_80044D88(s32);
void func_ovl12_06_80044D90(s32);
void func_ovl12_06_80044DA8(void);
UnkStructA *func_ovl12_06_80044DD8(s32);
void func_ovl12_06_80044E08(s32);
void func_ovl12_06_80044E28(s32, s32);
UnkStructA_40 *func_ovl12_06_80045278(s32, s32);
void func_ovl12_06_80045280(UnkStructA_40 *);
void func_ovl12_06_800452C0(UnkStructA_40 *, s32, s32);
f32 func_ovl12_06_800452D8(UnkStructA_40 *, s32);
void func_ovl12_06_80045300(s32, s32);
void func_ovl12_06_800453D8(f32, f32, f32);
void func_ovl12_06_800453F8(f32, f32, f32);
void func_ovl12_06_80045400(f32 *, f32 *, f32 *);
void func_ovl12_06_80045438(s32);
void func_ovl12_06_80045440(s32);
void func_ovl12_06_80045528(f32 *, f32 *, f32 *);
void func_ovl12_06_80045540(f32 *, f32 *);
void func_ovl12_06_800455A0(UnkStructA *, s32);
void func_ovl12_06_800455B0(UnkStructA *);
UnkStructA *func_ovl12_06_800455C0(s32, s32, s32, PORT_IP(s32));
void func_ovl12_06_80045618(UnkStructA *);
s32 func_ovl12_06_800456E8(void (*)(void), s32, PORT_IP(s32), s32, s32);
void func_ovl12_06_800456F8(void);
void func_ovl12_06_80045700(u16, s32);
s32 func_ovl12_06_80045708(s32);
void func_ovl12_06_80045730(void);
void func_ovl12_06_80045748(u16);
void func_ovl12_06_80045758(s32, s32, s32);
void func_ovl12_06_80045760(s32, s32);
UnkStruct_ovl12_06_80045FCC *func_ovl12_06_80045770(s32);
void func_ovl12_06_80045780(UnkStruct_ovl12_06_80045FCC *, s32);
f32 func_ovl12_06_80045890(f32);
f32 func_ovl12_06_80045898(f32);
void func_ovl12_06_80045908(void);
void func_ovl12_06_80045910(void);
void func_ovl12_06_80045B38(s32, f32, f32, f32, PORT_IP(s32));
void func_ovl12_06_80045C70(s32);
void func_ovl12_06_80045C80(s32);
void func_ovl12_06_80045C88(void);
s32 func_ovl12_06_80045C90(s32, f32, f32, f32);
void func_ovl12_06_80045D18(s32, s32, s32, s32, s32);
UnkStruct_ovl12_06_80045FC8 *func_ovl12_06_80045D30(s32);
void func_ovl12_06_80045D38(s32);
void func_ovl12_06_80045DE8(void);
void func_ovl12_06_80045DF0(void);

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
UnkStruct_ovl12_06_80045FC8 *D_ovl12_06_80045FC8;
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
#define CORE_ZJ_1704 func_ovl12_06_800453D8
#define CORE_ZJ_1708 func_ovl12_06_800453F8
#define CORE_ZJ_1709 func_ovl12_06_80045400
#define CORE_ZJ_1904 func_ovl12_06_80045708
#define CORE_ZJ_2B04 func_ovl12_06_80045C90
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
    func_ovl12_06_80045748(D_ovl12_06_80045FC8->unkA2);
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
    func_ovl12_06_80045700(D_ovl12_06_80045FC8->unkA2, 0);
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
