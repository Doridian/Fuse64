/*
 * Without STATICS_PART: the statics below, included before the functions.
 * With it: the runs of code (definitions, or statements inside a function)
 * 1F2230.c includes where they are, from this same file: the `.file`
 * switches around them end as1's scheduling blocks, as the original's do.
 */
#ifndef STATICS_PART
/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0F_04_80045E70(UnkStruct_ovl0F_01_80048264 *p, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        *p->unk0 = func_ovl0F_04_80047BA0(p->unk4, p->unk24 | 0x01000000, 0x30, p->unk8);
        func_ovl0F_04_800433FC(p);
        (*p->unk0)->unk40->unk1C = p->unk18;
        (*p->unk0)->unk40->unk20 = p->unk1C;
        (*p->unk0)->unk40->unk24 = p->unk20;
        p++;
    }
}

/* 80046D14's last store (its stub is the first of those at the object's
 * end): from here, the store and the epilogue are blocks of their own */
static void func_ovl0F_04_80046C70(void) {
    D_ovl0F_04_800484A0 = 0;
}

/* case 2's: the panels and their tiles; one hides something.  The loop's p
 * is the parameter (its binding takes the call's line, after the body's
 * lines, which as1's schedule of the handler's block shows); panels keeps
 * the first for 80043480 */
static void func_ovl0F_04_80043578(s32 n, UnkStruct_ovl0F_01_80048264 *p, UnkStruct_ovl0F_04_800487A8 *flags) {
    s32 i;
    UnkStruct_ovl0F_04_800487A8 *hidden; /* a guess: any held value fills this word (n - 1, a tile) */
    UnkStruct_ovl0F_01_80048264 *panels;

    panels = p;
    func_ovl0F_04_80043480(n, panels);
    for (i = 0; i < n; i++) {
        if (p->unk1C == 0.0f) {
            func_ovl0F_04_800471B8((s32) p->unkC - 1, (s32) p->unk10,
                                   (s32) p->unk14 - 1, (s32) p->unkC - 1,
                                   (s32) p->unk14, 0x1411);
            func_ovl0F_04_800471B8((s32) p->unkC, (s32) p->unk10,
                                   (s32) p->unk14 - 1, (s32) p->unkC,
                                   (s32) p->unk14, 0x141A);
        }
        flags->unk0 = 1;
        flags->unk8 = -1;
        p++;
        flags++;
    }
    hidden = &D_ovl0F_04_800487A8[func_ovl0F_04_80047E88(n - 1)];
    hidden->unk8 = 3;
}

/* case 2's: the panels' call (its stub with the handler's) */
static void func_ovl0F_04_8004624C_inl4(void) {
    func_ovl0F_04_80043578(5, D_ovl0F_04_800486CC, D_ovl0F_04_800487A8);
}

/* case 2's: the stage's tables (inlined into _inl0; its stub is 800460C4) */
static void func_ovl0F_04_8004623C(void) {
}

/* the switch at (0x12, 0xA): the wind stops and the camera shows it.  The
 * values held in locals (eye, flag, fog, vol, frames, msg) are a guess: the
 * handler's frame has room for eight words here, and any held values fill it
 * (_inl2, the switch back, is in 1F2230.c) */
static void func_ovl0F_04_8004624C_inl1(void) {
    Vec3f eye;
    s32 flag;
    f32 fog;
    s32 vol;
    s32 frames;
    char *msg;

    flag = 0x406;
    fog = 0.0f;
    vol = 0x7F;
    eye.x = 581.4f;
    eye.y = 1430.1f;
    eye.z = 1133.3f;
    frames = 60;
    msg = (char *) D_ovl0F_04_80048958;
    if (func_ovl0F_04_80047F50(flag) == 0) {
        func_ovl0F_04_80047C58(fog);
        func_ovl0F_04_80047C60(vol);
        func_ovl0F_04_80047F40(flag);
        func_ovl0F_04_80047F48(0x407);
        func_ovl0F_04_80047AD8();
        func_ovl0F_04_80047A20(0xF);
        func_ovl0F_04_80047A18(0x10);
        func_ovl0F_04_800479A8(0);
        func_ovl0F_04_800479C8(eye.x, eye.y, eye.z);
        func_ovl0F_04_80043324(frames, msg);
    }
}
#elif STATICS_PART == 1
    pos[0] = obj->unk40->unk10;
    pos[1] = obj->unk40->unk14;
    pos[2] = obj->unk40->unk18;
    dir = 0x40;
    if (func_ovl0F_04_80047238(obj->unk40) == obj->unk40->unk14) {
        func_ovl0F_04_800481C8(obj, dir, speed);
    } else {
        switch (dir) {
            case 0x00:
                func_ovl0F_04_80043768(obj, speed);
                break;
            case 0x40:
                func_ovl0F_04_80043760(obj, speed);
                break;
            case 0x80:
                func_ovl0F_04_80043758(obj, speed);
                break;
            case 0xC0:
                func_ovl0F_04_80043750(obj, speed);
                break;
        }
    }
#elif STATICS_PART == 2
            func_ovl0F_04_80043020(5, D_ovl0F_04_800486CC);
            func_ovl0F_04_80043020(1, &D_ovl0F_04_80048850);
            if (func_ovl0F_04_80047F50(D_ovl0F_04_80048880.unk0[0].unk28) == 0) {
                func_ovl0F_04_80043020(2, D_ovl0F_04_80048880.unk0);
            } else {
                func_ovl0F_04_80043020(1, &D_ovl0F_04_80048880.unk0[1]);
            }
            *D_ovl0F_04_80048880.unk0[0].unk0 = NULL;
#endif
