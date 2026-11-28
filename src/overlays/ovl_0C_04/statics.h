/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0C_04_8004365C(void) {
    UnkStruct_ovl0C_01_8004A54C *rec;

    rec = &D_ovl0C_04_80045D34;
    rec->unk0 = func_ovl0C_04_80045360(rec->unk8, 0xD0, 0.0f, 0.0f, 0.0f);
    func_ovl0C_04_80045318(rec->unk0, 0x01080000);
    func_ovl0C_04_80045320(rec->unk0, 0x02000000);
    if (rec->unkC != -1 && rec->unkE != -1 && rec->unk10 != -1 && rec->unk12 != -1) {
        rec->unk4 = func_ovl0C_04_80045060(rec->unk0->unk40, 0, rec->unkC, rec->unkE, rec->unk10, rec->unk12);
        if (rec->unk4 == -1) {
            for (;;) {
            }
        }
    }
    rec->unk0->unk40->unk10 = rec->unk14 * 10.0f;
    rec->unk0->unk40->unk14 = rec->unk18 * 10.0f;
    rec->unk0->unk40->unk18 = rec->unk1C * 10.0f;
    rec->unk0->unk40->unk1C = rec->unk20;
    rec->unk0->unk40->unk20 = rec->unk24;
    rec->unk0->unk40->unk24 = rec->unk28;
    rec->unk0->unk40->unk28 = rec->unk2C;
    rec->unk0->unk40->unk2C = rec->unk30;
    rec->unk0->unk40->unk30 = rec->unk34;
}
