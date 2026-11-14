/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

/* Here: its inlined (empty) body's `.file` ends the block before
 * the load of D_ovl12_01_800464E0, so `sw ra` comes first (2 words to 0). */
static void func_ovl12_01_8004374C(void) {
}

/* Inlined into func_ovl12_01_80043190 only; that one is inlined into its
 * callers in turn (ovl_12_03's func_ovl12_03_8004316C). Defined in this
 * file: the `.file` at the inlined body ends as1's block after
 * the prologue and the hoisted &D_ovl12_01_800464F0, 100.0f and count, as
 * in the original (17 words to 0). */
static void func_ovl12_01_80043188(f32 x, s32 z, s32 count) {
    s32 i;

    D_ovl12_01_800464F0[0] = x * 100.0f;
    D_ovl12_01_800464F0[1] = 0.0f;
    for (i = 0; i < count; i++) {
        D_ovl12_01_800464F0[1] += func_ovl12_01_80044FA0(i) * 100.0f;
    }
    D_ovl12_01_800464F0[1] += 100.0f;
    D_ovl12_01_800464F0[2] = z * 100.0f;
    D_ovl12_01_800464E0 = func_ovl12_01_80045EE0(0x28, (Vec3f *) D_ovl12_01_800464F0, 0);
}
