#include "common.h"
#include "gfx.h"
#include "41FD0.h"
#include "40300.h"

/* gbi.h's Light: col, colc and dir */
typedef struct {
    /* 0x00 */ u8 col[3];
    /* 0x03 */ s8 pad1;
    /* 0x04 */ u8 colc[3];
    /* 0x07 */ s8 pad2;
    /* 0x08 */ s8 dir[3];
    /* 0x0B */ s8 pad3;
    /* 0x0C */ u8 pad4[4];
} UnkStruct_8029F800; /* size 0x10 */

/* gbi.h's Lights2: an ambient light and two lights */
typedef struct {
    /* 0x00 */ u8 col[3];
    /* 0x03 */ s8 pad1;
    /* 0x04 */ u8 colc[3];
    /* 0x07 */ s8 pad2;
    /* 0x08 */ UnkStruct_8029F800 l[2];
} UnkStruct_8029F7F8; /* size 0x28 */

void *func_80227678(s32);
void guPerspective(UnkStruct_802A53D8 *, u16 *, f32, f32, f32, f32, f32);
void guLookAt(UnkStruct_802A53D8 *, f32, f32, f32, f32, f32, f32, f32, f32, f32);
void guMtxL2F(void *, UnkStruct_802A53D8 *);

extern UnkStruct_800BD5B0 D_800BD5B0[4];
extern s32 D_802A5368;
extern UnkStruct_802A538C *D_802A538C;

void func_80227BAC(s8 x, s8 y, s8 z);
void func_80227BE8(u8 r, u8 g, u8 b);
void func_80227834(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4);

s32 D_8029F7E0 = 0;
s32 D_8029F7E4 = 900;
s32 D_8029F7E8 = 1000;
s32 D_8029F7EC = 0xFF;
s32 D_8029F7F0 = 0xFF;
s32 D_8029F7F4 = 0xFF;
/* the lights: ambient 0x50, light 0 0x32 and light 1 0xC8 from (0, 0x72,
   0x37), as func_80227C18 sets them again */
UnkStruct_8029F7F8 D_8029F7F8 = {
    { 0x50, 0x50, 0x50 }, 0, { 0x50, 0x50, 0x50 }, 0,
    { { { 0x32, 0x32, 0x32 }, 0, { 0x32, 0x32, 0x32 }, 0, { 0, 0x72, 0x37 }, 0, { 0 } },
      { { 0xC8, 0xC8, 0xC8 }, 0, { 0xC8, 0xC8, 0xC8 }, 0, { 0, 0x72, 0x37 }, 0, { 0 } } },
};
/* ZeroJump module 0x21's table (func_802277D0); its .data's last three
   words are zeros, taken here for the section's padding */
void *D_8029F820[] = { func_80227C50, func_80227BAC, func_80227BE8, func_80227870, func_80227834 };

u16 D_802A53D0;
UnkStruct_802A53D8 D_802A53D8;
f32 D_802A5418[4][4];

UnkStruct_800BD5B0 *func_80227E38(void) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (D_800BD5B0[i].unk10 == 0) {
            D_800BD5B0[i].unk10 = 1;
            return &D_800BD5B0[i];
        }
    }
    return (UnkStruct_800BD5B0 *) -1;
}

void func_80227E2C(UnkStruct_800BD5B0 *arg0) {
    arg0->unk10 = 0;
    arg0->unk58 = NULL;
}

void func_80227D50(UnkStruct_800BD5B0 *arg0, u32 x, u32 y, u32 w, u32 h) {
    arg0->vscale[0] = w * 2;
    arg0->vscale[1] = h * 2;
    arg0->vscale[2] = 0x1FF;
    arg0->vscale[3] = 0;
    arg0->vtrans[0] = (x * 2 + w) * 2;
    arg0->vtrans[1] = (y * 2 + h) * 2;
    arg0->vtrans[2] = 0x1FF;
    arg0->vtrans[3] = 0;
    arg0->unk5C = x;
    arg0->unk60 = y;
    arg0->unk64 = x + w - 1;
    arg0->unk68 = y + h - 1;
}

void func_80227CD0(UnkStruct_800BD5B0 *arg0, f32 xEye, f32 yEye, f32 zEye, f32 xAt, f32 yAt, f32 zAt, f32 xUp,
                   f32 yUp, f32 zUp) {
    guLookAt(&arg0->unk18, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    guMtxL2F(D_802A5418, &arg0->unk18);
}

void func_80227C9C(UnkStruct_800BD5B0 *arg0, s32 arg1) {
    arg0->unk14 = arg1;
    arg0->unk58 = func_80227678(arg1);
}

void func_80227C50(f32 fovy, f32 aspect, f32 near, f32 far, f32 scale) {
    guPerspective(&D_802A53D8, &D_802A53D0, fovy, aspect, near, far, scale);
}

void func_80227BE8(u8 r, u8 g, u8 b);
void func_80227BAC(s8 x, s8 y, s8 z);

void func_80227C18(void) {
    func_80227BE8(0x32, 0x32, 0x32);
    func_80227BAC(0, 0x72, 0x37);
}

void func_80227BE8(u8 r, u8 g, u8 b) {
    D_8029F7F8.l[0].col[0] = D_8029F7F8.l[0].colc[0] = r;
    D_8029F7F8.l[0].col[1] = D_8029F7F8.l[0].colc[1] = g;
    D_8029F7F8.l[0].col[2] = D_8029F7F8.l[0].colc[2] = b;
}

void func_80227BAC(s8 x, s8 y, s8 z) {
    D_8029F7F8.l[1].dir[0] = x;
    D_8029F7F8.l[1].dir[1] = y;
    D_8029F7F8.l[1].dir[2] = z;
    D_8029F7F8.l[0].dir[0] = D_8029F7F8.l[1].dir[0];
    D_8029F7F8.l[0].dir[1] = D_8029F7F8.l[1].dir[1];
    D_8029F7F8.l[0].dir[2] = D_8029F7F8.l[1].dir[2];
}

void func_8022787C(Gfx_80225840 **arg0) {
    Gfx_80225840 *gfx = *arg0;
    UnkStruct_800BD5B0 *p;
    s32 i;

    { Gfx_80225840 *g = gfx++; g->w0 = 0xBC000002; g->w1 = 0x80000060; } { Gfx_80225840 *g = gfx++; g->w0 = 0x03860010; g->w1 = (PORT_UP(u32)) &D_8029F7F8.l[0]; } { Gfx_80225840 *g = gfx++; g->w0 = 0x03880010; g->w1 = (PORT_UP(u32)) &D_8029F7F8.l[1]; } { Gfx_80225840 *g = gfx++; g->w0 = 0x038A0010; g->w1 = (PORT_UP(u32)) &D_8029F7F8; }
    { Gfx_80225840 *g = gfx++; g->w0 = 0xBC00000E; g->w1 = D_802A53D0; }
    for (i = 0; i < 4; i++) {
        p = &D_800BD5B0[i];
        if (p->unk10 != 0 && p->unk58 != NULL) {
            func_80225C44(&D_802A538C->mtx[D_802A5368], &D_802A53D8, 0x40);
            { Gfx_80225840 *g = gfx++; g->w0 = 0x01030040; g->w1 = (PORT_UP(u32)) &D_802A538C->mtx[D_802A5368]; }
            D_802A5368++;
            { Gfx_80225840 *g = gfx++; g->w0 = 0x03800010; g->w1 = (PORT_UP(u32)) p; }
            func_80225C44(&D_802A538C->mtx[D_802A5368], &p->unk18, 0x40);
            { Gfx_80225840 *g = gfx++; g->w0 = 0x01010040; g->w1 = (PORT_UP(u32)) &D_802A538C->mtx[D_802A5368]; }
            D_802A5368++;
            { Gfx_80225840 *g = gfx++; g->w0 = 0xED000000 | (((s32) (p->unk5C * 4.0f) & 0xFFF) << 12) | ((s32) (p->unk60 * 4.0f) & 0xFFF); g->w1 = (((s32) (p->unk64 * 4.0f) & 0xFFF) << 12) | ((s32) (p->unk68 * 4.0f) & 0xFFF); }
            { Gfx_80225840 *g = gfx++; g->w0 = 0x06000000; g->w1 = (PORT_UP(u32)) func_80227678(p->unk14); }
        }
    }
    *arg0 = gfx;
    func_80297D60(p, 0x1C0);
}

void func_80227870(void) {
    D_8029F7E0 = 0;
}

void func_80227834(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    D_8029F7E0 = 1;
    D_8029F7E4 = arg0;
    D_8029F7E8 = arg1;
    D_8029F7EC = arg2;
    D_8029F7F0 = arg3;
    D_8029F7F4 = arg4;
}

void func_802277D0(void) {
    s32 i;

    func_80297D30(0x21, D_8029F820);
    for (i = 0; i < 4; i++) {
        D_800BD5B0[i].unk10 = 0;
        D_800BD5B0[i].unk58 = NULL;
    }
}
