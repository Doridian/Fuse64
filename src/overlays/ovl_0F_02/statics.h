/*
 * Without STATICS_PART: the statics below, included before the functions.
 * With it: the runs of code (definitions, or statements inside a function)
 * 1E84A0.c includes where they are, from this same file: the `.file`
 * switches around them end as1's scheduling blocks, as the original's do.
 */
#ifndef STATICS_PART
/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 * 800462E8, 80046150, _inl4 and _inl1 are the handler's statics that need
 * it (2026-11-15: each of the others tried in 1E84A0.c matches there).
 */

static void func_ovl0F_02_80045F0C(UnkStruct_ovl0F_01_80048264 *p, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        *p->unk0 = func_ovl0F_02_80047D10(p->unk4, p->unk24 | 0x01000000, 0x30, p->unk8);
        func_ovl0F_02_80044C94(p);
        (*p->unk0)->unk40->unk1C = p->unk18;
        (*p->unk0)->unk40->unk20 = p->unk1C;
        (*p->unk0)->unk40->unk24 = p->unk20;
        p++;
    }
}

/* 80046E80's last store: the stub before 80046E18 is its (the body is a
 * guess; the store has to be in another file than 80046E80's other code) */
static void func_ovl0F_02_80046E10(void) {
    D_ovl0F_02_80048610 = 0;
}

/* case 2's: the stage's tables */
static void func_ovl0F_02_800462E8(void) {
}

/* case 2's: the panels and their tiles; one hides something */
static void func_ovl0F_02_80046150(s32 n, UnkStruct_ovl0F_01_80048264 *p, UnkStruct_ovl0F_02_80048918 *flags) {
    s32 i;

    func_ovl0F_02_80044F74(n, p);
    for (i = 0; i < n; i++) {
        if (p->unk1C == 0.0f) {
            func_ovl0F_02_80047328((s32) p->unkC - 1, (s32) p->unk10,
                                   (s32) p->unk14 - 1, (s32) p->unkC - 1,
                                   (s32) p->unk14, 0x1411);
            func_ovl0F_02_80047328((s32) p->unkC, (s32) p->unk10,
                                   (s32) p->unk14 - 1, (s32) p->unkC,
                                   (s32) p->unk14, 0x141A);
        }
        flags->unk0 = 1;
        flags->unk8 = -1;
        p++;
        flags++;
    }
    D_ovl0F_02_80048918[func_ovl0F_02_80047FF8(n - 1)].unk8 = 3;
}

/* case 2's: 80046150 for the stage's panels (defined after it: its call's
 * line has to follow 80046150's for as1; its stub, 80046158, lands between
 * 80046150's and the door's) */
static void func_ovl0F_02_800462F8_inl4(void) {
    func_ovl0F_02_80046150(5, D_ovl0F_02_8004883C, D_ovl0F_02_80048918);
}

/* case 0x52's, on the switch at (0x12, 0xA): the camera and script */
static void func_ovl0F_02_800462F8_inl1(void) {
    /* held values (frame space the original has; which: a guess) */
    f32 dist = 0.0f;
    s32 flag = 0x406;
    s32 other = 0x407;
    f32 x = 581.4f;
    f32 y = 1430.1f;
    f32 z = 1133.3f;
    s32 wait = 60;
    char *script = (char *) D_ovl0F_02_80048BD8;

    if (func_ovl0F_02_800480C0(flag) == 0) {
        func_ovl0F_02_80047DC8(dist);
        func_ovl0F_02_80047DD0(0x7F);
        func_ovl0F_02_800480B0(flag);
        func_ovl0F_02_800480B8(other);
        func_ovl0F_02_80047C48();
        func_ovl0F_02_80047B90(0xF);
        func_ovl0F_02_80047B88(0x10);
        func_ovl0F_02_80047B18(0);
        func_ovl0F_02_80047B38(x, y, z);
        func_ovl0F_02_80043324(wait, script);
    }
}
#elif STATICS_PART == 1
    pos[0] = obj->unk40->unk10;
    pos[1] = obj->unk40->unk14;
    pos[2] = obj->unk40->unk18;
    dir = 0x40;
    if (func_ovl0F_02_800473A8(obj->unk40) == obj->unk40->unk14) {
        func_ovl0F_02_80048338(obj, dir, speed);
    } else {
        switch (dir) {
            case 0x00:
                func_ovl0F_02_800436D0(obj, speed);
                break;
            case 0x40:
                func_ovl0F_02_800436C8(obj, speed);
                break;
            case 0x80:
                func_ovl0F_02_800436C0(obj, speed);
                break;
            case 0xC0:
                func_ovl0F_02_800436B8(obj, speed);
                break;
        }
    }
#elif STATICS_PART == 2
            func_ovl0F_02_80043020(5, D_ovl0F_02_8004883C);
            func_ovl0F_02_80043020(1, &D_ovl0F_02_800489C4);
            if (func_ovl0F_02_800480C0(D_ovl0F_02_800489F4.unk0[0].unk28) == 0) {
                func_ovl0F_02_80043020(2, D_ovl0F_02_800489F4.unk0);
            } else {
                func_ovl0F_02_80043020(1, &D_ovl0F_02_800489F4.unk0[1]);
            }
            *D_ovl0F_02_800489F4.unk0[0].unk0 = NULL;
            if (D_ovl0F_02_80048954 == 1) {
                func_ovl0F_02_80048540();
            }
            break;
        case 12:
            func_ovl0F_02_80047530(1);
            break;
        case 3:
            D_ovl0F_02_80048D54 = func_ovl0F_02_800474C8();
            func_ovl0F_02_800474A8(&D_ovl0F_02_80048D48);
            func_ovl0F_02_800474D0(D_ovl0F_02_80048D54, D_ovl0F_02_80048D38);
            D_ovl0F_02_80048B08[0] = 1;
            func_ovl0F_02_80043028();
            break;
#elif STATICS_PART == 3
}

void func_ovl0F_02_80043010(void) {
}

#endif
