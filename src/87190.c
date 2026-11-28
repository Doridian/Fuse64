#include "common.h"
#include "87190.h"

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ void *unk4;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ s32 unkC;
} UnkStruct_800A7F30; /* size 0x10 */

void free(void *);
void *malloc(s32);

extern UnkStruct_800A7F30 D_800A7F30[100];
/* ZeroJump module 20's table (func_8026C990). The zeros after the
   functions end it, as 87D80's tables' do (there they can't be padding),
   though here they also fill the section to 16. */
void *D_802A13D0[] = { func_8026CE28, func_8026CCBC, NULL, NULL };

s32 D_802AFC30;

void func_8026C990(void) {
    s32 i;

    func_80297D30(20, D_802A13D0);
    for (i = 0; i < 100; i++) {
        D_800A7F30[i].unk0 = -1;
        D_800A7F30[i].unk4 = (void *) -1;
        D_800A7F30[i].unk8 = (u32) -1;
        D_800A7F30[i].unkC = 0;
    }
    D_802AFC30 = 0;
}

s32 func_8026CA20(s32 arg0) {
    s32 i;

    for (i = 0; i < 100; i++) {
        if (arg0 == (PORT_IP(s32)) D_800A7F30[i].unk4 && D_800A7F30[i].unk0 != -1 && D_800A7F30[i].unkC != 0) {
            return D_800A7F30[i].unk0;
        }
    }
    return -1;
}

void func_8026CB2C(void) {
    s32 i;

    for (i = 0; i < 100; i++) {
        if (D_800A7F30[i].unk0 != -1 && D_800A7F30[i].unkC == 0) {
            free(D_800A7F30[i].unk4);
            D_800A7F30[i].unk4 = (void *) -1;
            D_800A7F30[i].unk0 = -1;
            D_802AFC30--;
        }
    }
}

void func_8026CBD4(void *arg0) {
    s32 i;

    for (i = 0; i < 100; i++) {
        if (arg0 == D_800A7F30[i].unk4 && D_800A7F30[i].unk0 != -1 && D_800A7F30[i].unkC != 0) {
            D_800A7F30[i].unkC--;
            break;
        }
    }
}

s32 func_8026CC34(s32 arg0) {
    s32 i;

    for (i = 0; i < 100; i++) {
        if (arg0 == D_800A7F30[i].unk0) {
            return i;
        }
    }
    return -1;
}

void func_8026CCBC(s32 arg0) {
    s32 i;

    if (arg0 != -1) {
        i = func_8026CC34(arg0);
        if (i != -1) {
            if (arg0 == D_800A7F30[i].unk0) {
                if (D_800A7F30[i].unkC != 0) {
                    D_800A7F30[i].unkC--;
                }
            }
        }
    }
}

s32 func_8026CD24(s32 arg0) {
    s32 i;

    for (i = 0; i < 100; i++) {
        if (arg0 == D_800A7F30[i].unk0) {
            return i;
        }
    }
    for (i = 0; i < 100; i++) {
        if (D_800A7F30[i].unk0 == -1 && D_800A7F30[i].unk4 == (void *) -1) {
            return i;
        }
    }
    return -1;
}

void *func_8026CE28(s32 arg0) {
    s32 i;
    s32 size;
    struct UnkStruct_80225CC0 *file;

    if (arg0 == -1) {
        return (void *) -1;
    }
    i = func_8026CD24(arg0);
    if (arg0 != D_800A7F30[i].unk0) {
        D_802AFC30++;
        D_800A7F30[i].unk0 = arg0;
        func_802267E0(0x18);
        file = func_80226604(arg0, 1);
        func_8022616C((u8 *) &size, 4, 1, file);
#if defined(TARGET_PC) && defined(PORT_HOST_MALLOC)
        /* PORT_ASSET_PAD bytes past the file, what the N64 has after it: a
         * picture ends with its palette, which a TLUT load reads 16 or 256
         * entries of whatever its count (port/src/native.c).  Only with the
         * host's malloc: in gmalloc's arena the block is the N64's size and
         * the bytes past it the heap's own, as on the N64 (a longer block
         * used the arena up sooner: jp's 116:28 and 126:30 with random
         * input ran out of it where the N64 didn't) */
        D_800A7F30[i].unk4 = malloc(size + PORT_ASSET_PAD);
        port_asset_tail(arg0, D_800A7F30[i].unk4, size);
#else
        D_800A7F30[i].unk4 = malloc(size);
#endif
        func_80292BD0(file, D_800A7F30[i].unk4, size);
#ifdef TARGET_PC
        /* the file in the host's byte order, by what it is */
        port_asset_native(arg0, D_800A7F30[i].unk4, size);
#endif
        func_80226368(file);
    }
    D_800A7F30[i].unkC++;
    return D_800A7F30[i].unk4;
}
