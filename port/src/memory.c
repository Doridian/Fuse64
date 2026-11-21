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

/* (the host's allocator: pc_names.h makes calloc the game's) */
#undef calloc
#undef free
void *calloc(size_t n, size_t m);
void free(void *p);

void *port_alloc(unsigned long size) {
    void *p = calloc(1, size ? size : 1);
    if (p == NULL)
        host_fatal("port_alloc: no memory for %lu bytes", size);
    return p;
}

void port_free(void *p) {
    free(p);
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
