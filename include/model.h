#ifndef MODEL_H
#define MODEL_H

/*
 * A model file and the skeletons, instances and animations built from it
 * (main_code's 426E0, 45170, 46300 and 479D0).
 */

#include "common.h"

/* A model file: a count of chunks and the chunks (type, count, offset). */
typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
} UnkChunk; /* size 0xC */

typedef struct UnkModel {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ UnkChunk unkC[1];
} UnkModel;

typedef struct {
    /* 0x00 */ UnkModel *unk0;
    /* 0x04 */ s32 unk4;
} UnkTexSlot; /* size 8 */

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
} UnkTexFrame; /* size 8 */

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ UnkTexFrame *unkC;
} UnkTexAnim; /* size 0x10 */

/* A model object's textures (UnkStructA_40's unk4C, 426E0's func_8022984C):
 * models of their own (UnkTexSlot's unk0) and their animations. */
typedef struct UnkTexSlots {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkTexSlot *unk4;
    /* 0x08 */ UnkTexAnim *unk8;
} UnkTexSlots;

/* A joint of chunk 1 (0x30 bytes). */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ Vec3f unk4;
    /* 0x10 */ Vec3f unk10;
    /* 0x1C */ Vec3f unk1C;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
} UnkJoint; /* size 0x30 */

/* Chunk 2: per animation, its keys. */
typedef struct {
    /* 0x00 */ s32 *unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ Vec3f unk8;
    /* 0x14 */ Vec3f unk14;
    /* 0x20 */ Vec3f unk20;
} UnkKey; /* size 0x2C */

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkKey *unk4;
    /* 0x08 */ s32 unk8;
} UnkKeySet; /* size 0xC */

/* Chunk 3: channels (0x2F: key list; 0x30: a curve, tag 0x31). */
typedef struct {
    /* 0x00 */ PORT_IP(s32) unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 *unk8;
    /* 0x0C */ f32 *unkC;
} UnkCurve;

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 *unkC;
    /* 0x10 */ s32 *unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 *unk18;
    /* 0x1C */ f32 *unk1C;
} UnkChan; /* size 0x20 */

/* A key list channel's part from unk4 on. */
#if defined(TARGET_PC) && PORT_LP64
/* (where pointers are 8 bytes UnkChan's unkC is 8 bytes after its unk8,
   past the padding before a pointer) */
typedef struct __attribute__((packed)) {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    u8 port_pad[4];
    /* 0x08 */ s32 *unk8;
    /* 0x0C */ s32 *unkC;
} UnkChanKeys;
#else
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 *unk8;
    /* 0x0C */ s32 *unkC;
} UnkChanKeys;
#endif

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkChan *unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
} UnkChanSet; /* size 0x10 */

/* What func_8022AF1C reads a model into. */
typedef struct UnkSkel {
    /* 0x00 */ s32 unk0[3];
    /* 0x0C */ s32 unkC[3];
    /* 0x18 */ s32 unk18;
    /* 0x1C */ u32 unk1C;
    /* 0x20 */ UnkJoint *unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ UnkKeySet *unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ UnkChanSet *unk30;
    /* 0x34 */ s32 unk34;
} UnkSkel; /* size 0x38 */

/* A node of an instance's tree. */
typedef struct UnkNode {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ struct UnkNode *unk8;
    /* 0x0C */ struct UnkNode **unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ Vec3f unk14;
    /* 0x20 */ Vec3f unk20;
    /* 0x2C */ Vec3f unk2C;
    /* 0x38 */ s32 unk38;
} UnkNode; /* size 0x3C */

/* An instance (func_8022D1D0). */
typedef struct UnkInst {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ Vec3f unk4;
    /* 0x10 */ Vec3f unk10;
    /* 0x1C */ Vec3f unk1C;
    /* 0x28 */ UnkNode *unk28;
    /* 0x2C */ UnkJoint *unk2C;
    /* 0x30 */ UnkKeySet *unk30;
    /* 0x34 */ UnkChanSet *unk34;
    /* 0x38 */ s32 *unk38;
} UnkInst; /* size 0x3C */

/* A copy of an instance's tree with two key states per node. */
typedef struct UnkTree {
    /* 0x00 */ UnkNode *unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ struct UnkTree **unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10[2];
    /* 0x18 */ Vec3f unk18[2][3];
} UnkTree; /* size 0x60 */

/* A playing animation's track. */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 *unk10;
    /* 0x14 */ s32 *unk14;
    /* 0x18 */ PORT_IP(s32) unk18;
    /* 0x1C */ UnkTree *unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
} UnkTrack; /* size 0x28 */

/* A playing animation (func_8022C8B8). */
typedef struct UnkAnim {
    /* 0x00 */ UnkInst *unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ UnkTrack *unk14;
} UnkAnim; /* size 0x18 */

/* The texture owners (D_80098C18, 5 of them): a model object and, per
 * mesh, the file and the mesh drawn instead (47A80's func_8022D58C). */
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ UnkStructA_40 *unk4;
    /* 0x08 */ void **unk8;
    /* 0x0C */ s32 *unkC;
} UnkTexOwner; /* size 0x10 */

#endif
