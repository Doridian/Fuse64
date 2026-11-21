/*
 * PORT_DIGEST=FILE: the gameplay digest (docs/PORT.md, "The gameplay
 * digest", after DozerEncore's port/host/digest.c): at every controller
 * read, the scene, the game's frame and a few dozen gameplay fields, read
 * from the game's variables.  port/tools/emu_digest.py writes the same from
 * mupen64plus running the original ROM (the same fields, read from RDRAM at
 * the same moment of the frame), and port/tools/digest_cmp.py compares two
 * digests by game frame, not by retrace.
 *
 * It only reads game memory, so it changes nothing else (test.py's hashes
 * are the same with it).  The read is osContStartReadData's (port/src/
 * ultra.c), as in mupen64plus, where the PIF reads the pads during it:
 * 40040.c's main loop starts it (func_8023A208) after the frame's tasks and
 * before func_80226E84 counts the frame, and takes the pads at the start of
 * the next (func_8023A104).  So FRAME is the frame the tasks just ran, and
 * PAD what the next one plays with.  (Before the loop, init reads them
 * twice at frame 0, and the emulator's core more often: frame 0 never has
 * a pad.)
 *
 * One line per read:
 *
 *   POLL VI SCENE SUB FRAME PAD [*] key=value ...
 *
 * POLL counts the reads, VI the retraces (the clock: never compared),
 * SCENE D_802AC5D4 and SUB D_802AC5D8 (54E40.c's scene and sub-mode, as
 * func_8023AE6C sets them), FRAME D_8029F600 (412F0.c's func_80226E84
 * counts the frames it ends, from boot), PAD the buttons this read gives
 * (mupen64plus's BUTTONS word, as the input scripts have it).  The fields
 * are written when they change: a line marked * has them all (the first
 * line and the first of each scene), "key=-" is a pointer that is NULL.
 * Every value is the words' bits in hex (floats too), so the two sides
 * agree to the bit or differ.
 *
 *   fl     D_802AC5D0, the scene's flags (bit 0: a change is pending)
 *   rng    78CA0.c's generator: D_802ACFE0 (the index) and an FNV-1a hash
 *          of D_802ACFE8[56] (its table)
 *   st     D_802AC610's 20 words (54E40.c: the stage clock in unk30/unk34,
 *          the play time in unk38, the scene's state)
 *   bt     D_802AC660's 62 words (54E40.c: the battle's lives, the match
 *          clock in unk2C/unk30/unk34, the score)
 *   eN     D_800AEDF0[N], N 0-4 (5E3F0.c's entries: the players and the
 *          other characters the scripts drive): every word but the 7
 *          pointers (0x28, 0x2C, 0x30, 0x40, 0x4C, 0xB0, 0xB4), at the N64's
 *          offsets in order
 *   pN     entry N's object's model (unk28->unk40, 5E3F0.c's UnkModel):
 *          its position unk10, unk14, unk18, unk20 and unk28-unk30
 *
 * The scenario's seed: PORT_SEED=N gives the random generator the seed N
 * where 40040.c seeds it from the clock (osGetTime) at boot, as
 * tools/shift_check.py's patch does in the ROM (0x1997).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

typedef uint8_t u8;
typedef int32_t s32;
typedef uint32_t u32;
typedef float f32;

typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ f32 unk10;
    /* 0x14 */ f32 unk14;
    /* 0x18 */ f32 unk18;
    /* 0x1C */ u8 unk1C[4];
    /* 0x20 */ f32 unk20;
    /* 0x24 */ u8 unk24[4];
    /* 0x28 */ f32 unk28;
    /* 0x2C */ f32 unk2C;
    /* 0x30 */ f32 unk30;
} DigestModel; /* 5E3F0.c's UnkModel */

typedef struct {
    /* 0x00 */ u8 unk0[0x20];
    /* 0x20 */ intptr_t unk20;
    /* 0x24 */ intptr_t unk24;
    /* 0x28 */ u8 unk28[0x3C - 0x28];
    /* 0x3C */ void *unk3C;
    /* 0x40 */ DigestModel *unk40;
} DigestObj; /* 5E3F0.c's UnkObj, as far as unk40 */

typedef struct {
    /* 0x00 */ u32 unk0, unk4, unk8, unkC, unk10, unk14, unk18, unk1C, unk20, unk24;
    /* 0x28 */ DigestObj *unk28;
    /* 0x2C */ intptr_t unk2C;
    /* 0x30 */ intptr_t unk30;
    /* 0x34 */ u32 unk34, unk38, unk3C;
    /* 0x40 */ intptr_t unk40;
    /* 0x44 */ u32 unk44, unk48;
    /* 0x4C */ intptr_t unk4C;
    /* 0x50 */ u32 unk50, unk54, unk58, unk5C, unk60, unk64, unk68, unk6C, unk70, unk74, unk78, unk7C, unk80,
        unk84, unk88, unk8C, unk90, unk94, unk98, unk9C, unkA0, unkA4, unkA8, unkAC;
    /* 0xB0 */ intptr_t unkB0;
    /* 0xB4 */ intptr_t unkB4;
    /* 0xB8 */ u32 unkB8, unkBC, unkC0, unkC4;
} DigestEntry; /* 5E3F0.c's UnkStruct_800AEDF0 */

extern DigestEntry D_800AEDF0[5];
extern s32 D_8029F600;
extern s32 D_802AC5D0;
extern s32 D_802AC5D4;
extern s32 D_802AC5D8;
extern u32 D_802AC610[20];
extern u32 D_802AC660[62];
extern s32 D_802ACFE0;
extern u32 D_802ACFE8[56];

/* the pad word this read gives (input.c), chosen by the frame with
   --replay */
uint32_t host_input_poll(unsigned frame);

unsigned int port_seed(unsigned int clock) {
    const char *e = getenv("PORT_SEED");
    return e && *e ? (unsigned int)strtoul(e, NULL, 0) : clock;
}

#define MAXF 16
#define VALLEN 1024
static char last[MAXF][VALLEN];
static FILE *out;
static unsigned polls;
static s32 last_scene = -1;

static unsigned f2u(const void *p) {
    u32 w;
    memcpy(&w, p, 4);
    return w;
}

static void words(char *buf, const u32 *w, int n) {
    int i;
    char *p = buf;
    for (i = 0; i < n; i++)
        p += sprintf(p, i ? ",%x" : "%x", (unsigned)w[i]);
}

static void put(char *line, int *len, int idx, const char *key, const char *val, int full) {
    if (!full && !strcmp(last[idx], val))
        return;
    strcpy(last[idx], val);
    *len += sprintf(line + *len, " %s=%s", key, val);
}

void port_digest_poll(void) {
    static int init;
    static char line[16384];
    char val[VALLEN];
    int len, full, i, f = 0;
    unsigned frame = (unsigned)D_8029F600;
    uint32_t pad = host_input_poll(frame);

    if (!init) {
        const char *e = getenv("PORT_DIGEST");
        init = 1;
        if (e && *e && !(out = fopen(e, "w")))
            host_fatal("PORT_DIGEST=%s: can't write it", e);
    }
    if (!out)
        return;
    polls++;
    full = polls == 1 || D_802AC5D4 != last_scene;
    last_scene = D_802AC5D4;
    len = sprintf(line, "%u %u %d %d %u %08x%s", polls, host_vi_count, (int)D_802AC5D4, (int)D_802AC5D8, frame,
                  (unsigned)pad, full ? " *" : "");

    sprintf(val, "%x", (unsigned)D_802AC5D0);
    put(line, &len, f++, "fl", val, full);
    {
        u32 h = 0x811C9DC5u;
        int b;
        for (i = 0; i < 56; i++)
            for (b = 0; b < 32; b += 8) {
                h ^= (D_802ACFE8[i] >> b) & 0xFF;
                h *= 16777619u;
            }
        sprintf(val, "%d,%08x", (int)D_802ACFE0, (unsigned)h);
        put(line, &len, f++, "rng", val, full);
    }
    words(val, D_802AC610, 20);
    put(line, &len, f++, "st", val, full);
    words(val, D_802AC660, 62);
    put(line, &len, f++, "bt", val, full);
    for (i = 0; i < 5; i++) {
        DigestEntry *e = &D_800AEDF0[i];
        u32 w[] = { e->unk0,  e->unk4,  e->unk8,  e->unkC,  e->unk10, e->unk14, e->unk18, e->unk1C, e->unk20,
                    e->unk24, e->unk34, e->unk38, e->unk3C, e->unk44, e->unk48, e->unk50, e->unk54, e->unk58,
                    e->unk5C, e->unk60, e->unk64, e->unk68, e->unk6C, e->unk70, e->unk74, e->unk78, e->unk7C,
                    e->unk80, e->unk84, e->unk88, e->unk8C, e->unk90, e->unk94, e->unk98, e->unk9C, e->unkA0,
                    e->unkA4, e->unkA8, e->unkAC, e->unkB8, e->unkBC, e->unkC0, e->unkC4 };
        char key[8];
        words(val, w, (int)(sizeof w / sizeof w[0]));
        sprintf(key, "e%d", i);
        put(line, &len, f++, key, val, full);
        if (e->unk28 && e->unk28->unk40) {
            DigestModel *m = e->unk28->unk40;
            u32 p[] = { f2u(&m->unk10), f2u(&m->unk14), f2u(&m->unk18), f2u(&m->unk20),
                        f2u(&m->unk28), f2u(&m->unk2C), f2u(&m->unk30) };
            words(val, p, 7);
        } else
            strcpy(val, "-");
        sprintf(key, "p%d", i);
        put(line, &len, f++, key, val, full);
    }
    fprintf(out, "%s\n", line);
}
