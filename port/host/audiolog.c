/*
 * The libaudio oracle's recorder, host side (port/include/audio_log.h;
 * DozerEncore's): PORT_AUDIO_LOG=FILE in a -DPORT_AUDIO_RECORD=ON build
 * writes the log of every libaudio call the game makes
 * (port/src/audio_record.c).
 *
 * The tracked memory has a shadow copy, taken each time the library hands
 * control back to the game; when the game hands it to the library again,
 * the bytes that differ from the shadow are logged (MEM) and the shadow
 * updated.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port.h"
#include "host.h"
#include "audio_log.h"

#ifdef PORT_AUDIO_RECORD

#include <elf.h>

#define MAX_RANGES 256

typedef struct {
    const uint8_t *p;
    uint32_t len;
    uint8_t *shadow;
} Range;

static Range ranges[MAX_RANGES];
static int nranges;
static int started;
static FILE *out;

static void w32(uint32_t v) {
    fwrite(&v, 4, 1, out);
}

static void wbytes(const void *p, uint32_t n) {
    static const uint8_t zero[4];
    fwrite(p, 1, n, out);
    if (n & 3)
        fwrite(zero, 1, 4 - (n & 3), out);
}

/* an address as the log has it (an i386 build's) */
static uint32_t log_addr(const void *p) { return (uint32_t)(uintptr_t)p; }

static void finish(void) {
    if (out != NULL) {
        w32(ALOG_END);
        fclose(out);
        out = NULL;
    }
}

int host_alog_started(void) {
    return started;
}

/* add [p, p+len) (the caller gives ranges apart from the others); once the
   log has started, with its bytes as they are */
void host_alog_track(const void *p, uint32_t len) {
    Range *r;
    if (len == 0)
        return;
    if (nranges == MAX_RANGES)
        host_fatal("audiolog: too many ranges");
    r = &ranges[nranges++];
    r->p = p;
    r->len = len;
    r->shadow = NULL;
    if (out != NULL) {
        r->shadow = malloc(len);
        memcpy(r->shadow, p, len);
        w32(ALOG_TRACK);
        w32(log_addr(p));
        w32(len);
        w32(ALOG_MEM);
        w32(log_addr(p));
        w32(len);
        wbytes(p, len);
    }
}

/* the address and size of the game's variable name, by the executable's
   symbol table (a static one too): NULL if it has none */
void *host_alog_symbol(const char *name, uint32_t *size) {
    static uint8_t *img;
    static long len;
    Elf32_Ehdr *eh;
    Elf32_Shdr *sh;
    int i;

    if (img == NULL) {
        FILE *f = fopen("/proc/self/exe", "rb");
        if (f == NULL)
            host_fatal("audiolog: can't read /proc/self/exe");
        fseek(f, 0, SEEK_END);
        len = ftell(f);
        fseek(f, 0, SEEK_SET);
        img = malloc(len);
        if (fread(img, 1, len, f) != (size_t)len)
            host_fatal("audiolog: can't read /proc/self/exe");
        fclose(f);
    }
    eh = (Elf32_Ehdr *)img;
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 || eh->e_ident[EI_CLASS] != ELFCLASS32)
        host_fatal("audiolog: not a 32-bit ELF executable");
    sh = (Elf32_Shdr *)(img + eh->e_shoff);
    for (i = 0; i < eh->e_shnum; i++) {
        Elf32_Sym *sym;
        const char *str;
        uint32_t k, n;
        if (sh[i].sh_type != SHT_SYMTAB)
            continue;
        sym = (Elf32_Sym *)(img + sh[i].sh_offset);
        str = (const char *)(img + sh[sh[i].sh_link].sh_offset);
        n = sh[i].sh_size / sizeof(Elf32_Sym);
        for (k = 0; k < n; k++)
            if (ELF32_ST_TYPE(sym[k].st_info) == STT_OBJECT && strcmp(str + sym[k].st_name, name) == 0) {
                if (size != NULL)
                    *size = sym[k].st_size;
                return (void *)(uintptr_t)sym[k].st_value;
            }
    }
    return NULL;
}

int host_alog_track_name(const char *name) {
    uint32_t size;
    void *p = host_alog_symbol(name, &size);
    if (p == NULL)
        return 0;
    host_alog_track(p, size);
    return 1;
}

/* take [p, p+len) out of the tracked ranges, splitting one */
void host_alog_untrack(const void *p, uint32_t len) {
    const uint8_t *addr = p, *end = addr + len;
    int i;
    if (len == 0)
        return;
    for (i = 0; i < nranges; i++) {
        Range *r = &ranges[i];
        const uint8_t *rend = r->p + r->len;
        if (addr >= rend || end <= r->p)
            continue;
        if (addr <= r->p && end >= rend) {         /* all of it */
            r->len = 0;
        } else if (addr <= r->p) {                 /* its start */
            uint32_t cut = (uint32_t)(end - r->p);
            if (r->shadow != NULL)
                memmove(r->shadow, r->shadow + cut, r->len - cut);
            r->p = end;
            r->len -= cut;
        } else if (end >= rend) {                  /* its end */
            r->len = (uint32_t)(addr - r->p);
        } else {                                   /* its middle */
            Range *n;
            if (nranges == MAX_RANGES)
                host_fatal("audiolog: too many ranges");
            n = &ranges[nranges++];
            r = &ranges[i];
            n->p = end;
            n->len = (uint32_t)(rend - end);
            n->shadow = NULL;
            if (r->shadow != NULL) {
                n->shadow = malloc(n->len);
                memcpy(n->shadow, r->shadow + (end - r->p), n->len);
            }
            r->len = (uint32_t)(addr - r->p);
        }
    }
    /* (empty ranges' slots reused) */
    for (i = 0; i < nranges;)
        if (ranges[i].len == 0) {
            free(ranges[i].shadow);
            ranges[i] = ranges[--nranges];
        } else {
            i++;
        }
    if (out != NULL) {
        w32(ALOG_UNTRACK);
        w32(log_addr(addr));
        w32(len);
    }
}

/* port_alloc's blocks (80580.c's sounds, port/src/memory.c): shared from
   their allocation to their free */
void host_alog_block(const void *p, uint32_t len) {
    host_alog_track(p, len);
}

void host_alog_block_free(const void *p) {
    int i;
    for (i = 0; i < nranges; i++)
        if (ranges[i].p == p) {
            host_alog_untrack(p, ranges[i].len);
            return;
        }
}

void host_alog_start(void) {
    const char *path = getenv("PORT_AUDIO_LOG");
    int i;

    started = 1;
    if (path == NULL || *path == 0)
        return;
    out = fopen(path, "wb");
    if (out == NULL)
        host_fatal("audiolog: can't write %s", path);
    setvbuf(out, NULL, _IOFBF, 1 << 20);
    atexit(finish);
    w32(ALOG_MAGIC);
    w32(ALOG_VERSION);
    for (i = 0; i < nranges; i++) {
        Range *r = &ranges[i];
        r->shadow = malloc(r->len);
        memcpy(r->shadow, r->p, r->len);
        w32(ALOG_TRACK);
        w32(log_addr(r->p));
        w32(r->len);
        w32(ALOG_MEM);
        w32(log_addr(r->p));
        w32(r->len);
        wbytes(r->shadow, r->len);
    }
    host_log("audiolog: writing %s\n", path);
}

void host_alog_var(uint32_t id, const void *p) {
    if (out == NULL)
        return;
    w32(ALOG_VAR);
    w32(id);
    w32(log_addr(p));
}

/* the game hands control to the library: log what changed */
static void diff(void) {
    int i;
    for (i = 0; i < nranges; i++) {
        Range *r = &ranges[i];
        const uint8_t *cur = r->p;
        uint32_t o = 0;
        while (o < r->len) {
            uint32_t n = r->len - o < 64 ? r->len - o : 64, s, e;
            if (memcmp(cur + o, r->shadow + o, n) == 0) {
                o += n;
                continue;
            }
            /* the changed blocks, as runs of the bytes that changed (an
               unchanged byte between two is the library's: its contents
               may differ in another library) */
            s = o;
            while (o < r->len) {
                n = r->len - o < 64 ? r->len - o : 64;
                if (memcmp(cur + o, r->shadow + o, n) == 0)
                    break;
                o += n;
            }
            e = o;
            while (s < e) {
                uint32_t rs;
                while (s < e && cur[s] == r->shadow[s])
                    s++;
                if (s == e)
                    break;
                rs = s;
                while (s < e && cur[s] != r->shadow[s])
                    s++;
                w32(ALOG_MEM);
                w32(log_addr(r->p) + rs);
                w32(s - rs);
                wbytes(cur + rs, s - rs);
                memcpy(r->shadow + rs, cur + rs, s - rs);
            }
        }
    }
}

/* the library hands control to the game: its writes are its own */
static void snapshot(void) {
    int i;
    for (i = 0; i < nranges; i++)
        memcpy(ranges[i].shadow, ranges[i].p, ranges[i].len);
}

void host_alog_call(uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
                    uint32_t a5) {
    if (out == NULL)
        return;
    diff();
    w32(ALOG_CALL);
    w32(fn);
    w32(a0);
    w32(a1);
    w32(a2);
    w32(a3);
    w32(a4);
    w32(a5);
}

static void blob(uint32_t tag, const void *p, uint32_t len) {
    if (out == NULL)
        return;
    w32(tag);
    w32(log_addr(p));
    w32(len);
    wbytes(p, len);
}

void host_alog_in(const void *p, uint32_t len) { blob(ALOG_IN, p, len); }
void host_alog_out(const void *p, uint32_t len) { blob(ALOG_OUT, p, len); }

void host_alog_acmd(const void *addr, uint32_t len) {
    const uint8_t *p = addr;
    uint64_t h = 0xcbf29ce484222325ull;
    uint32_t i;
    if (out == NULL)
        return;
    for (i = 0; i < len; i++)
        h = (h ^ p[i]) * 0x100000001b3ull;
    w32(ALOG_ACMD);
    w32(len);
    w32((uint32_t)h);
    w32((uint32_t)(h >> 32));
}

void host_alog_ret(uint32_t value) {
    if (out == NULL)
        return;
    w32(ALOG_RET);
    w32(value);
    snapshot();
}

void host_alog_cb_enter(uint32_t kind, uint32_t a0, uint32_t a1, uint32_t a2) {
    if (out == NULL)
        return;
    w32(ALOG_CBENTER);
    w32(kind);
    w32(a0);
    w32(a1);
    w32(a2);
    snapshot();
}

void host_alog_cb_leave(uint32_t value, uint32_t x0, uint32_t x1) {
    if (out == NULL)
        return;
    diff();
    w32(ALOG_CBRET);
    w32(value);
    w32(x0);
    w32(x1);
}

#endif
