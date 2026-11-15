/*
 * Without STATICS_PART: the statics below, included before the functions.
 * With it: the runs of code (definitions, or statements inside a function)
 * 1A0CD0.c includes where they are, from this same file: the `.file`
 * switches around them end as1's scheduling blocks, as the original's do.
 */
#ifndef STATICS_PART
/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0D_03_80043C14(void) {
    Ent_0D03 *p;

    func_ovl0D_03_80047400(D_ovl0D_03_8004817C);
    for (p = D_ovl0D_03_80047D1C; p < D_ovl0D_03_80047D1C + 3; p++) {
        if (p->obj != NULL) {
            func_ovl0D_03_80043B98(p);
        }
    }
}

/* the tiles under the gate: solid, and the frame around its opening */
static void func_ovl0D_03_8004337C(Ent_0D03 *e) {
    s32 grid[6];
    s32 x0;
    s32 y0;
    s32 z0;
    s32 x1;
    s32 y1;
    s32 z1;
    s32 x2;
    s32 z2;
    s32 x3;
    s32 z3;
    s32 len;
    UnkStructA_40 *p;

    p = e->obj->unk40;
    func_ovl0D_03_80046A40(p->unk10, p->unk14, p->unk18, grid, 0);
    len = x1 + z1; /* fake: (see the top; pass 12: the locals declared x1, z1 first or z0 last, x0..z0 set up after the len switch: no) */
    x0 = grid[0];
    y0 = grid[1];
    z0 = grid[2];
    switch (e->obj->unk28) {
        case 0x15C:
            len = 5;
            break;
        case 0x15D:
            len = 7;
            break;
        case 0x15E:
            len = 10;
            break;
        default:
            while (1) {
            }
    }
    y1 = grid[1] + 2;
    switch (func_ovl0D_03_80043374(e)) {
        case 0:
            x0 = grid[0] - 1;
            x1 = x0 + 4;
            z1 = grid[2] + len;
            x2 = x0 + 1;
            x3 = x1 - 1;
            z2 = grid[2];
            z3 = z1;
            break;
        case 1:
            z1 = grid[2] + 1;
            z0 = grid[2] - 4;
            x1 = grid[0] + len;
            x3 = x1;
            z2 = z0 + 1;
            z3 = z1 - 1;
            x2 = grid[0];
            break;
        case 2:
            x1 = grid[0] + 1;
            x0 = grid[0] - 3;
            z0 = grid[2] - len;
            x2 = x0 + 1;
            x3 = x1 - 1;
            z2 = z0;
            z1 = grid[2];
            z3 = grid[2];
            break;
        case 3:
            z1 = grid[2] + 3;
            z0 = grid[2] - 1;
            x0 = grid[0] - len;
            x2 = x0;
            z2 = z0 + 1;
            z3 = z1 - 1;
            x3 = grid[0];
            x1 = grid[0];
            break;
    }
    func_ovl0D_03_80043300(0, &x0, D_ovl0D_03_80048188);
    func_ovl0D_03_80043300(1, &y0, D_ovl0D_03_80048188);
    func_ovl0D_03_80043300(2, &z0, D_ovl0D_03_80048188);
    func_ovl0D_03_80043300(0, &x1, D_ovl0D_03_80048188);
    func_ovl0D_03_80043300(1, &y1, D_ovl0D_03_80048188);
    func_ovl0D_03_80043300(2, &z1, D_ovl0D_03_80048188);
    func_ovl0D_03_80043300(0, &x2, D_ovl0D_03_80048188);
    func_ovl0D_03_80043300(2, &z2, D_ovl0D_03_80048188);
    func_ovl0D_03_80043300(0, &x3, D_ovl0D_03_80048188);
    func_ovl0D_03_80043300(2, &z3, D_ovl0D_03_80048188);
    func_ovl0D_03_80046A30(x0, y0, z0, x1, y1, z1, 0x10);
    func_ovl0D_03_80046A28(x0, y0 + 1, z0, x1, z1, 0x11);
    func_ovl0D_03_80046A28(x0, y0, z0, x1, z1, 0xF);
    func_ovl0D_03_80046A28(x2, y0, z2, x3, z3, 0x11);
    func_ovl0D_03_80046A28(x2, y0 + 1, z2, x3, z3, 0x10);
}
#elif STATICS_PART == 1
    func_ovl0D_03_80044A3C();
#elif STATICS_PART == 2
static void func_ovl0D_03_800441BC(Trio_0D03 *t, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        func_ovl0D_03_80047A48(t->unk0, t->unk4, t->unk8);
        t++;
    }
}
#elif STATICS_PART == 3
    UNK_MACRO_ovl0D_03_80043770();
#endif
