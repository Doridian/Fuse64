/*
 * ovl_17_09's game object, IDO -O3; it matches in us, eu and jp.
 *
 * Every function and stub comes out in the original's order, at the
 * original's address. The order is ovl_0F_04's (see its source): the
 * handler (func_ovl17_09_80043290) is compiled first, its direct callees
 * (80043010, the static 80043144) come out first with its inlined
 * statics' stubs, in the reverse of its cases and calls. The six stubs
 * around 80043144 are the handler's statics, named for them: case 3's
 * 8004312C (the wind timer and the fog-less second chain of ovl_0F_04's
 * 80043028) and its 80043124 (the step, ovl_0F_04's 80043028_inl1);
 * case 1's model loop 80043134; case 2's 8004313C (the two zeroed floats
 * and the 80043010 call), 80043288 (the speed by 800450D8) and the empty
 * 80043280 (which static it was is open). The four empty statics
 * 8004454C..80044564 are the pusher's (8004456C) movers, inlined.
 * The address-taken functions came out with 800448A0 after 80044A2C until
 * an unused reference to it in an early-defined static (an `if (0)` call
 * in the stub 80044B90, a fake as in ovl_0F_04's 800460B4). The 22 stubs
 * at the end are empty.
 *
 * as1 orders a block partly by `.loc` lines and a change of file (ugen's
 * `.file`) ends a block, so `#line` into statics.h (ovl_0F_04's fake
 * file name) decides several schedules, as in ovl_0F_04 and ovl_0F_02.
 *
 * Per function:
 * - func_ovl17_09_80043750 (the particles) matches: ovl_0F_04's
 *   8004544C and its statics 80045444 (here 80043748, the stub before it)
 *   and 80045308 (here 80043144), renamed; the only change is that the
 *   update turns by the wind's angle D_ovl17_09_80046298, not view[0].
 * - 8004456C (the pusher) matches: ovl_0F_04's 80043770 with the
 *   direction from D_ovl17_09_8004629C and all eight cases; pos is f32[3];
 *   from `pos[0] = ...` to the end of the switch it is in statics.h
 *   (included in the body), but for `dir = D_ovl17_09_8004629C;` in
 *   2B8D20.c between two parts (else its load is hoisted into the first
 *   block). So 80044828, 800448A0 and
 *   80044A2C, which inline it, match too; 800448A0 has an unused pad[2]
 *   (its frame, 0x60).
 * - 80044198 matches: an unused pad[4] declared before cell (its frame,
 *   0x98, and cell at 0x7C). The nops after its endless loop pad the dead
 *   epilogue to a 32-byte boundary, so they come right once everything
 *   before it has the original's size.
 * - The handler matches: 80043010's call in case 2 is in statics.h
 *   (inside 8004313C), which leaves the jal's slot empty as the original
 *   does, and case 3's final test, the handler's end and everything after
 *   it are in statics.h (the bnez's slot gets the epilogue's lw ra).
 *   Case 2's stores are A0, A4, B4 in that order; the object of 800456E8
 *   goes through a local before D_ovl17_09_8004656C (no address CSE); the
 *   ovl_0F_04 fake `if (D_ovl17_09_800462A4) {}` after the 8004628C store
 *   is kept (the 0.5 product's registers differ without it). The timer
 *   (8004312C) wraps with `D_ovl17_09_800462A0 = 0` and is followed by a
 *   fake `if (D_ovl17_09_800462A0) {}`: the extra reference makes uopt
 *   keep the counter in a0 (`move a0, zero` when it wraps, reloaded only
 *   after the 80046060 call) for case 3's final `== 0` test, as the
 *   original does; without it the test reloads it and case 3's and case
 *   2's registers differ.
 * - func_ovl17_09_80043010 matches as a static: IDO -O3 gives a static
 *   function that isn't inlined only the temporaries t6-t9 and f4-f10
 *   (its eight angles cycle f4, f6, f8, f10, as the original's), where an
 *   external one also takes f16 and f18 (ovl_0A_06's static 800434E4
 *   shows the same).
 *
 * 2026-11-13: 3 `#line`s that turned out redundant were removed (each tried away
 * against the sha1 in us, eu and jp). Where the notes above name a `#line`,
 * region or renumbering that is gone, it is no longer needed.
 */
#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_17_09's game object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse).
 */

/* A display list command (gbi.h's Gfx, as two words) */
typedef struct {
    /* 0x00 */ PORT_UP(u32) w0;
    /* 0x04 */ PORT_UP(u32) w1;
} UnkStruct_ovl17_09_Gfx; /* size 0x8 */

/* gbi.h's _SHIFTL */
#define _SHIFTL(v, s, w) ((u32) (((u32) (v) & ((0x01 << (w)) - 1)) << (s)))

void func_ovl17_09_80044F88(s32);
void func_ovl17_09_80044F90(s32, f32, f32, f32, s32, s32);
void func_ovl17_09_80044FA0(s32);
void func_ovl17_09_80044FD0(s32, s32, s32, s32, s32, s32, s32);
void func_ovl17_09_80044FE0(f32, f32, f32, s32 *, s32);
f32 func_ovl17_09_80045048(UnkStructA_40 *);
void func_ovl17_09_80045068(void (*)(UnkStructA *), s32);
UnkStruct_802AC610 *func_ovl17_09_800450D8(void);
void func_ovl17_09_80045100(s32);
UnkStruct_8023A834 *func_ovl17_09_80045108(void);
void func_ovl17_09_80045138(void);
UnkStructA *func_ovl17_09_800451C8(s32);
UnkStructA *func_ovl17_09_80045208(s32);
UnkStruct_ovl0F_01_80048130 *func_ovl17_09_80045658(s32);
void func_ovl17_09_80045660(s32);
void func_ovl17_09_80045678(UnkStructA_40 *);
s32 func_ovl17_09_800456D0(UnkStructA_40 *, s32, s32, s32, s32, s32);
UnkStructA_40 *func_ovl17_09_800456E8(void (*)(s32, UnkStruct_ovl17_09_Gfx **), s32);
void func_ovl17_09_800457A8(s32);
void func_ovl17_09_800457B8(s32);
void func_ovl17_09_800457C8(f32, f32, f32);
void func_ovl17_09_800457E8(f32, f32, f32);
void func_ovl17_09_800457F0(f32 *, f32 *, f32 *);
void func_ovl17_09_80045830(s32);
void func_ovl17_09_800458D0(Vec3f *);
void func_ovl17_09_800458E0(f32 *);
s32 func_ovl17_09_80045938(UnkStructA *, s32);
void func_ovl17_09_800459A0(UnkStructA *);
UnkStructA *func_ovl17_09_800459A8(s32, s32, s32, s32, PORT_IP(s32));
void func_ovl17_09_80045A68(f32);
void func_ovl17_09_80045A70(s32);
PORT_IP(s32) func_ovl17_09_80045AA8(UnkStructA *, s32, UnkStructA *);
void func_ovl17_09_80045AD8(void (*)(void), void *, PORT_IP(s32), s32, s32);
void func_ovl17_09_80045AF8(s32);
void func_ovl17_09_80045B20(void);
f32 func_ovl17_09_80045C80(f32);
f32 func_ovl17_09_80045C88(f32);
f64 func_ovl17_09_80045C90(void);
s32 func_ovl17_09_80045C98(s32);
void func_ovl17_09_80045D50(s32);
s32 func_ovl17_09_80045D60(s32);
void func_ovl17_09_80045DD8(s32, s32, s32, f32);
void func_ovl17_09_80045E28(s32, s32, s32);
void func_ovl17_09_80045E40(u32 (*)[3]);
void func_ovl17_09_80045F28(s32, f32, f32, f32, PORT_IP(s32));
s32 func_ovl17_09_80045F80(UnkStructA *);
s32 func_ovl17_09_80045F88(UnkStructA *);
PORT_IP(s32) func_ovl17_09_80045F90(s16);
void func_ovl17_09_80045F98(s16, s32);
void func_ovl17_09_80045FD8(UnkStructA *, s32, f32);
void func_ovl17_09_80046060(s32);
s32 func_ovl17_09_80046080(s32, f32, f32, f32);

static void func_ovl17_09_80043010(void);
static void func_ovl17_09_80043144(UnkStruct_ovl0F_01_80048798 *);
static void func_ovl17_09_80043748(UnkStruct_ovl17_09_Gfx **);
static void func_ovl17_09_8004412C(UnkStruct_ovl0C_01_8004A3BC *);
static s32 func_ovl17_09_80044504(UnkStructA *);
static void func_ovl17_09_8004456C(UnkStructA *, f32);
void func_ovl17_09_80043750(s32, UnkStruct_ovl17_09_Gfx **);
void func_ovl17_09_80044198(void);
void func_ovl17_09_80044828(UnkStructA *);
void func_ovl17_09_800448A0(UnkStructA *);
void func_ovl17_09_80044A2C(UnkStructA *);

/* .bss */
u8 D_ovl17_09_80046390[0x24];
s32 D_ovl17_09_800463B4;
s32 D_ovl17_09_800463B8;
Vec3f D_ovl17_09_800463C0;
u8 D_ovl17_09_800463CC[0x4];
UnkStruct_ovl0F_01_80048798 D_ovl17_09_800463D0[20];
Vec3f D_ovl17_09_80046560;
UnkStructA_40 *D_ovl17_09_8004656C;
f32 D_ovl17_09_80046570;
f32 D_ovl17_09_80046574;
u8 D_ovl17_09_80046578[0x8];

/* .data */
UnkStruct_ovl0F_01_80048130 *D_ovl17_09_80046280 = NULL;
f32 D_ovl17_09_80046284 = 0.0f;
f32 D_ovl17_09_80046288 = 0.0f;
f32 D_ovl17_09_8004628C = 0.0f;
f32 D_ovl17_09_80046290 = 0.0f;
f32 D_ovl17_09_80046294 = 0.0f;
f32 D_ovl17_09_80046298 = 0.0f;
s32 D_ovl17_09_8004629C = 0x40;
s32 D_ovl17_09_800462A0 = 0;
f32 D_ovl17_09_800462A4 = 0.0f;
f32 D_ovl17_09_800462A8 = 0.0f;
f32 D_ovl17_09_800462AC = 0.0f;
f32 D_ovl17_09_800462B0 = 0.0f;
s32 D_ovl17_09_800462B4 = 1;
/* passed to func_ovl17_09_80045E40: three 12-byte records */
u32 D_ovl17_09_800462B8[][3] = {
    { 0xDA0000, 0x100018, 4 },
    { 0, 0, 4 },
    { 0, 0, 4 },
};
UnkStruct_ovl0C_01_8004A3BC D_ovl17_09_800462DC[2] = {
    { NULL, 420.0f, 2000.0f, 1800.0f },
    { NULL, 4200.0f, 2000.0f, 1800.0f },
};

/* unused: the stubs IDO -O3 leaves at the end of the object */
static void func_ovl17_09_80044B88(void) {
}

static void func_ovl17_09_80044B90(void) {
    /* fake: 800448A0 comes out after 80044828 (the header) */
    if (0) {
        func_ovl17_09_80045068((void (*)(UnkStructA *)) func_ovl17_09_800448A0, 0);
        /* and the .bss nothing uses keeps its place (unreferenced, -O3
         * drops it) */
        D_ovl17_09_80046390[0] = D_ovl17_09_800463CC[0] =
            D_ovl17_09_80046578[0] = 0;
    }
}

static void func_ovl17_09_80044B98(void) {
}

static void func_ovl17_09_80044BA0(void) {
}

static void func_ovl17_09_80044BA8(void) {
}

static void func_ovl17_09_80044BB0(void) {
}

static void func_ovl17_09_80044BB8(void) {
}

static void func_ovl17_09_80044BC0(void) {
}

static void func_ovl17_09_80044BC8(void) {
}

static void func_ovl17_09_80044BD0(void) {
}

static void func_ovl17_09_80044BD8(void) {
}

static void func_ovl17_09_80044BE0(void) {
}

static void func_ovl17_09_80044BE8(void) {
}

static void func_ovl17_09_80044BF0(void) {
}

static void func_ovl17_09_80044BF8(void) {
}

static void func_ovl17_09_80044C00(void) {
}

static void func_ovl17_09_80044C08(void) {
}

static void func_ovl17_09_80044C10(void) {
}

static void func_ovl17_09_80044C18(void) {
}

static void func_ovl17_09_80044C20(void) {
}

static void func_ovl17_09_80044C28(void) {
}

static void func_ovl17_09_80044C30(void) {
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl17_09_80044B70
#define CORE_FUNC_B func_ovl17_09_80044B14
#define CORE_FUNC_C func_ovl17_09_80044ACC
#define CORE_FUNC_D func_ovl17_09_80044A98
#define CORE_BSS_A0 D_ovl17_09_800463B4
#define CORE_BSS_A1 D_ovl17_09_800463B8
#define CORE_BSS_B D_ovl17_09_800463C0
#define CORE_ZJ_1704 func_ovl17_09_800457C8
#define CORE_ZJ_1708 func_ovl17_09_800457E8
#define CORE_ZJ_1709 func_ovl17_09_800457F0
#define CORE_ZJ_1904 func_ovl17_09_80045AF8
#define CORE_ZJ_2B04 func_ovl17_09_80046080
#include "overlays/common/core.h"











static s32 func_ovl17_09_80044504(UnkStructA *obj) {
    s32 idx;
    s32 i;

    idx = -1;
    for (i = 0; i < 4; i++) {
        if (func_ovl17_09_800451C8(i) == obj) {
            idx = i;
            break;
        }
    }
    return idx;
}

static void func_ovl17_09_8004454C(UnkStructA *obj, f32 speed) {
    obj->unk40->unk10 -= speed;
}

static void func_ovl17_09_8004455C(UnkStructA *obj, f32 speed) {
    obj->unk40->unk18 += speed;
}

static void func_ovl17_09_80044564(UnkStructA *obj, f32 speed) {
    obj->unk40->unk10 += speed;
}

static void func_ovl17_09_80044554(UnkStructA *obj, f32 speed) {
    obj->unk40->unk18 -= speed;
}

static void func_ovl17_09_8004456C(UnkStructA *obj, f32 speed) {
    f32 pos[3];
    s32 dir;
    s32 saved;
    s32 hit;
    UnkStructA *other;
    s32 k;

#define STATICS_PART 3
#include "overlays/ovl_17_09/statics.h"
#undef STATICS_PART
    dir = D_ovl17_09_8004629C;
#define STATICS_PART 1
#include "overlays/ovl_17_09/statics.h"
#undef STATICS_PART
    saved = obj->unkC;
    hit = FALSE;
    other = NULL;
    obj->unkC = 0;
    if (obj->unk8 & 1) {
        other = func_ovl17_09_80045208(func_ovl17_09_80044504(obj));
    } else if (obj->unk8 & 2) {
        k = func_ovl17_09_80045F80(obj);
        if (func_ovl17_09_80045208(k) == obj) {
            other = func_ovl17_09_800451C8(k);
        }
    }
    if (other != NULL) {
        if (func_ovl17_09_80045AA8(obj, 0x10, other) != 0) {
            hit = TRUE;
        }
    } else {
        if (func_ovl17_09_80045938(obj, 0x10) != 0) {
            hit = TRUE;
        }
    }
    obj->unkC = saved;
    if (hit) {
        obj->unk40->unk10 = pos[0];
        obj->unk40->unk14 = pos[1];
        obj->unk40->unk18 = pos[2];
    }
}

void func_ovl17_09_80044A2C(UnkStructA *obj) {
    func_ovl17_09_8004456C(obj, D_ovl17_09_800462A4);
}

void func_ovl17_09_800448A0(UnkStructA *obj) {
    s32 idx;
    s16 k;

    func_ovl17_09_80044828(obj);
    if (obj->unk40->unk10 <= D_ovl17_09_80046570 || D_ovl17_09_80046574 <= obj->unk40->unk10 ||
        obj->unk40->unk14 < -10.0f) {
        func_ovl17_09_80045F28(0x10, obj->unk40->unk10, obj->unk40->unk14 + 150.0f, obj->unk40->unk18, 0);
        if (obj->unk8 & 1) {
            idx = func_ovl17_09_80044504(obj);
            if (idx != -1) {
                k = idx;
                if (func_ovl17_09_80045F90(k) != 0) {
                    func_ovl17_09_80045F98(k, 0);
                }
            }
        } else {
            idx = func_ovl17_09_80045F88(obj);
            if (idx != -1) {
                func_ovl17_09_80045F98(idx, 0);
            }
        }
        obj->unk40->unk14 = -700.0f;
        obj->unk5C = -700.0f;
    }
}

void func_ovl17_09_80044828(UnkStructA *obj) {
    func_ovl17_09_8004456C(obj, D_ovl17_09_800462A4 * 0.42857143f);
}

static void func_ovl17_09_8004412C(UnkStruct_ovl0C_01_8004A3BC *rec) {
    rec->unk0 = func_ovl17_09_800459A8(0x269, -1, 0x01000000, 0xD0, 0);
    rec->unk0->unk40->unk10 = rec->unk4;
    rec->unk0->unk40->unk14 = rec->unk8;
    rec->unk0->unk40->unk18 = rec->unkC;
}

void func_ovl17_09_80044198(void) {
    UnkStruct_8023A834 *b;
    s32 i;
    s32 pad[4]; /* fake: unused, frame space the original has (no source for it found) */
    s32 cell;
    s32 last0;
    s32 last1;

    b = func_ovl17_09_80045108();
    D_ovl17_09_800462DC[0].unk0 = NULL;
    D_ovl17_09_800462DC[1].unk0 = NULL;
    last0 = -1;
    last1 = -1;
    while (b->unk2C > 0) {
        func_ovl17_09_80045AF8(5);
    }
    D_ovl17_09_80046570 = 1410.0f;
    D_ovl17_09_80046574 = 3190.0f;
    func_ovl17_09_80045068(func_ovl17_09_800448A0, 6);
    func_ovl17_09_8004412C(&D_ovl17_09_800462DC[0]);
    func_ovl17_09_8004412C(&D_ovl17_09_800462DC[1]);
    func_ovl17_09_800456D0(D_ovl17_09_800462DC[0].unk0->unk40, 0, 0, 0x400, 0x20, 0x400);
    func_ovl17_09_800456D0(D_ovl17_09_800462DC[1].unk0->unk40, 0, 0, 0x400, 0x20, 0x400);
    D_ovl17_09_800462DC[0].unk0->unk40->unk30 = 6.0f;
    D_ovl17_09_800462DC[0].unk0->unk40->unk24 = -55.0f;
    D_ovl17_09_800462DC[1].unk0->unk40->unk30 = 6.0f;
    D_ovl17_09_800462DC[1].unk0->unk40->unk24 = 55.0f;
    D_ovl17_09_800462DC[1].unk0->unk40->unk20 = 180.0f;
    D_ovl17_09_800462DC[0].unk0->unk40->unk28 = 20.0f;
    D_ovl17_09_800462DC[1].unk0->unk40->unk28 = 20.0f;
    for (i = 0; i < 3600; i++) {
        D_ovl17_09_800462DC[0].unk0->unk40->unk10 += 0.21666667f;
        D_ovl17_09_800462DC[1].unk0->unk40->unk10 -= 0.21666667f;
        D_ovl17_09_80046570 += 0.23f;
        D_ovl17_09_80046574 -= 0.23f;
        func_ovl17_09_80044FE0(D_ovl17_09_80046570 - 100.0f, 0.0f, 1500.0f, &cell, 2);
        if (cell != last0) {
            last0 = cell;
            func_ovl17_09_80044FD0(cell, 0, 9, cell, 5, 0x15, 0x1800);
        }
        func_ovl17_09_80044FE0(D_ovl17_09_80046574 + 100.0f, 0.0f, 1500.0f, &cell, 2);
        if (cell != last1) {
            last1 = cell;
            func_ovl17_09_80044FD0(cell, 0, 9, cell, 5, 0x15, 0x1800);
        }
        func_ovl17_09_80045B20();
    }
    while (TRUE) {
        func_ovl17_09_80045AF8(100);
    }
}

static void func_ovl17_09_80043748(UnkStruct_ovl17_09_Gfx **gdl) {
    UnkStruct_ovl17_09_Gfx *g2;
    f32 oldx;
    f32 oldz;
    f32 s;
    s32 i;
    f32 view[3];

    g2 = *gdl;
    oldx = D_ovl17_09_80046560.x;
    oldz = D_ovl17_09_80046560.z;
    func_ovl17_09_800458D0(&D_ovl17_09_80046560);
    func_ovl17_09_800458E0(view);
    s = func_ovl17_09_80045C88(view[0]);
    D_ovl17_09_80046284 = ((D_ovl17_09_80046560.x - oldx) * s - func_ovl17_09_80045C80(view[0]) * (D_ovl17_09_80046560.z - oldz)) * 0.5f;
    s = func_ovl17_09_80045C88(view[0]);
    D_ovl17_09_80046288 = (func_ovl17_09_80045C80(view[0]) * (D_ovl17_09_80046560.x - oldx) + (D_ovl17_09_80046560.z - oldz) * s) * 0.2f;
    D_ovl17_09_80046284 *= 1500.0f / view[2];
    D_ovl17_09_80046288 *= 1500.0f / view[2];
    for (i = 0; i < 20; i++) {
        if (func_ovl17_09_80045D60(0x407) != 0) {
            UnkStruct_ovl0F_01_80048798 *p = &D_ovl17_09_800463D0[i];

            { UnkStruct_ovl17_09_Gfx *g = g2++; g->w0 = _SHIFTL(0xE4, 24, 8) | _SHIFTL(PORT_F2U(p->unk8 + p->unk4 - 1) << 2, 0, 12) | _SHIFTL(PORT_F2U(p->unk0 + p->unk8 - 1) << 2, 12, 12); g->w1 = _SHIFTL(0, 24, 3) | _SHIFTL(PORT_F2U(p->unk4) << 2, 0, 12) | _SHIFTL(PORT_F2U(p->unk0) << 2, 12, 12); { UnkStruct_ovl17_09_Gfx *g = g2++; g->w0 = 0xB4000000; g->w1 = 0; } { UnkStruct_ovl17_09_Gfx *g = g2++; g->w0 = 0xB3000000; g->w1 = _SHIFTL((s32) (16.0f / (p->unk8 - 1) * 1024.0f), 0, 16) | _SHIFTL((s32) (16.0f / (p->unk8 - 1) * 1024.0f), 16, 16); } }
        }
        if (D_ovl17_09_80046294 == 0.0f) {
            continue;
        }
        {
            f32 r;
            f32 dx;
            f32 dy;
            f64 drift;

            s = func_ovl17_09_80045C88(D_ovl17_09_80046298);
            r = func_ovl17_09_80045C90();
            dx = -D_ovl17_09_80046284;
            D_ovl17_09_800463D0[i].unk0 += dx - r * (dx * 0.25f) + D_ovl17_09_8004628C * s;
            s = (D_ovl17_09_8004628C != 0.0f ? 1.0f : 0);
            if (func_ovl17_09_80045C80(D_ovl17_09_80046298) * (s * D_ovl17_09_8004628C) + D_ovl17_09_80046290 < 0.0f) {
                f32 sign = (D_ovl17_09_8004628C != 0.0f ? 1.0f : 0);

                drift = (func_ovl17_09_80045C80(D_ovl17_09_80046298) * (sign * D_ovl17_09_8004628C) + D_ovl17_09_80046290) * 0.5;
            } else {
                f32 sign = (D_ovl17_09_8004628C != 0.0f ? 1.0f : 0);

                drift = func_ovl17_09_80045C80(D_ovl17_09_80046298) * (sign * D_ovl17_09_8004628C) + D_ovl17_09_80046290;
            }
            r = func_ovl17_09_80045C90();
            dy = D_ovl17_09_800463D0[i].unkC - D_ovl17_09_80046288;
            D_ovl17_09_800463D0[i].unk4 += dy - r * (dy * 0.6f) + drift;
            D_ovl17_09_800463D0[i].unk8 -= 0.1f * (D_ovl17_09_800463D0[i].unk8 * 0.0625f);
            if (340.0f <= D_ovl17_09_800463D0[i].unk0 || 240.0f <= D_ovl17_09_800463D0[i].unk4 || D_ovl17_09_800463D0[i].unk8 <= 3.0f ||
                D_ovl17_09_800463D0[i].unk10++ >= 240 || D_ovl17_09_800463D0[i].unk0 < -20.0f || D_ovl17_09_800463D0[i].unk4 < -10.0f) {
                func_ovl17_09_80043144(&D_ovl17_09_800463D0[i]);
            }
        }
    }
    *gdl = g2;
}

void func_ovl17_09_80043750(s32 arg0, UnkStruct_ovl17_09_Gfx **arg1) {
    UnkStruct_ovl17_09_Gfx *gfx;
    UnkStruct_ovl0F_01_80048130 *t;

    gfx = *arg1;
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xB900031D; g->w1 = 0x00553048; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xFCFFFFFF; g->w1 = 0xFFFCF279; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xBB000001; g->w1 = 0x80008000; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xBA001301; g->w1 = 0; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xBA000E02; g->w1 = 0x8000; }
    t = D_ovl17_09_80046280;
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xFD100000; g->w1 = t->unk20 + (PORT_UP(u32)) t; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xE8000000; g->w1 = 0; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xF5000100; g->w1 = 0x07000000; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xE6000000; g->w1 = 0; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xF0000000; g->w1 = 0x0703C000; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xE7000000; g->w1 = 0; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xFD500000; g->w1 = t->unk14 + (PORT_UP(u32)) t; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xF5500000; g->w1 = 0x07000000; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xE6000000; g->w1 = 0; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xF3000000; g->w1 = 0x0703F800; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xE7000000; g->w1 = 0; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xF5400200; g->w1 = 0; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xF2000000; g->w1 = 0x0003C03C; }
    func_ovl17_09_80043748(&gfx);
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xBA001301; g->w1 = 0x00080000; }
    { UnkStruct_ovl17_09_Gfx *g = gfx++; g->w0 = 0xBA000E02; g->w1 = 0; }
    *arg1 = gfx;
}

static f32 func_ovl17_09_80043124(s32 t) {
    f32 d;

    d = 0.0f;
    if (t >= 160) {
        if (t < 460) {
            d = D_ovl17_09_800462A8 / 300.0f;
        } else if (t >= 540 && t < 720) {
            d = -(D_ovl17_09_800462A8 / 180.0f);
        }
    }
    return d;
}

static void func_ovl17_09_8004312C(void) {
    s32 t;
    f32 d;

    t = 0;
    D_ovl17_09_800462A0++;
    if (D_ovl17_09_800462A0 >= 720) {
        D_ovl17_09_800462A0 = 0;
    } else {
        t = D_ovl17_09_800462A0;
    }
    if (D_ovl17_09_800462A0) { /* fake: a dead test, for the register allocation or schedule */
    }
    d = func_ovl17_09_80043124(t);
    D_ovl17_09_800462A4 += d;
    if (2.0f <= D_ovl17_09_800462A4 && D_ovl17_09_800462B4 == 1) {
        D_ovl17_09_800462B4 = 0;
        func_ovl17_09_80046060(0x89);
    } else if (D_ovl17_09_800462A4 < 2.0f) {
        D_ovl17_09_800462B4 = 1;
    }
    d = D_ovl17_09_800462A4 * 0.5f;
    D_ovl17_09_8004628C = d;
    if (D_ovl17_09_800462A4) { /* fake: a dead test, for the register allocation or schedule */
    }
    D_ovl17_09_80046290 = 0.0f;
}

static void func_ovl17_09_8004313C(void) {
    D_ovl17_09_800462AC = 0.0f;
    D_ovl17_09_800462B0 = 0.0f;
#define STATICS_PART 2
#include "overlays/ovl_17_09/statics.h"
#undef STATICS_PART
