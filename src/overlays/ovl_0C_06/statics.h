/*
 * Code from another file than 182450.c (its `.file` switches end as1's
 * scheduling blocks): without STATICS_PART the statics, included before
 * the functions; with it the runs of code (definitions, or statements inside
 * a function) 182450.c includes where they are.
 */
#ifndef STATICS_PART
#elif STATICS_PART == 1
    func_ovl0C_06_80044C0C();
}

void func_ovl0C_06_800450D4(void) {
    func_ovl0C_06_80045094();
    func_ovl0C_06_80047CA0(func_ovl0C_06_80047550);
    func_ovl0C_06_80047CF0(func_ovl0C_06_80047490);
}

void func_ovl0C_06_80045094(void) {
    func_ovl0C_06_8004508C();
    func_ovl0C_06_80048988(0x400);
    func_ovl0C_06_80048988(0x401);
    func_ovl0C_06_80048988(0x402);
    func_ovl0C_06_80048988(0x403);
    func_ovl0C_06_80048988(0x404);
}

static void func_ovl0C_06_8004508C(void) {
}

void func_ovl0C_06_80044F44(void) {
    UnkStruct_ovl0C_06_80048F88 *p;
    UnkStructA *obj;
    s16 i;

    func_ovl0C_06_80043060();
    for (i = 0; i < 1; i++) {
        p = &D_ovl0C_06_80048F88[i];
        obj = func_ovl0C_06_80048600(0x2AC, 0xD0, p->unk10, p->unk1C, p->unk18);
        func_ovl0C_06_800485B8(obj, 0x01000000);
        p->unk8 = obj;
        p->unk44 = 0;
        obj->unk30 = 4;
        obj->unk34 = 100.0f;
        obj->unk38 = 100.0f;
        func_ovl0C_06_80047BF8(p->unk34, p->unk36, p->unk38, p->unk3A, p->unk3E, 0x11);
        D_ovl0C_06_80048F88[i].unkC = func_ovl0C_06_800485E0(-1, 0, 0x50, 0);
        D_ovl0C_06_80048F88[i].unkC->unk40->unk10 = D_ovl0C_06_80048F88[i].unk10;
        D_ovl0C_06_80048F88[i].unkC->unk40->unk14 = D_ovl0C_06_80048F88[i].unk1C - 100.0f;
        D_ovl0C_06_80048F88[i].unkC->unk40->unk18 = D_ovl0C_06_80048F88[i].unk18;
    }
}

void func_ovl0C_06_80044DA0(void) {
    UnkStruct_ovl0C_01_8004A104 *p;
    f32 angle;
    UnkStructA *obj;
    s16 i;

    for (i = 0; i < 1; i++) {
        func_ovl0C_06_80044CE4(i);
        p = &D_ovl0C_06_80048F10[i];
        obj = func_ovl0C_06_80048600(0x2A9, 0xD0, p->unk8, p->unkC, p->unk10);
        func_ovl0C_06_800485B8(obj, 0x01000000);
        switch (p->unk14) {
            case 0:
                angle = 270.0f;
                break;
            case 0x40:
                angle = 180.0f;
                break;
            case 0x80:
                angle = 90.0f;
                break;
            case 0xC0:
                angle = 0.0f;
                break;
        }
        obj->unk40->unk20 = angle;
        p->unk4 = obj;
        func_ovl0C_06_80044C88(p);
    }
}

void func_ovl0C_06_80044CE4(s32 arg0) {
    D_ovl0C_06_800493C8[arg0] = func_ovl0C_06_800485E0(-1, 0, 0x20, 0);
    D_ovl0C_06_800493C8[arg0]->unk40->unk10 = D_ovl0C_06_80048F10[arg0].unk18;
    D_ovl0C_06_800493C8[arg0]->unk40->unk14 = D_ovl0C_06_80048F10[arg0].unk1C;
    D_ovl0C_06_800493C8[arg0]->unk40->unk18 = D_ovl0C_06_80048F10[arg0].unk20;
    D_ovl0C_06_800493C8[arg0]->unk30 = 1;
    D_ovl0C_06_800493C8[arg0]->unk34 = 200.0f;
    D_ovl0C_06_800493C8[arg0]->unk38 = 100.0f;
}

void func_ovl0C_06_80044C88(UnkStruct_ovl0C_01_8004A104 *arg0) {
    func_ovl0C_06_80044C80();
    arg0->unk4->unk40->unk48->unk4[0].unk4 = func_ovl0C_06_800482F8(arg0->unk4->unk40, 0) - 0.0001f;
    func_ovl0C_06_80043710(arg0);
}

static void func_ovl0C_06_80044C80(void) {
}

/* the sign by the start */
void func_ovl0C_06_80044C0C(void) {
    UnkStructA *obj;

    obj = func_ovl0C_06_80048600(0x323, 0xC0, D_ovl0C_06_800492AC.unk4, D_ovl0C_06_800492AC.unk8, D_ovl0C_06_800492AC.unkC);
    D_ovl0C_06_800492AC.unk0 = obj;
    obj->unk40->unk20 = 90.0f;
    func_ovl0C_06_800485F0(obj);
    func_ovl0C_06_800485B8(obj, 0x03000010);
}

void func_ovl0C_06_80044BC8(void) {
    func_ovl0C_06_80047D70(0xF, 0, 0x1B);
    func_ovl0C_06_80044808(0, 0, 0, 0.0f, 0);
}

void func_ovl0C_06_80044B60(void) {
    func_ovl0C_06_80048980(0x43A);
    func_ovl0C_06_80048980(0x439);
    func_ovl0C_06_80048980(0x438);
    func_ovl0C_06_80048980(0x437);
    func_ovl0C_06_80047D70(0x24, 4, 4);
    func_ovl0C_06_80044808(1, 0, 0, 0.0f, 0);
}

void func_ovl0C_06_80044B10(void) {
    func_ovl0C_06_80047D70(0xF, 0, 1);
    func_ovl0C_06_80044808(2, 0x80, 0, 0.0f, 4);
}

void func_ovl0C_06_80044AB0(void) {
    func_ovl0C_06_80047D70(0x1B, 4, 0x15);
    func_ovl0C_06_80044808(3, 0, 0xC0, 0.0f, 2);
    func_ovl0C_06_80047D40(0x2D, 0x19);
}

void func_ovl0C_06_80044A4C(void) {
    func_ovl0C_06_80047D70(0x2D, 1, 0xE);
    func_ovl0C_06_80044808(4, 0xC0, 0x40, 0.0f, 1);
    func_ovl0C_06_80047D40(0x2D, 0x1A);
}

void func_ovl0C_06_800449F0(void) {
    func_ovl0C_06_80047D70(0xF, 0, 0x10);
    func_ovl0C_06_80044808(5, 0, 0, 0.0f, 0);
    func_ovl0C_06_80048520();
    func_ovl0C_06_80047D40(0x2D, 0x18);
}

void func_ovl0C_06_800449C8(void) {
    func_ovl0C_06_80044808(6, 0, 0, 0.0f, 0);
}

void func_ovl0C_06_80044950(void) {
    func_ovl0C_06_80047D70(0x2B, 5, 3);
    func_ovl0C_06_80044808(7, 0x40, 0x40, 0.0f, 0);
    func_ovl0C_06_80047D40(0x2D, 0x18);
    func_ovl0C_06_8004491C();
}

void func_ovl0C_06_8004491C(void) {
    func_ovl0C_06_80048520();
    func_ovl0C_06_800484E8(D_ovl0C_06_80049370);
    func_ovl0C_06_80048458(0x1F);
}

void func_ovl0C_06_800448D4(void) {
    func_ovl0C_06_80047D70(0xF, 0, 0x1B);
    func_ovl0C_06_80044808(8, 0, 0, 0.0f, 0);
}

void func_ovl0C_06_80044884(void) {
    func_ovl0C_06_80047D70(0x1B, 4, 0x15);
    func_ovl0C_06_80044808(9, 0, 0xC0, 0.0f, 2);
}

void func_ovl0C_06_80044830(void) {
    func_ovl0C_06_80047D70(0x2D, 1, 0xE);
    func_ovl0C_06_80044808(0xA, 0xC0, 0x40, 0.0f, 1);
}

void func_ovl0C_06_80044808(s32 arg0, s32 arg1, s32 arg2, f32 arg3, s32 arg4) {
    D_ovl0C_06_800493B0.unk0 = arg0;
    D_ovl0C_06_800493B0.unk4 = arg1;
    D_ovl0C_06_800493B0.unk8 = arg2;
    D_ovl0C_06_800493B0.unkC = arg3;
    D_ovl0C_06_800493B0.unk10 = arg4;
}

void func_ovl0C_06_800446FC(void) {
    func_ovl0C_06_800445CC();
    if (!func_ovl0C_06_80048990(0x43B)) {
        func_ovl0C_06_80044494(&D_ovl0C_06_80049264, D_ovl0C_06_8004914C, 1);
        func_ovl0C_06_80044340(D_ovl0C_06_800490FC, 1);
    }
    if (!func_ovl0C_06_80048990(0x43A)) {
        func_ovl0C_06_800442BC(&D_ovl0C_06_80048FD0[0]);
    }
    if (func_ovl0C_06_80048990(0x438) && !func_ovl0C_06_80048990(0x437)) {
        func_ovl0C_06_800442BC(&D_ovl0C_06_80048FD0[1]);
        func_ovl0C_06_8004399C(&D_ovl0C_06_80049030[1]);
    }
    if (func_ovl0C_06_80048990(0x437) && !func_ovl0C_06_80048990(0x436)) {
        func_ovl0C_06_800442BC(&D_ovl0C_06_80048FD0[2]);
    }
    if (D_ovl0C_06_800493B0.unk0 == 3 || D_ovl0C_06_800493B0.unk0 == 4) {
        func_ovl0C_06_80044264();
    }
    if (D_ovl0C_06_800493B0.unk0 == 2) {
        func_ovl0C_06_80044228(D_ovl0C_06_80048F10);
    }
}

void func_ovl0C_06_800445CC(void) {
    f32 angle;

    func_ovl0C_06_80047DC8();
    switch (D_ovl0C_06_800493B0.unk8) {
        case 0:
            angle = 0.0f;
            break;
        case 0x80:
            angle = 180.0f;
            break;
        case 0xC0:
            angle = 90.0f;
            break;
        case 0x40:
            angle = 270.0f;
            break;
        case 1:
        default:
            angle = 360.0f;
            break;
    }
    if (angle != 360.0f) {
        func_ovl0C_06_80048418(angle, 32768.0f, 32768.0f);
    } else {
        func_ovl0C_06_80044584(D_ovl0C_06_800493B0.unkC);
    }
    func_ovl0C_06_80047DB0(D_ovl0C_06_800493B0.unk4);
    func_ovl0C_06_800484B0(45.0f, 45.1f);
}








/* the wall the blocks stand in front of, and the blocks' tiles */
void func_ovl0C_06_80044494(UnkStruct_ovl0C_01_8004A3BC *arg0, UnkStruct_ovl0C_06_8004914C *arg1, s32 arg2) {
    UnkStructA *obj;
    union { s32 i; u32 u; } fl;

    obj = func_ovl0C_06_80048600(0x2BB, 0xD0, arg0->unk4, arg0->unk8, arg0->unkC);
    fl.i = 0x01000000;
    if (!arg2) {
        fl.u = 0x01000000 | (PORT_IP(s32))obj * 0;
    }
    func_ovl0C_06_800485B8(obj, fl.i);
    arg0->unk0 = obj;
    for (; arg1->unk2 != 0; arg1++) {
        func_ovl0C_06_80047BF8(arg1->unk18, arg1->unk1A, arg1->unk1C, arg1->unk18 + 3, arg1->unk1C + 2, 0x11);
        func_ovl0C_06_80047BF8(arg1->unk18, arg1->unk1E, arg1->unk1C, arg1->unk18 + 3, arg1->unk1C + 2, 0x1F);
    }
}

/* the switches' objects, turned if already hit, and their tiles */
void func_ovl0C_06_80044340(UnkStruct_ovl0C_06_800490FC *arg0, s32 arg1) {
    UnkStructA *obj;
    union { s32 i; u32 u; } fl;

    func_ovl0C_06_80043060();
    fl.u = 0x01000000;
    for (; arg0->unk2 != 0; arg0++) {
        obj = func_ovl0C_06_80048600(0x2AE, 0xD0, arg0->unk8, arg0->unkC, arg0->unk10);
        if (!arg1) {
            fl.i = 0x01000000;
        }
        func_ovl0C_06_800485B8(obj, fl.i);
        arg0->unk4 = obj;
        arg0->unk22 = 0;
        func_ovl0C_06_800482C0(obj->unk40, 2);
        if (!func_ovl0C_06_80048990(arg0->unk24)) {
            func_ovl0C_06_800482C8(arg0->unk4->unk40, 0x2AF, 0, 0);
            func_ovl0C_06_800482C8(arg0->unk4->unk40, 0x2AF, 1, 1);
        } else {
            func_ovl0C_06_800482C8(arg0->unk4->unk40, 0x2AF, 0, 2);
            func_ovl0C_06_800482C8(arg0->unk4->unk40, 0x2AF, 1, 3);
        }
        func_ovl0C_06_80047C00(arg0->unk18, arg0->unk1A, arg0->unk1C, arg0->unk18 + 1, arg0->unk1E, arg0->unk1C + 1, 0x1F);
    }
}

void func_ovl0C_06_800442BC(UnkStruct_ovl0C_06_80048FD0 *arg0) {
    UnkStructA *obj;
    UnkStructA *shadow;

    obj = func_ovl0C_06_80048600(0x2A6, 0xD0, arg0->unk4, arg0->unk10, arg0->unkC);
    shadow = func_ovl0C_06_800485F0(obj);
    shadow->unk54 = 2.0f;
    shadow->unk4C = 2.0f;
    shadow->unk40->unk36 = 0x19;
    func_ovl0C_06_800485B8(obj, 0x0D080000);
    arg0->unk0 = obj;
    arg0->unk8 = arg0->unk10;
}

void func_ovl0C_06_80044264(void) {
    D_ovl0C_06_800493AC = func_ovl0C_06_80047DF8(func_ovl0C_06_80047D98());
    func_ovl0C_06_800485B8(D_ovl0C_06_800493AC, 0x80000);
    D_ovl0C_06_800493AC->unk40->unk36 = 0;
}

void func_ovl0C_06_80044228(UnkStruct_ovl0C_01_8004A104 *arg0) {
    arg0->unk4->unk40->unk48->unk4[0].unk4 = 0.0f;
    func_ovl0C_06_80044164(arg0);
}

void func_ovl0C_06_80044164(UnkStruct_ovl0C_01_8004A104 *arg0) {
    func_ovl0C_06_80047C00(arg0->unk24, arg0->unk28, arg0->unk2C, arg0->unk30, arg0->unk34, arg0->unk38, 0x10);
    func_ovl0C_06_80047BF8(arg0->unk24, arg0->unk28, arg0->unk2C, arg0->unk30, arg0->unk38, 0x11);
    func_ovl0C_06_80047BF8(arg0->unk3C, arg0->unk40, arg0->unk44, arg0->unk48, arg0->unk4C, 0x12);
    func_ovl0C_06_80047BF0(arg0->unk50, arg0->unk54, arg0->unk58, arg0->unk60);
    func_ovl0C_06_80047BF0(arg0->unk64, arg0->unk68, arg0->unk6C, arg0->unk74);
}

void func_ovl0C_06_800440B4(void) {
    if (D_ovl0C_06_800493B0.unk0 == 3 || D_ovl0C_06_800493B0.unk0 == 4) {
        func_ovl0C_06_80044028();
    }
    func_ovl0C_06_80043FE0();
    func_ovl0C_06_80043F6C();
    if (D_ovl0C_06_800493B0.unk0 == 6) {
        func_ovl0C_06_80043F34();
    }
    func_ovl0C_06_80043EC8();
}

void func_ovl0C_06_80044028(void) {
    s32 alpha;
    s32 step;

    func_ovl0C_06_80047DA8(1);
    func_ovl0C_06_80048C90(0x45);
    step = 0x11;
    for (alpha = 0; alpha < 0xFF; alpha += step) {
        D_ovl0C_06_800493AC->unk40->unk36 = alpha;
        func_ovl0C_06_80048750();
    }
    D_ovl0C_06_800493AC->unk40->unk36 = 0xFF;
    func_ovl0C_06_800485C0(D_ovl0C_06_800493AC, 0x80000);
    func_ovl0C_06_80047DA8(0);
}

void func_ovl0C_06_80043FE0(void) {
    func_ovl0C_06_80047DB8(D_ovl0C_06_800493B0.unk4, 3, 4.0f, D_ovl0C_06_800493B0.unk10, 0, 1, 1);
}

void func_ovl0C_06_80043F6C(void) {
    UnkStruct_ovl0C_06_80048F88 *p;
    s16 i;

    for (i = 0, p = D_ovl0C_06_80048F88; i < 1; i++, p++) {
        p->unk4 = func_ovl0C_06_80048708(func_ovl0C_06_80046FFC, p, 0, 0, 0x110);
    }
}

void func_ovl0C_06_80043F34(void) {
    func_ovl0C_06_80048708(func_ovl0C_06_800458D8, NULL, 0, 0, 0x110);
}

void func_ovl0C_06_80043EC8(void) {
    s32 i;

    for (i = 0; i < 1; i++) {
        func_ovl0C_06_80048708(func_ovl0C_06_80045FC8, &D_ovl0C_06_80048F10[i], 0, 0, 0x110);
    }
}

void func_ovl0C_06_80043DC0(void) {
    if (func_ovl0C_06_80047D80(0xF, 1, 0x10, 1)) {
        func_ovl0C_06_80043D54(D_ovl0C_06_80048F10);
        func_ovl0C_06_80047CB0(0x2E, 0x12, 0);
    }
    if (func_ovl0C_06_80047D80(0x18, 0x1D, 0x19, 0x1E)) {
        func_ovl0C_06_80047E00(1);
        func_ovl0C_06_80048708(func_ovl0C_06_80045F44, NULL, 0, 0, 0x130);
        func_ovl0C_06_80047D58(0x3C);
        func_ovl0C_06_80047CB0(0x2F, 0x12, 4);
    }
    if (func_ovl0C_06_80047D80(0x2D, 0xE, 0x2E, 0xF)) {
        func_ovl0C_06_80047E00(1);
        func_ovl0C_06_80048708(func_ovl0C_06_80045F44, NULL, 0, 0, 0x130);
        func_ovl0C_06_80047D58(0x3C);
        func_ovl0C_06_80047CB0(0x30, 0x12, 4);
    }
}

void func_ovl0C_06_80043D54(UnkStruct_ovl0C_01_8004A104 *arg0) {
    func_ovl0C_06_80047DA8(1);
    func_ovl0C_06_80048C90(0x4B);
    func_ovl0C_06_800485B8(arg0->unk4, 0x04000000);
    func_ovl0C_06_80048728(60);
    func_ovl0C_06_80048C90(0x46);
    func_ovl0C_06_80043710(arg0);
    func_ovl0C_06_80048988(0x429);
    func_ovl0C_06_80048728(10);
}

/* the movers' switches: a cut scene each */
void func_ovl0C_06_80043B3C(void) {
    if (func_ovl0C_06_80047D80(0xF, 0xF, 0x10, 0x10) && !func_ovl0C_06_80048990(0x43A)) {
        func_ovl0C_06_80043AEC();
        func_ovl0C_06_80047E00(1);
        func_ovl0C_06_80047D28(1);
        func_ovl0C_06_80047D20(0);
        func_ovl0C_06_80047DB8(0x80, 3, 4.0f, 0, 1, 1, 1);
        func_ovl0C_06_80048728(0x1E);
        func_ovl0C_06_80047DE0(0x04000000);
        func_ovl0C_06_8004399C(&D_ovl0C_06_80049030[0]);
        func_ovl0C_06_80048750();
        func_ovl0C_06_80043AB4();
        func_ovl0C_06_80048728(0x5A);
        func_ovl0C_06_80047DA8(1);
        func_ovl0C_06_80047DB8(0, 3, 4.0f, 0, 1, 1, 1);
    }
    if (func_ovl0C_06_80047D80(0x2B, 2, 0x2C, 3) && func_ovl0C_06_80048990(0x437) && !func_ovl0C_06_80048990(0x436)) {
        func_ovl0C_06_80043A5C();
        func_ovl0C_06_80047D28(1);
        func_ovl0C_06_80047D20(0);
        func_ovl0C_06_80047E00(1);
        func_ovl0C_06_80047DB8(0x80, 3, 4.0f, 0, 1, 1, 1);
        func_ovl0C_06_80048728(0x1E);
        func_ovl0C_06_80047DE0(0x04000000);
        func_ovl0C_06_8004399C(&D_ovl0C_06_80049030[2]);
        func_ovl0C_06_80048750();
        func_ovl0C_06_80043914();
        func_ovl0C_06_80048728(0x5A);
        func_ovl0C_06_80047DA8(1);
        func_ovl0C_06_80047DB8(0x20, 3, 4.0f, 0, 1, 1, 1);
    }
}

void func_ovl0C_06_80043AEC(void) {
    func_ovl0C_06_80048518();
    func_ovl0C_06_80048460(0x7F);
    func_ovl0C_06_800483F8(1500.0f, 0.0f, 1600.0f);
    func_ovl0C_06_80048418(0.0f, 45.0f, 2800.0f);
}

void func_ovl0C_06_80043AB4(void) {
    func_ovl0C_06_80048708(func_ovl0C_06_80045BA0, NULL, 0, 0, 0x110);
}

void func_ovl0C_06_80043A5C(void) {
    func_ovl0C_06_80048518();
    func_ovl0C_06_80048460(0x7F);
    func_ovl0C_06_800483F8(4300.0f, 1000.0f, 200.0f);
    func_ovl0C_06_80048418(-45.0f, 45.0f, 2000.0f);
}

void func_ovl0C_06_8004399C(UnkStruct_ovl0C_06_80049030 *arg0) {
    UnkStructA *obj;
    UnkStructA *other;
    UnkStruct_ovl0C_06_80048FD0 *lim;

    obj = func_ovl0C_06_80048600(0x2B2, 0xD0, arg0->unk28, arg0->unk2C, arg0->unk30);
    func_ovl0C_06_800485F0(obj);
    func_ovl0C_06_800485B8(obj, 0x0D000000);
    lim = &D_ovl0C_06_80048FD0[arg0->unk0];
    arg0->unk4 = obj;
    other = lim->unk0;
    if (arg0->unk20 & 2) {
        func_ovl0C_06_8004394C(obj, other, lim->unk14);
    }
}

void func_ovl0C_06_8004394C(UnkStructA *arg0, UnkStructA *arg1, f32 arg2) {
    func_ovl0C_06_80048598(arg0, arg1);
    func_ovl0C_06_800485B8(arg1, 0x200000);
    arg1->unk4C = 0.0f;
    arg1->unk50 = arg2;
    arg1->unk54 = 0.0f;
}

void func_ovl0C_06_80043914(void) {
    func_ovl0C_06_8004390C();
    func_ovl0C_06_80048708(func_ovl0C_06_800455DC, NULL, 0, 0, 0x110);
}

static void func_ovl0C_06_8004390C(void) {
}

void func_ovl0C_06_80043868(void) {
    UNK_MACRO_ovl0C_06_80043820();
    if (!func_ovl0C_06_80048990(0x404) && D_ovl0C_06_800493B0.unk0 == 2) {
        func_ovl0C_06_8004379C(D_ovl0C_06_80048F10);
    }
    if (!func_ovl0C_06_80048990(0x43B)) {
        func_ovl0C_06_80043544(&D_ovl0C_06_80049264, D_ovl0C_06_8004914C, D_ovl0C_06_800490FC, 0x43B);
    }
    func_ovl0C_06_80048980(0x404);
}

void func_ovl0C_06_80043820(void) {
    func_ovl0C_06_80043818();
    D_ovl0C_06_8004938C = func_ovl0C_06_80047D98();
    func_ovl0C_06_80047D78(&D_ovl0C_06_80049380);
    func_ovl0C_06_80047DA0(D_ovl0C_06_8004938C, D_ovl0C_06_80049370);
}
static void func_ovl0C_06_80043818(void) {
}
void func_ovl0C_06_8004379C(UnkStruct_ovl0C_01_8004A104 *arg0) {
    func_ovl0C_06_80047DA8(1);
    func_ovl0C_06_80048728(90);
    func_ovl0C_06_80048C90(0x4B);
    func_ovl0C_06_800485B8(arg0->unk4, 0x04000000);
    func_ovl0C_06_80048728(60);
    func_ovl0C_06_80048C90(0x46);
    func_ovl0C_06_80048728(10);
    func_ovl0C_06_80043710(arg0);
    func_ovl0C_06_80048988(0x429);
    func_ovl0C_06_80047DA8(0);
}

void func_ovl0C_06_80043710(UnkStruct_ovl0C_01_8004A104 *arg0) {
    func_ovl0C_06_80047C00(arg0->unk24, arg0->unk28, arg0->unk2C, arg0->unk30, arg0->unk34, arg0->unk38, 0x1F);
    func_ovl0C_06_80047BF0(arg0->unk50, arg0->unk54, arg0->unk58, arg0->unk5C);
    func_ovl0C_06_80047BF0(arg0->unk64, arg0->unk68, arg0->unk6C, arg0->unk70);
}

/* bombs on the switches: a switch hit turns, and once none is left the
 * blocks crumble */
s32 func_ovl0C_06_80043544(UnkStruct_ovl0C_01_8004A3BC *arg0, UnkStruct_ovl0C_06_8004914C *arg1, UnkStruct_ovl0C_06_800490FC *list, s32 arg3) {
    UnkStructA_40 *pos;
    s32 hit;
    s32 left;
    s32 ret;
    UnkStruct_ovl0C_06_800490FC *saved;

    saved = list;
    D_ovl0C_06_800493C4->unk30 = 4;
    D_ovl0C_06_800493C4->unk34 = 90.0f;
    D_ovl0C_06_800493C4->unk38 = 400.0f;
    left = 0;
    for (; list->unk2 != 0; list++) {
        if (!func_ovl0C_06_80048990(list->unk24)) {
            D_ovl0C_06_800493C4->unk40->unk10 = list->unk8;
            D_ovl0C_06_800493C4->unk40->unk14 = list->unkC;
            D_ovl0C_06_800493C4->unk40->unk18 = list->unk10;
            hit = func_ovl0C_06_80048568(D_ovl0C_06_800493C4, 0xC0);
            if (hit) {
                if (hit & 0x6000) {
                    func_ovl0C_06_80048CB0(0x49, list->unk8, list->unkC, list->unk10);
                    func_ovl0C_06_800482C8(list->unk4->unk40, 0x2AF, 0, 2);
                    func_ovl0C_06_800482C8(list->unk4->unk40, 0x2AF, 1, 3);
                    func_ovl0C_06_80048980(list->unk24);
                } else {
                    pos = D_ovl0C_06_800493C4->unk40;
                    func_ovl0C_06_80048CB0(0x26, pos->unk10, pos->unk14, pos->unk18);
                    left = 1;
                }
            } else {
                left = 1;
            }
        }
    }
    ret = 0;
    if (!left && !func_ovl0C_06_80048990(arg3)) {
        ret = 1;
        func_ovl0C_06_80048980(arg3);
        func_ovl0C_06_80043354(arg0, arg1, saved, arg3);
    }
    return ret;
}

/* the blocks crumble (a cut scene) */
void func_ovl0C_06_80043354(UnkStruct_ovl0C_01_8004A3BC *arg0, UnkStruct_ovl0C_06_8004914C *arg1, UnkStruct_ovl0C_06_800490FC *arg2, s32 arg3) {
    s32 sparks;
    s32 task;
    UnkStruct_ovl0C_06_8004914C *p;

    func_ovl0C_06_80047E68();
    func_ovl0C_06_80047DA8(1);
    task = func_ovl0C_06_80048708(func_ovl0C_06_80045524, arg1, 0, 0, 0x110);
    func_ovl0C_06_800485D0(arg0->unk0);
    func_ovl0C_06_80043248(arg1, 0x40C);
    sparks = func_ovl0C_06_80048708(func_ovl0C_06_80046BD8, arg2, 0, 0, 0x110);
    func_ovl0C_06_80047D28(1);
    func_ovl0C_06_80048768(task);
    func_ovl0C_06_80048768(sparks);
    func_ovl0C_06_80048780(0xB0, 0xB0);
    for (p = arg1; p->unk2 != 0; p++) {
        func_ovl0C_06_80048B58(0x10, p->unk8 - 300.0f, p->unkC + 200.0f, p->unk10, 0);
        func_ovl0C_06_80048728(0x1E);
    }
    func_ovl0C_06_80048CB0(0x42, arg1->unk8, arg1->unkC, arg1->unk10);
    for (p = arg1; p->unk2 != 0; p++) {
        func_ovl0C_06_80048708(func_ovl0C_06_8004696C, p, 0, 0, 0x110);
        func_ovl0C_06_80048728(0xF);
    }
    func_ovl0C_06_80048728(0x3C);
    func_ovl0C_06_80048710(sparks);
    func_ovl0C_06_80048728(0x3C);
    func_ovl0C_06_80047D28(0);
    if (*func_ovl0C_06_80047D08() & 2) {
        func_ovl0C_06_80047DA8(1);
    }
}

/* the blocks' objects, and their tiles made solid */
void func_ovl0C_06_80043248(UnkStruct_ovl0C_06_8004914C *arg0, s32 arg1) {
    UnkStructA *obj;
    union { s32 i; u32 u; } fl;

    if (arg0->unk2 != 0) {
        fl.u = 0x01000000;
        do {
            obj = func_ovl0C_06_80048600(0x2B0, 0xD0, arg0->unk8, arg0->unk14, arg0->unk10);
            if (!arg1) {
            fl.i = 0x01000000;
        }
        func_ovl0C_06_800485B8(obj, fl.i);
            arg0->unk4 = obj;
            arg0->unkC = arg0->unk14;
            func_ovl0C_06_80047BF8(arg0->unk18, arg0->unk1A, arg0->unk1C, arg0->unk18 + 3, arg0->unk1C + 2, 0x11);
            func_ovl0C_06_80047BF8(arg0->unk18, arg0->unk1E, arg0->unk1C, arg0->unk18 + 3, arg0->unk1C + 2, 0x1F);
            arg0++;
        } while (arg0->unk2 != 0);
    }
}

void func_ovl0C_06_800431B8(void) {
    if (func_ovl0C_06_80047D88(3, 1, 0xF, 4, 0x11) && func_ovl0C_06_800430A8(func_ovl0C_06_80047DF8(func_ovl0C_06_80047D98()), D_ovl0C_06_800492AC.unk0)) {
        if (func_ovl0C_06_80048970(0xA4)) {
            func_ovl0C_06_80048320(0x291, 1);
        } else {
            func_ovl0C_06_80048320(0x361, 4);
        }
    }
}

/* whether the player (arg0) faces arg1: probes 50 units ahead with the
 * shared probe object */
s32 func_ovl0C_06_800430A8(UnkStructA *arg0, UnkStructA *arg1) {
    f32 dz;
    f32 dx;

    func_ovl0C_06_80043060();
    dx = func_ovl0C_06_800488B0(arg0->unk40->unk20) * 50.0f;
    dz = func_ovl0C_06_800488B8(arg0->unk40->unk20);
    D_ovl0C_06_800493C4->unk30 = 1;
    D_ovl0C_06_800493C4->unk34 = 30.0f;
    D_ovl0C_06_800493C4->unk38 = 50.0f;
    D_ovl0C_06_800493C4->unk40->unk10 = arg0->unk40->unk10 + dx;
    D_ovl0C_06_800493C4->unk40->unk14 = arg0->unk40->unk14;
    D_ovl0C_06_800493C4->unk40->unk18 = arg0->unk40->unk18 + dz * 50;
    if (func_ovl0C_06_80048648(D_ovl0C_06_800493C4, 0x10) != arg1) {
        return 0;
    }
    return 1;
}

void func_ovl0C_06_80043060(void) {
    if (func_ovl0C_06_80048990(0x400) == 0) {
        D_ovl0C_06_800493C4 = func_ovl0C_06_800485E0(-1, 0, 0x20, 0);
        func_ovl0C_06_80048980(0x400);
    }
}

void func_ovl0C_06_80043040(void) {
    func_ovl0C_06_80047E00(1);
}

void func_ovl0C_06_80043020(void) {
    func_ovl0C_06_80047E00(1);
}

void func_ovl0C_06_80043018(void) {
}

void func_ovl0C_06_80043010(void) {
}


#endif
