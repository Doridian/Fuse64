/*
 * Code from another file than 1734E0.c (its `.file` switches end as1's
 * scheduling blocks): without STATICS_PART the statics, included before
 * the functions; with it the runs of code (definitions, or statements inside
 * a function) 1734E0.c includes where they are.
 */
#ifndef STATICS_PART
#elif STATICS_PART == 1
    rec = &D_ovl0C_02_80049EEC;
#elif STATICS_PART == 2
    if (func_ovl0C_02_800494A0(0x41A) != 0) {
        func_ovl0C_02_80048E18(D_ovl0C_02_80049EEC.unk4);
        D_ovl0C_02_80049EEC.unk4 = func_ovl0C_02_80048E10(D_ovl0C_02_80049EEC.unk0->unk40, 0, 0xA, 0x400, D_ovl0C_02_80049EEC.unk10, D_ovl0C_02_80049EEC.unk12);
    }
    func_ovl0C_02_800454B4();
}
static void func_ovl0C_02_80045A74(f32 x, s32 z, s32 count) {
    s32 i;

    D_ovl0C_02_8004A100.x = x * 100.0f;
    D_ovl0C_02_8004A100.y = 0;
    for (i = 0; i < count; i++) {
        D_ovl0C_02_8004A100.y += func_ovl0C_02_80048730(i) * 100.0f;
    }
    D_ovl0C_02_8004A100.y += 100.0f;
    D_ovl0C_02_8004A100.z = z * 100.0f;
    D_ovl0C_02_8004A0F0 = func_ovl0C_02_80049670(0x28, &D_ovl0C_02_8004A100.x, 0);
}
#endif
