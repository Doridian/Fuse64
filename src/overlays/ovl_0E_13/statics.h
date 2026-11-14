/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

/* Inlined into func_ovl0E_13_80043110 only, which is inlined into its
 * callers in turn; in this file for the block boundary where its body
 * starts. */
static void func_ovl0E_13_80043108(f32 x, s32 z, s32 count) {
    s32 i;

    D_ovl0E_13_80046D80.x = x * 100.0f;
    D_ovl0E_13_80046D80.y = 0.0f;
    for (i = 0; i < count; i++) {
        D_ovl0E_13_80046D80.y += func_ovl0E_13_80045510(i) * 100.0f;
    }
    D_ovl0E_13_80046D80.y += 100.0f;
    D_ovl0E_13_80046D80.z = z * 100.0f;
    D_ovl0E_13_80046D70 = func_ovl0E_13_80046450(0x27, &D_ovl0E_13_80046D80.x, 0);
}

static void func_ovl0E_13_800437B8(UnkStruct_ovl0E_13_80046800 *e) {
    func_ovl0E_13_80045EB0(e->unk0, 0x01000000);
    func_ovl0E_13_80045EC0(e->unk0);
    /* 0xFFFF, not -1: never equal to the s16 fields */
    if (e->unkC != 0xFFFF && e->unkE != 0xFFFF && e->unk10 != 0xFFFF && e->unk12 != 0xFFFF) {
        func_ovl0E_13_80045BF8(e->unk4);
    }
    e->unk0 = NULL;
    e->unk4 = -1;
}

static void func_ovl0E_13_800437C0(UnkStruct_ovl0E_13_80046800 *e) {
    e->unk0 = func_ovl0E_13_80045EF0(e->unk8, 0xD0, 0.0f, 0.0f, 0.0f);
    func_ovl0E_13_80045EA8(e->unk0, 0x01080000);
    func_ovl0E_13_80045EB0(e->unk0, 0x02000000);
    if (e->unkC != -1 && e->unkE != -1 && e->unk10 != -1 && e->unk12 != -1) {
        e->unk4 = func_ovl0E_13_80045BF0(e->unk0->unk40, 0, e->unkC, e->unkE, e->unk10, e->unk12);
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
