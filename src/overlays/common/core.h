/*
 * Code every overlay but ovl_16_01 was built with: four functions the
 * overlays compile from one source, byte for byte the same in all of them
 * (but for relocations).  IDO -O3 emits a file's functions last to first,
 * so they come out together at the end of the overlay's functions (only
 * its unused statics' stubs after them), unless one of them is inlined:
 * then that one comes out just before its first inliner.  Each overlay's
 * file includes this before its own functions.
 *
 * Symbols are named for their addresses, which differ per overlay, so
 * before the include an overlay defines the names these have there:
 *   CORE_FUNC_A .. CORE_FUNC_D  the functions, in definition order
 *   CORE_BSS_A0, CORE_BSS_A1    two s32 in .bss (CORE_FUNC_A sets them)
 *   CORE_BSS_B                  a Vec3f in .bss (CORE_FUNC_B reads it)
 *
 * The ZeroJump stubs it calls (0x1704, 0x1708, 0x1709, 0x1904; core_d.h's
 * 0x2B04) are the overlay's ZJ_<id> names, declared by
 * overlays/common/zerojump.h, which the overlay includes before this.
 *
 * CORE_FUNC_D is core_d.h, which this includes last; an overlay that
 * defines CORE_D_LATE includes it itself, further down (ovl_15_01: its
 * copy is emitted first, so defined last).
 */

void CORE_FUNC_A(void) {
    CORE_BSS_A0 = -1;
    CORE_BSS_A1 = -1;
}

void CORE_FUNC_B(f32 arg0, f32 arg1, f32 arg2) {
    ZJ_1704(CORE_BSS_B.x, CORE_BSS_B.y, CORE_BSS_B.z);
    ZJ_1708(arg0, arg1, arg2);
    ZJ_1904(180);
}

void CORE_FUNC_C(f32 arg0) {
    f32 sp1C;

    ZJ_1709(&sp1C, NULL, NULL);
    ZJ_1708(sp1C + arg0, 32768.0f, 32768.0f);
}

#ifndef CORE_D_LATE
#include "overlays/common/core_d.h"
#endif
