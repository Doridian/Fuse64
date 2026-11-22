/*
 * The libaudio oracle's replay (docs/PORT.md, "Audio"; DozerEncore's
 * port/tools/audio_oracle, for this game).
 *
 *   replay LIB.so LOG [options]
 *
 * Replays a log the port recorded (port/src/audio_record.c, PORT_AUDIO_LOG)
 * into a libaudio built as LIB.so (the decompiled original or the port's
 * own, port/tools/audio_oracle/Makefile), and checks every answer it gives
 * against the recording: each call's result and the structures it hands
 * back, each audio frame's command list (by hash), and the order and
 * arguments of its calls back into the game (the DMA routine, the
 * sequence player's oscillators).  The game's side is the log: its
 * memory writes, its arguments, what its callbacks returned.
 *
 * Built for i386 (the N64's 32-bit pointers and structure layouts).  The
 * log's addresses are the recorder's (its image at 0x10000000, its heap
 * after it): the replay maps the pages the log names where they were, as
 * the log names them, filled with --poison's byte (0 by default), so that
 * a library that read memory the recorder didn't share reads something
 * else and differs (or faults: the fault's address is printed).
 *
 * Options:
 *   -k             keep going after a mismatch (default: stop at the first)
 *   -q             quiet: only mismatches and the summary
 *   --poison B     the byte the pages are mapped with
 *   --hashes FILE  after every call the game makes, the call's number and a
 *                  hash of the shared memory the game can see (the library's
 *                  own heap blocks and the bank files left out), for
 *                  comparing two libraries'
 *   --dump-acmd N FILE   write audio frame N's command list
 *   --dump-mem N FILE    write the shared memory after call N (addr, len, bytes)
 *   --stop N       stop after call N
 *   --translations FILE  every bank-object address translated (below)
 *
 * A library that builds its own bank objects instead of relocating the
 * file's in place exports its map from the file's objects to its own
 * (audio_log.h, ALOG_BANK_MAP).  After each alBnkfNew the replay takes it:
 * a word of the log that is a mapped object's file address (a call's
 * argument, a 4-aligned word the game writes, outside the bank files) is
 * given to the library as its object, and a word the library gives back
 * (a structure, a result, a callback's argument) that is one of its
 * objects is compared as the file's.  The bank files' own bytes are the
 * library's business after alBnkfNew (relocated or not), and are left out
 * of --hashes and masked in --dump-mem.  Without the symbol nothing is
 * translated.
 */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <dlfcn.h>
#include <link.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/personality.h>
#include <unistd.h>

#define ALOG_NO_HOST
#include "../../include/audio_log.h"

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef float f32;

static const struct {
    const char *name, *ret, *params;
} fns[] = {
    {"-", "v", ""},
#define FN(f, r, a) {#f, r, a},
    ALOG_FUNCS(FN)
#undef FN
};

/* ---- the log --------------------------------------------------------------- */

/* a window on the log, which can be larger than this 32-bit program's memory */
#define WINDOW (16u << 20)      /* words */
static u32 *logw;
static size_t nlogw, pos;
static FILE *logf;
static int log_eof;

/* at least n words from pos in the window, if the log has them */
static void ensure(size_t n) {
    size_t got;
    if (pos + n <= nlogw || log_eof)
        return;
    if (n > WINDOW) {
        fprintf(stderr, "replay: a record larger than the window\n");
        exit(2);
    }
    memmove(logw, logw + pos, (nlogw - pos) * 4);
    nlogw -= pos;
    pos = 0;
    got = fread(logw + nlogw, 4, WINDOW - nlogw, logf);
    nlogw += got;
    if (got == 0)
        log_eof = 1;
    ensure(n);
}

static u32 next(void) {
    ensure(1);
    if (pos >= nlogw) {
        fprintf(stderr, "replay: the log ends early\n");
        exit(2);
    }
    return logw[pos++];
}

static u32 peek(void) {
    ensure(1);
    return pos < nlogw ? logw[pos] : ALOG_END;
}

static const u8 *take_bytes(u32 len) {
    const u8 *p;
    ensure((len + 3) / 4);
    p = (const u8 *)&logw[pos];
    pos += (len + 3) / 4;
    if (pos > nlogw) {
        fprintf(stderr, "replay: the log ends early\n");
        exit(2);
    }
    return p;
}

/* ---- the recorder's memory, mapped where it was ------------------------------ */

static u8 mapped[1u << 17];     /* a bit a 4 KiB page */
static int poison;

static void map(u32 addr, u32 len) {
    u32 p, last;
    if (len == 0)
        return;
    last = (addr + len - 1) >> 12;
    for (p = addr >> 12; p <= last; p++) {
        void *want = (void *)(uintptr_t)(p << 12);
        if (mapped[p >> 3] & (1 << (p & 7)))
            continue;
        if (mmap(want, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) !=
            want) {
            fprintf(stderr, "replay: can't map the recorder's page %08X (the replay's own memory?)\n", p << 12);
            exit(2);
        }
        if (poison)
            memset(want, poison, 0x1000);
        mapped[p >> 3] |= 1 << (p & 7);
    }
}

static void *mem(u32 addr) {
    return (void *)(uintptr_t)addr;
}

/* ---- options and results -------------------------------------------------- */

static int keep_going, quiet;
static FILE *hashes;
static long dump_acmd_frame = -1, dump_mem_call = -1, stop_call = -1;
static const char *dump_acmd_path, *dump_mem_path;

static long ncalls, nframes, nmismatch, ncallbacks;
static u32 cur_fn;          /* the call in progress, for messages */
static long cur_call;

static void mismatch(const char *fmt, ...) {
    va_list ap;
    nmismatch++;
    fprintf(stderr, "MISMATCH at call %ld (%s), frame %ld: ", cur_call, fns[cur_fn].name, nframes);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    if (!keep_going) {
        fprintf(stderr, "replay: stopped (%ld calls, %ld frames)\n", ncalls, nframes);
        exit(1);
    }
}

static void desync(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "DIVERGED at call %ld (%s), frame %ld: ", cur_call, fns[cur_fn].name, nframes);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

static void on_fault(int sig, siginfo_t *si, void *uc) {
    (void)uc;
    fprintf(stderr, "replay: signal %d at %p in call %ld (%s), frame %ld (memory the recorder didn't share?)\n",
            sig, si->si_addr, cur_call, fns[cur_fn].name, nframes);
    _exit(3);
}

/* ---- the shared memory ------------------------------------------------------ */

typedef struct { u32 addr, len; } Span;

#define MAX_SPANS 4096
static Span tracked[MAX_SPANS];
static int ntracked;
static Span internal[256];      /* the library's own blocks of the heap */
static int ninternal;
static u32 heap_addr;           /* the ALHeap alHeapInit was given */
static u32 mono_addr;           /* ALOG_VAR_MONO */

/* the game's byte the library reads by name (synstartvoiceparam.c), from
   where the recorder had it, before each call */
__attribute__((visibility("default"))) u8 D_8001C4B0;

static void track(u32 addr, u32 len) {
    map(addr, len);
    if (ntracked == MAX_SPANS) {
        fprintf(stderr, "replay: too many tracked ranges\n");
        exit(2);
    }
    tracked[ntracked].addr = addr;
    tracked[ntracked].len = len;
    ntracked++;
}

/* [addr, addr+len) no longer shared: out of the ranges, splitting one */
static void untrack(u32 addr, u32 len) {
    u32 end = addr + len;
    int i;
    for (i = 0; i < ntracked; i++) {
        Span *r = &tracked[i];
        u32 rend = r->addr + r->len;
        if (addr >= rend || end <= r->addr)
            continue;
        if (addr <= r->addr && end >= rend) {
            r->len = 0;
        } else if (addr <= r->addr) {
            r->len = rend - end;
            r->addr = end;
        } else if (end >= rend) {
            r->len = addr - r->addr;
        } else {
            if (ntracked == MAX_SPANS) {
                fprintf(stderr, "replay: too many tracked ranges\n");
                exit(2);
            }
            tracked[ntracked].addr = end;
            tracked[ntracked].len = rend - end;
            ntracked++;
            tracked[i].len = addr - tracked[i].addr;
        }
    }
    for (i = 0; i < ntracked;)
        if (tracked[i].len == 0)
            tracked[i] = tracked[--ntracked];
        else
            i++;
}

static int masked(u32 a) {
    int i;
    for (i = 0; i < ninternal; i++)
        if (a - internal[i].addr < internal[i].len)
            return 1;
    return 0;
}

/* ---- the bank objects' map (audio_log.h, ALOG_BANK_MAP) ------------------- */

typedef u32 (*BankMapFn)(const AlogBankObj **objs);
static BankMapFn bank_map_fn;
static AlogBankObj *by_file, *by_native;
static u32 nmap, nfile;
static Span bankfiles[64];      /* the bank files alBnkfNew had */
static int nbankfiles;
static Span allocs[1024];       /* the game's last heap blocks, to size them */
static int nallocs;
static long nxlate_arg, nxlate_mem, nxlate_back;
static FILE *xlog;

static int cmp_file(const void *a, const void *b) {
    u32 x = ((const AlogBankObj *)a)->file, y = ((const AlogBankObj *)b)->file;
    return x < y ? -1 : x > y;
}

static int cmp_native(const void *a, const void *b) {
    u32 x = ((const AlogBankObj *)a)->native, y = ((const AlogBankObj *)b)->native;
    return x < y ? -1 : x > y;
}

/* the library's object for a file address, or the word as it is */
static u32 to_native(u32 w) {
    AlogBankObj key, *o;
    if (nmap == 0)
        return w;
    key.file = w;
    o = bsearch(&key, by_file, nfile, sizeof(*o), cmp_file);
    return o ? o->native : w;
}

/* the file's object for one of the library's, or the word as it is */
static u32 to_file(u32 w) {
    AlogBankObj key, *o;
    if (nmap == 0)
        return w;
    key.native = w;
    o = bsearch(&key, by_native, nmap, sizeof(*o), cmp_native);
    return o ? o->file : w;
}

static int in_bankfile(u32 a) {
    int i;
    for (i = 0; i < nbankfiles; i++)
        if (a - bankfiles[i].addr < bankfiles[i].len)
            return 1;
    return 0;
}

static void note_alloc(u32 addr, u32 len) {
    allocs[nallocs % 1024].addr = addr;
    allocs[nallocs % 1024].len = len;
    nallocs++;
}

/* the length of the game's heap block at addr, 0 if none */
static u32 alloc_len(u32 addr) {
    int i, n = nallocs < 1024 ? nallocs : 1024;
    u32 len = 0;
    for (i = 0; i < n; i++)
        if (allocs[i].addr == addr)
            len = allocs[i].len;
    return len;
}

/* after alBnkfNew(file): the file's extent (the game's heap block it is
   at the start of) and the library's map */
static void bank_file_new(u32 file) {
    const AlogBankObj *objs;
    u32 len = alloc_len(file), k;
    int i;

    for (i = 0; i < nbankfiles; i++)
        if (bankfiles[i].addr == file)
            break;
    if (len == 0)
        fprintf(stderr, "replay: the bank file at %08X isn't a block the game allocated: its bytes stay "
                        "in the hashes\n", file);
    else if (i < nbankfiles)
        bankfiles[i].len = len;
    else if (nbankfiles < 64) {
        bankfiles[nbankfiles].addr = file;
        bankfiles[nbankfiles].len = len;
        nbankfiles++;
    }
    if (bank_map_fn == NULL)
        return;
    nmap = bank_map_fn(&objs);
    by_file = realloc(by_file, (nmap ? nmap : 1) * sizeof(*by_file));
    by_native = realloc(by_native, (nmap ? nmap : 1) * sizeof(*by_native));
    memcpy(by_native, objs, nmap * sizeof(*objs));
    qsort(by_native, nmap, sizeof(*by_native), cmp_native);
    for (k = 1; k < nmap; k++)
        if (by_native[k].native == by_native[k - 1].native) {
            fprintf(stderr, "replay: the library's bank map maps an object twice (%08X)\n", by_native[k].native);
            exit(2);
        }
    /* a file loaded again where another was: its newest object for an
       address of the file (the map is in the order they were made) */
    {
        u32 n = 0;
        int j;
        for (j = (int)nmap - 1; j >= 0; j--) {
            AlogBankObj key = objs[j];
            u32 m;
            for (m = 0; m < n; m++)
                if (by_file[m].file == key.file)
                    break;
            if (m == n)
                by_file[n++] = key;
        }
        nfile = n;
    }
    qsort(by_file, nfile, sizeof(*by_file), cmp_file);
    if (!quiet)
        fprintf(stderr, "replay: the library's bank map: %u objects\n", nmap);
}

/* the game's bytes at addr: its words that are file objects become the
   library's */
/* where the library is loaded: its functions' addresses in the shared
   memory (a player's handler, a filter's) are its own */
static u32 lib_lo, lib_hi;

/* the words the game wrote that happen to fall in the library's addresses
   (the recorder's own pointers, into its libc): as_file leaves them be */
#define MAX_GAMEWORDS 1024
static struct { u32 addr, value; } gamewords[MAX_GAMEWORDS];
static int ngamewords;

static void game_word(u32 a, u32 w) {
    int i;
    for (i = 0; i < ngamewords; i++)
        if (gamewords[i].addr == a)
            break;
    if (w - lib_lo >= lib_hi - lib_lo) {
        if (i < ngamewords)
            gamewords[i] = gamewords[--ngamewords];
        return;
    }
    if (i == ngamewords) {
        if (ngamewords == MAX_GAMEWORDS)
            return;
        ngamewords++;
    }
    gamewords[i].addr = a;
    gamewords[i].value = w;
}

static int is_game_word(u32 a, u32 w) {
    int i;
    for (i = 0; i < ngamewords; i++)
        if (gamewords[i].addr == a)
            return gamewords[i].value == w;
    return 0;
}

static void game_writes(u32 addr, u32 len) {
    u32 a, e = addr + len;
    for (a = (addr + 3) & ~3u; a + 4 <= e && a + 4 > a; a += 4)
        game_word(a, *(u32 *)mem(a));
    if (nmap == 0)
        return;
    for (a = (addr + 3) & ~3u; a + 4 <= e && a + 4 > a; a += 4) {
        u32 w = *(u32 *)mem(a), n;
        if (in_bankfile(a) || (n = to_native(w)) == w)
            continue;
        *(u32 *)mem(a) = n;
        nxlate_mem++;
        if (xlog)
            fprintf(xlog, "call %ld: memory %08X: %08X -> %08X\n", ncalls, a, w, n);
    }
}

static int find_lib(struct dl_phdr_info *info, size_t size, void *data) {
    int i;
    (void)size;
    if (strcmp(info->dlpi_name, (const char *)data) != 0)
        return 0;
    for (i = 0; i < info->dlpi_phnum; i++)
        if (info->dlpi_phdr[i].p_type == PT_LOAD) {
            u32 lo = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr, hi = lo + info->dlpi_phdr[i].p_memsz;
            if (lib_lo == 0 || lo < lib_lo)
                lib_lo = lo;
            if (hi > lib_hi)
                lib_hi = hi;
        }
    return 1;
}

/* the memory at addr as the original's objects would have it: the
   library's objects' addresses in 4-aligned words as the file's, and its
   own addresses (functions, its variables) as 0x4C1B0000 */
static const u8 *as_file(u32 addr, u32 len) {
    static u8 *buf;
    static u32 cap;
    u32 a, e = addr + len;
    if (len > cap) {
        cap = len;
        buf = realloc(buf, cap);
    }
    memcpy(buf, mem(addr), len);
    for (a = (addr + 3) & ~3u; a + 4 <= e && a + 4 > a; a += 4) {
        u32 w, f;
        memcpy(&w, buf + (a - addr), 4);
        f = to_file(w);
        if (f == w && w - lib_lo < lib_hi - lib_lo && !is_game_word(a, w))
            f = 0x4C1B0000;
        if (f != w)
            memcpy(buf + (a - addr), &f, 4);
    }
    return buf;
}

static uint64_t mem_hash(void) {
    uint64_t h = 0xcbf29ce484222325ull;
    int i;
    for (i = 0; i < ntracked; i++) {
        u32 a = tracked[i].addr, e = a + tracked[i].len;
        const u8 *v = as_file(a, tracked[i].len);
        for (; a < e; a++) {
            if (masked(a) || in_bankfile(a))
                continue;
            h = (h ^ v[a - tracked[i].addr]) * 0x100000001b3ull;
        }
    }
    return h;
}

static void dump_mem(const char *path) {
    FILE *f = fopen(path, "wb");
    int i;
    for (i = 0; i < ntracked; i++) {
        fwrite(&tracked[i].addr, 4, 1, f);
        fwrite(&tracked[i].len, 4, 1, f);
        fwrite(as_file(tracked[i].addr, tracked[i].len), 1, tracked[i].len, f);
    }
    /* then the masks: the bank files and the internal blocks */
    for (i = 0; i < nbankfiles; i++) {
        u32 z = 0xFFFFFFFFu;
        fwrite(&z, 4, 1, f);
        fwrite(&bankfiles[i].addr, 4, 1, f);
        fwrite(&bankfiles[i].len, 4, 1, f);
    }
    for (i = 0; i < ninternal; i++) {
        u32 z = 0xFFFFFFFFu;
        fwrite(&z, 4, 1, f);
        fwrite(&internal[i].addr, 4, 1, f);
        fwrite(&internal[i].len, 4, 1, f);
    }
    fclose(f);
}

/* ---- the library -------------------------------------------------------------- */

static void *lib;
static char *lib_path;
static void *fnp[ALOG_FN_COUNT];

/* what the library needs from the rest of libultra and the port */
__attribute__((visibility("default"))) u32 osSetIntMask(u32 m) {
    static u32 cur = 0x003FFF01;
    u32 old = cur;
    cur = m;
    return old;
}

/* the identity, as the port's (port/src/ultra.c) */
__attribute__((visibility("default"))) u32 osVirtualToPhysical(void *p) {
    return (u32)(uintptr_t)p;
}

/* the port's stop with a message: the library's on a full heap, a bad bank */
__attribute__((visibility("default"))) void host_fatal(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "replay: the library stopped at call %ld (%s): ", cur_call, fns[cur_fn].name);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(1);
}

/* ---- callbacks: the game's side, from the log ---------------------------- */

static void run_call(void);

/* the library calls back: check it is the call the original made, then
   play the game's part until it returns */
static u32 callback(u32 kind, u32 a0, u32 a1, u32 a2, u32 *extra /* [2] */) {
    u32 tag = peek();
    ncallbacks++;
    if (tag != ALOG_CBENTER) {
        desync("the library calls back (kind %u: %08X %08X %08X) where the original %s",
               kind, a0, a1, a2,
               tag == ALOG_RET ? "returned" : tag == ALOG_OUT || tag == ALOG_ACMD ? "had returned"
                                                                                : "did something else");
    }
    next();
    {
        u32 k = next(), b0 = next(), b1 = next(), b2 = next();
        if (k != kind || b0 != to_file(a0) || b1 != to_file(a1) || b2 != to_file(a2))
            desync("callback kind %u (%08X %08X %08X), the original's was kind %u (%08X %08X %08X)",
                   kind, a0, a1, a2, k, b0, b1, b2);
    }
    for (;;) {
        u32 t = next();
        if (t == ALOG_MEM) {
            u32 addr = next(), len = next();
            map(addr, len);
            memcpy(mem(addr), take_bytes(len), len);
            game_writes(addr, len);
        } else if (t == ALOG_TRACK) {
            u32 addr = next(), len = next();
            track(addr, len);
        } else if (t == ALOG_UNTRACK) {
            u32 addr = next(), len = next();
            untrack(addr, len);
        } else if (t == ALOG_CALL) {
            pos--;
            run_call();
        } else if (t == ALOG_CBRET) {
            u32 v = next(), x0 = next(), x1 = next();
            if (extra) {
                extra[0] = x0;
                extra[1] = x1;
            }
            return v;
        } else {
            desync("unexpected record %u in a callback", t);
        }
    }
}

static s32 stub_dma(s32 addr, s32 len, void *state) {
    return (s32)callback(ALOG_CB_DMA, (u32)addr, (u32)len, (u32)(uintptr_t)state, NULL);
}

static void *stub_dmanew(void *state) {
    u32 x[2];
    callback(ALOG_CB_DMANEW, (u32)(uintptr_t)state, 0, 0, x);
    *(u32 *)state = x[0];
    return (void *)stub_dma;
}

/* the sequence player's oscillators: what the game gave back goes where
   the library asked for it (the oscillator's state is the game's, and
   only handed back to it) */
static s32 stub_initosc(void **state, f32 *value, u8 type, u8 rate, u8 depth, u8 delay) {
    u32 x[2];
    s32 r = (s32)callback(ALOG_CB_INITOSC, type | rate << 8 | depth << 16 | (u32)delay << 24, 0, 0, x);
    *state = (void *)(uintptr_t)x[0];
    memcpy(value, &x[1], 4);
    return r;
}

static s32 stub_updateosc(void *state, f32 *value) {
    u32 x[2];
    s32 r = (s32)callback(ALOG_CB_UPDATEOSC, (u32)(uintptr_t)state, 0, 0, x);
    memcpy(value, &x[0], 4);
    return r;
}

static void stub_stoposc(void *state) {
    callback(ALOG_CB_STOPOSC, (u32)(uintptr_t)state, 0, 0, NULL);
}

/* ---- calls --------------------------------------------------------------------- */

static uint64_t fnv(const u8 *p, u32 n) {
    uint64_t h = 0xcbf29ce484222325ull;
    u32 i;
    for (i = 0; i < n; i++)
        h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}

/* a result as its type has it */
static u32 narrow(char kind, u32 v) {
    switch (kind) {
    case 'h':
        return (u32)(s32)(s16)v;
    case 'b':
        return v & 0xFF;
    case 'v':
        return 0;
    default:
        return v;
    }
}

typedef u32 (*IntFn)(u32, u32, u32, u32, u32, u32);
typedef f32 (*FloatFn)(u32, u32, u32, u32, u32, u32);

static void run_call(void) {
    u32 a[6], fn, ret = 0;
    int i;
    long my_call;
    u32 saved_fn = cur_fn;
    long saved_call = cur_call;
    u32 heap_before = 0;
    const char *params;

    next();         /* CALL */
    fn = next();
    for (i = 0; i < 6; i++)
        a[i] = next();
    if (fn == 0 || fn >= ALOG_FN_COUNT)
        desync("an unknown function %u", fn);
    my_call = ++ncalls;
    cur_fn = fn;
    cur_call = my_call;
    params = fns[fn].params;
    for (i = 0; params[i]; i++)
        if (params[i] == 'p') {
            u32 n;
            if (a[i] != 0)
                map(a[i], 1);
            if (fn == ALOG_alBnkfNew || fn == ALOG_alHeapInit || fn == ALOG_alHeapDBAlloc)
                continue;
            n = to_native(a[i]);
            if (n != a[i]) {
                nxlate_arg++;
                if (xlog)
                    fprintf(xlog, "call %ld %s: argument %d: %08X -> %08X\n", my_call, fns[fn].name, i, a[i], n);
                a[i] = n;
            }
        }

    while (peek() == ALOG_IN) {
        u32 addr, len;
        next();
        addr = next();
        len = next();
        map(addr, len);
        memcpy(mem(addr), take_bytes(len), len);
        game_writes(addr, len);
    }
    if (heap_addr != 0)
        heap_before = *(u32 *)mem(heap_addr + 4);
    if (mono_addr != 0)
        D_8001C4B0 = *(u8 *)mem(mono_addr);

    switch (fn) {
    case ALOG_alHeapInit:
        heap_addr = a[0];
        nbankfiles = 0;     /* (a new heap: nothing of the old is a bank file) */
        break;
    case ALOG_alInit:
        /* the config's DMA routine is the game's: ours stands in */
        *(u32 *)mem(a[1] + 0x10) = (u32)(uintptr_t)stub_dmanew;
        break;
    case ALOG_alCSPNew:
        /* and so are the config's oscillators */
        if (*(u32 *)mem(a[1] + 0x10) != 0)
            *(u32 *)mem(a[1] + 0x10) = (u32)(uintptr_t)stub_initosc;
        if (*(u32 *)mem(a[1] + 0x14) != 0)
            *(u32 *)mem(a[1] + 0x14) = (u32)(uintptr_t)stub_updateosc;
        if (*(u32 *)mem(a[1] + 0x18) != 0)
            *(u32 *)mem(a[1] + 0x18) = (u32)(uintptr_t)stub_stoposc;
        break;
    }
    if (fns[fn].ret[0] == 'f')
        ret = ({ f32 f = ((FloatFn)fnp[fn])(a[0], a[1], a[2], a[3], a[4], a[5]); u32 u; memcpy(&u, &f, 4); u; });
    else
        ret = narrow(fns[fn].ret[0], ((IntFn)fnp[fn])(a[0], a[1], a[2], a[3], a[4], a[5]));
    switch (fn) {
    case ALOG_alHeapDBAlloc:
        note_alloc(ret, a[3] * a[4]);
        break;
    case ALOG_alBnkfNew:
        bank_file_new(a[0]);
        break;
    }

    /* the library's own blocks of the heap */
    if (heap_addr != 0 && fn != ALOG_alHeapDBAlloc && fn != ALOG_alHeapInit) {
        u32 heap_after = *(u32 *)mem(heap_addr + 4);
        if (heap_after > heap_before && ninternal < 256) {
            internal[ninternal].addr = heap_before;
            internal[ninternal].len = heap_after - heap_before;
            ninternal++;
        }
    }

    for (;;) {
        u32 t = peek();
        if (t == ALOG_UNTRACK || t == ALOG_TRACK) {
            u32 addr, len;
            next();
            addr = next();
            len = next();
            if (t == ALOG_TRACK)
                track(addr, len);
            else
                untrack(addr, len);
        } else if (t == ALOG_OUT) {
            u32 addr, len;
            const u8 *want;
            next();
            addr = next();
            len = next();
            want = take_bytes(len);
            if (memcmp(mem(addr), want, len) != 0) {
                const u8 *got = as_file(addr, len);
                if (memcmp(got, want, len) == 0)
                    nxlate_back++;
                else {
                    char buf[1024];
                    int o = 0;
                    u32 k;
                    for (k = 0; k < len && o < 900; k++)
                        if (got[k] != want[k])
                            o += sprintf(buf + o, " +%X:%02X/%02X", k, got[k], want[k]);
                    mismatch("the structure at %08X (%u bytes) differs (here/original):%s", addr, len, buf);
                }
            }
        } else if (t == ALOG_ACMD) {
            u32 len, lo, hi;
            uint64_t want, got;
            u32 have = ret - a[0];
            next();
            len = next();
            lo = next();
            hi = next();
            want = (uint64_t)hi << 32 | lo;
            got = fnv(mem(a[0]), have);
            if (dump_acmd_frame == nframes && dump_acmd_path) {
                FILE *f = fopen(dump_acmd_path, "wb");
                fwrite(mem(a[0]), 1, have, f);
                fclose(f);
            }
            if (have != len || got != want)
                mismatch("audio frame %ld's command list: %u bytes here, %u in the original%s", nframes,
                         have, len, have == len ? " (the same length, other commands)" : "");
            nframes++;
        } else if (t == ALOG_RET) {
            u32 v;
            next();
            v = next();
            if (v != ret && v != to_file(ret))
                mismatch("returned %08X, the original %08X", ret, v);
            break;
        } else if (t == ALOG_CBENTER) {
            ensure(2);
            desync("the original called back (kind %u) where this library had returned", logw[pos + 1]);
        } else {
            desync("unexpected record %u after the call", t);
        }
    }

    if (hashes) {
        uint64_t h = mem_hash();
        fprintf(hashes, "%ld %s %016llx\n", my_call, fns[fn].name, (unsigned long long)h);
    }
    if (dump_mem_call == my_call && dump_mem_path)
        dump_mem(dump_mem_path);
    if (!quiet && fn == ALOG_alAudioFrame && nframes % 1000 == 0)
        fprintf(stderr, "replay: %ld frames, %ld calls\n", nframes, ncalls);
    if (stop_call == my_call) {
        fprintf(stderr, "replay: stopped after call %ld\n", my_call);
        exit(nmismatch ? 1 : 0);
    }
    cur_fn = saved_fn;
    cur_call = saved_call;
}

/* ---- main -------------------------------------------------------------------- */

int main(int argc, char **argv) {
    struct sigaction sa;
    int i;

    /* the same addresses every run (this program's DMA routine is in the
       shared memory, as the synthesizer's) */
    if (!(personality(0xFFFFFFFF) & ADDR_NO_RANDOMIZE)) {
        personality(ADDR_NO_RANDOMIZE);
        execv("/proc/self/exe", argv);
    }
    if (argc < 3) {
        fprintf(stderr, "usage: replay LIB.so LOG [-k] [-q] [--poison B] [--hashes FILE] [--dump-acmd N FILE] "
                        "[--dump-mem N FILE] [--stop N] [--translations FILE]\n");
        return 2;
    }
    for (i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "-k"))
            keep_going = 1;
        else if (!strcmp(argv[i], "-q"))
            quiet = 1;
        else if (!strcmp(argv[i], "--poison") && i + 1 < argc)
            poison = (int)strtol(argv[++i], NULL, 0) & 0xFF;
        else if (!strcmp(argv[i], "--hashes") && i + 1 < argc)
            hashes = fopen(argv[++i], "w");
        else if (!strcmp(argv[i], "--dump-acmd") && i + 2 < argc) {
            dump_acmd_frame = atol(argv[++i]);
            dump_acmd_path = argv[++i];
        } else if (!strcmp(argv[i], "--dump-mem") && i + 2 < argc) {
            dump_mem_call = atol(argv[++i]);
            dump_mem_path = argv[++i];
        } else if (!strcmp(argv[i], "--stop") && i + 1 < argc)
            stop_call = atol(argv[++i]);
        else if (!strcmp(argv[i], "--translations") && i + 1 < argc)
            xlog = fopen(argv[++i], "w");
        else {
            fprintf(stderr, "replay: unknown option %s\n", argv[i]);
            return 2;
        }
    }
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_fault;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);

    lib_path = realpath(argv[1], NULL);
    lib = lib_path ? dlopen(lib_path, RTLD_NOW | RTLD_LOCAL) : NULL;
    if (lib == NULL) {
        fprintf(stderr, "replay: %s\n", dlerror());
        return 2;
    }
    {
        dl_iterate_phdr(find_lib, lib_path);
        if (lib_hi == 0) {
            fprintf(stderr, "replay: can't find where %s is loaded\n", argv[1]);
            return 2;
        }
    }
    for (i = 1; i < ALOG_FN_COUNT; i++) {
        fnp[i] = dlsym(lib, fns[i].name);
        if (fnp[i] == NULL) {
            fprintf(stderr, "replay: %s has no %s\n", argv[1], fns[i].name);
            return 2;
        }
    }
    bank_map_fn = (BankMapFn)dlsym(lib, ALOG_BANK_MAP);

    logf = fopen(argv[2], "rb");
    if (logf == NULL) {
        perror(argv[2]);
        return 2;
    }
    logw = malloc(WINDOW * 4);
    if (next() != ALOG_MAGIC || next() != ALOG_VERSION) {
        fprintf(stderr, "replay: %s isn't an audio log of version %d\n", argv[2], ALOG_VERSION);
        return 2;
    }

    for (;;) {
        u32 t = peek();
        if (t == ALOG_END)
            break;
        if (t == ALOG_TRACK) {
            u32 addr, len;
            next();
            addr = next();
            len = next();
            track(addr, len);
        } else if (t == ALOG_UNTRACK) {
            u32 addr, len;
            next();
            addr = next();
            len = next();
            untrack(addr, len);
        } else if (t == ALOG_VAR) {
            u32 id;
            next();
            id = next();
            if (id == ALOG_VAR_MONO)
                mono_addr = next();
            else
                next();
        } else if (t == ALOG_MEM) {
            u32 addr, len;
            next();
            addr = next();
            len = next();
            map(addr, len);
            memcpy(mem(addr), take_bytes(len), len);
            game_writes(addr, len);
        } else if (t == ALOG_CALL) {
            run_call();
        } else {
            fprintf(stderr, "replay: unexpected record %u at word %zu\n", t, pos);
            return 2;
        }
    }
    printf("replay: %ld calls, %ld audio frames, %ld callbacks, %ld mismatches\n", ncalls, nframes,
           ncallbacks, nmismatch);
    if (bank_map_fn != NULL)
        printf("replay: bank objects: %u mapped; translated %ld arguments, %ld memory words; %ld structures "
               "the same as the file's\n", nmap, nxlate_arg, nxlate_mem, nxlate_back);
    if (xlog)
        fclose(xlog);
    if (hashes)
        fclose(hashes);
    return nmismatch ? 1 : 0;
}
