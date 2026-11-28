/*
 * Statics the original defined in another file, included where they were
 * defined: where one is inlined, the `.file` ugen emits for it ends an as1
 * scheduling block, which the original's schedule has. That file's name and
 * what else it held are unknown.
 */
static void func_ovl0F_10_80044B84(void) {
    func_ovl0F_10_80045E98(0.0f);
    func_ovl0F_10_80045EA0(0x7F);
    if (D_ovl0F_10_800466BC != -1) {
        func_ovl0F_10_80046368(D_ovl0F_10_800466BC);
    }
    func_ovl0F_10_80045F28(250);
    func_ovl0F_10_800455D0(1);
    func_ovl0F_10_80045600(1);
    func_ovl0F_10_80045C80(30.0f);
    func_ovl0F_10_80045C88(1.0f);
    func_ovl0F_10_80045CA0(10.0f);
    func_ovl0F_10_800455B8(D_ovl0F_10_800468C0.unk0, D_ovl0F_10_800468C0.unk4, D_ovl0F_10_800468C0.unk8,
                           D_ovl0F_10_800468C0.unkC, D_ovl0F_10_800468C0.unk10, D_ovl0F_10_800468C0.unk14,
                           D_ovl0F_10_800468C0.unk18);
    func_ovl0F_10_80046490(0x8E);
}

/* .data after func_ovl0F_10_80044B7C's static */
s32 D_ovl0F_10_800466C8 = 0;
s32 D_ovl0F_10_800466CC = 0;
s32 D_ovl0F_10_800466D0 = 0;
s32 D_ovl0F_10_800466D4 = 0;
/* passed to func_ovl0F_10_80046270: three 12-byte records */
u32 D_ovl0F_10_800466D8[][3] = {
    { 0x25D0001, 0xE, 4 },
    { 0, 0, 4 },
    { 0, 0, 4 },
};
/* four 12-byte records, each passed to func_ovl0F_10_80046418 */
u32 D_ovl0F_10_800466FC[][3] = {
    { 0, 0x14, 0xF2 },
    { 1, 0x16, 0xF4 },
    { 2, 0x12, 0xF0 },
    { 3, 0x15, 0xF3 },
};
UnkStruct_ovl0F_01_80048264 D_ovl0F_10_8004672C[4] = {
    { &D_ovl0F_10_80046AE0[0], 0x234, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0x80000, 0 },
    { &D_ovl0F_10_80046AE0[1], 0x22F, 0, 63.0f, 0.0f, 25.0f, 0.0f, -90.0f, 0.0f, 0, 0 },
    { &D_ovl0F_10_80046AE0[2], 0x235, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
    { &D_ovl0F_10_80046AE0[3], 0x22E, 0, 63.0f, 5.0f, 25.0f, 0.0f, -90.0f, 0.0f, 0, 0 },
};
UnkStruct_ovl0F_01_80048264 D_ovl0F_10_800467DC[2] = {
    { &D_ovl0F_10_80046AE0[4], 0x1B3, 0, 11.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0, 0x405 },
    { &D_ovl0F_10_80046AE0[5], 0x262, 0, 22.0f, 5.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0, 0 },
};
/* func_ovl0F_10_800432E0's: four arguments to test, three to pass on */
s32 D_ovl0F_10_80046834[7] = { 0x2D, 0x1D, 0x2D, 0x1E, 0x5F, 0x14, 0 };
s32 D_ovl0F_10_80046850[7] = { 0xA, 0x1E, 0xA, 0x1E, 0x60, 0x14, 0 };
UnkStruct_ovl0F_10_800468C0 D_ovl0F_10_8004686C[2] = {
    { 0, 3, 8.5f, 2, 0, 1, 0 },
    { 0, 3, 8.5f, 2, 0, 1, 0 },
};
s32 D_ovl0F_10_800468A4 = 0;
s32 D_ovl0F_10_800468A8 = 0;
s32 D_ovl0F_10_800468AC = 0;
f32 D_ovl0F_10_800468B0 = 0.0f;
s32 D_ovl0F_10_800468B4 = 0;
UnkStructA *D_ovl0F_10_800468B8 = NULL;
s32 D_ovl0F_10_800468BC = -1;
UnkStruct_ovl0F_10_800468C0 D_ovl0F_10_800468C0 = { 0, 3, 3.75f, 15, 0, 0, 0 };
/* the camera scripts func_ovl0F_10_80043E08 passes to func_ovl0F_10_800455C0 */
char D_ovl0F_10_800468DC[] = "WM1 WG1 WSL WAP WDN WB3 WDNE WB1 WDN WB2";
char D_ovl0F_10_80046908[] = "M9 ST90";
char D_ovl0F_10_80046910[] = "WM1 WG1 WSH WAP WDN WB12 WDNW WB7 WDW WB8 WDNW WB3 WDW WB4";
char D_ovl0F_10_8004694C[] = "WM1 WG1 WSL WAP WDSW WB0 ST80";
char D_ovl0F_10_8004696C[] = "M8 ST80";
char D_ovl0F_10_80046974[] = "M7 ST120 WM1 WG1 WSH WAP WDSW WB1";
char D_ovl0F_10_80046998[] = "WM1 WG1 WSL WAP WDSE WB1 WDS WB0 ST60";

/* a task: the trembling flight to the end of the stage */
void func_ovl0F_10_80044AD4(void) {
    UnkStructA *obj;
    s32 i;

    obj = func_ovl0F_10_80045F30();
    for (i = 0; i < 5; i++) {
        func_ovl0F_10_80046358(0x19, obj->unk40->unk10, obj->unk40->unk14, obj->unk40->unk18, 0);
        if (D_ovl0F_10_800466C8 == 1) {
            func_ovl0F_10_80046490(0x27);
        }
        func_ovl0F_10_80045F28(20);
    }
    func_ovl0F_10_80045F18();
}

void func_ovl0F_10_800448A0(void) {
    s32 pad[1]; /* fake: unused, frame space the original has (no source for it found) */
    f32 x;
    f32 y;
    f32 z;
    f32 rx;
    f32 ry;
    f32 rz;
    s32 i;

    x = 2898.7f;
    y = 3582.5f;
    z = 1940.3f;
    rx = 247.7f;
    ry = 1.7f;
    rz = 344.0f;
    for (i = 0; i < 300; i++) {
        x += -2.7783334f;
        z += -1.1396668f;
        func_ovl0F_10_80045BF8(x, 32768.0f, z);
        func_ovl0F_10_80045F50();
    }
    func_ovl0F_10_80045F28(110);
    for (i = 0; i < 500; i++) {
        rx += 0.18460001f;
        func_ovl0F_10_80045C18(rx, 32768.0f, 32768.0f);
        func_ovl0F_10_80045F50();
    }
    func_ovl0F_10_80045F28(120);
    for (i = 0; i < 180; i++) {
        x += 2.7854452f;
        y += -5.2938886f;
        z += -9.473778f;
        ry += 0.14222221f;
        rz += 14.183333f;
        func_ovl0F_10_80045BF8(x, y, z);
        func_ovl0F_10_80045C18(32768.0f, ry, rz);
        func_ovl0F_10_80045F50();
    }
    func_ovl0F_10_80045F28(360);
    for (i = 0; i < 600; i++) {
        rx += -0.33816665f;
        ry += -0.0068333307f;
        rz += 2.8933334f;
        func_ovl0F_10_80045C18(rx, ry, rz);
        func_ovl0F_10_80045F50();
    }
    while (1) {
        func_ovl0F_10_80045F28(100);
    }
}

void func_ovl0F_10_80044780(UnkStructA *arg0) {
    f32 dx;
    f32 dy;
    f32 dz;
    s32 i;

    if (arg0->unk8 & 1) {
        func_ovl0F_10_80045C60(0x10);
    }
    dx = arg0->unk40->unk28 / 30.0f;
    dy = arg0->unk40->unk2C / 30.0f;
    dz = arg0->unk40->unk30 / 30.0f;
    func_ovl0F_10_80045DC0(arg0, 0x02000000);
    for (i = 0; i < 30; i++) {
        arg0->unk40->unk28 -= dx;
        arg0->unk40->unk2C -= dy;
        arg0->unk40->unk30 -= dz;
        func_ovl0F_10_80045E38(arg0);
        func_ovl0F_10_80045F50();
    }
    func_ovl0F_10_80045DC0(arg0, 0x01000000);
}

void func_ovl0F_10_80044074(UnkStructA *arg0) {
    s32 bits;
    s32 flags;
    f32 speed;
    f32 dist;
    f32 angle;
    f32 pos[3];
    s32 pad[12]; /* fake: unused, frame space the original has (no source for it found) */

    bits = arg0->unk64 & 0x0FFFFFFF;
    flags = arg0->unk64 & 0xF0000000;
    speed = bits * 0.01;
    dist = 0.0f;
    angle = 0.0f;
    if (arg0->unk8 & 0x20000) {
        return;
    }
    func_ovl0F_10_8004406C(arg0);
    func_ovl0F_10_80044064(arg0, pos);
    func_ovl0F_10_80045440(arg0->unk40, &dist, &angle);
    switch ((s32)angle) {
        case 270:
        if (flags == 0) {
            flags = 0x30000000;
            if (D_ovl0F_10_800466D4 == 0) {
                D_ovl0F_10_800466D4 = 10; func_ovl0F_10_80045F08(func_ovl0F_10_80044AD4, arg0, 0, 0, 0x110);
            }
        }
        if (flags == 0x30000000) {
            if (arg0->unk40->unk10 <= arg0->unk58) {
                func_ovl0F_10_8004405C(&speed, (dist < 50.0f) ? 0.1f : 0.16f);
                func_ovl0F_10_8004405C(&speed, (arg0->unk58 - arg0->unk40->unk10) * 0.01f);
            } else {
                arg0->unk40->unk14 -= (arg0->unk40->unk14 - arg0->unk5C) * 0.8f;
                arg0->unk40->unk10 -= (arg0->unk40->unk10 - arg0->unk58) * 0.8f;
                func_ovl0F_10_8004405C(&speed, (dist < 50.0f) ? 0.1f : 0.16f);
            }
            if (D_ovl0F_10_800466C8 == 1 && D_ovl0F_10_800466CC == 0) {
                func_ovl0F_10_80046490(0x32);
                D_ovl0F_10_800466CC = 20;
            }
        } else if (flags == 0x40000000) {
            func_ovl0F_10_80043FAC(dist + 100.0f, &speed, &flags);
        }
            break;
        case 90:
        if (flags == 0) {
            flags = 0x40000000;
            if (D_ovl0F_10_800466D4 == 0) {
                D_ovl0F_10_800466D4 = 10; func_ovl0F_10_80045F08(func_ovl0F_10_80044AD4, arg0, 0, 0, 0x110);
            }
        }
        if (flags == 0x40000000) {
            if (arg0->unk58 <= arg0->unk40->unk10) {
                func_ovl0F_10_8004405C(&speed, (dist < 50.0f) ? 0.1f : 0.16f);
                func_ovl0F_10_8004405C(&speed, (arg0->unk40->unk10 - arg0->unk58) * 0.01f);
            } else {
                arg0->unk40->unk14 -= (arg0->unk40->unk14 - arg0->unk5C) * 0.8f;
                arg0->unk40->unk10 -= (arg0->unk40->unk10 - arg0->unk58) * 0.8f;
                func_ovl0F_10_8004405C(&speed, (dist < 50.0f) ? 0.1f : 0.16f);
            }
            if (D_ovl0F_10_800466C8 == 1 && D_ovl0F_10_800466CC == 0) {
                func_ovl0F_10_80046490(0x32);
                D_ovl0F_10_800466CC = 20;
            }
        } else if (flags == 0x30000000) {
            func_ovl0F_10_80043FAC(dist + 100.0f, &speed, &flags);
        }
            break;
    }
    func_ovl0F_10_80044054(arg0, flags, &speed);
    func_ovl0F_10_80043FAC(dist, &speed, &flags);
    if (!func_ovl0F_10_80043FA4(arg0, pos)) {
        func_ovl0F_10_80043F94(&arg0->unk64, flags, speed);
    }
}

/* the side flags */
static void func_ovl0F_10_8004406C(UnkStructA *arg0) {
    if (arg0->unk8 & 1) {
        D_ovl0F_10_800466C8 = 1;
    } else {
        D_ovl0F_10_800466C8 = 0;
    }
}

static void func_ovl0F_10_80044064(UnkStructA *arg0, f32 *pos) {
    pos[0] = arg0->unk40->unk10;
    pos[1] = arg0->unk40->unk14;
    pos[2] = arg0->unk40->unk18;
}

static void func_ovl0F_10_8004405C(f32 *speed, f32 d) {
    *speed += d;
    if (25.0f < *speed) {
        *speed = 25.0f;
    }
}

static void func_ovl0F_10_80044054(UnkStructA *arg0, s32 flags, f32 *speed) {
    switch (flags) {
        case 0x30000000:
            func_ovl0F_10_80046408(arg0, 0xC0, *speed);
            break;
        case 0x40000000:
            func_ovl0F_10_80046408(arg0, 0x40, *speed);
            break;
    }
}

static void func_ovl0F_10_80043FAC(f32 dist, f32 *speed, s32 *flags) {
    if (dist < 50.0f) {
        *speed -= 0.05f;
    } else if (dist < 100.0f) {
        *speed -= 0.1f;
    } else {
        *speed -= 0.2f;
    }
    if (*speed < 0.0f) {
        *speed = 0.0f;
        *flags = 0;
    }
}

static s32 func_ovl0F_10_80043FA4(UnkStructA *arg0, f32 *pos) {
    s32 hit;

    hit = func_ovl0F_10_80043F9C(arg0);
    if (hit) {
        arg0->unk40->unk10 = pos[0];
        arg0->unk40->unk14 = pos[1];
        arg0->unk40->unk18 = pos[2];
    }
    return hit;
}

static s32 func_ovl0F_10_80043F9C(UnkStructA *arg0) {
    s32 hit;
    s32 saved;
    UnkStructA *other;
    s32 id;

    hit = 0;
    other = NULL;
    saved = arg0->unkC;
    arg0->unkC = 0;
    if (arg0->unk8 & 1) {
        other = func_ovl0F_10_80045638(func_ovl0F_10_80045598());
    } else if (arg0->unk8 & 2) {
        id = func_ovl0F_10_80045598();
        if (func_ovl0F_10_80045638(id) == arg0) {
            other = func_ovl0F_10_800455F8(id);
        } else {
            other = NULL;
        }
    }
    if (other != NULL) {
        if (func_ovl0F_10_80045ED8(arg0, 0x10, other) != 0) {
            hit = 1;
        }
    } else if (func_ovl0F_10_80045D68(arg0, 0x10) != 0) {
        hit = 1;
    }
    arg0->unkC = saved;
    return hit;
}

static void func_ovl0F_10_80043F94(s32 *dst, s32 flags, f32 speed) {
    *dst = (flags & 0xF0000000) | ((s32)(speed * 100.0f) & 0x0FFFFFFF);
}


void func_ovl0F_10_80043E08(void) {
    s32 task;
    s32 id;

    id = func_ovl0F_10_80045598();
    func_ovl0F_10_800455F8(id);
    D_ovl0F_10_800468A4 = 0;
    D_ovl0F_10_80046B34->unk3C = D_ovl0F_10_80046B34->unk4C * 0.05f;
    func_ovl0F_10_80045C60(0x7F);
    func_ovl0F_10_80045BF8(2898.7f, 3582.5f, 1940.3f);
    func_ovl0F_10_80045C18(247.7f, 1.7f, 344.0f);
    while (D_ovl0F_10_800468A4 == 0) {
        func_ovl0F_10_80045F50();
    }
    task = func_ovl0F_10_80045F08(func_ovl0F_10_800448A0, NULL, 0, 0, 0x130);
    func_ovl0F_10_80045658(1);
    func_ovl0F_10_800455C0(D_ovl0F_10_800468DC);
    func_ovl0F_10_800455C0(D_ovl0F_10_80046908);
    func_ovl0F_10_800455C0(D_ovl0F_10_80046910);
    func_ovl0F_10_800455C0(D_ovl0F_10_8004694C);
    func_ovl0F_10_80045648(id, 0xB);
    func_ovl0F_10_80045F28(60);
    func_ovl0F_10_800455C0(D_ovl0F_10_8004696C);
    func_ovl0F_10_800455C0(D_ovl0F_10_80046974);
    func_ovl0F_10_800455C0(D_ovl0F_10_80046998);
    func_ovl0F_10_80045658(0);
    func_ovl0F_10_800455E0(0x4000000);
    func_ovl0F_10_80045F28(40);
    func_ovl0F_10_80045658(1);
    func_ovl0F_10_80045F28(0x262);
    func_ovl0F_10_80046608();
    func_ovl0F_10_80045F10(task);
    func_ovl0F_10_80045F18();
}

void func_ovl0F_10_80043CCC(void) {
    s32 i;

    D_ovl0F_10_800468AC = 0;
    D_ovl0F_10_800468B0 = 0.0f;
    for (;;) {
        while (D_ovl0F_10_800468AC != 0) {
            for (i = 0; i < 90; i++) {
                if (D_ovl0F_10_800468B4 != 0) {
                    D_ovl0F_10_800468B0 += 0.22222222f;
                } else {
                    D_ovl0F_10_800468B0 += -0.22222222f;
                }
                func_ovl0F_10_80045CB0(D_ovl0F_10_800468B0, D_ovl0F_10_800468B0 - 0.1f);
                func_ovl0F_10_80045F50();
            }
            D_ovl0F_10_800468AC = 0;
        }
        func_ovl0F_10_80045F28(5);
    }
}

/* case 2's: the models of D_ovl0F_10_80046AE0[1] */
static void func_ovl0F_10_80043408(void) {
    func_ovl0F_10_80045AB0(D_ovl0F_10_80046AE0[1]->unk40, 0x61);
    D_ovl0F_10_80046AE0[1]->unk40->unk14 += 3200.0f;
}
