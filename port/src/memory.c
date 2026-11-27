/*
 * Addresses (docs/PORT.md, "Memory").
 *
 * The game's variables are the host's, wherever the linker puts them; what
 * the N64 has at fixed addresses is port_rdram (gen/regions.c), laid out as
 * the N64's RDRAM.  Display lists, DMAs and the VI take an address in any of
 * the forms the game's C makes: a pointer, a KSEG0 or KSEG1 address
 * (0x80000000 + physical: a fixed one, 0x80043000), a physical one (below
 * 8 MiB), and, where pointers are 4 bytes, a pointer the game turned into a
 * "physical" one by subtracting 0x80000000 (gSPSegment's bases: 412F0,
 * 47A80).  port_addr tells them apart by where the game's memory is.
 */
#include <stdint.h>
#include <stdlib.h>
#include "port.h"

#if defined(__linux__) && !defined(__EMSCRIPTEN__)
extern char __executable_start[], _end[];
#define IMAGE_LO ((uintptr_t)__executable_start)
#define IMAGE_HI ((uintptr_t)_end)
#elif defined(_WIN32)
/* the PE image: from __ImageBase, SizeOfImage bytes (its optional header's,
   at the same offset in PE32 and PE32+) */
extern char __ImageBase[];
#define IMAGE_LO ((uintptr_t)__ImageBase)
#define IMAGE_HI (IMAGE_LO + *(const uint32_t *)(__ImageBase + *(const int32_t *)(__ImageBase + 0x3C) + 0x50))
#endif

int port_coro_stack(const void *p);
#ifdef PORT_HOST_MALLOC
int port_hostmalloc_owns(const void *p);
#endif

int port_is_game_memory(const void *p) {
    uintptr_t a = (uintptr_t)p;
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return 1;
#ifdef PORT_HOST_MALLOC
    /* (the game's blocks, the host's: host/hostmalloc.c) */
    if (port_hostmalloc_owns(p))
        return 1;
#endif
#if defined(__EMSCRIPTEN__)
    /* the data from 256 MiB (-sGLOBAL_BASE, CMakeLists.txt) to the memory's end */
    return a >= 0x10000000u && a < (uintptr_t)__builtin_wasm_memory_size(0) * 65536u;
#elif defined(IMAGE_LO)
    if (a >= IMAGE_LO && a < IMAGE_HI)
        return 1;
    return host_in_thread_stack(p) || port_coro_stack(p);
#else
    return a >= 0x10000;
#endif
}

/* gen/regions.c: where absolute symbols can't be (WebAssembly), the
   segment and physical addresses the game names are small objects */
extern const struct { uint8_t *p; uint32_t v; } port_abs_proxies[];
extern const unsigned port_nabs_proxies;

uintptr_t port_unproxy(uintptr_t a) {
    for (unsigned i = 0; i < port_nabs_proxies; i++)
        if (a >= (uintptr_t)port_abs_proxies[i].p && a < (uintptr_t)port_abs_proxies[i].p + 16)
            return port_abs_proxies[i].v + (a - (uintptr_t)port_abs_proxies[i].p);
    return a;
}

void *port_addr(uintptr_t a) {
    a = port_unproxy(a);
    /* (the program is linked above 8 MiB, so no pointer of its own looks
       physical: CMakeLists.txt's image base) */
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return (void *)a;
    if (a < PORT_RDRAM_SIZE)
        return port_rdram + a;
    if (port_is_game_memory((void *)a))
        return (void *)a;
    if ((uint32_t)a >= 0x80000000u && (uint32_t)a < 0x80000000u + PORT_RDRAM_SIZE && a == (uint32_t)a)
        return port_rdram + ((uint32_t)a - 0x80000000u);
    if ((uint32_t)a >= 0xA0000000u && (uint32_t)a < 0xA0000000u + PORT_RDRAM_SIZE && a == (uint32_t)a)
        return port_rdram + ((uint32_t)a - 0xA0000000u);
#if UINTPTR_MAX == 0xFFFFFFFFu
    if (port_is_game_memory((void *)(a + 0x80000000u)))
        return (void *)(a + 0x80000000u);
#else
    /* (where pointers are 8 bytes the game's pointer less 0x80000000 is
       still all of it) */
    if (a > 0xFFFFFFFFu && port_is_game_memory((void *)(a + 0x80000000u)))
        return (void *)(a + 0x80000000u);
    /* (and one plus 0x80000000, which on the N64 wraps to the physical
       address: 47A80.c's segment bases; on a host whose image is below
       4 GiB, a static aarch64 one, that sum is a 32-bit number) */
    if (a >= 0x80000000u && port_is_game_memory((void *)(a - 0x80000000u)))
        return (void *)(a - 0x80000000u);
#endif
    return (void *)a;
}

#ifdef PORT_SCATTER_LAYOUT
/* gen/scatter.c's objects in its arena, by offset (and its end) */
typedef struct { const char *name; uint32_t off, size, n64, n64size; } PortScatterObj;
extern const PortScatterObj port_scatter_objs[];
extern const unsigned port_scatter_nobjs;
extern uint8_t port_scatter_arena[];
#endif

/* The N64's (KSEG0) address of p, a place in a name the N64 has at a fixed
   address: port_rdram's offset, or in the scattered layout the object's own
   N64 address.  For what the N64 leaves of such a pointer where the game
   reads it back (9CAB0.c's stack byte). */
uint32_t port_n64_address(const void *p) {
    uintptr_t a = (uintptr_t)p;
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return 0x80000000u + (uint32_t)(a - (uintptr_t)port_rdram);
#ifdef PORT_SCATTER_LAYOUT
    uintptr_t ar = (uintptr_t)port_scatter_arena;
    if (a >= ar && a < ar + port_scatter_objs[port_scatter_nobjs].off) {
        uint32_t off = (uint32_t)(a - ar);
        unsigned l = 0, h = port_scatter_nobjs;
        while (l < h) {     /* the first object starting after off */
            unsigned m = (l + h) / 2;
            if (port_scatter_objs[m].off <= off)
                l = m + 1;
            else
                h = m;
        }
        if (l && off < port_scatter_objs[l - 1].off + port_scatter_objs[l - 1].size)
            return port_scatter_objs[l - 1].n64 + (off - port_scatter_objs[l - 1].off);
    }
#endif
    host_fatal("port_n64_address: %p is no fixed-address name's", p);
}

/* The other way: the host's place of the N64's (KSEG0) address a, NULL in
   the scattered layout where no object has it (the VI's image) */
void *port_n64_host(uint32_t a) {
#ifdef PORT_SCATTER_LAYOUT
    static unsigned last;
    const PortScatterObj *o = &port_scatter_objs[last];
    if (a - o->n64 < o->size)
        return port_scatter_arena + o->off + (a - o->n64);
    for (unsigned i = 0; i < port_scatter_nobjs; i++) {
        o = &port_scatter_objs[i];
        if (a - o->n64 < o->size) {
            last = i;
            return port_scatter_arena + o->off + (a - o->n64);
        }
    }
    return NULL;
#else
    return port_rdram + ((a - 0x80000000u) & (PORT_RDRAM_SIZE - 1));
#endif
}

/* port_alloc's blocks (80580.c's sounds as the host lays them out): an
   arena of their own in the image, not the host's heap, since the game's
   structures keep pointers to them and RDRAM must not depend on what else
   the host allocated (the GL driver, from its threads too: docs/PORT.md,
   "The host's blocks the game keeps").  First fit, in address order; the
   blocks' records are apart from it, and each block has 16 bytes after it
   that no block has.  The NMI (port_alloc_reset) empties it, as at
   power-on.  With AddressSanitizer (PORT_ASAN) everything but the blocks'
   bytes is poisoned, so that ASan reports an overrun or a use after
   port_free as it would the host's calloc's. */
#ifdef PORT_ASAN
#include <sanitizer/asan_interface.h>
#define POISON(p, n) ASAN_POISON_MEMORY_REGION(p, n)
#define UNPOISON(p, n) ASAN_UNPOISON_MEMORY_REGION(p, n)
#else
#define POISON(p, n) ((void)(p), (void)(n))
#define UNPOISON(p, n) ((void)(p), (void)(n))
#endif

#define ARENA_SIZE (1u << 20)
#define ARENA_BLOCKS 4096
#define REDZONE 16u
typedef struct {
    uint32_t off, size;         /* the block's place and room, with its redzone */
    uint32_t used;              /* what was asked (at least 1), 0: free */
} ArenaBlock;
static uint8_t arena[ARENA_SIZE] __attribute__((aligned(16)));
static ArenaBlock blocks[ARENA_BLOCKS];        /* in address order, from 0 to top */
static unsigned nblocks;
static uint32_t top, high;
static int arena_ready;

void port_alloc_reset(void) {
    nblocks = 0;
    top = 0;
    __builtin_memset(arena, 0, sizeof arena);
    POISON(arena, sizeof arena);
    arena_ready = 1;
}

void *port_alloc(unsigned long size) {
    uint32_t used = size ? (uint32_t)size : 1, need;
    unsigned i;
    ArenaBlock *b = NULL;
    if (!arena_ready)
        port_alloc_reset();
    if (size > ARENA_SIZE)
        host_fatal("port_alloc: no room for %lu bytes", size);
    need = ((used + 15u) & ~15u) + REDZONE;
    for (i = 0; i < nblocks; i++)
        if (!blocks[i].used && blocks[i].size >= need) {
            b = &blocks[i];
            if (b->size - need >= 16 + REDZONE) {
                /* (the rest a free block of its own) */
                if (nblocks == ARENA_BLOCKS)
                    host_fatal("port_alloc: more than %d blocks", ARENA_BLOCKS);
                __builtin_memmove(&blocks[i + 2], &blocks[i + 1], (nblocks - i - 1) * sizeof *blocks);
                nblocks++;
                blocks[i + 1].off = b->off + need;
                blocks[i + 1].size = b->size - need;
                blocks[i + 1].used = 0;
                b->size = need;
            }
            break;
        }
    if (b == NULL) {
        if (ARENA_SIZE - top < need)
            host_fatal("port_alloc: no room for %lu bytes (the arena's 0x%X, 0x%X in use)", size, ARENA_SIZE, top);
        if (nblocks == ARENA_BLOCKS)
            host_fatal("port_alloc: more than %d blocks", ARENA_BLOCKS);
        b = &blocks[nblocks++];
        b->off = top;
        b->size = need;
        top += need;
        if (top > high) {
            high = top;
            if (host_verbose > 1)
                host_log("port_alloc: 0x%X bytes of the arena in use, %u blocks\n", top, nblocks);
        }
    }
    b->used = used;
    UNPOISON(arena + b->off, used);
    __builtin_memset(arena + b->off, 0, used);
#ifdef PORT_AUDIO_RECORD
    {
        /* (the libaudio oracle's recorder: the blocks are 80580.c's sounds) */
        void host_alog_block(const void *p, unsigned int len);
        host_alog_block(arena + b->off, used);
    }
#endif
    return arena + b->off;
}

void port_free(void *p) {
    unsigned l = 0, h = nblocks, i;
    uint32_t off;
    if (p == NULL)
        return;
    off = (uint32_t)((uint8_t *)p - arena);
    while (l < h) {     /* the block at off */
        unsigned m = (l + h) / 2;
        if (blocks[m].off < off)
            l = m + 1;
        else
            h = m;
    }
    i = l;
    if ((uint8_t *)p < arena || i == nblocks || blocks[i].off != off || !blocks[i].used)
        host_fatal("port_free: %p is no block of port_alloc's (or one freed)", p);
#ifdef PORT_AUDIO_RECORD
    {
        void host_alog_block_free(const void *p);
        host_alog_block_free(p);
    }
#endif
    POISON(arena + off, blocks[i].used);
    blocks[i].used = 0;
    /* (joined to the free blocks beside it; one at the top is the top's) */
    if (i + 1 < nblocks && !blocks[i + 1].used) {
        blocks[i].size += blocks[i + 1].size;
        __builtin_memmove(&blocks[i + 1], &blocks[i + 2], (nblocks - i - 2) * sizeof *blocks);
        nblocks--;
    }
    if (i > 0 && !blocks[i - 1].used) {
        blocks[i - 1].size += blocks[i].size;
        __builtin_memmove(&blocks[i], &blocks[i + 1], (nblocks - i - 1) * sizeof *blocks);
        nblocks--;
        i--;
    }
    if (i == nblocks - 1) {
        top = blocks[i].off;
        nblocks--;
    }
}

unsigned char *port_audio_heap(unsigned char *n64, int *len) {
#if PORT_LP64
    static unsigned char heap[2 * 300000] __attribute__((aligned(16)));
    if (*len * 2 > (int)sizeof heap)
        host_fatal("port_audio_heap: %d bytes", *len);
    *len *= 2;
    return heap;
#else
    (void)len;
    return n64;
#endif
}
