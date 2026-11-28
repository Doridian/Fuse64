/*
 * The ROM's data, taken from the player's ROM at startup (docs/PORT.md,
 * "The ROM's data").
 *
 * The game's initialized data is the ROM's, and none of it is in the
 * executable: the asm data's values are zeros in gen/asm_data.c (asm2c.py),
 * and the game's C is compiled from copies whose initializers have zeros
 * where the ROM's values were (port/tools/cdata.py, the game's C's
 * compiler launcher).  Each comes with a table of where its values go and
 * where they come from in the code image: port_asmfill (gen/asm_data_fill.c)
 * and each C file's port_cfill_<file> (gen/cdata_list.c lists them).
 * Before anything reads them (main, right after the ROM is loaded),
 * host_rom_fill builds the code image from the ROM as tools/layout.py does
 * (the ROM below its first archive, then every overlay inflated, archive by
 * archive) and puts every value in place, in the host's byte order.  The
 * asm data's bytes are checked against the checksum asm2c.py took of them,
 * which tells this version's ROM from any other.
 *
 * A variable whose initializer is all zeros here is in .bss: the soft reset
 * and an overlay's loading clear those (port/src/reset.c, overlay.c), and
 * host_rom_refill puts their values back.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "port.h"

#if defined(VERSION_JP)
#define ROM_NAME "Baku Bomberman (Japan)"
#elif defined(VERSION_EU)
#define ROM_NAME "Bomberman 64 (Europe)"
#else
#define ROM_NAME "Bomberman 64 (USA)"
#endif

#define FIRST_BANK 0x120000u
#define BANK_STEP 0x20000u
#define ASSET_BANK 0x300000u

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

/* Hudson's LZSS (docs/COMPRESSION.md, tools/hudson_lzss.py's decode): the
   stream at src, its size word first, appended to out */
static uint8_t *inflate(const uint8_t *src, const uint8_t *end, uint8_t *out, uint8_t *out_end) {
    enum { N = 1024, F = 66, THRESHOLD = 2 };
    uint8_t window[N];
    uint32_t size, r = N - F, flags = 0, n = 0;
    if (end - src < 4)
        return NULL;
    size = be32(src);
    src += 4;
    if (size > (uint32_t)(out_end - out))
        return NULL;
    memset(window, 0, sizeof window);
    while (n < size) {
        flags >>= 1;
        if (!(flags & 0x100)) {
            if (src >= end)
                return NULL;
            flags = 0xFF00 | *src++;
        }
        if (flags & 1) {
            if (src >= end)
                return NULL;
            out[n++] = window[r] = *src++;
            r = (r + 1) & (N - 1);
        } else {
            uint32_t lo, hi, off, k, len;
            if (end - src < 2)
                return NULL;
            lo = src[0];
            hi = src[1];
            src += 2;
            off = lo | (hi & 0xC0) << 2;
            len = (hi & 0x3F) + THRESHOLD + 1;
            for (k = 0; k < len; k++) {
                uint8_t c = window[(off + k) & (N - 1)];
                if (n < size)
                    out[n] = c;
                n++;
                window[r] = c;
                r = (r + 1) & (N - 1);
            }
        }
    }
    return out + size;
}

/* the code image (tools/layout.py's image): the ROM below the first archive,
   then every overlay archive's members but its index, inflated, in order */
static uint8_t *code_image(const uint8_t *rom, uint32_t rom_size, uint32_t want) {
    static const uint8_t magic[8] = {0, 0, 0x20, 0x08, 0, 0, 0x04, 0};
    uint8_t *img, *p, *end;
    uint32_t base;
    if (rom_size < FIRST_BANK || want < FIRST_BANK)
        return NULL;
    img = malloc(want);
    if (!img)
        host_fatal("no memory for the ROM's code image");
    memcpy(img, rom, FIRST_BANK);
    p = img + FIRST_BANK;
    end = img + want;
    for (base = FIRST_BANK; base + 8 <= rom_size; base += BANK_STEP) {
        uint32_t data, o, k;
        if (base == ASSET_BANK || memcmp(rom + base, magic, 8))
            continue;
        data = base + be32(rom + base);
        for (o = base + 8, k = 0;; o += 8, k++) {
            uint32_t off, size;
            if (o + 8 > rom_size)
                goto bad;
            off = be32(rom + o);
            size = be32(rom + o + 4);
            if (off == 0xFFFFFFFFu)
                break;
            if (k == 0)
                continue;       /* the archive's index */
            if (data + off > rom_size || size > rom_size - (data + off))
                goto bad;
            p = inflate(rom + data + off, rom + data + off + size, p, end);
            if (!p)
                goto bad;
        }
    }
    if (p == end)
        return img;
bad:
    free(img);
    return NULL;
}

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define NO_ASAN __attribute__((no_sanitize("address")))
#endif
#endif
#ifndef NO_ASAN
#define NO_ASAN
#endif

/* The asm data's bytes (asm2c.py's gen/asm_data_fill.c): every datum of
   main_code's data that is still asm and of boot_code's VI modes is 0 in
   the C, and port_asmfill says where each run of them goes, where in the
   code image it comes from and its kind: bytes, big-endian halves,
   big-endian words, or big-endian words into pointer-wide slots. */
struct PortAsmFill {
    uint8_t *dst;
    uint32_t src, count, kind;
};
extern const struct PortAsmFill port_asmfill[];
extern const uint32_t port_asmfill_count, port_asmfill_image_size;
extern const uint64_t port_asmfill_sum;

/* The game's C's values (cdata.py's tables, gen/cdata_list.c): n2 runs of
   n1 values of `width` bytes (1, or big-endian 2, 4 or 8) from the image at
   src, consecutive in each run, the runs hs2 bytes apart here and ns2 in
   the image (a field of an array of structures).  width's second byte,
   where it isn't 0, is the value's width here (a long or a uintptr_t: 4
   bytes on the N64, 8 on most 64-bit hosts), widened as signed where its
   third byte is 1. */
struct PortCFill {
    void *dst;
    unsigned int src, width, n1, n2, hs2, ns2;
};
struct PortCFillList {
    const struct PortCFill *e;
    const unsigned int *n;
};
extern const struct PortCFillList port_cfill_lists[];
extern const unsigned int port_cfill_nlists;

static uint8_t *image;          /* kept for host_rom_refill */

static const uint32_t asm_width[4] = {1, 2, 4, 4};

/* an asm entry's bytes here */
static size_t asm_extent(const struct PortAsmFill *e) {
    return (size_t)e->count * (e->kind == 3 ? sizeof(uintptr_t) : asm_width[e->kind]);
}

/* 1 if the image's bytes the asm data takes are the ones the build was made
   from (their FNV-1a) */
static int asm_check(const uint8_t *img) {
    uint64_t sum = 0xCBF29CE484222325ull;
    uint32_t i, c, n;
    for (i = 0; i < port_asmfill_count; i++) {
        const struct PortAsmFill *e = &port_asmfill[i];
        const uint8_t *s = img + e->src;
        if (e->kind > 3 || e->src > port_asmfill_image_size ||
            e->count > (port_asmfill_image_size - e->src) / asm_width[e->kind])
            return 0;
        n = e->count * asm_width[e->kind];
        for (c = 0; c < n; c++)
            sum = (sum ^ s[c]) * 0x100000001B3ull;
    }
    return sum == port_asmfill_sum;
}

static NO_ASAN void asm_put(const uint8_t *img, const struct PortAsmFill *e) {
    const uint8_t *s = img + e->src;
    uint8_t *d = e->dst;
    uint32_t c;
    for (c = 0; c < e->count; c++) {
        if (e->kind == 0) {
            d[c] = s[c];
        } else if (e->kind == 1) {
            uint16_t v = (uint16_t)(s[2 * c] << 8 | s[2 * c + 1]);
            memcpy(d + 2 * c, &v, 2);
        } else if (e->kind == 2) {
            uint32_t v = be32(s + 4 * c);
            memcpy(d + 4 * c, &v, 4);
        } else {
            uintptr_t v = be32(s + 4 * c);
            memcpy(d + sizeof v * c, &v, sizeof v);
        }
    }
}

#define CF_N64(e) ((e)->width & 0xFF)
#define CF_HOST(e) ((e)->width >> 8 & 0xFF ? (e)->width >> 8 & 0xFF : CF_N64(e))
#define CF_SIGNED(e) ((e)->width >> 16 & 1)

static int cf_width_ok(unsigned w) { return w == 1 || w == 2 || w == 4 || w == 8; }

/* 1 if every C entry reads inside the image */
static int cfill_check(void) {
    uint32_t l, i;
    for (l = 0; l < port_cfill_nlists; l++) {
        const struct PortCFill *e = port_cfill_lists[l].e;
        uint32_t n = *port_cfill_lists[l].n;
        for (i = 0; i < n; i++, e++) {
            uint64_t last;
            if (!cf_width_ok(CF_N64(e)) || !cf_width_ok(CF_HOST(e)) || CF_HOST(e) < CF_N64(e) ||
                (CF_N64(e) == 1 && CF_HOST(e) != 1) || !e->n1 || !e->n2)
                return 0;
            last = (uint64_t)e->src + (uint64_t)(e->n2 - 1) * e->ns2 + (uint64_t)e->n1 * CF_N64(e);
            if (last > port_asmfill_image_size)
                return 0;
        }
    }
    return 1;
}

/* a C entry's span here */
static size_t cfill_extent(const struct PortCFill *e) {
    return (size_t)(e->n2 - 1) * e->hs2 + (size_t)e->n1 * CF_HOST(e);
}

static NO_ASAN void cfill_put(const uint8_t *img, const struct PortCFill *e) {
    uint32_t i, j, nw = CF_N64(e), hw = CF_HOST(e);
    for (j = 0; j < e->n2; j++) {
        uint8_t *d = (uint8_t *)e->dst + (size_t)j * e->hs2;
        const uint8_t *s = img + e->src + (size_t)j * e->ns2;
        if (hw == 1) {
            memcpy(d, s, e->n1);
            continue;
        }
        for (i = 0; i < e->n1; i++, s += nw, d += hw) {
            uint64_t v;
            if (nw == 2)
                v = (uint16_t)(s[0] << 8 | s[1]);
            else if (nw == 4)
                v = be32(s);
            else
                v = (uint64_t)be32(s) << 32 | be32(s + 4);
            if (CF_SIGNED(e) && nw < 8 && (v >> (nw * 8 - 1) & 1))
                v |= ~(uint64_t)0 << (nw * 8);
            if (hw == 2) {
                uint16_t x = (uint16_t)v;
                memcpy(d, &x, 2);
            } else if (hw == 4) {
                uint32_t x = (uint32_t)v;
                memcpy(d, &x, 4);
            } else {
                memcpy(d, &v, 8);
            }
        }
    }
}

/* every value whose place overlaps [lo, hi) (all of them: lo and hi NULL) */
static void put_all(const uint8_t *img, const char *lo, const char *hi) {
    uint32_t i, l, n = 0, bytes = 0;
    for (i = 0; i < port_asmfill_count; i++) {
        const struct PortAsmFill *e = &port_asmfill[i];
        const char *d = (const char *)e->dst;
        if (lo && (d >= hi || d + asm_extent(e) <= lo))
            continue;
        asm_put(img, e);
    }
    for (l = 0; l < port_cfill_nlists; l++) {
        const struct PortCFill *e = port_cfill_lists[l].e;
        uint32_t k = *port_cfill_lists[l].n;
        for (i = 0; i < k; i++, e++) {
            const char *d = (const char *)e->dst;
            if (lo && (d >= hi || d + cfill_extent(e) <= lo))
                continue;
            cfill_put(img, e);
            n++;
            bytes += e->n1 * CF_N64(e) * e->n2;
        }
    }
    if (host_verbose && !lo)
        host_log("the ROM's data: the asm data's %u runs, the C's %u bytes in %u runs\n",
                 (unsigned)port_asmfill_count, (unsigned)bytes, (unsigned)n);
}

void host_rom_fill(const uint8_t *rom, uint32_t rom_size) {
    uint8_t *img = code_image(rom, rom_size, port_asmfill_image_size);
    if (!img)
        host_fatal("the ROM's data: this ROM's code isn't the one this build was made from (give the %s ROM)",
                   ROM_NAME);
    if (!asm_check(img))
        host_fatal("the ROM's data: this ROM's data isn't what this build was made from (give the %s ROM)",
                   ROM_NAME);
    if (!cfill_check())
        host_fatal("the ROM's data: the executable's tables are damaged");
    put_all(img, NULL, NULL);
    free(image);
    image = img;
}

/* the values of the variables in [lo, hi) again, as at startup (the soft
   reset and an overlay's loading clear a .bss that holds some) */
void host_rom_refill(char *lo, char *hi) {
    if (image && lo && hi > lo)
        put_all(image, lo, hi);
}

/* 1 if ROM is the one this build's data comes from (the asm data's bytes,
   by their checksum, which no other version's or other game's has);
   nothing is written (romfind.c: a ROM the player picks, before it is used) */
int host_rom_fill_check(const uint8_t *rom, uint32_t rom_size) {
    uint8_t *img = code_image(rom, rom_size, port_asmfill_image_size);
    int ok;
    if (!img)
        return 0;
    ok = asm_check(img);
    free(img);
    return ok;
}
