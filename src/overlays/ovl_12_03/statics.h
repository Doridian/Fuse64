/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

/* Inlined into func_ovl12_03_80043174 only; that one is inlined into its
 * callers in turn. Called from more than one function it isn't inlined.
 * The parameter order sets func_ovl12_03_80043408's frame. Defined in
 * this file, as ovl_12_01's func_ovl12_01_80043188: the
 * `.file` at the inlined body ends as1's block after the prologue and the
 * hoisted &D_ovl12_03_80045ED0, 100.0f and count (17 words to 0). */
static void func_ovl12_03_8004316C(f32 x, s32 z, s32 count) {
    s32 i;

    D_ovl12_03_80045ED0.x = x * 100.0f;
    D_ovl12_03_80045ED0.y = 0.0f;
    for (i = 0; i < count; i++) {
        D_ovl12_03_80045ED0.y += func_ovl12_03_80044950(i) * 100.0f;
    }
    D_ovl12_03_80045ED0.y += 100.0f;
    D_ovl12_03_80045ED0.z = z * 100.0f;
    D_ovl12_03_80045EC0 = func_ovl12_03_80045890(0x28, &D_ovl12_03_80045ED0, 0);
}

/* Here: its inlined (empty) body's `.file` ends the block before
 * the load of D_ovl12_03_80045EC0, so `sw ra` comes first (2 words to 0). */
static void func_ovl12_03_80043100(void) {
}
