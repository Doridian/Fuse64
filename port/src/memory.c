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
#include <string.h>
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
#ifdef PORT_LAYOUT_N64
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return 1;
#endif
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

/* the place of the N64 layout's RDRAM offset off that the game reached by
   a number (port_addr) */
static void *numeric(uint32_t off) {
#ifdef PORT_LAYOUT_N64
    return port_rdram + off;
#else
    /* (where no object is, a page of zeros that nothing else has, as the
       N64 layout's port_rdram has zeros there: 412F0.c's func_80227678
       gives a number before the first frame, 0x10000, which the game
       makes a segment's base; nothing reads it) */
    static uint8_t nowhere[0x40000] __attribute__((aligned(16)));
    static uint32_t said = 0xFFFFFFFFu;
    void *p = port_image_host(off, NULL);
    if (p != NULL)
        return p;
    if (host_verbose && said != off)
        host_log("port_addr: the N64's address 0x%08X is in no fixed-address name\n", 0x80000000u + off);
    said = off;
    return nowhere + (off & 0x1FFFF);
#endif
}

void *port_addr(uintptr_t a) {
    a = port_unproxy(a);
    /* (the program is linked above 8 MiB, so no pointer of its own looks
       physical: CMakeLists.txt's image base) */
#ifdef PORT_LAYOUT_N64
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return (void *)a;
#endif
    if (a < PORT_RDRAM_SIZE)
        return numeric((uint32_t)a);
    if (port_is_game_memory((void *)a))
        return (void *)a;
    if ((uint32_t)a >= 0x80000000u && (uint32_t)a < 0x80000000u + PORT_RDRAM_SIZE && a == (uint32_t)a)
        return numeric((uint32_t)a - 0x80000000u);
    if ((uint32_t)a >= 0xA0000000u && (uint32_t)a < 0xA0000000u + PORT_RDRAM_SIZE && a == (uint32_t)a)
        return numeric((uint32_t)a - 0xA0000000u);
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

#ifndef PORT_LAYOUT_N64
/* The default layout's objects (docs/PORT.md, "The default layout"): each
   fixed-address name (gen/regions.c's port_region_objs) and each asm data
   label (gen/asm_data_fill.c's port_asm_objs) an object of its own, or in
   the scattered layout each one's place in its arena (gen/scatter.c, all
   of them in port_region_objs).  Each has its N64 (KSEG0) address and,
   for a fixed-address name, its offset in the N64 layout's port_rdram
   (the "image": what the RDRAM dumps and the renderer's and the audio's
   references are made of, so that they stay the same files). */
extern const PortObj port_region_objs[], port_asm_objs[];
extern const unsigned port_region_nobjs, port_asm_nobjs;

static const PortObj **by_p, **by_n64, **by_img;
static unsigned n_obj, n_img;

static int cmp_p(const void *a, const void *b) {
    uintptr_t x = (uintptr_t)(*(const PortObj *const *)a)->p, y = (uintptr_t)(*(const PortObj *const *)b)->p;
    return x < y ? -1 : x > y;
}
static int cmp_n64(const void *a, const void *b) {
    uint32_t x = (*(const PortObj *const *)a)->n64, y = (*(const PortObj *const *)b)->n64;
    return x < y ? -1 : x > y;
}
static int cmp_img(const void *a, const void *b) {
    uint32_t x = (*(const PortObj *const *)a)->img, y = (*(const PortObj *const *)b)->img;
    return x < y ? -1 : x > y;
}

/* (the host's malloc, not the game's heap, which pc_names.h's malloc is:
   the index is the host's own) */
#undef malloc
void *malloc(size_t n);

static void obj_init(void) {
    unsigned i;
    if (by_p)
        return;
    n_obj = port_region_nobjs + port_asm_nobjs;
    by_p = malloc(sizeof *by_p * n_obj * 3);
    if (!by_p)
        host_fatal("no memory for the objects' index");
    by_n64 = by_p + n_obj;
    by_img = by_n64 + n_obj;
    for (i = 0; i < n_obj; i++) {
        const PortObj *o = i < port_region_nobjs ? &port_region_objs[i] : &port_asm_objs[i - port_region_nobjs];
        by_p[i] = by_n64[i] = o;
        if (o->img != PORT_OBJ_NO_IMG)
            by_img[n_img++] = o;
    }
    qsort(by_p, n_obj, sizeof *by_p, cmp_p);
    qsort(by_n64, n_obj, sizeof *by_n64, cmp_n64);
    qsort(by_img, n_img, sizeof *by_img, cmp_img);
}

/* the object p is in (NULL: none) */
const PortObj *port_obj_of(const void *p) {
    static const PortObj *last, *prev;     /* (the renderer's color and depth images) */
    const PortObj *o;
    uintptr_t a = (uintptr_t)p;
    unsigned l = 0, h;
    if (last && a - (uintptr_t)last->p < last->size)
        return last;
    if (prev && a - (uintptr_t)prev->p < prev->size) {
        o = prev;
        prev = last;
        return last = o;
    }
    obj_init();
    h = n_obj;
    while (l < h) {     /* the first object starting after p */
        unsigned m = (l + h) / 2;
        if ((uintptr_t)by_p[m]->p <= a)
            l = m + 1;
        else
            h = m;
    }
    if (l && a - (uintptr_t)by_p[l - 1]->p < by_p[l - 1]->size) {
        prev = last;
        return last = by_p[l - 1];
    }
    return NULL;
}

/* the object with the N64 (KSEG0) address a (NULL: none) */
static const PortObj *obj_n64(uint32_t a) {
    static const PortObj *last;
    unsigned l = 0, h;
    if (last && a - last->n64 < last->size)
        return last;
    obj_init();
    h = n_obj;
    while (l < h) {
        unsigned m = (l + h) / 2;
        if (by_n64[m]->n64 <= a)
            l = m + 1;
        else
            h = m;
    }
    if (l && a - by_n64[l - 1]->n64 < by_n64[l - 1]->size)
        return last = by_n64[l - 1];
    return NULL;
}

long port_image_off(const void *p) {
    const PortObj *o = port_obj_of(p);
    if (!o || o->img == PORT_OBJ_NO_IMG)
        return -1;
    return (long)(o->img + (uint32_t)((const uint8_t *)p - o->p));
}

size_t port_image_room(const void *p) {
    const PortObj *o = port_obj_of(p);
    if (!o || o->img == PORT_OBJ_NO_IMG)
        return 0;
    return o->size - (size_t)((const uint8_t *)p - o->p);
}

void *port_image_host(uint32_t off, size_t *room) {
    static const PortObj *last;
    const PortObj *o = last;
    if (!o || off - o->img >= o->size) {
        unsigned l = 0, h;
        obj_init();
        h = n_img;
        while (l < h) {
            unsigned m = (l + h) / 2;
            if (by_img[m]->img <= off)
                l = m + 1;
            else
                h = m;
        }
        if (!l || off - by_img[l - 1]->img >= by_img[l - 1]->size)
            return NULL;
        last = o = by_img[l - 1];
    }
    if (room)
        *room = o->size - (off - o->img);
    return o->p + (off - o->img);
}

void port_image_copy(uint8_t *dst) {
    unsigned i;
    obj_init();
    memset(dst, 0, PORT_RDRAM_SIZE);
    for (i = 0; i < n_img; i++)
        memcpy(dst + by_img[i]->img, by_img[i]->p, by_img[i]->size);
}

void port_image_each(uint32_t lo, uint32_t hi, void (*f)(const void *p, size_t n)) {
    unsigned i;
    obj_init();
    for (i = 0; i < n_img; i++) {
        uint32_t a = by_img[i]->img, b = a + by_img[i]->size;
        if (b <= lo || a >= hi)
            continue;
        a = a > lo ? a : lo;
        b = b < hi ? b : hi;
        f(by_img[i]->p + (a - by_img[i]->img), b - a);
    }
}

void port_image_zero(uint32_t end) {
    unsigned i;
    obj_init();
    for (i = 0; i < n_img && by_img[i]->img < end; i++) {
        uint32_t n = end - by_img[i]->img;
        memset(by_img[i]->p, 0, n < by_img[i]->size ? n : by_img[i]->size);
    }
}
#else
long port_image_off(const void *p) {
    uintptr_t a = (uintptr_t)p;
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return (long)(a - (uintptr_t)port_rdram);
    return -1;
}

size_t port_image_room(const void *p) {
    long off = port_image_off(p);
    return off < 0 ? 0 : PORT_RDRAM_SIZE - (size_t)off;
}

void *port_image_host(uint32_t off, size_t *room) {
    if (off >= PORT_RDRAM_SIZE)
        return NULL;
    if (room)
        *room = PORT_RDRAM_SIZE - off;
    return port_rdram + off;
}

void port_image_copy(uint8_t *dst) { memcpy(dst, port_rdram, PORT_RDRAM_SIZE); }

void port_image_each(uint32_t lo, uint32_t hi, void (*f)(const void *p, size_t n)) {
    if (hi > PORT_RDRAM_SIZE)
        hi = PORT_RDRAM_SIZE;
    if (lo < hi)
        f(port_rdram + lo, hi - lo);
}

void port_image_zero(uint32_t end) { memset(port_rdram, 0, end); }
#endif

/* The N64's (KSEG0) address of p, a place in a name the N64 has at a fixed
   address: port_rdram's offset in the N64 layout, else the object's own
   N64 address.  For what the N64 leaves of such a pointer where the game
   reads it back (9CAB0.c's stack byte), and the VI's origin. */
uint32_t port_n64_address(const void *p) {
#ifdef PORT_LAYOUT_N64
    uintptr_t a = (uintptr_t)p;
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
        return 0x80000000u + (uint32_t)(a - (uintptr_t)port_rdram);
#else
    const PortObj *o = port_obj_of(p);
    if (o)
        return o->n64 + (uint32_t)((const uint8_t *)p - o->p);
#endif
    host_fatal("port_n64_address: %p is no fixed-address name's", p);
}

/* The other way: the host's place of the N64's (KSEG0) address a, NULL
   where no object has it (the VI's image past the framebuffers) */
void *port_n64_host(uint32_t a) {
#ifdef PORT_LAYOUT_N64
    return port_rdram + ((a - 0x80000000u) & (PORT_RDRAM_SIZE - 1));
#else
    const PortObj *o = obj_n64(a);
    return o ? o->p + (a - o->n64) : NULL;
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
