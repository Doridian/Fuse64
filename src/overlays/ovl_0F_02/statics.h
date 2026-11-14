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
