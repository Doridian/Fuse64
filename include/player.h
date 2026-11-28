#ifndef PLAYER_H
#define PLAYER_H

/* The players: D_800AEDF0[5]. */

#include "common.h"

typedef struct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ f32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ UnkStructA *unk28;
    /* 0x2C */ PORT_IP(s32) unk2C;
    /* 0x30 */ PORT_IP(s32) unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ PORT_IP(s32) unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s32 unk48;
    /* 0x4C */ PORT_IP(s32) unk4C;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ f32 unk58;
    /* 0x5C */ f32 unk5C;
    /* 0x60 */ f32 unk60;
    /* 0x64 */ f32 unk64;
    /* 0x68 */ s32 unk68;
    /* 0x6C */ f32 unk6C;
    /* 0x70 */ f32 unk70;
    /* 0x74 */ s32 unk74;
    /* 0x78 */ s32 unk78;
    /* 0x7C */ s32 unk7C;
    /* 0x80 */ s32 unk80;
    /* 0x84 */ s32 unk84;
    /* 0x88 */ f32 unk88;
    /* 0x8C */ f32 unk8C;
    /* 0x90 */ s32 unk90;
    /* 0x94 */ s32 unk94;
    /* 0x98 */ s32 unk98;
    /* 0x9C */ s32 unk9C;
    /* 0xA0 */ s32 unkA0;
    /* 0xA4 */ s32 unkA4;
    /* 0xA8 */ s32 unkA8;
    /* 0xAC */ s32 unkAC;
    /* 0xB0 */ PORT_IP(s32) unkB0;
    /* 0xB4 */ PORT_IP(s32) unkB4;
    /* 0xB8 */ s32 unkB8;
    /* 0xBC */ s32 unkBC;
    /* 0xC0 */ f32 unkC0;
    /* 0xC4 */ f32 unkC4;
} UnkStruct_800AEDF0; /* size 0xC8 */

#endif
