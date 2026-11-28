/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0F_03_800434A4(UnkStructA *arg0, f32 *pos) {
    pos[0] = arg0->unk40->unk10;
    pos[1] = arg0->unk40->unk14;
    pos[2] = arg0->unk40->unk18;
}

static void func_ovl0F_03_8004348C(UnkStructA *arg0, f32 *pos) {
    arg0->unk40->unk10 = pos[0];
    arg0->unk40->unk14 = pos[1];
    arg0->unk40->unk18 = pos[2];
}

/* case 4's: the particles' texture and display list */
static void func_ovl0F_03_80043434(void) {
    UnkStructA_40 *obj;

    if (0) { /* fake: dead code, for the order or allocation it gives (no code) */
        func_ovl0F_03_80046648(func_ovl0F_03_80044C20, NULL, 0, 0, 0);
    }
    D_ovl0F_03_80046DF0 = func_ovl0F_03_800461C8(0x258);
    obj = func_ovl0F_03_80046258(func_ovl0F_03_80044258, -60);
    D_ovl0F_03_80047284 = obj;
    obj->unkC |= 0x210;
}

/* case 2's: the door, closed or already open */
static void func_ovl0F_03_80044C7C(UnkStruct_ovl0F_02_800489F4 *door) {
    UnkStruct_ovl0F_01_80048264 *m1; /* the door's two entries held in locals (the */
    UnkStruct_ovl0F_01_80048264 *m0; /* frame's 8 bytes; which values: a guess, as 0F_02's) */

    m0 = door->unk0;
    m1 = &door->unk0[1];
    if (func_ovl0F_03_800468D0(m0->unk28) == 0) {
        func_ovl0F_03_80046648(func_ovl0F_03_80043F38, door, 0, 0, 0x110);
    } else {
        door->unk58();
        door->unk5C();
        m1->unk10 -= 1.0f;
        func_ovl0F_03_800430B4(1, m1);
        *m0->unk0 = NULL;
        m1->unk10 += 1.0f;
    }
}

static void func_ovl0F_03_8004326C(void) {
    if (func_ovl0F_03_800468D0(0x404) == 0 && func_ovl0F_03_80045CC0(0x16, 0x1F, 0x16, 0x1F) != 0) {
        func_ovl0F_03_80045D20(0x40);
        func_ovl0F_03_80045CE8(1);
        func_ovl0F_03_80046BB8();
        func_ovl0F_03_80046AD8();
        func_ovl0F_03_80045D10(1);
        func_ovl0F_03_80046BD0(0x29);
        func_ovl0F_03_800468C0(D_ovl0F_03_80046EB4[0].unk28);
        func_ovl0F_03_80043020(1, D_ovl0F_03_80046EB4);
        func_ovl0F_03_80043164(D_ovl0F_03_80046EB4, 0);
        func_ovl0F_03_80046500(*D_ovl0F_03_80046EB4[1].unk0, 0x01000000);
        func_ovl0F_03_80046648(func_ovl0F_03_80043AF0, D_ovl0F_03_80046EB4, 0, 0, 0x130);
    }
}

static void func_ovl0F_03_8004327C(void) {
    s32 pad[10]; /* fake: unused, frame space the original has (no source for it found) */
    if (func_ovl0F_03_800468D0(0x404) != 0 && func_ovl0F_03_80045CC0(0x16, 7, 0x16, 7) != 0) {
        func_ovl0F_03_80045D10(1);
        func_ovl0F_03_80045C70(0);
        func_ovl0F_03_80046648(func_ovl0F_03_80043944, D_ovl0F_03_80046EB4, 0, 0, 0x130);
        func_ovl0F_03_80043274(func_ovl0F_03_800468E0(0x21));
        func_ovl0F_03_80045C18();
    }
}

static void func_ovl0F_03_800432D0(void) {
    switch (func_ovl0F_03_80045BF8()) {
        case 0x12:
            func_ovl0F_03_80045CF8(D_ovl0F_03_80046FA4[0].unk0, D_ovl0F_03_80046FA4[0].unk4,
                                   D_ovl0F_03_80046FA4[0].unk8, D_ovl0F_03_80046FA4[0].unkC,
                                   D_ovl0F_03_80046FA4[0].unk10, D_ovl0F_03_80046FA4[0].unk14,
                                   D_ovl0F_03_80046FA4[0].unk18);
            break;
        case 0x14:
            func_ovl0F_03_80045CF8(D_ovl0F_03_80046FA4[1].unk0, D_ovl0F_03_80046FA4[1].unk4,
                                   D_ovl0F_03_80046FA4[1].unk8, D_ovl0F_03_80046FA4[1].unkC,
                                   D_ovl0F_03_80046FA4[1].unk10, D_ovl0F_03_80046FA4[1].unk14,
                                   D_ovl0F_03_80046FA4[1].unk18);
            func_ovl0F_03_80046648(func_ovl0F_03_8004389C, NULL, 0, 0, 0x130);
            break;
    }
}

/* case 2's: the stage's tables */
static void func_ovl0F_03_80044C8C(u32 (*p)[3], s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        func_ovl0F_03_80046B58((*p)[0], (*p)[1], (*p)[2]);
        p++;
    }
}
