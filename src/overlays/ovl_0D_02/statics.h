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
