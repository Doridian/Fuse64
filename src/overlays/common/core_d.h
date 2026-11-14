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
    if (1) { /* fake: a uopt block boundary; at -O2 a one-block body homes arg1 to the stack (do/while(0) and goto work alike; an initializer, one expression, a copy of arg0, a struct-field array or returning the stub's value do not); at -O3 it changes nothing */
    }
    return CORE_ZJ_2B04(arg0, p->unk10, p->unk14, p->unk18);
}
