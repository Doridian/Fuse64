/*
 * ovl_0C_06's game object, IDO -O3; it matches in us, eu and jp.
 *
 * What it took:
 * - The blocks' and switches' flag (80043248, 80044340, 80044494): the
 *   original keeps `arg ? 0x01000000 : 0x01000000` unfolded.  A union of
 *   s32 and u32 does it: stores through members of different types aren't
 *   merged.  80044340 and 80043248 set `fl.u` before the loop and
 *   `if (!arg1) fl.i = ...` in it (80043248's loop is an `if` around a
 *   do-while, so the store lands after the guard); 80044494 needs the two
 *   constants of the same type (one register, v1), hence the fake
 *   `| (s32)obj * 0` on the u32 store.  The loops walk their parameter.
 * - Stubs: empty statics 80043818, 8004390C and 80044C80, each inlined
 *   first in the function after it (as in ovl_0C_07); 8004508C's call is
 *   in 80045094.
 * - 800455DC calls 80046200(1, ...) (inlined: the frame's 16 bytes),
 *   800458D8's `vol = 0x7F;` shares the loop's line, 80045BA0 has 8 bytes
 *   of unused locals, 800446FC reads the state twice, 80044F44's loop runs
 *   once, 80043820 has its three statements on their own lines.
 * - 8004510C's last call is in statics.h (a fake `#line`, to the end of
 *   the file): its `.file` keeps the store out of the jal's delay slot.
 * - func_ovl0C_06_80048750 returns s32 (as ovl_0C_07's), which gives
 *   80045FC8 its v1 and 80046288 its registers.
 */
#include "common.h"
#include "overlays/common/types.h"

/*
 * ovl_0C_06's game object, IDO -O3: the functions are written last to first
 * (IDO -O3 emits a file's functions in reverse).  ovl_0C_07's skeleton for
 * the gate, the movers, the lift and the stage object; the crumbling blocks
 * and their switches are ovl_0C_06's own (as in ovl_0C_01 and ovl_0C_02).
 */

/* an object, its position and the limits the movers check it against */
typedef struct {
    /* 0x00 */ UnkStructA *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
} UnkStruct_ovl0C_06_80048FD0;

/* a mover: the tasks func_ovl0C_07_80044CE0 and func_ovl0C_07_80044DC8
 * take one (unk0 indexes D_ovl0C_06_80048FD0) */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkStructA *unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ f32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ f32 unk30;
    /* 0x34 */ f32 unk34;
    /* 0x38 */ f32 unk38;
    /* 0x3C */ f32 unk3C;
    /* 0x40 */ f32 unk40;
} UnkStruct_ovl0C_06_80049030;

/* a crumbling block (func_ovl0C_06_8004696C's task); the list ends at the
 * first unk2 of 0 */
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ UnkStructA *unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ UnkStruct_ovl0C_01_8004A22C *unk20;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ s16 unk26;
} UnkStruct_ovl0C_06_8004914C;

void func_ovl0C_06_80047BB8(s32);
void func_ovl0C_06_80047BD0(s32);
void func_ovl0C_06_80047BF0(s32, s32, s32, s32);
void func_ovl0C_06_80047BF8(s32, s32, s32, s32, s32, s32);
void func_ovl0C_06_80047C00(s32, s32, s32, s32, s32, s32, s32);
void func_ovl0C_06_80047C40(UnkStructA_40 *, f32 *, f32 *);
void func_ovl0C_06_80047CA0(void (*)(UnkStructA *));
void func_ovl0C_06_80047CB0(s32, s32, s32);
void func_ovl0C_06_80047CF0(void (*)(UnkStructA *));
s32 *func_ovl0C_06_80047D08(void);
void func_ovl0C_06_80047D20(s32);
void func_ovl0C_06_80047D28(s32);
void func_ovl0C_06_80047D40(s32, s32);
void func_ovl0C_06_80047D58(s32);
void func_ovl0C_06_80047D68(void);
void func_ovl0C_06_80047D70(s32, s32, s32);
void func_ovl0C_06_80047D78(void *);
s32 func_ovl0C_06_80047D80(s32, s32, s32, s32);
s32 func_ovl0C_06_80047D88(s32, s32, s32, s32, s32);
s32 func_ovl0C_06_80047D98(void);
void func_ovl0C_06_80047DA0(s32, void *);
void func_ovl0C_06_80047DA8(s32);
void func_ovl0C_06_80047DB0(s32);
void func_ovl0C_06_80047DB8(s32, s32, f32, s32, s32, s32, s32);
void func_ovl0C_06_80047DC8(void);
void func_ovl0C_06_80047DE0(s32);
UnkStructA *func_ovl0C_06_80047DF8(s32);
void func_ovl0C_06_80047E00(s32);
void func_ovl0C_06_80047E68(void);
void func_ovl0C_06_800482C0(UnkStructA_40 *, s32);
void func_ovl0C_06_800482C8(UnkStructA_40 *, s32, s32, s32);
f32 func_ovl0C_06_800482F8(UnkStructA_40 *, s32);
s32 func_ovl0C_06_80048300(UnkStructA_40 *, s32, s32, s32, s32, s32);
void func_ovl0C_06_80048320(s32, s32);
void func_ovl0C_06_800483F8(f32, f32, f32);
void func_ovl0C_06_80048418(f32, f32, f32);
void func_ovl0C_06_80048420(f32 *, f32 *, f32 *);
void func_ovl0C_06_80048458(s32);
void func_ovl0C_06_80048460(s32);
void func_ovl0C_06_800484B0(f32, f32);
void func_ovl0C_06_800484E8(void *);
void func_ovl0C_06_80048518(void);
void func_ovl0C_06_80048520(void);
s32 func_ovl0C_06_80048568(UnkStructA *, s32);
void func_ovl0C_06_80048590(UnkStructA *);
void func_ovl0C_06_80048598(UnkStructA *, UnkStructA *);
void func_ovl0C_06_800485B8(UnkStructA *, s32);
void func_ovl0C_06_800485C0(UnkStructA *, s32);
void func_ovl0C_06_800485D0(UnkStructA *);
UnkStructA *func_ovl0C_06_800485E0(s32, s32, s32, PORT_IP(s32));
UnkStructA *func_ovl0C_06_800485F0(UnkStructA *);
UnkStructA *func_ovl0C_06_80048600(s32, s32, f32, f32, f32);
void func_ovl0C_06_80048638(UnkStructA *);
UnkStructA *func_ovl0C_06_80048648(UnkStructA *, s32);
void func_ovl0C_06_800486C8(UnkStructA *);
s32 func_ovl0C_06_80048708(void (*)(void), void *, PORT_IP(s32), s32, s32);
void func_ovl0C_06_80048710(s32);
void func_ovl0C_06_80048718(void);
void func_ovl0C_06_80048728(s32);
void *func_ovl0C_06_80048730(void);
s32 func_ovl0C_06_80048750(void);
void func_ovl0C_06_80048768(u16);
void func_ovl0C_06_80048780(s32, s32);
f32 func_ovl0C_06_800488B0(f32);
f32 func_ovl0C_06_800488B8(f32);
s32 func_ovl0C_06_800488C8(s32);
s32 func_ovl0C_06_80048970(s32);
void func_ovl0C_06_80048980(s32);
void func_ovl0C_06_80048988(s32);
s32 func_ovl0C_06_80048990(s32);
void func_ovl0C_06_80048A70(u32 (*)[3]);
void func_ovl0C_06_80048A88(u32 (*)[3]);
void func_ovl0C_06_80048B58(s32, f32, f32, f32, PORT_IP(s32));
void func_ovl0C_06_80048BA8(f32 *);
void func_ovl0C_06_80048C18(s32, s32, s32);
void func_ovl0C_06_80048C78(void);
s32 func_ovl0C_06_80048C90(s32);
s32 func_ovl0C_06_80048CB0(s32, f32, f32, f32);
void func_ovl0C_06_80048CC0(s32);
void func_ovl0C_06_80048CC8(s32, s8);
void func_ovl0C_06_80048E08(void);

void func_ovl0C_06_80043010(void);
void func_ovl0C_06_80043018(void);
void func_ovl0C_06_80043020(void);
void func_ovl0C_06_80043040(void);
void func_ovl0C_06_80043060(void);
s32 func_ovl0C_06_800430A8(UnkStructA *arg0, UnkStructA *arg1);
void func_ovl0C_06_800431B8(void);
void func_ovl0C_06_80043248(UnkStruct_ovl0C_06_8004914C *arg0, s32 arg1);
void func_ovl0C_06_80043354(UnkStruct_ovl0C_01_8004A3BC *arg0, UnkStruct_ovl0C_06_8004914C *arg1, UnkStruct_ovl0C_01_8004A22C *arg2, s32 arg3);
s32 func_ovl0C_06_80043544(UnkStruct_ovl0C_01_8004A3BC *arg0, UnkStruct_ovl0C_06_8004914C *arg1, UnkStruct_ovl0C_01_8004A22C *list, s32 arg3);
void func_ovl0C_06_80043710(UnkStruct_ovl0C_01_8004A104 *arg0);
void func_ovl0C_06_8004379C(UnkStruct_ovl0C_01_8004A104 *arg0);
void func_ovl0C_06_80043820(void);
static void func_ovl0C_06_80043818(void);
void func_ovl0C_06_80043868(void);
void func_ovl0C_06_80043914(void);
static void func_ovl0C_06_8004390C(void);
void func_ovl0C_06_8004394C(UnkStructA *arg0, UnkStructA *arg1, f32 arg2);
void func_ovl0C_06_8004399C(UnkStruct_ovl0C_06_80049030 *arg0);
void func_ovl0C_06_80043A5C(void);
void func_ovl0C_06_80043AB4(void);
void func_ovl0C_06_80043AEC(void);
void func_ovl0C_06_80043B3C(void);
void func_ovl0C_06_80043D54(UnkStruct_ovl0C_01_8004A104 *arg0);
void func_ovl0C_06_80043DC0(void);
void func_ovl0C_06_80043EC8(void);
void func_ovl0C_06_80043F34(void);
void func_ovl0C_06_80043F6C(void);
void func_ovl0C_06_80043FE0(void);
void func_ovl0C_06_80044028(void);
void func_ovl0C_06_800440B4(void);
void func_ovl0C_06_80044164(UnkStruct_ovl0C_01_8004A104 *arg0);
void func_ovl0C_06_80044228(UnkStruct_ovl0C_01_8004A104 *arg0);
void func_ovl0C_06_80044264(void);
void func_ovl0C_06_800442BC(UnkStruct_ovl0C_06_80048FD0 *arg0);
void func_ovl0C_06_800442BC(UnkStruct_ovl0C_06_80048FD0 *arg0);
void func_ovl0C_06_80044340(UnkStruct_ovl0C_01_8004A22C *arg0, s32 arg1);
void func_ovl0C_06_80044340(UnkStruct_ovl0C_01_8004A22C *arg0, s32 arg1);
void func_ovl0C_06_80044494(UnkStruct_ovl0C_01_8004A3BC *arg0, UnkStruct_ovl0C_06_8004914C *arg1, s32 arg2);
void func_ovl0C_06_80044584(f32);
void func_ovl0C_06_800445CC(void);
void func_ovl0C_06_800446FC(void);
void func_ovl0C_06_80044808(s32 arg0, s32 arg1, s32 arg2, f32 arg3, s32 arg4);
void func_ovl0C_06_80044830(void);
void func_ovl0C_06_80044884(void);
void func_ovl0C_06_800448D4(void);
void func_ovl0C_06_8004491C(void);
void func_ovl0C_06_80044950(void);
void func_ovl0C_06_800449C8(void);
void func_ovl0C_06_800449F0(void);
void func_ovl0C_06_80044A4C(void);
void func_ovl0C_06_80044AB0(void);
void func_ovl0C_06_80044B10(void);
void func_ovl0C_06_80044B60(void);
void func_ovl0C_06_80044BC8(void);
void func_ovl0C_06_80044C0C(void);
void func_ovl0C_06_80044C88(UnkStruct_ovl0C_01_8004A104 *arg0);
static void func_ovl0C_06_80044C80(void);
void func_ovl0C_06_80044CE4(s32 arg0);
void func_ovl0C_06_80044DA0(void);
void func_ovl0C_06_80044F44(void);
static void func_ovl0C_06_8004508C(void);
void func_ovl0C_06_80045094(void);
void func_ovl0C_06_800450D4(void);
void func_ovl0C_06_8004510C(void);
void func_ovl0C_06_80045364(s32 arg0);
void func_ovl0C_06_80045524(void);
void func_ovl0C_06_800455DC(void);
void func_ovl0C_06_80045874(UnkStructA *arg0);
void func_ovl0C_06_800458A0(void *arg0);
void func_ovl0C_06_800458D8(void);
void func_ovl0C_06_80045BA0(void);
void func_ovl0C_06_80045F44(void);
void func_ovl0C_06_80045FC8(void);
void func_ovl0C_06_80046160(void *arg0);
void func_ovl0C_06_80046198(void);
void func_ovl0C_06_80046200(s32 arg0, UnkStructA *arg1, UnkStructA *arg2, f32 arg3);
void func_ovl0C_06_80046288(void);
void func_ovl0C_06_8004696C(void);
void func_ovl0C_06_80046BD8(void);
void func_ovl0C_06_80046CF0(void);
void func_ovl0C_06_80046E14(UnkStruct_ovl0C_06_80048F88 *arg0, UnkStructA *arg1, UnkStructA *arg2);
void func_ovl0C_06_80046EB0(UnkStruct_ovl0C_06_80048F88 *arg0, UnkStructA *arg1);
void func_ovl0C_06_80046FFC(void);
void func_ovl0C_06_80047490(UnkStructA *arg0);
void func_ovl0C_06_80047550(UnkStructA *arg0);
void func_ovl0C_06_80047594(UnkStructA *arg0, s32 arg1, s32 arg2, f32 arg3, f32 arg4);
void func_ovl0C_06_8004766C(void);
s32 func_ovl0C_06_8004777C(s32, void *);
void func_ovl0C_06_800477B0(f32, f32, f32);
void func_ovl0C_06_8004780C(void);
static void func_ovl0C_06_80047824(void);

/* The three statements are on one line, a macro: only then does IDO
 * schedule across them (as in ovl_0C_04, ovl_0C_07 and ovl_0C_09). */
#define UNK_MACRO_ovl0C_06_80043820() \
    D_ovl0C_06_8004938C = func_ovl0C_06_80047D98(); func_ovl0C_06_80047D78(&D_ovl0C_06_80049380); func_ovl0C_06_80047DA0(D_ovl0C_06_8004938C, D_ovl0C_06_80049370)

/* func_ovl0C_06_80046200's body, written out where the mover drops its
 * object: the mover's frame has no room for its parameters (as in
 * ovl_0C_09) */
#define UNK_MACRO_ovl0C_06_80046200(flag, a, b, f)                             \
    fl = (flag);                                                               \
    if (fl & 1) {                                                              \
        func_ovl0C_06_80048C90(0x4C);                                          \
        func_ovl0C_06_80048598(a, b);                                          \
        func_ovl0C_06_800485B8(b, 0x200000);                                   \
        (b)->unk4C = 0.0f;                                                     \
        (b)->unk50 = f;                                                        \
        (b)->unk54 = 0.0f;                                                     \
    } else {                                                                   \
        func_ovl0C_06_80048C90(0x4C);                                          \
        func_ovl0C_06_80048590(b);                                             \
        func_ovl0C_06_800485C0(b, 0x200000);                                   \
    }

/* .bss */
u8 D_ovl0C_06_80049370[0x10];
Vec3f D_ovl0C_06_80049380;
s32 D_ovl0C_06_8004938C;
u8 D_ovl0C_06_80049390[0x4];
s32 D_ovl0C_06_80049394;
s32 D_ovl0C_06_80049398;
Vec3f D_ovl0C_06_800493A0;
UnkStructA *D_ovl0C_06_800493AC;
UnkStruct_ovl0C_01_8004A9C8 D_ovl0C_06_800493B0;
UnkStructA *D_ovl0C_06_800493C4;
UnkStructA *D_ovl0C_06_800493C8[2];

/* .data */
/* passed to func_ovl0C_06_80048A70: three 12-byte records */
u32 D_ovl0C_06_80048EB0[][3] = {
    { 0x910002, 0x2000E, 4 },
    { 0x940002, 0xE, 4 },
    { 0x950000, 0xE, 4 },
};
/* passed to func_ovl0C_06_80048A88 */
u32 D_ovl0C_06_80048ED4[][3] = {
    { 0, 4, 0 },
    { 4, 0x960000, 0x84 },
};
/* three 12-byte records, each passed to func_ovl0C_06_80048C18 */
u32 D_ovl0C_06_80048EEC[][3] = {
    { 0, 0, 0xDE },
    { 1, 0x22, 0xE0 },
    { 2, 5, 0x34F },
};
UnkStruct_ovl0C_01_8004A104 D_ovl0C_06_80048F10[] = {
    { 0, NULL, 1550.0f, 0.0f, 200.0f, 0x80, 1550.0f, 0.0f, 250.0f, 0xF, 0, 1, 0x10, 1, 4, 0xF, 0, 1, 0x10, 1,
      0xE, 0, 3, 0x1C, 0x11, 0x11, 0, 3, 0x17, 0x11 },
};
UnkStruct_ovl0C_06_80048F88 D_ovl0C_06_80048F88[] = {
    { 0, 0, 0, NULL, NULL, 3150.0f, 100.0f, 850.0f, 100.0f, 700.0f, 3150.0f, 0.0f, 850.0f, 43.5f,
      0x1F, 1, 8, 0x20, 4, 9, 0x40, 0, 0 },
};
UnkStruct_ovl0C_06_80048FD0 D_ovl0C_06_80048FD0[] = {
    { NULL, 1550.0f, 0.0f, 1550.0f, 0.0f, -400.0f, 1550.0f, 1550.0f },
    { NULL, 4350.0f, 1000.0f, 250.0f, 1000.0f, -400.0f, 4350.0f, 250.0f },
    { NULL, 4350.0f, 1000.0f, 250.0f, 1000.0f, -400.0f, 4350.0f, 250.0f },
};
UnkStruct_ovl0C_06_80049030 D_ovl0C_06_80049030[] = {
    { 0, NULL, 0x2E, 0x13, 12.0f, 0.0f, 35.0f, 1.0f, 1, 0x43A,
      550.0f, 1000.0f, 1550.0f, 1050.0f, 1550.0f, 2550.0f, 1550.0f },
    { 1, NULL, 0x2E, 0x14, 0.0f, -12.0f, 35.0f, 1.0f, 2, 0x437,
      2650.0f, 1500.0f, 1950.0f, 4350.0f, 550.0f, 435.0f, -300.0f },
    { 2, NULL, 0x30, 0x13, 0.0f, -12.0f, 35.0f, 1.0f, 1, 0x436,
      4350.0f, 1800.0f, 250.0f, 4350.0f, 650.0f, 435.0f, -300.0f },
};
UnkStruct_ovl0C_01_8004A22C D_ovl0C_06_800490FC[] = {
    { 0, 1, NULL, 4350.0f, 200.0f, 1450.0f, 450.0f, 0x2B, 3, 0xE, 1, 0, 0, 0x43C },
    { 1, 0, NULL, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0, 0 },
};
UnkStruct_ovl0C_06_8004914C D_ovl0C_06_8004914C[] = {
    { 0, 1, NULL, 4350.0f, 700.0f, 1050.0f, 700.0f, 0x2A, 4, 8, 3, NULL, 1, 0 },
    { 1, 2, NULL, 4350.0f, 700.0f, 1350.0f, 700.0f, 0x2A, 4, 0xB, 3, NULL, 1, 0 },
    { 2, 3, NULL, 4350.0f, 700.0f, 1650.0f, 700.0f, 0x2A, 4, 0xE, 3, D_ovl0C_06_800490FC, 1, 0 },
    { 3, 4, NULL, 4350.0f, 700.0f, 1950.0f, 700.0f, 0x2A, 4, 0x11, 3, NULL, 1, 0 },
    { 4, 5, NULL, 4350.0f, 700.0f, 2250.0f, 700.0f, 0x2A, 4, 0x14, 3, NULL, 1, 0 },
    { 5, 6, NULL, 4350.0f, 700.0f, 2549.9f, 700.0f, 0x2A, 4, 0x17, 3, NULL, 1, 0 },
    { 6, 0, NULL, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, NULL, 1, 0 },
};
UnkStruct_ovl0C_01_8004A3BC D_ovl0C_06_80049264 = { NULL, 4350.0f, 700.0f, 1050.0f };
UnkStruct_ovl0C_01_8004A54C D_ovl0C_06_80049274 = {
    NULL, -1, 0x2B8, -5, -0x400, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
};
UnkStruct_ovl0C_01_8004A3BC D_ovl0C_06_800492AC = { NULL, 300.0f, 200.0f, 1600.0f };

/* unused: the stubs IDO -O3 leaves at the end of the object */
static void func_ovl0C_06_80047864(void) {
}
static void func_ovl0C_06_8004785C(void) {
}
static void func_ovl0C_06_80047854(void) {
}
static void func_ovl0C_06_8004784C(void) {
}
static void func_ovl0C_06_80047844(void) {
}
static void func_ovl0C_06_8004783C(void) {
}
static void func_ovl0C_06_80047834(void) {
}
static void func_ovl0C_06_8004782C(void) {
}

/* unused (its body is dropped): the reference keeps the unused words in
 * .bss, which would otherwise be common */
static void func_ovl0C_06_80047824(void) {
    D_ovl0C_06_80049390[0] = 0;
}

/* the names src/overlays/common/core.h's code has here */
#define CORE_FUNC_A func_ovl0C_06_8004780C
#define CORE_FUNC_B func_ovl0C_06_800477B0
#define CORE_FUNC_C func_ovl0C_06_80044584
#define CORE_FUNC_D func_ovl0C_06_8004777C
#define CORE_BSS_A0 D_ovl0C_06_80049394
#define CORE_BSS_A1 D_ovl0C_06_80049398
#define CORE_BSS_B D_ovl0C_06_800493A0
#define CORE_ZJ_1704 func_ovl0C_06_800483F8
#define CORE_ZJ_1708 func_ovl0C_06_80048418
#define CORE_ZJ_1709 func_ovl0C_06_80048420
#define CORE_ZJ_1904 func_ovl0C_06_80048728
#define CORE_ZJ_2B04 func_ovl0C_06_80048CB0
#include "overlays/common/core.h"




/* the names src/overlays/common/core_0c.h's code has here */
#define CORE0C_80048694 func_ovl0C_06_80047490
#define CORE0C_80048754 func_ovl0C_06_80047550
#define CORE0C_80048798 func_ovl0C_06_80047594
#define CORE0C_80048870 func_ovl0C_06_8004766C
#define CORE0C_80048E40 func_ovl0C_06_80047C40
#define CORE0C_800497C0 func_ovl0C_06_800485C0
#define CORE0C_800497D0 func_ovl0C_06_800485D0
#define CORE0C_800497E0 func_ovl0C_06_800485E0
#define CORE0C_80049908 func_ovl0C_06_80048708
#define CORE0C_80049918 func_ovl0C_06_80048718

#define CORE0C_80049930 func_ovl0C_06_80048730
#define CORE0C_80049950 func_ovl0C_06_80048750
#define CORE0C_80049AB0 func_ovl0C_06_800488B0

#include "overlays/common/core_0c.h"

/* a lift: rises while something stands on it, falls back otherwise */
void func_ovl0C_06_80046FFC(void) {
    UnkStruct_ovl0C_06_80048F88 *rec;
    UnkStructA *obj;
    UnkStructA *player;
    s16 wait;

    rec = func_ovl0C_06_80048730();
    obj = rec->unk8;
    player = func_ovl0C_06_80047DF8(0);
    obj->unk68 = 0.0f;
    wait = 2;
    obj->unk70 = 1.6333334f;
    while (TRUE) {
        D_ovl0C_06_800493C4->unk30 = 4;
        D_ovl0C_06_800493C4->unk34 = 100.0f;
        D_ovl0C_06_800493C4->unk38 = 100.0f;
        D_ovl0C_06_800493C4->unk40->unk10 = rec->unk24;
        D_ovl0C_06_800493C4->unk40->unk14 = rec->unk28;
        D_ovl0C_06_800493C4->unk40->unk18 = rec->unk2C;
        func_ovl0C_06_800486C8(rec->unkC);
        if (func_ovl0C_06_80048568(D_ovl0C_06_800493C4, 2) != 0) {
            wait = 2;
            rec->unk44 |= 1;
        } else if (wait == 0) {
            rec->unk44 &= ~1;
        } else {
            wait--;
        }
        if (func_ovl0C_06_80048568(D_ovl0C_06_800493C4, 0x40) != 0 && (rec->unk44 & 1)) {
            func_ovl0C_06_80047BF8(rec->unk34, rec->unk36, rec->unk38, rec->unk3A, rec->unk3E, 0x10);
            func_ovl0C_06_800485C0(rec->unkC, 0x100);
            obj->unk68 = rec->unk30;
        }
        if (obj->unk40->unk14 - obj->unk6C + obj->unk68 <= rec->unk1C) {
            obj->unk6C = 0.0f;
            obj->unk68 = 0.0f;
            obj->unk40->unk14 = rec->unk1C;
            func_ovl0C_06_800485B8(rec->unkC, 0x100);
        } else {
            obj->unk40->unk14 += obj->unk68;
            obj->unk40->unk14 -= obj->unk6C;
            if (obj->unk6C <= 99.0f) {
                obj->unk6C += obj->unk70;
            } else {
                obj->unk6C = 99.0f;
            }
        }
        if (obj->unk40->unk14 - obj->unk6C + obj->unk68 <= rec->unk1C + 100.0f &&
            obj->unk40->unk14 < player->unk40->unk14 && obj->unk68 == 0) {
            func_ovl0C_06_80047BF8(rec->unk34, rec->unk36, rec->unk38, rec->unk3A, rec->unk3E, 0x11);
        }
        if (rec->unk44 & 2) {
            player->unk6C = 0;
        }
        func_ovl0C_06_80046EB0(rec, obj);
        if (obj->unk6C == 0 && obj->unk68 == 0) {
            func_ovl0C_06_80047BF8(rec->unk34, rec->unk36, rec->unk38, rec->unk3A, rec->unk3E, 0x11);
            if (rec->unk44 & 2) {
                func_ovl0C_06_80048750();
                rec->unk44 &= ~2;
                func_ovl0C_06_80045874(player);
                func_ovl0C_06_80047DA8(0);
            }
        }
        if (rec->unk20 < obj->unk40->unk14) {
            func_ovl0C_06_80047BF8(rec->unk34, rec->unk3C, rec->unk38, rec->unk3A, rec->unk3E, 0x11);
            func_ovl0C_06_80048750();
            if (rec->unk44 & 2) {
                func_ovl0C_06_80046E14(rec, obj, player);
            }
            func_ovl0C_06_80048728(60);
            obj->unk40->unk14 = rec->unk20;
            obj->unk6C = 0.0f;
            obj->unk68 = 0.0f;
            player->unk6C = 0.0f;
            player->unk68 = 0.0f;
            func_ovl0C_06_80047BF8(rec->unk34, rec->unk3C, rec->unk38, rec->unk3A, rec->unk3E, 0x10);
        }
        func_ovl0C_06_80048750();
    }
}

void func_ovl0C_06_80046EB0(UnkStruct_ovl0C_06_80048F88 *arg0, UnkStructA *arg1) {
    UnkStructA *player;

    if (arg1->unk68 != 0.0f && func_ovl0C_06_80048568(arg1, 1) != 0) {
        player = func_ovl0C_06_80047DF8(0);
        if (arg0->unk14 + 100.0f <= player->unk40->unk14 &&
            player->unk40->unk10 < arg0->unk10 + 100.0f && arg0->unk10 - 100.0f < player->unk40->unk10 &&
            player->unk40->unk18 < arg0->unk18 + 100.0f && arg0->unk18 - 100.0f < player->unk40->unk18) {
            func_ovl0C_06_80047DA8(1);
            func_ovl0C_06_80048598(arg1, player);
            func_ovl0C_06_800485B8(player, 0x200000);
            player->unk4C = player->unk40->unk10 - arg0->unk10;
            player->unk50 = 100.0f;
            player->unk54 = player->unk40->unk18 - arg0->unk18;
            arg0->unk44 |= 2;
        }
    }
}

void func_ovl0C_06_80046E14(UnkStruct_ovl0C_06_80048F88 *arg0, UnkStructA *arg1, UnkStructA *arg2) {
    f32 pos[3];

    arg0->unk44 &= ~2;
    pos[0] = arg2->unk40->unk10;
    pos[1] = arg2->unk40->unk14;
    pos[2] = arg2->unk40->unk18;
    func_ovl0C_06_80048BA8(pos);
    func_ovl0C_06_80048C78();
    func_ovl0C_06_80045874(arg2);
    func_ovl0C_06_80048750();
    func_ovl0C_06_80047DA8(0);
}

/* a switch fading out with its block: its tiles open */
void func_ovl0C_06_80046CF0(void) {
    UnkStruct_ovl0C_01_8004A22C *rec;
    s16 alpha;

    rec = func_ovl0C_06_80048730();
    alpha = 0xFF;
    while (TRUE) {
        alpha -= 0xA;
        func_ovl0C_06_800485B8(rec->unk4, 0x80000);
        if (alpha < 0) {
            alpha = 0;
            func_ovl0C_06_800485C0(rec->unk4, 0x01000000);
            func_ovl0C_06_80047C00(rec->unk18, rec->unk1A, rec->unk1C, rec->unk18 + 1, rec->unk1E + 1, rec->unk1C + 1, 0x10);
            func_ovl0C_06_80047BF8(rec->unk18, rec->unk1E, rec->unk1C, rec->unk18 + 1, rec->unk1C + 1, 0x11);
            rec->unk22 = 0;
            func_ovl0C_06_800485D0(rec->unk4);
            func_ovl0C_06_80048718();
        }
        rec->unk4->unk40->unk36 = alpha;
        func_ovl0C_06_80048750();
    }
}

/* the switches' sparks, each on its own random timer */
void func_ovl0C_06_80046BD8(void) {
    UnkStruct_ovl0C_01_8004A22C *list;
    UnkStruct_ovl0C_01_8004A22C *p;

    list = func_ovl0C_06_80048730();
    for (p = list; p->unk2 != 0; p++) {
        p->unk20 = func_ovl0C_06_800488C8(5) + 0x14;
        p->unk22 = 1;
    }
    while (TRUE) {
        for (p = list; p->unk2 != 0; p++) {
            if (p->unk20-- <= 0 && p->unk22 == 1) {
                func_ovl0C_06_80048B58(0xF, p->unk8 - 100.0f, p->unk14, p->unk10, 0);
                func_ovl0C_06_80048C90(0x4A);
                p->unk20 = func_ovl0C_06_800488C8(5) + 0x14;
            }
        }
        func_ovl0C_06_80048750();
    }
}

/* a block crumbling: it sinks, fades, and its smoke rises */
void func_ovl0C_06_8004696C(void) {
    UnkStruct_ovl0C_06_8004914C *rec;
    UnkStructA *obj;
    s16 alpha;

    rec = func_ovl0C_06_80048730();
    obj = rec->unk4;
    if (rec->unk20 != NULL) {
        func_ovl0C_06_80048708(func_ovl0C_06_80046CF0, rec->unk20, 0, 0, 0x110);
    }
    func_ovl0C_06_80047BF8(rec->unk18, rec->unk1A, rec->unk1C, rec->unk18 + 3, rec->unk1C + 2, 0x10);
    func_ovl0C_06_80047BF8(rec->unk18, rec->unk1E, rec->unk1C, rec->unk18 + 3, rec->unk1C + 2, 0x10);
    if (rec->unk24 == 1) {
        func_ovl0C_06_80047BF8(0x2B, 3, 0xB, 0x2D, 0x12, 0x11);
        func_ovl0C_06_80047BF8(0x2C, 3, 0xC, 0x2D, 0x11, 0x10);
        func_ovl0C_06_80047BF8(0x2B, 3, 0xE, 0x2B, 0xF, 0x10);
    }
    while (TRUE) {
        obj->unk40->unk1C -= 5.0f;
        if (obj->unk40->unk1C < -50.0f) {
            break;
        }
        func_ovl0C_06_80048750();
    }
    func_ovl0C_06_800485B8(obj, 0x80000);
    alpha = 0xFF;
    while (TRUE) {
        alpha -= 0xA;
        if (alpha < 0) {
            alpha = 0;
        }
        obj->unk40->unk36 = alpha;
        func_ovl0C_06_80048638(obj);
        if (obj->unk6C == 0.0f && obj->unk68 == 0.0f) {
            func_ovl0C_06_80048B58(0x10, obj->unk40->unk10, obj->unk40->unk14 + 100.0f, obj->unk40->unk18, 0);
            func_ovl0C_06_800485C0(obj, 0x01000000);
            func_ovl0C_06_80048718();
        }
        func_ovl0C_06_80048750();
    }
}

/* a mover on x and z: carries D_ovl0C_06_80048FD0[rec->unk0]'s object
 * across, then drops it */
void func_ovl0C_06_80046288(void) {
    UnkStruct_ovl0C_06_80049030 *rec;
    UnkStructA *obj;
    UnkStructA *other;
    UnkStruct_ovl0C_06_80048FD0 *lim;
    s16 done;
    f32 dist;
    s32 fl;

    rec = func_ovl0C_06_80048730();
    obj = rec->unk4;
    obj->unk68 = rec->unk18;
    obj->unk70 = rec->unk1C;
    lim = &D_ovl0C_06_80048FD0[rec->unk0];
    other = lim->unk0;
    done = FALSE;
    func_ovl0C_06_80047E00(1);
    func_ovl0C_06_80048CB0(0x48, obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18);
    while (TRUE) {
        obj->unk40->unk10 += rec->unk10;
        obj->unk40->unk18 += rec->unk14;
        if (rec->unk10 != 0) {
            if (rec->unk10 > 0) {
                if (rec->unk34 <= obj->unk40->unk10) {
                    break;
                }
            } else if (obj->unk40->unk10 <= rec->unk34) {
                break;
            }
        }
        if (rec->unk14 != 0) {
            if (rec->unk14 > 0) {
                if (rec->unk38 <= obj->unk40->unk18) {
                    break;
                }
            } else if (obj->unk40->unk18 <= rec->unk38) {
                break;
            }
        }
        func_ovl0C_06_80048750();
    }
    while (TRUE) {
        if (rec->unk2C <= obj->unk40->unk14 + obj->unk6C - obj->unk68) {
            break;
        }
        obj->unk40->unk14 -= obj->unk68;
        obj->unk40->unk14 += obj->unk6C;
        if (obj->unk6C <= 99.0f) {
            obj->unk6C += obj->unk70;
        } else {
            obj->unk6C = 99.0f;
        }
        obj->unk40->unk10 += rec->unk10;
        obj->unk40->unk18 += rec->unk14;
        if (done == FALSE) {
            dist = lim->unk14;
            if (rec->unk10 != 0) {
                if (rec->unk10 > 0) {
                    if (lim->unk18 < obj->unk40->unk10) {
                        done = TRUE;
                        UNK_MACRO_ovl0C_06_80046200(rec->unk20, obj, other, dist);
                    }
                } else if (obj->unk40->unk10 < lim->unk18) {
                    done = TRUE;
                    UNK_MACRO_ovl0C_06_80046200(rec->unk20, obj, other, dist);
                }
            }
            if (rec->unk14 != 0) {
                if (rec->unk14 > 0) {
                    if (lim->unk1C < obj->unk40->unk18) {
                        done = TRUE;
                        UNK_MACRO_ovl0C_06_80046200(rec->unk20, obj, other, dist);
                    }
                } else if (obj->unk40->unk18 < lim->unk1C) {
                    done = TRUE;
                    UNK_MACRO_ovl0C_06_80046200(rec->unk20, obj, other, dist);
                }
            }
        }
        func_ovl0C_06_80048750();
    }
    done = FALSE;
    while (TRUE) {
        if (rec->unk10 != 0) {
            if (rec->unk10 > 0) {
                if (rec->unk3C <= obj->unk40->unk10) {
                    done = TRUE;
                }
            } else if (obj->unk40->unk10 <= rec->unk3C) {
                done = TRUE;
            }
        }
        if (rec->unk14 != 0) {
            if (rec->unk14 > 0) {
                if (rec->unk40 <= obj->unk40->unk18) {
                    done = TRUE;
                }
            } else if (obj->unk40->unk18 <= rec->unk40) {
                done = TRUE;
            }
        }
        if (done == TRUE) {
            func_ovl0C_06_80047DA8(0);
            func_ovl0C_06_80048980(rec->unk24);
            if (func_ovl0C_06_80048990(0x431) == 0) {
                func_ovl0C_06_80047CB0(rec->unk8, rec->unkC, 0);
            } else {
                func_ovl0C_06_80048E08();
            }
            func_ovl0C_06_80048718();
        }
        obj->unk40->unk10 += rec->unk10;
        obj->unk40->unk18 += rec->unk14;
        func_ovl0C_06_80048750();
    }
}

void func_ovl0C_06_80046200(s32 arg0, UnkStructA *arg1, UnkStructA *arg2, f32 arg3) {
    if (arg0 & 1) {
        func_ovl0C_06_80048C90(0x4C);
        func_ovl0C_06_8004394C(arg1, arg2, arg3);
    } else {
        func_ovl0C_06_80048C90(0x4C);
        func_ovl0C_06_80045874(arg2);
    }
}

void func_ovl0C_06_80046198(void) {
    UnkStructA *obj;

    obj = *(UnkStructA **) func_ovl0C_06_80048730();
    obj->unk68 = 0.0f;
    obj->unk70 = 1.6333334f;
    while (TRUE) {
        func_ovl0C_06_80048638(obj);
        func_ovl0C_06_80048750();
    }
}

void func_ovl0C_06_80046160(void *arg0) {
    func_ovl0C_06_80048708(func_ovl0C_06_80046288, arg0, 0, 0, 0x110);
}

void func_ovl0C_06_80045FC8(void) {
    UnkStruct_ovl0C_01_8004A104 *rec;
    UnkStructA_40 *p;

    rec = func_ovl0C_06_80048730();
    p = rec->unk4->unk40;
    D_ovl0C_06_800493C8[rec->unk0]->unkC = 0x40;
    while (TRUE) {
        if (D_ovl0C_06_800493C8[rec->unk0]->unk10 & 0x40) {
            func_ovl0C_06_80048980(0x429);
            func_ovl0C_06_80048C90(0x53);
            p->unk48->unk4[0].unk4 = func_ovl0C_06_800482F8(p, 0) - 0.01f;
            while (p->unk48->unk4[0].unk4 > 1.0f) {
                func_ovl0C_06_80048750();
                p->unk48->unk4[0].unk4 -= 1.0f;
            }
            p->unk48->unk4[0].unk4 = 0.0f;
            func_ovl0C_06_80044164(rec);
            func_ovl0C_06_80048718();
        }
        func_ovl0C_06_80048750();
    }
}

void func_ovl0C_06_80045F44(void) {
    UnkStructA *obj;
    s32 alpha;
    s32 step;

    func_ovl0C_06_80047DA8(1);
    func_ovl0C_06_80048728(10);
    obj = func_ovl0C_06_80047DF8(func_ovl0C_06_80047D98());
    func_ovl0C_06_80048C90(0x44);
    func_ovl0C_06_800485B8(obj, 0x80000);
    step = 0x11;
    for (alpha = 0xFF; alpha > 0; alpha -= step) {
        obj->unk40->unk36 = alpha;
        func_ovl0C_06_80048750();
    }
    obj->unk40->unk36 = 0;
    func_ovl0C_06_80048718();
}

/* mover 0: swings its object round on a circle, drops limit 0's object
 * on the way */
void func_ovl0C_06_80045BA0(void) {
    f32 pad[2]; /* fake: unused; the original's frame is 8 bytes bigger */
    UnkStructA *obj;
    UnkStructA *other;
    f32 lim;
    f32 cx;
    f32 cz;
    f32 a;
    s32 snd;
    s32 dropped;
    s32 vol;

    func_ovl0C_06_800458A0(&D_ovl0C_06_80048FD0[0]);
    obj = D_ovl0C_06_80049030[0].unk4;
    snd = func_ovl0C_06_80048C90(0x4E);
    obj->unk68 = D_ovl0C_06_80049030[0].unk18 * 0.75f;
    obj->unk70 = D_ovl0C_06_80049030[0].unk1C * 0.5f;
    dropped = 0;
    cx = obj->unk40->unk10 - 30.0f;
    a = 0.0f;
    cz = obj->unk40->unk18 - 600.0f;
    while (TRUE) {
        if (D_ovl0C_06_80049030[0].unk2C <= obj->unk40->unk14 + obj->unk6C - obj->unk68) {
            break;
        }
        obj->unk40->unk14 -= obj->unk68;
        obj->unk40->unk14 += obj->unk6C;
        if (obj->unk6C <= 99.0f) {
            obj->unk6C += obj->unk70;
        } else {
            obj->unk6C = 99.0f;
        }
        if (dropped == 0 && D_ovl0C_06_80048FD0[0].unk18 < obj->unk40->unk10) {
            lim = D_ovl0C_06_80048FD0[0].unk14;
            dropped = 1;
            func_ovl0C_06_80048C90(0x4C);
            other = D_ovl0C_06_80048FD0[0].unk0;
            func_ovl0C_06_80048598(obj, other);
            func_ovl0C_06_800485B8(other, 0x200000);
            other->unk4C = 0.0f;
            other->unk50 = lim;
            other->unk54 = 0.0f;
        }
        obj->unk40->unk10 = func_ovl0C_06_800488B0(a) * 1200.0f + cx;
        obj->unk40->unk18 = func_ovl0C_06_800488B8(a) * 1200.0f + cz;
        obj->unk40->unk20 = a + 90.0f;
        func_ovl0C_06_80048750();
        a += 0.85f;
    }
    vol = 0x7F;
    while (obj->unk40->unk10 > 1500.0f) {
        obj->unk40->unk10 = func_ovl0C_06_800488B0(a) * 1200.0f + cx;
        obj->unk40->unk18 = func_ovl0C_06_800488B8(a) * 1200.0f + cz;
        obj->unk40->unk20 = a + 90.0f;
        if (vol > 0) {
            vol -= 5;
            func_ovl0C_06_80048CC8(snd, vol);
        } else {
            func_ovl0C_06_80048CC8(snd, 0);
        }
        func_ovl0C_06_80048750();
        a += 0.85f;
    }
    func_ovl0C_06_80048CC0(snd);
    func_ovl0C_06_80048980(D_ovl0C_06_80049030[0].unk24);
    func_ovl0C_06_80047CB0(D_ovl0C_06_80049030[0].unk8, D_ovl0C_06_80049030[0].unkC, 0);
    while (TRUE) {
        func_ovl0C_06_80048750();
    }
}

/* mover 1: carries limit 1's object along x and z, then drops it */
void func_ovl0C_06_800458D8(void) {
    UnkStructA *obj;
    s32 snd;
    s32 vol;

    func_ovl0C_06_80047D28(1);
    func_ovl0C_06_80047D20(0);
    func_ovl0C_06_80047DA8(1);
    func_ovl0C_06_80048460(0x7F);
    func_ovl0C_06_800483F8(2650.0f, 1500.0f, 1950.0f);
    func_ovl0C_06_80048418(0.0f, 45.0f, 2000.0f);
    func_ovl0C_06_800458A0(&D_ovl0C_06_80048FD0[1]);
    obj = D_ovl0C_06_80049030[1].unk4;
    snd = func_ovl0C_06_80048C90(0x4E);
    obj->unk40->unk20 = 135.0f;
    while (obj->unk40->unk10 < 4350.0f) {
        obj->unk40->unk10 += 7.0f;
        obj->unk40->unk18 -= 7.0f;
        func_ovl0C_06_800483F8(obj->unk40->unk10, 1500.0f, obj->unk40->unk18);
        func_ovl0C_06_80048750();
    }
    obj->unk40->unk10 = 4350.0f;
    D_ovl0C_06_80048FD0[1].unk0->unk6C = 0.0f;
    func_ovl0C_06_80048C90(0x4C);
    func_ovl0C_06_80045874(D_ovl0C_06_80048FD0[1].unk0);
    vol = 0x7F; while (obj->unk40->unk10 < 5000.0f) {

        obj->unk40->unk10 += 7.0f;
        obj->unk40->unk14 += 10.0f;
        obj->unk40->unk18 -= 7.0f;
        if (vol > 0) {
            vol -= 2;
            func_ovl0C_06_80048CC8(snd, vol);
        } else {
            func_ovl0C_06_80048CC8(snd, 0);
        }
        func_ovl0C_06_80048750();
    }
    func_ovl0C_06_80048CC0(snd);
    func_ovl0C_06_80048980(D_ovl0C_06_80049030[1].unk24);
    func_ovl0C_06_80047CB0(D_ovl0C_06_80049030[1].unk8, D_ovl0C_06_80049030[1].unkC, 0);
    while (TRUE) {
        func_ovl0C_06_80048750();
    }
}

void func_ovl0C_06_800458A0(void *arg0) {
    func_ovl0C_06_80048708(func_ovl0C_06_80046198, arg0, 0, 0, 0x110);
}

void func_ovl0C_06_80045874(UnkStructA *arg0) {
    func_ovl0C_06_80048590(arg0);
    func_ovl0C_06_800485C0(arg0, 0x200000);
}

/* mover 2: lifts its object up, then the camera follows it */
void func_ovl0C_06_800455DC(void) {
    UnkStructA *obj;
    f32 dy;
    f32 pitch;

    obj = D_ovl0C_06_80049030[2].unk4;
    func_ovl0C_06_80048CB0(0x48, obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18);
    obj->unk40->unk20 = -45.0f;
    while (obj->unk40->unk14 > 1400.0f) {
        obj->unk40->unk14 -= 5.0f;
        func_ovl0C_06_80048750();
    }
    obj->unk40->unk14 = 1400.0f;
    func_ovl0C_06_80048C90(0x4C);
    func_ovl0C_06_8004394C(obj, D_ovl0C_06_80048FD0[2].unk0, -400.0f);
    dy = 0.0f;
    pitch = 45.0f;
    while (obj->unk40->unk14 < 2000.0f) {
        obj->unk40->unk14 += 10.0f;
        dy -= 10.0f;
        func_ovl0C_06_800483F8(4300.0f, 1000.0f - dy, 200.0f);
        func_ovl0C_06_80048418(-45.0f, pitch, 2000.0f);
        func_ovl0C_06_80048750();
        pitch -= 1.0f;
    }
    while (obj->unk40->unk14 < 2700.0f) {
        obj->unk40->unk14 += 10.0f;
        func_ovl0C_06_80048750();
    }
    func_ovl0C_06_80048980(D_ovl0C_06_80049030[2].unk24);
    func_ovl0C_06_80047CB0(D_ovl0C_06_80049030[2].unk8, D_ovl0C_06_80049030[2].unkC, 0);
    while (TRUE) {
        func_ovl0C_06_80048750();
    }
}

/* the camera of the blocks' cut scene */
void func_ovl0C_06_80045524(void) {
    s32 i;

    func_ovl0C_06_80047D20(0);
    func_ovl0C_06_80048518();
    func_ovl0C_06_80048460(0x7F);
    func_ovl0C_06_800483F8(4500.0f, 200.0f, 1750.0f);
    func_ovl0C_06_80048418(225.0f, 45.0f, 2400.0f);
    for (i = 0; i < 360; i++) {
        func_ovl0C_06_80048750();
    }
    func_ovl0C_06_8004491C();
    func_ovl0C_06_80047D20(1);
    func_ovl0C_06_80048718();
}

/* the stage's event handler (the overlay's entry returns it) */
void func_ovl0C_06_80045364(s32 arg0) {
    switch (arg0) {
        case 2:
            func_ovl0C_06_8004510C();
            break;
        case 0x10:
            func_ovl0C_06_80044BC8();
            break;
        case 0x11:
            func_ovl0C_06_80044B60();
            break;
        case 0x12:
            func_ovl0C_06_80044B10();
            break;
        case 0x13:
            func_ovl0C_06_80044AB0();
            break;
        case 0x14:
            func_ovl0C_06_80044A4C();
            break;
        case 0x15:
            func_ovl0C_06_800449F0();
            break;
        case 0x16:
            func_ovl0C_06_800449C8();
            break;
        case 0x17:
            func_ovl0C_06_80044950();
            break;
        case 0x18:
            func_ovl0C_06_800448D4();
            break;
        case 0x19:
            func_ovl0C_06_80044884();
            break;
        case 0x1A:
            func_ovl0C_06_80044830();
            break;
        case 4:
            func_ovl0C_06_800446FC();
            break;
        case 5:
            func_ovl0C_06_800440B4();
            break;
        case 0x50:
            func_ovl0C_06_80043DC0();
            break;
        case 0x52:
            func_ovl0C_06_80043B3C();
            break;
        case 0x53:
            break;
        case 3:
            func_ovl0C_06_80043868();
            break;
        case 0xF:
            func_ovl0C_06_800431B8();
            break;
        case 0xC:
            func_ovl0C_06_80043020();
            break;
        case 1:
            func_ovl0C_06_80043010();
            break;
    }
}

/* the stage's setup (ovl_0C_07's, with the sign) */
void func_ovl0C_06_8004510C(void) {
    UnkStruct_ovl0C_01_8004A54C *rec;
    u32 (*p)[3];
    s32 i;
    s32 count;

    func_ovl0C_06_800450D4();
    func_ovl0C_06_80048A70(D_ovl0C_06_80048EB0);
    func_ovl0C_06_80048A88(D_ovl0C_06_80048ED4);
    func_ovl0C_06_80047BB8(0x247);
    p = D_ovl0C_06_80048EEC;
    count = 3;
    for (i = 0; i < count; i++) {
        func_ovl0C_06_80048C18((*p)[0], (*p)[1], (*p)[2]);
        p++;
    }
    func_ovl0C_06_80047BD0(0x251);
    func_ovl0C_06_80047D68();
    func_ovl0C_06_80044F44();
    func_ovl0C_06_80044DA0();
    rec = &D_ovl0C_06_80049274;
    rec->unk0 = func_ovl0C_06_80048600(rec->unk8, 0xD0, 0.0f, 0.0f, 0.0f);
    func_ovl0C_06_800485B8(rec->unk0, 0x01080000);
    func_ovl0C_06_800485C0(rec->unk0, 0x02000000);
    if (rec->unkC != -1 && rec->unkE != -1 && rec->unk10 != -1 && rec->unk12 != -1) {
        rec->unk4 = func_ovl0C_06_80048300(rec->unk0->unk40, 0, rec->unkC, rec->unkE, rec->unk10, rec->unk12);
        if (rec->unk4 == -1) {
            for (;;) {
            }
        }
    }
    rec->unk0->unk40->unk10 = rec->unk14 * 10.0f;
    rec->unk0->unk40->unk14 = rec->unk18 * 10.0f;
    rec->unk0->unk40->unk18 = rec->unk1C * 10.0f;
    rec->unk0->unk40->unk1C = rec->unk20;
    rec->unk0->unk40->unk20 = rec->unk24;
    rec->unk0->unk40->unk24 = rec->unk28;
    rec->unk0->unk40->unk28 = rec->unk2C;
    rec->unk0->unk40->unk2C = rec->unk30;
    rec->unk0->unk40->unk30 = rec->unk34;
#define STATICS_PART 1
#include "overlays/ovl_0C_06/statics.h"
#undef STATICS_PART
