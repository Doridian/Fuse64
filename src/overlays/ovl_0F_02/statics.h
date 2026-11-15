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
 */

static void func_ovl0F_02_80045F0C(UnkStruct_ovl0F_02_80048734 *p, s32 count) {
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
