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

static void func_ovl0F_04_80045E70(UnkStruct_ovl0F_04_800485C4 *p, s32 count) {
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
