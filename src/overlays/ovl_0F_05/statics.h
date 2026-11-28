/*
 * Code from another file than 1F7CA0.c (its `.file` switches end as1's
 * scheduling blocks): without STATICS_PART the statics, included before
 * the functions; with it the runs of code (definitions, or statements inside
 * a function) 1F7CA0.c includes where they are.
 */
#ifndef STATICS_PART
static void func_ovl0F_05_80043FE4(s32 *dst, f32 x, f32 z) {
    *dst = ((s16)(x * 500.0f) << 16) | ((s16)(z * 500.0f) & 0xFFFF);
}

static s32 func_ovl0F_05_80043FEC(UnkStructA *arg0) {
    s32 hit;
    s32 saved;
    UnkStructA *other;
    s32 id;

    hit = 0;
    other = NULL;
    saved = arg0->unkC;
    arg0->unkC = 0;
    if (arg0->unk8 & 1) {
        other = func_ovl0F_05_800450D8(func_ovl0F_05_80045038());
    } else if (arg0->unk8 & 2) {
        id = func_ovl0F_05_80045038();
        if (func_ovl0F_05_800450D8(id) == arg0) {
            other = func_ovl0F_05_80045098(id);
        } else {
            other = NULL;
        }
    }
    if (other != NULL) {
        if (func_ovl0F_05_80045978(arg0, 0x10, other) != 0) {
            hit = 1;
        }
    } else if (func_ovl0F_05_80045808(arg0, 0x10) != 0) {
        hit = 1;
    }
    arg0->unkC = saved;
    return hit;
}

static void func_ovl0F_05_80043FF4(f32 arg0) {
    if (D_ovl0F_05_8004615C != 0) {
        if (D_ovl0F_05_80046158 == 0) {
            if (arg0 == 0.0f) {
                func_ovl0F_05_80045F30(0x32);
            } else {
                func_ovl0F_05_80045F30(0x27);
            }
            D_ovl0F_05_80046158 = 20;
            func_ovl0F_05_80045DF8(0x19, D_ovl0F_05_80046160->unk40->unk10, D_ovl0F_05_80046160->unk40->unk14,
                                   D_ovl0F_05_80046160->unk40->unk18, 0);
        } else {
            D_ovl0F_05_80046158--;
        }
    }
}

/* empty: the stub IDO leaves before func_ovl0F_05_80043B74 (an inlined callee
 * of it; what it was is unknown) */
static void func_ovl0F_05_80043B6C(void) {
}

void func_ovl0F_05_80043B74(void) {
    UnkStruct_802ACF08 *cam;
    s32 i;
    s32 task;
    s32 j;
    s32 k;
    f32 angle;
    UnkStructA_40 *objs[4];

    func_ovl0F_05_80043B6C();
    cam = func_ovl0F_05_80045110(0);
    D_ovl0F_05_800463F0 = 0;
    while (D_ovl0F_05_800463F0 == 0) {
        func_ovl0F_05_800459F0();
    }
    task = func_ovl0F_05_800459A8(func_ovl0F_05_8004472C, 0, 0, 0, 0x130);
    func_ovl0F_05_80044F38(func_ovl0F_05_800440AC, 1);
    func_ovl0F_05_800450E0(0, 2);
    cam->unk8 = 30.0f;
    cam->unk4 = 0.0f;
    cam->unk0 = 32.0f;
    func_ovl0F_05_800459C8(0x23A);
    cam->unk8 = 0.0f;
    cam->unkC = 0x4000;
    cam->unkE = 0x8000;
    func_ovl0F_05_800459F0();
    cam->unkC = 0;
    func_ovl0F_05_800459C8(40);
    cam->unkE = 0;
    func_ovl0F_05_800459F0();
    cam->unk8 = 30.0f;
    cam->unk0 = 60.0f;
    /* its own counter: with i (used in the loops below) it compares with
     * slti rather than bne */
    for (k = 0; k < 300; k++) {
        cam->unk0 += 0.08f;
        func_ovl0F_05_800459F0();
    }
    cam->unk0 = 180.0f;
    func_ovl0F_05_800459F0();
    cam->unk8 = 0.0f;
    func_ovl0F_05_800459C8(120);
    for (i = 0; i < 4; i++) {
        objs[i] = func_ovl0F_05_80045538(0x307, 0);
        objs[i]->unkC |= 0x80;
        objs[i]->unk36 = 60;
        objs[i]->unk10 = 2450.0f;
        objs[i]->unk14 = 0;
        objs[i]->unk18 = 500.0f;
        objs[i]->unk1C = 90.0f;
        objs[i]->unk28 = objs[i]->unk2C = objs[i]->unk30 = (i + 1) * 0.5f;
    }
    for (i = 0; i < 180; i++) {
        for (j = 0; j < 4; j++) {
            objs[j]->unk10 -= (j + 1) * 4.0f;
            objs[j]->unk14 -= (j + 1);
        }
        angle = D_ovl0F_05_80046550.y;
        ANGLE_SUB(angle, 0.33333334f);
        func_ovl0F_05_800459F0();
    }
    for (i = 0; i < 4; i++) {
        func_ovl0F_05_80045540(objs[i]);
    }
    func_ovl0F_05_800459C8(120);
    cam->unk8 = 36.0f;
    func_ovl0F_05_800459C8(30);
    cam->unk8 = 61.0f;
    func_ovl0F_05_800459C8(30);
    cam->unk8 = 0.0f;
    func_ovl0F_05_800459C8(80);
    cam->unkE = 0x4000;
    cam->unkC = 0x4000;
    func_ovl0F_05_800459F0();
    cam->unkE = 0;
    func_ovl0F_05_800459C8(30);
    func_ovl0F_05_800460A8();
    if (task != -1) {
        func_ovl0F_05_800459B0(task);
    }
    func_ovl0F_05_800459B8();
}

void func_ovl0F_05_8004373C(void) {
    s32 running;
    f32 pos[3]; /* below running: pos at sp+0x78 */

    running = 1;
    do {
        D_ovl0F_05_80046164 += 1.0f;
        if (D_ovl0F_05_80046164 < 500.0f) {
            func_ovl0F_05_800456B8(D_ovl0F_05_80046168, D_ovl0F_05_8004616C, 6408.0f);
            D_ovl0F_05_80046168 += 0.2734f;
            D_ovl0F_05_8004616C += 0.065699995f;
            D_ovl0F_05_80046174 -= 2.5f;
        } else if (D_ovl0F_05_80046164 == 500.0f) {
            func_ovl0F_05_80045F30(0x8B);
            func_ovl0F_05_80045698(2200.0f, 100.0f, 332.0f);
            func_ovl0F_05_800456B8(135.0f, 56.8f, 2000.0f);
            D_ovl0F_05_800464E0[3]->unk40->unk28 = 1.0f;
            D_ovl0F_05_800464E0[3]->unk40->unk2C = 1.0f;
            D_ovl0F_05_800464E0[3]->unk40->unk30 = 1.0f;
            D_ovl0F_05_80046174 = 600.0f;
        } else if (D_ovl0F_05_80046164 < 650.0f) {
            D_ovl0F_05_80046174 -= 4.0f;
        } else if (D_ovl0F_05_80046164 == 650.0f) {
            D_ovl0F_05_80046184 = 0.0f;
            func_ovl0F_05_80045F30(0x90);
        } else if (D_ovl0F_05_80046164 < 830.0f) {
            D_ovl0F_05_80046174 += func_ovl0F_05_80045B58(D_ovl0F_05_80046184);
            D_ovl0F_05_80046184 += 1.0f;
        } else if (D_ovl0F_05_80046164 == 830.0f) {
            D_ovl0F_05_80046184 = 0.0f;
            func_ovl0F_05_80045F30(0x90);
        } else if (D_ovl0F_05_80046164 < 890.0f) {
            D_ovl0F_05_80046174 += func_ovl0F_05_80045B58(D_ovl0F_05_80046184);
            D_ovl0F_05_80046184 += 3.0f;
        } else if (D_ovl0F_05_80046164 == 890.0f) {
            D_ovl0F_05_80046174 = 0.0f;
            func_ovl0F_05_80045F30(0x90);
        } else if (D_ovl0F_05_80046164 < 940.0f) {
        } else if (D_ovl0F_05_80046164 == 940.0f) {
            /* 0, not 0.0f: `mtc1 zero` rather than the 0.0f in $f22 */
            func_ovl0F_05_80044EB8(22.0f, 0, 3.0f, pos, 0);
            pos[1] += 2.0f;
            func_ovl0F_05_80045F30(0x2C);
            D_ovl0F_05_80046180 = func_ovl0F_05_80045E00(0x25, pos, 0);
        } else if (D_ovl0F_05_80046164 < 1090.0f) {
        } else {
            running = 0;
        }
        D_ovl0F_05_800464E0[3]->unk40->unk18 = D_ovl0F_05_80046178 - D_ovl0F_05_80046174;
        D_ovl0F_05_800464E0[3]->unk40->unk14 =
            (1.0f - D_ovl0F_05_800464E0[3]->unk40->unk2C) * 600.0f + (D_ovl0F_05_8004617C + D_ovl0F_05_80046174);
        func_ovl0F_05_800459F0();
    } while (running);
    func_ovl0F_05_80044F50(0x61, 0x15, 0);
    func_ovl0F_05_800459B8();
}

static void func_ovl0F_05_800430B4(void) {
    s32 i;
    u32 (*p)[3];
    s32 count;

    func_ovl0F_05_80045D10(D_ovl0F_05_8004618C);
    p = D_ovl0F_05_800461B0;
    count = 4;
    for (i = 0; i < count; i++) {
        func_ovl0F_05_80045EB8((*p)[0], (*p)[1], (*p)[2]);
        p++;
    }
}

static void func_ovl0F_05_800430AC(void) {
    func_ovl0F_05_80044E58(0x22A);
    func_ovl0F_05_80044E70(0x23D);
    func_ovl0F_05_80045008();
}

static void func_ovl0F_05_800430A4(s32 arg1, UnkStruct_ovl0F_01_80048264 *arg0) {
    s32 i;

    for (i = 0; i < arg1; i++) {
        *arg0->unk0 = func_ovl0F_05_800458C8(arg0->unk4, arg0->unk8);
        func_ovl0F_05_80045858(*arg0->unk0, arg0->unk24 | 0x01000000);
        func_ovl0F_05_80043020(arg0);
        (*arg0->unk0)->unk40->unk1C = arg0->unk18;
        (*arg0->unk0)->unk40->unk20 = arg0->unk1C;
        (*arg0->unk0)->unk40->unk24 = arg0->unk20;
        arg0++;
    }
}

static void func_ovl0F_05_80043018(s32 count, UnkStruct_ovl0F_01_80048264 *p) {
    s32 i;

    for (i = 0; i < count; i++) {
        func_ovl0F_05_80045870(*p->unk0);
        p++;
    }
}
#elif STATICS_PART == 1
            D_ovl0F_05_8004654C = func_ovl0F_05_80045A30(0x279);
            func_ovl0F_05_80045A40(D_ovl0F_05_8004654C, 0x20000000);
            D_ovl0F_05_8004654C->unk40 = 0.6f;
            D_ovl0F_05_8004654C->unk44 = D_ovl0F_05_8004654C->unk40 * 0.75f;
            D_ovl0F_05_8004654C->unk3C = D_ovl0F_05_8004654C->unk4C * 0.3f;
#endif
