/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0D_05_800431F8(void) {
    Ent05 *p;

    func_ovl0D_05_80045610(D_ovl0D_05_8004616C);
    for (p = D_ovl0D_05_80045F2C; p < D_ovl0D_05_80045F2C + 1; p++) {
        if (p->obj != NULL) {
            func_ovl0D_05_8004317C(p);
        }
    }
}
