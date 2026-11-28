/*
 * ovl_0D_01's stage object, IDO -O3; it matches in us, eu and jp.
 *
 * The sibling of ovl_0D_04 and ovl_0D_06 (src/overlays): the gate code
 * (800444E0, 800448D4), the model tables, the setup's loops and the camera
 * statics are theirs, renamed. The layout rules that placed every stub: a
 * global the setup (or 80044F00, 800439CC) calls comes out between its
 * statics only when defined above it, so 8004553C, 800454F8, 800455B8,
 * 80044E2C, 80044EC4 and 800438C0 sit above their callers; 80044D74 stays a
 * call from the handler only with its two stubs as empty statics called
 * before each block. What fixed what (from 1089 words in us and jp):
 * - 800436C8/800434BC: the fields compared with 0xFFFF (as the siblings).
 * - 800448D4: the per-gate static 800448BC (Ent, flag) starts with the
 *   flag test (`flag == 0x400 && !0x407` returns), so the loop's float
 *   constants are hoisted into $fs0-$fs4 as the original; its first call,
 *   the macro's static 800448CC, is defined in another file (statics.h,
 *   as ovl_0D_02's 80043508), which keeps it out of the prologue, with the
 *   setup 800455E0 writing the macro itself so that 800448D4 is the
 *   static's first inliner (pass 12; before, the call was in a statics.h
 *   region and 80044D6C defined in another; the old fake read of
 *   D_ovl0D_01_80047560 stopped the hoisting).
 * - 80043134: the second probe's x is an int 2300 (its own .rodata
 *   2300.0f, as the original's two), and the fade's length held in a
 *   float local above pos puts it at sp+96 (a guess; it was an unused
 *   local, a fake, before pass 12).
 * - 800455E0: both 1600s of the camera static 800455B0 floats, and that
 *   static defined in another file (statics.h; 1600.0f in $f16);
 *   80046C70 takes a u32 (with s32,
 *   uopt kept the three 0x02000000s in s7); 80046460 returns s32
 *   (v1, not v0, for the two inlined flag tests at the end). The stub
 *   800455D8 is the probe static (ovl_0D_02's 800444EC, with the 0xC02
 *   call 80046278 that comes just before it there), defined in statics.h:
 *   its `.file` puts the call's a0 after the three stores, and as a
 *   parameterless inlined static it is the frame's word (pass 13; it
 *   replaces an empty static called after the 800455D0 rows for the frame
 *   and a statics.h region around the 80046278 call, a fake).
 * - 80044154: 15.0f through a local y, and the model reached as D_ovl0D_01_800475D4[2] (the address of
 *   D_ovl0D_01_8004760C[1]; a fake, so uopt doesn't keep &[1] in a
 *   register for both the store and the load).
 * - 80044464/800444D8/800444E0: ovl_0D_06's clamp (axis, v, size),
 *   direction (takes the entity) and tiles (`len = x1 + z1`; the tiles
 *   static defined in statics.h, which replaces a statics.h region around
 *   case 0's first statement).
 * - 800437C8: its parameters (id, x, n, z), as ovl_0D_06's 8004355C.
 * - 80045A00: an empty `case 31` (the jump table runs to 31).
 * - D_ovl0D_01_80047AFC is unused .bss: the dead stub 80045F14 refers to
 *   it (a fake), so it keeps its word.
 */
#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_0D_01's stage object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse; docs/DECOMPILING.md).
 * ovl_0D_04 and ovl_0D_06 (src/overlays) are its siblings.
 */

#define ZJ_0C00 func_ovl0D_01_80046268
#define ZJ_0C02 func_ovl0D_01_80046278
#define ZJ_0D05 func_ovl0D_01_800462A8
#define ZJ_0D06 func_ovl0D_01_800462B0
#define ZJ_0D08 func_ovl0D_01_800462C0
#define ZJ_0D0A func_ovl0D_01_800462D0
#define ZJ_0D0C func_ovl0D_01_800462E0
#define ZJ_0D0D func_ovl0D_01_800462E8
#define ZJ_0D16 func_ovl0D_01_80046330
#define ZJ_0E00 func_ovl0D_01_80046338
#define ZJ_0E01 func_ovl0D_01_80046340
#define ZJ_0E03 func_ovl0D_01_80046350
#define ZJ_0F00 func_ovl0D_01_80046360
#define ZJ_0F01 func_ovl0D_01_80046368
#define ZJ_0F05 func_ovl0D_01_80046388
#define ZJ_0F0A func_ovl0D_01_800463B0
#define ZJ_0F0B func_ovl0D_01_800463B8
#define ZJ_0F0E func_ovl0D_01_800463D0
#define ZJ_0F0F func_ovl0D_01_800463D8
#define ZJ_0F12 func_ovl0D_01_800463F0
#define ZJ_1000 func_ovl0D_01_80046418
#define ZJ_1001 func_ovl0D_01_80046420
#define ZJ_1002 func_ovl0D_01_80046428
#define ZJ_1003 func_ovl0D_01_80046430
#define ZJ_1004 func_ovl0D_01_80046438
#define ZJ_1006 func_ovl0D_01_80046448
#define ZJ_1007 func_ovl0D_01_80046450
#define ZJ_1008 func_ovl0D_01_80046458
#define ZJ_1009 func_ovl0D_01_80046460
#define ZJ_100A func_ovl0D_01_80046468
#define ZJ_100B func_ovl0D_01_80046470
#define ZJ_100C func_ovl0D_01_80046478
#define ZJ_100D func_ovl0D_01_80046480
#define ZJ_100F func_ovl0D_01_80046490
#define ZJ_1020 func_ovl0D_01_80046518
#define ZJ_1501 func_ovl0D_01_80046950
#define ZJ_1509 func_ovl0D_01_80046990
#define ZJ_150C func_ovl0D_01_800469A8
#define ZJ_150D func_ovl0D_01_800469B0
#define ZJ_150E func_ovl0D_01_800469B8
#define ZJ_1511 func_ovl0D_01_800469D0
#define ZJ_1704 func_ovl0D_01_80046AA8
#define ZJ_1708 func_ovl0D_01_80046AC8
#define ZJ_1709 func_ovl0D_01_80046AD0
#define ZJ_170B func_ovl0D_01_80046AE0
#define ZJ_170E func_ovl0D_01_80046AF8
#define ZJ_170F func_ovl0D_01_80046B00
#define ZJ_1710 func_ovl0D_01_80046B08
#define ZJ_1711 func_ovl0D_01_80046B10
#define ZJ_1713 func_ovl0D_01_80046B20
#define ZJ_1718 func_ovl0D_01_80046B48
#define ZJ_171A func_ovl0D_01_80046B58
#define ZJ_171B func_ovl0D_01_80046B60
#define ZJ_171C func_ovl0D_01_80046B68
#define ZJ_171F func_ovl0D_01_80046B80
#define ZJ_1721 func_ovl0D_01_80046B90
#define ZJ_1722 func_ovl0D_01_80046B98
#define ZJ_1728 func_ovl0D_01_80046BC8
#define ZJ_1729 func_ovl0D_01_80046BD0
#define ZJ_1804 func_ovl0D_01_80046C00
#define ZJ_1807 func_ovl0D_01_80046C18
#define ZJ_1808 func_ovl0D_01_80046C20
#define ZJ_1811 func_ovl0D_01_80046C68
void func_ovl0D_01_80046C70(UnkStructA *, u32); /* 0x1812, kept: its u32 parameter 1: the header's s32 changes its code */
#define ZJ_1814 func_ovl0D_01_80046C80
#define ZJ_1816 func_ovl0D_01_80046C90
#define ZJ_1818 func_ovl0D_01_80046CA0
#define ZJ_181A func_ovl0D_01_80046CB0
#define ZJ_1900 func_ovl0D_01_80046DB8
#define ZJ_1902 func_ovl0D_01_80046DC8
#define ZJ_1904 func_ovl0D_01_80046DD8
#define ZJ_1905 func_ovl0D_01_80046DE0
#define ZJ_1909 func_ovl0D_01_80046E00
#define ZJ_1D00 func_ovl0D_01_80046F60
#define ZJ_1D02 func_ovl0D_01_80046F70
#define ZJ_1E04 func_ovl0D_01_80047020
#define ZJ_1E06 func_ovl0D_01_80047030
#define ZJ_1E08 func_ovl0D_01_80047040
#define ZJ_1E0A func_ovl0D_01_80047050
#define ZJ_2002 func_ovl0D_01_800470A8
#define ZJ_2003 func_ovl0D_01_800470B0
#define ZJ_2004 func_ovl0D_01_800470B8
#define ZJ_2101 func_ovl0D_01_800470E0
#define ZJ_2200 func_ovl0D_01_80047100
#define ZJ_2201 func_ovl0D_01_80047108
#define ZJ_2300 func_ovl0D_01_80047120
#define ZJ_2303 func_ovl0D_01_80047138
#define ZJ_2601 func_ovl0D_01_80047210
#define ZJ_2602 func_ovl0D_01_80047218
#define ZJ_2704 func_ovl0D_01_80047248
#define ZJ_2A00 func_ovl0D_01_800472C8
#define ZJ_2A0C func_ovl0D_01_80047328
#define ZJ_2B00 func_ovl0D_01_80047340
#define ZJ_2B04 func_ovl0D_01_80047360
#define ZJ_2B06 func_ovl0D_01_80047370
#define ZJ_3000 func_ovl0D_01_800474B8
#define ZJ_3001 func_ovl0D_01_800474C0
#define ZJ_3103 func_ovl0D_01_800474F0
#define ZJ_3104 func_ovl0D_01_800474F8
#define ZJ_3105 func_ovl0D_01_80047500
#define ZJ_3106 func_ovl0D_01_80047508
#define ZJ_3108 func_ovl0D_01_80047518
#define ZJ_3109 func_ovl0D_01_80047520
#define ZJ_310A func_ovl0D_01_80047528
#include "overlays/common/zerojump.h"

#define UNK_MACRO_ovl0D_01_800448D4() \
    D_ovl0D_01_80047AEC = func_ovl0D_01_80046448(); func_ovl0D_01_80046428(&D_ovl0D_01_80047AE0); func_ovl0D_01_80046450(D_ovl0D_01_80047AEC, &D_ovl0D_01_80047AD0)

static void func_ovl0D_01_80043EFC(void);
static void func_ovl0D_01_800434BC(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_01_800436C8(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_01_800437C8(s32, s32, s32, s32);
static void func_ovl0D_01_80043868(UnkStructA *);
static void func_ovl0D_01_80043944(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_01_80043CF8(UnkStructA *);
static void func_ovl0D_01_80044464(s32, s32 *, s32 *);
static s32 func_ovl0D_01_800444D8(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_01_800444E0(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_01_800448BC(UnkStruct_ovl0C_01_8004A54C *, s32);
static void func_ovl0D_01_800448C4(void);
static void func_ovl0D_01_800448CC(void);
static void func_ovl0D_01_80044D64(void);
static void func_ovl0D_01_80044D6C(void);
static void func_ovl0D_01_80044E04(void);
static void func_ovl0D_01_80044E0C(void);
static void func_ovl0D_01_80044E14(void);
static void func_ovl0D_01_80044E1C(void);
static void func_ovl0D_01_80044E24(void);
static void func_ovl0D_01_80045088(void);
static void func_ovl0D_01_800454F0(void);
static void func_ovl0D_01_80045580(void);
static void func_ovl0D_01_80045588(void);
static void func_ovl0D_01_80045590(void);
static void func_ovl0D_01_80045598(void);
static void func_ovl0D_01_800455A0(void);
static void func_ovl0D_01_800455A8(void);
static void func_ovl0D_01_800455B0(void);
static void func_ovl0D_01_800455D0(Trio_0D01 *, s32);
static void func_ovl0D_01_800455D8(void);
static void func_ovl0D_01_80045F14(void);

void func_ovl0D_01_80043010(void);
void func_ovl0D_01_80043098(void);
s32 func_ovl0D_01_80043100(s32, void *);
void func_ovl0D_01_80043134(void);
void func_ovl0D_01_8004363C(void);
void func_ovl0D_01_80043744(void);
void func_ovl0D_01_8004376C(f32, f32, f32);
void func_ovl0D_01_800438C0(void);
void func_ovl0D_01_800439CC(void);
void func_ovl0D_01_80043D3C(void);
void func_ovl0D_01_80043DDC(void);
void func_ovl0D_01_80043E1C(s32, s32);
void func_ovl0D_01_80043E90(void);
void func_ovl0D_01_80043F04(f32, f32, f32, f32, s32);
void func_ovl0D_01_80044038(void);
void func_ovl0D_01_800443BC(void);
void func_ovl0D_01_80044154(void);
void func_ovl0D_01_80044408(void);
void func_ovl0D_01_80044428(void);
void func_ovl0D_01_800448D4(void);
void func_ovl0D_01_80044C0C(void);
void func_ovl0D_01_80044CB0(void);
void func_ovl0D_01_80044D74(void);
void func_ovl0D_01_80044EC4(void);
void func_ovl0D_01_80044E2C(void);
void func_ovl0D_01_80044F00(void);
void func_ovl0D_01_80045090(void);
void func_ovl0D_01_800450C0(void);
void func_ovl0D_01_80045164(void);
void func_ovl0D_01_800451F4(void);
void func_ovl0D_01_80045270(f32);
void func_ovl0D_01_800452B8(void);
void func_ovl0D_01_80045408(void);
void func_ovl0D_01_80045430(void);
void func_ovl0D_01_80045458(void);
void func_ovl0D_01_80045480(void);
void func_ovl0D_01_800454A8(void);
void func_ovl0D_01_800454F8(void);
void func_ovl0D_01_8004553C(void);
void func_ovl0D_01_800455B8(void);
void func_ovl0D_01_800455E0(void);
void func_ovl0D_01_80045A00(s32);
void func_ovl0D_01_80045BB4(void);
void func_ovl0D_01_80045C60(void);
void func_ovl0D_01_80045CFC(UnkStructA *);
void func_ovl0D_01_80045D40(UnkStructA *, s32, s32, f32, f32);
void func_ovl0D_01_80045E18(void);

/* .bss */
Vec4f_0D01 D_ovl0D_01_80047AD0;
Vec3f D_ovl0D_01_80047AE0;
s32 D_ovl0D_01_80047AEC;
s32 D_ovl0D_01_80047AF0;
s32 D_ovl0D_01_80047AF4;
s32 D_ovl0D_01_80047AF8;
u8 D_ovl0D_01_80047AFC[0x4]; /* unused (func_ovl0D_01_80045F14) */
Vec3f D_ovl0D_01_80047B00;
UnkStructA *D_ovl0D_01_80047B0C;
s32 D_ovl0D_01_80047B10;
s32 D_ovl0D_01_80047B14;
s32 D_ovl0D_01_80047B18[3];
s32 D_ovl0D_01_80047B24;
s32 D_ovl0D_01_80047B28;
UnkStruct_802AC610 *D_ovl0D_01_80047B2C;
Vec3f D_ovl0D_01_80047B30;

/* .data */
u32 D_ovl0D_01_80047560[][3] = {
    { 0x990002, 0x6000E, 4 },
    { 0x9A0002, 0xE, 4 },
    { 0x970001, 0xF, 0x2004 },
};
u32 D_ovl0D_01_80047584[][3] = {
    { 0, 4, 0 },
    { 4, 0x980000, 0x84 },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_8004759C[1] = {
    { NULL, -1, 0x148, -1, -1, -1, -1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_800475D4[1] = {
    { NULL, -1, 0x146, -1, -1, -1, -1, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_8004760C[2] = {
    { NULL, -1, 0x14C, 0, 0x400, 0x20, 0x400, 290.0f, 140.0f, -100.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x14D, 0, 0x400, 0x20, 0x400, 140.0f, 45.0f, 10.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_8004767C[2] = {
    { NULL, -1, 0x14B, 0, 0x400, 0x20, 0x400, 350.0f, 140.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x14B, -1, -1, -1, -1, 130.0f, 45.0f, 40.0f, 0.0f, 270.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_800476EC[2] = {
    { NULL, -1, 0x15E, -1, -1, -1, -1, 270.0f, 60.0f, 40.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x15C, -1, -1, -1, -1, 460.0f, 120.0f, 80.0f, 0.0f, 180.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
s32 D_ovl0D_01_8004775C[2] = { 0x400, 0x422 };
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_80047764[1] = {
    { NULL, -1, 0x14A, -1, -1, -1, -1, 140.0f, -20.0f, 40.0f, 0.0f, -90.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_8004779C[2] = {
    { NULL, -1, 0x149, -1, -1, -1, -1, 130.0f, 60.0f, 20.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x149, -1, -1, -1, -1, 200.0f, 40.0f, 190.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_8004780C[1] = {
    { NULL, -1, 0x15F, -1, -1, -1, -1, 130.0f, 40.0f, 260.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
Trio_0D01 D_ovl0D_01_80047844[6] = {
    { 0, 7, 0xE5 },
    { 1, 2, 0xE0 },
    { 2, 9, 0xE7 },
    { 3, 9, 0xE7 },
    { 4, 8, 0xE6 },
    { 5, 0, 0xDE },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_8004788C[1] = {
    { NULL, -1, 0x147, -1, -1, -1, -1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_01_800478C4[1] = {
    { NULL, -1, 0x323, -1, -1, -1, -1, 130.0f, 60.0f, 100.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_802363BC D_ovl0D_01_800478FC = { 0x708, 1, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f };

#include "statics.h"

/* unused: the stub IDO -O3 leaves at the end of the object; it refers to the
 * .bss nothing else uses, which would otherwise be misplaced (a fake) */
static void func_ovl0D_01_80045F14(void) {
    D_ovl0D_01_80047AFC[0] = 0;
}

void func_ovl0D_01_80045E18(void) {
    UnkStructA *self;
    UnkStructA *o;
    s32 i;
    s32 n;

    self = func_ovl0D_01_80046DE0();
    o = func_ovl0D_01_80046C90(0x183, 0x11080000, 0xD0, 0);
    o->unk40->unk10 = self->unk40->unk10;
    o->unk40->unk14 = self->unk40->unk14;
    o->unk40->unk18 = self->unk40->unk18;
    n = 20;
    func_ovl0D_01_80043100(0xB3, o);
    for (i = 0; i < n; i++) {
        func_ovl0D_01_80045D40(o, i, 20, 2.0f, 4.0f);
        func_ovl0D_01_80046E00();
    }
    func_ovl0D_01_80046C80(o);
    func_ovl0D_01_80046DC8();
}

void func_ovl0D_01_80045D40(UnkStructA *arg0, s32 arg1, s32 arg2, f32 arg3, f32 arg4) {
    arg0->unk40->unk28 = func_ovl0D_01_80046F60((f32) arg1 / (f32) arg2 * 90.0f) * arg3;
    arg0->unk40->unk2C = func_ovl0D_01_80046F60((f32) arg1 / (f32) arg2 * 180.0f) * arg4;
    arg0->unk40->unk36 = (s32) ((1.0f - (f32) arg1 / (f32) arg2) * 63.0f) + 0x80;
}

/* the callback func_ovl0D_01_800455E0 registers */
void func_ovl0D_01_80045CFC(UnkStructA *arg0) {
    func_ovl0D_01_80046C70(arg0, 0x01000000);
    func_ovl0D_01_80046DB8(func_ovl0D_01_80045E18, arg0, 0, 0, 0x150);
}

/* a task: the scripted run when the second exit is open */
void func_ovl0D_01_80045C60(void) {
    func_ovl0D_01_80046458(1);
    while (func_ovl0D_01_80047040(0x424) == 0) {
        func_ovl0D_01_80046E00();
    }
    func_ovl0D_01_80046458(1);
    func_ovl0D_01_800463D0(0);
    func_ovl0D_01_80046AE0(&D_ovl0D_01_800478FC, 0x170, 0);
    func_ovl0D_01_80046470("WM1 WAP WG1 WSH WDE WB6 ST1 WDN WB0 ST1 M1 ST1 M6 ST30");
    func_ovl0D_01_80046470("WM1 WAP WG1 WSH WDN WB5 ST10 M2 ST1 WDS WB3 ST1 WDN WB0 ST120 WB10");
    func_ovl0D_01_80046DD8(90);
    func_ovl0D_01_800474B8();
    func_ovl0D_01_80046DC8();
}

/* a task: the scripted run when the first exit is open */
void func_ovl0D_01_80045BB4(void) {
    func_ovl0D_01_80046458(1);
    while (func_ovl0D_01_80047040(0x424) == 0) {
        func_ovl0D_01_80046E00();
    }
    func_ovl0D_01_80046458(1);
    func_ovl0D_01_800463D0(0);
    func_ovl0D_01_80046470("WM1 WAP WG1 WSH ST1 WDN WB3 ST1 WDNE WB5 ST1");
    func_ovl0D_01_80047518(0x14);
    func_ovl0D_01_800474F0(1);
    while (func_ovl0D_01_80047500() != 0) {
        func_ovl0D_01_80046E00();
    }
    func_ovl0D_01_80047520();
    func_ovl0D_01_80046DC8();
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl0D_01_80045A00(s32 arg0) {
    switch (arg0) {
        case 2:
            func_ovl0D_01_800455E0();
            break;
        case 17:
            func_ovl0D_01_800454A8();
            break;
        case 18:
            func_ovl0D_01_80045480();
            break;
        case 19:
            func_ovl0D_01_80045458();
            break;
        case 20:
            func_ovl0D_01_80045430();
            break;
        case 26:
            func_ovl0D_01_80045408();
            break;
        case 27:
            func_ovl0D_01_80045408();
            break;
        case 4:
            func_ovl0D_01_800452B8();
            break;
        case 5:
            func_ovl0D_01_800450C0();
            break;
        case 0x50:
            func_ovl0D_01_80044D74();
            break;
        case 0x52:
            func_ovl0D_01_80044CB0();
            break;
        case 0x53:
            func_ovl0D_01_80044C0C();
            break;
        case 15:
            func_ovl0D_01_80043010();
            break;
        case 3:
            func_ovl0D_01_800448D4();
            break;
        case 12:
            func_ovl0D_01_80045090();
            break;
        case 1:
            func_ovl0D_01_80044F00();
            break;
        case 31: /* nothing: the jump table runs to 31 */
            break;
    }
}

void func_ovl0D_01_8004553C(void) {
    if (D_ovl0D_01_80047B2C->unk0 & 2) {
        func_ovl0D_01_80046420(0x27, 3, 0xF);
        func_ovl0D_01_80046460(0x40);
    }
}

void func_ovl0D_01_800454F8(void) {
    if (D_ovl0D_01_80047B2C->unk0 & 4) {
        func_ovl0D_01_80046420(0x1D, 2, 0x1C);
        func_ovl0D_01_80046460(0);
    }
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl0D_01_800455B8
#define CORE_FUNC_B func_ovl0D_01_8004376C
#define CORE_FUNC_C func_ovl0D_01_80045270
#define CORE_FUNC_D func_ovl0D_01_80043100
#define CORE_BSS_A0 D_ovl0D_01_80047AF4
#define CORE_BSS_A1 D_ovl0D_01_80047AF8
#define CORE_BSS_B D_ovl0D_01_80047B00
#define CORE_ZJ_1704 func_ovl0D_01_80046AA8
#define CORE_ZJ_1708 func_ovl0D_01_80046AC8
#define CORE_ZJ_1709 func_ovl0D_01_80046AD0
#define CORE_ZJ_1904 func_ovl0D_01_80046DD8
#define CORE_ZJ_2B04 func_ovl0D_01_80047360
#include "overlays/common/core.h"

void func_ovl0D_01_800455E0(void) {
    func_ovl0D_01_80047120(D_ovl0D_01_80047560);
    func_ovl0D_01_80047138(D_ovl0D_01_80047584);
    func_ovl0D_01_80046268(0x13A);
    if (func_ovl0D_01_80047040(0x407) != 0) {
        D_ovl0D_01_80047844[2].unk4 = 0xA;
        D_ovl0D_01_80047844[2].unk8 = 0xE8;
    }
    if (func_ovl0D_01_80047040(0x408) != 0) {
        D_ovl0D_01_80047844[3].unk4 = 0x24;
        D_ovl0D_01_80047844[3].unk8 = 0xE8;
    }
    func_ovl0D_01_800455D0(D_ovl0D_01_80047844, 6);
    func_ovl0D_01_800462E0(0x140);
    func_ovl0D_01_80046418();
    D_ovl0D_01_80047B30.x = 32768.0f;
    D_ovl0D_01_80047B30.y = 32768.0f;
    D_ovl0D_01_80047B30.z = 32768.0f;
    func_ovl0D_01_800455D8();
    func_ovl0D_01_800463B0(0);
    func_ovl0D_01_80047100(1);
    func_ovl0D_01_80046330(0x201F);
    func_ovl0D_01_80046350(func_ovl0D_01_80045CFC);
    func_ovl0D_01_800455B8();
    D_ovl0D_01_80047B2C = func_ovl0D_01_800463B8();
    func_ovl0D_01_800455B0();
    UNK_MACRO_ovl0D_01_800448D4();
    func_ovl0D_01_800455A8();
    func_ovl0D_01_800455A0();
    D_ovl0D_01_80047B10 = 0;
    D_ovl0D_01_80047B14 = 0;
    func_ovl0D_01_80045598();
    func_ovl0D_01_80045590();
    func_ovl0D_01_80045588();
    func_ovl0D_01_80045580();
    func_ovl0D_01_80044408();
    func_ovl0D_01_800462E8();
    func_ovl0D_01_800434BC(D_ovl0D_01_800478C4);
    func_ovl0D_01_80046C70(D_ovl0D_01_800478C4[0].unk0, 0x80000);
    func_ovl0D_01_80046C68(D_ovl0D_01_800478C4[0].unk0, 0x02000000);
    func_ovl0D_01_80046CA0(D_ovl0D_01_800478C4[0].unk0);
    func_ovl0D_01_8004553C();
    func_ovl0D_01_800454F8();
    func_ovl0D_01_800454F0();
}

static void func_ovl0D_01_800455D0(Trio_0D01 *t, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        func_ovl0D_01_800472C8(t->unk0, t->unk4, t->unk8);
        t++;
    }
}

static void func_ovl0D_01_800455A8(void) {
    UnkStruct_ovl0C_01_8004A54C *p;

    for (p = D_ovl0D_01_8004760C; p < D_ovl0D_01_8004760C + 2; p++) {
        func_ovl0D_01_800434BC(p);
        p->unk0->unk40->unk36 = 0x80;
    }
}

static void func_ovl0D_01_800455A0(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        func_ovl0D_01_800434BC(&D_ovl0D_01_8004767C[i]);
        D_ovl0D_01_8004767C[i].unk0->unk40->unk36 = 0x80;
    }
}

static void func_ovl0D_01_80045598(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        func_ovl0D_01_800434BC(&D_ovl0D_01_800476EC[i]);
        func_ovl0D_01_80046C68(D_ovl0D_01_800476EC[i].unk0, 0x02000000);
        func_ovl0D_01_80046C70(D_ovl0D_01_800476EC[i].unk0, 0x80000);
    }
}

static void func_ovl0D_01_80045590(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (func_ovl0D_01_80047040(D_ovl0D_01_8004775C[i]) != 0) {
            func_ovl0D_01_80043CF8(D_ovl0D_01_800476EC[i].unk0);
            func_ovl0D_01_80046C70(D_ovl0D_01_800476EC[i].unk0, 0x02000000);
            func_ovl0D_01_800444E0(&D_ovl0D_01_800476EC[i]);
        }
    }
}

static void func_ovl0D_01_80045588(void) {
    s32 i;

    for (i = 0; i < 1; i++) {
        func_ovl0D_01_800434BC(&D_ovl0D_01_80047764[i]);
        func_ovl0D_01_80046C70(D_ovl0D_01_80047764[i].unk0, 0x80000);
    }
}

static void func_ovl0D_01_80045580(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        func_ovl0D_01_800434BC(&D_ovl0D_01_8004779C[i]);
        func_ovl0D_01_80046C70(D_ovl0D_01_8004779C[i].unk0, 0x80000);
    }
}

static void func_ovl0D_01_800454F0(void) {
}

void func_ovl0D_01_800454A8(void) {
    func_ovl0D_01_80046420(0x2D, 5, 2);
    func_ovl0D_01_80046420(0xF, 5, 0x16);
    func_ovl0D_01_80046420(0x12, 3, 2);
}

void func_ovl0D_01_80045480(void) {
    func_ovl0D_01_80046420(0x2D, 5, 0);
}

void func_ovl0D_01_80045458(void) {
    func_ovl0D_01_80046420(0x26, 5, 1);
}

void func_ovl0D_01_80045430(void) {
    func_ovl0D_01_80046420(0x26, 1, 1);
}

void func_ovl0D_01_80045408(void) {
    func_ovl0D_01_80046420(0xD, 3, 2);
}

void func_ovl0D_01_800452B8(void) {
    func_ovl0D_01_800434BC(D_ovl0D_01_8004759C);
    func_ovl0D_01_80046C70(D_ovl0D_01_8004759C[0].unk0, 0x80000);
    func_ovl0D_01_800434BC(D_ovl0D_01_800475D4);
    func_ovl0D_01_80043DDC();
    func_ovl0D_01_80043D3C();
    func_ovl0D_01_8004363C();
    func_ovl0D_01_80046478();
    if (!(D_ovl0D_01_80047B2C->unk0 & 2)) {
        switch (func_ovl0D_01_80046368()) {
            case 16:
            case 17:
                func_ovl0D_01_80046AC8(0.0f, 32768.0f, 32768.0f);
                break;
            case 18:
            case 19:
                func_ovl0D_01_80045270(-90.0f);
                break;
            case 26:
                func_ovl0D_01_80047340(0x6A);
                break;
        }
    }
    func_ovl0D_01_800470E0(0, 0x72, 0x37);
    func_ovl0D_01_800451F4();
    func_ovl0D_01_80045164();
}

void func_ovl0D_01_800451F4(void) {
    if (D_ovl0D_01_80047B2C->unk0 & 2) {
        func_ovl0D_01_80046AC8(45.0f, 32768.0f, 32768.0f);
        func_ovl0D_01_80046DB8(func_ovl0D_01_80045C60, NULL, 0, 0, 0x130);
        func_ovl0D_01_80046B10(0xF);
        func_ovl0D_01_80046B20(2);
    }
}

void func_ovl0D_01_80045164(void) {
    if (D_ovl0D_01_80047B2C->unk0 & 4) {
        func_ovl0D_01_80046AC8(45.0f, 32768.0f, 32768.0f);
        func_ovl0D_01_80046DB8(func_ovl0D_01_80045BB4, NULL, 0, 0, 0x130);
        func_ovl0D_01_80046B10(0xF);
        func_ovl0D_01_80046B20(1);
        func_ovl0D_01_800470B8(0, 0, 0, 0.0f);
    }
}

void func_ovl0D_01_800450C0(void) {
    switch (func_ovl0D_01_80046368()) {
        case 18:
            func_ovl0D_01_80046468(0x80, 3, 8.5f, 3, 0, 1, 1);
            func_ovl0D_01_80046458(0);
            break;
        case 19:
            func_ovl0D_01_800463F0(0x38, 0x14);
            break;
        case 26:
            func_ovl0D_01_80046DD8(30);
            func_ovl0D_01_800463F0(0x38, 0x1B);
            break;
    }
}

void func_ovl0D_01_80045090(void) {
    func_ovl0D_01_80045088();
    if (D_ovl0D_01_80047AF4 == 0x39) {
        func_ovl0D_01_80046458(1);
    }
}

static void func_ovl0D_01_80045088(void) {
}

void func_ovl0D_01_80044EC4(void) {
    if (D_ovl0D_01_80047B2C->unk0 & 2) {
        func_ovl0D_01_800474C0();
    }
}

void func_ovl0D_01_80044E2C(void) {
    UnkStruct_802B03C0 *r;

    if (D_ovl0D_01_80047B2C->unk0 & 4) {
        PORT_ONLY(if (func_ovl0D_01_80047508(0)->unkC != NULL)) func_ovl0D_01_80047508(0)->unkC->unkC |= 1; /* NULL: a demo left early (docs/PORT.md, "Reads through NULL") */
        func_ovl0D_01_80047108(0, 0, 0);
        func_ovl0D_01_800474F8();
        r = func_ovl0D_01_80047508(4);
        if (r->unkC != NULL) {
            func_ovl0D_01_80046950(r->unkC);
        }
        r->unkC = NULL;
        func_ovl0D_01_80047528();
    }
}

void func_ovl0D_01_80044F00(void) {
    func_ovl0D_01_80044EC4();
    func_ovl0D_01_80044E2C();
    func_ovl0D_01_80044E24();
    func_ovl0D_01_80044E1C();
    func_ovl0D_01_80044E14();
    func_ovl0D_01_80044E0C();
    func_ovl0D_01_80044E04();
    func_ovl0D_01_800436C8(D_ovl0D_01_8004759C);
    func_ovl0D_01_80043744();
}

static void func_ovl0D_01_80044E24(void) {
    UnkStruct_ovl0C_01_8004A54C *p;

    func_ovl0D_01_80046C80(D_ovl0D_01_80047B0C);
    for (p = D_ovl0D_01_8004760C; p < D_ovl0D_01_8004760C + 2; p++) {
        if (p->unk0 != NULL) {
            func_ovl0D_01_800436C8(p);
        }
    }
}

static void func_ovl0D_01_80044E1C(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (D_ovl0D_01_8004767C[i].unk0 != NULL) {
            func_ovl0D_01_800436C8(&D_ovl0D_01_8004767C[i]);
        }
    }
}

static void func_ovl0D_01_80044E14(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (D_ovl0D_01_800476EC[i].unk0 != NULL) {
            func_ovl0D_01_800436C8(&D_ovl0D_01_800476EC[i]);
        }
    }
}

static void func_ovl0D_01_80044E0C(void) {
    s32 i;

    for (i = 0; i < 1; i++) {
        if (D_ovl0D_01_80047764[i].unk0 != NULL) {
            func_ovl0D_01_800436C8(&D_ovl0D_01_80047764[i]);
        }
    }
}

static void func_ovl0D_01_80044E04(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (D_ovl0D_01_8004779C[i].unk0 != NULL) {
            func_ovl0D_01_800436C8(&D_ovl0D_01_8004779C[i]);
        }
    }
}

void func_ovl0D_01_80044D74(void) {
    func_ovl0D_01_80044D6C();
    if (func_ovl0D_01_80046430(0x2C, 0, 0x2E, 0) != 0) {
        D_ovl0D_01_80047AF4 = 0x39;
        D_ovl0D_01_80047AF8 = 0x11;
        func_ovl0D_01_80046360(0x39, 0x11, 0);
    }
    func_ovl0D_01_80044D64();
    if (func_ovl0D_01_80046430(7, 0x1A, 7, 0x1D) != 0) {
        func_ovl0D_01_80047218(D_ovl0D_01_80047AF0);
        func_ovl0D_01_80044428();
        func_ovl0D_01_80046388();
    }
}

static void func_ovl0D_01_80044D6C(void) {
}


static void func_ovl0D_01_80044D64(void) {
}

void func_ovl0D_01_80044CB0(void) {
    if (func_ovl0D_01_80046430(0x11, 0x16, 0x11, 0x17) != 0 || func_ovl0D_01_80046430(0x13, 0x19, 0x19, 0x19) != 0) {
        func_ovl0D_01_80046C68(D_ovl0D_01_8004759C[0].unk0, 0x01000000);
        func_ovl0D_01_80046B60(70.0f, 45.0f);
        func_ovl0D_01_80046AC8(32768.0f, 45.0f, 32768.0f);
        D_ovl0D_01_80047B30.z = 150.0f;
        func_ovl0D_01_80046B48(&D_ovl0D_01_80047B30.x);
    }
    func_ovl0D_01_800443BC();
    func_ovl0D_01_800439CC();
}

void func_ovl0D_01_80044C0C(void) {
    if (func_ovl0D_01_80046430(0x12, 0x16, 0x12, 0x17) != 0 || func_ovl0D_01_80046430(0x13, 0x17, 0x19, 0x17) != 0) {
        func_ovl0D_01_80046C70(D_ovl0D_01_8004759C[0].unk0, 0x01000000);
        func_ovl0D_01_80046B60(85.0f, 84.0f);
        func_ovl0D_01_80046AC8(32768.0f, 84.0f, 32768.0f);
        D_ovl0D_01_80047B30.z = 1.0f;
        func_ovl0D_01_80046B48(&D_ovl0D_01_80047B30.x);
    }
}

/* the gates: each opens when something hits it */
void func_ovl0D_01_800448D4(void) {
    s32 i;

    func_ovl0D_01_800448CC();
    func_ovl0D_01_800448C4();
    for (i = 0; i < 2; i++) {
        func_ovl0D_01_800448BC(&D_ovl0D_01_800476EC[i], D_ovl0D_01_8004775C[i]);
    }
}

static void func_ovl0D_01_800448C4(void) {
    func_ovl0D_01_80047030(0x424);
}

static void func_ovl0D_01_800448BC(UnkStruct_ovl0C_01_8004A54C *e, s32 flag) {
    Vec3f dir;
    Vec3f pos;
    s32 r;

    if (flag == 0x400 && func_ovl0D_01_80047040(0x407) == 0) {
        return;
    }
    if (!(e->unk0->unk8 & 0x04000000)) {
        D_ovl0D_01_80047B0C->unk30 = 1;
        D_ovl0D_01_80047B0C->unk34 = 75.0f;
        D_ovl0D_01_80047B0C->unk38 = 50.0f;
        dir.x = e->unk0->unk40->unk20 + 90.0f;
        dir.y = 0.0f;
        dir.z = 100.0f;
        func_ovl0D_01_80046C00(&dir.x, &pos.x);
        D_ovl0D_01_80047B0C->unk40->unk10 = e->unk0->unk40->unk10 + pos.x;
        D_ovl0D_01_80047B0C->unk40->unk14 = e->unk0->unk40->unk14 + pos.y;
        D_ovl0D_01_80047B0C->unk40->unk18 = e->unk0->unk40->unk18 + pos.z;
        r = func_ovl0D_01_80046C18(D_ovl0D_01_80047B0C, 0x40);
        if (r != 0) {
            if ((r & 0x6000) && func_ovl0D_01_80047040(flag) == 0 &&
                !(e->unk0->unk8 & 0x04000000)) {
                func_ovl0D_01_80046C68(e->unk0, 0x04000000);
                func_ovl0D_01_80046C70(e->unk0, 0x02000000);
                func_ovl0D_01_80047360(0x60, D_ovl0D_01_80047B0C->unk40->unk10, D_ovl0D_01_80047B0C->unk40->unk14, D_ovl0D_01_80047B0C->unk40->unk18);
            } else if (func_ovl0D_01_80047040(flag) == 0) {
                func_ovl0D_01_80047360(0x26, D_ovl0D_01_80047B0C->unk40->unk10, D_ovl0D_01_80047B0C->unk40->unk14, D_ovl0D_01_80047B0C->unk40->unk18);
            }
        }
    } else if ((e->unk0->unk40->unkC & 0x40) && func_ovl0D_01_80047040(flag) == 0) {
        func_ovl0D_01_800444E0(e);
        func_ovl0D_01_80047030(flag);
        func_ovl0D_01_80047340(0x68);
    }
}

static s32 func_ovl0D_01_800444D8(UnkStruct_ovl0C_01_8004A54C *e) {
    if (e->unk0->unk40->unk20 == 0.0f) {
        return 0;
    } else if (e->unk0->unk40->unk20 == 90.0f) {
        return 1;
    } else if (e->unk0->unk40->unk20 == 180.0f) {
        return 2;
    } else if (e->unk0->unk40->unk20 == 270.0f || e->unk0->unk40->unk20 == -90.0f) {
        return 3;
    }
    return -1;
}

static void func_ovl0D_01_80044464(s32 axis, s32 *v, s32 *size) {
    s32 max;

    switch (axis) {
        case 0:
            max = size[0];
            break;
        case 1:
            max = size[1];
            break;
        case 2:
            max = size[2];
            break;
    }
    if (*v < 0) {
        *v = 0;
    } else if (*v >= max) {
        *v = max - 1;
    }
}

void func_ovl0D_01_80044428(void) {
    s32 *p;

    p = func_ovl0D_01_80047050(0x19);
    if (*p <= 0 || *p == 0) {
        *p = 1;
    }
}

/* a global wrapper around the static (docs/DECOMPILING.md, ovl_0D_03's 80043770) */
void func_ovl0D_01_80044408(void) {
    func_ovl0D_01_80044154();
}

void func_ovl0D_01_800443BC(void) {
    if (func_ovl0D_01_80046430(0xD, 2, 0xD, 2) != 0 && func_ovl0D_01_80047040(0x408) == 0) {
        func_ovl0D_01_80044038();
    }
}

void func_ovl0D_01_80044154(void) {
    f32 y;

    D_ovl0D_01_80047B24 = func_ovl0D_01_80046338(0x155);
    D_ovl0D_01_80047B28 = func_ovl0D_01_80046338(0x154);
    if (func_ovl0D_01_80047040(0x407) != 0) {
        func_ovl0D_01_80046C70(D_ovl0D_01_8004760C[0].unk0, 0x01000000);
        y = 15.0f;
        /*
         * fake: D_ovl0D_01_8004760C[1] reached from another symbol, so
         * uopt doesn't keep &[1] in a register for both
         */
        D_ovl0D_01_8004760C[1].unk18 = y;
        PORT_HOST(D_ovl0D_01_800475D4[2], D_ovl0D_01_8004760C[1]).unk0->unk40->unk14 = y * 10.0f;
        func_ovl0D_01_80046C70(D_ovl0D_01_8004767C[0].unk0, 0x01000000);
        func_ovl0D_01_80046C70(D_ovl0D_01_8004767C[1].unk0, 0x01000000);
        func_ovl0D_01_80046340(D_ovl0D_01_80047B24, 0, 0, D_ovl0D_01_80047B18[0] - 1, D_ovl0D_01_80047B18[2] - 1, 0, 1, 0);
        func_ovl0D_01_800462B0(0xD, 0, 4, 0xD, 0, 9, 0x11);
        func_ovl0D_01_800462B0(0xE, 0, 4, 0xE, 1, 9, 0x11);
        func_ovl0D_01_800462B0(0xE, 0, 4, 0xE, 0, 9, 0x1F);
    }
    if (func_ovl0D_01_80047040(0x408) != 0) {
        func_ovl0D_01_80046C70(D_ovl0D_01_8004760C[0].unk0, 0x01000000);
        func_ovl0D_01_80046C70(D_ovl0D_01_8004760C[1].unk0, 0x01000000);
        func_ovl0D_01_80046C70(D_ovl0D_01_8004767C[0].unk0, 0x01000000);
        func_ovl0D_01_80046C70(D_ovl0D_01_8004767C[1].unk0, 0x01000000);
        func_ovl0D_01_80046990(D_ovl0D_01_80047764[0].unk0->unk40, 1, 0);
        func_ovl0D_01_80043CF8(D_ovl0D_01_80047764[0].unk0);
        func_ovl0D_01_80043CF8(D_ovl0D_01_8004779C[0].unk0);
        func_ovl0D_01_80046340(D_ovl0D_01_80047B28, 0, 0, D_ovl0D_01_80047B18[0] - 1, D_ovl0D_01_80047B18[2] - 1, 0, 0, 0);
        func_ovl0D_01_80043944(&D_ovl0D_01_8004779C[0]);
        func_ovl0D_01_80043E90();
        if (func_ovl0D_01_80046368() == 26) {
            func_ovl0D_01_80046BD0();
        }
        func_ovl0D_01_80046B98(&D_ovl0D_01_80047AD0);
    }
}

/* the first gate opens */
void func_ovl0D_01_80044038(void) {
    func_ovl0D_01_80043F04(1300.0f, 600.0f, 400.0f, 270.0f, 0xC0);
    func_ovl0D_01_80046990(D_ovl0D_01_80047764[0].unk0->unk40, 1, 0);
    func_ovl0D_01_80046C68(D_ovl0D_01_80047764[0].unk0, 0x04000000);
    func_ovl0D_01_80046C68(D_ovl0D_01_8004779C[0].unk0, 0x04000000);
    func_ovl0D_01_80046C68(D_ovl0D_01_8004760C[1].unk0, 0x04000000);
    func_ovl0D_01_80046990(D_ovl0D_01_8004767C[1].unk0->unk40, 0, 0);
    func_ovl0D_01_80046C68(D_ovl0D_01_8004767C[1].unk0, 0x04000000);
    func_ovl0D_01_80046340(D_ovl0D_01_80047B28, 0, 0, D_ovl0D_01_80047B18[0] - 1, D_ovl0D_01_80047B18[2] - 1, 0, 0, 0);
    func_ovl0D_01_80043944(&D_ovl0D_01_8004779C[0]);
    func_ovl0D_01_80043E90();
    func_ovl0D_01_80047030(0x408);
    func_ovl0D_01_80043E1C(0x38, 0x1A);
}

/* a gate opening: the camera goes to it */
void func_ovl0D_01_80043F04(f32 x, f32 y, f32 z, f32 ang, s32 arg4) {
    func_ovl0D_01_80043EFC();
    func_ovl0D_01_800463D0(0);
    func_ovl0D_01_80046458(1);
    func_ovl0D_01_80046B10(0xF);
    if (func_ovl0D_01_80046AF8() == 1) {
        func_ovl0D_01_80046B00();
    }
    func_ovl0D_01_80046518();
    func_ovl0D_01_80047248();
    func_ovl0D_01_80047328();
    func_ovl0D_01_80046DD8(60);
    func_ovl0D_01_80046480(1);
    func_ovl0D_01_80047340(0x29);
    func_ovl0D_01_800463D8(1);
    func_ovl0D_01_80046490(0x40);
    func_ovl0D_01_80046468(arg4, 6, 8.5f, 0, 1, 1, 0);
    func_ovl0D_01_80046458(1);
    func_ovl0D_01_80046DD8(30);
    func_ovl0D_01_80046BC8();
    func_ovl0D_01_80046B10(0x7F);
    func_ovl0D_01_80046AA8(x, y, z);
    func_ovl0D_01_80046AC8(ang, 45.0f, 32768.0f);
    func_ovl0D_01_80046B20(0);
    func_ovl0D_01_80046DD8(15);
    func_ovl0D_01_80047340(0x69);
    func_ovl0D_01_80046DD8(15);
    func_ovl0D_01_80047340(0x6F);
}

static void func_ovl0D_01_80043EFC(void) {
}

void func_ovl0D_01_80043E90(void) {
    func_ovl0D_01_800462B0(0xE, 0, 4, 0xE, 0, 9, 0x11);
    func_ovl0D_01_800462B0(0xE, 1, 4, 0xE, 1, 9, 0x10);
}

void func_ovl0D_01_80043E1C(s32 arg0, s32 arg1) {
    func_ovl0D_01_80046468(0x200, 5, 1.1111f, 1, 0, 0, 0);
    func_ovl0D_01_80046458(1);
    func_ovl0D_01_80046DD8(30);
    func_ovl0D_01_800470A8(2);
    func_ovl0D_01_800470B0();
    func_ovl0D_01_80046360(arg0, arg1, 1);
}

void func_ovl0D_01_80043DDC(void) {
    func_ovl0D_01_800434BC(D_ovl0D_01_8004780C);
    func_ovl0D_01_80046C70(D_ovl0D_01_8004780C[0].unk0, 0x80000);
}

void func_ovl0D_01_80043D3C(void) {
    if (func_ovl0D_01_80047040(0x406) != 0) {
        func_ovl0D_01_800437C8(0x26, 7, 2, 0x1B);
        func_ovl0D_01_80043CF8(D_ovl0D_01_8004779C[1].unk0);
        func_ovl0D_01_80043944(&D_ovl0D_01_8004779C[1]);
        func_ovl0D_01_80043CF8(D_ovl0D_01_8004780C[0].unk0);
        func_ovl0D_01_800438C0();
    }
}

static void func_ovl0D_01_80043CF8(UnkStructA *o) {
    o->unk40->unk48->unk4->unk4 = func_ovl0D_01_800469A8(o->unk40, 0) - 0.001f;
}

void func_ovl0D_01_800438C0(void) {
    func_ovl0D_01_800462A8(8, 2, 0x1A, 0xC, 0x1D, 0x11);
    func_ovl0D_01_800462A8(8, 3, 0x1A, 0xC, 0x1D, 0x10);
    func_ovl0D_01_800462A8(8, 4, 0x1A, 0xC, 0x1D, 0x10);
}

/* the second gate's switch: the camera shows the gate opening */
void func_ovl0D_01_800439CC(void) {
    if (func_ovl0D_01_80047040(0x406) == 0) {
        if (func_ovl0D_01_80046430(0x14, 0x13, 0x14, 0x13) != 0) {
            func_ovl0D_01_800463D0(0);
            func_ovl0D_01_80046458(1);
            func_ovl0D_01_80046B10(0xF);
            if (func_ovl0D_01_80046AF8() == 1) {
                func_ovl0D_01_80046B00();
            }
            func_ovl0D_01_80046518();
            func_ovl0D_01_80047248();
            func_ovl0D_01_80047328();
            func_ovl0D_01_80046DD8(60);
            func_ovl0D_01_800463D8(1);
            func_ovl0D_01_80046480(1);
            func_ovl0D_01_80047340(0x29);
            func_ovl0D_01_80046468(0x80, 6, 8.5f, 0, 1, 1, 0);
            func_ovl0D_01_80046458(1);
            func_ovl0D_01_80046DD8(30);
            func_ovl0D_01_80046BC8();
            func_ovl0D_01_80046B10(0x7F);
            func_ovl0D_01_80046AA8(2000.0f, 700.0f, 1800.0f);
            func_ovl0D_01_80046AC8(45.0f, 45.0f, 800.0f);
            func_ovl0D_01_80046DD8(30);
            func_ovl0D_01_80046C68(D_ovl0D_01_8004759C[0].unk0, 0x01000000);
            func_ovl0D_01_80047340(0x6F);
            func_ovl0D_01_80046C68(D_ovl0D_01_8004779C[1].unk0, 0x04000000);
            func_ovl0D_01_80043944(&D_ovl0D_01_8004779C[1]);
            func_ovl0D_01_80046468(0x200, 5, 1.1111f, 1, 0, 0, 0);
            func_ovl0D_01_80046458(1);
            func_ovl0D_01_80043868(D_ovl0D_01_8004779C[1].unk0);
            func_ovl0D_01_80047340(0x6B);
            func_ovl0D_01_80046C68(D_ovl0D_01_8004780C[0].unk0, 0x04000000);
            func_ovl0D_01_80046C70(D_ovl0D_01_8004780C[0].unk0, 0x02000000);
            func_ovl0D_01_800438C0();
            func_ovl0D_01_80046AA8(1000.0f, 750.0f, 2500.0f);
            func_ovl0D_01_80046AC8(315.0f, 30.0f, 3500.0f);
            func_ovl0D_01_80043868(D_ovl0D_01_8004780C[0].unk0);
            func_ovl0D_01_80047370(0x6B);
            func_ovl0D_01_80047340(0x6C);
            func_ovl0D_01_80046B90(10.0f, 10.0f, 10.0f);
            func_ovl0D_01_80046B80(60);
            func_ovl0D_01_80046DD8(60);
            func_ovl0D_01_80047340(0x2C);
            func_ovl0D_01_800437C8(0x26, 7, 2, 0x1B);
            func_ovl0D_01_8004376C(315.0f, 45.0f, 1500.0f);
            func_ovl0D_01_80046C70(D_ovl0D_01_8004759C[0].unk0, 0x01000000);
            func_ovl0D_01_80046E00();
            func_ovl0D_01_80046BD0();
            func_ovl0D_01_80046B98(&D_ovl0D_01_80047AD0);
            func_ovl0D_01_80046B08(0x1F);
            func_ovl0D_01_80047030(0x406);
            func_ovl0D_01_800463D8(0);
            func_ovl0D_01_800463D0(1);
            func_ovl0D_01_80046458(0);
        }
    }
}

static void func_ovl0D_01_80043944(UnkStruct_ovl0C_01_8004A54C *e) {
    s32 pos[6];
    UnkStructA_40 *p;

    p = e->unk0->unk40;
    func_ovl0D_01_800462C0(p->unk10, p->unk14, p->unk18, pos, 0);
    func_ovl0D_01_800462A8(pos[0], pos[1], pos[2], pos[0], pos[2], 0x11);
    func_ovl0D_01_800462A8(pos[0], pos[1] + 1, pos[2], pos[0], pos[2], 0x10);
}

/* waits for the object's animation to end */
static void func_ovl0D_01_80043868(UnkStructA *o) {
    while (!(o->unk40->unkC & 0x40)) {
        func_ovl0D_01_80046E00();
    }
}

/* the camera: x tiles across, above the n rows' heights, z tiles down */
static void func_ovl0D_01_800437C8(s32 id, s32 x, s32 n, s32 z) {
    s32 i;

    D_ovl0D_01_80047B00.x = x * 100.0f;
    D_ovl0D_01_80047B00.y = 0.0f;
    for (i = 0; i < n; i++) {
        D_ovl0D_01_80047B00.y += func_ovl0D_01_800462D0(i) * 100.0f;
    }
    D_ovl0D_01_80047B00.y += 100.0f;
    D_ovl0D_01_80047B00.z = z * 100.0f;
    D_ovl0D_01_80047AF0 = func_ovl0D_01_80047210(id, &D_ovl0D_01_80047B00.x, 0);
}




void func_ovl0D_01_80043744(void) {
    func_ovl0D_01_800436C8(D_ovl0D_01_8004780C);
}

static void func_ovl0D_01_800436C8(UnkStruct_ovl0C_01_8004A54C *arg0) {
    func_ovl0D_01_80046C80(arg0->unk0);
    if (arg0->unkC != 0xFFFF && arg0->unkE != 0xFFFF && arg0->unk10 != 0xFFFF && arg0->unk12 != 0xFFFF) {
        func_ovl0D_01_800469B8(arg0->unk4);
    }
    arg0->unk0 = NULL;
    arg0->unk4 = -1;
}

/* the exit's model: it fades out (the task) unless it already has */
void func_ovl0D_01_8004363C(void) {
    func_ovl0D_01_800434BC(D_ovl0D_01_8004788C);
    func_ovl0D_01_80046C70(D_ovl0D_01_8004788C[0].unk0, 0x02080000);
    if (func_ovl0D_01_80047040(0x425) != 0) {
        func_ovl0D_01_80046C70(D_ovl0D_01_8004788C[0].unk0, 0x01000000);
        func_ovl0D_01_80043098();
    } else {
        func_ovl0D_01_80046DB8(func_ovl0D_01_80043134, NULL, 0, 0, 0x130);
    }
}

static void func_ovl0D_01_800434BC(UnkStruct_ovl0C_01_8004A54C *arg0) {
    arg0->unk0 = func_ovl0D_01_80046CB0(arg0->unk8, 0xD0, 0.0f, 0.0f, 0.0f);
    func_ovl0D_01_80046C68(arg0->unk0, 0x01080000);
    func_ovl0D_01_80046C70(arg0->unk0, 0x02000000);
    if (arg0->unkC != -1 && arg0->unkE != -1 && arg0->unk10 != -1 && arg0->unk12 != -1) {
        arg0->unk4 = func_ovl0D_01_800469B0(arg0->unk0->unk40, 0, arg0->unkC, arg0->unkE, arg0->unk10, arg0->unk12);
        if (arg0->unk4 == -1) {
            while (1) {
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

/* a task: waits for something to hit the exit, then fades its model out */
void func_ovl0D_01_80043134(void) {
    s32 i;
    s32 n;
    f32 frames; /* fake: the fade's length held: a guess (any float held here fills its word, which puts pos at sp+96) */
    Vec3f pos;

    while (TRUE) {
        D_ovl0D_01_80047B0C->unk30 = 1;
        D_ovl0D_01_80047B0C->unk34 = 75.0f;
        D_ovl0D_01_80047B0C->unk38 = 50.0f;
        D_ovl0D_01_80047B0C->unk40->unk10 = 2000.0f;
        D_ovl0D_01_80047B0C->unk40->unk14 = 400.0f;
        D_ovl0D_01_80047B0C->unk40->unk18 = 2300.0f;
        D_ovl0D_01_80047B0C->unk14 = 0;
        if (func_ovl0D_01_80046C20(D_ovl0D_01_80047B0C, 0x4040) != 0) {
            break;
        }
        if (func_ovl0D_01_80046C20(D_ovl0D_01_80047B0C, 0x40) != 0) {
            func_ovl0D_01_80043100(0x26, D_ovl0D_01_80047B0C);
        }
        func_ovl0D_01_80046DD8(5);
        D_ovl0D_01_80047B0C->unk30 = 1;
        D_ovl0D_01_80047B0C->unk34 = 75.0f;
        D_ovl0D_01_80047B0C->unk38 = 50.0f;
        D_ovl0D_01_80047B0C->unk40->unk10 = 2300; /* an int: its own .rodata 2300.0f */
        D_ovl0D_01_80047B0C->unk40->unk14 = 400.0f;
        D_ovl0D_01_80047B0C->unk40->unk18 = 2300.0f;
        D_ovl0D_01_80047B0C->unk14 = 0;
        if (func_ovl0D_01_80046C20(D_ovl0D_01_80047B0C, 0x4040) != 0) {
            break;
        }
        if (func_ovl0D_01_80046C20(D_ovl0D_01_80047B0C, 0x40) != 0) {
            func_ovl0D_01_80043100(0x26, D_ovl0D_01_80047B0C);
        }
        func_ovl0D_01_80046DD8(5);
    }
    func_ovl0D_01_80047360(0x63, 2150.0f, 800.0f, 2400.0f);
    func_ovl0D_01_80046C68(D_ovl0D_01_8004788C[0].unk0, 0x80000);
    D_ovl0D_01_8004788C[0].unk0->unk40->unk36 = 0xFF;
    n = 10;
    frames = 120.0f;
    for (i = 0; i < 120; i++) {
        n++;
        D_ovl0D_01_8004788C[0].unk0->unk40->unk36 = (1.0f - i / frames) * 255.0f;
        if (n >= 11) {
            pos.x = func_ovl0D_01_80046F70() * 600.0 + 1900.0;
            pos.y = func_ovl0D_01_80046F70() * 500.0 + 500.0;
            pos.z = 2500.0f;
            func_ovl0D_01_80047210(0x14, &pos.x, 0);
            n = 0;
        }
        func_ovl0D_01_80046E00();
    }
    func_ovl0D_01_80046C70(D_ovl0D_01_8004788C[0].unk0, 0x01000000);
    func_ovl0D_01_80043098();
    func_ovl0D_01_80047030(0x425);
    func_ovl0D_01_80046DC8();
}








void func_ovl0D_01_80043098(void) {
    func_ovl0D_01_800462B0(0x13, 3, 0x18, 0x19, 5, 0x18, 0x10);
    func_ovl0D_01_800462A8(0x13, 2, 0x18, 0x19, 0x18, 0x11);
}

void func_ovl0D_01_80043010(void) {
    if (func_ovl0D_01_80046438(0xE, 3, 0xA, 0xE, 0xA) != 0 || func_ovl0D_01_80046438(0xD, 3, 0xB, 0xD, 0xB) != 0) {
        if (func_ovl0D_01_80047020(0xA4) != 0) {
            func_ovl0D_01_800469D0(0x291, 1);
        } else {
            func_ovl0D_01_800469D0(0x290, 2);
        }
    }
}
