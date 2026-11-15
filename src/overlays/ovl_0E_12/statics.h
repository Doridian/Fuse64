#ifndef STATICS_PART
/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0E_12_8004315C(s16 i) {
    func_ovl0E_12_8004311C(&D_ovl0E_12_80045FCC[i]);
}
#elif STATICS_PART == 1
            D_ovl0E_12_80045F94[i].unk0->unk40->unk10 = D_ovl0E_12_80045F94[i].unk14 * 10.0f;
            D_ovl0E_12_80045F94[i].unk0->unk40->unk14 = D_ovl0E_12_80045F94[i].unk18 * 10.0f;
            D_ovl0E_12_80045F94[i].unk0->unk40->unk18 = D_ovl0E_12_80045F94[i].unk1C * 10.0f;
            D_ovl0E_12_80045F94[i].unk0->unk40->unk1C = D_ovl0E_12_80045F94[i].unk20;
            D_ovl0E_12_80045F94[i].unk0->unk40->unk20 = D_ovl0E_12_80045F94[i].unk24;
            D_ovl0E_12_80045F94[i].unk0->unk40->unk24 = D_ovl0E_12_80045F94[i].unk28;
            D_ovl0E_12_80045F94[i].unk0->unk40->unk28 = D_ovl0E_12_80045F94[i].unk2C;
            D_ovl0E_12_80045F94[i].unk0->unk40->unk2C = D_ovl0E_12_80045F94[i].unk30;
            D_ovl0E_12_80045F94[i].unk0->unk40->unk30 = D_ovl0E_12_80045F94[i].unk34;
#endif
