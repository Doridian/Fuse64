/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0A_02_80043068(Obj02 *obj) {
    obj->unk40->unk48->unk4->unk4 = func_ovl0A_02_80047738(obj->unk40, 0) - 0.001f;
}
