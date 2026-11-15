/*
 * Without STATICS_PART: a static the original defined in another file;
 * with it: the run of statements 2BC0B0.c includes in place (the speeds
 * case 4 sets). The `.file` switch back ends as1's scheduling block, so
 * the next call isn't scheduled into the stores; a first switch to a file
 * ugen hasn't seen yet ends none, so the static comes first.
 */
#ifndef STATICS_PART
static void func_ovl17_10_80044E40(void) {
    D_ovl17_10_800466F0[0] = 0;
}
#elif STATICS_PART == 1
            D_ovl17_10_80046490 = 0.3f;
            D_ovl17_10_80046494 = 0.5f;
            D_ovl17_10_80046498 = 0;
#endif
