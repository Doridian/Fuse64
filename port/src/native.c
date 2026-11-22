/*
 * Byte order: what arrives from the ROM as bytes, put in the host's order
 * by type where it arrives (docs/PORT.md, "Byte order"; DozerEncore's
 * port/host/native.c in its model).  On a big-endian host every function
 * here is the identity.
 *
 * The data is the N64's: each datum at its N64 offset, a 16-bit one a host
 * half, a 32-bit one a host word, so the game's C reads it as it was
 * written.  What only the RDP or the RSP's audio reads as bytes (texels,
 * palettes, samples) stays as the ROM has it.
 *
 * The asset cache's files (87190's func_8026CE28) go through
 * port_asset_native, by what the file is: the "64" container's pictures
 * and models (docs/ASSETS.md; tools/assetlib/bm64model.py has the model's
 * layout this follows).
 */
#include <string.h>
#include "port.h"

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PORT_SWAP 1
#endif

static uint32_t rd32(const uint8_t *p) { return port_be32(p); }

static void sw32(uint8_t *p) {
    uint32_t v = port_be32(p);
    memcpy(p, &v, 4);
    port_wc_mark(p, 4, 4);
}

static void sw16(uint8_t *p) {
    uint16_t v = port_be16(p);
    memcpy(p, &v, 2);
    port_wc_mark(p, 2, 2);
}

void port_native32(void *p, uint32_t n) {
#ifdef PORT_SWAP
    for (uint32_t i = 0; i + 4 <= n; i += 4)
        sw32((uint8_t *)p + i);
#else
    (void)p, (void)n;
#endif
}

void port_native16(void *p, uint32_t n) {
#ifdef PORT_SWAP
    for (uint32_t i = 0; i + 2 <= n; i += 2)
        sw16((uint8_t *)p + i);
#else
    (void)p, (void)n;
#endif
}

#ifdef PORT_SWAP

/* ---- the "64" container -------------------------------------------------------------- */

#define MAGIC 0x36340038u
#define MAX_FILE 0x200000

/* each 4-byte unit of the file: what it is (0 untouched bytes) */
enum { U_BYTES, U_WORD, U_HALVES, U_VTX };
static uint8_t unit[MAX_FILE / 4];
static uint32_t fsize;
static const uint8_t *fdata;

static void claim(uint32_t s, uint32_t e, int kind) {
    if (e > fsize)
        e = fsize;
    for (uint32_t i = s / 4; i < (e + 3) / 4 && i < MAX_FILE / 4; i++)
        if (unit[i] == U_BYTES)
            unit[i] = (uint8_t)kind;
}

/* a picture's record (its a: the width and height, halves); a palette's (0x1A)
   a is its colours, a word (ACA00.c's func_80292410) */
static int is_pic(uint32_t t) { return t == 0x12 || t == 0x15 || t == 0x16 || t == 0x19; }

static uint8_t dl_seen[MAX_FILE / 8];

static void walk_dl(uint32_t off, int depth) {
    if (depth > 16)
        return;
    for (uint32_t p = off; p + 8 <= fsize && p >= 0x10; p += 8) {
        if (dl_seen[p / 8])
            return;
        dl_seen[p / 8] = 1;
        claim(p, p + 8, U_WORD);
        uint32_t w0 = rd32(fdata + p), w1 = rd32(fdata + p + 4);
        uint32_t c = w0 >> 24, seg = w1 >> 24, a = w1 & 0xFFFFFF;
        if (c == 0xB8)
            return;
        if (c == 0x06) {
            if (seg == 2)
                walk_dl(a, depth + 1);
            if (((w0 >> 16) & 0xFF) == 1)
                return;
        } else if (c == 0x04 && seg == 2) {
            uint32_t n = (w0 >> 10) & 0x3F;
            for (uint32_t k = 0; k < n; k++) {
                claim(a + 16 * k, a + 16 * k + 12, U_VTX);
                /* (the colour or normal: bytes) */
            }
        }
    }
}

static void walk_nodes(uint32_t off, int32_t root) {
    int32_t todo[512];
    int n = 0;
    static uint8_t seen[4096];
    memset(seen, 0, sizeof seen);
    todo[n++] = root;
    while (n > 0) {
        int32_t i = todo[--n];
        if (i < 0 || i >= 4096 || seen[i] || off + 0x30u * (uint32_t)(i + 1) > fsize)
            continue;
        seen[i] = 1;
        claim(off + 0x30 * i, off + 0x30 * (i + 1), U_WORD);
        int32_t nxt = (int32_t)rd32(fdata + off + 0x30 * i + 0x28);
        int32_t child = (int32_t)rd32(fdata + off + 0x30 * i + 0x2C);
        if (nxt && n < 511)
            todo[n++] = i + nxt;
        if (child && n < 511)
            todo[n++] = i + child;
    }
}

/* base poses: a groups of {id, m}, m times {c, c words, 9 floats} */
static void walk_poses(uint32_t p, uint32_t count) {
    for (uint32_t g = 0; g < count && p + 8 <= fsize; g++) {
        uint32_t m = rd32(fdata + p + 4);
        claim(p, p + 8, U_WORD);
        p += 8;
        for (uint32_t k = 0; k < m && p + 4 <= fsize; k++) {
            uint32_t c = rd32(fdata + p);
            uint32_t len = 4 + 4 * c + 36;
            if (c > 0x10000)
                return;
            claim(p, p + len, U_WORD);
            p += len;
        }
    }
}

/* animations: a clips of {id, m, length}, m tracks: {0x2F, c, x, c pairs}
   or {0x30, x, y, c, z, c pairs} */
static void walk_anims(uint32_t p, uint32_t count) {
    for (uint32_t g = 0; g < count && p + 12 <= fsize; g++) {
        uint32_t m = rd32(fdata + p + 4);
        claim(p, p + 12, U_WORD);
        p += 12;
        for (uint32_t k = 0; k < m && p + 12 <= fsize; k++) {
            uint32_t kind = rd32(fdata + p), c, len;
            if (kind == 0x2F) {
                c = rd32(fdata + p + 4);
                len = 12 + 8 * c;
            } else if (kind == 0x30) {
                c = rd32(fdata + p + 12);
                len = 20 + 8 * c;
            } else {
                return;
            }
            if (c > 0x10000)
                return;
            claim(p, p + len, U_WORD);
            p += len;
        }
    }
}

static void container(uint8_t *d, uint32_t size) {
    uint32_t n = rd32(d + 4), kind = rd32(d + 0xC);
    if (size > MAX_FILE || n > 4096 || 0x10 + 12 * n > size + 4)
        return;
    fsize = size;
    fdata = d;
    memset(unit, 0, (size + 3) / 4);
    memset(dl_seen, 0, (size + 7) / 8);
    /* the header: the magic, the count, four bytes, the first record's type */
    claim(0, 8, U_WORD);
    claim(0xC, 0x10, U_WORD);
    for (uint32_t i = 0; i < n; i++) {
        uint32_t r = 0x10 + 12 * i;
        uint32_t type = i == 0 ? kind : rd32(d + r - 4);
        uint32_t a = rd32(d + r), off = rd32(d + r + 4);
        if (r + 8 > size)
            break;
        /* a picture's a is its width and height, halves (TexEntry) */
        claim(r, r + 4, is_pic(type) ? U_HALVES : U_WORD);
        claim(r + 4, r + 8, U_WORD);
        /* the next record's type (after the last record the file's data
           begins: a type 4 record's texels, which stay bytes) */
        if (r + 12 <= size && i + 1 < n)
            claim(r + 8, r + 12, U_WORD);
        if (off >= size)
            continue;
        switch (type) {
        case 0: case 5: case 6: case 8:
            walk_dl(off, 0);
            break;
        case 1:
            walk_nodes(off, (int32_t)a);
            break;
        case 2:
            walk_poses(off, a);
            break;
        case 3:
            walk_anims(off, a);
            break;
        case 7:
            if (a == 4)
                claim(off, off + 24, U_WORD);
            break;
        }
    }
    for (uint32_t j = 0; j < (size + 3) / 4 && 4 * j + 4 <= size; j++) {
        uint8_t *p = d + 4 * j;
        switch (unit[j]) {
        case U_WORD:
            sw32(p);
            break;
        case U_HALVES:
        case U_VTX:
            sw16(p);
            sw16(p + 2);
            break;
        }
    }
}

#endif

/* a file the asset cache inflated (87190.c's func_8026CE28) */
/* ---- the ROM's display lists ------------------------------------------------------------
 *
 * Where pointers are 8 bytes the port's display list commands are 16 bytes
 * and the ROM's are 8 (port/host/gfx.c reads both).  A file of the asset
 * cache is where the renderer finds the ROM's: the files loaded are kept
 * here, the latest over any earlier one that the same memory held. */

#define NARROW_MAX 512
static struct {
    uintptr_t lo, hi;
} narrow[NARROW_MAX];
static int nnarrow;

static void narrow_add(const void *p, uint32_t size) {
    uintptr_t lo = (uintptr_t)p, hi = lo + size;
    int n = 0;
    for (int i = 0; i < nnarrow; i++)
        if (narrow[i].hi <= lo || narrow[i].lo >= hi)
            narrow[n++] = narrow[i];
    if (n == NARROW_MAX) {
        for (int i = 1; i < NARROW_MAX; i++)
            narrow[i - 1] = narrow[i];
        n--;
    }
    narrow[n].lo = lo, narrow[n].hi = hi;
    nnarrow = n + 1;
}

int port_narrow_dl(const void *p) {
    uintptr_t a = (uintptr_t)p;
    for (int i = nnarrow - 1; i >= 0; i--)
        if (a >= narrow[i].lo && a < narrow[i].hi)
            return 1;
    return 0;
}

/* What the N64 has past a file of the asset cache.  A picture ends with
 * its palette (docs/ASSETS.md), and the TLUT load (78F60.c, 7B2D0.c,
 * 7C4D0.c, 7D840.c, 54E40.c) reads 16 entries for a CI4 picture and 256
 * for a CI8 whatever the palette's count: 94 of us's pictures have fewer
 * colours than that, so the load reads up to 0x1FE bytes past the file,
 * into the rest of its heap block and on into the next.  The port's block
 * has PORT_ASSET_PAD bytes more (87190.c), given here the N64's bytes:
 * - 0x368 (us's and eu's title screen "TM", 19x11 CI4, 5 colours) uses
 *   entries 7 and 8.  Its 162 bytes take a 256-byte fragment, which on the
 *   N64 is where the LZSS window of the file loaded just before it (0x367)
 *   was until the decoder freed it (in the emulator both times the title
 *   loads them: 0x80308000, then 0x80318000), so the bytes past it are
 *   that window's: 0F0F 0F0F 0F00 0000, entries 5 to 8, the last two with
 *   alpha 0.
 * - Every other: zeros.  The legal screen's picture (0x366, 5 colours of
 *   16) is the first file the heap gets, and the emulator has zeros after
 *   it; no other picture has a texel past its count. */
static uint8_t lzss_window[0x400];

void port_lzss_window(const void *w) {
    memcpy(lzss_window, w, sizeof lzss_window);
}

void port_asset_tail(int32_t id, void *p, int32_t size) {
    uint8_t *t = (uint8_t *)p + size;
    memset(t, 0, PORT_ASSET_PAD);
    if (id == 0x368)
        for (int32_t i = 0; i < PORT_ASSET_PAD; i++)
            t[i] = lzss_window[(size + i) & 0x3FF];
}

void port_asset_native(int32_t id, void *p, int32_t size) {
    (void)id;
    if (size >= 0x10 && port_be32(p) == 0x36340038u)
        narrow_add(p, (uint32_t)size);
#ifdef PORT_SWAP
    uint8_t *d = p;
    if (size >= 0x10 && rd32(d) == MAGIC)
        container(d, (uint32_t)size);
#endif
}

/* ---- sound (80580.c's loads) ---------------------------------------------------------------
 *
 * The bank file (.ctl, ALBankFile and what it points at) and 'T2''s sounds
 * (the same ALSound records, with their offsets from the block's start):
 * every record and leaf in the host's order, by its fields (the port's
 * libaudio, port/libaudio/bankfile.h, reads them so).  A unit converted
 * once (a leaf several sounds share). */

#ifdef PORT_SWAP
static uint8_t *bk;
static uint32_t bklen;
#define BK_MAX 0x200000
static uint8_t done16[BK_MAX / 16];      /* converted, per 2-byte unit (a bit each) */

static int fresh(uint32_t off, uint32_t n) {
    /* (whether the units at off..off+n are all unconverted; marks them) */
    if (off + n > bklen)
        return 0;
    for (uint32_t i = off / 2; i < (off + n) / 2; i++)
        if (done16[i / 8] & (1 << (i % 8)))
            return 0;
    for (uint32_t i = off / 2; i < (off + n) / 2; i++)
        done16[i / 8] |= (uint8_t)(1 << (i % 8));
    return 1;
}

static void bk_words(uint32_t off, uint32_t n) {
    for (uint32_t i = 0; i < n; i += 4)
        sw32(bk + off + i);
}

static void bk_halves(uint32_t off, uint32_t n) {
    for (uint32_t i = 0; i < n; i += 2)
        sw16(bk + off + i);
}

/* a wave table (base, len, type, flags, loop, book) and its loop and book */
static void bk_wave(uint32_t off) {
    if (!off || !fresh(off, 0x14))
        return;
    uint8_t type = bk[off + 8];
    uint32_t loop = rd32(bk + off + 0xC), book = rd32(bk + off + 0x10);
    bk_words(off, 8);
    bk_words(off + 0xC, 8);
    if (type == 0) {                    /* ADPCM: the loop has its decoder state */
        if (loop && fresh(loop, 0x2C)) {
            bk_words(loop, 12);
            bk_halves(loop + 12, 32);
        }
        if (book && off + 0x14 <= bklen && fresh(book, 8)) {
            uint32_t order = rd32(bk + book), npred = rd32(bk + book + 4);
            bk_words(book, 8);
            if (order * npred <= 64 && fresh(book + 8, order * npred * 16))
                bk_halves(book + 8, order * npred * 16);
        }
    } else if (loop && fresh(loop, 0xC)) {
        bk_words(loop, 12);
    }
}

/* a sound (envelope, key map, wave table, three bytes) */
static void bk_sound_at(uint32_t off) {
    if (!fresh(off, 0xC))
        return;
    uint32_t env = rd32(bk + off), wave = rd32(bk + off + 8);
    bk_words(off, 12);
    if (env && fresh(env, 12))
        bk_words(env, 12);          /* (attack, decay, release; the two volumes are bytes) */
    bk_wave(wave);
}

static void bk_sound(uint32_t off) {
    if (off)
        bk_sound_at(off);
}

static void bk_inst(uint32_t off) {
    if (!off || !fresh(off, 0x10))
        return;
    uint16_t n = port_be16(bk + off + 0xE);
    bk_halves(off + 0xC, 4);
    for (uint32_t i = 0; i < n && fresh(off + 0x10 + 4 * i, 4); i++) {
        uint32_t s = rd32(bk + off + 0x10 + 4 * i);
        sw32(bk + off + 0x10 + 4 * i);
        bk_sound(s);
    }
}

static void bk_bank(uint32_t off) {
    if (!off || !fresh(off, 0xC))
        return;
    uint16_t n = port_be16(bk + off);
    uint32_t perc = rd32(bk + off + 8);
    bk_halves(off, 2);
    bk_words(off + 4, 8);
    bk_inst(perc);
    for (uint32_t i = 0; i < n && fresh(off + 0xC + 4 * i, 4); i++) {
        uint32_t in = rd32(bk + off + 0xC + 4 * i);
        sw32(bk + off + 0xC + 4 * i);
        bk_inst(in);
    }
}

static void bk_begin(void *p, uint32_t len) {
    bk = p;
    bklen = len < BK_MAX ? len : BK_MAX;
    memset(done16, 0, sizeof done16);
}
#endif

/* a bank file (ALBankFile: revision 'B1', the banks...) as it arrived */
void port_bank_native(void *p, int32_t len) {
#ifdef PORT_SWAP
    if (len < 4 || port_be16(p) != 0x4231)
        return;
    bk_begin(p, (uint32_t)len);
    uint16_t n = port_be16(bk + 2);
    fresh(0, 4);
    bk_halves(0, 4);
    for (uint32_t i = 0; i < n && fresh(4 + 4 * i, 4); i++) {
        uint32_t b = rd32(bk + 4 + 4 * i);
        sw32(bk + 4 + 4 * i);
        bk_bank(b);
    }
#else
    (void)p, (void)len;
#endif
}

/* 'T2''s sounds: count records of 16 bytes (80580.c's SndSound) */
void port_sounds_native(void *p, int32_t len, int32_t count) {
#ifdef PORT_SWAP
    bk_begin(p, (uint32_t)len);
    for (int32_t k = 0; k < count && (uint32_t)(16 * k + 16) <= bklen; k++)
        bk_sound_at((uint32_t)(16 * k));     /* (the first at offset 0) */
#else
    (void)p, (void)len, (void)count;
#endif
}

/* 'T1'/'T2', the sound table: s16 magic, s16 count, then 8-byte entries
   (three s8, a u8, an s16 and a u16) */
void port_sndtable_native(void *p, int32_t size) {
#ifdef PORT_SWAP
    uint8_t *d = p;
    if (size < 4)
        return;
    sw16(d);
    sw16(d + 2);
    for (int32_t o = 4; o + 8 <= size; o += 8) {
        sw16(d + o + 4);
        sw16(d + o + 6);
    }
#else
    (void)p, (void)size;
#endif
}

/* 'T2''s header (80580.c's SndHeader): six words, two halves */
void port_sndheader_native(void *p) {
#ifdef PORT_SWAP
    port_native32(p, 0x18);
    port_native16((uint8_t *)p + 0x18, 4);
#else
    (void)p;
#endif
}

/* the sound scripts: a word (the offset of the scripts' table of u16
   offsets), the table, then the scripts' bytes */
void port_scripts_native(void *p, int32_t size) {
#ifdef PORT_SWAP
    uint8_t *d = p;
    if (size < 4)
        return;
    uint32_t off = rd32(d), end = (uint32_t)size;
    sw32(d);
    for (uint32_t o = off; o + 2 <= end && o + 2 <= (uint32_t)size; o += 2) {
        uint16_t v = port_be16(d + o);
        if (v > off && v < end)
            end = v;
        sw16(d + o);
    }
#else
    (void)p, (void)size;
#endif
}

/* the sequence file ('S1'/'S2'): s16 revision, s16 count, (offset, len)
   words, and for 'S2' a 16-byte record a sequence (two bytes, then three
   words) */
void port_seqfile_native(void *p, int32_t size) {
#ifdef PORT_SWAP
    uint8_t *d = p;
    if (size < 4)
        return;
    sw16(d);
    sw16(d + 2);
    uint16_t rev, count;
    memcpy(&rev, d, 2);
    memcpy(&count, d + 2, 2);
    if (size < 4 + 8 * count)
        return;
    port_native32(d + 4, 8 * count);
    if (rev == 0x5332 && size >= 4 + 24 * count)
        for (uint32_t i = 0; i < count; i++)
            port_native32(d + 4 + 8 * count + 16 * i + 4, 12);
#else
    (void)p, (void)size;
#endif
}
