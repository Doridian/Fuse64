/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0D_04_800446C0(void) {
    Ent_0D04 *p;

    func_ovl0D_04_80046320(D_ovl0D_04_8004705C);
    for (p = D_ovl0D_04_80046C24; p < D_ovl0D_04_80046C24 + 3; p++) {
        if (p->obj != NULL) {
            func_ovl0D_04_80044644(p);
        }
    }
}

static void func_ovl0D_04_80044D40(void) {
    func_ovl0D_04_800461F8(2400.0f, 1600.0f, 800.0f, 1600.0f);
    func_ovl0D_04_80046208(1600.0f, 1600.0f, 3200.0f, 1600.0f);
}

/* the tiles under a gate: solid, and the frame around its opening */
static void func_ovl0D_04_80043F24(Ent_0D04 *e) {
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
    Obj40_0D04 *p;

    p = e->obj->unk40;
    func_ovl0D_04_80045960(p->unk10, p->unk14, p->unk18, grid, 0);
    len = x1 + z1; /* fake: (see the top) */
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
    switch (func_ovl0D_04_80043F1C(e)) {
        case 0:
            /* fake: (see the top) */
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
    func_ovl0D_04_80043EA8(0, &x0, D_ovl0D_04_80047068);
    func_ovl0D_04_80043EA8(1, &y0, D_ovl0D_04_80047068);
    func_ovl0D_04_80043EA8(2, &z0, D_ovl0D_04_80047068);
    func_ovl0D_04_80043EA8(0, &x1, D_ovl0D_04_80047068);
    func_ovl0D_04_80043EA8(1, &y1, D_ovl0D_04_80047068);
    func_ovl0D_04_80043EA8(2, &z1, D_ovl0D_04_80047068);
    func_ovl0D_04_80043EA8(0, &x2, D_ovl0D_04_80047068);
    func_ovl0D_04_80043EA8(2, &z2, D_ovl0D_04_80047068);
    func_ovl0D_04_80043EA8(0, &x3, D_ovl0D_04_80047068);
    func_ovl0D_04_80043EA8(2, &z3, D_ovl0D_04_80047068);
    func_ovl0D_04_80045950(x0, y0, z0, x1, y1, z1, 0x10);
    func_ovl0D_04_80045948(x0, y0 + 1, z0, x1, z1, 0x11);
    func_ovl0D_04_80045948(x0, y0, z0, x1, z1, 0xF);
    func_ovl0D_04_80045948(x2, y0, z2, x3, z3, 0x11);
    func_ovl0D_04_80045948(x2, y0 + 1, z2, x3, z3, 0x10);
}
