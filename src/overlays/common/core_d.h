/*
 * The last of the overlays' common functions (core.h): it passes an
 * object's position (its 0x40 structure's 0x10..0x18) to the ZeroJump stub
 * 0x2B04 with the caller's number, and returns what that returns.  The -O2
 * overlays (0E_06/07/08/10/14, 17_02..05) include this alone, where their
 * copy is (they have the other three in another order).
 */

s32 CORE_FUNC_D(s32 arg0, void *arg1) {
    UnkStructA_40 *p;

    p = ((UnkStructA *) arg1)->unk40;
    /* A check around compiled-out debug output, as in main_code's copy
     * (80420's func_80265C20): at -O2 the test's block keeps arg1 from
     * being spilled (the condition is a guess). */
    if (p == NULL) {
        DEBUG_PRINT(("p == NULL\n"));
    }
    return CORE_ZJ_2B04(arg0, p->unk10, p->unk14, p->unk18);
}
