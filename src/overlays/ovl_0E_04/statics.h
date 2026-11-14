/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

/* Inlined into func_ovl0E_04_800439A4 only, which is inlined into its
 * callers in turn; in this file for the block boundary where its body
 * starts (ovl_0C_09's func_ovl0C_09_80043D00). */
static void func_ovl0E_04_8004399C(f32 x, s32 z, s32 count) {
    s32 i;

    D_ovl0E_04_80046F30.x = x * 100.0f;
    D_ovl0E_04_80046F30.y = 0.0f;
    for (i = 0; i < count; i++) {
        D_ovl0E_04_80046F30.y += func_ovl0E_04_800457B0(i) * 100.0f;
    }
    D_ovl0E_04_80046F30.y += 100.0f;
    D_ovl0E_04_80046F30.z = z * 100.0f;
    D_ovl0E_04_80046F20 = func_ovl0E_04_800466F0(0x25, &D_ovl0E_04_80046F30.x, 0);
}

/* here: a block boundary before the load in func_ovl0E_04_80043158 */
static void func_ovl0E_04_80043150(void) {
}
