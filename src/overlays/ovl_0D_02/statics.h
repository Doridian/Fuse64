/*
 * Without STATICS_PART: the statics below, included before the functions.
 * With it: the runs of code (definitions, or statements inside a function)
 * 19C2D0.c includes where they are, from this same file: the `.file`
 * switches around them end as1's scheduling blocks, as the original's do.
 */
#ifndef STATICS_PART
/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0D_02_800438D8(void) {
    Ent_0D02 *p;

    func_ovl0D_02_80046D10(D_ovl0D_02_80047A3C);
    for (p = D_ovl0D_02_8004762C; p < D_ovl0D_02_8004762C + 2; p++) {
        if (p->obj != NULL) {
            func_ovl0D_02_80043858(p);
        }
    }
}

static void func_ovl0D_02_80043508(void) {
    UNK_MACRO_ovl0D_02_80043510();
}

/* the probe: an invisible object moved about to test for hits */
static void func_ovl0D_02_800444EC(void) {
    D_ovl0D_02_80047A3C = func_ovl0D_02_80046D40(-1, 0x80, 0.0f, 0.0f, 0.0f);
    D_ovl0D_02_80047A3C->unkC = 0x3FFFF;
}
#elif STATICS_PART == 1
    func_ovl0D_02_80044474();
#elif STATICS_PART == 2
            o->unk40->unk48->unk4->unk4 = func_ovl0D_02_80046A38(o->unk40, 0) - 0.001f;
#elif STATICS_PART == 3
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
    func_ovl0D_02_800430A0(0, &x0, D_ovl0D_02_80047A48);
    func_ovl0D_02_800430A0(1, &y0, D_ovl0D_02_80047A48);
    func_ovl0D_02_800430A0(2, &z0, D_ovl0D_02_80047A48);
    func_ovl0D_02_800430A0(0, &x1, D_ovl0D_02_80047A48);
    func_ovl0D_02_800430A0(1, &y1, D_ovl0D_02_80047A48);
    func_ovl0D_02_800430A0(2, &z1, D_ovl0D_02_80047A48);
    func_ovl0D_02_800430A0(0, &x2, D_ovl0D_02_80047A48);
    func_ovl0D_02_800430A0(2, &z2, D_ovl0D_02_80047A48);
    func_ovl0D_02_800430A0(0, &x3, D_ovl0D_02_80047A48);
    func_ovl0D_02_800430A0(2, &z3, D_ovl0D_02_80047A48);
    func_ovl0D_02_80046340(x0, y0, z0, x1, y1, z1, 0x10);
    func_ovl0D_02_80046338(x0, y0 + 1, z0, x1, z1, 0x12);
    func_ovl0D_02_80046338(x0, y0, z0, x1, z1, 0xF);
    func_ovl0D_02_80046338(x2, y0, z2, x3, z3, 0x12);
    func_ovl0D_02_80046338(x2, y0 + 1, z2, x3, z3, 0x10);
}

/* the clamp: v's parameter before size's (see the top) */
static void func_ovl0D_02_800430A0(s32 axis, s32 *v, s32 *size) {
    s32 max;

    switch (axis) {
        case 0:
            max = size[0];
            break;
        case 1:
            max = size[1];
            break;
        case 2:
            max = size[2];
            break;
    }
    if (*v < 0) {
        *v = 0;
    } else if (*v >= max) {
        *v = max - 1;
    }
}

/* the models' state from the flags already set */
void func_ovl0D_02_80043010(void) {
    f32 y;

    if (func_ovl0D_02_800470D0(0x407) != 0) {
        func_ovl0D_02_80046D00(D_ovl0D_02_8004762C[0].obj, 0x01000000);
        y = -105.0f;
        PORT_HOST(D_ovl0D_02_8004762C[1], D_ovl0D_02_80047664[0]).unk18 = y;
        D_ovl0D_02_80047664[0].obj->unk40->unk14 = y * 10.0f;
        func_ovl0D_02_80046D00(D_ovl0D_02_8004769C[0].obj, 0x01000000);
    }
    if (func_ovl0D_02_800470D0(0x408) != 0) {
        func_ovl0D_02_80046D00(D_ovl0D_02_80047664[0].obj, 0x01000000);
    }
}

#endif
