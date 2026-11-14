/* tasks: from ovl_10_02 */

s32 TASK_8004432C(void (*arg0)(void), const char *arg1) {
    s32 i;

    i = TASK_800432D0(arg0);
    TASK_800445A8("task_entry>%d:%s\n", i, arg1);
    return i;
}

void TASK_80044300(void (*arg0)(void)) {
    TASK_D_80045E80[TASK_800442BC(arg0)] = NULL;
}

s32 TASK_800442BC(void (*arg0)(void)) {
    s32 i;
    s32 found;

    found = -1;
    i = 0;
    do {
        if (arg0 == TASK_D_80045E80[i]) {
            found = i;
        }
        i++;
    } while (found == -1 && i < 64);
    return found;
}

void TASK_800442A0(void) {
    TASK_D_80045E80[TASK_D_80045E78] = NULL;
}

void TASK_8004428C(s32 arg0) {
    TASK_D_80045E80[arg0] = NULL;
}

void TASK_80044270(s32 arg0) {
    TASK_D_80045D78[TASK_D_80045E78] = arg0;
}

void TASK_80044254(void (*arg0)(void)) {
    TASK_D_80045E80[TASK_D_80045E78] = arg0;
}

void TASK_800441C4(void) {
    TASK_D_80045F80 = TASK_800454C0();
    TASK_800454D0(TASK_D_80045F80, 0x20000001);
    TASK_D_80045F80->unk40 = 0.5f;
    TASK_D_80045F80->unk44 = TASK_D_80045F80->unk40 * 0.75f;
    TASK_D_80045F80->unk18 = 0.1f;
    TASK_D_80045F80->unk1C = -0.1f;
}

void TASK_80044188(void) {
    s32 *p;

    p = TASK_800456D0();
    if (*p <= 0 || *p == 0) {
        *p = 1;
    }
}

void TASK_8004411C(UnkStructA *arg0) {
    s32 i;

    if (arg0->unk8 & 1) {
        TASK_80045190(16);
    }
    for (i = 0; i < 30; i++) {
        TASK_80045368(arg0);
        TASK_80045480();
    }
}

void TASK_8004409C(UnkStructA *arg0) {
    Vec3f pos;

    if (arg0->unk8 & 1) {
        TASK_80045738(0xFF, 0xFF, 0xFF, 100.0f);
        TASK_800452F0(arg0, 0x01000000);
        TASK_80045458(5);
        TASK_80045738(0, 0, 0, 0.0f);
        TASK_800453B8(arg0, &pos);
        TASK_80045890(12, &pos, 0);
    }
}
