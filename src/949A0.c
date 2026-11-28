/*
 * main_code's 949A0, IDO -O3; it matches in us, eu and jp: the rest of
 * the bonus item code (func_8027A1A0 to func_8027BCE8): the item table
 * read from file 0xDC (func_8027A640), the thread's setup
 * (func_8027A950), an item's creation (func_8027AA74) and the per-player
 * pickups. The C is written last function first, as -O3 emits it.
 *
 * Data: the file defines the .data (module 0x28's table) and the .bss that
 * 93850 shares (93850 has none of its own); all three sections are placed
 * in data_split.txt.
 */
#include "common.h"
#include "bonus.h"
#include "player.h"
#include "4E030.h"
#include "54E40.h"
#include "5E3F0.h"
#include "8A7A0.h"
#include "93850.h"
#include "949A0.h"
#include "A4860.h"
#include "530C0.h"
#include "86680.h"
#include "48600.h"
#include "404C0.h"

void func_80270354(s32 *, s32);
void func_8027036C(s32 *, s32);
PORT_RET(s32, s16) func_802712A8(s32 *, s32);
void *malloc(s32);
void free(void *);
void func_80270080(UnkStruct_8027A264 *);
void func_80277DA8(s16, s32);
UnkStructA *func_802443F4(s32);
UnkStruct_800BB4A8 *func_8027AA74(s16, f32, f32, f32, s32, s32, s32);
void func_8027A950(void);
PORT_RET(void, s32) func_8028A81C(s32, f32, f32, f32, PORT_IP(s32));

#ifdef TARGET_PC
/* the list's head is D_800BB4A0[0]'s link (its own name on the N64) */
#undef D_800BB4A8
#define D_800BB4A8 (D_800BB4A0[0].unk8)
#else
extern UnkStruct_800BB4A8 *D_800BB4A8;
#endif
extern UnkStruct_800AEDF0 D_800AEDF0[];
extern UnkStruct_800BB4A8 D_800BB4A0[48];
extern UnkStruct_800BB4A8 D_800BB4F8; /* [0] the list's head, [1] its sentinel, [2..47] the pool */

void func_80270BA4(UnkStructA *);
void func_80297D30(s32, void *);
void func_8027AFB8(s16 arg0);

extern UnkStruct_800BA8A0 D_800BA8A0[PORT_SIZE(5)];

UnkStructA *func_80231274(s32, s32, s32, void *);
s32 func_80238A10(void (*)(void), PORT_IP(s32), void *, s32, s32);
void func_8027A264(UnkStruct_800BB4A8 *arg0, s16 arg1);
s16 func_8027A4E8(s16 arg0);
s16 func_8027A574(s16 arg0);
void func_8027A380(s16 arg0);
void func_8027A478(UnkStruct_800BB4A8 *arg0);
UnkStruct_800BB4A8 *func_8027A4A0(void);
s16 func_8027AF70(void);
extern u8 D_800BD818[PORT_SIZE(0x1000)]; /* a thread's stack */
extern s32 D_802AC5D0;

#define ABSF(x) ((x) < 0.0f ? -(x) : (x))

s32 func_8027A1F4(UnkStructA *arg0, PORT_IP(s32) arg1);
UnkStruct_800BB4A8 *func_8027AA74(s16 arg0, f32 arg1, f32 arg2, f32 arg3, s32 arg4, s32 arg5, s32 arg6);
s32 func_8027B094(s16 arg0);
void func_8027B0BC(s16 arg0);
void func_8027B4B0(s16 arg0);
void func_8027B66C(s16 arg0, s16 arg1);
void func_8027B6A4(UnkStruct_800BB4A8 *arg0, UnkStruct_800BB4A8 *arg1);
void func_8027B6D4(UnkStruct_800BB4A8 *arg0, UnkStruct_800BB4A8 *arg1);
void func_8027B70C(void);
s32 func_8027B7C4(s16 arg0, f32 arg1, f32 arg2, f32 arg3);
s32 func_8027BA80(s16 arg0, f32 arg1, f32 arg2, f32 arg3, s32 arg4, s32 arg5);
s32 func_8027BAE8(s16 arg0, f32 arg1, f32 arg2, f32 arg3);
void func_8027BB48(void);
void func_8027BB9C(void);
void func_8027BC74(void);
s32 func_8027BCD8(void);

/* .data: ZeroJump module 0x28's table (func_8027BCE8); its last two words
   are zeros, taken here for the section's padding */
void *D_802A1500[] = {
    func_8027B828, func_8027B7C4, func_8027BCD8, func_8027BAE8, func_8027BA80, func_8027BC74,
};

/* .bss, all of 93850's and 949A0's (93850 uses D_802AFECC and D_802AFED0);
   the 4 bytes after D_802AFED0 are IDO's, aligning D_802AFF60 to 8 */
u8 *D_802AFE80;
u8 *D_802AFE84;
s16 D_802AFE88[10][3];
s16 D_802AFEC4;
s32 D_802AFEC8;
s32 D_802AFECC;
UnkStruct_802AFED0 D_802AFED0[5];
s16 D_802AFF60[7];

void func_8027BCE8(void) {
    func_80297D30(0x28, D_802A1500);
    D_802AFEC4 = 0;
    D_802AFECC = 0;
    D_802AFE80 = NULL;
    D_802AFE84 = NULL;
    func_8027BB48();
}

s32 func_8027BCD8(void) {
    return D_802AFECC;
}

void func_8027BC74(void) {
    UnkStruct_800BB4A8 *node;

    if (D_802AFEC4 != 0) {
        for (node = D_800BB4A8; node->unk8 != NULL; node = node->unk8) {
            func_802790E0(node);
        }
    }
}

void func_8027BBC4(void) {
    UnkStruct_800BB4A8 *node;

    D_802AFECC = 0;
    D_802AFF60[6] = 0;
    D_802AFF60[5] = 0;
    D_802AFF60[4] = 0;
    D_802AFF60[3] = 0;
    D_802AFF60[2] = 0;
    D_802AFF60[1] = 0;
    D_802AFF60[0] = 0;
    func_8027BB48();
    if (D_802AFEC4 != 0) {
        D_802AFEC4 = 0;
        for (node = D_800BB4A8; node->unk8 != NULL; node = node->unk8) {
            func_802790E0(node);
        }
        func_80238EA8(D_802AFEC8);
    }
}

void func_8027BB9C(void) {
    D_802AFF60[0] = 0;
    D_802AFF60[1] = 0;
    D_802AFF60[2] = 0;
    D_802AFF60[3] = 0;
    D_802AFF60[4] = 0;
    D_802AFF60[5] = 0;
    D_802AFF60[6] = 0;
}

void func_8027BB48(void) {
    s16 i;

    for (i = 0; i < 5; i++) {
        D_800BA8A0[i].unk0 = 0;
        D_800BA8A0[i].unk2 = 0;
        D_800BA8A0[i].unk4 = 0;
        D_800BA8A0[i].unk6 = 0;
        D_800BA8A0[i].unk8 = 0;
        D_800BA8A0[i].unkA = 0;
        D_800BA8A0[i].unkC = 0;
    }
}

s32 func_8027BAE8(s16 arg0, f32 arg1, f32 arg2, f32 arg3) {
    if (func_8027AA74(arg0, arg1, arg2, arg3, 0, 1, 0) == NULL) {
        return 0;
    }
    return 1;
}

s32 func_8027BA80(s16 arg0, f32 arg1, f32 arg2, f32 arg3, s32 arg4, s32 arg5) {
    if (func_8027AA74(arg0, arg1, arg2, arg3, arg4, 1, arg5) == NULL) {
        return 0;
    }
    return 1;
}

s32 func_8027B8D4(s16 arg0, s16 arg1, s16 arg2, s16 arg3) {
    s16 pad0; /* fake: the pads give the original's stack layout; the tail as func_8027B7C4 or the whole as func_8027B828 (inlined, any pad/t2 count) puts x/y/z 4 high or grows the frame */
    s16 type;
    s16 t1;
    UnkStruct_800BB4A8 * t2;
    s32 pad[3]; /* fake: frame slots */
    f32 x;
    f32 y;
    f32 z;
    s32 r;
    s16 i;
    s16 sum;
    s32 pad2[4]; /* fake: frame slots */

    t1 = func_8027AF70();
    if (t1 >= 8) {
        return 0;
    }
    r = func_80234248(100);
    for (i = 0, sum = 0; i < 10; i++) {
        sum += D_802AFE88[i][arg3];
        if (r < sum) {
            break;
        }
    }
    switch (i) {
        case 0:
            type = 0;
            break;
        case 1:
            type = 1;
            break;
        case 2:
            type = 2;
            break;
        case 3:
            type = 3;
            break;
        case 4:
            type = 4;
            break;
        case 5:
            type = 5;
            break;
        case 6:
            return 1;
        case 7:
            type = 7;
            break;
        case 8:
            type = 8;
            break;
        case 9:
            return 1;
        default:
            return 1;
    }
    func_8027404C(arg0, arg1, arg2, &x, &y, &z);
    t2 = func_8027AA74(type, x, y, z, 1, 1, 0);
    if (t2 == NULL) {
        return 0;
    }
    return 1;
}

s32 func_8027B828(s16 arg0, s16 arg1, s16 arg2, s16 arg3) {
    f32 x;
    f32 y;
    f32 z;

    func_8027404C(arg1, arg2, arg3, &x, &y, &z);
    return func_8027B7C4(arg0, x, y, z);
}

s32 func_8027B7C4(s16 arg0, f32 arg1, f32 arg2, f32 arg3) {
    if (func_8027AA74(arg0, arg1, arg2, arg3, 1, 1, 0) == NULL) {
        return 0;
    }
    return 1;
}

void func_8027B70C(void) {
    UnkStruct_800BB4A8 *a;
    UnkStruct_800BB4A8 *b;
    UnkStruct_800BB4A8 *next;
    UnkStruct_800BB4A8 *tmp;

    for (a = D_800BB4A8; a->unk8 != NULL; a = a->unk8) {
        b = a->unk8;
        if (b->unk8 == NULL) {
            break;
        }
        do {
            if (a->unk1C < b->unk1C) {
                next = b->unk8;
                func_8027B6A4(a, b);
                func_8027B6A4(next, a);
                tmp = a;
                a = b;
                b = tmp;
            }
            b = b->unk8;
        } while (b->unk8 != NULL);
    }
}

void func_8027B6D4(UnkStruct_800BB4A8 *arg0, UnkStruct_800BB4A8 *arg1) {
    UnkStruct_800BB4A8 *next = arg1->unk8;

    func_8027B6A4(arg0, arg1);
    func_8027B6A4(next, arg0);
}

void func_8027B6A4(UnkStruct_800BB4A8 *arg0, UnkStruct_800BB4A8 *arg1) {
    UnkStruct_800BB4A8 *prev = arg0->unkC;
    UnkStruct_800BB4A8 *next = arg1->unk8;

    next->unkC = arg1->unkC;
    arg1->unkC->unk8 = next;
    prev->unk8 = arg1;
    arg0->unkC = arg1;
    arg1->unk8 = arg0;
    arg1->unkC = prev;
}

void func_8027B66C(s16 arg0, s16 arg1) {
    func_8027AFB8(arg0);
    func_80279628(arg0, arg1);
}

s32 func_8027B5A8(UnkStructA *arg0) {
    UnkStruct_800BB4A8 *node = func_8027A1A0(arg0);

    if (node == NULL) {
        return 0;
    }
    if (func_802712A8(&node->unk54, 2) != 0) {
        func_80270BA4(arg0);
    }
    if (func_802712A8(&node->unk54, 1) != 0) {
        func_8027036C(&node->unk54, 1);
        func_802313DC(node->unk14, 0x20000);
        func_80270B20(PORT_PTR(node->unk18));
    }
    func_80270354(&node->unk54, 2);
    func_80270354(&node->unk54, 0x40);
    func_8027036C(&node->unk54, 8);
    node->unk48 = 360;
    return 1;
}

void func_8027B4B0(s16 arg0) {
    UnkStruct_800BB4A8 *node = &D_800BB4A0[arg0];
    f32 f;

    node->unk24 += node->unk38;
    node->unk2C -= node->unk40;
    f = func_80234434(node->unk3C) * 150.0;
    node->unk28 += f - node->unk44;
    node->unk44 = f;
    node->unk3C += 4.5;
    node->unk50 -= 1.0f;
    if (node->unk50 == 0.0f) {
        func_80270354(&node->unk54, 2);
    }
}

s32 func_8027B480(s32 arg0) {
    s32 ret;

    if ((arg0 & 0xF) == 0xF) {
        ret = 0;
    } else {
        ret = 1;
    }
    if (arg0 & 0x60) {
        ret = 0;
    }
    return ret;
}

void func_8027B440(UnkStructA *arg0) {
    UnkStruct_800BB4A8 *node = func_8027A1A0(arg0);

    if (node == NULL) {
        DEBUG_PRINT(("node == NULL\n"));
    }
    func_8027036C(&node->unk54, 1);
    node->unk3C = 0.0f;
}

void func_8027B39C(s16 arg0, s16 arg1) {
    UnkStructA *obj;
    UnkStructA_40 *pos;
    UnkStruct_800BB4A8 *node;

    obj = func_802443F4(arg1);
    pos = obj->unk40;
    node = func_8027AA74(arg0, pos->unk10, pos->unk14 + 200.0f, pos->unk18, 1, 0, 0);
    if (node != NULL) {
        func_80270354(&node->unk54, 2);
        func_8027A264(node, func_80272024(func_80234248(7)));
    }
}

void func_8027B0BC(s16 arg0) {
    s16 n;
    s16 prev;
    s16 i;
    s16 r;
    s16 kinds[5];
    s16 slots[8];
    s16 range;
    UnkStructA *obj;
    UnkStruct_800BB4A8 *node;

    for (n = 0, prev = 0; ; prev = n) {
        if (D_800BA8A0[arg0].unk4 != 0) {
            D_800BA8A0[arg0].unk4--;
            kinds[n++] = 2;
            if (n >= 5) {
                break;
            }
        }
        if (D_802AFEC4 == 1 && func_8027B094(arg0) != -1) {
            func_8027AFB8(arg0);
            kinds[n++] = 5;
            if (n >= 5) {
                break;
            }
        }
        if (D_800BA8A0[arg0].unk2 != 0) {
            D_800BA8A0[arg0].unk2--;
            kinds[n++] = 1;
            if (n >= 5) {
                break;
            }
        }
        if (D_800BA8A0[arg0].unk0 != 0) {
            D_800BA8A0[arg0].unk0--;
            kinds[n++] = 0;
            if (n >= 5) {
                break;
            }
        }
        if (n == prev) {
            break;
        }
    }
    obj = func_802443F4(arg0);
    range = 7;
    for (i = 0; i < 8; i++) {
        slots[i] = -1;
    }
    for (n--; n >= 0; n--, range--) {
        if (func_8027AF70() >= 8) {
            break;
        }
        node = func_8027AA74(kinds[n], obj->unk40->unk10, obj->unk40->unk14 + 200.0f, obj->unk40->unk18, 1, 0, 0);
        if (node != NULL) {
            func_80270354(&node->unk54, 2);
            r = func_80234248(range);
            for (i = 0; r >= 0; i++) {
                if (slots[i] == -1) {
                    r--;
                }
            }
            slots[i - 1] = n;
            func_8027A264(node, func_80272024(i - 1));
        }
    }
}

s32 func_8027B094(s16 arg0) {
    return D_802AFED0[arg0].unk8;
}

void func_8027AFB8(s16 arg0) {
#ifdef TARGET_PC
    s16 state = D_802AFED0[arg0].unk8; /* the word's low half, wherever the host has it */
#else
    s16 state = ((s16 *)&D_802AFED0[arg0].unk8)[1]; /* read as the s16 at 0xA */
#endif

    switch (state) {
        case 3:
            func_80246C3C(arg0, 0, 0);
            func_80277DA8(arg0, 0x400);
            func_80246D74(arg0);
            break;
        case 6:
            func_80246C3C(arg0, 0, 0);
            func_80277DA8(arg0, 4);
            func_80246D74(arg0);
            break;
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
            break;
        default:
            return;
    }
    D_802AFED0[arg0].unk8 = -1;
}

s16 func_8027AF70(void) {
    UnkStruct_800BB4A8 *node;
    s16 count;

    func_8027A950();
    count = 0;
    node = D_800BB4A8;
    while (node->unk8 != NULL) {
        node = node->unk8;
        count++;
    }
    return count;
}

/*
 * arg4 to arg6 are read as the s16 in their low half, from the stack at
 * each use (lh), while the callers pass words untruncated (func_8027BA80).
 */
UnkStruct_800BB4A8 *func_8027AA74(s16 arg0, f32 arg1, f32 arg2, f32 arg3, s32 arg4, s32 arg5, s32 arg6) {
    s16 model;
    UnkStruct_800BB4A8 *node;
    UnkStructA_40 *pos;
    s32 item;
    s32 r;
    f32 dx;
    f32 dz;
    UnkStructA *obj;
    s32 *flags;

    func_8027A950();
    if ((D_802AC5D0 & 4) && PORT_LO16(arg5) == 1 && func_8027A574(arg0) <= 0) {
        return NULL;
    }
    item = 0;
    switch (arg0) {
        case 0:
            model = 0x12;
            break;
        case 1:
            model = 0x13;
            break;
        case 2:
            model = 0x14;
            break;
        case 3:
            model = 0x15;
            break;
        case 4:
            model = 0x16;
            break;
        case 5:
            model = 0x17;
            break;
        case 6:
            model = 0x18;
            break;
        case 7:
            model = 0x1E;
            break;
        case 8:
            model = 0x1F;
            break;
        default:
            r = func_802791C4(arg0);
            if (r == -1) {
                return NULL;
            }
            if (func_8028A36C(r)) {
                return NULL;
            }
            item = 1;
            model = func_8027A4E8(arg0);
            break;
    }
    node = func_8027A4A0();
    if (node == NULL) {
        return NULL;
    }
    func_8027A478(node);
    node->unk2 = 0;
    obj = func_80231274(model, 0x0F000008, 0xB0, NULL);
    if (obj == NULL) {
        return NULL;
    }
    if (!func_802311E8(obj)) {
        return NULL;
    }
    func_80231408(obj, 0x01000000);
    func_80231408(obj, 0x02000000);
    func_80231408(obj, 0x0C000000);
    func_80231408(obj, 8);
    obj->unk30 = 1;
    obj->unk34 = 50.0f;
    obj->unk38 = 50.0f;
    node->unk14 = obj;
    node->unk10 = obj->unk40;
    node->unk1C = 0;
    node->unk1E = 0;
    node->unk20 = arg0;
    obj->unk58 = arg1;
    node->unk10->unk10 = node->unk24 = node->unk14->unk58;
    node->unk14->unk5C = arg2;
    node->unk10->unk14 = node->unk28 = node->unk14->unk5C;
    node->unk14->unk60 = arg3;
    node->unk10->unk18 = node->unk2C = node->unk14->unk60;
    node->unk14->unk6C = 1.0f;
    node->unk38 = 0.0f;
    node->unk3C = 0.0f;
    node->unk40 = 0.0f;
    if (item == 1) {
        pos = func_802443F4(0)->unk40;
        dx = ABSF(pos->unk10) - ABSF(arg1);
        dz = ABSF(pos->unk18) - ABSF(arg3);
        if (ABSF(dx) > ABSF(dz)) {
            if (dx > 0.0f) {
                node->unk30 = 2;
            } else {
                node->unk30 = 6;
            }
        } else if (dz > 0.0f) {
            node->unk30 = 4;
        } else {
            node->unk30 = 0;
        }
    } else {
        node->unk30 = func_80234248(3) * 2;
    }
    node->unk48 = 360;
    node->unk50 = 0.0f;
    flags = &node->unk54;
    func_8027036C(flags, 2);
    func_8027036C(flags, 1);
    func_80270354(flags, 8);
    func_8027036C(flags, 0x10);
    if (PORT_LO16(arg5) == 1) {
        func_80270354(flags, 0x40);
    } else {
        func_8027036C(flags, 0x40);
    }
    if (PORT_LO16(arg6) == 1) {
        func_80270354(flags, 0x80);
    } else {
        func_8027036C(flags, 0x80);
    }
    func_8027036C(flags, 0x20);
    if (PORT_LO16(arg4) == 1) {
        func_8028A81C(3, node->unk24, node->unk28 + 70.0f, node->unk2C, 0);
        func_8026C51C(3, node->unk24, node->unk28, node->unk2C);
    }
    if ((D_802AC5D0 & 4) && PORT_LO16(arg5) == 1) {
        func_8027A380(arg0);
    }
    return node;
}

void func_8027A950(void) {
    s16 i;
    UnkStruct_800BB4A8 *p;

    if (D_802AFEC4 == 1) {
        return;
    }
    D_802AFEC4 = 1;
    D_802AFEC8 = func_80238A10(func_8027A144, 0, D_800BD818, 0x1000, 0x50);
    if (D_802AFEC8 == -1) {
        DEBUG_PRINT(("D_802AFEC8 == -1\n"));
    }
    p = D_800BB4A0;
    for (i = 0; i < 48; i++, p++) {
        p->unk0 = i;
        p->unk2 = 1;
    }
    for (i = 0; i < 5; i++) {
        D_802AFED0[i].unk8 = -1;
    }
    func_8027A640();
    D_800BB4A0[0].unk2 = 0;
    D_800BB4A0[0].unk8 = &D_800BB4A0[1];
    D_800BB4A0[0].unkC = NULL;
    D_800BB4A0[0].unk20 = -1;
    D_800BB4A0[0].unk24 = -1.0f;
    D_800BB4A0[0].unk28 = -1.0f;
    D_800BB4A0[0].unk2C = -1.0f;
    p = D_800BB4A0[0].unk8;
    p->unk2 = 0;
    p->unk8 = NULL;
    p->unkC = &D_800BB4A0[0];
    p->unk20 = -1;
    p->unk24 = -1.0f;
    p->unk28 = -1.0f;
    p->unk2C = -1.0f;
}

void func_8027A640(void) {
    UnkStruct_802AC610 *st;
    u8 *p;
    s32 found;
    s32 off;
    s32 i;
    s16 x;
    s16 y;
    s16 z;
    s16 c;
    s16 sums[3];
    s32 j;
    s32 mode;
    struct UnkStruct_80225CC0 *file;
    s32 val;
    s32 done;

    if (D_802AFE84 != NULL) {
        free(D_802AFE84);
        D_802AFE84 = NULL;
    }
    file = func_80226604(0xDC, 1);
    st = func_8023A840();
    x = S16_LO(st->unk4);
    y = S16_LO(st->unk8);
    z = S16_LO(st->unkC);
    mode = st->unk10;
    D_802AFE80 = malloc(func_802266DC(file));
    if (D_802AFE80 == NULL) {
        DEBUG_PRINT(("D_802AFE80 == NULL\n"));
    }
    func_8022616C(D_802AFE80, 1, func_802266DC(file), file);
    p = D_802AFE80;
    D_802AFE84 = p;
    x++;
    y++;
    z++;
    found = 0;
    do {
        if (x == p[0] && y == p[1] && z == p[2]) {
            found = 1;
            off = p[3] << 8;
            off += p[4];
        }
        p += 7;
    } while (*p != 0x80);
    if (!found) {
        free(D_802AFE84);
        D_802AFE84 = NULL;
        for (i = 0; i < 10; i++) {
            for (j = 0; j < 3; j++) {
                D_802AFE88[i][j] = 0;
            }
        }
        func_80226368(file);
        return;
    }
    D_802AFE80 = p + off + 1;
    p = D_802AFE80;
    while (*p != 0x88) {
        p++;
    }
    p++;
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 3; j++) D_802AFE88[i][j] = 0;
    }
    for (i = 0; i < 3; i++) {
        sums[i] = 0;
    }
    done = FALSE;
    do {
        c = *p++;
        if (c == 0xF0) {
            break;
        }
        if (mode == 0) {
            p += 3;
        }
        for (i = 0; i < 3; i++) {
            val = *p++;
            D_802AFE88[c][i] = val;
            sums[i] += val;
        }
        if (mode == 1) {
            p += 3;
        }
    } while (!done);
    for (i = 0; i < 3; i++) {
        if (sums[i] <= 100) {
            D_802AFE88[9][i] = 100 - sums[i];
        }
    }
    func_80226368(file);
}

s16 func_8027A574(s16 arg0) {
    s16 ret;

    switch (arg0) {
        case 0:
            ret = 8 - D_802AFF60[0];
            break;
        case 1:
            ret = 12 - D_802AFF60[1];
            break;
        case 2:
            ret = 4 - D_802AFF60[2];
            break;
        case 4:
            ret = 3 - D_802AFF60[4];
            break;
        case 5:
            ret = 7 - D_802AFF60[5];
            break;
        case 6:
            ret = 1 - D_802AFF60[6];
            break;
        default:
            break;
    }
    return ret;
}

s16 func_8027A4E8(s16 arg0) {
    s32 temp = func_8027911C(arg0);
    s16 ret = temp;

    switch (temp) {
        case 9:
            ret = 0x19;
            break;
        case 0x39:
            ret = 0x1A;
            break;
        case 0x3F:
            ret = 0x1B;
            break;
        case 0x45:
            ret = 0x1C;
            break;
        case 0x4B:
            ret = 0x1D;
            break;
        default:
            break;
    }
    return ret;
}

UnkStruct_800BB4A8 *func_8027A4A0(void) {
    UnkStruct_800BB4A8 *end = &D_800BB4A0[48];
    UnkStruct_800BB4A8 *node;

    for (node = &D_800BB4A0[2]; node < end; node++) {
        if (node->unk2 == 1) {
            return node;
        }
    }
    return NULL;
}

void func_8027A478(UnkStruct_800BB4A8 *arg0) {
    UnkStruct_800BB4A8 *last = D_800BB4A0[1].unkC;

    D_800BB4A0[1].unkC = arg0;
    last->unk8 = arg0;
    arg0->unk8 = &D_800BB4A0[1];
    arg0->unkC = last;
}

void func_8027A380(s16 arg0) {
    switch (arg0) {
        case 0:
            D_802AFF60[0]++;
            break;
        case 1:
            D_802AFF60[1]++;
            break;
        case 2:
            D_802AFF60[2]++;
            break;
        case 3:
            D_802AFF60[3]++;
            break;
        case 4:
            D_802AFF60[4]++;
            break;
        case 5:
            D_802AFF60[5]++;
            break;
        case 6:
            D_802AFF60[6]++;
            break;
        default:
            break;
    }
}

void func_8027A264(UnkStruct_800BB4A8 *arg0, s16 arg1) {
    UnkStructA *obj = arg0->unk14;
    UnkStruct_8027A264 *ptr;

    if (!(obj->unk8 & 0x20000)) {
        ptr = malloc(sizeof(UnkStruct_8027A264));
        ptr->unk4 = obj;
        ptr->unk14 = func_8026FFA0(arg1);
        ptr->unk18 = 25.0f;
        ptr->unk1C = 7.7f;
        ptr->unk20 = 1;
        ptr->unk24 = 0.0f;
        ptr->unk28 = 0;
        ptr->unk2C = 10000.0f;
        ptr->unk30 = 0;
        ptr->unk34 = obj->unk40->unk10;
        ptr->unk38 = obj->unk40->unk18;
        obj->unk68 = 25.0f;
        obj->unk6C = 1.0f;
        ptr->unk10 = obj->unk8;
        if (func_8027A1F4(obj, (PORT_IP(s32))ptr)) {
            func_80270080(ptr);
            func_80270354(&arg0->unk54, 0x20);
            func_80231408(obj, 0x20000);
            ptr->unk8 = obj;
            ptr->unkC = obj;
        } else {
            free(ptr);
        }
    }
}

s32 func_8027A1F4(UnkStructA *arg0, PORT_IP(s32) arg1) {
    UnkStruct_800BB4A8 *node = func_8027A1A0(arg0);

    func_80231408(arg0, 0x20000);
    func_8027036C(&node->unk54, 2);
    func_80270354(&node->unk54, 8);
    node->unk18 = arg1;
    func_80270354(&node->unk54, 1);
    return 1;
}

UnkStruct_800BB4A8 *func_8027A1A0(UnkStructA *arg0) {
    UnkStruct_800BB4A8 *node = D_800BB4A8;

    while (node->unk8 != NULL) {
        if (arg0 == node->unk14) {
            return node;
        }
        node = node->unk8;
    }
    return NULL;
}
