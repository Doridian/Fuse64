/*
 * The ROM's data, taken from the player's ROM at startup (docs/PORT.md,
 * "The ROM's data").
 *
 * The game's initialized data (its C's initializers, asm2c's, the overlays'
 * .data and .rodata) is the ROM's.  After the link, port/tools/rom_strip.py
 * finds every run of it in the executable (rom_free_check.py's matching:
 * 4-byte chunks of the code image, each in one of the orders a host gives a
 * word's fields), zeroes those bytes in the file and writes where they go and
 * where they come from into port_romfill, which is reserved here.  Before
 * anything reads them (main, right after the ROM is loaded), host_rom_fill
 * builds the code image from the ROM as tools/layout.py does (the ROM below
 * its first archive, then every overlay inflated, archive by archive) and
 * puts each run back, then checks the result against the checksum the strip
 * took of the bytes it removed.  An executable the strip hasn't run on has
 * no runs (nruns 0) and is left as it is.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "port.h"
#if defined(_WIN32)
#include <windows.h>
#elif !defined(__EMSCRIPTEN__)
#include <sys/mman.h>
#include <unistd.h>
#endif

#if defined(VERSION_JP)
#define ROM_NAME "Baku Bomberman (Japan)"
#elif defined(VERSION_EU)
#define ROM_NAME "Bomberman 64 (Europe)"
#else
#define ROM_NAME "Bomberman 64 (USA)"
#endif

#define ROMFILL_CAP (96 * 1024)

/* The table, in the target's byte order (rom_strip.py writes it):
     0  magic[16]
    16  u32 capacity, u32 format (1), u32 nruns, u32 code image size
    32  u64 FNV-1a of every byte the runs give, in the runs' order
    40  u32 the words in all the runs, u32 (reserved)
    48  nruns entries: s32 the run's address less the table's, u32 its
        offset in the code image, u32 its words (bit 31: in read-only memory)
        then a nibble per word, two to a byte, low first: the word's order
        (ORDERS) */
struct RomFillHeader {
    char magic[16];
    uint32_t capacity, format, nruns, image_size;
    uint64_t checksum;
    uint32_t nwords, reserved;
};
struct RomFillRun {
    int32_t delta;
    uint32_t src, words;
};

/* filled with 0xA5 past the magic, so that no tool takes the space for
   zeros (wasm-opt splits zeros out of the data segments) */
#define R4(x) x, x, x, x
#define R16(x) R4(x), R4(x), R4(x), R4(x)
#define R64(x) R16(x), R16(x), R16(x), R16(x)
#define R256(x) R64(x), R64(x), R64(x), R64(x)
#define R1K(x) R256(x), R256(x), R256(x), R256(x)
#define R4K(x) R1K(x), R1K(x), R1K(x), R1K(x)
#define R16K(x) R4K(x), R4K(x), R4K(x), R4K(x)
__attribute__((used, aligned(16))) unsigned char port_romfill[ROMFILL_CAP] = {
    'F', 'u', 's', 'e', '6', '4', ' ', 'R', 'O', 'M', ' ', 'f', 'i', 'l', 'l', 0,
    R16(0xA5) /* the rest of the header */, R16(0xA5),
    R16K(0xA5), R16K(0xA5), R16K(0xA5), R16K(0xA5), R16K(0xA5),
    R4K(0xA5), R4K(0xA5), R4K(0xA5), R1K(0xA5), R1K(0xA5), R1K(0xA5),
    R256(0xA5), R256(0xA5), R256(0xA5), R64(0xA5), R64(0xA5), R64(0xA5),
    R16(0xA5),
};
_Static_assert(sizeof port_romfill == ROMFILL_CAP, "the table's size");

/* a host word's bytes from the ROM's: rom_free_check.py's PERMS */
static const unsigned char ORDERS[5][4] = {
    {0, 1, 2, 3}, {1, 0, 3, 2}, {3, 2, 1, 0}, {0, 1, 3, 2}, {1, 0, 2, 3},
};

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

/* the pages of [p, p + n) writable, or back to read-only */
static void protect(uint8_t *p, size_t n, int writable) {
#if defined(_WIN32)
    DWORD old;
    if (!VirtualProtect(p, n, writable ? PAGE_READWRITE : PAGE_READONLY, &old))
        host_fatal("the ROM's data: VirtualProtect failed");
#elif defined(__EMSCRIPTEN__)
    (void)p, (void)n, (void)writable;
#else
    uintptr_t pg = (uintptr_t)sysconf(_SC_PAGESIZE);
    uintptr_t lo = (uintptr_t)p & ~(pg - 1), hi = ((uintptr_t)p + n + pg - 1) & ~(pg - 1);
    if (mprotect((void *)lo, hi - lo, writable ? PROT_READ | PROT_WRITE : PROT_READ))
        host_fatal("the ROM's data: mprotect failed");
#endif
}

/* a run's place: the table's address and the run's distance from it */
static uint8_t *at(unsigned char *table, int32_t delta) {
    return (uint8_t *)((uintptr_t)table + (uintptr_t)(intptr_t)delta);
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

/* 1 if the image's bytes the asm data takes are the ones the build was made
   from (their FNV-1a); with WRITE, they are put in place too */
static int asm_fill(const uint8_t *img, int write) {
    static const uint32_t width[4] = {1, 2, 4, 4};
    uint64_t sum = 0xCBF29CE484222325ull;
    uint32_t i, c, n;
    for (i = 0; i < port_asmfill_count; i++) {
        const struct PortAsmFill *e = &port_asmfill[i];
        const uint8_t *s = img + e->src;
        uint8_t *d = e->dst;
        if (e->kind > 3 || e->src > port_asmfill_image_size ||
            e->count > (port_asmfill_image_size - e->src) / width[e->kind])
            return 0;
        n = e->count * width[e->kind];
        for (c = 0; c < n; c++)
            sum = (sum ^ s[c]) * 0x100000001B3ull;
        if (!write)
            continue;
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
    return sum == port_asmfill_sum;
}

NO_ASAN void host_rom_fill(const uint8_t *rom, uint32_t rom_size) {
    unsigned char *table = port_romfill;
    const struct RomFillHeader *h;
    const struct RomFillRun *runs;
    const uint8_t *orders;
    uint8_t *img;
    uint64_t sum = 0xCBF29CE484222325ull;
    uint32_t i, w, k = 0;
    int stripped;
    /* (the compiler knows nothing of what the strip wrote) */
    __asm__ volatile("" : "+r"(table));
    h = (const struct RomFillHeader *)table;
    /* (the magic is for the strip to find the table by: no copy of it
       elsewhere in the program) */
    stripped = h->format != 0xA5A5A5A5u && h->nruns != 0;
    if (stripped && (h->format != 1 || h->capacity != ROMFILL_CAP || h->image_size != port_asmfill_image_size))
        host_fatal("the ROM's data: the executable's table is not this build's");
    img = code_image(rom, rom_size, port_asmfill_image_size);
    if (!img)
        host_fatal("the ROM's data: this ROM's code isn't the one this build was made from (give the %s ROM)",
                   ROM_NAME);
    if (!asm_fill(img, 0))
        host_fatal("the ROM's data: this ROM's data isn't what this build was made from (give the %s ROM)",
                   ROM_NAME);
    asm_fill(img, 1);
    if (!stripped) {
        free(img);
        return;
    }
    runs = (const struct RomFillRun *)(table + sizeof *h);
    orders = (const uint8_t *)(runs + h->nruns);
    for (i = 0; i < h->nruns; i++)
        if (runs[i].words >> 31)
            protect(at(table, runs[i].delta), (runs[i].words & 0x7FFFFFFF) * 4, 1);
    for (i = 0; i < h->nruns; i++) {
        uint8_t *d = at(table, runs[i].delta);
        const uint8_t *s = img + runs[i].src;
        uint32_t n = runs[i].words & 0x7FFFFFFF;
        if (runs[i].src > h->image_size || n > (h->image_size - runs[i].src) / 4)
            host_fatal("the ROM's data: the executable's table is damaged");
        for (w = 0; w < n; w++, k++) {
            const unsigned char *o = ORDERS[(orders[k >> 1] >> (k & 1) * 4) & 0xF];
            int j;
            for (j = 0; j < 4; j++) {
                d[4 * w + j] = s[4 * w + o[j]];
                sum = (sum ^ d[4 * w + j]) * 0x100000001B3ull;
            }
        }
    }
    for (i = 0; i < h->nruns; i++)
        if (runs[i].words >> 31)
            protect(at(table, runs[i].delta), (runs[i].words & 0x7FFFFFFF) * 4, 0);
    free(img);
    if (k != h->nwords || sum != h->checksum)
        host_fatal("the ROM's data: this ROM's data isn't what this build was made from (give the %s ROM)",
                   ROM_NAME);
    if (host_verbose)
        host_log("the ROM's data: %u bytes in %u runs\n", (unsigned)k * 4, (unsigned)h->nruns);
}

/* 1 if ROM is the one this build's data comes from (the bytes the asm
   data takes and the strip took out, by their checksums); nothing is
   written (romfind.c: a ROM the player picks, before it is used) */
NO_ASAN int host_rom_fill_check(const uint8_t *rom, uint32_t rom_size) {
    unsigned char *table = port_romfill;
    const struct RomFillHeader *h;
    const struct RomFillRun *runs;
    const uint8_t *orders;
    uint8_t *img;
    uint64_t sum = 0xCBF29CE484222325ull;
    uint32_t i, w, k = 0;
    int ok = 1;
    __asm__ volatile("" : "+r"(table));
    h = (const struct RomFillHeader *)table;
    img = code_image(rom, rom_size, port_asmfill_image_size);
    if (!img)
        return 0;
    if (!asm_fill(img, 0)) {
        free(img);
        return 0;
    }
    if (h->format == 0xA5A5A5A5u || h->nruns == 0) {
        free(img);
        return 1;
    }
    if (h->format != 1 || h->capacity != ROMFILL_CAP || h->image_size != port_asmfill_image_size) {
        free(img);
        return 0;
    }
    runs = (const struct RomFillRun *)(table + sizeof *h);
    orders = (const uint8_t *)(runs + h->nruns);
    for (i = 0; ok && i < h->nruns; i++) {
        const uint8_t *s = img + runs[i].src;
        uint32_t n = runs[i].words & 0x7FFFFFFF;
        if (runs[i].src > h->image_size || n > (h->image_size - runs[i].src) / 4) {
            ok = 0;
            break;
        }
        for (w = 0; w < n; w++, k++) {
            const unsigned char *o = ORDERS[(orders[k >> 1] >> (k & 1) * 4) & 0xF];
            int j;
            for (j = 0; j < 4; j++)
                sum = (sum ^ s[4 * w + o[j]]) * 0x100000001B3ull;
        }
    }
    free(img);
    return ok && k == h->nwords && sum == h->checksum;
}
