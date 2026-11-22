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
 * Then the game's object pools (POOLS below), one key per entry (the
 * pool's key and the index: obj0 ... obj127), each the entry's fields in
 * the order its spec lists them.  A spec is "OFF:K" or "OFF-END:K" (hex
 * N64 offsets, END included), K the field's kind: w a word, h a halfword,
 * b a byte (each its unsigned value in hex, read with the field's own
 * width, so that the host's byte order doesn't matter) and p a pointer,
 * given as which pool entry it points to ("obj12", "+N" past its start),
 * "-" for NULL, the value for a small number (below 0x10000, or above
 * 0xFFFF0000: an index some code keeps there) and "*" for any other
 * pointer.  emu_digest.py reads this table out of this file, so the two
 * sides read the same fields by construction.  The pools are read where
 * pointers are 4 bytes (the host's layout is the N64's there); a 64-bit
 * host writes only the fields above.
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

/* The game's object pools: { key, the array, entries, the N64's stride,
   fields }.  The structs are the C's (the file named), the pointers and
   the unions (bytes in one use, a float in another) left out of the words. */
#define POOL(key, sym, n, stride, spec) { key, sym, n, stride, spec }
extern char D_800A3F00[], D_800A0D00[], D_8008E658[], D_8008F438[], D_802AFCE8[], D_800BB4A0[], D_800BA900[],
    D_800BB340[], D_800BC528[], D_800BC770[], D_802B0260[];
static const struct {
    const char *key;
    char *base;
    unsigned n, stride;
    const char *spec;
} pools[] = {
    /* 48600.c's UnkStructA80: the objects (the three lists), D_800AEDF0's unk28 */
    POOL("obj", D_800A3F00, 128, 0x80, "0-1c:w 28-38:w 48-74:w 20:p 24:p 3c:p 40:p 44:p 78:p 7c:p"),
    /* 426E0.c's UnkObj: the models drawn (an object's unk40 is one) */
    POOL("mdl", D_800A0D00, 160, 0x50, "0-30:w 34:h 36:h 38:w 3c:p 40:p 44:p 48:p 4c:p"),
    /* 926A0.c's Rec926 */
    POOL("rec", D_8008E658, 48, 0x48, "0:h c:h e:h 10:h 1c-30:w 36:h 38:h 3a:h 3c-40:w 44:h 46:h 8:p 14:p 18:p"),
    /* 8A7A0.c's Ent88 (ENTS: [0] the list's head) */
    POOL("ent", D_8008F438, 22, 0x88,
         "0:h 4-8:w c:h e:h 10:h 3c:h 40-54:w 5c-68:h 74:h 78-84:w 14-38:p"),
    /* 8A7A0.c's Slot30 (SLOTS: one a player) */
    POOL("pslot", D_802AFCE8, 5, 0x30, "0:h 8:w 10:h 1c-2c:w c:p"),
    /* 93850.c's UnkStruct_800BB4A8: the bonus items ([0] the head, [1] the sentinel) */
    POOL("item", D_800BB4A0, 48, 0x58, "0:h 2:h 4:w 1c-22:h 24-2c:w 30:h 32:h 34-44:w 48:h 4a-4f:b 50:w 54:h 8-18:p"),
    /* 9CAB0.c's UnkStruct_800BA900 (its unk14 an UnkStruct_80288040, whose
       unions at 0x50 and 0x58, bytes or floats by the type, are left
       out; its 0x8C, at 0xA0, a pointer into D_800BB340) */
    POOL("tobj", D_800BA900, 16, 0xA4,
         "0-10:w 18-2c:w 30-33:b 34-4c:w 50:b 51:b 52:h 54-60:w 68:w 70:b 71:b 72:h 74:w 78:w 80:b 81:b 82:h 84:w 88:w "
         "8c-8e:b 90:h 92:h 94:w 98:w 9c:h 14:p 7c:p a0:p"),
    /* 9CAB0.c's UnkStruct_800BB340: their tasks */
    POOL("ttask", D_800BB340, 16, 0x14, "0-c:w 10:p"),
    /* A4CE0.c's effects */
    POOL("fx", D_800BC528, 16, 0x24, "0-18:w 1c:p 20:p"),
    /* A8DE0.c's moving objects */
    POOL("mv", D_800BC770, 16, 0x28, "0:w c-24:w 4:p 8:p"),
    /* A1EA0.c's UnkStruct_802B0260 */
    POOL("a1", D_802B0260, 2, 0x50, "0-4c:w"),
};
#define NPOOLS (int)(sizeof pools / sizeof pools[0])

/* a spec's fields, (offset << 8) | kind */
#define MAXPF 64
static unsigned pool_fields[NPOOLS][MAXPF];
static int pool_nf[NPOOLS];
static char *pool_raw[NPOOLS];

static void pool_parse(int k) {
    const char *s = pools[k].spec;
    while (*s) {
        char *e;
        unsigned a = (unsigned)strtoul(s, &e, 16), b = a, step;
        if (*e == '-')
            b = (unsigned)strtoul(e + 1, &e, 16);
        if (*e != ':')
            host_fatal("digest: bad spec %s", pools[k].spec);
        step = e[1] == 'w' || e[1] == 'p' ? 4 : e[1] == 'h' ? 2 : 1;
        for (; a <= b; a += step) {
            if (pool_nf[k] == MAXPF)
                host_fatal("digest: spec %s too long", pools[k].spec);
            pool_fields[k][pool_nf[k]++] = a << 8 | (unsigned char)e[1];
        }
        s = e + 2;
        while (*s == ' ')
            s++;
    }
}

/* a pointer as the pool entry it points to */
static int pool_ptr(char *p, uintptr_t v) {
    int k;
    if (!v)
        return sprintf(p, "-");
    for (k = 0; k < NPOOLS; k++) {
        uintptr_t b = (uintptr_t)pools[k].base;
        if (v >= b && v < b + pools[k].n * pools[k].stride) {
            unsigned o = (unsigned)(v - b);
            return o % pools[k].stride ? sprintf(p, "%s%u+%x", pools[k].key, o / pools[k].stride, o % pools[k].stride)
                                       : sprintf(p, "%s%u", pools[k].key, o / pools[k].stride);
        }
    }
    if (v == (uintptr_t)D_800AEDF0 || (v > (uintptr_t)D_800AEDF0 && v < (uintptr_t)(D_800AEDF0 + 5))) {
        unsigned o = (unsigned)(v - (uintptr_t)D_800AEDF0);
        return o % 0xC8 ? sprintf(p, "e%u+%x", o / 0xC8, o % 0xC8) : sprintf(p, "e%u", o / 0xC8);
    }
    if ((u32)v < 0x10000 || (u32)v >= 0xFFFF0000u)
        return sprintf(p, "%x", (unsigned)(u32)v);
    return sprintf(p, "*");
}

static void pool_entry(char *val, int k, int i) {
    const unsigned char *e = (const unsigned char *)pools[k].base + i * pools[k].stride;
    char *p = val;
    int j;
    for (j = 0; j < pool_nf[k]; j++) {
        unsigned off = pool_fields[k][j] >> 8, kind = pool_fields[k][j] & 0xFF;
        if (j)
            *p++ = ',';
        switch (kind) {
        case 'w': p += sprintf(p, "%x", (unsigned)*(const u32 *)(e + off)); break;
        case 'h': p += sprintf(p, "%x", (unsigned)*(const uint16_t *)(e + off)); break;
        case 'b': p += sprintf(p, "%x", (unsigned)e[off]); break;
        default: p += pool_ptr(p, (uintptr_t)*(const u32 *)(e + off)); break;
        }
    }
    *p = 0;
}

/* the pad word this read gives (input.c), chosen by the frame with
   --replay */
uint32_t host_input_poll(unsigned frame);
void host_replay_wait(unsigned frame);

unsigned int port_seed(unsigned int clock) {
    const char *e = getenv("PORT_SEED");
    return e && *e ? (unsigned int)strtoul(e, NULL, 0) : clock;
}

#define MAXF 640
#define VALLEN 640
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

/* the end of the run: the file closed (under node, emscripten's runtime
   exits without flushing what libc still buffers: the last lines) */
void host_digest_close(void) {
    if (out)
        fclose(out);
    out = NULL;
}

static int force_full;

static void open_out(void) {
    static int init;
    if (!init) {
        const char *e = getenv("PORT_DIGEST");
        init = 1;
        if (e && *e && !(out = fopen(e, "w")))
            host_fatal("PORT_DIGEST=%s: can't write it", e);
    }
}

/* the reset button (`# prenmi VI`) and the NMI (`# reset VI`, before the
   new boot's first read, whose line then has every field), as
   emu_digest.py writes them */
void port_digest_mark(const char *what) {
    open_out();
    if (!out)
        return;
    fprintf(out, "# %s %u\n", what, host_vi_count);
    if (!strcmp(what, "reset"))
        force_full = 1;
}

void port_digest_poll(void) {
    static char line[MAXF * VALLEN];
    char val[VALLEN];
    int len, full, i, f = 0;
    unsigned frame = (unsigned)D_8029F600;
    uint32_t pad;

    host_replay_wait(frame);
    pad = host_input_poll(frame);

    open_out();
    if (!out)
        return;
    polls++;
    full = polls == 1 || D_802AC5D4 != last_scene || force_full;
    force_full = 0;
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
    if (sizeof(void *) == 4) {
        int k;
        for (k = 0; k < NPOOLS; k++) {
            unsigned size = pools[k].n * pools[k].stride;
            if (!pool_nf[k]) {
                pool_parse(k);
                if (!(pool_raw[k] = malloc(size)))
                    host_fatal("digest: out of memory");
            }
            for (i = 0; i < (int)pools[k].n; i++) {
                char key[16];
                const char *e = pools[k].base + i * pools[k].stride;
                char *r = pool_raw[k] + i * pools[k].stride;
                /* (an entry whose bytes are the last read's gives its value) */
                if (!full && !memcmp(r, e, pools[k].stride)) {
                    f++;
                    continue;
                }
                memcpy(r, e, pools[k].stride);
                pool_entry(val, k, i);
                sprintf(key, "%s%d", pools[k].key, i);
                put(line, &len, f++, key, val, full);
            }
        }
    }
    fprintf(out, "%s\n", line);
    {
        /* PORT_DIGEST_DUMP=FRAME:FILE: the RDRAM buffer at each read of game
           frame FRAME (emu_digest.py's --dump: the asm data and the regions
           are at their N64 offsets there, in the host's byte order) */
        extern uint8_t port_rdram[];
        static int at = -2;
        static const char *path;
        if (at == -2) {
            const char *e = getenv("PORT_DIGEST_DUMP");
            at = e && strchr(e, ':') ? atoi(e) : -1;
            path = at >= 0 ? strchr(e, ':') + 1 : NULL;
        }
        if (at >= 0 && frame == (unsigned)at) {
            FILE *d = fopen(path, "wb");
            if (d) {
                fwrite(port_rdram, 1, 8 << 20, d);
                fclose(d);
            }
        }
    }
}
