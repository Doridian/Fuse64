/*
 * A function the 0x0E bank's overlays and 12_01..05 share, the same bytes
 * (but for relocations) in all twenty: it turns the view to 0, 90, 180 or
 * 270 degrees by a record's direction (0x00, 0xC0, 0x80, 0x40), or else by
 * the record's own angle through core.h's CORE_FUNC_C (inlined there).
 * The -O3 ones compile it from this source; the -O2 ones (0E_06/07/08/10/14)
 * have CORE_FUNC_C's body written out in it, and keep their copies.
 *
 * Names are ovl_0E_01's addresses (ANGLE_<addr>, ANGLE_D_<addr>), mapped
 * before the include.
 */

void ANGLE_80043A78(void) {
    f32 angle;

    ANGLE_800458C8();
    switch (ANGLE_D_80046D28.unk8) {
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
        ANGLE_80045F18(angle, 32768.0f, 32768.0f);
    } else {
        ANGLE_80043A30(ANGLE_D_80046D28.unkC);
    }
    ANGLE_800458B0(ANGLE_D_80046D28.unk4);
}
