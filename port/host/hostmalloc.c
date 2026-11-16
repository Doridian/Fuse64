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

/* gmalloc's blocks are whole 8-byte fragments or more (src/libmalloc), so
 * the bytes up to the next multiple of 8 are the block's on the N64: the
 * legal screen's TLUT load reads two of them past its picture (87190.c's
 * asset, 38450 bytes in us and eu).  Here they are the block's too, zero
 * (on the N64 whatever the heap held there; no texel uses them). */
static size_t whole(size_t n) {
    return n ? (n + 7) & ~(size_t)7 : 8;
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
