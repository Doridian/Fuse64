/*
 * The scattered layout's check (-DPORT_SCATTER=SEED -DPORT_SCATTER_CHECK=ON;
 * docs/PORT.md, "The scattered layout"): every access that lands where no
 * variable is.
 *
 * Everything but this file is compiled with AddressSanitizer's
 * instrumentation and not its runtime (as the width check, widthcheck.c):
 * each load and store is a call to the functions below, and each global
 * variable has a redzone after it, which the compiler's constructors
 * register (__asan_register_elf_globals).  A byte map over the executable's
 * image says which bytes are no variable's: the redzones, the padding
 * between the scattered objects (gen/scatter.c, port_scatter_bad) and all of
 * port_rdram (the N64's RDRAM, which nothing should reach in this layout).
 * An access to such a byte is reported once per place (and object reached):
 *
 *   scatter: R 4 at +0x1234 (past D_80090050 [size 0x10] by 4; before D_x by 12) pc +0x5678 +0x9abc ...
 *
 * (the pcs are offsets in the executable, the access's first, then its
 * callers'), to $PORT_SCATTER_OUT or stderr; port/tools/scatter_list.py
 * makes the reports of several runs into the list of what depends on the
 * N64's layout.  The access itself happens as it would (reading the
 * poison, PORT_SCATTER_POISON, writing over it).
 */
#ifdef PORT_SCATTER_CHECK
#include <execinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char __executable_start[], _end[];
extern uint8_t port_rdram[], port_scatter_arena[];

/* gen/scatter.c's objects in the arena, by offset */
typedef struct {
    const char *name;
    uint32_t off, size, n64, n64size;
} PortScatterObj;
extern const PortScatterObj port_scatter_objs[];
extern const unsigned port_scatter_nobjs;

static uint8_t *bad;    /* a bit per byte of the image: no variable's */
static uintptr_t lo, hi;
static FILE *out;

typedef struct {
    uintptr_t beg, size, size_rz;
    const char *name, *module;
    uintptr_t dyn, loc, odr;
} AsanGlobal;
static AsanGlobal **globals;   /* sorted by beg */
static size_t nglobals, capglobals;

static void setup(void) {
    if (bad)
        return;
    lo = (uintptr_t)__executable_start;
    hi = (uintptr_t)_end;
    bad = calloc((hi - lo + 7) / 8, 1);
    if (!bad) {
        fputs("scatter: no memory for the map\n", stderr);
        abort();
    }
    const char *o = getenv("PORT_SCATTER_OUT");
    out = o ? fopen(o, "w") : stderr;
    if (!out)
        out = stderr;
}

static void mark(uintptr_t a, size_t n, int v) {
    setup();
    for (size_t i = 0; i < n; i++, a++) {
        if (a < lo || a >= hi)
            continue;
        if (v)
            bad[(a - lo) >> 3] |= (uint8_t)(1u << ((a - lo) & 7));
        else
            bad[(a - lo) >> 3] &= (uint8_t)~(1u << ((a - lo) & 7));
    }
}

void port_scatter_bad(void *p, unsigned long n) { mark((uintptr_t)p, n, 1); }

static int gcmp(const void *a, const void *b) {
    uintptr_t x = (*(AsanGlobal *const *)a)->beg, y = (*(AsanGlobal *const *)b)->beg;
    return x < y ? -1 : x > y;
}

static void add_globals(AsanGlobal *g, size_t n) {
    setup();
    if (nglobals + n > capglobals) {
        capglobals = (nglobals + n) * 2;
        globals = realloc(globals, capglobals * sizeof *globals);
    }
    for (size_t i = 0; i < n; i++) {
        globals[nglobals++] = &g[i];
        mark(g[i].beg + g[i].size, g[i].size_rz - g[i].size, 1);
    }
    qsort(globals, nglobals, sizeof *globals, gcmp);
}

void __asan_register_globals(AsanGlobal *g, uintptr_t n) { add_globals(g, n); }
void __asan_unregister_globals(AsanGlobal *g, uintptr_t n) { (void)g, (void)n; }
void __asan_register_elf_globals(uintptr_t *flag, void *start, void *stop) {
    if (*flag)
        return;
    *flag = 1;
    add_globals(start, (size_t)((AsanGlobal *)stop - (AsanGlobal *)start));
}
void __asan_unregister_elf_globals(uintptr_t *flag, void *start, void *stop) { (void)flag, (void)start, (void)stop; }
void __asan_init(void) { setup(); }
void __asan_version_mismatch_check_v8(void) {}
void __asan_handle_no_return(void) {}
void __asan_before_dynamic_init(const char *module) { (void)module; }
void __asan_after_dynamic_init(void) {}
/* (fiber.c tells AddressSanitizer of its stack switches; nothing here needs it) */
void __sanitizer_start_switch_fiber(void **save, const void *bottom, size_t size) { (void)save, (void)bottom, (void)size; }
void __sanitizer_finish_switch_fiber(void *save, const void **bottom, size_t *size) { (void)save, (void)bottom, (void)size; }

/* the places reported, and what they reached */
#define NSEEN (1 << 18)
static struct {
    uintptr_t pc;
    long what;
} seen[NSEEN];

/* what's around a: "past X [size] by d; before Y by e" */
static long describe(uintptr_t a, char *buf, size_t len) {
    if (a >= (uintptr_t)port_rdram && a < (uintptr_t)port_rdram + 0x800000) {
        snprintf(buf, len, "rdram 0x%08lX", (unsigned long)(a - (uintptr_t)port_rdram + 0x80000000u));
        return -1 - (long)((a - (uintptr_t)port_rdram) >> 12);
    }
    uintptr_t ar = (uintptr_t)port_scatter_arena;
    if (a >= ar && a < ar + (uintptr_t)port_scatter_objs[port_scatter_nobjs].off) {
        uint32_t off = (uint32_t)(a - ar);
        unsigned l = 0, h = port_scatter_nobjs;
        while (l < h) {     /* the first object starting after off */
            unsigned m = (l + h) / 2;
            if (port_scatter_objs[m].off <= off)
                l = m + 1;
            else
                h = m;
        }
        const PortScatterObj *p = l ? &port_scatter_objs[l - 1] : NULL, *n = l < port_scatter_nobjs ? &port_scatter_objs[l] : NULL;
        int k = 0;
        k += snprintf(buf + k, len - k, "arena");
        if (p)
            k += snprintf(buf + k, len - k, " past %s [0x%X] by %u", p->name, p->size, off - p->off - p->size);
        if (n)
            snprintf(buf + k, len - k, "; before %s by %u", n->name, n->off - off);
        /* (the nearer one) */
        return (p && (!n || off - p->off - p->size <= n->off - off)) ? (long)(l - 1) : (long)l;
    }
    size_t l = 0, h = nglobals;
    while (l < h) {
        size_t m = (l + h) / 2;
        if (globals[m]->beg <= a)
            l = m + 1;
        else
            h = m;
    }
    AsanGlobal *p = l ? globals[l - 1] : NULL, *n = l < nglobals ? globals[l] : NULL;
    int k = snprintf(buf, len, "global");
    if (p)
        k += snprintf(buf + k, len - k, " past %s [0x%lX] by %lu (%s)", p->name, (unsigned long)p->size,
                      (unsigned long)(a - p->beg - p->size), p->module);
    if (n)
        snprintf(buf + k, len - k, "; before %s by %lu (%s)", n->name, (unsigned long)(n->beg - a), n->module);
    return 100000 + (long)(p ? l - 1 : l);
}

static void report(uintptr_t a, size_t n, int st, void *pc) {
    char buf[512];
    uintptr_t b = a;
    /* (the first bad byte) */
    while (b < a + n && !(bad[(b - lo) >> 3] & (1u << ((b - lo) & 7))))
        b++;
    long what = describe(b, buf, sizeof buf);
    uintptr_t key = (uintptr_t)pc * 31 + (uintptr_t)what;
    for (unsigned i = 0;; i++) {
        unsigned s = (unsigned)((key * 2654435761u) + i) & (NSEEN - 1);
        if (seen[s].pc == (uintptr_t)pc && seen[s].what == what)
            return;
        if (!seen[s].pc) {
            seen[s].pc = (uintptr_t)pc;
            seen[s].what = what;
            break;
        }
        if (i > NSEEN)
            return;
    }
    void *bt[8];
    int nb = backtrace(bt, 8);
    fprintf(out, "scatter: %c %lu at +0x%lX (%s) pc +0x%lX", st ? 'W' : 'R', (unsigned long)n, (unsigned long)(b - lo),
            buf, (unsigned long)((uintptr_t)pc - lo));
    /* (the callers: backtrace's first two are report and the hook) */
    for (int i = 3; i < nb; i++)
        fprintf(out, " +0x%lX", (unsigned long)((uintptr_t)bt[i] - lo));
    fputc('\n', out);
    fflush(out);
}

static inline void check(uintptr_t a, size_t n, int st, void *pc) {
    if (a - lo >= hi - lo || !bad)
        return;
    uintptr_t o = a - lo;
    if (n == 0)
        return;
    if (o + n > hi - lo)
        n = hi - lo - o;
    /* (most accesses: one or two map bytes) */
    uintptr_t e = o + n - 1;
    uint8_t m;
    if ((o >> 3) == (e >> 3))
        m = bad[o >> 3] & (uint8_t)(((1u << (e - o + 1)) - 1) << (o & 7));
    else {
        m = 0;
        for (uintptr_t i = o; i <= e && !m; i++)
            m = bad[i >> 3] & (uint8_t)(1u << (i & 7));
    }
    if (m)
        report(a, n, st, pc);
}

#define LS(n)                                                                                \
    void __asan_load##n(uintptr_t a) { check(a, n, 0, __builtin_return_address(0)); }          \
    void __asan_store##n(uintptr_t a) { check(a, n, 1, __builtin_return_address(0)); }         \
    void __asan_load##n##_noabort(uintptr_t a) { check(a, n, 0, __builtin_return_address(0)); }  \
    void __asan_store##n##_noabort(uintptr_t a) { check(a, n, 1, __builtin_return_address(0)); }
LS(1)
LS(2)
LS(4)
LS(8)
LS(16)
void __asan_loadN(uintptr_t a, size_t n) { check(a, n, 0, __builtin_return_address(0)); }
void __asan_storeN(uintptr_t a, size_t n) { check(a, n, 1, __builtin_return_address(0)); }
void __asan_loadN_noabort(uintptr_t a, size_t n) { check(a, n, 0, __builtin_return_address(0)); }
void __asan_storeN_noabort(uintptr_t a, size_t n) { check(a, n, 1, __builtin_return_address(0)); }

void *__asan_memcpy(void *d, const void *s, size_t n) {
    check((uintptr_t)s, n, 0, __builtin_return_address(0));
    check((uintptr_t)d, n, 1, __builtin_return_address(0));
    return memmove(d, s, n);
}
void *__asan_memmove(void *d, const void *s, size_t n) {
    check((uintptr_t)s, n, 0, __builtin_return_address(0));
    check((uintptr_t)d, n, 1, __builtin_return_address(0));
    return memmove(d, s, n);
}
void *__asan_memset(void *d, int c, size_t n) {
    check((uintptr_t)d, n, 1, __builtin_return_address(0));
    return memset(d, c, n);
}
#endif
