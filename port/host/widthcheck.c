/*
 * The width check (-DPORT_WIDTHCHECK=ON; docs/PORT.md, "The width
 * check"): a debugging build that finds where the game's C reads memory
 * with another width or at another offset than it was written, which is
 * right on the N64 (big-endian) and wrong where the host is little-endian:
 * a word stored as an s32 and read back as the s16 at its offset 2, bytes
 * stored one at a time and read as a word, a word of the ROM's read as it
 * came (not converted, port/src/native.c).
 *
 * The game's C (game_c, ovl_c) is compiled with AddressSanitizer's
 * instrumentation alone, every access a call (`-mllvm
 * -asan-instrumentation-with-call-threshold=0`, no globals, no stack, and
 * not linked with ASan's runtime): the calls are these functions.  Every
 * byte the game's C or the host's marks write has a shadow (made as it is
 * first written, 64 KiB at a time): the width of the store that last wrote
 * it, the byte's place in that store, and where the store was.  A load whose
 * bytes aren't one store of its own width, from its first byte, is reported
 * once per pair of places (load, store), with a count, at exit (or a crash),
 * to $PORT_WIDTHCHECK_OUT (stderr by default) as offsets in the executable;
 * port/tools/widthcheck.py symbolizes them.
 *
 * A load followed at once by a store of its width is taken for a copy
 * (`*(s32 *)&p->unk8 = *(s32 *)&q->unk8`, two halves at a time): the store
 * takes the load's shadow and the load isn't reported.  The host's writes
 * (port_wc_mark): the ROM's bytes as they arrive are "raw" (a load wider
 * than a byte of them is reported: data no conversion reached),
 * port_native32/16's words and halves; memset's are any width, memcpy's
 * the source's shadow.  The game's initialized variables start with their
 * types' shadow ($PORT_WIDTHCHECK_DATA, from widthcheck.py data; "store" 1
 * in the report).  32-bit hosts only (the shadow's table).
 */
#ifdef PORT_WIDTHCHECK
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "port.h"

#if UINTPTR_MAX != 0xFFFFFFFFu
#error "the width check's shadow table is for 32-bit hosts"
#endif

extern char __executable_start[], _end[];
int port_coro_stack(const void *p);

#define W_ANY 15  /* memset: any width */
#define W_RAW 14  /* the ROM's bytes, unconverted */

#define CHUNK 0x10000
typedef struct {
    uint8_t info[CHUNK];  /* per byte: the store's width (low 4 bits), the byte's place in it (high 4) */
    uint32_t pc[CHUNK];   /* per byte: where the store was */
} Chunk;
static Chunk *table[0x10000];

static Chunk *chunk(uintptr_t a, int make) {
    Chunk *c = table[a >> 16];
    if (!c && make) {
        c = table[a >> 16] = calloc(1, sizeof *c);
        if (!c) {
            fprintf(stderr, "widthcheck: no memory for the shadow\n");
            abort();
        }
    }
    return c;
}

static void put(uintptr_t a, uint8_t info, uint32_t pc) {
    Chunk *c = chunk(a, 1);
    c->info[a & (CHUNK - 1)] = info;
    c->pc[a & (CHUNK - 1)] = pc;
}

static uint8_t get(uintptr_t a, uint32_t *pc) {
    Chunk *c = chunk(a, 0);
    if (!c)
        return 0;
    if (pc)
        *pc = c->pc[a & (CHUNK - 1)];
    return c->info[a & (CHUNK - 1)];
}

typedef struct {
    uint32_t load, store;
    uint8_t lw, sw, pos;
    uint64_t count;
    uintptr_t addr;   /* the first one's */
} Pair;
#define NPAIRS 65536
static Pair pairs[NPAIRS];
static unsigned npairs;

static uint32_t rel(void *pc) { return (uint32_t)((uintptr_t)pc - (uintptr_t)__executable_start); }

static void report(uint32_t load, uint32_t store, int lw, int sw, int pos, uintptr_t addr) {
    uint32_t h = (load * 2654435761u) ^ (store * 40503u) ^ (uint32_t)(lw << 8 | sw << 4 | pos);
    for (unsigned i = 0; i < NPAIRS; i++) {
        Pair *p = &pairs[(h + i) & (NPAIRS - 1)];
        if (p->count && p->load == load && p->store == store && p->lw == lw && p->sw == sw && p->pos == pos) {
            p->count++;
            return;
        }
        if (!p->count) {
            if (npairs >= NPAIRS - 1)
                return;
            npairs++;
            p->load = load, p->store = store, p->lw = (uint8_t)lw, p->sw = (uint8_t)sw, p->pos = (uint8_t)pos;
            p->count = 1;
            p->addr = addr;
            return;
        }
    }
}

/* the last load, for the copy rule, and its report, held until the next access */
static uintptr_t last_load;
static unsigned last_n;
static int pending;
static uint32_t pend_load, pend_store;
static int pend_lw, pend_sw, pend_pos;
static uintptr_t pend_addr;

static void flush(void) {
    if (pending)
        report(pend_load, pend_store, pend_lw, pend_sw, pend_pos, pend_addr);
    pending = 0;
}

static void copy_shadow(uintptr_t d, uintptr_t s, size_t n) {
    if (d == s)
        return;
    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            uint32_t pc = 0;
            uint8_t v = get(s + i, &pc);
            if (v || chunk(d + i, 0))
                put(d + i, v, pc);
        }
    } else {
        for (size_t i = n; i-- > 0;) {
            uint32_t pc = 0;
            uint8_t v = get(s + i, &pc);
            if (v || chunk(d + i, 0))
                put(d + i, v, pc);
        }
    }
}

/*
 * The pointer slots ($PORT_PTRSLOTS=FILE; docs/PORT.md, "Where pointers
 * are 8 bytes"): every word the game's C stores a pointer into (a value in
 * the game's memory, read back after the store: the callbacks come before
 * the access), with the store's place, and every place that loads such a
 * word, written at exit.  Where pointers are 8 bytes each of those places
 * must be an access of a pointer's type, and each word a pointer's room.
 */
#define NSLOTS (1u << 20)
typedef struct {
    uintptr_t addr;
    uint32_t store;
} Slot;
static Slot *slots;
static const char *slots_out;
static uintptr_t ps_addr;
static uint32_t ps_pc;
#define NSITES (1u << 16)
typedef struct {
    uint32_t pc, n;
    uint8_t kind;   /* 1 a store, 2 a load */
} Site;
static Site sites[NSITES];

static void site(uint32_t pc, int kind) {
    for (uint32_t i = (pc * 2654435761u) & (NSITES - 1);; i = (i + 1) & (NSITES - 1)) {
        if (sites[i].pc == pc && sites[i].kind == kind) {
            sites[i].n++;
            return;
        }
        if (!sites[i].pc) {
            sites[i].pc = pc, sites[i].kind = (uint8_t)kind, sites[i].n = 1;
            return;
        }
    }
}

static Slot *slot(uintptr_t a, int make) {
    for (uint32_t i = ((uint32_t)a * 2654435761u >> 2) & (NSLOTS - 1);; i = (i + 1) & (NSLOTS - 1)) {
        if (slots[i].addr == a)
            return &slots[i];
        if (!slots[i].addr) {
            if (!make)
                return NULL;
            slots[i].addr = a;
            return &slots[i];
        }
    }
}

static inline void ps_check(void) {
    if (ps_addr) {
        uint32_t v = *(volatile uint32_t *)ps_addr;
        if (v >= 0x10000 && port_is_game_memory((void *)(uintptr_t)v)) {
            Slot *s = slot(ps_addr, 1);
            s->store = ps_pc;
            site(ps_pc, 1);
        }
        ps_addr = 0;
    }
}

static inline void ps_access(uintptr_t a, unsigned n, void *pc, int st) {
    ps_check();
    if (n != 4 || (a & 3))
        return;
    if (st) {
        ps_addr = a, ps_pc = rel(pc);
    } else if (slot(a, 0)) {
        site(rel(pc), 2);
    }
}

static inline void store(uintptr_t a, unsigned n, void *pc) {
    if (slots)
        ps_access(a, n, pc, 1);
    unsigned sn = last_n;
    last_n = 0;
    if (sn == n && n > 1) {
        pending = 0;
        copy_shadow(a, last_load, n);
        return;
    }
    flush();
    uint32_t r = rel(pc);
    if (n > 8) {
        for (unsigned i = 0; i < n; i++)
            put(a + i, W_ANY, r);
        return;
    }
    for (unsigned i = 0; i < n; i++)
        put(a + i, (uint8_t)(n | i << 4), r);
}

static inline void load(uintptr_t a, unsigned n, void *pc) {
    if (slots)
        ps_access(a, n, pc, 0);
    flush();
    last_load = a;
    last_n = n;
    if (n == 1 || n > 8)
        return;
    for (unsigned i = 0; i < n; i++) {
        uint32_t spc = 0;
        uint8_t s = get(a + i, &spc);
        unsigned w = s & 15, pos = s >> 4;
        if (w == 0 || w == W_ANY)
            continue;
        if (w == W_RAW || w != n || pos != i) {
            /* a stack's bytes keep what an earlier frame stored there: not
               reported (the stacks' locals are the host compiler's own) */
            if (host_in_thread_stack((void *)a) || port_coro_stack((void *)a))
                return;
            pending = 1;
            pend_load = rel(pc);
            pend_store = w == W_RAW ? 0 : spc;
            pend_lw = (int)n, pend_sw = (int)w, pend_pos = (int)pos;
            pend_addr = a;
            return;
        }
    }
}

void port_wc_mark(const void *p, size_t n, int width) {
    uintptr_t a = (uintptr_t)p;
    last_n = 0;
    for (size_t i = 0; i < n; i++) {
        uint8_t w = width == 0 ? W_RAW : width < 0 ? W_ANY : (uint8_t)width;
        put(a + i, (uint8_t)(w | (width > 0 ? (i % (size_t)width) << 4 : 0)), 0);
    }
}

#define LS(n)                                                                   \
    void __asan_load##n(uintptr_t a) { load(a, n, __builtin_return_address(0)); } \
    void __asan_store##n(uintptr_t a) { store(a, n, __builtin_return_address(0)); }
LS(1)
LS(2)
LS(4)
LS(8)
LS(16)
void __asan_loadN(uintptr_t a, size_t n) { load(a, (unsigned)n, __builtin_return_address(0)); }
void __asan_storeN(uintptr_t a, size_t n) { store(a, (unsigned)n, __builtin_return_address(0)); }

void *__asan_memcpy(void *d, const void *s, size_t n) {
    last_n = 0;
    copy_shadow((uintptr_t)d, (uintptr_t)s, n);
    return memmove(d, s, n);
}
void *__asan_memmove(void *d, const void *s, size_t n) { return __asan_memcpy(d, s, n); }
void *__asan_memset(void *d, int c, size_t n) {
    port_wc_mark(d, n, -1);
    return memset(d, c, n);
}

/* the game's own memset, memcpy and memmove (40300.c), byte loops: their
   stores are any width, their copies the source's shadow (the link wraps
   them: CMakeLists.txt) */
void *__real_func_80225C7C(void *d, int32_t c, int32_t n);
void *__real_func_80225C44(void *d, void *s, int32_t n);
void *__real_func_80225BB8(void *d, void *s, int32_t n);
void *__wrap_func_80225C7C(void *d, int32_t c, int32_t n) {
    void *r = __real_func_80225C7C(d, c, n);
    port_wc_mark(d, (size_t)n, -1);
    return r;
}
/* (the shadow copied after the copy, whose own byte stores it replaces;
   an overlapping memmove's source is the copy's by then) */
void *__wrap_func_80225C44(void *d, void *s, int32_t n) {
    void *r = __real_func_80225C44(d, s, n);
    copy_shadow((uintptr_t)d, (uintptr_t)s, (size_t)n);
    return r;
}
void *__wrap_func_80225BB8(void *d, void *s, int32_t n) {
    void *r = __real_func_80225BB8(d, s, n);
    copy_shadow((uintptr_t)d, (uintptr_t)s, (size_t)n);
    return r;
}

static int cmp(const void *a, const void *b) {
    const Pair *x = a, *y = b;
    return x->count < y->count ? 1 : x->count > y->count ? -1 : 0;
}

static void dump_slots(void) {
    FILE *f = fopen(slots_out, "w");
    if (!f)
        return;
    ps_check();
    fprintf(f, "# slot ADDR STORE (ADDR: rN64 in port_rdram, xOFF in the executable, h the host's);"
               " site KIND PC COUNT\n");
    for (uint32_t i = 0; i < NSLOTS; i++)
        if (slots[i].addr) {
            uintptr_t a = slots[i].addr;
            if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
                fprintf(f, "slot r%08lx %08x\n", (unsigned long)(a - (uintptr_t)port_rdram + 0x80000000u),
                        slots[i].store);
            else if (a >= (uintptr_t)__executable_start && a < (uintptr_t)_end)
                fprintf(f, "slot x%08x %08x\n", rel((void *)a), slots[i].store);
            else
                fprintf(f, "slot h%08lx %08x\n", (unsigned long)a, slots[i].store);
        }
    for (uint32_t i = 0; i < NSITES; i++)
        if (sites[i].pc)
            fprintf(f, "site %s %08x %u\n", sites[i].kind == 1 ? "store" : "load", sites[i].pc, sites[i].n);
    fclose(f);
}

static void dump(void) {
    static int done;
    if (slots && !done)
        dump_slots();
    const char *path = getenv("PORT_WIDTHCHECK_OUT");
    FILE *f = path ? fopen(path, "w") : stderr;
    unsigned n = 0;
    if (done)
        return;
    done = 1;
    flush();
    if (!f)
        f = stderr;
    for (unsigned i = 0; i < NPAIRS; i++)
        if (pairs[i].count)
            pairs[n++] = pairs[i];
    qsort(pairs, n, sizeof *pairs, cmp);
    fprintf(f, "# load store load_width store_width byte_in_store count first_address (offsets in the executable;"
               " store 0: the host's, 1: an initializer; the address an N64 one where it is in port_rdram)\n");
    for (unsigned i = 0; i < n; i++) {
        uintptr_t a = pairs[i].addr;
        if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + PORT_RDRAM_SIZE)
            a = a - (uintptr_t)port_rdram + 0x80000000u;
        fprintf(f, "%08x %08x %u %u %u %llu %08lx\n", pairs[i].load, pairs[i].store, pairs[i].lw, pairs[i].sw,
                pairs[i].pos, (unsigned long long)pairs[i].count, (unsigned long)a);
    }
    if (f != stderr)
        fclose(f);
}

static void crash(int sig) {
    dump();
    signal(sig, SIG_DFL);
    raise(sig);
}

void __asan_init(void) {
    static int once;
    if (once)
        return;
    once = 1;
    atexit(dump);
    slots_out = getenv("PORT_PTRSLOTS");
    if (slots_out)
        slots = calloc(NSLOTS, sizeof *slots);
    /* the initialized variables, as stores of their types (widthcheck.py data) */
    const char *data = getenv("PORT_WIDTHCHECK_DATA");
    if (data) {
        FILE *f = fopen(data, "r");
        char line[128];
        if (!f) {
            fprintf(stderr, "widthcheck: can't read %s\n", data);
            abort();
        }
        while (fgets(line, sizeof line, f)) {
            unsigned long off, n, w;
            if (line[0] == '#' || sscanf(line, "%lx %lu %lu", &off, &n, &w) != 3)
                continue;
            uintptr_t a = (uintptr_t)__executable_start + off;
            for (unsigned long i = 0; i < n * w; i++)
                put(a + i, (uint8_t)(w | (i % w) << 4), 1);
        }
        fclose(f);
    }
    signal(SIGSEGV, crash);
    signal(SIGABRT, crash);
    signal(SIGBUS, crash);
    signal(SIGTERM, crash);
    signal(SIGINT, crash);
}
void __asan_version_mismatch_check_v8(void) {}
void __asan_handle_no_return(void) {}
#endif
