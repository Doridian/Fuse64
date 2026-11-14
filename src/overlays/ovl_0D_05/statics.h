/*
 * Without STATICS_PART: the statics below, included before the functions.
 * With it: the runs of code (definitions, or statements inside a function)
 * 1A9E30.c includes where they are, from this same file: the `.file`
 * switches around them end as1's scheduling blocks, as the original's do.
 */
#ifndef STATICS_PART
/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0D_05_800431F8(void) {
    Ent05 *p;

    func_ovl0D_05_80045610(D_ovl0D_05_8004616C);
    for (p = D_ovl0D_05_80045F2C; p < D_ovl0D_05_80045F2C + 1; p++) {
        if (p->obj != NULL) {
            func_ovl0D_05_8004317C(p);
        }
    }
}
#elif STATICS_PART == 1
static void func_ovl0D_05_80043788(void) {
}

static void func_ovl0D_05_80043780(Trio05 *t, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        func_ovl0D_05_80045C58(t->unk0, t->unk4, t->unk8);
        t++;
    }
}

static void func_ovl0D_05_80043760(void) {
    func_ovl0D_05_800454E8(2400.0f, 1600.0f, 800.0f, 1600.0f);
    func_ovl0D_05_800454F8(1600.0f, 1600.0f, 3200.0f, 1600.0f);
}

#elif STATICS_PART == 2
        j = D_ovl0D_05_8004617C = func_ovl0D_05_80044CC8(0x15A);
#endif
