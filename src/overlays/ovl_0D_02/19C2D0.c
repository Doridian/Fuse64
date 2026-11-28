/*
 * ovl_0D_02's stage object, IDO -O3; it matches in us, eu and jp.
 *
 * The sibling of ovl_0D_04/05/06 (src/ and nonmatching/): the gate code
 * (80043510, 8004311C) is 0D_06's, the models' tasks 0D_05's.
 * What fixed what (from 98 words):
 * - 8004311C's second and fourth 80046338 calls pass 0x12 (the draft had
 *   0x11; all three ROMs have 0x12).
 * - 80043010: D_ovl0D_02_8004762C split in two, [1] being its own symbol
 *   D_ovl0D_02_80047664 (the store to unk18 still through 8004762C[1]):
 *   three symbols, so uopt doesn't keep &8004762C in v0 and each access
 *   gets its own lui/%lo, as in the original.
 * - 80043114 (the direction) takes the entity and compares
 *   e->obj->unk40->unk20 itself: `c.eq.s const, a` as the original (with
 *   an f32 parameter it was `c.eq.s a, const`); 8004311C's `dir` local
 *   dropped (switch on the call) to keep the frame.
 * - 800430A0 (the clamp) is (axis, v, size): IPA still gives size a1 and
 *   v a2, but ugen sets a2 up before a1, so `move a1,a3` lands in each
 *   call's delay slot (hand-edited ugen output through as0/as1 showed that
 *   only that order gives the original).
 * - 8004311C: `len = x1 + z1` (dead: the switch sets len) ahead of x0's
 *   set-up, a fake. x1, z1 and z0 weigh the same to uopt's register
 *   allocator (case 1's z2 = z0 + 1, as the original), and a tie goes to
 *   the variable it met first: z0's set-up came first, giving x0 s0, z0
 *   s1, x1 s2, z1 s3 (41 words). Reading x1 and z1 before it gives the
 *   original's x0 s0, x1 s1, z1 s2, z0 s3. (Moving z0's set-up after case
 *   0's x1 and z1, or making its store volatile, shows the same order.)
 *   Tried in pass 10: x1/z1 (and len) with dead initialisers or `x1 = z1
 *   = 0` (the homed locals keep the stores: 3400 words), `len = 0` (41);
 *   a compiled-out check `if (x1 + z1 == 0)` matches but tests unset
 *   values, so the plain read stays.
 * - 8004596C: the word between tgt and timer is idx + 1 held in n for
 *   the timer's division (a guess: any held value there matches).
 * - 800444FC: the probe static 800444EC is defined in another file
 *   (statics.h; its store stays ahead of the next call), and `#line 1
 *   statics.h` regions (as in ovl_0D_07): around
 *   the 80044474 call (the store of D_ovl0D_02_80047A54 is lui/%lo and the
 *   call's delay slot stays empty; it replaces the `if ((D = f())) {}`
 *   fake), and around the gate's store in 80044444 (the arguments of the
 *   calls around it go into their delay slots). 8004421C takes its first
 *   row j as a parameter (0 from 8004422C, whose line comes after the
 *   loop's; a guess for what was a `j = 0` at `#line 3000`): that puts
 *   `move v0,a1` ahead of `move a0,zero`. In 8004311C, case 0's first statement in a statics.h
 *   region: as1 then computes z1 right after x1 and stores z2 in order,
 *   with x0's home store in the branch's delay slot, as the original (no
 *   order of the case's ugen instructions fed to as1 by hand did that; a
 *   .file at the block's start did).
 * - 800438FC and 80043510: the statics they inline first, 800438D8 and
 *   80043508, are defined in another file (statics.h): the `.file` keeps
 *   the first call out of the prologue (they replace two
 *   `if (!D && !D && !D) {}` fakes).
 * - 80045964: grid[7] (not [6]) puts the timer static's grid at sp+168.
 */
#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_0D_02's stage object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse; docs/DECOMPILING.md).
 * ovl_0D_04, 0D_05 and 0D_06 (src/ and nonmatching/) are its siblings.
 */

void func_ovl0D_02_800462F8(s32); /* 0xC00 */
void func_ovl0D_02_80046308(s32 *); /* 0xC02 */
void func_ovl0D_02_80046338(s32, s32, s32, s32, s32, s32); /* 0xD05 */
void func_ovl0D_02_80046340(s32, s32, s32, s32, s32, s32, s32); /* 0xD06 */
void func_ovl0D_02_80046350(f32, f32, f32, s32 *, s32); /* 0xD08 */
void func_ovl0D_02_80046358(f32, f32, f32, s32 *, s32); /* 0xD09 */
s32 func_ovl0D_02_80046368(s32, s32, s32); /* 0xD0B */
void func_ovl0D_02_80046370(s32); /* 0xD0C */
void func_ovl0D_02_80046378(void); /* 0xD0D */
void func_ovl0D_02_800463C0(s32); /* 0xD16 */
void func_ovl0D_02_800463E0(void (*)(UnkStructA *)); /* 0xE03 */
void func_ovl0D_02_800463F0(s32, s32, s32); /* 0xF00 */
s32 func_ovl0D_02_800463F8(void); /* 0xF01 */
void func_ovl0D_02_80046440(s32); /* 0xF0A */
UnkStruct_802AC610 *func_ovl0D_02_80046448(void); /* 0xF0B */
void func_ovl0D_02_80046460(s32); /* 0xF0E */
void func_ovl0D_02_800464A8(void); /* 0x1000 */
void func_ovl0D_02_800464B0(s32, s32, s32); /* 0x1001 */
void func_ovl0D_02_800464B8(void *); /* 0x1002 */
s32 func_ovl0D_02_800464C0(s32, s32, s32, s32); /* 0x1003 */
s32 func_ovl0D_02_800464D8(void); /* 0x1006 */
void func_ovl0D_02_800464E0(s32, void *); /* 0x1007 */
void func_ovl0D_02_800464E8(s32); /* 0x1008 */
void func_ovl0D_02_800464F0(s32); /* 0x1009 */
void func_ovl0D_02_800464F8(s32, s32, f32, s32, s32, s32, s32); /* 0x100A */
void func_ovl0D_02_80046500(const char *); /* 0x100B */
void func_ovl0D_02_80046508(void); /* 0x100C */
void func_ovl0D_02_80046590(s32); /* 0x101D */
void func_ovl0D_02_80046598(s32); /* 0x101E */
f32 func_ovl0D_02_80046A38(UnkStructA_40 *, s32); /* 0x150C */
s32 func_ovl0D_02_80046A40(UnkStructA_40 *, s32, s32, s32, s32, s32); /* 0x150D */
void func_ovl0D_02_80046A48(s32); /* 0x150E */
void func_ovl0D_02_80046B38(f32, f32, f32); /* 0x1704 */
void func_ovl0D_02_80046B58(f32, f32, f32); /* 0x1708 */
void func_ovl0D_02_80046B60(f32 *, f32 *, f32 *); /* 0x1709 */
void func_ovl0D_02_80046B98(s32); /* 0x1710 */
void func_ovl0D_02_80046BA0(s32); /* 0x1711 */
void func_ovl0D_02_80046BB0(s32); /* 0x1713 */
void func_ovl0D_02_80046BE8(f32, f32, f32, f32); /* 0x171A */
void func_ovl0D_02_80046BF8(f32, f32, f32, f32); /* 0x171C */
void func_ovl0D_02_80046C10(s32); /* 0x171F */
void func_ovl0D_02_80046C20(f32, f32, f32); /* 0x1721 */
void func_ovl0D_02_80046C68(f32, f32); /* 0x172A */
void func_ovl0D_02_80046C88(f32 *, f32 *, f32 *); /* 0x1803 */
void func_ovl0D_02_80046C90(f32 *, f32 *); /* 0x1804 */
s32 func_ovl0D_02_80046CA8(UnkStructA *, s32); /* 0x1807 */
void func_ovl0D_02_80046CC8(UnkStructA *, s32); /* 0x180B */
void func_ovl0D_02_80046CF8(UnkStructA *, s32); /* 0x1811 */
void func_ovl0D_02_80046D00(UnkStructA *, s32); /* 0x1812 */
void func_ovl0D_02_80046D10(UnkStructA *); /* 0x1814 */
UnkStructA *func_ovl0D_02_80046D18(s32, s32, s32, s32, PORT_IP(s32)); /* 0x1815 */
UnkStructA *func_ovl0D_02_80046D20(s32, s32, s32, PORT_IP(s32)); /* 0x1816 */
UnkStructA *func_ovl0D_02_80046D30(UnkStructA *); /* 0x1818 */
UnkStructA *func_ovl0D_02_80046D40(s32, s32, f32, f32, f32); /* 0x181A */
s32 func_ovl0D_02_80046D70(UnkStructA *, f32 *, f32 *, f32 *); /* 0x1820 */
void func_ovl0D_02_80046D78(UnkStructA *); /* 0x1821 */
void func_ovl0D_02_80046DC8(UnkStructA *, f32 *); /* 0x182B */
void func_ovl0D_02_80046DD0(void *, UnkStructA *); /* 0x182C */
void func_ovl0D_02_80046E48(void (*)(void), void *, PORT_IP(s32), s32, s32); /* 0x1900 */
void func_ovl0D_02_80046E58(void); /* 0x1902 */
void func_ovl0D_02_80046E68(s32); /* 0x1904 */
void *func_ovl0D_02_80046E70(void); /* 0x1905 */
void func_ovl0D_02_80046E90(void); /* 0x1909 */
f32 func_ovl0D_02_80046FF0(f32); /* 0x1D00 */
f64 func_ovl0D_02_80047000(void); /* 0x1D02 */
s32 func_ovl0D_02_80047008(s32); /* 0x1D03 */
void func_ovl0D_02_800470C0(s32); /* 0x1E06 */
void func_ovl0D_02_800470C8(s32); /* 0x1E07 */
s32 func_ovl0D_02_800470D0(s32); /* 0x1E08 */
void func_ovl0D_02_80047170(s32, s32, s32); /* 0x2101 */
void func_ovl0D_02_80047190(s32); /* 0x2200 */
void func_ovl0D_02_800471B0(u32 (*)[3]); /* 0x2300 */
void func_ovl0D_02_800471C8(u32 (*)[3]); /* 0x2303 */
void func_ovl0D_02_800472A0(s32, f32 *, UnkStructA *); /* 0x2601 */
void func_ovl0D_02_800472C8(s16, s32); /* 0x2702 */
void func_ovl0D_02_800472D0(s16, s32); /* 0x2703 */
void func_ovl0D_02_80047318(s32, f32, f32, f32); /* 0x2801 */
void func_ovl0D_02_80047358(s32, s32, s32); /* 0x2A00 */
void func_ovl0D_02_80047390(s32, s32, s32, s32); /* 0x2A07 */
void func_ovl0D_02_800473D0(s32); /* 0x2B00 */
s32 func_ovl0D_02_800473F0(s32, f32, f32, f32); /* 0x2B04 */
void func_ovl0D_02_800473F8(s32, f32 *); /* 0x2B05 */
void func_ovl0D_02_80047548(void); /* 0x3000 */
void func_ovl0D_02_80047550(void); /* 0x3001 */

void func_ovl0D_02_80043010(void);
void func_ovl0D_02_80043510(void);
void func_ovl0D_02_8004381C(void);
void func_ovl0D_02_800438FC(void);
void func_ovl0D_02_80043A70(void);
void func_ovl0D_02_80043ABC(void);
void func_ovl0D_02_80043BA4(void);
void func_ovl0D_02_80043C84(f32);
void func_ovl0D_02_80043CCC(void);
void func_ovl0D_02_80043D78(void);
void func_ovl0D_02_80043ECC(void);
void func_ovl0D_02_80043EF4(void);
void func_ovl0D_02_80043F1C(void);
void func_ovl0D_02_80043F44(void);
void func_ovl0D_02_80043F6C(void);
void func_ovl0D_02_80044474(void);
void func_ovl0D_02_800444D4(void);
void func_ovl0D_02_800444FC(void);
void func_ovl0D_02_80044A50(s32);
void func_ovl0D_02_80044B8C(void);
void func_ovl0D_02_80044CA0(UnkStructA *);
void func_ovl0D_02_80044CE4(UnkStructA *, s32, s32, f32, f32);
s32 func_ovl0D_02_80044DBC(s32, void *);
void func_ovl0D_02_80044DF0(void);
void func_ovl0D_02_80044EEC(void);
void func_ovl0D_02_80045274(void);
void func_ovl0D_02_800455A0(void);
void func_ovl0D_02_8004596C(void);
void func_ovl0D_02_80045F20(f32, f32, f32);

static void func_ovl0D_02_800430A0(s32, s32 *, s32 *);
static s32 func_ovl0D_02_80043114(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_02_8004311C(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_02_800434F8(UnkStruct_ovl0C_01_8004A54C *, s32);
static void func_ovl0D_02_80043500(void);
static void func_ovl0D_02_80043508(void);
static void func_ovl0D_02_80043858(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_02_800438D8(void);
static void func_ovl0D_02_800438E0(void);
static void func_ovl0D_02_800438E8(void);
static void func_ovl0D_02_800438F0(void);
static void func_ovl0D_02_800438F8(void);
static void func_ovl0D_02_80043A68(void);
static void func_ovl0D_02_80043AB4(void);
static void func_ovl0D_02_80043F94(s32);
static void func_ovl0D_02_80044094(s32);
static void func_ovl0D_02_8004409C(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_02_8004421C(Vec3f *, Vec3f (*)[4], s32);
static void func_ovl0D_02_80044224(void);
static void func_ovl0D_02_8004422C(void);
static void func_ovl0D_02_80044234(UnkStruct_ovl0C_01_8004A54C *);
static void func_ovl0D_02_800442D8(void);
static void func_ovl0D_02_80044444(void);
static void func_ovl0D_02_8004444C(void);
static void func_ovl0D_02_80044454(void);
static void func_ovl0D_02_8004445C(void);
static void func_ovl0D_02_80044464(void);
static void func_ovl0D_02_8004446C(void);
static void func_ovl0D_02_800444EC(void);
static void func_ovl0D_02_800444F4(Trio_0D01 *, s32);
static void func_ovl0D_02_8004595C(UnkStruct_ovl0C_01_8004A54C *, s32);
static void func_ovl0D_02_80045964(UnkStruct_ovl0C_01_8004A54C *, s32 *);
static void func_ovl0D_02_80045F80(void);
static void func_ovl0D_02_80045F88(void);
static void func_ovl0D_02_80045F90(void);
static void func_ovl0D_02_80045F98(void);
static void func_ovl0D_02_80045FA0(void);
static void func_ovl0D_02_80045FA8(void);

#define UNK_MACRO_ovl0D_02_80043510() \
    D_ovl0D_02_80047A1C = func_ovl0D_02_800464D8(); func_ovl0D_02_800464B8(&D_ovl0D_02_80047A10); func_ovl0D_02_800464E0(D_ovl0D_02_80047A1C, &D_ovl0D_02_80047A00)

/* .bss */
Vec4f_0D01 D_ovl0D_02_80047A00;
Vec3f D_ovl0D_02_80047A10;
s32 D_ovl0D_02_80047A1C;
u8 D_ovl0D_02_80047A20[0x4]; /* unused (func_ovl0D_02_80045FA8) */
s32 D_ovl0D_02_80047A24;
s32 D_ovl0D_02_80047A28;
Vec3f D_ovl0D_02_80047A30;
UnkStructA *D_ovl0D_02_80047A3C;
s32 D_ovl0D_02_80047A40;
s32 D_ovl0D_02_80047A44;
s32 D_ovl0D_02_80047A48[3];
UnkStruct_802AC610 *D_ovl0D_02_80047A54;
s32 D_ovl0D_02_80047A58;
/* where the debris of each of D_ovl0D_02_80047800 falls: three rows of four */
Vec3f D_ovl0D_02_80047A60[2][3][4];
u8 D_ovl0D_02_80047B80[0x10]; /* unused (func_ovl0D_02_80045FA8) */

/* .data */
/* passed to func_ovl0D_02_800471B0: three 12-byte records */
u32 D_ovl0D_02_800475F0[][3] = {
    { 0x9C0002, 0x5000E, 4 },
    { 0x9B0002, 0xE, 4 },
    { 0x970001, 0xF, 0x2004 },
};
/* passed to func_ovl0D_02_800471C8 */
u32 D_ovl0D_02_80047614[][3] = {
    { 0, 4, 0 },
    { 4, 0x980000, 0x84 },
};
/* two symbols (a fake, see the top): 80043010 addresses [1] apart */
#ifdef TARGET_PC
/* 8004445C walks both from 762C: one array on the host */
UnkStruct_ovl0C_01_8004A54C D_ovl0D_02_8004762C[2] = {
    { NULL, -1, 0x14F, 0, 0x400, 0x20, 0x400, 10.0f, 60.0f, 300.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x14D, 0, 0x400, 0x20, 0x400, 470.0f, -80.0f, 570.0f, 0.0f, 90.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
#define D_ovl0D_02_80047664 (D_ovl0D_02_8004762C + 1)
#else
UnkStruct_ovl0C_01_8004A54C D_ovl0D_02_8004762C[1] = {
    { NULL, -1, 0x14F, 0, 0x400, 0x20, 0x400, 10.0f, 60.0f, 300.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
UnkStruct_ovl0C_01_8004A54C D_ovl0D_02_80047664[1] = {
    { NULL, -1, 0x14D, 0, 0x400, 0x20, 0x400, 470.0f, -80.0f, 570.0f, 0.0f, 90.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
#endif
UnkStruct_ovl0C_01_8004A54C D_ovl0D_02_8004769C[1] = {
    { NULL, -1, 0x14B, 0, 0x400, 0x20, 0x400, 460.0f, 20.0f, 350.0f, 0.0f, 90.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
/* the gate */
UnkStruct_ovl0C_01_8004A54C D_ovl0D_02_800476D4[1] = {
    { NULL, -1, 0x15D, -1, -1, -1, -1, 150.0f, 80.0f, 300.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
/* the flag set once the gate is open */
s32 D_ovl0D_02_8004770C[1] = { 0x401 };
UnkStruct_ovl0C_01_8004A54C D_ovl0D_02_80047710[4] = {
    { NULL, -1, 0x161, -1, -1, -1, -1, 40.0f, 100.0f, 80.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x161, -1, -1, -1, -1, 100.0f, 100.0f, 20.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x161, -1, -1, -1, -1, 250.0f, 80.0f, 10.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x161, -1, -1, -1, -1, 390.0f, 100.0f, 20.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
/* the flags set once each of D_ovl0D_02_80047710 is done */
s32 D_ovl0D_02_800477F0[4] = { 0x40A, 0x40B, 0x40C, 0x40D };
UnkStruct_ovl0C_01_8004A54C D_ovl0D_02_80047800[2] = {
    { NULL, -1, 0x160, -1, -1, -1, -1, 350.0f, 60.0f, 230.0f, 0.0f, 180.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { NULL, -1, 0x160, -1, -1, -1, -1, 140.0f, 80.0f, 230.0f, 0.0f, 180.0f, 0.0f, 1.0f, 1.0f, 1.0f },
};
/* the corners of D_ovl0D_02_80047A60's rows */
Vec3f D_ovl0D_02_80047870[2] = {
    { 340.0f, 65.0f, 230.0f },
    { 130.0f, 85.0f, 230.0f },
};
/* the flags set once each of D_ovl0D_02_80047800 is done */
s32 D_ovl0D_02_80047888[2] = { 0x417, 0x418 };
Trio_0D01 D_ovl0D_02_80047890[4] = {
    { 0, 7, 0xE5 },
    { 1, 2, 0xE0 },
    { 2, 8, 0xE6 },
    { 3, 0, 0xDE },
};

#include "statics.h"

/* unused: the stubs IDO -O3 leaves at the end of the object; one of them
 * refers to the .bss nothing else uses, which would otherwise be common */
static void func_ovl0D_02_80045FA8(void) {
    D_ovl0D_02_80047A20[0] = D_ovl0D_02_80047B80[0];
}

static void func_ovl0D_02_80045FA0(void) {
}

static void func_ovl0D_02_80045F98(void) {
}

static void func_ovl0D_02_80045F90(void) {
}

static void func_ovl0D_02_80045F88(void) {
}

static void func_ovl0D_02_80045F80(void) {
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl0D_02_800444D4
#define CORE_FUNC_B func_ovl0D_02_80045F20
#define CORE_FUNC_C func_ovl0D_02_80043C84
#define CORE_FUNC_D func_ovl0D_02_80044DBC
#define CORE_BSS_A0 D_ovl0D_02_80047A24
#define CORE_BSS_A1 D_ovl0D_02_80047A28
#define CORE_BSS_B D_ovl0D_02_80047A30
#define CORE_ZJ_1704 func_ovl0D_02_80046B38
#define CORE_ZJ_1708 func_ovl0D_02_80046B58
#define CORE_ZJ_1709 func_ovl0D_02_80046B60
#define CORE_ZJ_1904 func_ovl0D_02_80046E68
#define CORE_ZJ_2B04 func_ovl0D_02_800473F0
#include "overlays/common/core.h"

/* the task of each of D_ovl0D_02_80047710 (its index is the argument) */
void func_ovl0D_02_8004596C(void) {
    PORT_IP(s32) idx;
    UnkStruct_ovl0C_01_8004A54C *e;
    s32 flag;
    Vec3f pos;
    s32 grid[6];
    s32 cnt;
    s32 r;
#ifdef TARGET_PC
    Vec3f port_ang; /* func_8022FF58 (ZeroJump 0x1803) writes three floats through &ang: on the N64 into the frame's next words */
#define ang port_ang.x
#else
    f32 ang;
#endif
    Vec3f tgt;
    s32 n; /* idx + 1 held (the frame's word here; which value: a guess, any held one fits) */
    s32 timer;

    idx = (PORT_IP(s32)) func_ovl0D_02_80046E70();
    e = &D_ovl0D_02_80047710[idx];
    flag = D_ovl0D_02_800477F0[idx];
    pos.x = e->unk14 * 10.0f + 150.0f;
    pos.y = e->unk18 * 10.0f;
    pos.z = e->unk1C * 10.0f + 150.0f;
    cnt = 0;
    n = idx + 1;
    timer = 240 / n;
    func_ovl0D_02_80046350(pos.x, pos.y, pos.z, grid, 0);
    while (func_ovl0D_02_800470D0(flag) == 0) {
        if (e->unk0 == NULL) {
            break;
        }
        e->unk0->unk40->unk10 = pos.x;
        e->unk0->unk40->unk18 = pos.z;
        tgt.x = D_ovl0D_02_80047A00.x;
        tgt.y = pos.y;
        tgt.z = D_ovl0D_02_80047A00.z;
        func_ovl0D_02_80046C88(&pos.x, &tgt.x, &ang);
        e->unk0->unk40->unk20 = ang;
        if (func_ovl0D_02_800470D0(0x424) != 0) {
            func_ovl0D_02_80045964(e, &timer);
        }
        if (idx == (cnt >> 2)) {
            cnt = 0;
            r = func_ovl0D_02_80046CA8(e->unk0, 0x40);
            if (r & 0x40) {
                if ((r & 0x2000) || (r & 0x4000)) {
                    func_ovl0D_02_8004595C(e, flag);
                    if (flag == D_ovl0D_02_80047A58) {
                        func_ovl0D_02_80047318(15, pos.x, pos.y, pos.z);
                    }
                } else {
                    func_ovl0D_02_80044DBC(0x26, e->unk0);
                }
            }
        }
        cnt++;
        func_ovl0D_02_80046E90();
    }
    func_ovl0D_02_80046E58();
}
#ifdef TARGET_PC
#undef ang
#endif

/* every few seconds, when the player is near, the model throws a bomb */
static void func_ovl0D_02_80045964(UnkStruct_ovl0C_01_8004A54C *e, s32 *timer) {
    UnkStructA_40 *p;
    s32 grid[7]; /* [7]: the frame (see the top) */

    if (*timer == 0) {
        p = e->unk0->unk40;
        func_ovl0D_02_80046350(p->unk10, p->unk14, p->unk18, grid, 0);
        if (func_ovl0D_02_800464C0(grid[0] - 12, grid[2] - 12, grid[0] + 12, grid[2] + 12) != 0 &&
            func_ovl0D_02_800464C0(grid[0] - 2, grid[2] - 2, grid[0] + 2, grid[2] + 2) == 0) {
            func_ovl0D_02_80046E48(func_ovl0D_02_800455A0, e, 0, 0, 0x110);
            func_ovl0D_02_80046C20(10.0f, 10.0f, 10.0f);
            func_ovl0D_02_80046C10(60);
        }
        *timer = (D_ovl0D_02_80047A54->unk10 != 0) ? 240 : 360;
    } else {
        (*timer)--;
    }
}

/* a model's hit: it fades out, scattering debris, and its tiles become solid */
static void func_ovl0D_02_8004595C(UnkStruct_ovl0C_01_8004A54C *e, s32 flag) {
    s32 i;
    s32 k;
    Vec3f v;
    UnkStructA *o;

    o = e->unk0;
    k = 5;
    func_ovl0D_02_80046CF8(o, 0x80000);
    func_ovl0D_02_80044DBC(0x73, o);
    for (i = 0; i < 30; i++) {
        o->unk40->unk36 = (u8) ((1.0f - i / 30.0f) * 255.0f);
        if (++k >= 6) {
            k = 0;
            func_ovl0D_02_80046DC8(o, &v.x);
            if (func_ovl0D_02_80047008(1) != 0) {
                v.x += 300.0 * func_ovl0D_02_80047000();
            } else {
                v.x -= 300.0 * func_ovl0D_02_80047000();
            }
            v.y += 100.0 + 300.0 * func_ovl0D_02_80047000();
            if (func_ovl0D_02_80047008(1) != 0) {
                v.z += 300.0 * func_ovl0D_02_80047000();
            } else {
                v.z -= 300.0 * func_ovl0D_02_80047000();
            }
            func_ovl0D_02_800472A0(20, &v.x, NULL);
        }
        func_ovl0D_02_80046E90();
    }
    func_ovl0D_02_80046D00(o, 0x01080000);
    func_ovl0D_02_80044234(e);
    func_ovl0D_02_800470C0(flag);
}

/* the task a model's hit starts: a bomb flies from it to the player */
void func_ovl0D_02_800455A0(void) {
    Vec3f tmp;
    Vec3f dir;
    Vec3f pos;
    UnkStruct_ovl0C_01_8004A54C *e;
    UnkStructA *o;
    UnkStructA *o2;
    s32 n;
    f32 dx;
    f32 dz;

    e = func_ovl0D_02_80046E70();
    o = func_ovl0D_02_80046D18(0, 0, 0x01000210, 0x60, 0);
    o2 = func_ovl0D_02_80046D30(o);
    o2->unk4C = 1.5f;
    o2->unk54 = 1.5f;
    o->unk40->unk30 = 1.5f;
    o->unk40->unk2C = 1.5f;
    o->unk40->unk28 = 1.5f;
    dir.x = e->unk0->unk40->unk20;
    dir.y = 60.0f;
    dir.z = 500.0f;
    func_ovl0D_02_80046C90(&dir.x, &pos.x);
    pos.x += e->unk0->unk40->unk10;
    pos.y += e->unk0->unk40->unk14;
    pos.z += e->unk0->unk40->unk18;
    o->unk40->unk10 = pos.x;
    o->unk40->unk14 = pos.y;
    o->unk40->unk18 = pos.z;
    func_ovl0D_02_800472A0(7, &pos.x, NULL);
    func_ovl0D_02_800472A0(14, &pos.x, NULL);
    func_ovl0D_02_80046E90();
    func_ovl0D_02_800473F8(0x61, &pos.x);
    o->unk68 = 100.0f;
    o->unk70 = 1.2250001f;
    o->unk6C = 0.0f;
    o->unk74 = 1000.0f;
    n = 0;
    while ((D_ovl0D_02_80047A00.y < o->unk40->unk14 || o->unk6C < o->unk68) && o->unk68 != 0.0f) {
        func_ovl0D_02_80046D70(o, &o->unk68, &o->unk6C, &o->unk70);
        n++;
    }
    o->unk40->unk14 = pos.y;
    o->unk6C = 0.0f;
    o->unk68 = 100.0f;
    dx = (D_ovl0D_02_80047A00.x - pos.x) / n;
    dz = (D_ovl0D_02_80047A00.z - pos.z) / n;
    while (o->unk68 != 0.0f) {
        o->unk40->unk10 += dx;
        o->unk40->unk18 += dz;
        func_ovl0D_02_80046D70(o, &o->unk68, &o->unk6C, &o->unk70);
        func_ovl0D_02_80046E90();
    }
    func_ovl0D_02_80046D00(o, 0x01000000);
    pos.x = o->unk40->unk10;
    pos.y = o->unk40->unk14 + 5.0f;
    pos.z = o->unk40->unk18;
    func_ovl0D_02_800472A0(10, &pos.x, NULL);
    func_ovl0D_02_800473F8(0x62, &pos.x);
    func_ovl0D_02_80046D10(o);
    func_ovl0D_02_80046E58();
}

/* the task of each of D_ovl0D_02_80047800 (its index is the argument):
 * once hit, it burns for three seconds, throwing debris */
void func_ovl0D_02_80045274(void) {
    PORT_IP(s32) idx;
    s32 i;
    s32 r;
    s32 t0;
    s32 t1;
    s32 t2;
    s32 t3;

    idx = (PORT_IP(s32)) func_ovl0D_02_80046E70();
    do {
        func_ovl0D_02_80046E68(idx + 7);
        D_ovl0D_02_80047A3C->unk30 = 1;
        D_ovl0D_02_80047A3C->unk34 = 50.0f;
        D_ovl0D_02_80047A3C->unk38 = 50.0f;
        D_ovl0D_02_80047A3C->unk40->unk10 = D_ovl0D_02_80047800[idx].unk0->unk40->unk10 + -300.0f;
        D_ovl0D_02_80047A3C->unk40->unk14 = D_ovl0D_02_80047800[idx].unk0->unk40->unk14 + 200.0f;
        D_ovl0D_02_80047A3C->unk40->unk18 = D_ovl0D_02_80047800[idx].unk0->unk40->unk18 + 200.0f;
    } while (func_ovl0D_02_80046CA8(D_ovl0D_02_80047A3C, 0x40) == 0);
    func_ovl0D_02_800470C0(D_ovl0D_02_80047888[idx]);
    func_ovl0D_02_80043F94(idx);
    func_ovl0D_02_80043858(&D_ovl0D_02_80047800[idx]);
    t0 = 180;
    t1 = 180;
    t2 = 180;
    t3 = 180;
    func_ovl0D_02_800473F8(0x63, &D_ovl0D_02_80047A60[idx][1][2].x);
    for (i = 0; i < 180; i++) {
        t3++;
        r = func_ovl0D_02_80047008(11);
        if (t3 >= 31) {
            func_ovl0D_02_800472A0(20, &D_ovl0D_02_80047A60[idx][0][r].x, NULL);
            t3 = 0;
        }
        t2++;
        r = func_ovl0D_02_80047008(11);
        if (t2 >= 31) {
            func_ovl0D_02_800472A0(17, &D_ovl0D_02_80047A60[idx][0][r].x, NULL);
            t2 = 0;
        }
        t1++;
        r = func_ovl0D_02_80047008(11);
        if (t1 >= 17) {
            func_ovl0D_02_800472A0(14, &D_ovl0D_02_80047A60[idx][0][r].x, NULL);
            t1 = 0;
        }
        t0++;
        r = func_ovl0D_02_80047008(11);
        if (t0 >= 33) {
            func_ovl0D_02_80046E48(func_ovl0D_02_80044EEC, &D_ovl0D_02_80047A60[idx][0][r], 0, 0, 0x150);
            t0 = 0;
        }
        func_ovl0D_02_80046E90();
    }
    func_ovl0D_02_80046E58();
}

/* a piece of debris (the argument is where it starts) */
void func_ovl0D_02_80044EEC(void) {
    Vec3f *start;
    Vec3f v;
    UnkStructA *o;
    s32 model;
    f32 spin;
    s32 k;
    f32 fall;
    f32 s;

    start = func_ovl0D_02_80046E70();
    switch (func_ovl0D_02_80047008(2)) {
        case 0:
            model = 0xDE;
            break;
        case 1:
            model = 0xE0;
            break;
        case 2:
            model = 0xE6;
            break;
    }
    o = func_ovl0D_02_80046D20(model, 0x01000000, 0xD0, 0);
    func_ovl0D_02_80046DD0(start, o);
    o->unk40->unk20 = func_ovl0D_02_80047000() * 360.0;
    fall = func_ovl0D_02_80047000() * 2 + 3.0;
    if (func_ovl0D_02_80047008(1) != 0) {
        spin = 10.0f;
    } else {
        spin = -10.0f;
    }
    k = 20;
    o->unk68 = func_ovl0D_02_80047000() * 10.0 + 20.0;
    while (o->unk68 != 0.0f) {
        o->unk40->unk18 -= fall;
        o->unk40->unk20 += spin;
        func_ovl0D_02_80046D78(o);
        if (++k >= 21) {
            k = 0;
            if (func_ovl0D_02_80047008(1) != 0) {
                v.x = 50.0f;
            } else {
                v.x = -50.0f;
            }
            v.y = func_ovl0D_02_80047000() * 50.0;
            if (func_ovl0D_02_80047008(1) != 0) {
                v.z = 50.0f;
            } else {
                v.z = -50.0f;
            }
            func_ovl0D_02_800472A0(7, &v.x, o);
        }
        func_ovl0D_02_80046E90();
    }
    func_ovl0D_02_80046DC8(o, &v.x);
    func_ovl0D_02_800473F0(0x168, v.x, v.y, v.z);
    func_ovl0D_02_800472A0(13, &v.x, NULL);
    for (s = 1.0f; s > 0.1f; s -= 0.025f) {
        o->unk40->unk20 += spin;
        o->unk40->unk30 = s;
        o->unk40->unk28 = o->unk40->unk2C = o->unk40->unk30;
        func_ovl0D_02_80046E90();
    }
    if (func_ovl0D_02_80047008(1) != 0) {
        func_ovl0D_02_80047318(7, o->unk40->unk10, o->unk40->unk14, o->unk40->unk18);
    } else {
        func_ovl0D_02_80047318(8, o->unk40->unk10, o->unk40->unk14, o->unk40->unk18);
    }
    func_ovl0D_02_80046D10(o);
    func_ovl0D_02_80046E58();
}

/* the task func_ovl0D_02_80044CA0 starts: a hit model shrinks away */
void func_ovl0D_02_80044DF0(void) {
    UnkStructA *self;
    UnkStructA *o;
    s32 i;
    s32 n;

    self = func_ovl0D_02_80046E70();
    o = func_ovl0D_02_80046D20(0x183, 0x11080000, 0xD0, 0);
    o->unk40->unk10 = self->unk40->unk10;
    o->unk40->unk14 = self->unk40->unk14;
    o->unk40->unk18 = self->unk40->unk18;
    n = 20;
    func_ovl0D_02_80044DBC(0xB3, o);
    for (i = 0; i < n; i++) {
        func_ovl0D_02_80044CE4(o, i, 20, 2.0f, 4.0f);
        func_ovl0D_02_80046E90();
    }
    func_ovl0D_02_80046D10(o);
    func_ovl0D_02_80046E58();
}

void func_ovl0D_02_80044CE4(UnkStructA *arg0, s32 arg1, s32 arg2, f32 arg3, f32 arg4) {
    arg0->unk40->unk28 = func_ovl0D_02_80046FF0((f32) arg1 / (f32) arg2 * 90.0f) * arg3;
    arg0->unk40->unk2C = func_ovl0D_02_80046FF0((f32) arg1 / (f32) arg2 * 180.0f) * arg4;
    arg0->unk40->unk36 = (s32) ((1.0f - (f32) arg1 / (f32) arg2) * 63.0f) + 0x80;
}

/* the stage's callback (func_ovl0D_02_800444FC registers it) */
void func_ovl0D_02_80044CA0(UnkStructA *arg0) {
    func_ovl0D_02_80046D00(arg0, 0x01000000);
    func_ovl0D_02_80046E48(func_ovl0D_02_80044DF0, arg0, 0, 0, 0x150);
}

/* the cutscene once the gate opens */
void func_ovl0D_02_80044B8C(void) {
    s32 i;

    func_ovl0D_02_800464E8(1);
    while (func_ovl0D_02_800470D0(0x424) == 0) {
        func_ovl0D_02_80046E90();
    }
    func_ovl0D_02_800464E8(1);
    func_ovl0D_02_80046460(0);
    func_ovl0D_02_800472C8(func_ovl0D_02_800464D8(), 2);
    func_ovl0D_02_800472D0(func_ovl0D_02_800464D8(), 6);
    func_ovl0D_02_80046598(1);
    func_ovl0D_02_80046590(3);
    func_ovl0D_02_80046500("WM1 WAP WG1 WSH WDW WB0 ST1 M0 M0 ST30 M4 WDN WB0 M1 M6 ST20 M2");
    func_ovl0D_02_80046500("WM1 WAP WG1 WSH WDW WB5 WDN WB0 M1 M6 ST20 M2");
    for (i = 0; i < 2; i++) {
        func_ovl0D_02_80046500("WM1 WAP WG1 WSH WDW WB2 WDN WB0 M1 M6 ST20 M2");
    }
    func_ovl0D_02_80046500("WM1 WAP WG1 WSH WDW WB3 WDN WB5 M4 WDE WB3");
    func_ovl0D_02_80046E68(60);
    func_ovl0D_02_80047548();
    func_ovl0D_02_80046E58();
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl0D_02_80044A50(s32 arg0) {
    switch (arg0) {
        case 2:
            func_ovl0D_02_800444FC();
            break;
        case 16:
            func_ovl0D_02_80043F6C();
            break;
        case 17:
            func_ovl0D_02_80043F44();
            break;
        case 18:
            func_ovl0D_02_80043F1C();
            break;
        case 19:
            func_ovl0D_02_80043EF4();
            break;
        case 20:
            func_ovl0D_02_80043ECC();
            break;
        case 4:
            func_ovl0D_02_80043D78();
            break;
        case 5:
            func_ovl0D_02_80043BA4();
            break;
        case 0x50:
            func_ovl0D_02_80043ABC();
            break;
        case 3:
            func_ovl0D_02_80043510();
            break;
        case 12:
            func_ovl0D_02_80043A70();
            break;
        case 1:
            func_ovl0D_02_800438FC();
            break;
    }
}




void func_ovl0D_02_80044474(void) {
    if (D_ovl0D_02_80047A54->unk0 & 2) {
        func_ovl0D_02_800470C0(0x40A);
        func_ovl0D_02_800470C0(0x40B);
        func_ovl0D_02_800470C8(0x40C);
        func_ovl0D_02_800470C0(0x40D);
        D_ovl0D_02_80047A58 = 0x40C;
    }
}

/* the stage's set-up (the handler's case 2) */
void func_ovl0D_02_800444FC(void) {
    func_ovl0D_02_800471B0(D_ovl0D_02_800475F0);
    func_ovl0D_02_800471C8(D_ovl0D_02_80047614);
    func_ovl0D_02_800462F8(0x13B);
    func_ovl0D_02_800444F4(D_ovl0D_02_80047890, 4);
    func_ovl0D_02_80046370(0x141);
    func_ovl0D_02_800464A8();
    func_ovl0D_02_80046308(D_ovl0D_02_80047A48);
    func_ovl0D_02_800444EC();
    func_ovl0D_02_80046440(0);
    func_ovl0D_02_80047190(1);
    func_ovl0D_02_800463C0(0x201F);
    func_ovl0D_02_800463E0(func_ovl0D_02_80044CA0);
    func_ovl0D_02_800444D4();
    D_ovl0D_02_80047A54 = func_ovl0D_02_80046448();
    /* fake: (see the top): the call's delay slot stays empty */
#define STATICS_PART 1
#include "overlays/ovl_0D_02/statics.h"
#undef STATICS_PART
    func_ovl0D_02_8004446C();
    func_ovl0D_02_80044464();
    func_ovl0D_02_8004445C();
    func_ovl0D_02_80044454();
    D_ovl0D_02_80047A40 = 0;
    D_ovl0D_02_80047A44 = 0;
    func_ovl0D_02_8004444C();
    func_ovl0D_02_80044444();
    func_ovl0D_02_800442D8();
    func_ovl0D_02_8004422C();
    func_ovl0D_02_80044224();
    func_ovl0D_02_80043010();
    func_ovl0D_02_80046378();
}

static void func_ovl0D_02_800444F4(Trio_0D01 *t, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        func_ovl0D_02_80047358(t->unk0, t->unk4, t->unk8);
        t++;
    }
}

static void func_ovl0D_02_8004446C(void) {
    func_ovl0D_02_80046BE8(2400.0f, 1600.0f, 800.0f, 1600.0f);
    func_ovl0D_02_80046BF8(1600.0f, 1600.0f, 3200.0f, 1600.0f);
}

static void func_ovl0D_02_80044464(void) {
    UNK_MACRO_ovl0D_02_80043510();
}

static void func_ovl0D_02_8004445C(void) {
    UnkStruct_ovl0C_01_8004A54C *p;

    for (p = D_ovl0D_02_8004762C; p < D_ovl0D_02_8004762C + 2; p++) {
        func_ovl0D_02_8004409C(p);
        p->unk0->unk40->unk36 = 0x80;
    }
}

static void func_ovl0D_02_80044454(void) {
    s32 i;

    for (i = 0; i < 1; i++) {
        func_ovl0D_02_8004409C(&D_ovl0D_02_8004769C[i]);
        D_ovl0D_02_8004769C[i].unk0->unk40->unk36 = 0x80;
    }
}

static void func_ovl0D_02_8004444C(void) {
    s32 i;

    for (i = 0; i < 1; i++) {
        func_ovl0D_02_8004409C(&D_ovl0D_02_800476D4[i]);
        func_ovl0D_02_80046CF8(D_ovl0D_02_800476D4[i].unk0, 0x02000000);
        func_ovl0D_02_80046D00(D_ovl0D_02_800476D4[i].unk0, 0x80000);
    }
}

/* the gate: open already if its flag is set */
static void func_ovl0D_02_80044444(void) {
    s32 i;
    UnkStruct_ovl0C_01_8004A54C *e;
    UnkStructA *o;

    for (i = 0; i < 1; i++) {
        if (func_ovl0D_02_800470D0(D_ovl0D_02_8004770C[i]) != 0) {
            e = &D_ovl0D_02_800476D4[i];
            o = e->unk0;
            /* fake: (see the top) */
#define STATICS_PART 2
#include "overlays/ovl_0D_02/statics.h"
#undef STATICS_PART
            func_ovl0D_02_80046D00(e->unk0, 0x02000000);
            func_ovl0D_02_8004311C(e);
        }
    }
}

/* the rows of points the debris of D_ovl0D_02_80047800 falls from, from row
 * j on (the caller passes 0: its line, after this loop's, is what as1 reads) */
static void func_ovl0D_02_8004421C(Vec3f *corner, Vec3f (*rows)[4], s32 j) {
    s32 k;

    for (; j < 3; j++) {
        for (k = 0; k < 4; k++) {
            rows[j][k].x = corner->x * 10.0f - k * 200.0f;
            rows[j][k].y = corner->y * 10.0f + j * 200.0f;
            rows[j][k].z = corner->z * 10.0f - 100.0f;
        }
    }
}

static void func_ovl0D_02_8004422C(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        func_ovl0D_02_8004421C(&D_ovl0D_02_80047870[i], D_ovl0D_02_80047A60[i], 0);
    }
}

/* the models of D_ovl0D_02_80047800: those whose flags are set are done */
static void func_ovl0D_02_80044224(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        func_ovl0D_02_8004409C(&D_ovl0D_02_80047800[i]);
        if (func_ovl0D_02_800470D0(D_ovl0D_02_80047888[i]) == 0) {
            func_ovl0D_02_80046E48(func_ovl0D_02_80045274, (void *) i, 0, 0, 0x110);
        } else if (func_ovl0D_02_800470D0(D_ovl0D_02_80047888[i]) != 0) {
            func_ovl0D_02_80044094(i);
        }
    }
}

static void func_ovl0D_02_80044094(s32 i) {
    func_ovl0D_02_80043F94(i);
    func_ovl0D_02_80043858(&D_ovl0D_02_80047800[i]);
}

/* the models of D_ovl0D_02_80047710: one of those still standing is picked
 * (D_ovl0D_02_80047A58) */
static void func_ovl0D_02_800442D8(void) {
    s32 i;
    s32 n;
    s32 flag;
    s32 left[4];

    for (i = 0; i < 4; i++) {
        left[i] = -1;
    }
    for (i = 0, n = 0; i < 4; i++) {
        flag = 0x40A + i;
        if (func_ovl0D_02_800470D0(flag) == 0) {
            left[n] = flag;
            n++;
        }
    }
    if (n > 0) {
        D_ovl0D_02_80047A58 = left[func_ovl0D_02_80047008(n - 1)];
    } else {
        D_ovl0D_02_80047A58 = -1;
    }
    for (i = 0; i < 4; i++) {
        if (func_ovl0D_02_800470D0(D_ovl0D_02_800477F0[i]) == 0) {
            func_ovl0D_02_8004409C(&D_ovl0D_02_80047710[i]);
            func_ovl0D_02_80046CF8(D_ovl0D_02_80047710[i].unk0, 0x02000000);
            func_ovl0D_02_80046D00(D_ovl0D_02_80047710[i].unk0, 0x80000);
            func_ovl0D_02_80046CC8(D_ovl0D_02_80047710[i].unk0, 0xC0);
            D_ovl0D_02_80047710[i].unk0->unk30 = 4;
            D_ovl0D_02_80047710[i].unk0->unk34 = 200.0f;
            D_ovl0D_02_80047710[i].unk0->unk38 = 300.0f;
            func_ovl0D_02_80046E48(func_ovl0D_02_8004596C, (void *) i, 0, 0, 0x110);
        } else {
            func_ovl0D_02_80044234(&D_ovl0D_02_80047710[i]);
        }
    }
}

static void func_ovl0D_02_80044234(UnkStruct_ovl0C_01_8004A54C *arg0) {
    s32 pos[6];

    func_ovl0D_02_80046350(arg0->unk14 * 10.0f, arg0->unk18 * 10.0f, arg0->unk1C * 10.0f, pos, 0);
    func_ovl0D_02_80046338(pos[0], pos[1], pos[2], pos[0] + 3, pos[2] + 3, 0x11);
    func_ovl0D_02_80046338(pos[0], pos[1] + 1, pos[2], pos[0] + 3, pos[2] + 3, 0x10);
}

static void func_ovl0D_02_8004409C(UnkStruct_ovl0C_01_8004A54C *arg0) {
    arg0->unk0 = func_ovl0D_02_80046D40(arg0->unk8, 0xD0, 0.0f, 0.0f, 0.0f);
    func_ovl0D_02_80046CF8(arg0->unk0, 0x01080000);
    func_ovl0D_02_80046D00(arg0->unk0, 0x02000000);
    if (arg0->unkC != -1 && arg0->unkE != -1 && arg0->unk10 != -1 && arg0->unk12 != -1) {
        arg0->unk4 = func_ovl0D_02_80046A40(arg0->unk0->unk40, 0, arg0->unkC, arg0->unkE, arg0->unk10, arg0->unk12);
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

/* the tile under one of D_ovl0D_02_80047800 once it is done */
static void func_ovl0D_02_80043F94(s32 idx) {
    s32 grid[6];
    s32 type;
    UnkStructA_40 *p;
    s32 pos[3];

    p = D_ovl0D_02_80047800[idx].unk0->unk40;
    func_ovl0D_02_80046350(p->unk10 + -300.0f, p->unk14 + 200.0f, p->unk18 + 200.0f, grid, 0);
    grid[2] -= 1;
    type = func_ovl0D_02_80046368(grid[0], grid[1], grid[2]);
    func_ovl0D_02_80046340(grid[0], grid[1], grid[2], grid[0], grid[1], grid[2], 0x1F);
    type = ((type >> 7) & 7) - 2;
    func_ovl0D_02_80046358(grid[0], grid[1], grid[2], pos, 0);
    func_ovl0D_02_80047390(type, pos[0], pos[1], pos[2]);
}

void func_ovl0D_02_80043F6C(void) {
    func_ovl0D_02_800464B0(0x10, 2, 0x18);
}

void func_ovl0D_02_80043F44(void) {
    func_ovl0D_02_800464B0(0x2E, 0, 0x19);
}

void func_ovl0D_02_80043F1C(void) {
    func_ovl0D_02_800464B0(0x10, 2, 0x18);
}

void func_ovl0D_02_80043EF4(void) {
    func_ovl0D_02_800464B0(0x10, 2, 0x1D);
}

void func_ovl0D_02_80043ECC(void) {
    func_ovl0D_02_800464B0(0, 4, 0x13);
}

void func_ovl0D_02_80043D78(void) {
    func_ovl0D_02_80046508();
    func_ovl0D_02_80046B98(0x40);
    func_ovl0D_02_80046C68(400.0f, 800.0f);
    func_ovl0D_02_80047170(0x2D, 0x6E, -0x2C);
    switch (func_ovl0D_02_800463F8()) {
        case 17:
            func_ovl0D_02_80043C84(90.0f);
            func_ovl0D_02_800464F0(0xC0);
            break;
        case 20:
            func_ovl0D_02_800464F0(0x40);
            func_ovl0D_02_80043C84(90.0f);
            break;
        case 19:
            func_ovl0D_02_80043C84(0.0f);
            func_ovl0D_02_800464F0(0);
            break;
        default:
            func_ovl0D_02_80046B58(0.0f, 32768.0f, 32768.0f);
            break;
    }
    func_ovl0D_02_80043CCC();
}

void func_ovl0D_02_80043CCC(void) {
    if (D_ovl0D_02_80047A54->unk0 & 2) {
        func_ovl0D_02_800464B0(0x1D, 1, 8);
        func_ovl0D_02_80046508();
        func_ovl0D_02_80046E48(func_ovl0D_02_80044B8C, NULL, 0, 0, 0x130);
        func_ovl0D_02_80046BA0(15);
        func_ovl0D_02_80046BB0(1);
        func_ovl0D_02_80043C84(45.0f);
        func_ovl0D_02_800464F0(0);
    }
}








void func_ovl0D_02_80043BA4(void) {
    switch (func_ovl0D_02_800463F8()) {
        case 17:
            func_ovl0D_02_800464F8(0xC0, 3, 8.5f, 7, 0, 1, 1);
            func_ovl0D_02_800464E8(0);
            break;
        case 19:
            func_ovl0D_02_800464F8(0, 3, 8.5f, 2, 0, 1, 1);
            func_ovl0D_02_800464E8(0);
            break;
        case 20:
            func_ovl0D_02_800464F8(0x40, 3, 8.5f, 7, 0, 1, 1);
            func_ovl0D_02_800464E8(0);
            break;
    }
}

void func_ovl0D_02_80043ABC(void) {
    func_ovl0D_02_80043AB4();
    if (func_ovl0D_02_800464C0(0x2E, 0x18, 0x2E, 0x1A) != 0) {
        D_ovl0D_02_80047A24 = 0x38;
        D_ovl0D_02_80047A28 = 0x12;
        func_ovl0D_02_800463F0(0x38, 0x12, 0);
    }
    if (func_ovl0D_02_800464C0(0xF, 0x1D, 0x11, 0x1F) != 0 && func_ovl0D_02_800470D0(0x401) != 0) {
        D_ovl0D_02_80047A24 = 0x3A;
        D_ovl0D_02_80047A28 = 0x12;
        func_ovl0D_02_800463F0(0x3A, 0x12, 0);
    }
    if (func_ovl0D_02_800464C0(0, 0x12, 0, 0x13) != 0) {
        D_ovl0D_02_80047A24 = 0x3B;
        D_ovl0D_02_80047A28 = 0x12;
        func_ovl0D_02_800463F0(0x3B, 0x12, 0);
    }
}

static void func_ovl0D_02_80043AB4(void) {
}

void func_ovl0D_02_80043A70(void) {
    func_ovl0D_02_80043A68();
    func_ovl0D_02_80043A68();
    if (D_ovl0D_02_80047A24 == 0x38 || D_ovl0D_02_80047A24 == 0x3A || D_ovl0D_02_80047A24 == 0x3B) {
        func_ovl0D_02_800464E8(1);
    }
}

static void func_ovl0D_02_80043A68(void) {
}

/* the stage's clean-up (the handler's case 1) */
void func_ovl0D_02_800438FC(void) {
    func_ovl0D_02_800438D8();
    func_ovl0D_02_800438E0();
    func_ovl0D_02_800438E8();
    func_ovl0D_02_800438F0();
    func_ovl0D_02_800438F8();
    func_ovl0D_02_8004381C();
}

static void func_ovl0D_02_800438F8(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (D_ovl0D_02_80047800[i].unk0 != NULL) {
            func_ovl0D_02_80043858(&D_ovl0D_02_80047800[i]);
        }
    }
}

static void func_ovl0D_02_800438F0(void) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (D_ovl0D_02_80047710[i].unk0 != NULL) {
            func_ovl0D_02_80043858(&D_ovl0D_02_80047710[i]);
        }
    }
}

static void func_ovl0D_02_800438E8(void) {
    s32 i;

    for (i = 0; i < 1; i++) {
        if (D_ovl0D_02_800476D4[i].unk0 != NULL) {
            func_ovl0D_02_80043858(&D_ovl0D_02_800476D4[i]);
        }
    }
}

static void func_ovl0D_02_800438E0(void) {
    s32 i;

    for (i = 0; i < 1; i++) {
        if (D_ovl0D_02_8004769C[i].unk0 != NULL) {
            func_ovl0D_02_80043858(&D_ovl0D_02_8004769C[i]);
        }
    }
}

static void func_ovl0D_02_80043858(UnkStruct_ovl0C_01_8004A54C *arg0) {
    func_ovl0D_02_80046D10(arg0->unk0);
    if (arg0->unkC != 0xFFFF && arg0->unkE != 0xFFFF && arg0->unk10 != 0xFFFF && arg0->unk12 != 0xFFFF) {
        func_ovl0D_02_80046A48(arg0->unk4);
    }
    arg0->unk0 = NULL;
    arg0->unk4 = -1;
}

void func_ovl0D_02_8004381C(void) {
    if (D_ovl0D_02_80047A54->unk0 & 2) {
        func_ovl0D_02_80047550();
    }
}

/* the gate: it opens when something hits it */
void func_ovl0D_02_80043510(void) {
    s32 i;

    func_ovl0D_02_80043508();
    func_ovl0D_02_80043500();
    for (i = 0; i < 1; i++) {
        func_ovl0D_02_800434F8(&D_ovl0D_02_800476D4[i], D_ovl0D_02_8004770C[i]);
    }
}

static void func_ovl0D_02_80043500(void) {
    func_ovl0D_02_800470C0(0x424);
}

static void func_ovl0D_02_800434F8(UnkStruct_ovl0C_01_8004A54C *e, s32 flag) {
    Vec3f dir;
    Vec3f pos;
    s32 r;

    if (!(e->unk0->unk8 & 0x04000000)) {
        D_ovl0D_02_80047A3C->unk30 = 1;
        D_ovl0D_02_80047A3C->unk34 = 75.0f;
        D_ovl0D_02_80047A3C->unk38 = 50.0f;
        dir.x = e->unk0->unk40->unk20 + 90.0f;
        dir.y = 0.0f;
        dir.z = 100.0f;
        func_ovl0D_02_80046C90(&dir.x, &pos.x);
        D_ovl0D_02_80047A3C->unk40->unk10 = e->unk0->unk40->unk10 + pos.x;
        D_ovl0D_02_80047A3C->unk40->unk14 = e->unk0->unk40->unk14 + pos.y;
        D_ovl0D_02_80047A3C->unk40->unk18 = e->unk0->unk40->unk18 + pos.z;
        r = func_ovl0D_02_80046CA8(D_ovl0D_02_80047A3C, 0x40);
        if (r != 0) {
            if ((r & 0x6000) && func_ovl0D_02_800470D0(flag) == 0 &&
                !(e->unk0->unk8 & 0x04000000)) {
                func_ovl0D_02_80046CF8(e->unk0, 0x04000000);
                func_ovl0D_02_80046D00(e->unk0, 0x02000000);
                func_ovl0D_02_800473F0(0x60, D_ovl0D_02_80047A3C->unk40->unk10, D_ovl0D_02_80047A3C->unk40->unk14, D_ovl0D_02_80047A3C->unk40->unk18);
            } else if (func_ovl0D_02_800470D0(flag) == 0) {
                func_ovl0D_02_800473F0(0x26, D_ovl0D_02_80047A3C->unk40->unk10, D_ovl0D_02_80047A3C->unk40->unk14, D_ovl0D_02_80047A3C->unk40->unk18);
            }
        }
    } else if ((e->unk0->unk40->unkC & 0x40) && func_ovl0D_02_800470D0(flag) == 0) {
        func_ovl0D_02_8004311C(e);
        func_ovl0D_02_800470C0(flag);
        func_ovl0D_02_800473D0(0x68);
    }
}

static s32 func_ovl0D_02_80043114(UnkStruct_ovl0C_01_8004A54C *e) {
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

/* the tiles under the gate: solid, and the frame around its opening */
static void func_ovl0D_02_8004311C(UnkStruct_ovl0C_01_8004A54C *e) {
    s32 grid[6];
    s32 x0;
    s32 y0;
    s32 z0;
    s32 x1;
    s32 y1;
    s32 z1;
    s32 x2;
    s32 z2;
    s32 x3;
    s32 z3;
    s32 len;
    UnkStructA_40 *p;

    p = e->unk0->unk40;
    func_ovl0D_02_80046350(p->unk10, p->unk14, p->unk18, grid, 0);
    len = x1 + z1; /* fake: (see the top) */
    x0 = grid[0];
    y0 = grid[1];
    z0 = grid[2];
    switch (e->unk0->unk28) {
        case 0x15C:
            len = 5;
            break;
        case 0x15D:
            len = 7;
            break;
        case 0x15E:
            len = 10;
            break;
        default:
            while (1) {
            }
    }
    y1 = grid[1] + 2;
    switch (func_ovl0D_02_80043114(e)) {
        case 0:
            /* fake: (see the top) */
#define STATICS_PART 3
#include "overlays/ovl_0D_02/statics.h"
#undef STATICS_PART
