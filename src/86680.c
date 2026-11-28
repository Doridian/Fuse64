#include "common.h"

typedef struct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
} UnkStruct_800BD778; /* size 0x14 */

void func_8022FEAC(Vec3f *, Vec3f *);
void func_8022FF58(Vec3f *, Vec3f *, Vec3f *);
void func_80234858(f32 *, f32 *, f32 *);
void func_802351DC(Vec3f *);
UnkStruct_802AC610 *func_8023A840(void);
void func_80265DD8(s8);
void func_80266118(s8);
void func_80266510(s16, s8);
void func_80266644(s16, s16, f32);
void func_802668D4(s16);
void func_80266930(s16);
void func_80266B50(void);
PORT_RET(s32, u8) func_80266CD4(s16);
PORT_RET(s32, s16) func_8026724C(s16);
PORT_RET(s32, s16) func_802675F4(s16, s16, f32);
void func_80267654(s32, f32, f32, f32, f32);
void func_80267730(s32, s32, s32, s32);
void func_802690DC(s32, s32, s32, s32, s32);
void func_80268D94(s8);
void func_80268E7C(s16);
void func_80268EE4(void);
PORT_RET(void, s32) func_80268F94(s16);
PORT_RET(s32, s16) func_8026A754(s32, s32, void *, void *, s32);
f32 sqrtf(f32);

static void func_8026C0A8(Vec3f *pos, f32 *angle, f32 *dist);
static s32 func_8026C494(s32 id);
static void func_8026C774(void);
s32 func_8026C51C(s32 arg0, f32 x, f32 y, f32 z);
void func_8026C6D0(s32 arg0);
void func_8026BE80(s32 arg0);
void func_8026BEB0(void);
void func_8026BFD0(s32 arg0, s8 arg1);

extern s32 D_800BD770[PORT_SIZE(1)];
extern s32 D_800BD774[PORT_SIZE(1)];
extern UnkStruct_800BD778 D_800BD778[PORT_SIZE(8)];
extern u8 D_8019C2E0[PORT_SIZE(300000)]; /* the audio heap */


extern u8 D_802A89B0[];
extern s32 D_802AC7CC;
s32 D_802AFC20;
s32 D_802AFC24;
f32 D_802AFC28;

void func_80269704(void);
s32 func_8026C660(s32 arg0);
void func_8026C964(void);
void func_8026C93C(s32 arg0);
void func_8026C8E8(void);
s32 func_8026C630(s32 arg0, Vec3f *arg1);
void func_8026BF90(s8 arg0);
void func_8026BF68(s8 arg0);
void func_8026C454(s32 arg0, f32 x, f32 y, f32 z);
void func_8026C40C(s32 arg0, Vec3f *arg1);
void func_8026BF28(s8 arg0);
void func_8026C894(void);

void *D_802A1380[] = {
    func_8026C660, func_8026C964, func_8026C93C, func_8026C8E8, func_8026C51C,
    func_8026C630, func_8026BE80, func_8026BFD0, func_8026BF90, func_8026BF68,
    func_8026C454, func_8026C40C, func_8026BF28, func_8026C894, func_80269704,
};
s32 D_802A13BC[] = { 60 };

void func_8026C964(void) {
    func_80268EE4();
}

void func_8026C93C(s32 arg0) {
    func_80268F94(arg0);
}

void func_8026C8E8(void) {
    D_802AFC20 = 1;
    if (D_802AC7CC < 5) {
        func_80268EE4();
    } else {
        func_80268E7C(D_802AC7CC - 4);
    }
}

void func_8026C894(void) {
    D_802AFC24 = 1;
    if (D_802AC7CC < 5) {
        func_80266B50();
    } else {
        func_802668D4(D_802AC7CC - 4);
    }
}

void func_8026C77C(void) {
    struct UnkStruct_80225CC0 *file;
    s32 sp28;
    s32 sp24;
    s32 i;

    func_80297D30(0x2B, D_802A1380);
    func_802267E0(0x18);
    file = func_80226604(0x20, 1);
    sp28 = file->unk4;
    func_80226368(file);
    file = func_80226604(0x21, 1);
    sp24 = file->unk4;
    func_80226368(file);
    func_802690DC(sp28, 0, 0, 0, 0);
    func_80267730(sp24, 0, 0, 0);
#ifdef TARGET_PC
    {
        /* (where pointers are 8 bytes the heap's structures are bigger: a
           heap twice the size, port/src/memory.c) */
        s32 len = 300000;
        u8 *heap = port_audio_heap(D_8019C2E0, &len);

        func_8026A754(0x7E, 4, D_802A89B0, heap, len);
    }
#else
    func_8026A754(0x7E, 4, D_802A89B0, D_8019C2E0, 300000);
#endif
    for (i = 0; i < 8; i++) {
        D_800BD778[i].unk10 = -1;
    }
    for (i = 0; i < 1; i++) {
        D_800BD770[i] = 0;
    }
    func_8026C774();
    func_8026C6D0(0);
    func_80265DD8(0);
    D_802AFC20 = 0;
    D_802AFC24 = 0;
}

/* unnamed in the asm: a static function inlined into func_8026C77C; IDO -O3
 * still emits it, empty (uopt reports it "deleted").  Its body is unknown. */
static void func_8026C774(void) {
}

void func_8026C6D0(s32 arg0) {
    switch (arg0) {
        case 0:
            func_80267654(0, 300.0f, 4000.0f, 300.0f, 1200.0f);
            D_802AFC28 = 1300.0f;
            break;
        case 1:
            func_80267654(0, 300.0f, 5000.0f, 300.0f, 1200.0f);
            D_802AFC28 = 1700.0f;
            break;
    }
}

s32 func_8026C660(s32 arg0) {
    s32 idx;

    if (func_8026C494(arg0) != 0) {
        idx = func_8026724C(arg0);
        if (idx != -1) {
            D_800BD778[idx].unkC = 0;
            D_800BD778[idx].unk10 = arg0;
            return idx;
        }
    }
    return -1;
}

s32 func_8026C630(s32 arg0, Vec3f *arg1) {
    return func_8026C51C(arg0, arg1->x, arg1->y, arg1->z);
}

s32 func_8026C51C(s32 arg0, f32 x, f32 y, f32 z) {
    f32 dist;
    f32 angle;
    s32 idx;
    Vec3f pos;

    if (func_8026C494(arg0) != 0) {
        pos.x = x;
        pos.y = y;
        pos.z = z;
        func_8026C0A8(&pos, &angle, &dist);
        idx = func_802675F4(arg0, angle, dist);
        if (idx != -1) {
            D_800BD778[idx].unkC = 1;
            D_800BD778[idx].unk0 = x;
            D_800BD778[idx].unk4 = y;
            D_800BD778[idx].unk8 = z;
            D_800BD778[idx].unk10 = arg0;
            return idx;
        }
    }
    return -1;
}

static s32 func_8026C494(s32 id) {
    s32 idx;

    if (!(func_8023A840()->unk0 & 0xA)) {
        switch (id) {
            case 0x26:
                idx = 0;
                break;
            default:
                idx = -1;
                break;
        }
        if (idx == -1) {
            return 1;
        }
        if (D_800BD770[idx] == 0) {
            D_800BD770[idx] = D_802A13BC[idx];
            return 1;
        }
    }
    return 0;
}

void func_8026C454(s32 arg0, f32 x, f32 y, f32 z) {
    if (arg0 != -1) {
        D_800BD778[arg0].unk0 = x;
        D_800BD778[arg0].unk4 = y;
        D_800BD778[arg0].unk8 = z;
    }
}

void func_8026C40C(s32 arg0, Vec3f *arg1) {
    if (arg0 != -1) {
        D_800BD778[arg0].unk0 = arg1->x;
        D_800BD778[arg0].unk4 = arg1->y;
        D_800BD778[arg0].unk8 = arg1->z;
    }
}

void func_8026C394(void) {
    s32 i;

    for (i = 0; i < 8; i++) {
        if (D_800BD778[i].unk10 != -1) {
            D_800BD778[i].unkC = 0;
        }
    }
}

/* The counters are this function's statics: as globals (extern or
 * defined) IDO -O3 keeps their addresses in registers, as local statics it
 * reaches them with lui/%lo on every access, as the original does. */
void func_8026C208(void) {
    f32 dist;
    f32 angle;
    s32 i;
    s16 id;
    static s32 D_802A13C0 = 0;
    static s32 D_802A13C4 = 0;

    D_802A13C4++;
    for (i = 0; i < 8; i++) {
        id = i;
        if (D_800BD778[i].unk10 != -1) {
            if (func_80266CD4(id) == 0) {
                D_800BD778[i].unk10 = -1;
            } else if (i == D_802A13C0 && D_800BD778[i].unkC != 0) {
                func_8026C0A8((Vec3f *) &D_800BD778[i], &angle, &dist);
                func_80266644(id, angle, dist);
            }
        }
    }
    D_802A13C0++;
    if (D_802A13C0 >= 8) {
        D_802A13C0 = 0;
    }
    for (i = 0; i < 1; i++) {
        if (D_800BD770[i] != 0) {
            D_800BD770[i]--;
        }
    }
}

/* Static: IDO -O3 passes pos/dist/angle in $s0/$s1/$s2 and uses
 * $s3/$s4/$f20 unsaved (the callers save them).  The wrap is applied to the
 * expression itself: through a temporary, the result goes to $f2 by a
 * `mov.s`. */
#define WRAP(x) ((x) >= 360.0f ? (x) - 360.0f : (x) < 0.0f ? (x) + 360.0f : (x))
static void func_8026C0A8(Vec3f *pos, f32 *angle, f32 *dist) {
    Vec3f sp54;
    Vec3f sp48;
    Vec3f sp3C;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 sp2C;
    f32 sp28;
    f32 sp24;

    func_802351DC(&sp54);
    func_80234858(&sp2C, &sp28, &sp24);
    sp3C.x = sp2C;
    sp3C.y = 0.0f;
    sp3C.z = 300.0f;
    func_8022FEAC(&sp3C, &sp48);
    sp54.x += sp48.x;
    sp54.y += sp48.y - 100.0f;
    sp54.z += sp48.z;
    func_8022FF58(&sp54, pos, &sp3C);
    dx = pos->x - sp54.x;
    dy = pos->y - sp54.y;
    dz = pos->z - sp54.z;
    *dist = sqrtf(dx * dx + dy * dy + dz * dz);
    if (*dist > D_802AFC28) {
        *dist = D_802AFC28;
    }
    *angle = WRAP(270.0f - sp3C.x + sp2C);
}

void func_8026C00C(s8 arg0) {
    s32 i;

    for (i = 0; i < 8; i++) {
        if (D_800BD778[i].unk10 != -1) {
            func_8026BFD0(i, arg0);
        }
    }
}

void func_8026BFD0(s32 arg0, s8 arg1) {
    if (arg0 != -1) {
        func_80266510(arg0, arg1);
    }
}

void func_8026BF90(s8 arg0) {
    if (D_802AFC20 != 0) {
        func_80268EE4();
    }
    func_80268D94(arg0);
    D_802AFC20 = 0;
}

void func_8026BF68(s8 arg0) {
    func_80265DD8(arg0);
}

void func_8026BF28(s8 arg0) {
    if (D_802AFC24 != 0) {
        func_8026BEB0();
    }
    func_80266118(arg0);
    D_802AFC24 = 0;
}

void func_8026BEB0(void) {
    s32 i;

    for (i = 0; i < 8; i++) {
        if (D_800BD778[i].unk10 != -1) {
            func_8026BE80(i);
        }
    }
}

void func_8026BE80(s32 arg0) {
    if (arg0 != -1) {
        func_80266930(arg0);
    }
}
