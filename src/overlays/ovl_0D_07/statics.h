/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

/* the stage's model (D_ovl0D_07_800465A0) placed */
static void func_ovl0D_07_80043424(UnkStruct_ovl0C_01_8004A54C *e) {
    e->unk0 = func_ovl0D_07_80045CF0(e->unk8, 0xD0, 0.0f, 0.0f, 0.0f);
    func_ovl0D_07_80045CA8(e->unk0, 0x01080000);
    func_ovl0D_07_80045CB0(e->unk0, 0x02000000);
    if (e->unkC != -1 && e->unkE != -1 && e->unk10 != -1 && e->unk12 != -1) {
        e->unk4 = func_ovl0D_07_800459F0(e->unk0->unk40, 0, e->unkC, e->unkE, e->unk10, e->unk12);
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

/* the stage's model removed */
static void func_ovl0D_07_800432F0(UnkStruct_ovl0C_01_8004A54C *e) {
    func_ovl0D_07_80045CC0(e->unk0);
    if (e->unkC != 0xFFFF && e->unkE != 0xFFFF && e->unk10 != 0xFFFF && e->unk12 != 0xFFFF) {
        func_ovl0D_07_800459F8(e->unk4);
    }
    e->unk0 = NULL;
    e->unk4 = -1;
}
