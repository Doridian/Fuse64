/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0C_09_80043D00(f32 x, s32 z, s32 count) {
    s32 i;

    D_ovl0C_09_80047B10.x = x * 100.0f;
    D_ovl0C_09_80047B10.y = 0.0f;
    for (i = 0; i < count; i++) {
        D_ovl0C_09_80047B10.y += func_ovl0C_09_80046540(i) * 100.0f;
    }
    D_ovl0C_09_80047B10.y += 100.0f;
    D_ovl0C_09_80047B10.z = z * 100.0f;
    D_ovl0C_09_80047B00 = func_ovl0C_09_80047480(0x26, &D_ovl0C_09_80047B10, 0);
}
