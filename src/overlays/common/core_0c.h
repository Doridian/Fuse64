/*
 * Four functions the 0x0C bank's overlays 0C_01..09 have right after
 * core.h's (so first in the source after them, as the overlays are written
 * last to first), the same bytes (but for relocations) in all nine: an
 * object (0x183) grown over 20 frames at another's position, its per-frame
 * scale, the task that starts it, and an object handler.  They include this
 * after core.h, and call the overlay's ZJ_<id> stubs too; the other names are
 * ovl_0C_01's addresses (CORE0C_<addr>), mapped before the include.
 */

void CORE0C_80048870(void) {
    f32 scale28;
    f32 scale2C;
    UnkStructA *src;
    UnkStructA *obj;
    s32 i;
    s32 count;

    src = CORE0C_80049930();
    ZJ_2B04(0xB3, src->unk40->unk10, src->unk40->unk14, src->unk40->unk18);
    obj = CORE0C_800497E0(0x183, 0x11080000, 0xD0, 0);
    obj->unk40->unk10 = src->unk40->unk10;
    obj->unk40->unk14 = src->unk40->unk14;
    obj->unk40->unk18 = src->unk40->unk18;
    scale28 = 2.0f;
    scale2C = 4.0f;
    count = 20;
    for (i = 0; i < count; i++) {
        CORE0C_80048798(obj, i, count, scale28, scale2C);
        CORE0C_80049950();
    }
    CORE0C_800497D0(obj);
    CORE0C_80049918();
}

void CORE0C_80048798(UnkStructA *arg0, s32 arg1, s32 arg2, f32 arg3, f32 arg4) {
    arg0->unk40->unk28 = CORE0C_80049AB0((f32) arg1 / (f32) arg2 * 90.0f) * arg3;
    arg0->unk40->unk2C = CORE0C_80049AB0((f32) arg1 / (f32) arg2 * 180.0f) * arg4;
    arg0->unk40->unk36 = (s32) ((1.0f - (f32) arg1 / (f32) arg2) * 63.0f) + 0x80;
}

void CORE0C_80048754(UnkStructA *arg0) {
    CORE0C_800497C0(arg0, 0x01000000);
    CORE0C_80049908(CORE0C_80048870, arg0, 0, 0, 0x150);
}

void CORE0C_80048694(UnkStructA *arg0) {
    ZJ_1904(10);
    CORE0C_80048E40(arg0->unk40, &arg0->unk40->unk1C, &arg0->unk40->unk20);
    arg0->unk40->unk28 *= 2.0f;
    arg0->unk40->unk30 *= 2.0f;
    arg0->unk40->unk2C *= 0.1f;
    arg0->unk40->unk14 += 20.0f;
    arg0->unk6C = 0.0f;
    arg0->unk70 = 0.0f;
    ZJ_1904(60);
}
