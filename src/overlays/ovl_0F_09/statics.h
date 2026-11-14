/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0F_09_80043390(void) {
    D_ovl0F_09_80046200[4]->unk40->unk14 -= 3200.0f;
    D_ovl0F_09_80046200[5]->unk40->unk14 -= 3200.0f;
    D_ovl0F_09_80046200[6]->unk40->unk10 += 50.0f;
}
