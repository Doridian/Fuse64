/*
 * Without STATICS_PART: the statics below, included before the functions.
 * With it: the runs of code (definitions, or statements inside a function)
 * 1FCAC0.c includes where they are, from this same file: the `.file`
 * switches around them end as1's scheduling blocks, as the original's do.
 */
#ifndef STATICS_PART
/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0F_07_80044200(void) {
    func_ovl0F_07_800452F8(0.0f);
    func_ovl0F_07_80045300(0x7F);
    if (D_ovl0F_07_80045B28 != -1) {
        func_ovl0F_07_800457C8(D_ovl0F_07_80045B28);
    }
    func_ovl0F_07_80045388(250);
    func_ovl0F_07_80044A30(1);
    func_ovl0F_07_80044A60(1);
    func_ovl0F_07_800450E0(30.0f);
    func_ovl0F_07_800450E8(1.0f);
    func_ovl0F_07_80045100(10.0f);
    func_ovl0F_07_80044A18(D_ovl0F_07_80045D90.unk0, D_ovl0F_07_80045D90.unk4, D_ovl0F_07_80045D90.unk8,
                           D_ovl0F_07_80045D90.unkC, D_ovl0F_07_80045D90.unk10, D_ovl0F_07_80045D90.unk14,
                           D_ovl0F_07_80045D90.unk18);
    func_ovl0F_07_800458F0(0x8E);
}
#elif STATICS_PART == 1
static void func_ovl0F_07_80043998(s32 *dst, f32 x, f32 z) {
    *dst = ((s16)(x * 500.0f) << 16) | ((s16)(z * 500.0f) & 0xFFFF);
}
#elif STATICS_PART == 2
            D_ovl0F_07_80045E30[2]->unk40->unk10 = 2400.0f;
            D_ovl0F_07_80045E30[2]->unk40->unk14 = 2600.0f;
            D_ovl0F_07_80045E30[2]->unk40->unk18 = -2000.0f;
            D_ovl0F_07_80045E30[2]->unk40->unk28 = 16.0f;
            D_ovl0F_07_80045E30[2]->unk40->unk2C = 16.0f;
            D_ovl0F_07_80045E30[2]->unk40->unk30 = 16.0f;
            func_ovl0F_07_80043228(D_ovl0F_07_80045CBC, 1);
            break;
#endif
