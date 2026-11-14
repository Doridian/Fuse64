/*
 * Code 274480.c includes where it is, from this same file (docs/DECOMPILING.md,
 * "The overlays' shared source"): the `.file` switches around a part end as1's
 * scheduling blocks, as the original's do. A switch back to a file ugen has
 * already seen ends a block; a first switch to a new one doesn't, so part 1
 * (emitted before part 2) is what makes part 2's switch count.
 */
#if STATICS_PART == 1
/* 80043250: the animation set, as 800437B8's store below reads it */
void func_ovl14_01_80043250(s32 arg0) {
    func_ovl14_01_8004C5C0(D_ovl14_01_8004D5DC->unk40, arg0, 0);
    D_ovl14_01_8004D558 = (s32) (func_ovl14_01_8004C5D8(D_ovl14_01_8004D5DC->unk40, 0) - 0.001f);
    func_ovl14_01_8004C8A0(D_ovl14_01_8004D5DC, 0x04000000);
}
#elif STATICS_PART == 2
/* 800437B8's frame store (in place; what the original's file held here is
 * not known) */
        temp_s0->unk40->unk48->unk4->unk4 = func_ovl14_01_8004C5D8(temp_s0->unk40, 0) - 0.001f;
#endif
