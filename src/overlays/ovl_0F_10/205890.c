/*
 * ovl_0F_10's game object, IDO -O3; it matches in us, eu and jp.
 *
 * Measured in jp. What went right, from 826 words (ovl_0F_07/0F_08/0F_09's
 * fixes where the code is shared):
 * - func_ovl0F_10_80043430 (430 to 2): case 2's 80043408 (the 3200 store
 *   before 80046190's call) is defined in "panels.c": the `.file` keeps
 *   the store out of the `jal`'s delay slot (the missing word). The rest
 *   is the frame (as ovl_0F_01's model: locals from the top, then each
 *   inlined static's area in call order, at least 8 bytes): the table loop
 *   is the handler's own (i, y, count, p are its locals), its stub
 *   80043428 is an empty static called first, 80043420 (the 0x10 check)
 *   and 80043418 (the model) are separate statics, and case 15's code is
 *   the handler's with the empty static 80043080 called first (as
 *   ovl_0F_03's case 15: as a static of its own it takes 8 bytes below
 *   case 0x52's slots).
 * - func_ovl0F_10_80044074 (252 to 0): the speed bits are their own local
 *   `bits` (a1 in the original), read with the flags straight from unk64;
 *   the four `speed += ...; if (25 < speed) speed = 25` are the static
 *   8004405C(&speed, d) (the product in f0); 80044054 takes &speed (`lw
 *   a2` in each case); the hit test 80043F9C is called from 80043FA4,
 *   which puts pos back if it hit and returns the hit (the original's
 *   one load of it in v1, tested twice); `D_ovl0F_10_800466D4 = 10;` is on
 *   its call's line (the store after the stack argument's). pad[12] is
 *   the frame's unused rest (a guess).
 * - func_ovl0F_10_80044B8C (97 to 0): the inlined 80044B84 is defined in
 *   "panels.c" (ovl_0F_07's 80044200).
 * - func_ovl0F_10_80043CCC (29 to 0): the flag test is a `while`, not
 *   an `if` (the body clears the flag, so it runs once): as1 then leaves
 *   `move s0,zero` in the `beqz`'s delay slot, where with `if` it hoists
 *   it into the load delay (hand-edited ugen output matches when the
 *   load sits in the preheader and at the loop's end, so the `beqz`
 *   starts a block with two predecessors, which the `while`'s rotation
 *   likely gives). The bound is the constant 90 (no variable n).
 * - func_ovl0F_10_80043430 (2 to 0): i is case 2's counter at sp+0xD4
 *   below count and p because case 15's object is a local declared first
 *   (obj, y, count, p, i), and the frame keeps its size because the empty
 *   80043428 is defined without a prototype (`static void f() {}`):
 *   inlined, a K&R-style empty static takes no frame, where `f(void)`
 *   takes 8 bytes.
 * The unused static 80044FF8 (its body is dropped) refers to the .bss
 * nothing else uses (80046B18, 80046B24, 80046B38), which would otherwise
 * be common.
 * The statics once in a "panels.c" `#line` region are statics.h, included
 * where the region was (p3: the same .file switches, and the definition
 * order of the statics that aren't inlined is kept).
 *
 * 2026-11-13: 2 `#line`s that turned out redundant were removed (each tried away
 * against the sha1 in us, eu and jp). Where the notes above name a `#line`,
 * region or renumbering that is gone, it is no longer needed.
 */
#include "common.h"

/*
 * ovl_0F_10's game object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse). ovl_0F_07 (1FCAC0) and
 * ovl_0F_08 (1FF8F0) share most of its helpers. The statics IDO inlines
 * everywhere are named for the empty stubs they leave.
 */

/* a model the stage places: four in D_ovl0F_10_8004672C, two in
 * D_ovl0F_10_800467DC */
typedef struct {
    /* 0x00 */ UnkStructA **unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ f32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
} UnkStruct_ovl0F_10_8004672C; /* size 0x2C */

/* seven arguments of func_ovl0F_10_800455B8 */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} UnkStruct_ovl0F_10_800468C0; /* size 0x1C */

/* the task func_ovl0F_10_80044B8C runs as: its argument at 0x2C */
typedef struct {
    /* 0x00 */ u8 unk0[0x2C];
    /* 0x2C */ UnkStructA **unk2C;
} UnkStruct_ovl0F_10_80045F30;

void func_ovl0F_10_800453B8(s32);
void func_ovl0F_10_800453D0(s32);
void func_ovl0F_10_80045418(f32, f32, f32, f32 *, s32);
void func_ovl0F_10_80045440(UnkStructA_40 *, f32 *, f32 *);
void func_ovl0F_10_80045498(void (*)(UnkStructA *), s32);
void func_ovl0F_10_800454A0(void (*)(UnkStructA *));
void func_ovl0F_10_800454B0(s32, s32, s32);
s32 func_ovl0F_10_800454B8(void);
s32 *func_ovl0F_10_80045508(void);
void func_ovl0F_10_80045520(s32);
void func_ovl0F_10_80045528(s32);
void func_ovl0F_10_80045568(void);
void func_ovl0F_10_80045570(s32, s32, s32);
void func_ovl0F_10_80045578(Vec3f *);
s32 func_ovl0F_10_80045580(s32, s32, s32, s32);
s32 func_ovl0F_10_80045588(s32, s32, s32, s32, s32);
s32 func_ovl0F_10_80045598(void);
void func_ovl0F_10_800455A0(s32, void *);
void func_ovl0F_10_800455A8(s32);
void func_ovl0F_10_800455B0(s32);
void func_ovl0F_10_800455B8(s32, s32, f32, s32, s32, s32, s32);
void func_ovl0F_10_800455C0(char *);
void func_ovl0F_10_800455C8(void);
void func_ovl0F_10_800455D0(s32);
void func_ovl0F_10_800455E0(s32);
UnkStructA *func_ovl0F_10_800455F8(s32);
void func_ovl0F_10_80045600(s32);
UnkStructA *func_ovl0F_10_80045638(s32);
void func_ovl0F_10_80045648(s32, s32);
void func_ovl0F_10_80045658(s32);
void func_ovl0F_10_80045AB0(UnkStructA_40 *, s32);
void func_ovl0F_10_80045B20(s32, s32);
void func_ovl0F_10_80045BF8(f32, f32, f32);
void func_ovl0F_10_80045C18(f32, f32, f32);
void func_ovl0F_10_80045C20(f32 *, f32 *, f32 *);
void func_ovl0F_10_80045C58(s32);
void func_ovl0F_10_80045C60(s32);
void func_ovl0F_10_80045C80(f32);
void func_ovl0F_10_80045C88(f32);
void func_ovl0F_10_80045CA0(f32);
void func_ovl0F_10_80045CB0(f32, f32);
s32 func_ovl0F_10_80045D68(UnkStructA *, s32);
void func_ovl0F_10_80045DB8(UnkStructA *, s32);
void func_ovl0F_10_80045DC0(UnkStructA *, s32);
s32 func_ovl0F_10_80045DD0(UnkStructA *);
UnkStructA *func_ovl0F_10_80045DE0(s32, s32, s32, s32);
UnkStructA *func_ovl0F_10_80045E28(s32, s32);
void func_ovl0F_10_80045E38(UnkStructA *);
void func_ovl0F_10_80045E98(f32);
void func_ovl0F_10_80045EA0(s32);
s32 func_ovl0F_10_80045ED8(UnkStructA *, s32, UnkStructA *);
s32 func_ovl0F_10_80045F08(void (*)(void), void *, s32, s32, s32);
void func_ovl0F_10_80045F10(s32);
void func_ovl0F_10_80045F18(void);
void func_ovl0F_10_80045F28(s32);
void *func_ovl0F_10_80045F30(void);
void func_ovl0F_10_80045F50(void);
UnkStructC *func_ovl0F_10_80045F90(s32);
void func_ovl0F_10_80045FA0(UnkStructC *, s32);
s32 func_ovl0F_10_80046170(s32);
void func_ovl0F_10_80046180(s32);
s32 func_ovl0F_10_80046190(s32);
void func_ovl0F_10_80046270(u32 (*)[3]);
void func_ovl0F_10_80046358(s32, f32, f32, f32, s32);
void func_ovl0F_10_80046368(s32);
void func_ovl0F_10_80046398(void);
void func_ovl0F_10_80046408(UnkStructA *, s32, f32);
void func_ovl0F_10_80046418(s32, s32, s32);
void func_ovl0F_10_80046438(s32);
void func_ovl0F_10_80046478(void);
void func_ovl0F_10_80046490(s32);
void func_ovl0F_10_800464B0(s32, f32, f32, f32);
void func_ovl0F_10_80046608(void);
void func_ovl0F_10_80046610(void);

void func_ovl0F_10_80043344(f32);
void func_ovl0F_10_80043430(s32);
void func_ovl0F_10_80043CCC(void);
void func_ovl0F_10_80043E08(void);
void func_ovl0F_10_80044074(UnkStructA *);
void func_ovl0F_10_80044780(UnkStructA *);
void func_ovl0F_10_800448A0(void);
void func_ovl0F_10_80044AD4(void);
void func_ovl0F_10_80044B8C(void);
void func_ovl0F_10_80044D3C(void);
void func_ovl0F_10_80044F40(s32, UnkStructA *);
void func_ovl0F_10_80044F74(f32, f32, f32);
void func_ovl0F_10_80044FD0(void);
void func_ovl0F_10_80044FE8(void);

static s32 func_ovl0F_10_80043010(s32);
static void func_ovl0F_10_80043088(UnkStruct_ovl0F_10_8004672C *);
static void func_ovl0F_10_8004310C(UnkStruct_ovl0F_10_8004672C *, s32);
static void func_ovl0F_10_800431BC(UnkStruct_ovl0F_10_8004672C *, s32);
static void func_ovl0F_10_800432E0(s32 *);
static void func_ovl0F_10_8004338C(s32);
static void func_ovl0F_10_80043FAC(f32, f32 *, s32 *);
static void func_ovl0F_10_8004406C(UnkStructA *);
static void func_ovl0F_10_80044064(UnkStructA *, f32 *);
static void func_ovl0F_10_8004405C(f32 *, f32);
static void func_ovl0F_10_80044054(UnkStructA *, s32, f32 *);
static s32 func_ovl0F_10_80043FA4(UnkStructA *, f32 *);
static s32 func_ovl0F_10_80043F9C(UnkStructA *);
static void func_ovl0F_10_80043F94(s32 *, s32, f32);
static void func_ovl0F_10_80044B7C(UnkStruct_ovl0F_10_80045F30 *, UnkStructA *);
static void func_ovl0F_10_80044B84(void);

/* .bss */
UnkStructA *D_ovl0F_10_80046AE0[6];
f32 D_ovl0F_10_80046AF8[3];
s32 D_ovl0F_10_80046B04;
Vec3f D_ovl0F_10_80046B08;
s32 D_ovl0F_10_80046B14;
u8 D_ovl0F_10_80046B18[0x4];
s32 D_ovl0F_10_80046B1C;
s32 D_ovl0F_10_80046B20;
u8 D_ovl0F_10_80046B24[0x4];
Vec3f D_ovl0F_10_80046B28;
UnkStructC *D_ovl0F_10_80046B34;
u8 D_ovl0F_10_80046B38[0x8];

/* .data */
s32 D_ovl0F_10_800466B0 = 0;
s32 D_ovl0F_10_800466B4 = 0;
s32 D_ovl0F_10_800466B8 = 0;
s32 D_ovl0F_10_800466BC = -1;
u8 D_ovl0F_10_800466C0[4] = { 0 };
extern UnkStruct_ovl0F_10_8004672C D_ovl0F_10_8004672C[4];
extern UnkStruct_ovl0F_10_8004672C D_ovl0F_10_800467DC[2];
extern s32 D_ovl0F_10_800466C8;
extern s32 D_ovl0F_10_800466CC;
extern s32 D_ovl0F_10_800466D0;
extern s32 D_ovl0F_10_800466D4;
extern s32 D_ovl0F_10_800468A4;
extern s32 D_ovl0F_10_800468A8;
extern s32 D_ovl0F_10_800468AC;
extern f32 D_ovl0F_10_800468B0;
extern s32 D_ovl0F_10_800468B4;
extern UnkStructA *D_ovl0F_10_800468B8;
extern s32 D_ovl0F_10_800468BC;
extern UnkStruct_ovl0F_10_800468C0 D_ovl0F_10_800468C0;

/* unused: the stubs IDO -O3 leaves at the end of the object */
/* A second caller keeps func_ovl0F_10_80043088 from being inlined. */
static void func_ovl0F_10_80044FF0(void) {
    func_ovl0F_10_80043088(D_ovl0F_10_8004672C);
}

static void func_ovl0F_10_80044FF8(void) { D_ovl0F_10_80046B18[0] = D_ovl0F_10_80046B24[0] = D_ovl0F_10_80046B38[0] = 0; /* unused (body dropped): the reference keeps the unused .bss in place */
}

static void func_ovl0F_10_80045000(void) {
}

static void func_ovl0F_10_80045008(void) {
}

static void func_ovl0F_10_80045010(void) {
}

static void func_ovl0F_10_80045018(void) {
}

static void func_ovl0F_10_80045020(void) {
}

static void func_ovl0F_10_80045028(void) {
}

static void func_ovl0F_10_80045030(void) {
}

static void func_ovl0F_10_80045038(void) {
}

static void func_ovl0F_10_80045040(void) {
}

static void func_ovl0F_10_80045048(void) {
}

static void func_ovl0F_10_80045050(void) {
}

static void func_ovl0F_10_80045058(void) {
}

static void func_ovl0F_10_80045060(void) {
}

static void func_ovl0F_10_80045068(void) {
}

void func_ovl0F_10_80044FE8(void) {
}

void func_ovl0F_10_80044FD0(void) {
    D_ovl0F_10_80046B1C = -1;
    D_ovl0F_10_80046B20 = -1;
}

void func_ovl0F_10_80044F74(f32 arg0, f32 arg1, f32 arg2) {
    func_ovl0F_10_80045BF8(D_ovl0F_10_80046B28.x, D_ovl0F_10_80046B28.y, D_ovl0F_10_80046B28.z);
    func_ovl0F_10_80045C18(arg0, arg1, arg2);
    func_ovl0F_10_80045F28(180);
}

void func_ovl0F_10_80044F40(s32 arg0, UnkStructA *arg1) {
    UnkStructA_40 *p;

    p = arg1->unk40;
    func_ovl0F_10_800464B0(arg0, p->unk10, p->unk14, p->unk18);
}

void func_ovl0F_10_80044D3C(void) {
    s32 dir;
    UnkStructA *obj;
    f32 angle;
    f32 step;
    f32 i;

    dir = (s32)func_ovl0F_10_80045F30();
    obj = func_ovl0F_10_800455F8(func_ovl0F_10_80045598());
    D_ovl0F_10_800466B4 = 1;
    angle = 0.0f;
    i = 0.0f;
    func_ovl0F_10_80045C20(NULL, &angle, NULL);
    func_ovl0F_10_80045600(1);
    func_ovl0F_10_80045528(1);
    func_ovl0F_10_80045520(0);
    func_ovl0F_10_80045C60(0x7F);
    if (dir == 0) {
        step = -((angle > 180.0f ? 360.0f - angle : angle) / 50.0f);
    } else {
        step = (angle > 180.0f ? 360.0f - angle : angle) / 50.0f;
    }
    step *= 1.1f;
    while (i < 100.0f) {
        func_ovl0F_10_80045BF8(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18);
        func_ovl0F_10_80045C18(32768.0f, -angle, 32768.0f);
        i += 1.0f;
        angle += step;
        func_ovl0F_10_80045F50();
    }
    func_ovl0F_10_80045528(0);
    func_ovl0F_10_80045600(0);
    func_ovl0F_10_80045520(1);
    func_ovl0F_10_80045C58(0x1F);
    D_ovl0F_10_800466B4 = 0;
    func_ovl0F_10_80045F18();
}

void func_ovl0F_10_80044B8C(void) {
    UnkStruct_ovl0F_10_80045F30 *task;
    UnkStructA *obj;

    task = func_ovl0F_10_80045F30();
    obj = func_ovl0F_10_800455F8(func_ovl0F_10_80045598());
    func_ovl0F_10_80044B84();
    func_ovl0F_10_80044B7C(task, obj);
    func_ovl0F_10_80045F18();
}

static void func_ovl0F_10_80044B7C(UnkStruct_ovl0F_10_80045F30 *task, UnkStructA *obj) {
    static s32 D_ovl0F_10_800466C4 = 0;

    while (D_ovl0F_10_800466C4 < 400) {
        (*task->unk2C)->unk40->unk14 += 3.75f;
        (*task->unk2C)->unk40->unk18 -= 3.75f;
        obj->unk40->unk14 += 3.75f;
        D_ovl0F_10_800466C4++;
        func_ovl0F_10_80045F50();
    }
}

#include "statics.h"

static void func_ovl0F_10_80043410(void) {
    func_ovl0F_10_800453B8(0x233);
    func_ovl0F_10_800453D0(0x240);
    func_ovl0F_10_80045568();
}

static void func_ovl0F_10_80043418(void) {
    D_ovl0F_10_800468B8 = func_ovl0F_10_80045DE0(0x323, 0x1000010, 0xC0, 0);
    D_ovl0F_10_800468B8->unk40->unk10 = 1300.0f;
    D_ovl0F_10_800468B8->unk40->unk14 = 0.0f;
    D_ovl0F_10_800468B8->unk40->unk18 = 300.0f;
}

static void func_ovl0F_10_80043420(void) {
    D_ovl0F_10_80046B04 = func_ovl0F_10_800454B8();
    if (D_ovl0F_10_80046B04 == 0x10) {
        D_ovl0F_10_800466D0 = 1;
        func_ovl0F_10_80046180(0x405);
        func_ovl0F_10_80046438(0);
    } else {
        D_ovl0F_10_800466D0 = 0;
    }
}

static void func_ovl0F_10_80043428() {
}

static void func_ovl0F_10_8004333C(void) {
    func_ovl0F_10_80045C18(0.0f, 32768.0f, 32768.0f);
}

static void func_ovl0F_10_80043334(void) {
    func_ovl0F_10_80045508();
}

static void func_ovl0F_10_8004332C(void) {
    switch (func_ovl0F_10_800454B8()) {
        case 0x12:
            func_ovl0F_10_800455B8(D_ovl0F_10_8004686C[0].unk0, D_ovl0F_10_8004686C[0].unk4,
                                   D_ovl0F_10_8004686C[0].unk8, D_ovl0F_10_8004686C[0].unkC,
                                   D_ovl0F_10_8004686C[0].unk10, D_ovl0F_10_8004686C[0].unk14,
                                   D_ovl0F_10_8004686C[0].unk18);
            break;
        case 0x13:
            func_ovl0F_10_800455B8(D_ovl0F_10_8004686C[1].unk0, D_ovl0F_10_8004686C[1].unk4,
                                   D_ovl0F_10_8004686C[1].unk8, D_ovl0F_10_8004686C[1].unkC,
                                   D_ovl0F_10_8004686C[1].unk10, D_ovl0F_10_8004686C[1].unk14,
                                   D_ovl0F_10_8004686C[1].unk18);
            break;
    }
}

static void func_ovl0F_10_80043078(s32 count, UnkStruct_ovl0F_10_8004672C *p) {
    s32 i;

    for (i = 0; i < count; i++) {
        func_ovl0F_10_80045DD0(*p->unk0);
        p++;
    }
}

static void func_ovl0F_10_800432D8(void) {
    if (func_ovl0F_10_80046190(0x405) == 0 && func_ovl0F_10_80045580(0xB, 3, 0xC, 4) != 0) {
        func_ovl0F_10_800455A8(1);
        func_ovl0F_10_80046478();
        func_ovl0F_10_80046398();
        func_ovl0F_10_800455D0(1);
        func_ovl0F_10_80046490(0x29);
        func_ovl0F_10_80046180(D_ovl0F_10_800467DC[0].unk28);
        func_ovl0F_10_80043078(1, D_ovl0F_10_800467DC);
        func_ovl0F_10_800431BC(D_ovl0F_10_800467DC, 0);
        func_ovl0F_10_800454B0(0x5C, 0x11, 0);
    }
}

static void func_ovl0F_10_80043080(void) {
}

static void func_ovl0F_10_80043070(void) {
    D_ovl0F_10_80046B14 = func_ovl0F_10_80045598();
    func_ovl0F_10_80045578(&D_ovl0F_10_80046B08);
    func_ovl0F_10_800455A0(D_ovl0F_10_80046B14, D_ovl0F_10_80046AF8);
}

static void func_ovl0F_10_80043068(void) {
    if (D_ovl0F_10_800466D0 == 0) {
        if (2500.0f <= D_ovl0F_10_80046AF8[2] && D_ovl0F_10_800468A8 == 1) {
            if (func_ovl0F_10_80043010(0)) {
                D_ovl0F_10_800468A8 = 0;
            }
        } else if (D_ovl0F_10_80046AF8[2] < 2500.0f && D_ovl0F_10_800468A8 == 0) {
            if (func_ovl0F_10_80043010(1)) {
                D_ovl0F_10_800468A8 = 1;
            }
        }
    }
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl0F_10_80043430(s32 arg0) {
    UnkStructA *obj;
    f32 y;
    s32 count;
    u32 (*p)[3];
    s32 i;

    switch (arg0) {
        case 2:
            D_ovl0F_10_800468BC = -1;
            D_ovl0F_10_800466D4 = 0;
            func_ovl0F_10_80043428();
            func_ovl0F_10_80043420();
            func_ovl0F_10_80043418();
            func_ovl0F_10_80046270(D_ovl0F_10_800466D8);
            p = D_ovl0F_10_800466FC;
            count = 4;
            for (i = 0; i < count; i++) {
                func_ovl0F_10_80046418((*p)[0], (*p)[1], (*p)[2]);
                p++;
            }
            func_ovl0F_10_80043410();
            D_ovl0F_10_800466B4 = 0;
            D_ovl0F_10_800466B0 = 0;
            func_ovl0F_10_80045CB0(70.0f, 45.0f);
            func_ovl0F_10_8004310C(D_ovl0F_10_8004672C, 4);
            func_ovl0F_10_80043408();
            if (func_ovl0F_10_80046190(0x405) == 0) {
                func_ovl0F_10_8004338C(0x279);
            } else {
                func_ovl0F_10_8004338C(0x27A);
            }
            func_ovl0F_10_800431BC(D_ovl0F_10_800467DC, 1);
            break;
        case 16:
            D_ovl0F_10_800466D0 = 1;
            func_ovl0F_10_80045570(0x2D, 5, 0x1D);
            func_ovl0F_10_800455B0(0);
            break;
        case 18:
            func_ovl0F_10_800455B0(0);
            func_ovl0F_10_80045570(0x2D, 5, 0x1D);
            func_ovl0F_10_80043344(-90.0f);
            break;
        case 19:
            func_ovl0F_10_800455B0(0);
            func_ovl0F_10_80045570(0xA, 0, 0x1E);
            func_ovl0F_10_80043344(90.0f);
            break;
        case 20:
            func_ovl0F_10_80045570(0x2D, 5, 0x1A);
            break;
        case 21:
            func_ovl0F_10_80045570(0xB, 2, 3);
            func_ovl0F_10_8004333C();
            break;
        case 4:
            func_ovl0F_10_800455C8();
            D_ovl0F_10_800468A8 = 0;
            func_ovl0F_10_80045CB0(55.0f, 54.9f);
            func_ovl0F_10_800454A0(func_ovl0F_10_80044780);
            func_ovl0F_10_80043334();
            func_ovl0F_10_80045498(func_ovl0F_10_80044074, 1);
            if (D_ovl0F_10_800466D0 == 1) {
                func_ovl0F_10_800455A8(1);
                func_ovl0F_10_80045520(0);
                func_ovl0F_10_80045F08(func_ovl0F_10_80043E08, NULL, 0, 0, 0x130);
            }
            D_ovl0F_10_800468BC = func_ovl0F_10_80045F08(func_ovl0F_10_80043CCC, NULL, 0, 0, 0x130);
            break;
        case 5:
            func_ovl0F_10_8004332C();
            break;
        case 0x50:
            func_ovl0F_10_800432E0(D_ovl0F_10_80046834);
            func_ovl0F_10_800432E0(D_ovl0F_10_80046850);
            break;
        case 0x52:
            func_ovl0F_10_800432D8();
            break;
        case 15:
            func_ovl0F_10_80043080();
            if (func_ovl0F_10_80045588(0xD, 0, 4, 0xD, 4) != 0) {
                obj = func_ovl0F_10_800455F8(func_ovl0F_10_80045598());
                y = obj->unk40->unk20;
                if (135.0f <= y && y <= 225.0f) {
                    if (func_ovl0F_10_80046170(0xA4) != 0) {
                        func_ovl0F_10_80045B20(0x291, 1);
                    } else {
                        func_ovl0F_10_80045B20(0x278, 2);
                    }
                }
            }
            break;
        case 1:
            func_ovl0F_10_80043078(4, D_ovl0F_10_8004672C);
            if (D_ovl0F_10_800466B8 != 0) {
                func_ovl0F_10_80043078(D_ovl0F_10_800466B8, D_ovl0F_10_800467DC);
            }
            func_ovl0F_10_80045DD0(D_ovl0F_10_800468B8);
            D_ovl0F_10_800468B8 = NULL;
            if (D_ovl0F_10_800466D0 == 1) {
                func_ovl0F_10_80046610();
            }
            if (D_ovl0F_10_800468BC != -1) {
                func_ovl0F_10_80045F10(D_ovl0F_10_800468BC);
                D_ovl0F_10_800468BC = -1;
            }
            break;
        case 12:
            func_ovl0F_10_80045600(1);
            break;
        case 3:
            func_ovl0F_10_80043070();
            func_ovl0F_10_80043068();
            D_ovl0F_10_800468A4 = 1;
            if (D_ovl0F_10_800466CC > 0) {
                D_ovl0F_10_800466CC--;
            }
            if (D_ovl0F_10_800466D4 > 0) {
                D_ovl0F_10_800466D4--;
            }
            break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 13:
        case 14:
        case 17:
            break;
        case 0x53:
            break;
    }
}

static void func_ovl0F_10_8004338C(s32 arg0) {
    D_ovl0F_10_80046B34 = func_ovl0F_10_80045F90(arg0);
    func_ovl0F_10_80045FA0(D_ovl0F_10_80046B34, 0x20000000);
    D_ovl0F_10_80046B34->unk40 = 0.6f;
    D_ovl0F_10_80046B34->unk44 = D_ovl0F_10_80046B34->unk40 * 0.75f;
    D_ovl0F_10_80046B34->unk3C = D_ovl0F_10_80046B34->unk4C * 0.3f;
}

static void func_ovl0F_10_800431BC(UnkStruct_ovl0F_10_8004672C *arg0, s32 arg1) {
    if (func_ovl0F_10_80046190(arg0->unk28) != 0) {
        arg0->unk4 = 0x1B0;
        D_ovl0F_10_800466B8 = 1;
    } else {
        arg0->unk4 = 0x1B3;
        D_ovl0F_10_800466B8 = 1;
    }
    func_ovl0F_10_8004310C(arg0, D_ovl0F_10_800466B8);
    (*arg0->unk0)->unk40->unk10 += 50.0f;
    (*arg0->unk0)->unk40->unk14 += 2.0f;
    (*arg0->unk0)->unk40->unk18 += 50.0f;
    (*arg0->unk0)->unk40->unk28 = 2.0f;
    (*arg0->unk0)->unk40->unk30 = 2.0f;
}

static void func_ovl0F_10_8004310C(UnkStruct_ovl0F_10_8004672C *arg0, s32 arg1) {
    s32 i;

    for (i = 0; i < arg1; i++) {
        *arg0->unk0 = func_ovl0F_10_80045E28(arg0->unk4, arg0->unk8);
        func_ovl0F_10_80045DB8(*arg0->unk0, arg0->unk24 | 0x01000000);
        func_ovl0F_10_80043088(arg0);
        (*arg0->unk0)->unk40->unk1C = arg0->unk18;
        (*arg0->unk0)->unk40->unk20 = arg0->unk1C;
        (*arg0->unk0)->unk40->unk24 = arg0->unk20;
        arg0++;
    }
}

static void func_ovl0F_10_80043088(UnkStruct_ovl0F_10_8004672C *arg0) {
    f32 pos[3];

    func_ovl0F_10_80045418(arg0->unkC, arg0->unk10, arg0->unk14, pos, 0);
    (*arg0->unk0)->unk40->unk10 = pos[0];
    (*arg0->unk0)->unk40->unk14 = pos[1];
    (*arg0->unk0)->unk40->unk18 = pos[2];
}

static void func_ovl0F_10_800432E0(s32 *arg0) {
    if (func_ovl0F_10_80045580(arg0[0], arg0[1], arg0[2], arg0[3]) != 0) {
        func_ovl0F_10_800454B0(arg0[4], arg0[5], arg0[6]);
    }
}

static s32 func_ovl0F_10_80043010(s32 arg0) {
    s32 ret;

    ret = 0;
    if (D_ovl0F_10_800468AC == 0) {
        ret = 1;
        if (arg0 != 0) {
            D_ovl0F_10_800468B0 = 55.0f;
        } else {
            D_ovl0F_10_800468B0 = 75.0f;
        }
        D_ovl0F_10_800468B4 = arg0;
        D_ovl0F_10_800468AC = 1;
    }
    return ret;
}
