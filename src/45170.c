/*
 * main_code's 45170, IDO -O3; it matches in us, eu and jp: func_8022AF1C
 * reads a model file's chunks 1, 2, 3 and 7 into a skeleton,
 * func_8022AEDC attaches one to an instance (with index tables of its
 * key sets and channels), and the rest free one. Every function is
 * written last function first (-O3 emits a file's functions in
 * reverse). 46300 after it is the node tree and the animation tracks
 * (src/46300.c). Its .data (D_8029F880) and .bss
 * (D_802A54A0..AC, and func_8022AF1C's statics D_802A54B0..C8) are its
 * own, placed in data_split.txt.
 *
 * func_8022AF1C keeps its globals as function-scope statics: only then
 * does a store to one not kill pointer loads, and each access gets its
 * own lui.
 * D_802A54B0 is s32 against arg0->unk4's u32 on purpose: the unsigned
 * compare is what keeps the counter's store-then-reload.
 */
#include "common.h"
#include "model.h"
#include "45170.h"

void *malloc(u32);
void free(void *);
void *realloc(void *, u32);

s32 *D_8029F880 = NULL;
s32 D_802A54A0;
s32 D_802A54A4;
s32 D_802A54A8;
s32 D_802A54AC;

void func_8022A970(UnkChan *arg0, s32 arg1);
void func_8022AA50(UnkSkel *arg0);
void func_8022AAD4(UnkKey *arg0, s32 arg1);
void func_8022AB48(UnkSkel *arg0);
void func_8022AC2C(UnkSkel *arg0, UnkInst *arg1);
void func_8022AD98(UnkSkel *arg0, UnkInst *arg1);
void func_8022AED0(UnkSkel *arg0, UnkInst *arg1);

UnkSkel *func_8022AF1C(UnkModel *arg0) {
    static s32 D_802A54B0;
    static s32 D_802A54B4;
    static s32 D_802A54B8;
    static UnkSkel *D_802A54BC;
    static s32 *D_802A54C0;
    static f32 *D_802A54C4;
    static UnkChunk *D_802A54C8;
    UnkKey *key;
    UnkChan *chan;

    D_802A54C8 = arg0->unkC;
    D_802A54BC = malloc(sizeof(UnkSkel));
    D_802A54BC->unk1C = arg0->unk4;
    D_802A54BC->unk30 = NULL;
    D_802A54BC->unk28 = NULL;
    for (D_802A54B0 = 0; D_802A54B0 < arg0->unk4; D_802A54B0++) {
        if (D_802A54C8[D_802A54B0].unk0 == 1) {
            break;
        }
    }
    if (D_802A54B0 < arg0->unk4) {
        D_802A54BC->unk20 = (UnkJoint *)((u8 *)arg0 + D_802A54C8[D_802A54B0].unk8);
    }
    for (D_802A54B0 = 0; D_802A54B0 < arg0->unk4; D_802A54B0++) {
        if (D_802A54C8[D_802A54B0].unk0 == 2) {
            break;
        }
    }
    if (D_802A54B0 < arg0->unk4) {
        D_802A54C0 = (s32 *)((u8 *)arg0 + D_802A54C8[D_802A54B0].unk8);
        D_802A54BC->unk2C = D_802A54C8[D_802A54B0].unk4;
        D_802A54BC->unk28 = malloc(D_802A54BC->unk2C * sizeof(UnkKeySet));
        for (D_802A54B0 = 0; D_802A54B0 < D_802A54BC->unk2C; D_802A54B0++) {
            D_802A54BC->unk28[D_802A54B0].unk0 = *D_802A54C0++;
            D_802A54BC->unk28[D_802A54B0].unk8 = *D_802A54C0++;
            D_802A54BC->unk28[D_802A54B0].unk4 = malloc(D_802A54BC->unk28[D_802A54B0].unk8 * sizeof(UnkKey));
            for (D_802A54B4 = 0; D_802A54B4 < D_802A54BC->unk28[D_802A54B0].unk8; D_802A54B4++) {
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk4 = *D_802A54C0++;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk0 = malloc(D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk4 * 4);
                for (D_802A54B8 = 0; D_802A54B8 < D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk4; D_802A54B8++) {
                    D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk0[D_802A54B8] = *D_802A54C0++;
                }
                D_802A54C4 = (f32 *)D_802A54C0;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk8.x = *D_802A54C4++;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk8.y = *D_802A54C4++;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk8.z = *D_802A54C4++;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk14.x = *D_802A54C4++;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk14.y = *D_802A54C4++;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk14.z = *D_802A54C4++;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk20.x = *D_802A54C4++;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk20.y = *D_802A54C4++;
                D_802A54BC->unk28[D_802A54B0].unk4[D_802A54B4].unk20.z = *D_802A54C4++;
                D_802A54C0 = (s32 *)D_802A54C4;
            }
        }
    } else {
        D_802A54BC->unk2C = 0;
    }
    for (D_802A54B0 = 0; D_802A54B0 < arg0->unk4; D_802A54B0++) {
        if (D_802A54C8[D_802A54B0].unk0 == 3) {
            break;
        }
    }
    if (D_802A54B0 < arg0->unk4) {
        D_802A54C0 = (s32 *)((u8 *)arg0 + D_802A54C8[D_802A54B0].unk8);
        D_802A54BC->unk34 = D_802A54C8[D_802A54B0].unk4;
        D_802A54BC->unk30 = malloc(D_802A54BC->unk34 * sizeof(UnkChanSet));
        for (D_802A54B0 = 0; D_802A54B0 < D_802A54BC->unk34; D_802A54B0++) {
            D_802A54BC->unk30[D_802A54B0].unk0 = *D_802A54C0++;
            D_802A54BC->unk30[D_802A54B0].unk8 = *D_802A54C0++;
            D_802A54BC->unk30[D_802A54B0].unkC = *D_802A54C0++;
            D_802A54BC->unk30[D_802A54B0].unk4 = malloc(D_802A54BC->unk30[D_802A54B0].unk8 * sizeof(UnkChan));
            for (D_802A54B4 = 0; D_802A54B4 < D_802A54BC->unk30[D_802A54B0].unk8; D_802A54B4++) {
                if (*D_802A54C0 == 0x2F) {
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk0 = *D_802A54C0++;
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk8 = *D_802A54C0++;
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk4 = *D_802A54C0++;
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unkC = malloc(D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk8 * 4);
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk10 = malloc(D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk8 * 4);
                    for (D_802A54B8 = 0; D_802A54B8 < D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk8; D_802A54B8++) {
                        D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unkC[D_802A54B8] = *D_802A54C0++;
                        D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk10[D_802A54B8] = *D_802A54C0++;
                    }
                } else if (*D_802A54C0 == 0x30) {
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk0 = *D_802A54C0++;
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk4 = *D_802A54C0++;
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk8 = *D_802A54C0++;
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk14 = *D_802A54C0++;
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unkC = (s32 *)*D_802A54C0++;
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk10 = (s32 *)0x31;
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk18 = malloc(D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk14 * 4);
                    D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk1C = malloc(D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk14 * 4);
                    for (D_802A54B8 = 0; D_802A54B8 < D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk14; D_802A54B8++) {
                        D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk18[D_802A54B8] = *D_802A54C0++;
                        D_802A54BC->unk30[D_802A54B0].unk4[D_802A54B4].unk1C[D_802A54B8] = *(f32 *)D_802A54C0++;
                    }
                }
            }
        }
    } else {
        D_802A54BC->unk34 = 0;
    }
    for (D_802A54B0 = 0; D_802A54B0 < arg0->unk4; D_802A54B0++) {
        if (D_802A54C8[D_802A54B0].unk0 == 7) {
            break;
        }
    }
    if (D_802A54B0 < arg0->unk4 && D_802A54C8[D_802A54B0].unk4 == 4) {
        s32 *p = (s32 *)((u8 *)arg0 + D_802A54C8[D_802A54B0].unk8);
        s32 *q = p + 3;
        for (D_802A54B0 = 0; D_802A54B0 != 3; D_802A54B0++) {
            D_802A54BC->unk0[D_802A54B0] = p[D_802A54B0];
            D_802A54BC->unkC[D_802A54B0] = q[D_802A54B0];
        }
    }
    return D_802A54BC;
}

void func_8022AEDC(UnkSkel *arg0, UnkInst *arg1) {
    func_8022AED0(arg0, arg1);
    func_8022AD98(arg0, arg1);
    func_8022AC2C(arg0, arg1);
}

void func_8022AED0(UnkSkel *arg0, UnkInst *arg1) {
    arg1->unk2C = arg0->unk20;
}

void func_8022AD98(UnkSkel *arg0, UnkInst *arg1) {
    s32 i;
    s32 j;

    arg1->unk30 = arg0->unk28;
    D_802A54A0 = arg0->unk2C;
    D_802A54A8 = 0;
    if (D_802A54A0 != 0) {
        for (i = 0; i < D_802A54A0; i++) {
            if (D_802A54A8 < arg1->unk30[i].unk0) {
                D_802A54A8 = arg1->unk30[i].unk0;
            }
        }
        D_802A54A8++;
        arg1->unk38 = malloc(D_802A54A8 * 4);
        for (i = 0; i < D_802A54A8; i++) {
            for (j = D_802A54A0 - 1; j >= 0; j--) {
                if (i == arg1->unk30[j].unk0) {
                    break;
                }
            }
            arg1->unk38[i] = j;
        }
    }
}

void func_8022AC2C(UnkSkel *arg0, UnkInst *arg1) {
    s32 i;
    s32 j;

    arg1->unk34 = arg0->unk30;
    D_802A54A4 = arg0->unk34;
    if (D_8029F880 != NULL) {
        free(D_8029F880);
    }
    D_802A54AC = 0;
    D_8029F880 = NULL;
    if (D_802A54A4 != 0) {
        for (i = 0; i < D_802A54A4; i++) {
            if (D_802A54AC < arg1->unk34[i].unk0) {
                D_802A54AC = arg1->unk34[i].unk0;
            }
        }
        D_802A54AC++;
        D_8029F880 = malloc(D_802A54AC * 4);
        for (i = 0; i < D_802A54AC; i++) {
            for (j = D_802A54A4 - 1; j >= 0; j--) {
                if (i == arg1->unk34[j].unk0) {
                    break;
                }
            }
            D_8029F880[i] = j;
        }
    }
}

void func_8022ABCC(UnkSkel *arg0) {
    if (arg0->unk28 != NULL) {
        func_8022AB48(arg0);
    }
    if (arg0->unk30 != NULL) {
        func_8022AA50(arg0);
    }
    free(arg0);
}

void func_8022AB48(UnkSkel *arg0) {
    s32 i;
    UnkKeySet *p;

    for (i = 0, p = arg0->unk28; i < arg0->unk2C; i++) {
        func_8022AAD4(p[i].unk4, p[i].unk8);
    }
    free(arg0->unk28);
    arg0->unk28 = NULL;
    arg0->unk2C = 0;
}

void func_8022AAD4(UnkKey *arg0, s32 arg1) {
    s32 i;

    for (i = 0; i < arg1; i++) {
        if (arg0[i].unk4 != 0) {
            free(arg0[i].unk0);
        }
    }
    free(arg0);
}

void func_8022AA50(UnkSkel *arg0) {
    s32 i;
    UnkChanSet *p;

    for (i = 0, p = arg0->unk30; i < arg0->unk34; i++) {
        func_8022A970(p[i].unk4, p[i].unk8);
    }
    free(arg0->unk30);
    arg0->unk30 = NULL;
    arg0->unk34 = 0;
}

void func_8022A970(UnkChan *arg0, s32 arg1) {
    s32 i;

    for (i = 0; i < arg1; i++) {
        if (arg0[i].unk0 == 0x2F) {
            if (arg0[i].unk8 != 0) {
                free(arg0[i].unkC);
                free(arg0[i].unk10);
            }
        } else if (arg0[i].unk0 == 0x30 && arg0[i].unk14 != 0) {
            free(arg0[i].unk18);
            free(arg0[i].unk1C);
        }
    }
    free(arg0);
}
