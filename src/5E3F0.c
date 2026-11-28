/*
 * main_code's 5E3F0 (-O3): 155 functions (us 0x80243BF0-0x802513D8)
 * around a table of five 0xC8-byte entries, D_800AEDF0[5] (an object in
 * unk28, flag words in unk10 and unk68), the current entry in D_802AC894.
 *
 * Functions are in the reverse of the ROM's order (IDO -O3 emits last
 * to first); 0x80243BF0-0x802457D8 by hand, most of the rest m2c's,
 * with the fixes the -O3 build needs. The object is -O3: its
 * small getters and flag tests are inlined into every caller and still
 * emitted, and the inlined copies are what give the original its
 * registers:
 * - func_80243DDC (D_802AC894 through a variable, so in v1) in place of
 *   every read of D_802AC894: the wrappers that pass it (80243DEC,
 *   802443A0, 802450DC, 80245280, 8024557C, 80245F74, 8025068C) match
 *   only with the call;
 * - func_802443F4 (unk28 through a variable) for D_800AEDF0[i].unk28;
 * - func_80244BA4/func_80243EB4/func_802452AC/func_80243E18 for the
 *   `v = 0; if ((x & f) == f) v = 1;` tests of unk10, unk68 and D_802AC9E8
 *   (80244E04 goes from 64 differing words to 10 with them).
 * func_8024A0DC is called without its argument (80250DC8 leaves a0 as
 * it is: its own caller's flag; the port passes it), so it has no
 * prototype here.
 * The entry's unk58-unk64, unk6C, unk88, unk8C, unkC0 and unkC4 are
 * floats. An empty `if (!X && !X) {}` on a memory load at the top of a
 * function (X = D_800AEDF0[arg0].unk10, D_802AC9E8 or D_802AC8D0[arg0])
 * only uses up ugen temporaries: it puts the t-registers of 8024BD1C and
 * others in step with the original's. ugen's listing (cc -K, ugen -l)
 * shows what it does: `mul $14,arg0,200` (the D_800AEDF0 index, the FIFO's
 * first temp t6) and the load into $2, which as1 drops as dead; a single
 * `if (!X) {}` is removed by uopt, the `&&` pair isn't. In 8024ABA8,
 * 8024AF24 and 8024441C (fakes pass 3) the original's was an unused
 * `obj = func_802443F4(arg0)`: the inlined getter does exactly that.
 * 8024E088 is in clean C (frame and shape the original's); jp's
 * differences there are `#if VERSION_JP` arms. 80246F00 is in clean C
 * too: one switch in the original's case order, the arg1 == 0 copy of
 * case 0x20 a loop over D_800AEDF0[i] (uopt unrolls it to absolute
 * addresses), func_80243EF0 inlined for case 8's test. Its call of
 * func_80246D74 passes the s16 index unextended, so func_80246D74 has
 * only a `()` prototype; it copies its argument into a local first
 * (`move s0,a0`, then `sll/sra` for func_80270770's s16). In case 4,
 * obj takes s0 because each `obj = *D_802ACF58` web whose call it is
 * passed to is still live after it (a dead `!obj->unk40` read); with
 * that, `hit` gets no saved register, so it lives in its home and only
 * its tests share a0, as in the original.
 * The large m2c functions (8024891C, 8024D8C4, 802444B8, 8024A0DC) are
 * still m2c's shape.
 * func_80244354 is never inlined in the original: its explicit final
 * `return;` keeps it out of line here too (umerge counts the source's
 * size before uopt; a dead initialiser `v = 0` does the same; which one
 * the original had is a guess).
 * D_802ACEC8 is `s32 [][3]`; 802455A8/802456FC/802457D8 pass its row to
 * func_8024557C and func_80245480/func_80245690, which call
 * func_80245430. Where m2c expanded an inlined getter or flag test back
 * into loads, calling the getter again gives the original's frame (each
 * inlined copy keeps its frame area) and registers. Declaration order
 * of a variable initialised from a getter moves the code and the
 * stack slots (8024C0D8, 80250398, 8024D108). The same goes for
 * func_80244BE0 (its arg1 is s16): the `func_80270770(); unk2C = 0;`
 * pairs in 80244C30, 8024694C, 8024C884, 80249E30 and 8024D20C are
 * its inlined copies (80244C30's duplicate store comes from it).
 * Where the original indexes D_800AEDF0[arg0] directly, an entry
 * pointer variable or an `id = arg0` copy costs 8 bytes of frame or
 * moves a spill slot (8024BC20, 80244E04, 80246C3C, 80249648). An
 * empty `do {} while (0);` splits a block and changes uopt's choices
 * (8024B444, 8024878C; 80244FF8's was there for a dead `p = &entry`
 * store, and both went in fakes pass 10).
 * 8024C04C's unused `o = func_802443F4(arg0)` (as 8024AF24's): the
 * inlined copy emits nothing, but its R2 store and its return value's
 * range take v0 and v1 in the entry block.
 * Fakes pass 4: more dead reads and pads were inlined getters, each with
 * its frame area, the entry pointer local gone (D_800AEDF0[arg0] direct):
 * 80248148 (func_802443F4 for obj), 8024694C, 8024BD1C, 80247F90,
 * 8024CEB8 (func_802443F4 and func_802443CC); 8024C334's pad is
 * func_8024C314 inlined. A match only shows equivalence.
 * In 8024C334 the unused func_80243D54 call in the first branch does
 * the same for the first flag: its R2 store makes the flag's own R2
 * store dead (a store to R2 that reaches a call keeps a range), so the
 * flag gets v0. Its unk28 is loaded straight into obj (t0, kept by a
 * dead read); a getter there would end the entry block before the
 * first flag's unk10 load, which must share it with the parameters.
 * 80246078's third loop had the same unused func_80243D54 call after
 * its test; it was the getter for the func_80232D18 argument (its R2
 * store does the same), with the function-level `j` that the inner
 * block's own `j` shadows gone (its slot was the getter's area).
 * Fakes pass 10 (DEBUG_PRINT checks, include/macros.h, messages are
 * placeholders): 802500D4's dead read of r is `r >= 4` (tbl's size; any
 * single test of r matches; its frame pad holds the 50.0f spawn
 * distance, a guess among held values), 8024E088's of spA0 a range
 * check of the stick angle, 80246F00 case 4's final `*D_802ACF58` read
 * a check of D_802ACF58, 8024C334's `e->unk28` read in the t2 branch a
 * check of the object and its model (it must read twice: no single test
 * moves the spill slot). 8024D8C4's dx dead read: the diagonal cases set
 * dx and dz in two statements each, dx first (`dx = dz = v` chains
 * colour dx/dz the other way round), and its zeroing as three
 * statements of int 0 (0.0f changes the code).
 * One object: no jal goes forward, there's no nop padding between
 * functions, and .rodata and .data run in function order with no
 * internal pads; split after func_80244354 (the only out-of-line
 * function, 16-aligned end), the rest no longer inlines the getters
 * before it.
 * .data (0x190) is defined below with the
 * .rodata (0x870: 540 words, 403 of them jump-table entries). The .bss
 * (D_802AC890..D_802ACF68) is defined below too (87D80, 8A7A0 and 6BBE0
 * use some of it); the cut reaches it.
 */
#include "common.h"
#include "m2c.h"
#include "grid.h"
#include "player.h"
#include "48600.h"
#include "4DE50.h"
#include "4E030.h"
#include "4ED50.h"
#include "530C0.h"
#include "53D70.h"
#include "54E40.h"
#include "5E3F0.h"
#include "86680.h"
#include "87D80.h"
#include "8A7A0.h"
#include "949A0.h"
#include "9BFD0.h"
#include "9CAB0.h"
#include "A4CE0.h"
#include "426E0.h"
#include "ACA00.h"

#define M2C_FIELD(expr, type, offset) (*(type)((u8 *)(expr) + (offset)))
#define M2C_BITWISE(type, expr) ((type)(expr))
#define FLAG_SET(x, f) ((((x) & (f)) == (f)) ? 1 : 0)
#define FLAG_CLEAR(x, f) (!((x) & (f)) ? 1 : 0)

extern UnkStruct_800AEDF0 D_800AEDF0[5];

#ifdef TARGET_PC
/*
 * Entry -1: D_802AC894 is -1 when no entry is current, and func_80243DEC
 * (func_80243D90) and the scripts' func_80243D2C index D_800AEDF0 with it
 * all the same (the game's own bug; the scenarios reach both). On the N64
 * entry -1's unk68 is the word 0x60 bytes before the array,
 * D_800AED78[2].unk0 (5E090.c's: a pointer, NULL then), and its unk14 the
 * word 0xB4 before it, 0x800AED3C: in the bytes after D_800A7F30[100] that
 * nothing names or writes, 0 (the work area is cleared at boot; the
 * emulator's RDRAM has 0 there in every scenario). Where pointers are 8
 * bytes the entries and D_800AED78's elements are bigger, so the same
 * index reaches other words; a host reaches the N64's (on a 32-bit host
 * the same words as the original's index).
 */
typedef struct {
    /* 0x00 */ void *unk0;
    /* 0x04 */ void *unk4;
    /* 0x08 */ s32 unk8;
} Port_800AED78;
extern Port_800AED78 D_800AED78[];
#endif

typedef struct {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ f32 unkC;
#ifdef TARGET_PC
    /* the s32 at 0x18 (the players' unk18, stored whole elsewhere), whose
       low half IDO's code reads: the word in the host's order */
    /* 0x10 */ u8 unk10[0x18 - 0x10];
    /* 0x18 */ s32 unk1A;
#else
    /* 0x10 */ u8 unk10[0x1A - 0x10];
    /* 0x1A */ s16 unk1A;
#endif
    /* 0x1C */ u8 unk1C[0x24 - 0x1C];
    /* 0x24 */ s32 unk24;
#ifdef TARGET_PC
    /* (UnkStruct_800AEDF0's pointers, as wide as the host's) */
    /* 0x28 */ void *unk28;
    /* 0x2C */ UnkStructA *unk2C;
    /* 0x30 */ intptr_t unk30;
    /* 0x34 */ u8 unk34[0x40 - 0x34];
    /* 0x40 */ intptr_t unk40;
    /* 0x44 */ u8 unk44[4];
    /* 0x48 */ s32 unk48;
    /* 0x4C */ intptr_t unk4C;
#else
    /* 0x28 */ u8 unk28[4];
    /* 0x2C */ UnkStructA *unk2C;
    /* 0x30 */ u8 unk30[0x40 - 0x30];
    /* 0x40 */ s32 unk40;
    /* 0x44 */ u8 unk44[4];
    /* 0x48 */ s32 unk48;
    /* 0x4C */ s32 unk4C;
#endif
    /* 0x50 */ s32 unk50;
    /* 0x54 */ u8 unk54[0x60 - 0x54];
    /* 0x60 */ f32 unk60;
    /* 0x64 */ f32 unk64;
    /* 0x68 */ u8 unk68[0xA4 - 0x68];
    /* 0xA4 */ s32 unkA4;
    /* 0xA8 */ s32 unkA8;
#ifdef TARGET_PC
    /* 0xAC */ u8 unkAC[4];
    /* 0xB0 */ intptr_t unkB0;
    /* 0xB4 */ intptr_t unkB4;
    /* 0xB8 */ u8 unkB8[0xC8 - 0xB8];
#else
    /* 0xAC */ u8 unkAC[0xC8 - 0xAC];
#endif
} E88;

typedef struct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
} Res58;

typedef struct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ f32 unkC;
} Ctl58;

#define WRAP360(x) (((x) >= 360.0f) ? (x) - 360.0f : (((x) < 0.0f) ? (x) + 360.0f : (x)))

typedef struct {
    s32 v[8];
} Tbl8i_5E3F0;

typedef struct {
    f32 v[8];
} Tbl8f_5E3F0;

typedef struct {
    s32 v[4];
} Tbl4_5E3F0;
typedef struct { s32 v[5]; } Tbl5_5E3F0;

typedef struct {
    s32 v[3];
} Tbl3_5E3F0;
typedef struct { s32 x, y, z, w; f32 h; PORT_PAD(4) } Cell5_5E3F0; /* (func_8026DCAC writes 6 words) */


void free(void *);
PORT_RET(s32, void) func_8022979C(PORT_RET(s32, void *), s32, s32);
PORT_RET(s32, void) func_8022984C(PORT_RET(s32, void *), s32);
PORT_RET(s32, void) func_80229F74(PORT_RET(s32, void *), s32);
UnkStructA *func_8022F550(UnkStructA *arg0, s32 arg1);
PORT_RET(void, s32) func_8022FC90(void *);
void * func_80231274(s32, s32, s32, void *);
PORT_RET(s32, void) func_80232AA8(PORT_RET(s32, void *));
PORT_RET(s32, void) func_80232AC8(PORT_RET(s32, void *), PORT_RET(s32, void *));
s32 func_80232D18(UnkStructA *arg0, s32 arg1);
s32 func_80238A10(void *, PORT_IP(s32), PORT_RET(s32, void *), s32, s32);
PORT_RET(void, s32) func_8023903C(s32);
void func_80239094(void);
UnkStruct_8023A834 *func_8023A834(void);
PORT_RET(s32, void) func_80243824(s32);
void func_802506B8(u8 *);
PORT_RET(s32, u8) func_8026DC74(s32);
f32 func_8026E938(UnkStructA_40 *arg0);
PORT_IP(s32) func_80270714(s32);
PORT_IP(s32) func_80270770(s16, s16);
PORT_RET(s32, s16) func_802712A8(s32 *, s32);
PORT_RET(s32, void) func_802712C4(PORT_RET(s32, void *));
PORT_RET(s32, void) func_80271BB8(s32);
PORT_RET(s32, void) func_802741D8(s16);
PORT_RET(s32, s16) func_80274F64(s32);
PORT_IP(s32) func_80274F8C(s16, f32, f32, f32, s32);
PORT_RET(s32, void) func_80275C7C(s32, PORT_RET(s32, void *), s32);
PORT_RET(s32, void) func_8027750C(s32);
PORT_RET(s32, void) func_802775D8(PORT_RET(s32, void *));
PORT_RET(s32, void *) func_8027799C(s32);
PORT_RET(s32, void) func_80278E84(s32, s32);
PORT_RET(s32, void) func_8027B0BC(s16);
void func_8027F820(void *);
void func_8027F858(PORT_IP(s32), s32);
void * func_802867E4(s32, f32, f32, f32);
PORT_RET(s32, void) func_80287008(s32, s32, s32);
s32 func_8028A7CC(s32, f32 *, PORT_IP(s32));
PORT_RET(void, s32) func_8028A81C(s32, f32, f32, f32, PORT_IP(s32));
s32 func_802909FC(void *, void *);
void func_80291944(void *, s32, f32);
void func_80297D30(s32, void *);
void * malloc(s32);

extern s32 D_800AF1D8[PORT_SIZE(255)];
extern s32 D_800AF5D8[PORT_SIZE(255)];
extern s32 D_800AF9D8[PORT_SIZE(255)];
extern s32 D_800AFDD8[PORT_SIZE(255)];
extern s32 D_800B01D8[PORT_SIZE(255)];
extern s32 D_800B05D8[PORT_SIZE(255)];
extern s32 D_800B09D8[PORT_SIZE(255)];
extern s32 D_800B0DD8[PORT_SIZE(255)];
extern s32 D_800B11D8[PORT_SIZE(255)];
extern s32 D_800B15D4[PORT_SIZE(1)];
extern u8 D_800B15D8;
extern s32 D_802A01B0;
extern s32 D_802AC5D0;
/* .bss (0x802AC890-0x802ACFB0, from 5E090's to 6BBE0's), in address
 * order: IDO puts the arrays 8-aligned (the words before D_802AC8D0,
 * D_802AC968, ... are that padding) */
s32 D_802AC890;
s32 D_802AC894;
s32 D_802AC898;
s32 D_802AC89C;
s32 D_802AC8A0;
s32 D_802AC8A4;
s32 D_802AC8A8[5];
s32 D_802AC8BC;
s32 D_802AC8C0;
s32 D_802AC8C4;
s32 D_802AC8C8;
s32 D_802AC8D0[5];
s32 D_802AC8E8[5];
s32 D_802AC900[5];
s32 D_802AC918[5];
s32 D_802AC930[5];
s32 D_802AC948[5];
s32 D_802AC95C;
s32 D_802AC960;
s32 D_802AC968[5];
s32 D_802AC97C;
s32 D_802AC980[5];
s32 D_802AC998[5];
s32 D_802AC9B0[5];
s32 D_802AC9C8[5];
s32 D_802AC9DC;
s32 D_802AC9E0;
s32 D_802AC9E4;
s32 D_802AC9E8;
s32 D_802AC9EC;
s32 D_802AC9F0;
s32 D_802AC9F4;
s32 D_802AC9F8[5];
s32 D_802ACA10[5];
f32 D_802ACA24;
f32 D_802ACA28;
f32 D_802ACA2C;
f32 D_802ACA30[5];
f32 D_802ACA44;
f32 D_802ACA48[5];
f32 D_802ACA60[5];
f32 D_802ACA78[5];
f32 D_802ACA8C;
f32 D_802ACA90[255];
f32 D_802ACE90[5];
f32 D_802ACEA4;
f32 D_802ACEA8;
f32 D_802ACEAC;
f32 D_802ACEB0;
f32 D_802ACEB4;
f32 D_802ACEB8;
f32 D_802ACEBC;
f32 D_802ACEC0;
f32 D_802ACEC4;
s32 D_802ACEC8[5][3];
void *D_802ACF04;
UnkStruct_802ACF08 D_802ACF08[5];
UnkStructA **D_802ACF58;
void *D_802ACF5C;
UnkStruct_802AC610 *D_802ACF60;
f32 D_802ACF68[5][3];

void func_80243D20(s32 arg0);
s32 *func_80243D54(s32 arg0);
UnkStruct_802ACF08 *func_80243D7C(s32 arg0);
void func_80243D90(s32 arg0, s32 arg1, s32 arg2);
s32 func_80243E18(s32 arg0);
s32 func_80243E3C(void);
void func_80243E60(void);
void func_80243F2C(s32 arg0, s32 arg1);
s32 func_80243F50(f32 arg0);
s32 func_802442D0(s32 arg0);
s32 func_802442F8(void);
void func_80244308(void *arg0);
void func_80244354(s32 arg0, s32 arg1, s32 arg2);
void func_802443A0(s32 arg0);
s32 func_802443CC(s32 arg0);
UnkStructA *func_802443F4(s32 arg0);
f32 func_8024441C(s32 arg0, s32 arg1);
void func_80244BE0(s32 arg0, s16 arg1);
void func_802450DC(s32 arg0);
s32 func_80245150(s32 arg0);
s32 func_802452AC(s32 arg0, s32 arg1);
s32 func_80245430(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
s32 func_80245480(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);
void func_802454F8(s32 arg0, s32 *arg1);
void func_8024557C(s32 *arg0);
s32 func_802455A8(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
s32 func_80245690(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
s32 func_802456FC(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
void func_802457D8(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
void func_80245CDC(void);
void func_80245DFC(void);
void func_80245E1C(s32 arg0, M2C_UNK arg1, M2C_UNK arg2);
void func_80245E2C(s32 arg0, s32 arg1);
void func_80245F74(s32 arg0);
void func_80245FA0(s32 arg0, s32 arg1);
void func_80245FD4(void);
void func_80246018(void);
void func_80246078(void);
void func_80246604(void);
void func_8024663C(void);
void func_80246808(void);
void func_80246938(void);
void func_80247614(void);
void func_80247848(s32 arg0, s32 arg1);
void func_80247928(s32 arg0, PORT_IP(s32) arg1);
void func_8024796C(s32 arg0, s32 arg1);
void func_802479F0(s32 *arg0);
void func_80247A54(s32 arg0, PORT_IP(s32) arg1, s32 arg2, s32 arg3);
void func_80247CD0(s32 arg0, s32 arg1);
void func_80247D74(s32 arg0, f32 arg1, f32 arg2, s32 arg3, UnkStructA *arg4);
void func_80247F90(s32 arg0);
void func_80248148(s32 arg0, s32 arg1);
void func_8024857C(s32 arg0);
void func_8024878C(s32 arg0);
void func_8024891C(s32 arg0);
void func_8024946C(s32 arg0, s32 arg1);
void func_80249648(s32 arg0, UnkStructA *arg1, s32 arg2);
void func_802497D8(s32 arg0);
void func_80249810(s32 arg0, PORT_IP(s32) arg1, s32 arg2);
void func_802498D4(s32 arg0, f32 arg1);
void func_80249A30(s32 arg0);
void func_80249AD4(s32 arg0);
void func_80249D30(s32 arg0);
void func_80249D74(s32 arg0);
void func_80249E30(s32 arg0);
void func_80249EEC(s32 arg0);
void func_80249F24(s32 arg0);
void func_80249F5C(s32 arg0);
void func_80249FD4(s32 arg0, s32 arg1);
void func_8024A0DC();
void func_8024A6B4(s32 arg0, f32 arg1);
void func_8024A824(s32 arg0, s32 arg1, f32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);
void func_8024ABA8(s32 arg0);
void func_8024AF24(s32 arg0, f32 *arg1);
void func_8024AFD4(s32 arg0);
void func_8024B028(s32 arg0);
void func_8024B1CC(s32 arg0);
void func_8024B444(s32 arg0, f32 *arg1);
void func_8024BBA4(s32 arg0);
void func_8024BC20(s32 arg0);
void func_8024BC94(s32 arg0);
void func_8024BD1C(s32 arg0);
void func_8024BF04(s32 arg0);
void func_8024BFE0(s32 arg0);
void func_8024C04C(s32 arg0);
void func_8024C0D8(s32 arg0);
void func_8024C1E8(s32 arg0);
void func_8024C29C(s32 arg0);
void func_8024C314(s32 arg0);
void func_8024C334(s32 arg0, PORT_IP(s32) arg1, u16 arg2, f32 *arg3);
PORT_IP(s32) func_8024C4EC(UnkStructA *obj, s32 arg1);
void func_8024C53C(s32 arg0, void *arg1);
void func_8024C804(s32 arg0);
void func_8024C884(s32 arg0, f32 arg1, s32 arg2);
void func_8024CC58(s32 arg0);
void func_8024CEB8(s32 arg0);
void func_8024D108(s32 arg0, s32 arg1);
void func_8024D20C(s32 arg0);
void func_8024D374(s32 arg0);
s32 func_8024D41C(f32 key);
s32 func_8024D50C(void *arg0);
void func_8024D540(s32 arg0, f32 *arg1);
void func_8024D5D0(Ctl58 *arg0);
void func_8024D8C4(Ctl58 *arg0);
void func_8024E088(s32 arg0);
void func_8024FFF0(void);
void func_802500D4(void);
s32 func_80250398(s32 arg0);
void func_8025068C(s32 arg0);
void func_802506B8(u8 *str);
#ifdef TARGET_PC
void func_80250DC8(s32 arg0);
#else
void func_80250DC8(void);
#endif
void func_80250E2C(s32 arg0);
void func_802510C8(void);
void func_80251134(s32 arg0);
void func_802511C4(void);
void func_802512F8(void);
void func_80251368(s32 arg0);
s32 func_802513BC(void);
void func_802513CC(void);

/* .data (0x190, from the 16-byte boundary after 5E090's table D_802A0D50
 * and its pad to 6BBE0's D_802A0EF0): ten floats nothing references, the
 * id lists and angle tables the functions copy into locals, and the table
 * of functions handed to func_80297D30. */
f32 D_802A0D60[10] = {
    6.0f, 12.0f, 18.0f, 24.0f, 30.0f, -8.0f, -16.0f, -24.0f, -32.0f, -40.0f,
};
s32 D_802A0D88[3] = { 0x1A2, 0x1A3, -1 };
s32 D_802A0D94[4] = { 0x190, 0x191, 0x192, -1 };
s32 D_802A0DA4[4] = { 0x18D, 0x18E, 0x18F, -1 };
s32 D_802A0DB4[5] = { 0x193, 0x194, 0x195, 0x1F, -1 };
s32 D_802A0DC8[8] = { 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0 };
f32 D_802A0DE8[8] = { 180.0f, 135.0f, 90.0f, 45.0f, 0.0f, 315.0f, 270.0f, 225.0f };
s32 D_802A0E08[4] = { 0, 0x200, 0x400, 0x800 };
s32 D_802A0E18[8] = { 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0 };
f32 D_802A0E38[8] = { 180.0f, 135.0f, 90.0f, 45.0f, 0.0f, 315.0f, 270.0f, 225.0f };
void *D_802A0E58[] = {
    func_80245CDC, func_802512C0, func_8024557C, func_802457D8, func_802456FC,
    func_802455A8, func_80243DDC, func_80250338, func_802452E8, func_80245F74,
    func_8024A824, func_802506B8, func_802511C4, func_80250E2C, func_802450DC,
    func_80250DC8, func_80245DFC, func_802511EC, func_802443F4, func_80245280,
    func_80243F50, func_802443A0, func_80244308, func_802442F8, func_8024585C,
    func_802442D0, func_802442A8, func_80243F2C, func_802444B8, func_8025068C,
    func_80243DEC, func_80243E60, func_802510C8, func_80243D7C, func_80243D54,
    func_80243D2C, func_80243D20, func_802441AC,
};

void func_802513CC() {
    D_802AC890 = 0;
}

s32 func_802513BC() {
    return D_802AC890;
}

void func_80251368(s32 arg0) {
    void *temp_v0;

    temp_v0 = PORT_FIELD((func_802443F4(arg0)), void **, 0x40, Port_UnkStructA, unk40);
    func_8026EF50(M2C_FIELD(temp_v0, f32 *, 0x10), M2C_FIELD(temp_v0, f32 *, 0x14), M2C_FIELD(temp_v0, f32 *, 0x18));
}

void func_802512F8() {
    s32 id;
    UnkStructA *obj;

    id = func_80243DDC();
    obj = func_802443F4(id);
    obj->unk40->unk10 = D_800AEDF0[id].unk0;
    obj->unk40->unk14 = D_800AEDF0[id].unk4;
    obj->unk40->unk18 = D_800AEDF0[id].unk8;
    obj->unk40->unk20 = D_800AEDF0[id].unkC;
}

void func_802512C0(s32 arg0, s32 arg1, s32 arg2) {
    func_802511EC(func_80243DDC(), arg0, arg1, arg2);
}

void func_802511EC(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    f32 pos[3];
    UnkStructA *obj;

    func_8026E338(arg1, arg2, arg3, pos, 0);
    obj = func_802443F4(arg0);
    if (obj != NULL) {
        obj->unk40->unk10 = pos[0];
        obj->unk40->unk14 = pos[1];
        obj->unk40->unk18 = pos[2];
        D_800AEDF0[arg0].unk0 = -10000.0f;
        D_800AEDF0[arg0].unk4 = -10000.0f;
        D_800AEDF0[arg0].unk8 = -10000.0f;
    } else {
#ifdef TARGET_PC
        /* Entry -1 (above, func_802443F4): the N64 writes the words
         * 0x800AED28-0x800AED30, which nothing names; only
         * func_802512F8 reads entry -1's, through its NULL object. */
        if (arg0 == -1) {
            return;
        }
#endif
        D_800AEDF0[arg0].unk0 = pos[0];
        D_800AEDF0[arg0].unk4 = pos[1];
        D_800AEDF0[arg0].unk8 = pos[2];
    }
}

void func_802511C4() {
    func_80237664();
    func_802362D4(0x1F);
}

void func_80251134(s32 arg0) {
    if (func_80244BA4(arg0, 0x1000)) {
        func_80247A54(arg0, (PORT_IP(s32)) func_8022F4AC(func_802443F4(arg0), 2, PORT_PTR(D_800AEDF0[arg0].unk4C)), 0, 0);
        func_80244354(arg0, 0x1020, 1);
    }
}

void func_802510F0() {
    s32 i;

    for (i = 0; i < 5; i++) {
        func_80247848(i, 0);
    }
}

void func_802510C8() {
    func_80247C50(func_80243DDC());
}

void func_80250E2C(s32 arg0) {
    s32 id;
    Unk8026DCAC ipos;
    f32 fpos[3];
    f32 cur[3];
    f32 tgt[3];
    f32 dist;
    f32 dir[3];
    UnkStructA *obj;
    s32 angle;

    id = func_80243DDC();
    obj = func_802443F4(id);
    func_8022F808(obj, cur);
    func_80247C50(id);
    func_8026DCAC(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, &ipos, 2);
    func_8026E338(ipos.i[0], ipos.i[1], ipos.i[2], fpos, 0);
    D_802ACA10[id] = 0;
    tgt[0] = fpos[0];
    tgt[1] = fpos[1];
    tgt[2] = fpos[2];
    dist = func_8022DF68(cur, tgt);
    if (dist > 0.0f) {
        func_8022FF58(cur, tgt, dir);
        dir[1] -= 90.0f;
        angle = func_80243F50(dir[0]);
        D_802AC8C0 = D_802AC8BC = angle;
        D_802ACA24 = 4.0f;
        D_802AC95C = 3;
        D_802AC968[id] = 1;
        func_8026DCAC(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, &ipos, 2);
        func_8026E338(ipos.i[0], ipos.i[1], ipos.i[2], fpos, 0);
        D_802ACA28 = fpos[0];
        D_802ACA2C = fpos[1];
        D_802ACA8C = fpos[2];
        func_80244354(id, 0x10420, 1);
        D_802AC918[id] = 0;
        func_802313DC(obj, 0x40000);
        if (arg0 != 0) {
            while (func_80244BA4(id, 0x400)) {
                func_80239094();
            }
        }
    }
}

/* ZeroJump 0x100F: its callers pass a flag, which it hands on to
 * func_8024A0DC in $a0 */
#ifdef TARGET_PC
void func_80250DC8(s32 arg0) {
#else
void func_80250DC8() {
#endif
    s32 var_v1;

    var_v1 = func_80243EB4(func_80243DDC(), 0x8000);
    if (var_v1 == 0) {
#ifdef TARGET_PC
        func_8024A0DC(arg0);
#else
        func_8024A0DC();
#endif
    }
}

void func_802506B8(u8 *str) {
    s32 wait;
    u8 *s;
    s32 id = func_80243DDC();
    s32 n;
    s32 c;

    for (n = 0; n < 255; n++) {
        D_800AF1D8[n] = -1;
        D_800AF5D8[n] = -1;
        D_802ACA90[n] = -1.0f;
        D_800AF9D8[n] = -1;
        D_800AFDD8[n] = -1;
        D_800B01D8[n] = -1;
        D_800B05D8[n] = -1;
        D_800B09D8[n] = -1;
        D_800B0DD8[n] = -1;
        D_800B11D8[n] = -1;
    }
    s = str;
    n = 0;
    if (*s != 0) {
        do {
            c = *s;
            D_800AF1D8[n] = c;
            switch (*s) {
            case 'W':
            case 'w':
                switch (*++s) {
                case 'D':
                case 'd':
                    switch (*++s) {
                    case 'N':
                    case 'n':
                        switch (s[1]) {
                        case 'E':
                        case 'e':
                            D_800AF5D8[n] = 0x20;
                            s++;
                            break;
                        case 'W':
                        case 'w':
                            D_800AF5D8[n] = 0xE0;
                            s++;
                            break;
                        default:
                            D_800AF5D8[n] = 0;
                            break;
                        }
                        break;
                    case 'E':
                    case 'e':
                        D_800AF5D8[n] = 0x40;
                        break;
                    case 'S':
                    case 's':
                        switch (s[1]) {
                        case 'E':
                        case 'e':
                            D_800AF5D8[n] = 0x60;
                            s++;
                            break;
                        case 'W':
                        case 'w':
                            D_800AF5D8[n] = 0xA0;
                            s++;
                            break;
                        default:
                            D_800AF5D8[n] = 0x80;
                            break;
                        }
                        break;
                    case 'W':
                    case 'w':
                        D_800AF5D8[n] = 0xC0;
                        break;
                    case 'U':
                    case 'u':
                        D_800AF5D8[n] = 0x100;
                        break;
                    case 'D':
                    case 'd':
                        D_800AF5D8[n] = 0x200;
                        break;
                    }
                    break;
                case 'S':
                case 's':
                    switch (*++s) {
                    case 'L':
                    case 'l':
                        D_802ACA90[n] = 2.0f;
                        break;
                    case 'M':
                    case 'm':
                        D_802ACA90[n] = 4.0f;
                        break;
                    case 'H':
                    case 'h':
                        D_802ACA90[n] = 8.5f;
                        break;
                    case 'S':
                    case 's':
                        D_802ACA90[n] = 1.1111f;
                        break;
                    }
                    break;
                case 'A':
                case 'a':
                    switch (*++s) {
                    case 'O':
                    case 'o':
                        D_800AF9D8[n] = 1;
                        break;
                    case 'A':
                    case 'a':
                        D_800AF9D8[n] = 2;
                        break;
                    case 'P':
                    case 'p':
                        D_800AF9D8[n] = 3;
                        break;
                    case 'B':
                    case 'b':
                        D_800AF9D8[n] = 4;
                        break;
                    case 'D':
                    case 'd':
                        D_800AF9D8[n] = 5;
                        break;
                    }
                    break;
                case 'M':
                case 'm':
                    s++;
                    if (*s == '1') {
                        D_800B01D8[n] = 1;
                    } else {
                        D_800B01D8[n] = 0;
                    }
                    break;
                case 'G':
                case 'g':
                    s++;
                    if (*s == '1') {
                        D_800B05D8[n] = 1;
                    } else {
                        D_800B05D8[n] = 0;
                    }
                    break;
                case 'B':
                case 'b':
                    if (s[1] >= '0' && s[1] <= '9') {
                        D_800AFDD8[n] = 0;
                    }
                    while (s[1] >= '0' && s[1] <= '9') {
                        s++;
                        D_800AFDD8[n] *= 10;
                        D_800AFDD8[n] += *s - '0';
                    }
                    n++;
                    break;
                }
                break;
            case 'S':
            case 's':
                D_800B09D8[n] = s[1];
                c = *++s;
                if (c == 'T' || c == 't') {
                    if (s[1] >= '0' && s[1] <= '9') {
                        D_800B0DD8[n] = 0;
                    }
                    while (s[1] >= '0' && s[1] <= '9') {
                        s++;
                        D_800B0DD8[n] *= 10;
                        D_800B0DD8[n] += *s - '0';
                    }
                    n++;
                }
                break;
            case 'M':
            case 'm':
                D_800B11D8[n] = *++s;
                n++;
                break;
            }
            c = *++s;
        } while (c != 0);
    }
    D_800AF1D8[n] = -2;
    func_80244354(id, 0x800, 1);
    D_802AC960 = 0;
    wait = func_80243EB4(id, 0x8000);
    if (wait) {
        while (func_80244BA4(id, 0x800) || func_80244BA4(id, 0x80000) || func_80244BA4(id, 0x400)) {
            func_80239094();
        }
    } else {
        while (func_80244BA4(id, 0x800)) {
            func_80239094();
        }
    }
}

void func_8025068C(s32 arg0) {
    D_800AEDF0[func_80243DDC()].unk80 = arg0;
}

s32 func_80250398(s32 arg0) {
    s32 ret = 0;
    UnkStructA *obj = func_802443F4(arg0);
    Unk8026DCAC c;
    s32 dx;
    s32 dz;

    if ((obj->unk40->unk20 >= 0.0 && obj->unk40->unk20 < 22.5) || (obj->unk40->unk20 >= 337.5 && obj->unk40->unk20 < 360.0)) {
        dx = 0;
        dz = 1;
    }
    if (obj->unk40->unk20 >= 22.5 && obj->unk40->unk20 < 67.5) {
        dx = 1;
        dz = 1;
    }
    if (obj->unk40->unk20 >= 67.5 && obj->unk40->unk20 < 112.5) {
        dx = 1;
        dz = 0;
    }
    if (obj->unk40->unk20 >= 112.5 && obj->unk40->unk20 < 157.5) {
        dx = 1;
        dz = -1;
    }
    if (obj->unk40->unk20 >= 157.5 && obj->unk40->unk20 < 202.5) {
        dx = 0;
        dz = -1;
    }
    if (obj->unk40->unk20 >= 202.5 && obj->unk40->unk20 < 247.5) {
        dx = -1;
        dz = -1;
    }
    if (obj->unk40->unk20 >= 247.5 && obj->unk40->unk20 < 292.5) {
        dx = -1;
        dz = 0;
    }
    if (obj->unk40->unk20 >= 292.5 && obj->unk40->unk20 < 337.5) {
        dx = -1;
        dz = 1;
    }
    func_8026DCAC(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, &c, 2);
    if (!(func_8026D7D8(c.i[0] + dx, c.i[1], c.i[2] + dz) & 0x4000)) {
        ret = 1;
    }
    return ret;
}

void func_80250338(s32 arg0, void *arg1) {
    void *temp_v1;

    temp_v1 = (func_802443F4(arg0));
    M2C_FIELD(arg1, f32 *, 0) = (f32) M2C_FIELD(PORT_FIELD(temp_v1, void **, 0x40, Port_UnkStructA, unk40), f32 *, 0x10);
    M2C_FIELD(arg1, f32 *, 4) = (f32) M2C_FIELD(PORT_FIELD(temp_v1, void **, 0x40, Port_UnkStructA, unk40), f32 *, 0x14);
    M2C_FIELD(arg1, f32 *, 8) = (f32) M2C_FIELD(PORT_FIELD(temp_v1, void **, 0x40, Port_UnkStructA, unk40), f32 *, 0x18);
}

void func_802502BC(s32 arg0) {
    UnkStructA *obj;

    obj = func_802443F4(arg0);
    func_80243D90(arg0, 0x90, 1);
    D_800AEDF0[arg0].unk78 = 1200;
    func_80231408(obj, 0x40);
    func_80246C3C(arg0, 1, 1);
}

void func_802500D4() {
    UnkStructA *o;
    s32 kind;
    UnkStructA *objs[4];
    void * made;
    f32 dist = 50.0f; /* the spawn distance (its frame slot; a guess among held values) */
    Tbl4_5E3F0 tbl;
    f32 pos[3];
    Vec3f off;
    s32 i;
    s32 r;
    UnkStructA *obj;

    tbl = *(Tbl4_5E3F0 *)D_802A0E08;
    for (i = 0; i < 4; i++) {
        objs[i] = NULL;
        if (D_800AEDF0[i].unk14 != 0 && D_800AEDF0[i].unk24 != 0) {
            r = func_80234248(3);
            if (r >= 4) {
                DEBUG_PRINT(("r >= 4\n"));
            }
            if (r != 0) {
                obj = D_800AEDF0[i].unk28;
                pos[2] = dist;
                pos[0] = obj->unk40->unk20;
                pos[1] = 0.0f;
                func_8022FEAC(pos, &off.x);
                kind = tbl.v[r];
                made = func_80231274(-1, kind, 0x60, NULL);
                objs[i] = (UnkStructA *)made;
                objs[i]->unk40->unk10 = obj->unk40->unk10 + off.x;
                objs[i]->unk40->unk14 = obj->unk40->unk14 + off.y;
                objs[i]->unk40->unk18 = obj->unk40->unk18 + off.z;
                func_8022E884((PORT_RET(s32, void *)) objs[i], (PORT_RET(s32, void *)) obj);
            }
        }
    }
    func_80239094();
    for (i = 0; i < 4; i++) {
        o = objs[i];
        if (o != NULL) {
            func_8022FC90(o);
        }
    }
    func_80239430();
}

void func_8024FFF0() {
    UnkStruct_800AEDF0 *p;
    s32 i;

    for (;;) {
        for (i = 0; i != 5; i++) {
            if ((D_800AEDF0[i].unk24 == 1 || D_800AEDF0[i].unk24 == 2) && D_800AEDF0[i].unk28 != NULL) {
                func_8024E088(i);
            }
        }
        if (func_80243E18(1)) {
            func_80247614();
        }
        func_80239094();
    }
}

#define EN ((E88 *)&D_800AEDF0[arg0])
void func_8024E088(s32 arg0) {
    u16 sp126;
    u16 sp124;
    s32 sp120;
    s32 sp11C;
    UnkStructA *sp118;
    s32 sp114;
    s32 sp110;
    s32 sp10C;
    s32 sp108;
    s32 sp104;
    s32 sp100;
    s32 spFC;
    UnkStruct_802AC610 *spF8;
    s32 spF4;
    s32 spF0;
    Unk8026DCAC spD8;
    f32 spD4;
    f32 spD0;
    f32 spCC;
    f32 spC8;
    f32 spC4;
    f32 spC0;
#ifdef TARGET_PC
    /* spB4[0] is read unset where the stick is let go (func_80239880 sets
       only [2]) and func_8024C334 keeps it; sp74 is passed to it unset
       (sp74's only store before the call is in a branch): on the N64 each
       slot holds what the call before left there (one task calls this for
       each player in turn, and nothing between the calls reaches that
       deep) */
    static f32 spB4[3];
#else
    f32 spB4[3];
#endif
    s32 spB0;
    s32 spAC;
    f32 ang;
    f32 spA4;
    f32 spA0;
    f32 sp9C;
    f32 sp90[3];
    f32 sp84[3];
    f32 sp78[3];
#ifdef TARGET_PC
    static UnkStructA *sp74; /* (spB4's comment) */
#else
    UnkStructA *sp74;
#endif
    UnkStructA *obj;
    UnkStructA *sp6C;
    s32 *sp68;
    Res58 sp58;
    UnkStructA *r;

    obj = func_802443F4(arg0);
    if (obj->unk40->unk10 != -10000.0f) {
        sp104 = func_802441AC(obj);
        if (sp104 == arg0) {
            if (obj->unk58 == obj->unk40->unk10) {
                if (obj->unk40->unk14 == obj->unk5C) {
                    if (obj->unk40->unk18 == obj->unk60) {
                        sp120 = 0;
                    }
                }
            }
        }
        EN->unkC = obj->unk40->unk20;
        if (func_80244BA4(arg0, 0x200000)) {
            func_802733C8(obj);
        }
        func_8026DCAC(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, &spD8, 2);
        sp10C = func_8026DC74(spD8.i[1]);
        D_802AC8A4 = sp10C;
        sp11C = func_8026D7D8(spD8.i[0], spD8.i[1], spD8.i[2]);
        if (func_80244BA4(arg0, 0x400) && EN->unk48 == 0) {
            sp11C = 1;
        }
        if ((obj->unk10 & 0x20) && !func_80244BA4(arg0, 0x10000)) {
            func_80248148(arg0, 2);
        }
        if (((obj->unk10 & 0x40) || (obj->unk10 & 0x80)) && !func_80244BA4(arg0, 0x10000)) {
            func_80248148(arg0, 1);
        }
        if ((obj->unk10 & 0x100) && !func_80244BA4(arg0, 0x10000)) {
            func_80248148(arg0, 0);
        }
        sp108 = D_802AC5D0 & 4;
        D_802ACF5C = func_8023A834();
        if (sp108 && ((s32 *)D_802ACF5C)[0x2C / 4] == 0 && ((s32 *)D_802ACF5C)[0x30 / 4] == 0x3B &&
            func_80243EB4(arg0, 0x200)) {
            if (obj->unk44 != NULL && !func_80244BA4(arg0, 0x1000)) {
                sp100 = func_802441AC(obj->unk44);
                func_80247F90(sp100);
            }
            func_80248148(arg0, 5);
        }
        if (obj->unk10 & 0x1E00) {
            func_80249648(arg0, obj->unk20, obj->unk10);
        }
        if ((obj->unk10 & 0x40000000) && !func_80244BA4(arg0, 0x1000000)) {
            func_8027F820(obj);
            func_80248148(arg0, 3);
        }
#if VERSION_JP
        {
#else
        if (EN->unk24 != 3) {
#endif
            sp126 = 0;
            sp124 = 0;
            spFC = arg0;
            spB4[2] = 0.0f;
            if (func_80243EB4(arg0, 0x2000)) {
                spFC = EN->unkA4;
                if (EN->unk24 == 1) {
                    func_80239880(arg0, 0, sp90);
                } else {
                    sp90[2] = D_802ACF08[arg0].unk8;
                    sp90[1] = D_802ACF08[arg0].unk4;
                    sp90[0] = D_802ACF08[arg0].unk0;
                }
            }
            if (func_80244BA4(arg0, 0x1000) || obj->unk44 == NULL) {
                if (D_800AEDF0[spFC].unk24 == 1) {
                    func_80239880(spFC, 0, spB4);
                    sp126 = func_80239FF8(spFC);
                    sp124 = func_802399AC(spFC);
                } else {
                    spB4[2] = D_802ACF08[spFC].unk8;
                    spB4[1] = D_802ACF08[spFC].unk4;
                    spB4[0] = D_802ACF08[spFC].unk0;
                    sp126 = D_802ACF08[spFC].unkC;
                    sp124 = D_802ACF08[spFC].unkE;
                }
            }
            if (func_802452AC(arg0, 0x2000C20)) {
                if ((sp126 & 0x4000) && (sp124 & 0x8000) && func_802452AC(arg0, 0x840A182) &&
                    !func_80243EB4(arg0, 8) && !func_80243EB4(arg0, 0x80) && !func_80243EB4(arg0, 0x200) &&
                    !(sp11C & 0x4000) && func_80274994((s16)arg0)) {
                    func_80249FD4(arg0, 1);
                } else {
                    if ((sp124 & 0x8000) && !(sp126 & 0x4000) && func_802452AC(arg0, 0x4000C3) &&
                        !func_80243EB4(arg0, 8) && !func_80243EB4(arg0, 0x80) && !func_80243EB4(arg0, 0x200)) {
                        sp74 = func_8022F550(obj, 2);
                        if (sp74 != NULL && (PORT_IP(s32))sp74 == EN->unk4C) {
                            if (obj->unk6C == 0.0f) {
                                sp74 = func_8022F550(obj, 2);
                                func_80249810(arg0, (PORT_IP(s32))sp74, 2);
                            }
                        } else {
                            r = (UnkStructA *)func_80274F8C(EN->unk1A, obj->unk40->unk10, obj->unk40->unk14,
                                                       obj->unk40->unk18, arg0);
                            if (r != NULL) {
                                func_80247928(arg0, PORT_HOST(r->unk1C, (PORT_IP(s32)) ((Port_Ent88 *) r)->unk1C));
                                func_8026C660(0xA);
                            }
                        }
                    }
                    if ((sp124 & 0x4000) ||
                        (func_80243E18(2) && !(D_802AC9EC & 7) && obj->unk44 == NULL &&
                         func_802452AC(arg0, 0x1180))) {
                        if (!func_80270714((s16)arg0)) {
                            func_802313DC(obj, 1);
                            sp68 = (s32 *)func_80271540((s16)arg0);
#ifdef TARGET_PC
                            /* (nothing found: the N64 ors into the word at 0xC,
                               in ZeroJump's page, which is mapped at 0) */
                            if (sp68 != NULL)
#endif
                            sp68[3] |= 0x10;
                            func_80231408(obj, 1);
                            if (sp68 != NULL) {
                                func_80249FD4(arg0, 0);
                            } else {
                                func_80243824(0xF);
                            }
                        } else {
                            func_80249F5C(arg0);
                        }
                    }
                }
            }
            if (func_802452AC(arg0, 0x2000400)) {
                if (sp124 & 0x10) {
                    PORT_ONLY(if ((s16)func_80274F64((s16)arg0) >= 0)) func_802712C4(func_8027799C((s16)func_80274F64((s16)arg0))); /* entity -1: "Checks and debugging aids" */
                }
                if (sp124 & 0x2000) {
                    func_8027750C((s16)arg0);
                }
            }
            sp120 = 1;
            spD4 = 0.0f;
            if (func_80244BA4(arg0, 0x400) && func_802452AC(arg0, 0x1C0)) {
                func_8024D8C4((Ctl58 *)&sp58);
                spD4 = sp58.unk0;
                sp120 = sp58.unk4;
                if ((D_802AC968[arg0] & 1) == 1 && D_802AC97C == 0) {
                    obj->unk40->unk48->unk4[0].unk4 = 0.0f;
                    obj->unk40->unk48->unk4[1].unk4 = 0.0f;
                    obj->unk40->unk14 += sp58.unkC;
                }
            }
            if (func_80244BA4(arg0, 2)) {
                func_8024D540(arg0, (f32 *)&sp58);
                spD4 = sp58.unk0;
                sp120 = sp58.unk4;
            }
            if (!func_8023372C(spB4[2], 0.001f)) {
                func_80234858(&sp9C, &spA0, &spA4);
                if (spA0 >= 180.0f && spA0 < 360.0f && D_802AC9F4 != 0) {
                    spB4[0] = WRAP360(180.0f - spB4[0]);
                }
                spC8 = spB4[0] + sp9C;
                ang = WRAP360(spC8);
                if (spA0 < 0.0f || spA0 >= 360.0f) {
                    DEBUG_PRINT(("spA0 out of range\n"));
                }
                if (func_802452AC(arg0, 0x2000C20)) {
                    if (spB4[2] > 20.0f && spB4[2] < 35.0f) {
                        spD4 = 2.0f;
                    }
                    if (spB4[2] >= 35.0f && spB4[2] < 60.0f) {
                        spD4 = 4.0f;
                    }
                    if (spB4[2] >= 60.0f) {
                        spD4 = 8.5f;
                    }
                    if (spD4 > 0.0f) {
                        if ((ang >= 0.0 && ang < 22.5) || (ang >= 337.5 && ang < 360.0)) {
                            sp120 = 0x80;
                            spD0 = 0.0f;
                        }
                        if (ang >= 22.5 && ang < 67.5) {
                            spD0 = 45.0f;
                            sp120 = 0x60;
                        }
                        if (ang >= 67.5 && ang < 112.5) {
                            spD0 = 90.0f;
                            sp120 = 0x40;
                        }
                        if (ang >= 112.5 && ang < 157.5) {
                            spD0 = 135.0f;
                            sp120 = 0x20;
                        }
                        if (ang >= 157.5 && ang < 202.5) {
                            spD0 = 180.0f;
                            sp120 = 0;
                        }
                        if (ang >= 202.5 && ang < 247.5) {
                            spD0 = 225.0f;
                            sp120 = 0xE0;
                        }
                        if (ang >= 247.5 && ang < 292.5) {
                            spD0 = 270.0f;
                            sp120 = 0xC0;
                        }
                        if (ang >= 292.5 && ang < 337.5) {
                            spD0 = 315.0f;
                            sp120 = 0xA0;
                        }
                        if (!func_80244BA4(arg0, 0x8000000)) {
                            func_8024A6B4(arg0, spD0);
                        }
                        if (func_80243EB4(arg0, 4)) {
                            spD4 *= 0.5f;
                        }
                        if (func_80243EB4(arg0, 1)) {
                            spD4 *= 4.0f;
                        }
                        if (func_80243EB4(arg0, 0x200)) {
                            spD4 *= 0.8f;
                        }
                        if (func_8024D50C((void *)EN->unk2C)) {
                            spD4 *= 0.8f;
                        }
                    }
                }
            }
            if (func_80243EB4(arg0, 0x20) && !func_80244BA4(arg0, 0x20)) {
                spD4 = 8.5f - spD4;
                if (spD4 < 2.0f) {
                    spD4 = 0.0f;
                }
                if (sp120 == 1) {
                    spC4 = obj->unk40->unk20;
                    sp120 = func_8024D41C(spC4);
                    if (sp120 == -1) {
                        sp120 = 1;
                        spD4 = 0.0f;
                    }
                }
            }
            if (obj->unk40->unk14 - func_8026E938(obj->unk40) > 500.0f) {
                if (!func_80244BA4(arg0, 0x400000) && obj->unk6C > 2.0f && EN->unk50 == 0 &&
                    (obj->unk8 & 0x1000000)) {
                    func_8026C660(0x1B);
                    EN->unk50 = 1;
                }
            }
            if (obj->unk6C > 32.0f && func_802452AC(arg0, 0x60000A)) {
                func_8024D374(arg0);
            }
            if (obj->unk6C == 0.0f && func_80244BA4(arg0, 8)) {
                func_8024D20C(arg0);
            }
            if (func_80244BA4(arg0, 0x4000)) {
                spD4 *= 0.3f;
            }
            if (func_802452AC(arg0, 0x31C2)) {
                if (spD4 > 0.0f && spD4 < 4.0f) {
                    if (!func_80244BA4(arg0, 0x8000)) {
                        if (func_802443CC(arg0) != 1) {
                            func_802444B8(arg0, 1);
                        }
                    } else if (func_802443CC(arg0) != 0xD) {
                        func_802444B8(arg0, 0xD);
                    }
                }
                if (spD4 >= 4.0f && spD4 < 8.5f) {
                    if (!func_80244BA4(arg0, 0x8000)) {
                        if (func_802443CC(arg0) != 2) {
                            func_802444B8(arg0, 2);
                        }
                    } else if (func_802443CC(arg0) != 0xE) {
                        func_802444B8(arg0, 0xE);
                    }
                }
                if (spD4 >= 8.5f) {
                    if (!func_80244BA4(arg0, 0x8000)) {
                        spAC = func_802443CC(arg0);
                        if (spAC != 3) {
                            func_802444B8(arg0, 3);
                        }
                    } else if (func_802443CC(arg0) != 0xF) {
                        func_802444B8(arg0, 0xF);
                    }
                }
                if (spD4 == 0.0f) {
                    sp110 = func_802443CC(arg0);
                    switch (sp110) {
                        case 1:
                        case 2:
                        case 3:
                            func_8024D108(arg0, sp110);
                            func_802444B8(arg0, 0);
                            break;
                        case 13:
                        case 14:
                        case 15:
                            func_8024D108(arg0, sp110);
                            func_802444B8(arg0, 0xC);
                            break;
                    }
                }
                func_8024CEB8(arg0);
            }
            if (obj->unk8 & 0x20000) {
                func_802727D8(PORT_PTR(EN->unk40));
            } else {
                if (func_80244BA4(arg0, 0x80)) {
                    func_8024CC58(arg0);
                }
                if (func_80244BA4(arg0, 0x40)) {
                    func_8024C884(arg0, spB4[2], sp126);
                }
                if (func_80244BA4(arg0, 0x200)) {
                    func_8024C804(arg0);
                }
                if (func_80244BA4(arg0, 0x100)) {
                    func_8024C53C(arg0, D_802ACF68[arg0]);
                }
                if (!func_80244BA4(arg0, 0x2000000) && func_80244BA4(arg0, 0x1000)) {
                    func_8024C334(arg0, (PORT_IP(s32))sp74, sp124, spB4);
                }
                if (!func_80244BA4(arg0, 0x2000000) && func_80244BA4(arg0, 0x10000)) {
                    func_8024C29C(arg0);
                }
                if (func_80244BA4(arg0, 0x2000)) {
                    func_8024C1E8(arg0);
                }
                if (func_80244BA4(arg0, 0x20000)) {
                    func_8024C0D8(arg0);
                }
                if (func_80244BA4(arg0, 0x8000)) {
                    func_8024C04C(arg0);
                }
                if (func_80244BA4(arg0, 0x4000000)) {
                    func_8024BFE0(arg0);
                }
                if (func_80243EB4(arg0, 0x400)) {
                    func_8024BF04(arg0);
                }
                if (func_80244BA4(arg0, 0x8000000)) {
                    func_8024BD1C(arg0);
                }
                if (func_80243EB4(arg0, 0x10000)) {
                    func_8024BC94(arg0);
                }
                if (func_80243EB4(arg0, 0x20000)) {
                    func_8024BC20(arg0);
                }
                if (func_80243EB4(arg0, 0x200000)) {
                    func_8024BBA4(arg0);
                }
                if (func_80243EB4(arg0, 2)) {
                    func_8024B444(arg0, (f32 *)&sp58);
                    spD4 = sp58.unk0;
                    sp120 = sp58.unk4;
                }
                if (func_80243EB4(arg0, 0x10)) {
                    func_8024B1CC(arg0);
                }
                if (func_80243EB4(arg0, 0x100)) {
                    func_8024B028(arg0);
                }
                if (func_80243EB4(arg0, 0x40)) {
                    func_8024AFD4(arg0);
                }
                if (func_80243EB4(arg0, 0x2000)) {
                    func_8024AF24(arg0, sp90);
                }
                if (func_80244BA4(arg0, 0x800) && !func_80244BA4(arg0, 0x80000)) {
                    if (func_80243EB4(arg0, 0x8000)) {
                        if (!func_80244BA4(arg0, 0x400)) {
                            func_8024ABA8(arg0);
                        }
                    } else {
                        func_8024ABA8(arg0);
                    }
                }
                if (func_80244BA4(arg0, 0x80000) && !func_80244BA4(arg0, 0x400)) {
                    func_80249AD4(D_802AC980[arg0]);
                }
                if (func_80243EB4(arg0, 0x2000)) {
                    sp6C = func_802443F4(EN->unkA8);
                }
                if (func_802452AC(arg0, 0x600000) && sp120 != 1) {
                    func_80291944(obj, sp120, spD4);
                }
                spF4 = sp11C & 0x1C00;
                if (spF4) {
                    func_8027F858((PORT_IP(s32))obj, spF4 >> 10);
                }
                if (func_802452AC(arg0, 0x600000) && EN->unk48 == 1) {
                    func_80230914(obj);
                }
                if (EN->unk2C != NULL) {
                    func_80231434(EN->unk2C);
                }
                if (func_80243EB4(arg0, 0x2000)) {
                    func_80231434(sp6C);
                }
                sp110 = 0;
                D_802AC9E4 = arg0;
                if (EN->unk2C != NULL) {
                    func_8022F808(EN->unk2C, sp84);
                    sp114 = 0;
                    if (EN->unk2C->unk8 & 1) {
                        sp114 = func_802909FC(EN->unk2C, func_80245150);
                    }
                    if (EN->unk2C->unk8 & 2) {
                        sp114 = func_802909FC(EN->unk2C, func_80277378);
                    }
                    if (EN->unk2C->unk8 & 4) {
                        sp114 = func_802909FC(EN->unk2C, func_80245150);
                    }
                    if (EN->unk2C->unk8 & 8) {
                        sp114 = func_802909FC(EN->unk2C, func_8027B480);
                    }
                    if (sp114 != 0) {
                        obj->unk40->unk10 += EN->unk2C->unk40->unk10 - sp84[0];
                        obj->unk40->unk18 += EN->unk2C->unk40->unk18 - sp84[2];
                        spC0 = func_8026EAF8(obj);
                        if (obj->unk40->unk14 < spC0) {
                            func_80244354(arg0, 8, 0);
                            obj->unk6C = 0.0f;
                        }
                        obj->unk40->unk14 = spC0;
                        func_8022F808(obj, sp78);
                        if (func_8022DF68(sp78, &obj->unk58) >= 100.0f) {
                            func_80232BA0(&obj->unk58, obj);
                        }
                        if (func_802909FC(obj, func_80245150)) {
                            if (obj->unk40->unk14 < obj->unk5C && obj->unk68 < obj->unk6C) {
                                func_80244C30(arg0, 1);
                                func_802444B8(arg0, 0);
                            }
                            obj->unk40->unk20 = EN->unkC;
                            func_80244354(arg0, 0x8000000, 0);
                        }
                        sp110 = 1;
                    }
                }
                if (func_80243EB4(arg0, 0x2000)) {
                    sp6C = func_802443F4(EN->unkA8);
                    func_8022F808(sp6C, sp84);
                    spF0 = func_802909FC(sp6C, func_80245150);
                    if (spF0 != 0) {
                        obj->unk40->unk10 += sp6C->unk40->unk10 - sp84[0];
                        obj->unk40->unk18 += sp6C->unk40->unk18 - sp84[2];
                        spC0 = func_8026EAF8(obj);
                        if (obj->unk40->unk14 < spC0) {
                            func_80244354(arg0, 8, 0);
                            obj->unk6C = 0.0f;
                        }
                        obj->unk40->unk14 = spC0;
                        func_8022F808(obj, sp78);
                        if (func_8022DF68(sp78, &obj->unk58) >= 150.0f) {
                            func_80232BA0(&obj->unk58, obj);
                        }
                        if (func_802909FC(obj, func_80245150)) {
                            obj->unk40->unk20 = EN->unkC;
                            func_80244354(arg0, 0x8000000, 0);
                        }
                        sp110++;
                    }
                }
                if (sp110 == 0) {
                    func_802909FC(obj, func_80245150);
                }
                func_80249A30(arg0);
                if (func_80244BA4(arg0, 1)) {
                    func_802498D4(arg0, spD4);
                }
                func_8026E938(obj->unk40);
                spF8 = func_8023A840();
                D_802ACF60 = spF8;
                if (obj->unk5C - obj->unk40->unk14 > 15.0f) {
                    func_80244354(arg0, 0x4000, 1);
                } else if (func_80244BA4(arg0, 0x4000)) {
                    func_80244354(arg0, 0x4000, 0);
                }
#if VERSION_JP
                if (obj->unk40->unk14 < 0.0f && !func_80244BA4(arg0, 0x20)) {
                    func_80244354(arg0, 0x20, 1);
                    func_80270770((s16)arg0, 1);
                }
#endif
                r = func_8022F4AC(obj, 2, PORT_PTR(EN->unk4C));
                if (r != NULL && r->unk44 == NULL && !func_80244BA4(arg0, 1) && obj->unk6C == 0.0f) {
                    func_802497D8(arg0);
                }
                sp118 = func_8022F4AC(obj, 0x1E00, PORT_PTR(EN->unk4C));
                if (sp118 != NULL) {
                    func_80249648(arg0, (UnkStructA *)sp118, sp118->unk8);
                }
                func_8024891C(arg0);
                sp74 = func_8022F4AC(obj, 0x10, PORT_PTR(EN->unk4C));
                if (func_80243EB4(arg0, 0x2000)) {
                    sp6C = func_802443F4(EN->unkA8);
                    if (sp74 == sp6C) {
                        sp74 = NULL;
                    }
                }
                if (sp74 != NULL) {
                    D_802ACF68[arg0][0] = obj->unk40->unk10 - obj->unk58;
                    D_802ACF68[arg0][1] = obj->unk40->unk14 - obj->unk5C;
                    D_802ACF68[arg0][2] = obj->unk40->unk18 - obj->unk60;
                    func_80232BA0(&obj->unk58, obj);
                    if (func_80244BA4(arg0, 2)) {
                        func_8024878C(arg0);
                        obj->unk68 = 0.0f;
                        obj->unk70 = 1.6333334f;
                    }
                    if (obj->unk6C > 0.0f && obj->unk44 == NULL && sp74->unk40->unk14 < obj->unk40->unk14) {
                        if (func_80244BA4(arg0, 2)) {
                            spCC = EN->unkC + 180.0f;
                            if (spCC >= 360.0f) {
                                spCC -= 360.0f;
                            }
                        } else {
                            spCC = EN->unkC;
                        }
                        spB0 = func_80243F50(spCC);
                        EN->unk40 = (PORT_IP(s32)) func_802700EC(obj, sp74, spB0, 8.0f);
                        func_80244354(arg0, 0x2100, 0);
                        if (func_80244BA4(arg0, 0x80) || func_80244BA4(arg0, 0x40)) {
                            func_80244C30(arg0, 1);
                        }
                        func_80244354(arg0, 0x400020, 1);
                        if (!func_80244BA4(arg0, 0x8000)) {
                            func_802444B8(arg0, 0x13);
                        } else {
                            func_802444B8(arg0, 0x14);
                        }
                        if (func_80243EB4(arg0, 0x2000)) {
                            func_80247F90(arg0);
                        }
                    }
                    if ((func_80244BA4(arg0, 0x8000) && sp114 != 0) || (func_80243EB4(arg0, 0x2000) && spF0 != 0)) {
                        if (func_80244BA4(arg0, 0x8000000)) {
                            func_80244354(arg0, 0x8000000, 0);
                            obj->unk40->unk20 = EN->unk64;
                        }
                    }
                }
                if (EN->unk2C != NULL && (EN->unk2C->unk8 & 0x10)) {
                    func_80231434(EN->unk2C);
                    sp74 = func_8022F4AC(EN->unk2C, 0x10, obj);
                    if (func_80244BA4(arg0, 0x400)) {
                        sp74 = NULL;
                    }
                    if (sp74 != NULL) {
                        func_80232BA0(&EN->unk2C->unk58, EN->unk2C);
                        func_80232BA0(&obj->unk58, obj);
                        if (func_80244BA4(arg0, 0x8000000)) {
                            func_80244354(arg0, 0x8000000, 0);
                            obj->unk40->unk20 = EN->unk64;
                        }
                        if (obj->unk6C > 0.0f) {
                            func_80244C30(arg0, 0);
                        }
                    }
                }
                if (func_80243EB4(arg0, 0x2000)) {
                    sp6C = func_802443F4(EN->unkA8);
                    func_80231434(sp6C);
                    sp74 = func_8022F4AC(sp6C, 0x10, obj);
                    if (func_80244BA4(arg0, 0x400)) {
                        sp74 = NULL;
                    }
                    if (sp74 != NULL) {
                        if (obj->unk40->unk14 < obj->unk5C && obj->unk68 < obj->unk6C) {
                            func_80247F90(arg0);
                        } else {
                            func_80232BA0(&sp6C->unk58, sp6C);
                            func_80232BA0(&obj->unk58, obj);
                            if (func_80244BA4(arg0, 0x8000000)) {
                                func_80244354(arg0, 0x8000000, 0);
                                obj->unk40->unk20 = EN->unk64;
                                if (obj->unk6C > 0.0f) {
                                    func_80247F90(arg0);
                                }
                            }
                        }
                    }
                }
#if !VERSION_JP
                if (obj->unk40->unk14 < 0.0f) {
                    func_80270770((s16)arg0, 1);
                }
#endif
            }
            if (!func_80244BA4(arg0, 0x400) && !func_80244BA4(arg0, 8) && !func_80244BA4(arg0, 0x2000)) {
                func_8024857C(arg0);
            }
            EN->unk64 = EN->unk60;
        }
    }
}
#undef EN

void func_8024D8C4(Ctl58 *arg0) {
    f32 dist;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 angle;
    UnkStructA *obj;
    f32 pos[3];
    f32 target[3];
    f32 res[3];
    f32 rot[3];
    Unk8026DCAC ipos;
    s32 b1;
    s32 b0;
    s32 id;
    s32 dir;

    id = func_80243DDC();
    b0 = D_802AC968[id] & 1;
    b1 = (D_802AC968[id] >> 1) & 1;
    if (b0) {
        arg0->unk8 = D_802AC95C;
    } else {
        arg0->unk8 = 3;
    }
    arg0->unkC = 0.0f;
    obj = func_802443F4(id);
    func_8022F808(obj, pos);
    target[0] = D_802ACA28;
    if (D_802AC8BC == 0x100 || D_802AC8BC == 0x200) {
        target[1] = D_802ACA2C;
    } else {
        target[1] = pos[1];
    }
    target[2] = D_802ACA8C;
    angle = obj->unk40->unk20;
    func_8022FF58(pos, target, rot);
    rot[1] -= 90.0f;
    dist = func_8022DF68(pos, target);
    if (b0) {
        arg0->unk4 = D_802AC8BC;
    } else {
        dir = func_80243F50(rot[0]);
        arg0->unk4 = dir;
    }
    if (D_802ACA24 < dist || func_80244BA4(id, 0x4000)) {
        arg0->unk0 = D_802ACA24;
    } else {
        arg0->unk0 = dist;
    }
    if (func_80244BA4(id, 0x2000)) {
        arg0->unk0 = 0.0f;
    }
    if (arg0->unk8 != 3) {
        func_8024D5D0(arg0);
    }
    if ((D_802AC8BC != 0x100 && D_802AC8BC != 0x200) || b0 != 1) {
        if (dist >= 1.0f) {
            arg0->unk4 = func_80243F50(rot[0]);
        }
    }
    if (dist <= 1.0f) {
        if (!b0) {
            dx = 0;
            dy = 0;
            dz = 0;
            switch (D_802AC8BC) {
            case 0x0:
                dz = -D_802AC8C4;
                angle = 180.0f;
                break;
            case 0x80:
                angle = 0.0f;
                dz = D_802AC8C4;
                break;
            case 0xC0:
                angle = 270.0f;
                dx = -D_802AC8C4;
                break;
            case 0x40:
                angle = 90.0f;
                dx = D_802AC8C4;
                break;
            case 0x20:
                angle = 135.0f;
                dx = D_802AC8C4;
                dz = -D_802AC8C4;
                break;
            case 0xE0:
                angle = 225.0f;
                dx = -D_802AC8C4;
                dz = -D_802AC8C4;
                break;
            case 0x60:
                angle = 45.0f;
                dx = D_802AC8C4;
                dz = D_802AC8C4;
                break;
            case 0xA0:
                angle = 315.0f;
                dx = -D_802AC8C4;
                dz = D_802AC8C4;
                break;
            case 0x100:
                dy = D_802AC8C4;
                break;
            case 0x200:
                dy = -D_802AC8C4;
                break;
            }
            func_8026DCAC(obj->unk40->unk10 + dx * 100.0f, obj->unk40->unk14 + dy * 100.0f,
                          obj->unk40->unk18 + dz * 100.0f, &ipos, 2);
            func_8026E338(ipos.i[0], ipos.i[1], ipos.i[2], res, 0);
            D_802ACA28 = res[0];
            D_802ACA2C = res[1];
            D_802ACA8C = res[2];
            func_8024A6B4(id, angle);
        } else {
            switch (D_802AC8BC) {
            case 0x0:
                angle = 180.0f;
                break;
            case 0x80:
                angle = 0.0f;
                break;
            case 0xC0:
                angle = 270.0f;
                break;
            case 0x40:
                angle = 90.0f;
                break;
            case 0x20:
                angle = 135.0f;
                break;
            case 0xE0:
                angle = 225.0f;
                break;
            case 0x60:
                angle = 45.0f;
                break;
            case 0xA0:
                angle = 315.0f;
                break;
            }
            obj->unk40->unk10 = D_802ACA28;
            if (D_802AC8BC == 0x100 || D_802AC8BC == 0x200) {
                obj->unk40->unk14 = D_802ACA2C;
            }
            obj->unk40->unk18 = D_802ACA8C;
            func_80244354(id, 0x420, 0);
            if (!func_80244BA4(id, 0x2000000)) {
                func_80244354(id, 0x10000, 0);
            }
            D_800AEDF0[id].unk48 = 1;
            obj->unk70 = 1.6333334f;
            func_8024D108(id, func_802443CC(id));
        }
        arg0->unk0 = 0.0f;
        obj->unk40->unk10 = target[0];
        obj->unk40->unk18 = target[2];
        if (D_802AC8BC != 0x100 && D_802AC8BC != 0x200) {
            obj->unk40->unk20 = angle;
        }
        b0++;
    }
    D_802AC968[id] = b1 * 2 + b0;
    if (!(D_802ACA10[id] & 3)) {
        func_8022F808(obj, pos);
        D_802ACF60 = func_8023A840();
        if (!(D_802ACF60->unk0 & 2)) {
            func_802775D8((PORT_RET(s32, void *)) pos);
        }
        func_80282934();
    }
    D_802ACA10[id]++;
}

void func_8024D5D0(Ctl58 *arg0) {
    Vec3f angle; /* yaw, pitch, dist: func_8022FF58 writes all three from &angle.x */
    f32 org[3];
    f32 vec[3];
    f32 x;
    f32 y;
    f32 z;
    s32 dir;
    f32 cs;
    s32 tilt;
    s32 yawDir;
    f32 len;
    f32 s;

    len = arg0->unk0;
    dir = arg0->unk4;
    tilt = arg0->unk8;
    angle.z = len;
    switch (dir) {
    case 0x0:
        angle.x = 180.0f;
        break;
    case 0x20:
        angle.x = 135.0f;
        break;
    case 0x40:
        angle.x = 90.0f;
        break;
    case 0x60:
        angle.x = 45.0f;
        break;
    case 0x80:
        angle.x = 0.0f;
        break;
    case 0xA0:
        angle.x = 315.0f;
        break;
    case 0xC0:
        angle.x = 270.0f;
        break;
    case 0xE0:
        angle.x = 225.0f;
        break;
    }
    switch (tilt) {
    case 1:
        angle.y = 0.0f;
        angle.x = 0.0f;
        break;
    case 2:
        angle.y = 45.0f;
        break;
    case 3:
        angle.y = 90.0f;
        break;
    case 4:
        angle.y = 135.0f;
        break;
    case 5:
        angle.x = 0.0f;
        angle.y = 180.0f;
        break;
    }
    s = func_80234434(angle.x);
    x = func_80234434(angle.y) * (angle.z * s);
    cs = func_80234408(angle.y);
    y = cs * angle.z;
    s = func_80234408(angle.x);
    z = func_80234434(angle.y) * (angle.z * s);
    if (x < 0.01 && x > -0.01) {
        x = 0.0f;
    }
    if (y < 0.01 && y > -0.01) {
        y = 0.0f;
    }
    if (z < 0.01 && z > -0.01) {
        z = 0.0f;
    }
    org[0] = 0.0f;
    org[1] = 0.0f;
    org[2] = 0.0f;
    vec[1] = 0.0f;
    vec[2] = z;
    vec[0] = x;
    func_8022FF58(org, vec, &angle.x);
    yawDir = func_80243F50(angle.x);
    arg0->unk4 = yawDir;
    arg0->unk0 = angle.z;
    arg0->unkC = y;
}

void func_8024D540(s32 arg0, f32 *arg1) {
    UnkStructA *obj;

    arg1[0] = D_802ACA44;
    ((s32 *)arg1)[1] = D_800AEDF0[arg0].unkA0;
    obj = func_802443F4(arg0);
    if (obj->unk68 == 0.0f && obj->unk6C == 0.0f) {
        func_8024878C(arg0);
    }
}

s32 func_8024D50C(void *arg0) {
    s32 ret = 0;

    if (arg0 != NULL) {
        s32 v = D_802AC5D0 & 4;
        if ((M2C_FIELD(arg0, s32 *, 8) & 4) && v) {
            ret = 1;
        }
    }
    return ret;
}

s32 func_8024D41C(f32 key) {
    Tbl8i_5E3F0 vals;
    Tbl8f_5E3F0 keys;
    s32 i;
    s32 r;

    vals = *(Tbl8i_5E3F0 *)D_802A0DC8;
    keys = *(Tbl8f_5E3F0 *)D_802A0DE8;
    r = -1;
    i = 0;
    while (i < 8 && r == -1) {
        if (key == keys.v[i]) {
            r = vals.v[i];
        }
        i++;
    }
    return r;
}

void func_8024D374(s32 arg0) {
    func_80244354(arg0, 0x100, 0);
    if (!func_80244BA4(arg0, 0x1000)) {
        if (func_802452AC(arg0, 0x8080)) {
            func_802444B8(arg0, 0xA);
        } else {
            func_802444B8(arg0, 0x12);
        }
        func_80244354(arg0, 8, 1);
    }
}

void func_8024D20C(s32 arg0) {
    UnkStructA *obj;

    if (func_80244BA4(arg0, 0x40)) {
        func_80244BE0(arg0, 1);
    }
    func_80244354(arg0, 0x148, 0);
    func_80244354(arg0, 0x2020, 1);
    if (!func_80244BA4(arg0, 0x8000) && !func_80244BA4(arg0, 0x80)) {
        func_802444B8(arg0, 9);
    } else {
        func_802444B8(arg0, 0x11);
    }
    obj = func_802443F4(arg0);
    func_8028A81C(0xC, obj->unk40->unk10, obj->unk40->unk14 + 20.0f, obj->unk40->unk18, 0);
    if (M2C_FIELD(obj, s32 *, 8) & 0x01000000) {
        func_8026C660(0x1C);
    }
    D_800AEDF0[arg0].unk50 = 0;
}

void func_8024D108(s32 arg0, s32 arg1) {
    UnkStruct_800AEDF0 *e = &D_800AEDF0[arg0];
    f32 len;
    UnkStructA *o = func_802443F4(arg0);
    s32 anim;
    f32 t;


    D_802AC9F8[arg0] = arg1;
    t = 0.0f;
    switch (func_802443CC(arg0)) {
        case 1:
        case 2:
        case 3:
            anim = 0;
            break;
        case 13:
        case 14:
        case 15:
            anim = 1;
            break;
        default:
            anim = -1;
            break;
    }
    if (anim != -1) {
        len = func_80229D3C(o->unk40, anim);
        t = PORT_FIELD(PORT_FIELD(PORT_FIELD(o->unk40, void **, 0x48, Port_UnkStructA_40, unk48), PORT_IP(s32) *, 4, Port_UnkStructA_40_48, unk4) + anim * PORT_HOST(0xC, sizeof(Port_UnkStructA_40_48_4) / 2), f32 *, 4, Port_UnkStructA_40_48_4, unk4[0]) + 1.0f;
        if (len <= t) {
            t -= len;
        }
    }
    D_802ACA78[arg0] = t;
}

void func_8024CEB8(s32 arg0) {
    UnkStructA *obj;
    s32 code;
    s32 hit;
    s32 v;
    f32 pos[3];

    hit = 0;
    obj = func_802443F4(arg0);
    v = func_802443CC(arg0);
    switch (v) {
    case 1:
    case 13:
        v = obj->unk40->unk48->unk4[1].unk4;
        if (v == 12.0f || v == 36.0f) {
            hit = 1;
        }
        break;
    case 2:
    case 14:
        v = obj->unk40->unk48->unk4[1].unk4;
        if (v == 0.0f || v == 20.0f) {
            hit = 1;
        }
        break;
    case 3:
    case 15:
        v = obj->unk40->unk48->unk4[1].unk4;
        if (v == 0.0f || v == 16.0f) {
            hit = 1;
        }
        break;
    }
    if (hit) {
        D_802ACF60 = func_8023A840();
        if (D_802ACF60->unk4 == 3 && D_802ACF60->unk8 == 0) {
            func_8026C660(0x30);
        }
        if (D_802ACF60->unk4 == 0) {
            func_8022F808(obj, pos);
            code = func_8026DF74(pos[0], pos[1], pos[2]);
            if ((code & 0x1C00) == 0x400) {
                func_8026C660(0x31);
            }
        }
    }
}

void func_8024CC58(s32 arg0) {
    UnkStructA *obj;
    PORT_IP(s32) r;
    s32 i;
    f32 vec[3];

    obj = func_802443F4(arg0);
    if (func_80243EB4(arg0, 0x2000)) {
        UnkStructA *o2;
        i = D_800AEDF0[arg0].unkA4;
        o2 = func_802443F4(i);
        vec[2] = 70.0f;
        vec[1] = D_802ACE90[i];
        vec[0] = 180.0f;
        func_8022FEAC(vec, &o2->unk4C);
        D_802ACE90[i] -= 1.0f;
    }
    if (obj->unk40->unk48->unk4[0].unk4 == 2.0f && !(D_800AEDF0[arg0].unk38 & 0x80)) {
        if (D_800AEDF0[arg0].unk3C == 0) {
            r = func_80271540((s16)arg0);
            if (r != 0) {
                func_80271BB8((s16)arg0);
                D_800AEDF0[arg0].unk2C = r;
            } else {
                func_80244354(arg0, 0xA0, 0);
                func_802444B8(arg0, 0);
                D_800AEDF0[arg0].unk2C = 0;
            }
        } else {
            func_802741D8((s16)arg0);
            D_800AEDF0[arg0].unk2C = func_80270714((s16)arg0);
            if (D_800AEDF0[arg0].unk2C == 0) {
                func_80244354(arg0, 0xA0, 0);
                func_802444B8(arg0, 0);
            }
        }
        D_800AEDF0[arg0].unk38 |= 0x80;
    }
    if ((obj->unk40->unk48->unk4[0].unk8 & 0x40) && func_80277CEC(arg0)) {
        func_80244354(arg0, 0xA0, 0);
        func_80244354(arg0, 0x8000, 1);
        func_802444B8(arg0, 0xC);
    }
    if (D_800AEDF0[arg0].unk2C != 0 && (PORT_IP(s32))obj != PORT_FIELD(D_800AEDF0[arg0].unk2C, PORT_IP(s32) *, 0x44, Port_UnkStructA, unk44)) {
        func_80244C30(arg0, 0);
    }
}

void func_8024C884(s32 arg0, f32 arg1, s32 arg2) {
    Tbl4_5E3F0 tbl;
    s32 level;
    s32 t1;
    UnkStructA *obj;
    s32 t2;
    f32 vec[3];

    tbl = *(Tbl4_5E3F0 *)D_802A0D94;
    if (arg2 & 0x4000) {
        D_800AEDF0[arg0].unkAC++;
    }
    obj = func_802443F4(arg0);
    if (obj->unk40->unk48->unk4[0].unk4 == 12.0f && !(D_800AEDF0[arg0].unk38 & 0x40)) {
        t1 = func_80244BA4(arg0, 0x80000);
        if (t1) {
            switch (D_800AEDF0[arg0].unk80) {
            case 1:
                arg1 = 0.0f;
                break;
            case 2:
                arg1 = 10.0f;
                break;
            case 3:
                arg1 = 60.0f;
                break;
            }
        }
        if (func_8023A840()->unk48 == 1 && !func_80244BA4(arg0, 0x80000)) {
            if (D_800AEDF0[arg0].unkAC < 6) {
                level = 1;
            }
            if (D_800AEDF0[arg0].unkAC >= 6 && D_800AEDF0[arg0].unkAC < 13) {
                level = 2;
            }
            if (D_800AEDF0[arg0].unkAC >= 13) {
                level = 3;
            }
        } else {
            if (arg1 < 10.0f) {
                level = 1;
            }
            if (arg1 >= 10.0f && arg1 < 60.0f) {
                level = 2;
            }
            if (arg1 >= 60.0f) {
                level = 3;
            }
        }
        func_8026C660(0x20);
        func_80244BE0(arg0, (s16)level);
        if ((D_802AC5D0 & 4) || arg0 == 0) {
            func_802479F0(tbl.v);
        } else {
            func_8026C660(0x20);
        }
        D_800AEDF0[arg0].unk38 |= 0x40;
    }
    if ((obj->unk40->unk48->unk4[0].unk8 & 0x40) ||
        (obj->unk40->unk48->unk4[0].unk4 < 12.0f && D_800AEDF0[arg0].unk2C == 0)) {
        func_80244354(arg0, 0x60, 0);
        func_802444B8(arg0, 0);
        t2 = func_80243EB4(arg0, 0x2000);
        if (t2) {
            UnkStructA *o2 = func_802443F4(D_800AEDF0[arg0].unkA4);
            vec[2] = 70.0f;
            vec[1] = 60.0f;
            vec[0] = 180.0f;
            func_8022FEAC(vec, &o2->unk4C);
        }
    }
}

void func_8024C804(s32 arg0) {
    if (func_8024C4EC(func_802443F4(arg0), 2) != D_800AEDF0[arg0].unk4C) {
        func_80244354(arg0, 0x200, 0);
        D_800AEDF0[arg0].unk4C = 0;
    }
}

void func_8024C53C(s32 arg0, void *arg1) {
    Tbl5_5E3F0 tbl;
    UnkStructA *obj;
    PORT_IP(s32) r;
    f32 pos[3];

    tbl = *(Tbl5_5E3F0 *)D_802A0DB4;
    obj = func_802443F4(arg0);
    if (obj->unk40->unk48->unk4[0].unk4 == 11.0f && !(D_800AEDF0[arg0].unk38 & 0x100)) {
        func_8022F808(obj, pos);
        if (D_800AEDF0[arg0].unkBC == 1) {
            obj->unk40->unk10 += ((f32 *)arg1)[0];
            obj->unk40->unk14 += ((f32 *)arg1)[1];
            obj->unk40->unk18 += ((f32 *)arg1)[2];
        }
        r = func_8024C4EC((UnkStructA *)obj, 2);
        if (r != 0) {
            if ((D_802AC5D0 & 4) || arg0 == 0) {
                func_802479F0(tbl.v);
            } else {
                func_8026C660(0x1F);
            }
            func_80275C7C((s16)arg0, PORT_PTR(r), (s16)func_802704F8(obj->unk40->unk20));
        }
        func_80232BA0(pos, obj);
        D_800AEDF0[arg0].unk38 |= 0x100;
    }
    if (obj->unk40->unk48->unk4[0].unk8 & 0x40) {
        func_80244354(arg0, 0x100, 0);
        if (!func_80244BA4(arg0, 0x8000)) {
            func_802444B8(arg0, 0);
        } else {
            func_802444B8(arg0, 0xC);
        }
        r = func_8024C4EC((UnkStructA *)obj, 2);
        if (r != 0) {
            func_80247928(arg0, r);
        } else {
            func_80244354(arg0, 0x200, 0);
            D_800AEDF0[arg0].unk4C = 0;
        }
        func_80244354(arg0, 0x20, 0);
    }
}

PORT_IP(s32) func_8024C4EC(UnkStructA *obj, s32 arg1) {
    PORT_IP(s32) r;

    M2C_FIELD(obj, s32 *, 0xC) &= ~0x1E00;
    r = (PORT_IP(s32)) func_8022F550(obj, 2);
    M2C_FIELD(obj, s32 *, 0xC) |= 0x1E00;
    return r;
}

void func_8024C334(s32 arg0, PORT_IP(s32) arg1, u16 arg2, f32 *arg3) {
    s32 t1;
    s32 t2;
    s32 t3;
    UnkStruct_800AEDF0 *e = &D_800AEDF0[arg0];
    UnkStructA *obj = e->unk28;

    t1 = func_80244BA4(arg0, 2);
    if (!t1) {
        func_80243D54(arg0); /* fake: unused call: its R2 store frees v0 for the flag (header); p10: getters for unk28 in place of obj/e->unk28, at each fake's place, no */
        if (!obj->unk40 && !obj->unk40) {} /* fake: keeps unk28 loaded (t0); p10: no DEBUG_PRINT check of obj/obj->unk40 (single or `||`) anywhere in the branch */
        if (!(e->unk38 & 0x1000)) {
            func_8024C314(arg0);
            e->unk38 |= 0x1000;
        }
    }
    t2 = func_802452AC(arg0, 0x600002);
    if (t2) {
        if (e->unk28 == NULL || e->unk28->unk40 == NULL) {
            DEBUG_PRINT(("e->unk28 or its unk40 == NULL\n"));
        }
        D_802AC8D0[arg0]--;
    }
    t3 = func_80244BA4(arg0, 0x400000);
    if (!t3 && arg3[2] > 60.0) {
        if (arg3[0] != D_802ACA30[arg0]) {
            D_802AC900[arg0]--;
        }
    }
    if (D_802AC8D0[arg0] == 0 || D_802AC900[arg0] == 0) {
        func_80247A54(arg0, arg1, 30, 1);
    }
    D_802ACA30[arg0] = arg3[0];
}

void func_8024C314(s32 arg0) {
    func_802444B8(arg0, 8);
}

void func_8024C29C(s32 arg0) {
    UnkStructA *o = func_802443F4(arg0);

    if (D_802AC918[arg0] > 0) {
        if (--D_802AC918[arg0] == 0) {
            func_80244354(arg0, 0x10000, 0);
            func_802313DC(o, 0x40000);
        }
    }
}

void func_8024C1E8(s32 arg0) {
    if ((func_802443F4(arg0))->unk40->unk48->unk4[0].unk8 & 0x40) {
        func_80244354(arg0, 0x2020, 0);
        if (!func_80244BA4(arg0, 0x8000)) {
            func_802444B8(arg0, 0);
        } else {
            func_802444B8(arg0, 0xC);
        }
    }
}

void func_8024C0D8(s32 arg0) {
    UnkStructA *o = func_802443F4(arg0);
    f32 *p = &D_802ACA60[arg0];
    f32 t;

    *p += 16.0f;
    if (*p >= 360.0f) {
        o->unk40->unk20 = *p - 360.0f;
    } else {
        o->unk40->unk20 = *p;
    }
    t = D_802ACA48[arg0];
    if (*p - t > 360.0f) {
        *p = t;
        if (--D_802AC930[arg0] == 0) {
            o->unk40->unk20 = t;
            func_80244354(arg0, 0x20020, 0);
            func_802313DC(o, 0x40000);
            func_8024796C(arg0, 60);
        }
    }
}

void func_8024C04C(s32 arg0) {
    UnkStructA *o = func_802443F4(arg0); /* unused (its inlined load is the original's) */

    if (func_80270714((s16)arg0) == 0) {
        func_80244354(arg0, 0x8000, 0);
        func_802444B8(arg0, 0);
        D_800AEDF0[arg0].unk2C = 0;
        func_80244354(arg0, 0x120, 0);
    }
}

void func_8024BFE0(s32 arg0) {
    if (PORT_FIELD(PORT_FIELD(PORT_FIELD(PORT_FIELD((func_802443F4(arg0)), void **, 0x40, Port_UnkStructA, unk40), void **, 0x48, Port_UnkStructA_40, unk48), void **, 4, Port_UnkStructA_40_48, unk4), s32 *, 8, Port_UnkStructA_40_48_4, unk4[4]) & 0x40) {
        func_80244354(arg0, 0x04000020, 0);
    }
}

void func_8024BF04(s32 arg0) {
    UnkStruct_800AEDF0 *p = &D_800AEDF0[arg0];
    UnkStructA *obj = func_802443F4(arg0);

    if (p->unk2C != 0) {
        if (func_80274F8C(M2C_FIELD(p, s16 *, 0x1A), obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, arg0) == 0) {
            if (func_802712A8((s32 *)PORT_AT(func_802703B4(PORT_PTR(p->unk2C)), 0x64, Port_Ent88, unk3C[40]), 0x80) != 0) {
                func_80243D90(arg0, 0x400, 0);
                func_80244354(arg0, 0x20, 0);
            }
        }
    } else {
        func_80243D90(arg0, 0x400, 0);
        func_80244354(arg0, 0x20, 0);
    }
}

void func_8024BD1C(s32 arg0) {
    UnkStructA *o = func_802443F4(arg0);

    if (D_800AEDF0[arg0].unk64 > 360.0f) {
        D_800AEDF0[arg0].unk64 -= 360.0f;
    }
    if (D_800AEDF0[arg0].unk64 < 0.0f) {
        D_800AEDF0[arg0].unk64 += 360.0f;
    }
    D_800AEDF0[arg0].unk60 += D_800AEDF0[arg0].unk5C;
    o->unk40->unk20 = D_800AEDF0[arg0].unk60;
    if (D_800AEDF0[arg0].unk60 > 360.0f) {
        o->unk40->unk20 -= 360.0f;
    }
    if (D_800AEDF0[arg0].unk60 < 0.0f) {
        o->unk40->unk20 += 360.0f;
    }
    if ((D_800AEDF0[arg0].unk5C > 0.0f && D_800AEDF0[arg0].unk58 <= D_800AEDF0[arg0].unk60) || (D_800AEDF0[arg0].unk5C < 0.0f && D_800AEDF0[arg0].unk60 <= D_800AEDF0[arg0].unk58)) {
        o->unk40->unk20 = D_800AEDF0[arg0].unk58;
        if (o->unk40->unk20 >= 360.0f) {
            o->unk40->unk20 -= 360.0f;
        }
        if (o->unk40->unk20 < 0.0f) {
            o->unk40->unk20 += 360.0f;
        }
        func_80244354(arg0, 0x8000000, 0);
    }
    if ((void *)D_800AEDF0[arg0].unk2C != 0) {
        func_80231434((void *)D_800AEDF0[arg0].unk2C);
    }
}

void func_8024BC94(s32 arg0) {
    if (PORT_FIELD(PORT_FIELD(PORT_FIELD(PORT_FIELD((func_802443F4(arg0)), void **, 0x40, Port_UnkStructA, unk40), void **, 0x48, Port_UnkStructA_40, unk48), void **, 4, Port_UnkStructA_40_48, unk4), s32 *, 8, Port_UnkStructA_40_48_4, unk4[4]) & 0x40) {
        func_80244354(arg0, 0x20, 0);
        func_80243D90(arg0, 0x10000, 0);
        func_802444B8(arg0, 0);
    }
}

void func_8024BC20(s32 arg0) {
    if (--D_800AEDF0[arg0].unkB8 == 0) {
        func_8022FC90((void *)D_800AEDF0[arg0].unkB4);
        D_800AEDF0[arg0].unkB4 = 0;
        func_80243D90(arg0, 0x20000, 0);
    }
}

void func_8024BBA4(s32 arg0) {
    if (PORT_FIELD(PORT_FIELD(PORT_FIELD(PORT_FIELD((func_802443F4(arg0)), void **, 0x40, Port_UnkStructA, unk40), void **, 0x48, Port_UnkStructA_40, unk48), void **, 4, Port_UnkStructA_40_48, unk4), s32 *, 8, Port_UnkStructA_40_48_4, unk4[4]) & 0x40) {
        func_80244354(arg0, 0x20, 0);
        func_80243D90(arg0, 0x200000, 0);
    }
}

void func_8024B444(s32 arg0, f32 *arg1) {
    s32 t1;
    s32 blocked;
    UnkStructA *o2;
    f32 old;
    s32 t3;
    f32 dist;
    f32 *out;
    f32 posA[3];
    f32 posB[3];
    f32 vec[3];
    s32 other;
    UnkStruct_800AEDF0 *e;
    UnkStructA *o;
    PORT_IP(s32) found;
    s32 i;
    s32 j;

    M2C_FIELD(arg1, s32 *, 4) = 0;
    arg1[0] = 0.0f;
    e = &D_800AEDF0[arg0];
    o = e->unk28;
    t1 = func_80243EB4(arg0, 0x2000);
    if (t1) {
        other = e->unkA8;
        if (o->unk40->unk28 > 1.0f) {
            if (D_800AEDF0[other].unkC4 == 8.0f) {
                D_800AEDF0[other].unkC4 = 6.0f;
            }
            do {} while (0); /* fake: splits the block (only here; any DEBUG_PRINT check, even `if (0)`, does it; `else if` pairs, an entry pointer for `other`, the C0 update's forms: no) */
            if (D_800AEDF0[other].unkC4 == -8.0f) {
                D_800AEDF0[other].unkC4 = -6.0f;
            }
        }
        if (o->unk40->unk28 < 1.0f) {
            if (D_800AEDF0[other].unkC4 == 6.0f) {
                D_800AEDF0[other].unkC4 = 8.0f;
            }
            if (D_800AEDF0[other].unkC4 == -6.0f) {
                D_800AEDF0[other].unkC4 = -8.0f;
            }
        }
        old = D_800AEDF0[other].unkC0;
        D_800AEDF0[other].unkC0 += D_800AEDF0[other].unkC4;
        o2 = func_802443F4(other);
        vec[2] = D_800AEDF0[other].unkC0;
        vec[1] = 60.0f;
        vec[0] = 180.0f;
        out = &o2->unk4C;
        func_8022FEAC(vec, out);
        func_80231434(o2);
    }
    o->unk40->unk28 += e->unk6C;
    o->unk40->unk2C += e->unk6C;
    o->unk40->unk30 += e->unk6C;
    found = (PORT_IP(s32)) func_8022F4AC(o, 0x10, PORT_PTR(e->unk4C));
    if (found != 0) {
        func_8022F808(o, posA);
        func_8022F808((void *)found, posB);
        vec[1] = 0.0f;
        vec[0] = 0.0f;
        vec[2] = 5.0f;
        for (j = 0; j < 50; j++) {
            for (i = 0; i != 8; i++) {
                func_80232BA0(posA, o);
                func_80291944(o, i << 5, vec[2]);
                blocked = func_802909FC(o, func_80245150);
                if (!blocked && !func_8022F4AC(o, 0x10, PORT_PTR(e->unk4C))) {
                    PORT_FIELD(o, f32 *, 0x58, Port_UnkStructA, unk48[16]) = o->unk40->unk10;
                    PORT_FIELD(o, f32 *, 0x5C, Port_UnkStructA, unk48[20]) = o->unk40->unk14;
                    PORT_FIELD(o, f32 *, 0x60, Port_UnkStructA, unk48[24]) = o->unk40->unk18;
                    goto found;
                }
            }
            vec[2] += 2.0f;
        }
        func_80232BA0(posA, o);
        do {
            o->unk40->unk14 += 10.0f;
        } while (func_802909FC(o, func_80245150) || func_8022F4AC(o, 0x10, PORT_PTR(e->unk4C)));
        func_80247D74(arg0, 22.5f, 8.0f, 1, NULL);
    found:
        func_8022F808(o, posB);
        dist = func_8022DF68(posA, posB);
        arg1[0] = dist;
        t3 = func_80243EB4(arg0, 0x2000);
        if (t3) {
            D_800AEDF0[other].unkC0 = old;
            vec[2] = old;
            vec[1] = 60.0f;
            vec[0] = 180.0f;
            func_8022FEAC(vec, &o2->unk4C);
            func_80231434(o2);
        }
    }
    PORT_FIELD(e->unkB0, f32 *, 0x4C, Port_UnkStructA, unk48[4]) = o->unk40->unk28;
    PORT_FIELD(e->unkB0, f32 *, 0x54, Port_UnkStructA, unk48[12]) = o->unk40->unk30;
    if ((e->unk6C > 0.0f && e->unk70 <= o->unk40->unk28) || (e->unk6C < 0.0f && o->unk40->unk28 <= e->unk70)) {
        o->unk40->unk28 = e->unk70;
        o->unk40->unk2C = e->unk70;
        o->unk40->unk30 = e->unk70;
        PORT_FIELD(e->unkB0, f32 *, 0x4C, Port_UnkStructA, unk48[4]) = o->unk40->unk28;
        PORT_FIELD(e->unkB0, f32 *, 0x54, Port_UnkStructA, unk48[12]) = o->unk40->unk30;
        if (e->unk6C > 0.0f) {
            func_80243D90(arg0, 2, 0);
            if (o->unk40->unk28 == 1.0f) {
                func_80243D90(arg0, 1, 0);
                if (!(e->unk68 & 8 | 0x20 | 0x80) && !(e->unk18 & 0x404)) {
                    func_80243D90(arg0, 0x10, 0);
                }
            } else if (func_80243EB4(arg0, 0x2000)) {
                o2 = func_802443F4(e->unkA8); vec[2] = 100.0f;
                vec[1] = 60.0f;
                vec[0] = 180.0f;
                func_8022FEAC(vec, &o2->unk4C);
            }
        }
        if (e->unk6C < 0.0f) {
            func_80243D90(arg0, 2, 0);
            if (o->unk40->unk28 == 1.0f) {
                func_80243D90(arg0, 4, 0);
                if (!(e->unk68 & 8 | 0x20 | 0x80) && !(e->unk18 & 0x404)) {
                    func_80243D90(arg0, 0x10, 0);
                }
            } else if (func_80243EB4(arg0, 0x2000)) {
                o2 = func_802443F4(e->unkA8); vec[2] = 30.0f;
                vec[1] = 60.0f;
                vec[0] = 180.0f;
                func_8022FEAC(vec, &o2->unk4C);
            }
        }
        func_80243D90(arg0, 2, 0);
    }
    e->unk74++;
}

void func_8024B1CC(s32 arg0) {
    UnkStructA *obj;
    UnkStruct_800AEDF0 *p;
    s32 t;

    p = &D_800AEDF0[arg0];
    obj = func_802443F4(arg0);
    t = --p->unk78;
    if (t == 0) {
        if (func_80243EB4(arg0, 4)) {
            func_80270770((s16)arg0, 1);
            p->unk2C = 0;
            func_8024694C(arg0, 1, 0);
            func_80243D90(arg0, 4, 0);
        }
        if (func_80243EB4(arg0, 1)) {
            func_80270770((s16)arg0, 1);
            p->unk2C = 0;
            func_8024694C(arg0, 2, 0);
            func_80243D90(arg0, 1, 0);
        }
        if (func_80243EB4(arg0, 0x80)) {
            func_802313DC(obj, 0x40);
            func_80243D90(arg0, 0x80, 0);
            if (!func_80244BA4(arg0, 0x10000)) {
                func_80248148(arg0, 1);
            }
        }
        if (func_80243EB4(arg0, 0x100)) {
            func_80246C3C(arg0, 0, 0);
        }
        if (func_80243EB4(arg0, 8)) {
            func_80243D90(arg0, 8, 0);
        }
        if (func_80243EB4(arg0, 0x20)) {
            func_80243D90(arg0, 0x20, 0);
        }
        func_80243D90(arg0, 0x10, 0);
        p->unk18 &= ~0x404;
        func_8026C660(0x1ED);
        return;
    }
    if (func_80243EB4(arg0, 0x80) && !(t & 3)) {
        func_8028A81C(7, obj->unk40->unk10, obj->unk40->unk14 + 100.0f, obj->unk40->unk18, 0);
    }
}

void func_8024B028(s32 arg0) {
    s32 r;
    s32 g;
    s32 b;
    UnkStructA_40 *model;
    f32 level;
    UnkStruct_800AEDF0 *e = &D_800AEDF0[arg0];
    UnkStructA *o = e->unk28;

    e->unk8C += e->unk88;
    switch (e->unk84) {
        case 1:
            r = 0x1F;
            g = 0;
            b = 0;
            break;
        case 2:
            b = g = r = 4;
            break;
        case 3:
            g = r = 0x1F;
            b = 0;
            break;
    }
    if (e->unk8C > 1.0f && e->unk88 > 0.0f) {
        e->unk8C = 1.0f;
        e->unk88 = -e->unk88;
    }
    if (e->unk8C < 0.0f && e->unk88 < 0.0f) {
        e->unk8C = 0.0f;
        e->unk88 = -e->unk88;
    }
    model = o->unk40;
    level = e->unk8C;
    func_8029279C(model, r, g, b, level);
    if (--e->unk90 == 0 || !func_80243EB4(arg0, 0x10)) {
        func_80246C3C(arg0, 0, 0);
    }
}

void func_8024AFD4(s32 arg0) {
    if (--D_800AEDF0[arg0].unk7C == 0) {
        func_80243D90(arg0, 0x40, 0);
    }
}

void func_8024AF24(s32 arg0, f32 *arg1) {
    PORT_IP(s32) obj = (PORT_IP(s32)) func_802443F4(arg0); /* unused (its inlined load is the original's) */

    D_802AC998[arg0]--;
    if (arg1[2] > 60.0 && arg1[0] != D_802ACA30[arg0]) {
        D_802AC9B0[arg0]--;
    }
    if (D_802AC998[arg0] == 0 || D_802AC9B0[arg0] == 0) {
        func_80247F90(arg0);
    }
}

void func_8024ABA8(s32 arg0) {
    s32 flag;
    PORT_IP(s32) obj = (PORT_IP(s32)) func_802443F4(arg0); /* unused (its inlined load is the original's) */

    if (D_802AC960 != 0) {
        if (D_800AF1D8[D_802AC960] == -1) {
            D_800AF1D8[D_802AC960] = D_800AF1D8[D_802AC960 - 1];
        }
        if (D_800AF5D8[D_802AC960] == -1) {
            D_800AF5D8[D_802AC960] = D_800AF5D8[D_802AC960 - 1];
        }
        if (D_802ACA90[D_802AC960] == -1.0f) {
            D_802ACA90[D_802AC960] = D_802ACA90[D_802AC960 - 1];
        }
        if (D_800AF9D8[D_802AC960] == -1) {
            D_800AF9D8[D_802AC960] = D_800AF9D8[D_802AC960 - 1];
        }
        if (D_800AFDD8[D_802AC960] == -1) {
            D_800AFDD8[D_802AC960] = D_800AFDD8[D_802AC960 - 1];
        }
        if (D_800B01D8[D_802AC960] == -1) {
            D_800B01D8[D_802AC960] = D_800B01D8[D_802AC960 - 1];
        }
        if (D_800B05D8[D_802AC960] == -1) {
            D_800B05D8[D_802AC960] = D_800B05D8[D_802AC960 - 1];
        }
        if (D_800B09D8[D_802AC960] == -1) {
            D_800B09D8[D_802AC960] = D_800B09D8[D_802AC960 - 1];
        }
        if (D_800B0DD8[D_802AC960] == -1) {
            D_800B0DD8[D_802AC960] = D_800B0DD8[D_802AC960 - 1];
        }
        if (D_800B11D8[D_802AC960] == -1) {
            D_800B11D8[D_802AC960] = D_800B11D8[D_802AC960 - 1];
        }
    }
    switch (D_800AF1D8[D_802AC960]) {
        case 'W':
        case 'w':
            func_8024A824(D_800AF5D8[D_802AC960], D_800AF9D8[D_802AC960], D_802ACA90[D_802AC960],
                          D_800AFDD8[D_802AC960], 0, D_800B01D8[D_802AC960], D_800B05D8[D_802AC960]);
            break;
        case 'S':
        case 's':
            if (D_800B09D8[D_802AC960] == 'T' || D_800B09D8[D_802AC960] == 't') {
                func_8024A0DC(0x40000);
            }
            break;
        case 'M':
        case 'm':
            switch (D_800B11D8[D_802AC960]) {
                case '0':
                    flag = 0x200;
                    break;
                case '1':
                    flag = 0x80;
                    break;
                case '2':
                    flag = 0x40;
                    break;
                case '3':
                    flag = 0x100000;
                    break;
                case '4':
                    flag = 0x800;
                    break;
                case '5':
                    flag = 0x4000;
                    break;
                case '6':
                    flag = 0x400;
                    break;
                case '7':
                    flag = 0x10000;
                    break;
                case '8':
                    flag = 0x20000;
                    break;
                case '9':
                    flag = 0x200000;
                    break;
                case 'A':
                    flag = 0x400000;
                    break;
                case 'B':
                    flag = 0x4000000;
                    break;
            }
            func_8024A0DC(flag);
            break;
    }
    D_802AC960++;
    if (D_800AF1D8[D_802AC960] == -2) {
        func_80244354(arg0, 0x800, 0);
    }
}

void func_8024A824(s32 arg0, s32 arg1, f32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6) {
    s32 id;
    f32 angle;
    f32 dist;
    Unk8026DCAC ipos;
    f32 fpos[3];
    f32 cur[3];
    f32 tgt[3];
    UnkStructA *obj;
    f32 dir[3];

    id = func_80243DDC();
    obj = func_802443F4(id);
    if (arg0 != 0x400) {
        D_802AC8BC = arg0;
    }
    if (arg2 != 32768.0f) {
        D_802ACA24 = arg2;
    }
    if (arg1 != 6) {
        D_802AC95C = arg1;
    }
    D_802AC8C4 = arg3;
    if (arg5 != 6) {
        D_802AC97C = arg5;
    }
    D_800AEDF0[id].unk48 = arg6;
    if (arg6 == 0) {
        obj->unk70 = 0.0f;
    }
    D_802AC968[id] = arg5 * 2;
    func_8026DCAC(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, &ipos, 2);
    func_8026E338(ipos.i[0], ipos.i[1], ipos.i[2], fpos, 0);
    D_802ACA28 = fpos[0];
    D_802ACA2C = fpos[1];
    D_802ACA8C = fpos[2];
    D_802ACA10[id] = 0;
    func_8022F808(obj, cur);
    tgt[0] = fpos[0];
    tgt[1] = cur[1];
    tgt[2] = fpos[2];
    dist = func_8022DF68(cur, tgt);
    func_80244354(id, 0x10420, 1);
    D_802AC918[id] = 0;
    func_802313DC(obj, 0x40000);
    func_80247C50(id);
    if (dist > 0.0f) {
        func_8022FF58(cur, tgt, dir);
        D_802AC8C0 = func_80243F50(dir[0]);
    } else {
        D_802AC8C0 = D_802AC8BC;
    }
    if (D_802AC8BC != 0x100 && D_802AC8BC != 0x200) {
        switch (D_802AC8C0) {
        case 0x0:
            angle = 180.0f;
            break;
        case 0x80:
            angle = 0.0f;
            break;
        case 0xC0:
            angle = 270.0f;
            break;
        case 0x40:
            angle = 90.0f;
            break;
        case 0x20:
            angle = 135.0f;
            break;
        case 0xE0:
            angle = 225.0f;
            break;
        case 0x60:
            angle = 45.0f;
            break;
        case 0xA0:
            angle = 315.0f;
            break;
        }
        func_8024A6B4(id, angle);
    }
    if (arg4 != 0) {
        while (func_80244BA4(id, 0x400)) {
            func_80239094();
        }
    }
}

void func_8024A6B4(s32 arg0, f32 arg1) {
    UnkStruct_800AEDF0 *e = &D_800AEDF0[arg0];
    UnkStructA *o = func_802443F4(arg0);
    f32 lo;
    f32 hi;

    if (arg1 != o->unk40->unk20) {
        e->unk58 = arg1;
        e->unk64 = o->unk40->unk20;
        if (o->unk40->unk20 < arg1) {
            lo = o->unk40->unk20;
            hi = e->unk58;
        } else {
            hi = o->unk40->unk20;
            lo = e->unk58;
        }
        if ((lo + 360.0f) - hi < hi - lo) {
            if (lo == e->unk58) {
                e->unk58 += 360.0f;
            } else {
                e->unk58 -= 360.0f;
            }
            if (e->unk58 == -360.0f) {
                e->unk58 = 360.0f;
            }
        }
        if (o->unk40->unk20 < e->unk58) {
            e->unk5C = 20.0f;
        } else {
            e->unk5C = -20.0f;
        }
        e->unk60 = o->unk40->unk20;
        func_80244354(arg0, 0x8000000, 1);
    }
}

void func_8024A0DC(s32 arg0) {
    UnkStructA *obj;
    s32 hit;
    PORT_IP(s32) t2;
    s32 t3;
    PORT_IP(s32) t4;
    Unk8026DCAC ipos;
    s32 t5;
    s32 id;
    s32 flags;
    s16 sid;

    id = func_80243DDC();
    obj = func_802443F4(id);
    D_802AC980[id] = arg0;
    switch (arg0) {
    case 0x200:
        hit = func_80232D18(obj, 2);
        if (hit != 0) {
            func_80244354(id, 0x200, 0);
            D_800AEDF0[id].unk4C = 0;
            func_80249810(id, (PORT_IP(s32)) func_8022F550(obj, 2), 2);
        } else {
            func_80247928(id, PORT_HOST(((UnkStructA *)func_80274F8C(((E88 *)&D_800AEDF0[id])->unk1A, obj->unk40->unk10,
                                                           obj->unk40->unk14, obj->unk40->unk18, 0))->unk1C, (PORT_IP(s32)) ((Port_Ent88 *) func_80274F8C(((E88 *)&D_800AEDF0[id])->unk1A, obj->unk40->unk10,
                                                           obj->unk40->unk14, obj->unk40->unk18, 0))->unk1C));
        }
        break;
    case 0x80:
        sid = id;
        t2 = func_80270714(sid);
        if (!t2) {
            D_800AEDF0[id].unk2C = func_80271540(sid);
            if (D_800AEDF0[id].unk2C != 0) {
                func_80249FD4(id, 0);
                func_80244354(id, 0x90000, 1);
                D_802AC918[id] = 0;
                func_802313DC(obj, 0x40000);
            } else {
                func_8026DCAC(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, &ipos, 2);
                flags = func_8026D7D8(ipos.i[0], ipos.i[1], ipos.i[2]);
                if (func_802452AC(id, 0x8100) && !(flags & 0x4000)) {
                    t3 = func_80274994(sid);
                    if (t3) {
                        func_80249FD4(id, 1);
                    }
                }
                func_80244354(id, 0x90000, 1);
                D_802AC918[id] = 0;
                func_802313DC(obj, 0x40000);
            }
        }
        break;
    case 0x40:
        t4 = func_80270714((s16)id);
        if (t4) {
            func_80244354(id, 0x80000, 1);
            func_80249F5C(id);
        }
        break;
    case 0x40000:
        D_802AC9C8[id] = D_800B0DD8[D_802AC960];
        func_80244354(id, 0x90020, 1);
        D_802AC918[id] = 0;
        func_802313DC(obj, 0x40000);
        break;
    case 0x100000:
        obj = func_802443F4(id);
        obj->unk68 = 40.0f;
        func_80244354(id, 0x90020, 1);
        D_802AC918[id] = 0;
        func_802313DC(obj, 0x40000);
        break;
    case 0x4000000:
        func_80244354(id, 0x90000, 1);
        D_802AC918[id] = 0;
        func_802313DC(obj, 0x40000);
        func_80244354(id, 0x04000020, 1);
        func_802444B8(id, 0x15);
        break;
    case 0x400:
        func_80244354(id, 0x90000, 1);
        func_80243D90(id, 0x400, 1);
        func_80244354(id, 0x20, 1);
        break;
    case 0x800:
        func_8027750C((s16)id);
        break;
    case 0x4000:
        func_802712C4(func_8027799C((s16)func_80274F64((s16)id)));
        break;
    case 0x10000:
        func_80244354(id, 0x80000, 1);
        func_80249E30(id);
        break;
    case 0x20000:
        func_80244354(id, 0x80000, 1);
        func_80249D74(id);
        break;
    case 0x200000:
        func_80244354(id, 0x80000, 1);
        func_80249D30(id);
        break;
    case 0x400000:
        if (D_802AC8BC != 0 && D_802AC8BC != 0x20 && D_802AC8BC != 0x40 && D_802AC8BC != 0x60 &&
            D_802AC8BC != 0x80 && D_802AC8BC != 0xA0) {
        if (!flags && !flags) {} /* fake: dead read in an empty if (ugen temporaries); p10: only a test of flags (unset in this case) keeps the if: any other DEBUG_PRINT condition loses it */
        }
        func_80244354(id, 0x420, 0);
        t5 = func_80244BA4(id, 0x02000000);
        if (!t5) {
            func_80244354(id, 0x10000, 0);
        }
        D_800AEDF0[id].unk48 = 1;
        obj->unk70 = 1.6333334f;
        break;
    }
}

void func_80249FD4(s32 arg0, s32 arg1) {
    Tbl4_5E3F0 tbl;
    UnkStruct_800AEDF0 *p;

    tbl = *(Tbl4_5E3F0 *)D_802A0DA4;
    func_802444B8(arg0, 4);
    func_80244354(arg0, 0xA0, 1);
    p = &D_800AEDF0[arg0];
    p->unk3C = arg1;
    if ((D_802AC5D0 & 4) || arg0 == 0) {
        func_802479F0(tbl.v);
    }
    if (func_80243EB4(arg0, 0x2000)) {
        s32 k = p->unkA4;
        D_802ACE90[k] = 60.0f;
    }
    p->unk38 &= ~0x80;
}

void func_80249F5C(s32 arg0) {
    UnkStruct_800AEDF0 *temp_v0;

    func_802444B8(arg0, 6);
    func_80244354(arg0, 0x60, 1);
    func_80244354(arg0, 0x8000, 0);
    temp_v0 = &D_800AEDF0[arg0];
    temp_v0->unkAC = 0;
    temp_v0->unk38 &= ~0x40;
}

void func_80249F24(s32 arg0) {
    func_80244354(arg0, 0x04000020, 1);
    func_802444B8(arg0, 0x15);
}

void func_80249EEC(s32 arg0) {
    func_80243D90(arg0, 0x400, 1);
    func_80244354(arg0, 0x20, 1);
}

void func_80249E30(s32 arg0) {

    if (func_80244BA4(arg0, 0x40)) {
        func_80244BE0(arg0, 1);
    }
    func_80244354(arg0, 0x148, 0);
    func_80244354(arg0, 0x20, 1);
    func_80243D90(arg0, 0x10000, 1);
    func_802444B8(arg0, 0x17);
}

void func_80249D74(s32 arg0) {
    UnkStructA *o;

    if (!func_80243EB4(arg0, 0x20000)) {
        o = func_802443F4(arg0);
        func_80243D90(arg0, 0x20000, 1);
        D_800AEDF0[arg0].unkB8 = 60;
        D_800AEDF0[arg0].unkB4 = (PORT_IP(s32))func_80231274(0x4E, 0x11200000, 0xD0, o);
        PORT_FIELD(D_800AEDF0[arg0].unkB4, f32 *, 0x4C, Port_UnkStructA, unk48[4]) = 0.0f;
        PORT_FIELD(D_800AEDF0[arg0].unkB4, f32 *, 0x50, Port_UnkStructA, unk48[8]) = 200.0f;
        PORT_FIELD(D_800AEDF0[arg0].unkB4, f32 *, 0x54, Port_UnkStructA, unk48[12]) = 0.0f;
    }
}

void func_80249D30(s32 arg0) {
    func_80244354(arg0, 0x20, 1);
    func_80243D90(arg0, 0x200000, 1);
    func_802444B8(arg0, 0x18);
}

void func_80249AD4(s32 arg0) {
    s32 t1;
    s32 cur = func_80243DDC();
    s32 t2;
    s32 flag;

    switch (D_802AC980[cur]) {
        case 0x40000:
            if (--D_802AC9C8[cur] == 0) {
                func_80244354(cur, 0x80020, 0);
            }
            break;
        case 0x100000:
            if (PORT_FIELD(func_802443F4(cur), f32 *, 0x68, Port_UnkStructA, unk48[32]) == 0.0f) {
                flag = D_802AC980[cur] | 0x80020;
                func_80244354(cur, flag, 0);
                t1 = func_80244BA4(cur, 0x2000000);
                if (!t1) {
                    func_80244354(cur, 0x10000, 0);
                }
            }
            break;
        case 0x400:
        case 0x4000:
        case 0x10000:
        case 0x4000000:
            func_80244354(cur, 0x80000, 0);
            t2 = func_80244BA4(cur, 0x2000000);
            if (!t2) {
                func_80244354(cur, 0x10000, 0);
            }
            break;
        default:
            if (!func_80244BA4(cur, D_802AC980[cur])) {
                flag = D_802AC980[cur] | 0x80000;
                func_80244354(cur, flag, 0);
                if (!func_80244BA4(cur, 0x2000000)) {
                    func_80244354(cur, 0x10000, 0);
                }
            }
            break;
    }
}

void func_80249A30(s32 arg0) {
    Unk8026DCAC ipos;
    UnkStructA *obj;

    obj = func_802443F4(arg0);
    func_8026DCAC(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, &ipos, 2);
    if (func_8026E278(func_8026D7D8(ipos.i[0], ipos.i[1], ipos.i[2]))) {
        func_80244354(arg0, 0x10, 1);
    } else {
        func_80244354(arg0, 0x10, 0);
    }
}

void func_802498D4(s32 arg0, f32 arg1) {
    UnkStructA *obj;
    PORT_IP(s32) r;

    obj = func_802443F4(arg0);
    r = (PORT_IP(s32)) func_8022F4AC(obj, 2, PORT_PTR(D_800AEDF0[arg0].unk4C));
    if (r != 0 && arg1 > 0.0f) {
        if (--D_802AC948[arg0] == 0) {
            func_80244354(arg0, 1, 0);
            if (r != 0 && func_802452AC(arg0, 0x11C0)) {
                if (obj->unk6C == 0.0f) {
                    func_80249810(arg0, r, 1);
                } else if (!func_80244BA4(arg0, 0x10000)) {
                    func_80247D74(arg0, 22.5f, 8.0f, 1, NULL);
                }
            }
        }
    } else {
        func_80244354(arg0, 1, 0);
    }
}

void func_80249810(s32 arg0, PORT_IP(s32) arg1, s32 arg2) {
    if (!func_80244BA4(arg0, 0x8000)) {
        func_802444B8(arg0, 7);
    } else {
        func_802444B8(arg0, 0x10);
    }
    func_80244354(arg0, 0x120, 1);
    D_800AEDF0[arg0].unk30 = arg1;
    D_800AEDF0[arg0].unkBC = arg2;
    D_800AEDF0[arg0].unk38 &= ~0x100;
}

void func_802497D8(s32 arg0) {
    D_802AC948[arg0] = 2;
    func_80244354(arg0, 1, 1);
}

void func_80249648(s32 arg0, UnkStructA *arg1, s32 arg2) {
    UnkStruct_800AEDF0 *p = &D_800AEDF0[arg0];
    UnkStructA *obj;

    if (func_802452AC(arg0, 0x610400)) {
        obj = func_802443F4(arg0);
        if (!(D_802A01B0 & 2) && (arg2 & 0x1E00) && PORT_FIELD(obj, PORT_IP(s32) *, 0x44, Port_UnkStructA, unk44) == 0) {
            if ((D_802AC5D0 & 4) || arg0 == 0) {
                func_8026C660(0x196);
            }
            func_80247D74(arg0, 14.0f, 6.0f, 1, arg1);
            p->unk9C = arg2;
            func_80244354(arg0, 0x2108, 0);
            if (p->unk9C & 0x200) {
                func_8024946C(arg0, 0);
            } else if (p->unk9C & 0x400) {
                func_8024946C(arg0, 1);
            } else if (p->unk9C & 0x800) {
                func_8024946C(arg0, 2);
            } else if (p->unk9C & 0x1000) {
                func_80247C50(arg0);
            }
            p->unk9C = 0;
        }
    }
}

void func_8024946C(s32 arg0, s32 arg1) {
    UnkStructA *o;
    f32 pos[3];

    o = func_802443F4(arg0);
    if (!func_80244BA4(arg0, 0x1000)) {
        switch (arg1) {
            case 0:
                D_802AC8D0[arg0] = 1200;
                D_802AC900[arg0] = 80;
                break;
            case 1:
                D_802AC8D0[arg0] = 600;
                D_802AC900[arg0] = 50;
                break;
            case 2:
                D_802AC8D0[arg0] = 300;
                D_802AC900[arg0] = 30;
                break;
        }
        func_80247848(arg0, 1);
        pos[0] = 0.0f;
        pos[1] = 80.0f;
        pos[2] = 0.0f;
        if (D_802AC8E8[arg0] == -1) {
            D_802AC8E8[arg0] = func_8028A7CC(5, pos, (PORT_IP(s32))o);
        }
        func_8022979C((PORT_RET(s32, void *)) o->unk40, 4, 12);
        func_8022979C((PORT_RET(s32, void *)) o->unk40, 5, 13);
        D_802ACA30[arg0] = 0.0f;
    }
    func_80244354(arg0, 0x1C0, 0);
    func_80244354(arg0, 0x1020, 1);
    D_800AEDF0[arg0].unk38 &= ~0x1000;
}

void func_8024891C(s32 arg0) {
    s32 other;
    s32 p18;
    s32 p68;
    s32 q18;
    s32 q68;
    s32 a;
    s32 b;
    UnkStructA *obj;
    UnkStructA *hit;
    UnkStructA *oa;
    UnkStructA *ob;
    f32 vec[3];
    f32 f;

    obj = func_802443F4(arg0);

    hit = func_8022F550(obj, 1);
    if (hit != NULL) {
        other = func_802441AC(hit);
        if ((func_80243EB4(arg0, 0x10) || func_80243EB4(other, 0x10)) && !func_80243EB4(arg0, 0x200) &&
            !func_80243EB4(other, 0x200) && !func_80243EB4(arg0, 0x80) && !func_80243EB4(other, 0x80) &&
            !func_80243EB4(arg0, 0x40) && !func_80243EB4(other, 0x40)) {
            p18 = D_800AEDF0[arg0].unk18 & 0x404;
            q18 = D_800AEDF0[other].unk18 & 0x404;
            p68 = D_800AEDF0[arg0].unk68 & 0x3D;
            q68 = D_800AEDF0[other].unk68 & 0x3D;
            D_800AEDF0[arg0].unk68 &= ~0x3F;
            D_800AEDF0[arg0].unk18 &= ~0x404;
            D_800AEDF0[other].unk68 &= ~0x3F;
            D_800AEDF0[other].unk18 &= ~0x404;
            D_800AEDF0[other].unk18 |= p18;
            D_800AEDF0[other].unk68 |= p68;
            D_800AEDF0[arg0].unk18 |= q18;
            D_800AEDF0[arg0].unk68 |= q68;
            if (!func_80243EB4(arg0, 4) && !func_80243EB4(arg0, 1)) {
                if (p68 & 4) {
                    func_8024694C(arg0, 1, 0);
                }
                if (p68 & 1) {
                    func_8024694C(arg0, 2, 0);
                }
            }
            if (!func_80243EB4(arg0, 0x40)) {
                if (func_80243EB4(arg0, 4)) {
                    func_8024694C(arg0, 1, 1);
                }
                if (func_80243EB4(arg0, 1)) {
                    func_8024694C(arg0, 2, 1);
                }
                if (func_80243EB4(arg0, 8)) {
                    func_802488C0(arg0);
                }
                if (func_80243EB4(arg0, 0x20)) {
                    func_80248864(arg0);
                }
                if (D_800AEDF0[arg0].unk18 & 0x404) {
                    func_80246C3C(arg0, 3, 1);
                }
            }
            if (!func_80243EB4(other, 4) && !func_80243EB4(other, 1)) {
                if (q68 & 4) {
                    func_8024694C(other, 1, 0);
                }
                if (q68 & 1) {
                    func_8024694C(other, 2, 0);
                }
            }
            if (!func_80243EB4(other, 0x40)) {
                if (func_80243EB4(other, 4)) {
                    func_8024694C(other, 1, 1);
                }
                if (func_80243EB4(other, 1)) {
                    func_8024694C(other, 2, 1);
                }
                if (func_80243EB4(other, 8)) {
                    func_802488C0(other);
                }
                if (func_80243EB4(other, 0x20)) {
                    func_80248864(other);
                }
                if (D_800AEDF0[other].unk18 & 0x404) {
                    func_80246C3C(other, 3, 1);
                }
            }
            func_80243D90(arg0, 0x40, 1);
            D_800AEDF0[arg0].unk7C = 30;
            func_80243D90(other, 0x40, 1);
            D_800AEDF0[other].unk7C = 30;
        }
        a = -1;
        if (func_80243EB4(arg0, 0x200) && !func_80244BA4(arg0, 0x1000) && !func_80243EB4(other, 0x2000) &&
            !func_80243EB4(other, 0x200) && !func_80243EB4(other, 0x80) && D_800AEDF0[other].unk14 != 0 &&
            obj->unk44 == NULL && obj->unk6C == 0.0f) {
            a = other;
            b = arg0;
        }
        if (!func_80243EB4(arg0, 0x2000) && !func_80243EB4(arg0, 0x80) && !func_80243EB4(arg0, 0x200) &&
            D_800AEDF0[arg0].unk14 != 0 && func_802452AC(other, 0x401000) && func_80243EB4(other, 0x200) &&
            hit->unk44 == NULL && obj->unk6C == 0.0f) {
            a = arg0;
            b = other;
        }
        if (a != -1) {
            oa = func_802443F4(a);
            ob = func_802443F4(b);
            if (D_800AEDF0[a].unk2C != 0) {
                func_80244C30(a, 1);
            }
            if (D_800AEDF0[b].unk2C != 0) {
                func_80244C30(b, 1);
            }
            func_80243D90(a, 0x2000, 1);
            D_800AEDF0[a].unkA4 = b;
            func_80232AC8((PORT_RET(s32, void *)) oa, (PORT_RET(s32, void *)) ob);
            func_80231408(ob, 0x600000);
            D_800AEDF0[b].unkC0 = 70.0f;
            if (!func_80243EB4(a, 2)) {
                if (func_80243EB4(a, 4)) {
                    D_800AEDF0[b].unkC0 += 30.0f;
                }
                if (func_80243EB4(a, 1)) {
                    D_800AEDF0[b].unkC0 -= 40.0f;
                }
            } else {
                f = D_800AEDF0[a].unk74;
                if (func_80243EB4(a, 4)) {
                    if (D_800AEDF0[a].unk6C > 0.0f) {
                        D_800AEDF0[b].unkC0 += 6.0f * f;
                    }
                    if (D_800AEDF0[a].unk6C < 0.0f) {
                        D_800AEDF0[b].unkC0 += 6.0f * (4.0f - f);
                    }
                }
                if (func_80243EB4(a, 1)) {
                    if (D_800AEDF0[a].unk6C > 0.0f) {
                        D_800AEDF0[b].unkC0 += -8.0f * f;
                    }
                    if (D_800AEDF0[a].unk6C < 0.0f) {
                        D_800AEDF0[b].unkC0 = (4.0f - f) * -8.0f;
                    }
                }
            }
            D_802ACE90[b] = 60.0f;
            vec[1] = 60.0f;
            vec[2] = D_800AEDF0[b].unkC0;
            if (func_80244BA4(a, 0x40)) {
                vec[1] = 37.0f;
            }
            vec[0] = 180.0f;
            func_8022FEAC(vec, &ob->unk4C);
            ob->unk70 = 0.0f;
            func_802444B8(b, 0x12);
            D_802AC998[a] = 600;
            D_802AC9B0[a] = 50;
            D_800AEDF0[a].unkA8 = b;
            func_80244354(b, 2, 0);
        }
    }
}

void func_802488C0(s32 arg0) {
    func_80243D90(arg0, 0x18, 1);
    D_800AEDF0[arg0].unk78 = 0x4B0;
    func_80246C3C(arg0, 3, 1);
}

void func_80248864(s32 arg0) {
    func_80243D90(arg0, 0x30, 1);
    D_800AEDF0[arg0].unk78 = 0x4B0;
    func_80246C3C(arg0, 3, 1);
}

void func_8024878C(s32 arg0) {
    s32 t1;
    s32 t2;
    s32 t3;

    func_80244354(arg0, 2, 0);
    do {} while (0); /* fake: splits the block (p10: no DEBUG_PRINT check of arg0 here; direct-call tests, an early return, a ternary, `id = arg0`: no; an unused `s32 *p = func_80243D54(arg0)` matches but is no better) */
    t1 = func_80244BA4(arg0, 0x1000);
    if (!t1) {
        func_80244354(arg0, 0x20, 0);
        t2 = func_802452AC(arg0, 0x20C0);
        if (!t2) {
            t3 = func_80244BA4(arg0, 0x8000);
            if (!t3) {
                func_802444B8(arg0, 0);
            } else {
                func_802444B8(arg0, 0xC);
            }
        }
    }
}

void func_8024857C(s32 arg0) {
    s32 t1;
    Cell5_5E3F0 c;
    UnkStructA *obj;
    s32 r;

    obj = func_802443F4(arg0);
    func_8026DCAC(obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, &c, 2);
    if (c.h == 0.0f) {
        r = func_8026D7D8(c.x, c.y, c.z) & 0xF;
    } else {
        r = 1;
    }
    switch (r) {
    case 2:
        if (!(D_802AC8A8[arg0] & 1)) {
            D_802AC8A8[arg0] = 1;
            func_80243824(0x50);
            t1 = func_80244BA4(arg0, 0x1000);
            if (t1) {
                func_80247C50(arg0);
            }
        }
        break;
    case 9:
        if (!func_80244BA4(arg0, 0x10000) && !(D_802AC8A8[arg0] & 2)) {
            func_80248148(arg0, 4);
        }
        break;
    case 10:
        if (!(D_802AC8A8[arg0] & 4)) {
            D_802AC8A8[arg0] = 4;
            func_80243824(0x52);
        }
        break;
    case 11:
        if (!(D_802AC8A8[arg0] & 8)) {
            D_802AC8A8[arg0] = 8;
            func_80243824(0x53);
        }
        break;
    default:
        if (D_802AC8A8[arg0] != 0) {
            D_802AC8A8[arg0] = 0;
        }
        break;
    }
}

void func_80248148(s32 arg0, s32 arg1) {
    UnkStructA *obj;
    s32 link;
    s32 t1;
    s32 other;

    if (D_800AEDF0[arg0].unk24 != 3) {
        if (arg1 == 3 || arg1 == 5) {
            D_800AEDF0[arg0].unk14 = 0;
        } else {
            D_800AEDF0[arg0].unk14--;
        }
        func_802444B8(arg0, 0);
        obj = func_802443F4(arg0);
        func_80244354(arg0, 0x21C0, 0);
        link = D_802AC5D0 & 4;
        func_80244BE0(arg0, 1);
        if (D_802AC8E8[arg0] != -1) {
            func_8028A4E0(D_802AC8E8[arg0]);
            D_802AC8E8[arg0] = -1;
            func_8022979C((PORT_RET(s32, void *)) obj->unk40, 4, 4);
            func_8022979C((PORT_RET(s32, void *)) obj->unk40, 5, 5);
        }
        func_80247848(arg0, 0);
        if (D_800AEDF0[arg0].unk14 == 0) {
            t1 = func_8023A870(arg0, arg1);
            if (t1) {
                if (link) {
                    if (func_80243EB4(arg0, 0x2000)) {
                        func_80247F90(arg0);
                    }
                    if (obj->unk44 != NULL) {
                        other = func_802441AC(obj->unk44);
                        func_80244C30(other, 0);
                        func_802444B8(other, 0);
                        func_80243D90(other, 0x2000, 0);
                        func_802313DC(obj, 0x600000);
                    }
                    if (!func_80243EB4(arg0, 0x200)) {
                        func_8027B0BC((s16)arg0);
                    }
                }
                func_802313DC(obj, 1);
                D_800AEDF0[arg0].unk24 = 3;
                {
                    s32 v = 0;
                    if (obj->unk6C > 0.0f) {
                        v = D_800AEDF0[arg0].unk10 & 0x400000;
                    }
                    D_800AEDF0[arg0].unk10 = v;
                }
            } else {
                func_80244354(arg0, 0x161, 0);
                D_800AEDF0[arg0].unk14 = 1;
            }
        } else {
            D_800AEDF0[arg0].unk2C = 0;
            D_802ACF60 = func_8023A840();
            if (D_802ACF60->unk10 == 0 || link) {
                func_8027B39C(4, (s16)arg0);
            } else {
                func_8028A81C(6, obj->unk40->unk10, obj->unk40->unk14 + 200.0f, obj->unk40->unk18, 0);
            }
            func_80247CD0(arg0, 3);
            func_8026C660(0x1D);
            if (arg0 == 1) {
                switch ((u32)D_802ACF60->unk4) {
                case 0:
                case 5:
                    func_8026C660(0x1CD);
                    break;
                case 1:
                    func_8026C660(0x1B4);
                    break;
                case 2:
                    func_8026C660(0x1B7);
                    break;
                case 3:
                    func_8026C660(0x1BA);
                    break;
                case 4:
                    func_8026C660(0x1C5);
                    break;
                }
            }
            if (arg0 == 2 && D_802ACF60->unk4 == 5) {
                func_8026C660(0x1BA);
            }
            if (obj->unk44 != NULL) {
                other = func_802441AC(obj->unk44);
                func_80244C30(other, 1);
                func_802444B8(other, 0);
                func_80243D90(other, 0x2000, 0);
                func_802313DC(obj, 0x600000);
            }
        }
        func_80247C50(arg0);
    }
}

void func_80247F90(s32 arg0) {
    UnkStructA *other;
    UnkStructA *self;
    f32 vec[3];

    self = func_802443F4(arg0);
    other = func_802443F4(D_800AEDF0[arg0].unkA8);
    vec[2] = D_800AEDF0[arg0].unkC0 + 100.0f;
    vec[1] = 60.0f;
    vec[0] = 180.0f;
    func_8022FEAC(vec, &other->unk4C);
    func_80231434(other);
    func_80232AA8((PORT_RET(s32, void *)) other);
    func_802313DC(other, 0x600000);
    if (func_80244BA4(arg0, 0x80)) {
        func_80244C30(arg0, 0);
    }
    if (!func_80244BA4(arg0, 0x1000)) {
        func_80244354(arg0, 0x20, 0);
    }
    func_80243D90(arg0, 0x2000, 0);
    if (func_80244BA4(arg0, 0x80)) {
        func_802444B8(D_800AEDF0[arg0].unkA8, 0xC);
    } else {
        func_802444B8(D_800AEDF0[arg0].unkA8, 0);
    }
    other->unk40->unk14 += 5.0f;
    func_80247D74(D_800AEDF0[arg0].unkA8, 22.5f, 8.0f, 1, self);
    other->unk70 = 1.6333334f;
    func_80244354(arg0, 0x21C0, 0);
}

void func_80247D74(s32 arg0, f32 arg1, f32 arg2, s32 arg3, UnkStructA *arg4) {
    UnkStructA *obj;
    f32 angle;
    f32 a[3];
    f32 b[3];
    Vec3f dir;

    obj = func_802443F4(arg0);
    ((UnkStructA *)obj)->unk68 = arg1;
    D_802ACA44 = arg2;
    D_802AC8C8 = arg3;
    if (arg4 != NULL) {
        if (arg3 == 1) {
            func_8022F808(obj, a);
            func_8022F808(arg4, b);
            b[1] = a[1];
            if (a[0] == b[0] && a[2] == b[2]) {
                angle = -1.0f;
            } else {
                func_8022FF58(a, b, &dir.x);
                angle = (dir.x + 180.0f) - 360.0f * (dir.x >= 180.0f);
                obj->unk40->unk20 = (angle + 180.0f) - 360.0f * (angle >= 180.0f);
            }
        } else {
            angle = (obj->unk40->unk20 + 180.0f) - 360.0f * (obj->unk40->unk20 >= 180.0f);
        }
    } else {
        angle = obj->unk40->unk20;
    }
    D_800AEDF0[arg0].unkA0 = func_80243F50(angle);
    if (arg3 == 1 && arg4 != NULL) {
        func_802444B8(arg0, 0xB);
        func_80244354(arg0, 0xA0C8, 0);
        func_80244BE0(arg0, 1);
        D_800AEDF0[arg0].unk2C = 0;
    }
    func_80244354(arg0, 0x22, 1);
}

void func_80247CD0(s32 arg0, s32 arg1) {
    UnkStructA *o;

    D_802AC930[arg0] = arg1;
    func_80244354(arg0, 0x30020, 1);
    D_802AC918[arg0] = 0;
    o = (UnkStructA *)(func_802443F4(arg0));
    D_802ACA48[arg0] = o->unk40->unk20;
    D_802ACA60[arg0] = D_802ACA48[arg0];
    func_80231408(o, 0x40000);
}

void func_80247C50(s32 arg0) {
    if (func_80244BA4(arg0, 0x1000)) {
        func_80247A54(arg0, (PORT_IP(s32)) func_8022F4AC(func_802443F4(arg0), 2, PORT_PTR(D_800AEDF0[arg0].unk4C)), 0, 1);
    }
}

void func_80247A54(s32 arg0, PORT_IP(s32) arg1, s32 arg2, s32 arg3) {
    Tbl3_5E3F0 tbl;
    s32 other;
    UnkStructA *obj;

    tbl = *(Tbl3_5E3F0 *)D_802A0D88;
    obj = func_802443F4(arg0);
    if (func_80244BA4(arg0, 0x200000)) {
        other = func_802441AC(PORT_FIELD(obj, void **, 0x44, Port_UnkStructA, unk44));
        func_80244BE0(other, 1);
        D_800AEDF0[arg0].unk2C = 0;
    }
    if (arg3 != 0 && !(D_802AC5D0 & 0x10) && ((D_802AC5D0 & 4) || arg0 == 0)) {
        func_802479F0(tbl.v);
    }
    func_80244354(arg0, 0x1020, 0);
    func_802444B8(arg0, 0);
    if (arg2 != 0) {
        func_8024796C(arg0, arg2);
    }
    if (arg1 != 0) {
        func_80247928(arg0, arg1);
    }
    if (D_802AC8E8[arg0] != -1) {
        func_8028A4E0(D_802AC8E8[arg0]);
        D_802AC8E8[arg0] = -1;
    }
    func_8022979C((PORT_RET(s32, void *)) obj->unk40, 4, 4);
    func_8022979C((PORT_RET(s32, void *)) obj->unk40, 5, 5);
    func_80247848(arg0, 0);
}

void func_802479F0(s32 *arg0) {
    s32 *var_v0;
    s32 var_v1;

    var_v1 = 0;
    if (*arg0 != -1) {
        var_v0 = arg0;
        do {
            var_v1 += 1;
            var_v0 += 1;
        } while (var_v0[0] != -1);
    }
    func_8026C660(arg0[func_80234248(var_v1 - 1)]);
}

void func_8024796C(s32 arg0, s32 arg1) {
    void *sp1C;

    sp1C = (func_802443F4(arg0));
    func_80244354(arg0, 0x108, 0);
    func_80244354(arg0, 0x10000, 1);
    D_802AC918[arg0] = arg1;
    func_80231408(sp1C, 0x40000);
}

void func_80247928(s32 arg0, PORT_IP(s32) arg1) {
    D_800AEDF0[arg0].unk4C = arg1;
    func_80244354(arg0, 0x200, 1);
}

void func_80247848(s32 arg0, s32 arg1) {
    if (arg1 != 0) {
        if (D_800AEDF0[arg0].unk54 == 0) {
            D_800AEDF0[arg0].unk54++;
            if (++D_802AC9DC == 1) {
                D_802AC9E0 = func_8026C660(0x21);
            }
        }
    } else {
        if (D_800AEDF0[arg0].unk54 != 0) {
            D_800AEDF0[arg0].unk54--;
            if (--D_802AC9DC == 0) {
                func_8026BE80(D_802AC9E0);
                D_802AC9E0 = -1;
            }
        }
    }
}

void func_80247614() {
    if (--D_802AC9EC == 0) {
        if (func_80243E18(0x20)) {
            func_80246F00(0x20, 0);
        }
        if (func_80243E18(0x40)) {
            func_80246F00(0x40, 0);
        }
        if (func_80243E18(0x10)) {
            func_80246F00(0x10, 0);
        }
        if (func_80243E18(0x80)) {
            func_80246F00(0x80, 0);
        }
        if (func_80243E18(4)) {
            func_80246F00(4, 0);
        }
        if (func_80243E18(2)) {
            func_80246F00(2, 0);
        }
        D_802AC9EC = 0;
        D_802AC9E8 = 0;
    }
    if (func_80243E18(0x40)) {
        func_80246808();
    }
    if (func_80243E18(0x10) && --D_802AC9F0 == 0) {
        func_80246078();
    }
    if (func_80243E18(0x80)) {
        func_80246018();
    }
    if (func_80243E18(4)) {
        func_80245FD4();
    }
}

void func_80246F00(s32 arg0, s32 arg1) {
    s32 t1;
    s32 x;
    s32 t2;
    s32 t3;
    s32 t4;
    s32 flags;
    s32 hit;
    s32 t5;
    f32 pos[3];
    UnkStruct_800AEDF0 *e;
    s32 i;
    s32 r;
    UnkStructA *obj;

    if (arg1 == 0 || !func_80243E18(1)) {
        switch (arg0) {
        case 0x20:
            if (arg1 != 0) {
                for (i = 0; i < 4; i++) {
                    e = &D_800AEDF0[i];
                    if (e->unk24 == 1 || e->unk24 == 2) {
                        e->unk94 = e->unk18;
                        e->unk98 = e->unk20;
                        e->unk18 |= 0x200;
                        e->unk18 |= 1;
                        func_80278E84((s16)i, 8);
                    }
                }
            } else {
                for (i = 0; i < 4; i++) {
                    if (D_800AEDF0[i].unk24 == 1 || D_800AEDF0[i].unk24 == 2) {
                        D_800AEDF0[i].unk18 = D_800AEDF0[i].unk94;
                        D_800AEDF0[i].unk20 = D_800AEDF0[i].unk98;
                    }
                }
            }
            break;
        case 0x40:
            if (arg1 != 0) {
                D_802ACEA4 = 0.0f;
                D_802ACEA8 = 0.0f;
                D_802ACEAC = 0.0f;
                func_8024663C();
            } else {
                func_80282144(0, 0, 0, 0.0f);
            }
            break;
        case 0x10:
            if (arg1 != 0) {
                func_80246078();
            }
            break;
        case 0x80:
            if (arg1 != 0) {
                func_80238A10(func_802500D4, 0, 0, 0, 0x130);
                D_802AC9F0 = 0xB4;
            }
            break;
        case 8:
            for (i = 0; i < 4; i++) {
                e = &D_800AEDF0[i];
                if (e->unk14 != 0 && (e->unk24 == 1 || e->unk24 == 2)) {
                    t1 = func_80243EF0(i);
                    if (!t1) {
                        func_80246D74(i);
                        func_8024694C(i, 1, arg1);
                    }
                }
            }
            D_802AC9EC = 0;
            D_802AC9E8 = 0;
            break;
        case 4:
            if (arg1 != 0) {
                x = func_80234248(13) + 17;
                t2 = func_80234248(D_800B15D8 - 1);
                t3 = func_80234248(10);
                func_8026E338(x, t2, t3 + 10, pos, 0);
                D_802ACF58 = func_802867E4(3, pos[0], pos[1], pos[2]);
                if (D_802ACF58 != NULL) {
                    obj = *D_802ACF58;
                    flags = M2C_FIELD(obj, s32 *, 8);
                    while (1) {
                        (*D_802ACF58)->unk40->unk14 = func_8026E938(obj->unk40);
                        func_802313DC(*D_802ACF58, flags);
                        func_80231408(*D_802ACF58, 0x10);
                        hit = func_80232D18(*D_802ACF58, 0x10);
                        r = func_8026DF74((*D_802ACF58)->unk40->unk10, (*D_802ACF58)->unk40->unk14, (*D_802ACF58)->unk40->unk18);
                        if (hit) {
                            M2C_FIELD(*D_802ACF58, s32 *, 0x10) = 0;
                            M2C_FIELD(*D_802ACF58, s32 *, 0x14) = 0;
                            PORT_FIELD(*D_802ACF58, PORT_IP(s32) *, 0x24, UnkStructA, unk24) = 0;
                        }
                        if (!(r & 0x2000) && !(r & 0x60) && (r & 0xF) == 1) {
                            obj = *D_802ACF58;
                            if (obj->unk40->unk14 != 20000.0f && !hit) {
                                func_80231408(obj, flags | 0x80000);
                                obj = *D_802ACF58;
                                func_8022F808(obj, (f32 *)&obj->unk58);
                                if (!obj->unk40 && !obj->unk40) {} /* fake: obj crosses the call, so it takes s0 (p10: DEBUG_PRINT checks of obj, here or after the case; obj for *D_802ACF58 in the loop's calls: no) */
                                func_8026C660(0x1EA);
                                break;
                            }
                        }
                        x = func_80234248(13) + 17;
                        t4 = func_80234248(D_800B15D8 - 1);
                        t5 = func_80234248(10);
                        func_8026E338(x, t4, t5 + 10, pos, 0);
                        (*D_802ACF58)->unk40->unk10 = pos[0];
                        (*D_802ACF58)->unk40->unk14 = pos[1];
                        (*D_802ACF58)->unk40->unk18 = pos[2];
                        obj = *D_802ACF58;
                    }
                } else {
                    arg0 = 0x10;
                    func_80246078();
                }
            } else if (!(func_802871D0((PORT_RET(s32, void *)) D_802ACF58) & 3)) {
                func_8022F808(*D_802ACF58, pos);
                pos[1] += 100.0f;
                func_8028A7CC(1, pos, 0);
                (*D_802ACF58)->unk40->unk14 = func_8026E8B8();
                func_802313DC(*D_802ACF58, 0x01000000);
                obj = *D_802ACF58;
                func_8022F808(obj, (f32 *)&obj->unk58);
                if (!obj->unk40 && !obj->unk40) {} /* fake: likewise */
            }
            if (D_802ACF58 == NULL) {
                DEBUG_PRINT(("D_802ACF58 == NULL\n"));
            }
            break;
        }
        func_80245FA0(arg0, arg1);
    }
    if (arg1 != 0) {
        D_802AC9EC = 0x708;
    }
    func_80245FA0(1, arg1);
    if (arg0 == 8) {
        D_802AC9EC = 0;
        D_802AC9E8 = 0;
    }
}

void func_80246D74(s32 arg0) {
    UnkStructA *obj;
    s32 id = arg0;
    UnkStruct_800AEDF0 *p = &D_800AEDF0[id];

    obj = func_802443F4(id);
    if (func_80243EB4(id, 4)) {
        func_80270770(id, 1);
        p->unk2C = 0;
        func_8024694C(id, 1, 0);
    }
    if (func_80243EB4(id, 1)) {
        func_80270770(id, 1);
        p->unk2C = 0;
        func_8024694C(id, 2, 0);
    }
    if (func_80243EB4(id, 0x80)) {
        func_802313DC(obj, 0x40);
    }
    if (func_80243EB4(id, 0x100)) {
        func_80246C3C(id, 0, 0);
    }
    func_80243D90(id, 0xB9, 0);
    if (!func_80243E18(1)) {
        func_80243D90(id, 4, 0);
    }
    p->unk18 &= ~0x404;
}

void func_80246C3C(s32 arg0, s32 arg1, s32 arg2) {
    UnkStructA *obj = func_802443F4(arg0);

    if (arg2 != 0) {
        func_80243D90(arg0, 0x100, 1);
        D_800AEDF0[arg0].unk8C = 0.0f;
        D_800AEDF0[arg0].unk88 = 0.2f;
        D_800AEDF0[arg0].unk84 = arg1;
        func_8029279C(obj->unk40, 0, 0, 0, 0.0f);
        D_800AEDF0[arg0].unk90 = 60;
        if (func_80243EB4(arg0, 0x80)) {
            D_800AEDF0[arg0].unk90 = 1200;
        }
        if (D_800AEDF0[arg0].unk18 & 0x404) {
            func_80243D90(arg0, 0x10, 1);
            D_800AEDF0[arg0].unk78 = 1200;
        }
    } else {
        if (func_80243EB4(arg0, 0x100)) {
            func_802926EC(obj->unk40);
            func_80243D90(arg0, 0x100, 0);
        }
    }
}

void func_8024694C(s32 arg0, s32 arg1, s32 arg2) {
    s32 other;
    UnkStructA *obj;

    if (func_80243EB4(arg0, 0x2000)) {
        other = D_800AEDF0[arg0].unkA8;
    }
    if (arg1 == 2) {
        if (arg2 != 0) {
            D_800AEDF0[arg0].unk6C = -0.1f;
            D_800AEDF0[arg0].unk70 = 0.5f;
            if (func_80243EB4(arg0, 0x2000)) {
                D_800AEDF0[other].unkC4 = -8.0f;
            }
            func_80243D90(arg0, 0x11, arg2);
        } else {
            D_800AEDF0[arg0].unk6C = 0.1f;
            D_800AEDF0[arg0].unk70 = 1.0f;
            if (func_80243EB4(arg0, 0x2000)) {
                D_800AEDF0[other].unkC4 = 8.0f;
            }
        }
    } else if (arg2 != 0) {
        D_800AEDF0[arg0].unk6C = 0.1f;
        D_800AEDF0[arg0].unk70 = 1.5f;
        if (func_80243EB4(arg0, 0x2000)) {
            D_800AEDF0[other].unkC4 = 6.0f;
        }
        func_80243D90(arg0, 0x14, arg2);
    } else {
        D_800AEDF0[arg0].unk6C = -0.1f;
        D_800AEDF0[arg0].unk70 = 1.0f;
        if (func_80243EB4(arg0, 0x2000)) {
            D_800AEDF0[other].unkC4 = -6.0f;
        }
    }
    if (arg2 != 0) {
        if (D_802AC9E8 == 0) {
            D_800AEDF0[arg0].unk78 = 1200;
        } else {
            D_800AEDF0[arg0].unk78 = 1800;
        }
    }
    if (func_80243EB4(arg0, 0x2000)) {
        obj = (void *)func_802443F4(arg0);
        D_800AEDF0[other].unkC0 = 70.0f;
        if (obj->unk40->unk28 > 1.0f) {
            D_800AEDF0[other].unkC0 += 30.0f;
        }
        if (obj->unk40->unk28 < 1.0f) {
            D_800AEDF0[other].unkC0 -= 40.0f;
        }
    }
    func_80243D90(arg0, 2, 1);
    func_80244BE0(arg0, 1);
    D_800AEDF0[arg0].unk74 = 0;
}

void func_80246938() {
    D_802AC9EC = 0;
    D_802AC9E8 = 0;
}

void func_80246808() {
    func_80282144((s32)D_802ACEA4, (s32)D_802ACEA8, (s32)D_802ACEAC, 50.0f);
    D_802ACEA4 += D_802ACEBC;
    D_802ACEA8 += D_802ACEC0;
    D_802ACEAC += D_802ACEC4;
    if (--D_802AC9F0 == 0) {
        D_802ACEA4 = D_802ACEB0;
        D_802ACEA8 = D_802ACEB4;
        D_802ACEAC = D_802ACEB8;
        func_8024663C();
    }
}

void func_8024663C() {
    s32 r;
    s32 done;

    do {
        done = 0;
        r = func_80234248(7);
        D_802ACEB8 = (r & 1) * 255.0f;
        D_802ACEB0 = ((r & 2) >> 1) * 255.0f;
        D_802ACEB4 = ((r & 4) >> 2) * 255.0f;
        func_80234248(2);
        D_802AC9F0 = 15;
        D_802ACEBC = (D_802ACEB0 - D_802ACEA4) / (f32)D_802AC9F0;
        D_802ACEC0 = (D_802ACEB4 - D_802ACEA8) / (f32)D_802AC9F0;
        D_802ACEC4 = (D_802ACEB8 - D_802ACEAC) / (f32)D_802AC9F0;
        if (D_802ACEBC != 0.0f || D_802ACEC0 != 0.0f || D_802ACEC4 != 0.0f) {
            done = 1;
        }
    } while (!done);
}

void func_80246604() {
    if (--D_802AC9F0 == 0) {
        func_80246078();
    }
}

void func_80246078() {
    s32 t1;
    s32 t2;
    s32 used[5];
    s32 n;
    s32 c[3];
    s32 t3;
    s32 flags[5];
    f32 x[5];
    f32 y[5];
    f32 z[5];
    f32 r[5];
    f32 pos[3];
    UnkStruct_800AEDF0 *p;
    UnkStructA *obj;
    s32 i;
    s32 k;
    s32 m;
    void *q;

    c[0] = 0;
    n = 0;
    for (i = 0; i < 4; i++) {
        p = &D_800AEDF0[i];
        if ((p->unk24 == 1 || p->unk24 == 2) && p->unk14 != 0 && !func_80244BA4(i, 0x1000) && !func_80243EF0(i)) {
            obj = func_802443F4(i);
            x[n] = obj->unk40->unk10;
            y[n] = obj->unk40->unk14;
            z[n] = obj->unk40->unk18;
            r[n] = M2C_FIELD(obj->unk40, f32 *, 0x20);
            used[n] = 0;
            flags[i] = M2C_FIELD(obj, s32 *, 8);
            func_802313DC(obj, flags[i]);
            func_80231408(obj, 0x01000010);
            n++;
        }
    }
    for (i = 0; i < 4; i++) {
        p = &D_800AEDF0[i];
        k = 0;
        if ((p->unk24 == 1 || p->unk24 == 2) && p->unk14 != 0 && !func_80244BA4(i, 0x1000) && !func_80243EF0(i)) {
            do {
                s32 j = func_80234248(n - 1);
                if (used[j] == 0) {
                    UnkStructA *o = func_802443F4(i);
                    o->unk40->unk10 = x[j];
                    o->unk40->unk14 = y[j];
                    o->unk40->unk18 = z[j];
                    M2C_FIELD(o->unk40, f32 *, 0x20) = r[j];
                    {
                        void *q2 = (void *)p->unk2C;
                        if (q2) {
                            func_80231434(q2);
                        }
                    }
                    t1 = func_80244BA4(i, 0x2000);
                    if (t1) {
                        func_80231434((void *) func_802443F4(p->unkA8));
                    }
                    k++;
                    used[j]++;
                }
            } while (k == 0);
        }
    }
    for (i = 0; i < 4; i++) {
        p = &D_800AEDF0[i];
        if ((p->unk24 == 1 || p->unk24 == 2) && p->unk14 != 0 && !func_80244BA4(i, 0x1000) && !func_80243EF0(i)) {
            t2 = func_80232D18((void *) func_802443F4(i), 0x10);
            if (t2 != 0) {
                c[0]++;
            }
            if ((q = (void *)p->unk2C) != 0 && func_80232D18(q, 0x10) != 0) {
                c[0]++;
            }
            t3 = func_80243EB4(i, 0x2000);
            if (t3 && func_80232D18((void *) func_802443F4(p->unkA8), 0x10) != 0) {
                c[0]++;
            }
        }
    }
    m = 0;
    for (i = 0; i != 4; i++) {
        p = &D_800AEDF0[i];
        obj = func_802443F4(i);
        if ((p->unk24 == 1 || p->unk24 == 2) && p->unk14 != 0 && !func_80244BA4(i, 0x1000) && !func_80243EF0(i)) {
            if (c[0] == 0) {
                func_8022F808(obj, pos);
                pos[1] += 50.0f;
                func_8028A7CC(2, pos, 0);
                func_8022F808(obj, (f32 *)PORT_AT(obj, 0x58, UnkStructA, unk58));
            } else {
                obj->unk40->unk10 = x[m];
                obj->unk40->unk14 = y[m];
                obj->unk40->unk18 = z[m];
                M2C_FIELD(obj->unk40, f32 *, 0x20) = r[m];
                m++;
            }
            func_80231408(obj, flags[i]);
        }
    }
    D_802AC9F0 = 300;
}

void func_80246018() {
    if (--D_802AC9F0 == 0) {
        func_80238A10(func_802500D4, 0, 0, 0, 0x130);
        D_802AC9F0 = 180;
    }
}

void func_80245FD4() {
    if (func_802871D0((PORT_RET(s32, void *)) D_802ACF58) & 3) {
        D_802AC9EC = 0;
        func_80245FA0(5, 0);
    }
}

/* func_80244354 for D_802AC9E8: its first line kept from that function
 * (a dead store; that it is a copy's leftover is a guess). */
void func_80245FA0(s32 arg0, s32 arg1) {
#ifdef TARGET_PC
    /* arg0 is a bit here (1 to 0x80, 93850.c's func_80246F00 calls), not an
       entry: the dead read reaches up to 0x6400 bytes past the table's five
       entries, into whatever follows it in the N64's memory, and nothing
       uses the value. The port doesn't read it. */
    s32 v;
#else
    s32 v = D_800AEDF0[arg0].unk10;
#endif

    v = D_802AC9E8;

    if (arg1 == 1) {
        v |= arg0;
    } else {
        v &= arg0 ^ -1;
    }
    D_802AC9E8 = v;
}

void func_80245F74(s32 arg0) {
    func_80245E2C(func_80243DDC(), arg0);
}

void func_80245E2C(s32 arg0, s32 arg1) {
    f32 v;
    Tbl8i_5E3F0 keys;
    Tbl8f_5E3F0 vals;
    s32 i;

    keys = *(Tbl8i_5E3F0 *)D_802A0E18;
    vals = *(Tbl8f_5E3F0 *)D_802A0E38;
    v = -1.0f;
    for (i = 0; i < 8 && v == -1.0f; i++) {
        if (arg1 == keys.v[i]) {
            v = vals.v[i];
        }
    }
    if (v != -1.0f) {
        D_800AEDF0[arg0].unkC = v;
        if (arg0 != -1) {
            M2C_FIELD((func_802443F4(arg0))->unk40, f32 *, 0x20) = v;
        }
    }
}

void func_80245E1C(s32 arg0, M2C_UNK arg1, M2C_UNK arg2) {

}

void func_80245DFC() {
    func_80245CDC();
}

void func_80245CDC() {
    s32 i;
    s32 v;

    D_802AC894 = 0;
    D_802ACF04 = malloc(0x1000);
    D_802AC898 = func_80238A10(func_8024FFF0, 0, (PORT_RET(s32, void *)) D_802ACF04, 0x1000, 0x70);
    for (i = 0; i < 5; i++) {
        if (D_800AEDF0[i].unk24 != 0) {
            D_800AEDF0[i].unk68 = 0;
            func_802459BC(i);
        }
    }
    func_80237664();
    func_802362D4(0x1F);
    D_802AC8BC = 0;
    D_802ACA24 = 2.0f;
    D_802AC95C = 3;
    D_802AC97C = 1;
    D_802AC9DC = 0;
    D_802AC9E0 = -1;
    D_802AC9E8 = 0;
    v = D_802AC5D0;
    if (v & 4) {
        func_80287008(3, 0x1C, 0xD2);
    }
}

void func_802459BC(s32 arg0) {
    s32 t1;
    s32 m;
    UnkStruct_800AEDF0 *p;
    UnkStructA *v;
    UnkStructA *obj;
    s32 i;

    v = func_80231274(0x49, 0x0D000011, 0x70, NULL);
    p = &D_800AEDF0[arg0];
    p->unk28 = v;
    v->unk40->unk10 = p->unk0;
    p->unk28->unk40->unk14 = p->unk4;
    p->unk28->unk40->unk18 = p->unk8;
    p->unk28->unk40->unk20 = p->unkC;
    PORT_FIELD(p->unk28, s32 *, 0x30, Port_UnkStructA, unk28[8]) = 1;
    PORT_FIELD(p->unk28, f32 *, 0x34, Port_UnkStructA, unk28[12]) = 30.0f;
    PORT_FIELD(p->unk28, f32 *, 0x38, Port_UnkStructA, unk28[16]) = 51.0f;
    p->unk28->unk6C = 0.0f;
    PORT_FIELD(p->unk28, f32 *, 0x70, Port_UnkStructA, unk48[40]) = 1.6333334f;
    PORT_FIELD(p->unk28, f32 *, 0x68, Port_UnkStructA, unk48[32]) = 0.0f;
    M2C_FIELD(p->unk28, s32 *, 0xC) = 0x1FF0;
    func_80229F74((PORT_RET(s32, void *)) p->unk28->unk40, 2);
    func_8022984C((PORT_RET(s32, void *)) p->unk28->unk40, 12);
    t1 = func_80243EB4(arg0, 0x200);
    if (t1) {
        obj = (UnkStructA *)p->unk28;
        func_8022979C((PORT_RET(s32, void *)) obj->unk40, 4, 4);
        func_8022979C((PORT_RET(s32, void *)) obj->unk40, 5, 5);
        func_80231408(obj, 0x80000);
        M2C_FIELD(obj->unk40, s16 *, 0x36) = 0x80;
        obj->unk40->unkC |= 0x1000;
    }
    for (i = 0; i < 12; i++) {
        switch (arg0) {
        case 0:
            m = 0x4A;
            break;
        case 1:
            m = 0x4B;
            break;
        case 2:
            m = 0x4C;
            break;
        case 3:
            m = 0x4D;
            break;
        }
        func_802297B8((PORT_RET(s32, void *)) p->unk28->unk40, m, i, i);
    }
    p->unkB0 = PORT_INT(func_802311E8(p->unk28));
    D_802AC8A8[arg0] = 1;
    D_802AC89C = 0;
    D_802AC8A0 = 0;
    func_802444B8(arg0, 0);
    D_802AC8E8[arg0] = -1;
    D_802ACA78[arg0] = 0.0f;
    p->unk10 = 0;
    p->unk68 &= 0x200;
    p->unk2C = 0;
    p->unk48 = 1;
    p->unk4C = 0;
    p->unk50 = 0;
    p->unk54 = 0;
    p->unk80 = 3;
    p->unk9C = 0;
    p->unkA4 = 0;
    p->unkA8 = 0;
    p->unkB4 = 0;
    p->unk38 = 0;
    p->unk18 &= ~0x404;
    p->unk74 = 0;
    p->unk40 = 0;
    D_802ACF08[arg0].unkC = 0;
    D_802ACF08[arg0].unkE = 0;
    D_802ACF08[arg0].unk8 = 0.0f;
    D_802ACF08[arg0].unk4 = 0.0f;
    D_802ACF08[arg0].unk0 = 0.0f;
}

void func_80245868() {
    s32 i;

    func_802362B8(0x7F);
    D_802AC9F4 = 0;
    for (i = 0; i < 5; i++) {
        if (D_800AEDF0[i].unk24 != 0 && func_802443F4(i) != NULL && PORT_HOST(*(s32 *)((u8 *)D_800AEDF0[i].unk28 + 0x44), (PORT_IP(s32)) ((Port_UnkStructA *) D_800AEDF0[i].unk28)->unk44) != 0) {
            func_80232AA8((PORT_RET(s32, void *)) D_800AEDF0[i].unk28);
        }
    }
    for (i = 0; i < 5; i++) {
        if (D_800AEDF0[i].unk24 != 0 && func_802443F4(i) != NULL) {
            func_8022FC90(D_800AEDF0[i].unk28);
            D_800AEDF0[i].unk28 = NULL;
            D_802AC894 = -1;
            D_800AEDF0[i].unkC = 0.0f;
        }
    }
    if (D_802AC898 != 0) {
        func_80238EA8(D_802AC898);
        D_802AC898 = 0;
    }
    if (D_802ACF04 != NULL) {
        free(D_802ACF04);
        D_802ACF04 = NULL;
    }
}

void func_8024585C(s32 arg0) {
    D_802AC894 = arg0;
}

void func_802457D8(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 id = func_80243DDC();

    func_8024557C(D_802ACEC8[id]);
    PORT_V0(func_80245430(D_802ACEC8[id], arg0, arg1, arg2, arg3));
}

s32 func_802456FC(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    s32 id = func_80243DDC();

    func_8024557C(D_802ACEC8[id]);
    return func_80245690(D_802ACEC8[id], arg0, arg1, arg2, arg3, arg4);
}

s32 func_80245690(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    s32 ret = 0;

    if (func_80245430(arg0, arg1, arg3, arg4, arg5) && arg2 == arg0[1]) {
        ret = 1;
    }
    return ret;
}

s32 func_802455A8(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    s32 id = func_80243DDC();

    func_8024557C(D_802ACEC8[id]);
    return func_80245480(D_802ACEC8[id], arg0, arg1, arg2, arg3, arg4, arg5);
}

void func_8024557C(s32 *arg0) {
    func_802454F8(func_80243DDC(), arg0);
}

void func_802454F8(s32 arg0, s32 *arg1) {
    Unk8026DCAC out;
    UnkStructA_40 *m = (func_802443F4(arg0))->unk40;

    func_8026DCAC(m->unk10, m->unk14, m->unk18, &out, 2);
    arg1[0] = out.i[0];
    arg1[1] = out.i[1];
    arg1[2] = out.i[2];
}

s32 func_80245480(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6) {
    s32 ret = 0;

    if (func_80245430(arg0, arg1, arg3, arg4, arg6) && arg0[1] >= arg2 && arg5 >= arg0[1]) {
        ret = 1;
    }
    return ret;
}

s32 func_80245430(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    s32 ret = 0;

    if (arg0[0] >= arg1 && arg0[2] >= arg2 && arg3 >= arg0[0] && arg4 >= arg0[2]) {
        ret = 1;
    }
    return ret;
}

s32 func_80245408(s32 arg0) {
    return D_800AEDF0[arg0].unk10;
}

void func_802452E8(s32 arg0) {
    s32 id = func_80243DDC();
    void *obj;
    s32 t1;

    if (id != -1) {
        obj = (void *)func_802443F4(id);
        func_80244354(id, 0x02010000, arg0);
        D_802AC918[id] = 0;
        func_802313DC(obj, 0x40000);
        if (arg0 == 1) {
            func_802313DC(obj, 0x40000);
            t1 = func_802452AC(id, 0x21C0);
            if (t1 && D_800AEDF0[id].unk14 > 0) {
                if (!func_80244BA4(id, 0x8000)) {
                    func_802444B8(id, 0);
                } else {
                    func_802444B8(id, 0xC);
                }
            }
        }
    }
}

s32 func_802452AC(s32 arg0, s32 arg1) {
    s32 ret = 0;

    if (!(D_800AEDF0[arg0].unk10 & arg1)) {
        ret = 1;
    }
    return ret;
}

void func_80245280(s32 arg0) {
    func_80245208(func_80243DDC(), arg0);
}

void func_80245208(s32 arg0, s32 arg1) {
    void *obj;

    if (arg0 != -1) {
        obj = (void *)func_802443F4(arg0);
        func_80244354(arg0, 0x02010000, arg1);
        D_802AC918[arg0] = 0;
        func_802313DC(obj, 0x40000);
    }
}

s32 func_80245150(s32 arg0) {
    s32 ret;
    s32 id = D_802AC9E4;

    if (D_800AEDF0[id].unk48 != 0) {
        if (func_8026E608(arg0, 0x11) == 1) {
            if (func_80243EB4(id, 0x80) && (arg0 & 0x60)) {
                ret = 1;
            } else {
                ret = 0;
            }
        } else {
            ret = 1;
        }
    } else {
        ret = 1;
    }
    return ret;
}

void func_8024512C(s32 arg0, s32 arg1) {
    D_800AEDF0[arg0].unk14 = arg1;
}

void func_802450DC(s32 arg0) {
    s32 id = func_80243DDC();

    func_80244354(id, 0x20, 1);
    func_8023903C(arg0);
    func_80244354(id, 0x20, 0);
}

s32 func_80244FF8(UnkStructA *arg0) {
    s32 ret = 0;
    s32 id;
    s32 flag;

    func_802313DC(arg0, 0x400);
    id = func_802441AC(arg0);
    func_80244354(id, 0x400000, 0);
    flag = func_80244BA4(id, 0x1000);
    if (flag) {
        flag = func_80243EB4(id, 0x2000);
        if (!flag) {
            arg0->unk6C = 1.0f;
            ret = 1;
            func_80244354(id, 0x200000, 1);
            func_80270BA4(arg0);
            func_802313DC(arg0, 0x20000);
        }
    }
    return ret;
}

s32 func_80244F34(void *arg0, PORT_IP(s32) arg1) {
    s32 id = func_802441AC(arg0);
    s32 flag;

    D_800AEDF0[id].unk40 = arg1;
    func_80231408(arg0, 0x400);
    func_80244354(id, 0x200000, 0);
    func_80244354(id, 0x400000, 1);
    flag = func_80244BA4(id, 0x80);
    if (flag) {
        func_80244C30(id, 1);
    }
    return 1;
}

void func_80244E04(void *arg0) {
    s32 id;
    s32 flag;

    func_802313DC(arg0, 0x400);
    id = func_802441AC(arg0);
    func_80244354(id, 0x400000, 0);
    flag = func_80244BA4(id, 0x1000);
    if (!flag) {
        flag = func_80244BA4(id, 0x80);
        if (flag) {
            func_80244C30(id, 1);
        }
        func_80244354(id, 0x20, 0);
        flag = func_80244BA4(id, 0x8000);
        if (!flag) {
            func_802444B8(id, 0);
        } else {
            func_802444B8(id, 0xC);
        }
    } else {
        func_802444B8(id, 8);
    }
    D_800AEDF0[id].unk40 = 0;
}

void func_80244C30(s32 arg0, s32 arg1) {
    UnkStructA *obj;
    s32 t1;
    f32 vec[3];
    UnkStruct_800AEDF0 *p;

    p = &D_800AEDF0[arg0];
    if (p->unk2C != 0) {
        obj = func_802443F4(arg0);
        func_80244BE0(arg0, arg1);
        p->unk2C = 0;
        func_80270BA4(obj);
        t1 = func_80244BA4(arg0, 0x400000);
        if (t1) {
            func_802444B8(arg0, 0x13);
        }
        func_80244354(arg0, 0xA040, 0);
        if (!func_80244BA4(arg0, 0x400000)) {
            func_80244354(arg0, 0x20, 0);
        }
        if (func_80243EB4(arg0, 0x2000)) {
            UnkStructA *o2 = func_802443F4(p->unkA4);
            vec[2] = 70.0f;
            vec[1] = 60.0f;
            vec[0] = 180.0f;
            func_8022FEAC(vec, &o2->unk4C);
        }
        func_80244354(arg0, 0x100, 0);
    }
    if (func_80244BA4(arg0, 0x80)) {
        func_802444B8(arg0, 0);
    }
    func_80244354(arg0, 0x80, 0);
}

void func_80244BE0(s32 arg0, s16 arg1) {
    func_80270770((s16)arg0, arg1);
    D_800AEDF0[arg0].unk2C = 0;
}

s32 func_80244BA4(s32 arg0, s32 arg1) {
    s32 ret = 0;

    if (arg1 == (D_800AEDF0[arg0].unk10 & arg1)) {
        ret = 1;
    }
    return ret;
}

void func_802444B8(s32 arg0, s32 arg1) {
    s32 anim1;
    s32 anim2;
    s32 mode;
    UnkStructA *obj;

    D_800AEDF0[arg0].unk44 = arg1;
    obj = func_802443F4(arg0);
    switch (arg1) {
    case 0:
        anim1 = 0;
        anim2 = -1;
        mode = 3;
        break;
    case 1:
        anim1 = 1;
        anim2 = 2;
        mode = 5;
        break;
    case 2:
        anim1 = 3;
        anim2 = 4;
        mode = 5;
        break;
    case 3:
        anim1 = 5;
        anim2 = 6;
        mode = 5;
        break;
    case 4:
        anim1 = 7;
        anim2 = -1;
        mode = 0;
        break;
    case 5:
        anim1 = -1;
        anim2 = -1;
        break;
    case 6:
        anim1 = 0xB;
        anim2 = -1;
        mode = 0;
        break;
    case 7:
        anim1 = 0xA;
        anim2 = -1;
        mode = 0;
        break;
    case 8:
        anim1 = 0x11;
        anim2 = -1;
        mode = 3;
        break;
    case 10:
        anim1 = 0xC;
        anim2 = -1;
        mode = 0;
        break;
    case 9:
        anim1 = 0xD;
        anim2 = -1;
        mode = 0;
        break;
    case 11:
        anim1 = 0x10;
        anim2 = -1;
        mode = 0;
        break;
    case 12:
        anim1 = 9;
        anim2 = -1;
        mode = 4;
        break;
    case 13:
        anim1 = 8;
        anim2 = 2;
        mode = 6;
        break;
    case 14:
        anim1 = 8;
        anim2 = 4;
        mode = 6;
        break;
    case 15:
        anim1 = 8;
        anim2 = 6;
        mode = 6;
        break;
    case 16:
        anim1 = 0xF;
        anim2 = -1;
        mode = 0;
        break;
    case 18:
        anim1 = 8;
        anim2 = -1;
        mode = 0;
        break;
    case 17:
        anim1 = 0xE;
        anim2 = -1;
        mode = 0;
        break;
    case 19:
        anim1 = 0x12;
        anim2 = -1;
        mode = 0;
        break;
    case 20:
        anim1 = 8;
        anim2 = 0x15;
        mode = 2;
        break;
    case 21:
        anim1 = 0x16;
        anim2 = -1;
        mode = 2;
        break;
    case 23:
        anim1 = 0x18;
        anim2 = -1;
        mode = 0;
        break;
    case 24:
        anim1 = 0x19;
        anim2 = -1;
        mode = 0;
        break;
    default:
        anim1 = -1;
        anim2 = -1;
        mode = 0;
        break;
    }
    func_80229E38(obj->unk40, anim1, 0);
    func_80229E38(obj->unk40, anim2, 1);
    obj->unk40->unkC |= 0x4000;
    switch (mode) {
    case 0:
        obj->unk40->unk48->unk4[0].unk8 |= 0x20;
        obj->unk40->unk48->unk4[0].unk8 &= ~0x40;
        obj->unk40->unk48->unk4[1].unk8 &= ~0x20;
        obj->unk40->unk48->unk4[1].unk8 &= ~0x40;
        break;
    case 2:
        obj->unk40->unk48->unk4[0].unk8 |= 0x20;
        obj->unk40->unk48->unk4[0].unk8 &= ~0x40;
        obj->unk40->unk48->unk4[1].unk8 &= ~0x20;
        obj->unk40->unk48->unk4[1].unk8 &= ~0x40;
        break;
    case 3:
        obj->unk40->unk48->unk4[0].unk8 &= ~0x20;
        obj->unk40->unk48->unk4[0].unk8 &= ~0x40;
        obj->unk40->unk48->unk4[1].unk8 &= ~0x20;
        obj->unk40->unk48->unk4[1].unk8 &= ~0x40;
        break;
    case 4:
        obj->unk40->unk48->unk4[0].unk8 |= 0x20;
        obj->unk40->unk48->unk4[0].unk8 |= 0x40;
        obj->unk40->unk48->unk4[1].unk8 |= 0x20;
        obj->unk40->unk48->unk4[1].unk8 |= 0x40;
        break;
    case 5:
        obj->unk40->unk48->unk4[0].unk8 &= ~0x20;
        obj->unk40->unk48->unk4[0].unk8 &= ~0x40;
        obj->unk40->unk48->unk4[0].unk4 = func_8024441C(arg0, arg1);
        obj->unk40->unk48->unk4[1].unk8 &= ~0x20;
        obj->unk40->unk48->unk4[1].unk8 &= ~0x40;
        obj->unk40->unk48->unk4[1].unk4 = obj->unk40->unk48->unk4[0].unk4;
        break;
    case 6:
        obj->unk40->unk48->unk4[0].unk8 |= 0x20;
        obj->unk40->unk48->unk4[0].unk8 |= 0x40;
        obj->unk40->unk48->unk4[1].unk8 &= ~0x20;
        obj->unk40->unk48->unk4[1].unk8 &= ~0x40;
        obj->unk40->unk48->unk4[1].unk4 = func_8024441C(arg0, arg1);
        break;
    }
}

f32 func_8024441C(s32 arg0, s32 arg1) {
    f32 ret = 0.0f;
    s32 v;

    if (arg1 == D_802AC9F8[arg0]) {
        PORT_IP(s32) obj = (PORT_IP(s32)) func_802443F4(arg0); /* unused (its inlined load is the original's) */
        
        switch (func_802443CC(arg0)) {
            case 1:
            case 2:
            case 3:
                v = 0;
                break;
            case 13:
            case 14:
            case 15:
                v = 1;
                break;
            default:
                v = -1;
                break;
        }
        if (v != -1) {
            ret = D_802ACA78[arg0];
        }
    }
    return ret;
}

UnkStructA *func_802443F4(s32 arg0) {
    PORT_IP(s32) ret; /* fake: an integer (an UnkStructA * puts func_80245868's inlined copies in a1, not a0) */

#ifdef TARGET_PC
    /* Entry -1 (D_802AC894 with no entry current: ovl_09_01's room script
     * calls func_802512C0 from the handler, 20:16): on the N64 its unk28
     * is the word 0x800AED50, in the bytes after D_800A7F30[100] that
     * nothing names, 0 (the emulator's and i386's). */
    if (arg0 == -1) {
        return NULL;
    }
#endif
    ret = (PORT_IP(s32)) D_800AEDF0[arg0].unk28;
    return (UnkStructA *) ret;
}

s32 func_802443CC(s32 arg0) {
    return D_800AEDF0[arg0].unk44;
}

void func_802443A0(s32 arg0) {
    s32 id;

    id = func_80243DDC();
    func_80244354(id, 0x01000000, arg0);
}

void func_80244354(s32 arg0, s32 arg1, s32 arg2) {
    s32 v = D_800AEDF0[arg0].unk10;

    if (arg2 == 1) {
        v |= arg1;
    } else {
        v &= arg1 ^ -1;
    }
    D_800AEDF0[arg0].unk10 = v;
    return;
}

void func_80244308(void *arg0) {
    func_8022F4AC(arg0, 0x10, PORT_PTR(D_800AEDF0[func_802441AC(arg0)].unk4C));
}

s32 func_802442F8() {
    return D_802A01B0;
}

s32 func_802442D0(s32 arg0) {
    return D_800AEDF0[arg0].unk20;
}

PORT_IP(s32) func_802442A8(s32 arg0) {
    return D_800AEDF0[arg0].unk4C;
}

void func_802441F4(void *arg0, s32 arg1, s32 *arg2, f32 *arg3) {
    f32 x;
    f32 pos[3];
    f32 rot[3];
    f32 v[3];

    *arg2 = arg1;
    *arg3 = 0.0f;
    func_80239880(func_802441AC(arg0), 0, pos);
    func_80234858(&v[0], &v[1], &v[2]);
    if (pos[2] >= 20.0f) {
        x = v[0] + pos[0];
        if (x >= 360.0f) {
            x -= 360.0f;
        }
        *arg2 = func_80243F50(x);
        *arg3 = 8.5f;
    }
}

s32 func_802441AC(void *arg0) {
    s32 ret = -1;
    s32 i = 0;

    do {
        if (arg0 == D_800AEDF0[i].unk28) {
            ret = i;
        }
        i++;
    } while (i < 5 && ret == -1);
    return ret;
}

s32 func_80243F50(f32 arg0) {
    s32 ret;

    if ((arg0 >= 0.0 && arg0 < 22.5) || (arg0 >= 337.5 && arg0 < 360.0)) {
        ret = 0x80;
    }
    if (arg0 >= 22.5 && arg0 < 67.5) {
        ret = 0x60;
    }
    if (arg0 >= 67.5 && arg0 < 112.5) {
        ret = 0x40;
    }
    if (arg0 >= 112.5 && arg0 < 157.5) {
        ret = 0x20;
    }
    if (arg0 >= 157.5 && arg0 < 202.5) {
        ret = 0;
    }
    if (arg0 >= 202.5 && arg0 < 247.5) {
        ret = 0xE0;
    }
    if (arg0 >= 247.5 && arg0 < 292.5) {
        ret = 0xC0;
    }
    if (arg0 >= 292.5 && arg0 < 337.5) {
        ret = 0xA0;
    }
    if (arg0 == -1.0f) {
        ret = 1;
    }
    return ret;
}

void func_80243F2C(s32 arg0, s32 arg1) {
    D_800AEDF0[arg0].unk24 = arg1;
}

s32 func_80243EF0(s32 arg0) {
    s32 ret = 0;

    if ((D_800AEDF0[arg0].unk68 & 0x200) == 0x200) {
        ret = 1;
    }
    return ret;
}

s32 func_80243EB4(s32 arg0, s32 arg1) {
    s32 ret = 0;

    if (arg1 == (D_800AEDF0[arg0].unk68 & arg1)) {
        ret = 1;
    }
    return ret;
}

void func_80243E90(s32 arg0) {
    func_80243D90(arg0, 0x200, 1);
}

void func_80243E60() {
    func_8027750C((s16)func_80243DDC());
}

s32 func_80243E3C() {
    s32 ret = 0;

    if ((D_802AC9E8 & 2) == 2) {
        ret = 1;
    }
    return ret;
}

s32 func_80243E18(s32 arg0) {
    s32 ret = 0;

    if (arg0 == (D_802AC9E8 & arg0)) {
        ret = 1;
    }
    return ret;
}

void func_80243DEC(s32 arg0) {
    s32 id;

    id = func_80243DDC();
    func_80243D90(id, 0x8000, arg0);
}

s32 func_80243DDC() {
    s32 ret;

    ret = D_802AC894;
    return ret;
}

void func_80243D90(s32 arg0, s32 arg1, s32 arg2) {
#ifdef TARGET_PC
    if (arg0 == -1) {
        /* (the N64's word is the pointer's low half: the flag's bit there) */
        uintptr_t w = (uintptr_t)D_800AED78[2].unk0;

        if (arg2 == 1) {
            w |= (u32)arg1;
        } else {
            w &= ~(uintptr_t)(u32)arg1;
        }
        D_800AED78[2].unk0 = (void *)w;
        return;
    }
#endif
    s32 v = D_800AEDF0[arg0].unk68;

    if (arg2 == 1) {
        v |= arg1;
    } else {
        v &= arg1 ^ -1;
    }
    D_800AEDF0[arg0].unk68 = v;
}

UnkStruct_802ACF08 *func_80243D7C(s32 arg0) {
    return &D_802ACF08[arg0];
}

s32 *func_80243D54(s32 arg0) {
    return &D_800AEDF0[arg0].unk10;
}

s32 func_80243D2C(s32 arg0) {
#ifdef TARGET_PC
    if (arg0 == -1) {
        return 0; /* (the N64's word 0x800AED3C) */
    }
#endif
    return D_800AEDF0[arg0].unk14;
}

void func_80243D20(s32 arg0) {
    D_802AC9F4 = arg0;
}

void func_80243BF0() {
    s32 i;

    func_80297D30(0x10, D_802A0E58);
    for (i = 0; i < 5; i++) {
        D_800AEDF0[i].unk28 = 0;
        D_800AEDF0[i].unk18 = 0x200;
        D_800AEDF0[i].unk24 = 0;
        D_800AEDF0[i].unk0 = D_800AEDF0[i].unk4 = D_800AEDF0[i].unk8 = 0.0f;
        D_800AEDF0[i].unk10 = 0;
        D_800AEDF0[i].unkC = 0.0f;
    }
    D_800AEDF0[0].unk24 = 1;
    D_802AC894 = -1;
    D_802AC9F4 = 0;
}
