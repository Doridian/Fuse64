/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl17_07_80043204(UnkStruct_ovl17_07_800456B0 *e) {
    e->unk0 = func_ovl17_07_80044DD0(e->unk8, 0xD0, 0.0f, 0.0f, 0.0f);
    func_ovl17_07_80044D88(e->unk0, 0x01080000);
    func_ovl17_07_80044D90(e->unk0, 0x02000000);
    if (e->unkC != -1 && e->unkE != -1 && e->unk10 != -1 &&
        e->unk12 != -1) {
        e->unk4 =
            func_ovl17_07_80044AD0(e->unk0->unk40, 0, e->unkC,
                                   e->unkE, e->unk10, e->unk12);
        if (e->unk4 == -1) {
            while (TRUE) {
            }
        }
    }
    e->unk0->unk40->unk10 = e->unk14 * 10.0f;
    e->unk0->unk40->unk14 = e->unk18 * 10.0f;
    e->unk0->unk40->unk18 = e->unk1C * 10.0f;
    e->unk0->unk40->unk1C = e->unk20;
    e->unk0->unk40->unk20 = e->unk24;
    e->unk0->unk40->unk24 = e->unk28;
    e->unk0->unk40->unk28 = e->unk2C;
    e->unk0->unk40->unk2C = e->unk30;
    e->unk0->unk40->unk30 = e->unk34;
}
