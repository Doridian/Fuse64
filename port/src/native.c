/*
 * Byte order: what arrives from the ROM as bytes, put in the host's order
 * by type where it arrives (docs/PORT.md, "Byte order"; Blast Corps'
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
}

static void sw16(uint8_t *p) {
    uint16_t v = port_be16(p);
    memcpy(p, &v, 2);
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

static int is_pic(uint32_t t) { return t == 0x12 || t == 0x15 || t == 0x16 || t == 0x19 || t == 0x1A; }

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
        if (r + 12 <= size && i + 1 < n)
            claim(r + 8, r + 12, U_WORD);
        else if (r + 12 <= size)
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
void port_asset_native(int32_t id, void *p, int32_t size) {
#ifdef PORT_SWAP
    uint8_t *d = p;
    (void)id;
    if (size >= 0x10 && rd32(d) == MAGIC)
        container(d, (uint32_t)size);
#else
    (void)id, (void)p, (void)size;
#endif
}
