/*
 * -DPORT_HOST_MALLOC=ON (a debugging build): the game's allocator (GNU
 * malloc in the sbrk arena, src/libmalloc, pc_names.h's bm64_ names) is
 * the host's instead, so that AddressSanitizer sees every block the game
 * allocates.  Not for play: the heap's addresses then aren't the
 * game's own layout (docs/NETPLAY.md wants that).
 *
 * The blocks are recorded (sorted by address), so that port_is_game_memory
 * knows them: port_addr takes a block's address plus or minus 0x80000000
 * (a segment base, 47A80.c's) back to the block only where it knows the
 * block is the game's.
 */
#ifdef PORT_HOST_MALLOC
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "port.h"

void *malloc(size_t n);
void free(void *p);
void *realloc(void *p, size_t n);
void *calloc(size_t n, size_t m);

int bm64___malloc_initialized;

typedef struct { uintptr_t lo, hi; } Block;
static Block *blocks;
static size_t nblocks, cap;

/* the first block whose start is above a */
static size_t upper(uintptr_t a) {
    size_t l = 0, h = nblocks;
    while (l < h) {
        size_t m = (l + h) / 2;
        if (blocks[m].lo <= a)
            l = m + 1;
        else
            h = m;
    }
    return l;
}

static void add(void *p, size_t n) {
    if (p == NULL)
        return;
    if (nblocks == cap) {
        cap = cap ? cap * 2 : 1024;
        blocks = realloc(blocks, cap * sizeof *blocks);
        if (blocks == NULL)
            host_fatal("hostmalloc: no memory for the block list");
    }
    size_t i = upper((uintptr_t)p);
    memmove(&blocks[i + 1], &blocks[i], (nblocks - i) * sizeof *blocks);
    blocks[i].lo = (uintptr_t)p;
    blocks[i].hi = (uintptr_t)p + n;
    nblocks++;
}

static void del(void *p) {
    if (p == NULL)
        return;
    size_t i = upper((uintptr_t)p);
    if (i == 0 || blocks[i - 1].lo != (uintptr_t)p)
        host_fatal("hostmalloc: free of %p, no block of the game's", p);
    i--;
    memmove(&blocks[i], &blocks[i + 1], (nblocks - i - 1) * sizeof *blocks);
    nblocks--;
}

int port_hostmalloc_owns(const void *p) {
    size_t i = upper((uintptr_t)p);
    return i > 0 && (uintptr_t)p < blocks[i - 1].hi;
}

/* the size gmalloc gives a request of n (src/libmalloc/gmalloc.c: at least
 * a struct list, 8 bytes on the N64; up to half a 4 KB block a power-of-two
 * fragment, else whole blocks): on the N64 the bytes up to it are the
 * block's.  Here they are the block's too, zero (on the N64 whatever the
 * heap held there).  With PORT_HOST_MALLOC_EXACT=1 a block is the size
 * asked, so that AddressSanitizer reports a read past the request too:
 * the code's own reach past what it asked for, which the N64's rounding
 * hides (the asset cache's TLUT loads did, before 87190.c's
 * PORT_ASSET_PAD). */
static size_t whole(size_t n) {
    static int exact = -1;
    size_t w = 8;

    if (exact < 0) {
        const char *e = getenv("PORT_HOST_MALLOC_EXACT");
        exact = e != NULL && *e != '\0' && *e != '0';
    }
    if (exact)
        return n ? n : 1;
    if (n > 0x800)
        return (n + 0xFFF) & ~(size_t)0xFFF;
    while (w < n)
        w *= 2;
    return w;
}

void *bm64_malloc(size_t n) {
    size_t w = whole(n);
    void *p = malloc(w);
    if (p != NULL)
        memset((char *)p + n, 0, w - n);
    add(p, w);
    return p;
}

void bm64_free(void *p) {
    del(p);
    free(p);
}

void *bm64_realloc(void *p, size_t n) {
    del(p);
    size_t w = whole(n);
    p = realloc(p, w);
    if (p != NULL)
        memset((char *)p + n, 0, w - n);
    add(p, w);
    return p;
}

void *bm64_calloc(size_t n, size_t m) {
    void *p = calloc(n ? n : 1, m ? m : 1);
    add(p, (n ? n : 1) * (m ? m : 1));
    return p;
}
#endif
