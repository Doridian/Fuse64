/*
 * Code from another file than 2B8D20.c (its `.file` switches end as1's
 * scheduling blocks): without STATICS_PART the statics, included before
 * the functions; with it the runs of code (definitions, or statements inside
 * a function) 2B8D20.c includes where they are.
 */
#ifndef STATICS_PART
#elif STATICS_PART == 1
    if (func_ovl17_09_80045048(obj->unk40) == obj->unk40->unk14) {
        func_ovl17_09_80045FD8(obj, dir, speed);
    } else {
        switch (dir) {
            case 0x00:
                func_ovl17_09_80044554(obj, speed);
                break;
            case 0x20:
                func_ovl17_09_80044564(obj, speed);
                func_ovl17_09_80044554(obj, speed);
                break;
            case 0x40:
                func_ovl17_09_80044564(obj, speed);
                break;
            case 0x60:
                func_ovl17_09_8004455C(obj, speed);
                func_ovl17_09_80044564(obj, speed);
                break;
            case 0x80:
                func_ovl17_09_8004455C(obj, speed);
                break;
            case 0xA0:
                func_ovl17_09_8004455C(obj, speed);
                func_ovl17_09_8004454C(obj, speed);
                break;
            case 0xC0:
                func_ovl17_09_8004454C(obj, speed);
                break;
            case 0xE0:
                func_ovl17_09_80044554(obj, speed);
                func_ovl17_09_8004454C(obj, speed);
                break;
        }
    }
#elif STATICS_PART == 2
    func_ovl17_09_80043010();
}

static void func_ovl17_09_80043134(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (D_ovl17_09_800462DC[i].unk0 != NULL) {
            func_ovl17_09_800459A0(D_ovl17_09_800462DC[i].unk0);
            D_ovl17_09_800462DC[i].unk0 = NULL;
        }
    }
}

static void func_ovl17_09_80043288(void) {
    if (func_ovl17_09_800450D8()->unk10 == 0) {
        D_ovl17_09_800462A8 = 8.5f;
    } else {
        D_ovl17_09_800462A8 = 15.299999f;
    }
}

static void func_ovl17_09_80043280(void) {
}

/* The overlay's handler: the entry returns it to the loader. */
void func_ovl17_09_80043290(s32 arg0) {
    s32 i;
    s32 t;
    f32 step;
    UnkStructB *b;
    UnkStructA *obj;

    switch (arg0) {
        case 2:
            func_ovl17_09_80045E40(D_ovl17_09_800462B8);
            func_ovl17_09_80044F88(0x217);
            func_ovl17_09_80044F90(0xDA, 0.0f, 0.0f, 0.0f, 0x50, 0);
            func_ovl17_09_80044FA0(0x221);
            func_ovl17_09_80045138();
            func_ovl17_09_80045100(0);
            func_ovl17_09_80045830(0x7F);
            func_ovl17_09_800457A8(1);
            func_ovl17_09_800457B8(1);
            func_ovl17_09_800457C8(2310.0f, 60.0f, 1400.0f);
            func_ovl17_09_800457E8(0.0f, 55.0f, 2864.0f);
            func_ovl17_09_80043288();
            func_ovl17_09_80043280();
            D_ovl17_09_800462A0 = 0;
            D_ovl17_09_800462A4 = 0.0f;
            D_ovl17_09_800462B4 = 1;
            func_ovl17_09_80045068(func_ovl17_09_80044A2C, 5);
            func_ovl17_09_80045068(func_ovl17_09_80044828, 6);
            D_ovl17_09_80046280 = func_ovl17_09_80045658(0x258);
            obj = func_ovl17_09_800456E8(func_ovl17_09_80043750, -60);
            D_ovl17_09_8004656C = obj;
            obj->unkC |= 0x210;
            for (i = 0; i < 20; i++) {
                func_ovl17_09_80043144(&D_ovl17_09_800463D0[i]);
                D_ovl17_09_800463D0[i].unk4 = func_ovl17_09_80045C98(240);
            }
            func_ovl17_09_800458D0(&D_ovl17_09_80046560);
            D_ovl17_09_8004628C = 0.0f;
            D_ovl17_09_80046290 = 0.0f;
            D_ovl17_09_80046294 = 1.0f;
            func_ovl17_09_80045D50(0x407);
            func_ovl17_09_8004313C();
            func_ovl17_09_80045E28(90, 140, 255);
            break;
        case 4:
            func_ovl17_09_80045A68(50.0f);
            func_ovl17_09_80045A70(40);
            break;
        case 5:
            b = func_ovl17_09_80045108();
            if (b->unk38 == 1 && b->unk0 == 0) {
                func_ovl17_09_80045AD8(func_ovl17_09_80044198, NULL, 0, 0, 0x110);
            }
            break;
        case 1:
            func_ovl17_09_80045678(D_ovl17_09_8004656C);
            func_ovl17_09_80045660(0x258);
            func_ovl17_09_80043134();
            func_ovl17_09_80045DD8(0, 0, 0, 0.0f);
            break;
        case 3:
            func_ovl17_09_8004312C();
            if (D_ovl17_09_800462A0 == 0) {
                func_ovl17_09_80043010();
            }
            break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            break;
        case 0x50:
            break;
        case 0x51:
            break;
        case 0x52:
            break;
        case 0x53:
            break;
    }
}

static void func_ovl17_09_80043144(UnkStruct_ovl17_09_800463D0 *p) {
    f32 r;
    f64 t;

    r = func_ovl17_09_80045C90();
    p->unk0 = func_ovl17_09_80045C90() * 370.0 - 50.0;
    t = func_ovl17_09_80045C90();
    p->unk4 = (D_ovl17_09_8004628C < 0 ? -D_ovl17_09_8004628C : D_ovl17_09_8004628C) * 30.0f * t;
    if (D_ovl17_09_80046288 <= 0.0f) {
        p->unk4 += (1.0f - r) * 40.0f;
    } else {
        p->unk4 += func_ovl17_09_80045C98(90);
    }
    p->unk10 = 0;
    p->unk8 = r * 12.0f + 4.0f;
    p->unkC = r * r * 5.0f + 0.5f;
}

static void func_ovl17_09_80043010(void) {
    D_ovl17_09_8004629C = (func_ovl17_09_80045C98(8) << 5) & 0xF0;
    switch (D_ovl17_09_8004629C) {
        case 0x00:
            D_ovl17_09_80046298 = 270.0f;
            break;
        case 0x20:
            D_ovl17_09_80046298 = 315.0f;
            break;
        case 0x40:
            D_ovl17_09_80046298 = 0.0f;
            break;
        case 0x60:
            D_ovl17_09_80046298 = 45.0f;
            break;
        case 0x80:
            D_ovl17_09_80046298 = 90.0f;
            break;
        case 0xA0:
            D_ovl17_09_80046298 = 135.0f;
            break;
        case 0xC0:
            D_ovl17_09_80046298 = 180.0f;
            break;
        case 0xE0:
            D_ovl17_09_80046298 = 225.0f;
            break;
    }
}

#elif STATICS_PART == 3
    pos[0] = obj->unk40->unk10;
    pos[1] = obj->unk40->unk14;
    pos[2] = obj->unk40->unk18;
#endif
