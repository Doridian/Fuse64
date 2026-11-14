/*
 * Code from another file than 1FF8F0.c (its `.file` switches end as1's
 * scheduling blocks): without STATICS_PART the statics, included before
 * the functions; with it the runs of code (definitions, or statements inside
 * a function) 1FF8F0.c includes where they are.
 */
#ifndef STATICS_PART
static void func_ovl0F_08_80043968(s32 *dst, f32 x, f32 z) {
    *dst = ((s16)(x * 500.0f) << 16) | ((s16)(z * 500.0f) & 0xFFFF);
}

static s32 func_ovl0F_08_80043970(UnkStructA *arg0) {
    s32 hit;
    s32 saved;
    UnkStructA *other;
    s32 id;

    hit = 0;
    other = NULL;
    saved = arg0->unkC;
    arg0->unkC = 0;
    if (arg0->unk8 & 1) {
        other = func_ovl0F_08_80044AA8(func_ovl0F_08_80044A08());
    } else if (arg0->unk8 & 2) {
        id = func_ovl0F_08_80044A08();
        if (func_ovl0F_08_80044AA8(id) == arg0) {
            other = func_ovl0F_08_80044A68(id);
        } else {
            other = NULL;
        }
    }
    if (other != NULL) {
        if (func_ovl0F_08_80045348(arg0, 0x10, other) != 0) {
            hit = 1;
        }
    } else if (func_ovl0F_08_800451D8(arg0, 0x10) != 0) {
        hit = 1;
    }
    arg0->unkC = saved;
    return hit;
}

static void func_ovl0F_08_80043978(f32 arg0) {
    if (D_ovl0F_08_80045B2C != 0) {
        if (D_ovl0F_08_80045B28 == 0) {
            if (arg0 == 0.0f) {
                func_ovl0F_08_80045900(0x32);
            } else {
                func_ovl0F_08_80045900(0x27);
            }
            D_ovl0F_08_80045B28 = 20;
            func_ovl0F_08_800457C8(0x19, D_ovl0F_08_80045B30->unk40->unk10, D_ovl0F_08_80045B30->unk40->unk14,
                                   D_ovl0F_08_80045B30->unk40->unk18, 0);
        } else {
            D_ovl0F_08_80045B28--;
        }
    }
}









static void func_ovl0F_08_800433D4(void) {
    s32 count;
    u32 (*p)[3];
    s32 i;

    func_ovl0F_08_800456E0(D_ovl0F_08_80045B3C);
    p = D_ovl0F_08_80045B60;
    count = 4;
    for (i = 0; i < count; i++) {
        func_ovl0F_08_80045888((*p)[0], (*p)[1], (*p)[2]);
        p++;
    }
}

static void func_ovl0F_08_800433CC(void) {
    func_ovl0F_08_80044828(0x22D);
    func_ovl0F_08_80044840(0x23E);
    func_ovl0F_08_800449D8();
}

static void func_ovl0F_08_800433C4(void) {
    s32 pad[2]; /* fake: unused, frame space the original has (no source for it found) */

    D_ovl0F_08_80045B38 = 0;
    func_ovl0F_08_800430BC();
}

static void func_ovl0F_08_800433BC(s32 arg1, UnkStruct_ovl0F_08_80045B90 *arg0) {
    s32 i;

    for (i = 0; i < arg1; i++) {
        *arg0->unk0 = func_ovl0F_08_80045298(arg0->unk4, arg0->unk8);
        func_ovl0F_08_80045228(*arg0->unk0, arg0->unk24 | 0x01000000);
        func_ovl0F_08_80043338(arg0);
        (*arg0->unk0)->unk40->unk1C = arg0->unk18;
        (*arg0->unk0)->unk40->unk20 = arg0->unk1C;
        (*arg0->unk0)->unk40->unk24 = arg0->unk20;
        arg0++;
    }
}

static void func_ovl0F_08_80043330(void) {
    func_ovl0F_08_80044F20(D_ovl0F_08_80045DA0[3]->unk40, 0x61);
    func_ovl0F_08_80044F20(D_ovl0F_08_80045DA0[4]->unk40, 0x61);
    D_ovl0F_08_80045DA0[4]->unk40->unk14 -= 3200.0f;
}

static void func_ovl0F_08_80043254(f32 arg0, f32 arg1) {
    D_ovl0F_08_80045B20 = arg0;
    D_ovl0F_08_80045B24 = arg1;
    D_ovl0F_08_80045B28 = 0;
}

static void func_ovl0F_08_8004325C(void) {
    if (func_ovl0F_08_80044978()[4] == 0) {
        func_ovl0F_08_80043254(0.3f, 0.5f);
    } else {
        func_ovl0F_08_80043254(0.12f, 0.15f);
    }
}

static void func_ovl0F_08_80043264(void) {
    func_ovl0F_08_80044A38();
    func_ovl0F_08_80044910(func_ovl0F_08_800440A8);
    func_ovl0F_08_8004325C();
    func_ovl0F_08_80044908(func_ovl0F_08_80043A28, 1);
}

static void func_ovl0F_08_8004324C(void) {
    switch (func_ovl0F_08_80044928()) {
        case 17:
            func_ovl0F_08_80044A28(D_ovl0F_08_80045CD0[0].unk0, D_ovl0F_08_80045CD0[0].unk4,
                                   D_ovl0F_08_80045CD0[0].unk8, D_ovl0F_08_80045CD0[0].unkC,
                                   D_ovl0F_08_80045CD0[0].unk10, D_ovl0F_08_80045CD0[0].unk14,
                                   D_ovl0F_08_80045CD0[0].unk18);
            break;
        case 20:
            func_ovl0F_08_80044A28(D_ovl0F_08_80045CD0[1].unk0, D_ovl0F_08_80045CD0[1].unk4,
                                   D_ovl0F_08_80045CD0[1].unk8, D_ovl0F_08_80045CD0[1].unkC,
                                   D_ovl0F_08_80045CD0[1].unk10, D_ovl0F_08_80045CD0[1].unk14,
                                   D_ovl0F_08_80045CD0[1].unk18);
            break;
    }
}

static void func_ovl0F_08_800431F8(void) {
    func_ovl0F_08_80044A68(func_ovl0F_08_80044A08())->unk64 = 0;
    D_ovl0F_08_80045B28 = 0;
}

static void func_ovl0F_08_80043020(s32 count, UnkStruct_ovl0F_08_80045B90 *p) {
    s32 i;

    for (i = 0; i < count; i++) {
        func_ovl0F_08_80045240(*p->unk0);
        p++;
    }
}
#elif STATICS_PART == 1
            D_ovl0F_08_80045DA0[2]->unk40->unk10 = 2400.0f;
            D_ovl0F_08_80045DA0[2]->unk40->unk14 = 2600.0f;
            D_ovl0F_08_80045DA0[2]->unk40->unk18 = 5200.0f;
            D_ovl0F_08_80045DA0[2]->unk40->unk28 = 16.0f;
            D_ovl0F_08_80045DA0[2]->unk40->unk2C = 16.0f;
            D_ovl0F_08_80045DA0[2]->unk40->unk30 = 16.0f;
            break;
#endif
