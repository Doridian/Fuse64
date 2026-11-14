/*
 * Without STATICS_PART: the statics below, included before the functions.
 * With it: the runs of code (definitions, or statements inside a function)
 * 202690.c includes where they are, from this same file: the `.file`
 * switches around them end as1's scheduling blocks, as the original's do.
 */
#ifndef STATICS_PART
/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0F_09_80043390(void) {
    D_ovl0F_09_80046200[4]->unk40->unk14 -= 3200.0f;
    D_ovl0F_09_80046200[5]->unk40->unk14 -= 3200.0f;
    D_ovl0F_09_80046200[6]->unk40->unk10 += 50.0f;
}
#elif STATICS_PART == 1
static void func_ovl0F_09_80043CC8(s32 *dst, f32 x, f32 z) {
    *dst = ((s16)(x * 500.0f) << 16) | ((s16)(z * 500.0f) & 0xFFFF);
}

static s32 func_ovl0F_09_80043CD0(UnkStructA *arg0) {
    s32 hit;
    s32 saved;
    UnkStructA *other;
    s32 id;

    hit = 0;
    other = NULL;
    saved = arg0->unkC;
    arg0->unkC = 0;
    if (arg0->unk8 & 1) {
        other = func_ovl0F_09_80044ED8(func_ovl0F_09_80044E38());
    } else if (arg0->unk8 & 2) {
        id = func_ovl0F_09_80044E38();
        if (func_ovl0F_09_80044ED8(id) == arg0) {
            other = func_ovl0F_09_80044E98(id);
        } else {
            other = NULL;
        }
    }
    if (other != NULL) {
        if (func_ovl0F_09_80045778(arg0, 0x10, other) != 0) {
            hit = 1;
        }
    } else if (func_ovl0F_09_80045608(arg0, 0x10) != 0) {
        hit = 1;
    }
    arg0->unkC = saved;
    return hit;
}

static void func_ovl0F_09_80043CD8(f32 arg0) {
    if (D_ovl0F_09_80045F5C != 0) {
        if (D_ovl0F_09_80045F58 == 0) {
            if (arg0 == 0.0f) {
                func_ovl0F_09_80045D30(0x32);
            } else {
                func_ovl0F_09_80045D30(0x27);
            }
            D_ovl0F_09_80045F58 = 20;
            func_ovl0F_09_80045BF8(0x19, D_ovl0F_09_80045F60->unk40->unk10, D_ovl0F_09_80045F60->unk40->unk14,
                                   D_ovl0F_09_80045F60->unk40->unk18, 0);
        } else {
            D_ovl0F_09_80045F58--;
        }
    }
}








static void func_ovl0F_09_80043434(void) {
    s32 count;
    u32 (*p)[3];
    s32 i;

    D_ovl0F_09_80046260 = -1;
    func_ovl0F_09_80045B10(D_ovl0F_09_80045F6C);
    p = D_ovl0F_09_80045F90;
    count = 4;
    for (i = 0; i < count; i++) {
        func_ovl0F_09_80045CB8((*p)[0], (*p)[1], (*p)[2]);
        p++;
    }
}

static void func_ovl0F_09_8004342C(void) {
    func_ovl0F_09_80044C58(0x230);
    func_ovl0F_09_80044C70(0x23F);
    func_ovl0F_09_80044E08();
}

static void func_ovl0F_09_80043424(void) {
    s32 pad[2]; /* fake: unused, frame space the original has (no source for it found) */

    D_ovl0F_09_80045F68 = 0;
    func_ovl0F_09_800430BC();
}

static void func_ovl0F_09_8004341C(s32 arg1, UnkStruct_ovl0F_09_80045FC0 *arg0) {
    s32 i;

    for (i = 0; i < arg1; i++) {
        *arg0->unk0 = func_ovl0F_09_800456C8(arg0->unk4, arg0->unk8);
        func_ovl0F_09_80045658(*arg0->unk0, arg0->unk24 | 0x01000000);
        func_ovl0F_09_80043398(arg0);
        (*arg0->unk0)->unk40->unk1C = arg0->unk18;
        (*arg0->unk0)->unk40->unk20 = arg0->unk1C;
        (*arg0->unk0)->unk40->unk24 = arg0->unk20;
        arg0++;
    }
}
#elif STATICS_PART == 2
static void func_ovl0F_09_80043020(s32 count, UnkStruct_ovl0F_09_80045FC0 *p) {
    s32 i;

    for (i = 0; i < count; i++) {
        func_ovl0F_09_80045670(*p->unk0);
        p++;
    }
}
#elif STATICS_PART == 3
            D_ovl0F_09_80046200[2]->unk40->unk10 = 2400.0f;
            D_ovl0F_09_80046200[2]->unk40->unk14 = 2600.0f;
            D_ovl0F_09_80046200[2]->unk40->unk18 = 5200.0f;
            D_ovl0F_09_80046200[2]->unk40->unk28 = 16.0f;
            D_ovl0F_09_80046200[2]->unk40->unk2C = 16.0f;
            D_ovl0F_09_80046200[2]->unk40->unk30 = 16.0f;
            break;
#endif
