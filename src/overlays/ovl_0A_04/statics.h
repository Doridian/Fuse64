/*
 * Without STATICS_PART: a static the original defined in another file
 * (the animation wait, as in ovl_0A_02, 03 and 05); with it: the run of
 * statements 137330.c includes in place. The `.file` switches end as1's
 * scheduling blocks, as the original's do.
 */
#ifndef STATICS_PART
static void func_ovl0A_04_800432F0(UnkStructA *obj) {
    while (!(obj->unk40->unkC & 0x40)) {
        func_ovl0A_04_80045EA0();
    }
}
#elif STATICS_PART == 1
    obj->unk40->unk10 = 200.0f;
    obj->unk40->unk14 = 0.0f;
    obj->unk40->unk18 = -2000.0f;
#endif
