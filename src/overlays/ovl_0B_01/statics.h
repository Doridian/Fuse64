/*
 * Statics the original defined in another file: where one is inlined, the
 * `.file` ugen emits for it ends an as1 scheduling block, which the
 * original's schedule has.  That file's name and what else it held are
 * unknown; several overlays' copies of these statics are the same code.
 */

static void func_ovl0B_01_80044644(UnkStruct_ovl0B_01_80045E34 *fade, Gfx_80225840 **arg1) {
    Gfx_80225840 *gfx = *arg1;

    { Gfx_80225840 *g = gfx++; g->w0 = 0xFA000000; g->w1 = ((fade->unk4 & 0xFF) << 24) | ((fade->unk8 & 0xFF) << 16) | ((fade->unkC & 0xFF) << 8) | (fade->unk10 & 0xFF); }
    { Gfx_80225840 *g = gfx++; g->w0 = 0xFCFFFFFF; g->w1 = 0xFFFDF6FB; }
    { Gfx_80225840 *g = gfx++; g->w0 = 0xF7000000; g->w1 = 0x10001; }
    { Gfx_80225840 *g = gfx++; g->w0 = 0xF6000000 | (((fade->unk2C + (s32) fade->unk34) & 0x3FF) << 14) | (((fade->unk30 + (s32) (fade->unk34 * fade->unk20)) & 0x3FF) << 2); g->w1 = (((fade->unk2C - (s32) fade->unk34) & 0x3FF) << 14) | (((fade->unk30 - (s32) (fade->unk34 * fade->unk20)) & 0x3FF) << 2); }
    *arg1 = gfx;
}
