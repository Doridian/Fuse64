/*
 * The last of the overlays' common functions (core.h): it passes an
 * object's position (its 0x40 structure's 0x10..0x18) to the ZeroJump stub
 * 0x2B04 with the caller's number, and returns what that returns.
 */

s32 CORE_FUNC_D(s32 arg0, void *arg1) {
    UnkStructA_40 *p;

    p = ((UnkStructA *) arg1)->unk40;
    return CORE_ZJ_2B04(arg0, p->unk10, p->unk14, p->unk18);
}
