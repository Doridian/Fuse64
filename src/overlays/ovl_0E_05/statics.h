/*
 * Without STATICS_PART: a static the original defined in another file;
 * with it: the run of statements 1C3B40.c includes in place. The
 * `.file` switches around the run end as1's scheduling blocks, as the
 * original's do (as ovl_0E_04's and ovl_0E_12's statics.h); a first
 * switch to a file ugen hasn't seen yet ends none, so the static
 * comes first.
 */
#ifndef STATICS_PART
static void func_ovl0E_05_80044908(void) {
    D_ovl0E_05_80046230[0] = 0;
}
#elif STATICS_PART == 1
        D_ovl0E_05_8004615C[i].unk0->unk40->unk10 = D_ovl0E_05_8004615C[i].unk14 * 10.0f;
        D_ovl0E_05_8004615C[i].unk0->unk40->unk14 = D_ovl0E_05_8004615C[i].unk18 * 10.0f;
        D_ovl0E_05_8004615C[i].unk0->unk40->unk18 = D_ovl0E_05_8004615C[i].unk1C * 10.0f;
        D_ovl0E_05_8004615C[i].unk0->unk40->unk1C = D_ovl0E_05_8004615C[i].unk20;
        D_ovl0E_05_8004615C[i].unk0->unk40->unk20 = D_ovl0E_05_8004615C[i].unk24;
        D_ovl0E_05_8004615C[i].unk0->unk40->unk24 = D_ovl0E_05_8004615C[i].unk28;
        D_ovl0E_05_8004615C[i].unk0->unk40->unk28 = D_ovl0E_05_8004615C[i].unk2C;
        D_ovl0E_05_8004615C[i].unk0->unk40->unk2C = D_ovl0E_05_8004615C[i].unk30;
        D_ovl0E_05_8004615C[i].unk0->unk40->unk30 = D_ovl0E_05_8004615C[i].unk34;
#endif
