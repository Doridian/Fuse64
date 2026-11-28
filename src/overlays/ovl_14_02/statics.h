/*
 * Code 27E920.c includes where it is, from this same file (docs/DECOMPILING.md,
 * "The overlays' shared source"): the `.file` switches around a part end as1's
 * scheduling blocks, as the original's do.
 */
#if STATICS_PART == 1
/* 8004D638 and 8004D640 (the old `#line 1 "v.c"` before 8004D638): with
 * the static alone here, the switch back to 27E920.c ends a block at
 * 8004D640's start, which the original doesn't; whether more of the
 * original file came from here is not known */
static void func_ovl14_02_8004D638(s16 x, s16 z, Vec3f *vel) {
    vel->x = x * 0.002f;
    vel->z = z * 0.002f;
}

void func_ovl14_02_8004D640(UnkStructA *obj) {
    UnkStructA *spark;
    s32 id;
    Vec3f pos2;
    Vec3f dir;
    f32 pad1[4]; /* fake: unused, frame space the original has (four words between dir and vel; not
                  * found: x/z locals for the chained stores, a second pointer for the 0x800 object,
                  * ints for (s32) dx/dz, a player flag, block-scoped vectors) */
    Vec3f vel;
    f32 dx;
    f32 dz;
    Vec3f pos;

    func_ovl14_02_8004D630();
    func_ovl14_02_8004D628();
    dx = obj->unk40->unk10 - obj->unk58;
    dz = obj->unk40->unk18 - obj->unk60;
    if (obj->unk8 & 1) {
        D_ovl14_02_8004FF0C = 1;
        D_ovl14_02_8004FF10 = obj;
    } else {
        D_ovl14_02_8004FF0C = 0;
        D_ovl14_02_8004FF10 = NULL;
    }
    if (obj->unk68 != 0.0f || obj->unk6C != 0.0f) {
        obj->unk64 = 0;
        D_ovl14_02_8004FF08 = 0;
    } else {
        pos.x = obj->unk40->unk10;
        pos.y = obj->unk40->unk14;
        pos.z = obj->unk40->unk18;
        func_ovl14_02_8004D638(obj->unk64 >> 16, obj->unk64 & 0xFFFF, &vel);
        if ((s32) dx != (s32) vel.x) {
            obj->unk40->unk10 = obj->unk58;
            if ((s32) dx < (s32) vel.x) {
                vel.x -= (dz != 0.0f ? 0.707f : 1.0f) * (dx != 0.0f ? D_ovl14_02_8004FF04 : D_ovl14_02_8004FF00);
                if (vel.x < dx) {
                    vel.x = dx;
                }
                func_ovl14_02_8004D580(dx);
            } else if ((s32) vel.x < (s32) dx) {
                vel.x += (dz != 0.0f ? 0.707f : 1.0f) * (dx != 0.0f ? D_ovl14_02_8004FF04 : D_ovl14_02_8004FF00);
                if (dx < vel.x) {
                    vel.x = dx;
                }
                func_ovl14_02_8004D580(dx);
            }
            obj->unk40->unk10 += vel.x;
        }
        if ((s32) dz != (s32) vel.z) {
            obj->unk40->unk18 = obj->unk60;
            if ((s32) dz < (s32) vel.z) {
                vel.z -= (dx != 0.0f ? 0.707f : 1.0f) * (dz != 0.0f ? D_ovl14_02_8004FF04 : D_ovl14_02_8004FF00);
                if (vel.z < dz) {
                    vel.z = dz;
                }
                func_ovl14_02_8004D580(dz);
            } else if ((s32) vel.z < (s32) dz) {
                vel.z += (dx != 0.0f ? 0.707f : 1.0f) * (dz != 0.0f ? D_ovl14_02_8004FF04 : D_ovl14_02_8004FF00);
                if (dz < vel.z) {
                    vel.z = dz;
                }
                func_ovl14_02_8004D580(dz);
            }
            obj->unk40->unk18 += vel.z;
        }
        if (func_ovl14_02_8004D578(obj)) {
            func_ovl14_02_8004D570(&obj->unk64, 0.0f, 0.0f);
            obj->unk40->unk10 = pos.x;
            obj->unk40->unk14 = pos.y;
            obj->unk40->unk18 = pos.z;
        } else {
            obj->unk64 = ((s16) (vel.x * 500.0f) << 16) | ((s16) (vel.z * 500.0f) & 0xFFFF);
        }
    }
    if (obj->unk8 & 1) {
        pos2.x = D_ovl14_02_80050270.x;
        pos2.y = D_ovl14_02_800502EC->y;
        pos2.z = D_ovl14_02_80050270.z;
        func_ovl14_02_8004F598(D_ovl14_02_800502EC, &pos2, &dir);
        dir.y = 0.0f;
        if (dir.z > 1450.0f) {
            dir.z = 1449.0f;
            func_ovl14_02_8004F5A0(&dir, &pos2);
            id = func_ovl14_02_8004EDE8();
            spark = func_ovl14_02_8004EE48(id);
            D_ovl14_02_80050270.x = spark->unk40->unk10 = D_ovl14_02_800502EC->x + pos2.x;
            D_ovl14_02_80050270.z = spark->unk40->unk18 = D_ovl14_02_800502EC->z + pos2.z;
        }
        if (func_ovl14_02_8004F9E0(0x417)) {
            spark = func_ovl14_02_8004F630(-1, 0x800, 0x60, NULL);
            dir.z = 10.0f;
            func_ovl14_02_8004F5A0(&dir, &pos2);
            func_ovl14_02_8004F6E0((UnkStructA *) &D_ovl14_02_80050270, spark);
            spark->unk40->unk10 += pos2.x;
            spark->unk40->unk18 += pos2.z;
            func_ovl14_02_8004F5B8(spark, 1);
            func_ovl14_02_8004F620(spark);
            func_ovl14_02_8004F9D8(0x417);
        }
    }
}
#endif
