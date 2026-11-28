#include "common.h"
#include "40FB0.h"
#include "404C0.h"
#include "AD3D0.h"

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
} UnkStruct_8019B0D0; /* size 0x2 */

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ struct {
        /* 0x00 */ u8 unk0;
        /* 0x01 */ u8 unk1;
        /* 0x02 */ u8 unk2;
#ifdef TARGET_PC
    } unk1[(0x210 - 1) / 3]; /* as many as the N64's room up to D_8019C2E0 holds */
#else
    } unk1[1];
#endif
} UnkStruct_8019C0D0;

void func_802267B0(u32 arg0);
void func_80226818(void);
void func_80297D50(void *, s32);
void func_80297D58(void *, s32);

extern UnkStruct_8019B0D0 D_8019B0D0[PORT_SIZE(0x800)]; /* ROM 0x30800's 0x1000 bytes */
extern UnkStruct_8019C0D0 D_8019C0D0;
void func_802268F4(); /* (s32) on TARGET_PC, below */

/* ZeroJump module 11's table (func_80226860); its .data's other two words
   are zeros, taken here for the section's padding */
void *D_8029F5D0[] = { func_8022691C, func_802268F4 };

s32 D_802A5320;
u32 D_802A5324;
s32 D_802A5328;
PORT_IP(s32) D_802A532C;

#ifdef TARGET_PC
/* the N64's overlay slot, 0x80043000's 0x20000 bytes, as it holds them
   (port/src/overlay.c) */
extern u8 port_ovl_slot[0x20000];
#endif

void func_8022691C(s32 arg0) {
    struct UnkStruct_80225CC0 *file;
    s32 size;
    s32 id;
    void *ovl = (void *) 0x80043000; /* fake: where every overlay runs (its frame slot; a guess among held values) */

    if (D_802A5328 != arg0) {
        D_802A5328 = arg0;
#ifdef TARGET_PC
        if ((u32) arg0 >= sizeof(D_8019B0D0) / sizeof(D_8019B0D0[0])) {
            port_scene_unset(arg0);
        }
#endif
        func_802267B0((D_8019B0D0[arg0].unk0 << 17) + (D_8019B0D0[arg0].unk1 << 11));
        file = func_80226604(0, 1);
        size = func_802266DC(file);
        func_8022616C((u8 *) &D_8019C0D0, 1, size, file);
        for (size = 0; size < D_8019C0D0.unk0; size++) {
            id = (D_8019C0D0.unk1[size].unk0 << 8) + D_8019C0D0.unk1[size].unk1;
            if (id == arg0) {
                id = D_8019C0D0.unk1[size].unk2;
                break;
            }
        }
        func_80226368(file);
        file = func_80226604(id, 1);
        func_8022616C((u8 *) &size, 4, 1, file);
#ifdef TARGET_PC
        /* the overlay's code is linked in (port/src/overlay.c): it is
         * inflated all the same, for the time it takes and what it leaves
         * in the archive's buffers, into the port's copy of the N64's
         * overlays' slot at 0x80043000 (0x20000 bytes): what the image
         * leaves past its end is the .bss's start (port/src/overlay.c) */
        func_80292BD0(file, port_ovl_slot, size);
        func_80226368(file);
        func_802267E0(0x18);
        D_802A532C = (PORT_IP(s32)) port_overlay_load(D_8019B0D0[arg0].unk0, id, size);
#else
        func_80297D58(ovl, 0x20000);
        func_80297D50(ovl, 0x20000);
        func_80292BD0(file, ovl, size);
        func_80297D60(ovl, 0x20000);
        func_80226368(file);
        func_802267E0(0x18);
        D_802A532C = ((s32 (*)(void)) ovl)();
#endif
    }
}

#ifdef TARGET_PC
/* The overlay's main function takes the message its callers (54E40.c)
 * pass here: IDO leaves it in $a0 for the call. */
void func_802268F4(s32 arg0) {
    ((void (*)(s32)) D_802A532C)(arg0);
}
#else
void func_802268F4(void) {
    ((void (*)(void)) D_802A532C)();
}
#endif

void func_80226860(void) {
    func_80297D30(11, D_8029F5D0);
    D_802A5320 = -1;
    D_802A5324 = -1;
    D_802A5328 = -1;
    func_80226818();
    func_802267E0(0x18);
}

void func_80226818(void) {
    if (D_802A5320 == -1) {
        func_80297EC0(D_8019B0D0, 0x1000, 0x30800);
        D_802A5320 = 0;
    }
}

void func_802267E0(s32 arg0) {
    func_802267B0(arg0 << 17);
}

void func_802267B0(u32 arg0) {
    if (D_802A5324 != arg0) {
        D_802A5324 = arg0;
        func_802266E8(arg0);
    }
}
