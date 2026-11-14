/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

/* Waits for the object's animation to end; IDO passes obj in s0. */
static void func_ovl0A_03_80043010(Obj03 *obj) {
    while (!(obj->unk40->unkC & 0x40)) {
        func_ovl0A_03_80047F90();
    }
}
