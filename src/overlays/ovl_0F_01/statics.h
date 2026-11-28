/*
 * Code from another file than 1E2D80.c (its `.file` switches end as1's
 * scheduling blocks): without STATICS_PART the statics, included before
 * the functions; with it the runs of code (definitions, or statements inside
 * a function) 1E2D80.c includes where they are.
 */
#ifndef STATICS_PART
static void func_ovl0F_01_80045FB0(u32 (*p)[3], s32 count) {
    s32 i;

    func_ovl0F_01_80045FA0();
    for (i = 0; i < count; i++) {
        func_ovl0F_01_80047E98((*p)[0], (*p)[1], (*p)[2]);
        p++;
    }
}
#elif STATICS_PART == 1
            break;
        case 16:
            func_ovl0F_01_80046FF0(0x18, 3, 0x10);
            break;
        case 17:
            func_ovl0F_01_80046FF0(0xD, 0, 8);
            break;
        case 18:
            func_ovl0F_01_80047030(0x40);
            func_ovl0F_01_80046FF0(6, 1, 0x1A);
            break;
        case 20:
            func_ovl0F_01_80047698(315.0f, 32768.0f, 32768.0f);
            func_ovl0F_01_80047030(0xC0);
            func_ovl0F_01_80046FF0(0x2D, 1, 0x18);
            break;
        case 21:
            func_ovl0F_01_80047C68(1);
            func_ovl0F_01_80047030(0x40);
            func_ovl0F_01_80046FF0(1, 4, 0xC);
            func_ovl0F_01_8004324C();
            func_ovl0F_01_80046FC0(0x58, 0x17);
            break;
        case 22:
            func_ovl0F_01_80047C68(1);
            func_ovl0F_01_80047030(0xC0);
            func_ovl0F_01_80046FF0(0x2E, 4, 0xB);
            func_ovl0F_01_8004324C();
            func_ovl0F_01_80046FC0(0x58, 0x18);
            break;
        case 23:
            func_ovl0F_01_80046FF0(0xE, 0, 0xC);
            break;
        case 24:
            func_ovl0F_01_80046FF0(0x21, 0, 0xB);
            break;
        case 25:
            func_ovl0F_01_80047030(0);
            func_ovl0F_01_80046FF0(0x1B, 0, 0xE);
            break;
        case 4:
            func_ovl0F_01_80047048();
            func_ovl0F_01_80046F20(func_ovl0F_01_80043348);
            func_ovl0F_01_800476D8(0x5F);
            func_ovl0F_01_80047728(2200.0f, 1600.0f, 800.0f, 2000.0f);
            func_ovl0F_01_800477A8(800.0f, 1200.0f);
            func_ovl0F_01_80047738(2400.0f, 1600.0f, 2400.0f, 1600.0f);
            func_ovl0F_01_80047730(70.0f, 55.0f);
            func_ovl0F_01_80047CD8(0xE6, 0xF0, 0xFF);
            func_ovl0F_01_80047CC8(0x3B1, 0x3CA, 0xE6, 0xF0, 0xFF);
            D_ovl0F_01_80048130 = func_ovl0F_01_80047508(0x258);
            obj = func_ovl0F_01_80047598(func_ovl0F_01_8004540C, -60);
            D_ovl0F_01_8004878C = obj;
            obj->unkC |= 0x210;
            func_ovl0F_01_8004323C();
            func_ovl0F_01_80047780(&D_ovl0F_01_80048928);
            D_ovl0F_01_8004813C = 0.0f;
            D_ovl0F_01_80048140 = 0.0f;
            D_ovl0F_01_80048144 = 1.0f;
            func_ovl0F_01_80047C00(0x407);
            func_ovl0F_01_80047918(40.0f);
            func_ovl0F_01_80047920(30);
            if (D_ovl0F_01_80048790->unk0 & 4) {
                func_ovl0F_01_80047698(135.0f, 32768.0f, 32768.0f);
                func_ovl0F_01_80047988(func_ovl0F_01_80043298, NULL, 0, 0, 0x130);
                func_ovl0F_01_800476E0(0xF);
                func_ovl0F_01_800476F0(1);
            }
            break;
        case 5:
            switch (func_ovl0F_01_80046F38()) {
                case 18:
                    func_ovl0F_01_800430F8(&D_ovl0F_01_80048508[0]);
                    break;
                case 20:
                    func_ovl0F_01_800430F0(&D_ovl0F_01_80048508[1]);
                    break;
                case 21:
                    func_ovl0F_01_800430E8(&D_ovl0F_01_80048508[2]);
                    break;
                case 22:
                    func_ovl0F_01_800430E0(&D_ovl0F_01_80048508[3]);
                    break;
            }
            break;
        case 0x50:
            func_ovl0F_01_80043094(D_ovl0F_01_800484D0);
            func_ovl0F_01_80043094(D_ovl0F_01_800484EC);
            break;
        case 0x52:
            if (func_ovl0F_01_80047000(1, 0x15, 1, 0x15) != 0 && func_ovl0F_01_80047C10(0x406) == 0) {
                func_ovl0F_01_80047C00(0x406);
                if (func_ovl0F_01_800476C8() == 1) {
                    func_ovl0F_01_800476D0();
                }
                func_ovl0F_01_80047798();
                func_ovl0F_01_800476E0(0x1F);
                func_ovl0F_01_80047698(271.0f, 50.4f, 1728.0f);
                func_ovl0F_01_80047688(-627.4f, 2685.0f, 3200.3f);
                func_ovl0F_01_80047678(447.4f, 1425.9f, 2032.76f);
                func_ovl0F_01_80043030(30, D_ovl0F_01_800485B0);
            }
            break;
        case 0x53:
            if (func_ovl0F_01_80047000(2, 0x15, 2, 0x15) != 0 && func_ovl0F_01_80047C10(0x406) != 0) {
                func_ovl0F_01_80047C08(0x406);
                func_ovl0F_01_800476D8(0x1F);
                func_ovl0F_01_80047668(1);
                func_ovl0F_01_800477A0();
                func_ovl0F_01_80047768(D_ovl0F_01_80048750);
                func_ovl0F_01_80043030(30, D_ovl0F_01_800485D0);
            }
            break;
        case 15:
            func_ovl0F_01_80043028();
            break;
        case 1:
            func_ovl0F_01_80043018(3, D_ovl0F_01_8004836C);
            if (D_ovl0F_01_8004841C != -1) {
                func_ovl0F_01_80047990(D_ovl0F_01_8004841C);
            }
            func_ovl0F_01_80047528(D_ovl0F_01_8004878C);
            func_ovl0F_01_80043018(1, &D_ovl0F_01_8004845C);
            func_ovl0F_01_80047510(0x258);
            if (D_ovl0F_01_80048430 != -1) {
                func_ovl0F_01_80047990(D_ovl0F_01_80048430);
                D_ovl0F_01_80048430 = -1;
            }
            if (D_ovl0F_01_80048790->unk0 & 4) {
                PORT_ONLY(if (func_ovl0F_01_800480D8(2)->unkC != NULL)) func_ovl0F_01_800480D8(2)->unkC->unkC |= 1; /* NULL: a demo left early (docs/PORT.md, "Reads through NULL") */
                func_ovl0F_01_80047CD8(0, 0, 0);
                func_ovl0F_01_800480C8();
                q = func_ovl0F_01_800480D8(4);
                if (q->unkC != NULL) {
                    func_ovl0F_01_80047520(q->unkC);
                }
                q->unkC = NULL;
                func_ovl0F_01_800480F8();
            }
            func_ovl0F_01_80043020();
            break;
        case 12:
            func_ovl0F_01_80047080(1);
            break;
        case 3:
            D_ovl0F_01_80048434 = 1;
#elif STATICS_PART == 2
/* 80043B0C's: the platform's position put back */
static void func_ovl0F_01_80043AFC(UnkStructA *obj, f32 *sv) {
    obj->unk40->unk10 = sv[0];
    obj->unk40->unk14 = sv[1];
    obj->unk40->unk18 = sv[2];
}

/* 80043B0C's: the platform's position kept */
static void func_ovl0F_01_80043AF4(UnkStructA *obj, f32 *sv) {
    sv[0] = obj->unk40->unk10;
    sv[1] = obj->unk40->unk14;
    sv[2] = obj->unk40->unk18;
}
#endif
