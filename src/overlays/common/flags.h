/*
 * Two functions the 0x0C and 0x0E banks' overlays share, side by side and
 * the same bytes (but for relocations) in 25 of them: one passes 0x400 to
 * 0x404 to a ZeroJump stub (the numbers other overlays test as flags), the
 * other calls it and registers two of the overlay's functions.  The -O3
 * ones compile them from this source (written last to first, as the
 * overlays are); ovl_0C_06, 0E_01 and 0E_04 (another form) and the -O2
 * ones (the other order) keep their copies.
 *
 * Names are ovl_0C_01's addresses; an overlay maps them before the include
 * (FLAGS_<addr>, as with core.h).
 */

void FLAGS_8004701C(void) {
    FLAGS_80046FDC();
    FLAGS_80048EA0(FLAGS_80048754);
    FLAGS_80048EF0(FLAGS_80048694);
}

void FLAGS_80046FDC(void) {
    FLAGS_80049B88(0x400);
    FLAGS_80049B88(0x401);
    FLAGS_80049B88(0x402);
    FLAGS_80049B88(0x403);
    FLAGS_80049B88(0x404);
}
