/*
 * PORT_FB_WATCH=FILE: every read and write of a color or z image by
 * anything but the RDP, for a renderer that keeps its images elsewhere (a
 * GPU's): docs/PORT.md, "Graphics", "The framebuffer's other users".
 *
 * gfx.c names each image the RDP is given (SETCIMG, SETZIMG) and says
 * when a task starts and ends.  Between tasks the pages of the images
 * used in the last 60 retraces are made inaccessible; an access faults,
 * the handler records where it came from (the faulting instruction and
 * its callers, and the offset in the image), opens the page and steps the
 * one instruction (the trap flag), and the step's trap closes it again.
 * At the exit FILE gets one line per image and one per distinct access
 * (its count, the first and last retrace, the offsets, read or write, and
 * the call chain as module+offset, which port/tools/gbi_census.py fbwatch
 * gives addr2line).  Linux on x86 only; a debugging aid, off by default
 * (nothing is protected without the variable).
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <string.h>

#include "host.h"

int host_fbw_on;

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__)) && !defined(__EMSCRIPTEN__)
#include <dlfcn.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>

#if defined(__x86_64__)
#define REG_PC REG_RIP
#else
#define REG_PC REG_EIP
#endif

#define NIMG 32
#define NFRAMES 10
#define NREC 8192

static struct {
    const uint8_t *p;
    size_t n;
    int z;
    unsigned last;                      /* the retrace it was last given to the RDP */
    int armed;
} img[NIMG];
static int nimg;
static const char *path;
static uintptr_t page;
static volatile int in_task = 1;
static uintptr_t open_pages[8];
static int nopen;

typedef struct {
    void *pc[NFRAMES];
    int nf, write, im;
    uint64_t n;
    unsigned first, last;
    size_t lo, hi;
} Rec;
static Rec *rec;
static int nrec, lost;

static void prot(const uint8_t *p, size_t n, int how) {
    uintptr_t a = (uintptr_t)p & ~(page - 1), b = ((uintptr_t)p + n + page - 1) & ~(page - 1);
    mprotect((void *)a, b - a, how);
}

void host_fbw_image(const uint8_t *p, size_t n, int z) {
    if (!host_fbw_on || p < port_rdram || p >= port_rdram + PORT_RDRAM_SIZE)
        return;
    for (int i = 0; i < nimg; i++)
        if (img[i].p == p) {
            if (n > img[i].n)
                img[i].n = n;
            img[i].z |= z;
            img[i].last = host_vi_count;
            return;
        }
    if (nimg < NIMG) {
        img[nimg].p = p;
        img[nimg].n = n;
        img[nimg].z = z;
        img[nimg++].last = host_vi_count;
    }
}

void host_fbw_task(int begin) {
    if (!host_fbw_on)
        return;
    if (begin) {
        in_task = 1;
        for (int i = 0; i < nimg; i++)
            if (img[i].armed) {
                prot(img[i].p, img[i].n, PROT_READ | PROT_WRITE);
                img[i].armed = 0;
            }
        return;
    }
    for (int i = 0; i < nimg; i++)
        if (host_vi_count - img[i].last <= 60) {
            prot(img[i].p, img[i].n, PROT_NONE);
            img[i].armed = 1;
        }
    in_task = 0;
}

static int armed_page(uintptr_t a) {
    for (int i = 0; i < nimg; i++) {
        uintptr_t lo = (uintptr_t)img[i].p & ~(page - 1);
        uintptr_t hi = ((uintptr_t)img[i].p + img[i].n + page - 1) & ~(page - 1);
        if (img[i].armed && a >= lo && a < hi)
            return 1;
    }
    return 0;
}

static void record(const uint8_t *a, int write, void *pc) {
    int im = -1;
    for (int i = 0; i < nimg; i++)
        if (img[i].armed && a >= img[i].p && a < img[i].p + img[i].n)
            im = i;
    if (im < 0)
        return;                         /* (a neighbour on the image's page) */
    void *bt[NFRAMES + 8];
    int n = backtrace(bt, NFRAMES + 8), k = 0;
    while (k < n && bt[k] != pc)
        k++;
    Rec r = { 0 };
    r.pc[0] = pc;
    r.nf = 1;
    for (int j = (k < n ? k + 1 : 2); j < n && r.nf < NFRAMES; j++)
        r.pc[r.nf++] = bt[j];
    r.write = write;
    r.im = im;
    size_t off = (size_t)(a - img[im].p);
    for (int i = 0; i < nrec; i++) {
        Rec *e = &rec[i];
        if (e->im == im && e->write == write && e->nf == r.nf && !memcmp(e->pc, r.pc, sizeof r.pc)) {
            e->n++;
            e->last = host_vi_count;
            if (off < e->lo)
                e->lo = off;
            if (off > e->hi)
                e->hi = off;
            return;
        }
    }
    if (nrec == NREC) {
        lost++;
        return;
    }
    r.n = 1;
    r.first = r.last = host_vi_count;
    r.lo = r.hi = off;
    rec[nrec++] = r;
}

static void on_segv(int sig, siginfo_t *si, void *ctx) {
    ucontext_t *uc = ctx;
    uintptr_t a = (uintptr_t)si->si_addr;
    if (in_task || !armed_page(a) || nopen == 8) {
        signal(SIGSEGV, SIG_DFL);       /* (not ours: crash as before) */
        return;
    }
    record((const uint8_t *)a, (uc->uc_mcontext.gregs[REG_ERR] & 2) != 0, (void *)uc->uc_mcontext.gregs[REG_PC]);
    uintptr_t pg = a & ~(page - 1);
    mprotect((void *)pg, page, PROT_READ | PROT_WRITE);
    open_pages[nopen++] = pg;
    uc->uc_mcontext.gregs[REG_EFL] |= 0x100;
    (void)sig;
}

static void on_trap(int sig, siginfo_t *si, void *ctx) {
    ucontext_t *uc = ctx;
    if (!nopen) {
        signal(SIGTRAP, SIG_DFL);
        raise(SIGTRAP);
        return;
    }
    for (int i = 0; i < nopen; i++)
        if (!in_task && armed_page(open_pages[i]))
            mprotect((void *)open_pages[i], page, PROT_NONE);
    nopen = 0;
    uc->uc_mcontext.gregs[REG_EFL] &= ~0x100;
    (void)sig, (void)si;
}

static void where(FILE *f, void *p) {
    Dl_info d;
    if (dladdr(p, &d) && d.dli_fname)
        fprintf(f, " %s+%lx", d.dli_fname, (unsigned long)((uintptr_t)p - (uintptr_t)d.dli_fbase));
    else
        fprintf(f, " ?+%lx", (unsigned long)(uintptr_t)p);
}

static void finish(void) {
    for (int i = 0; i < nimg; i++)
        if (img[i].armed)
            prot(img[i].p, img[i].n, PROT_READ | PROT_WRITE);
    host_fbw_on = 0;
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    for (int i = 0; i < nimg; i++)
        fprintf(f, "img %d %06lX %lu %s\n", i, (unsigned long)(img[i].p - port_rdram), (unsigned long)img[i].n,
                img[i].z ? "z" : "color");
    for (int i = 0; i < nrec; i++) {
        Rec *e = &rec[i];
        fprintf(f, "acc %d %s %llu %u %u %lu %lu", e->im, e->write ? "w" : "r", (unsigned long long)e->n, e->first,
                e->last, (unsigned long)e->lo, (unsigned long)e->hi);
        for (int j = 0; j < e->nf; j++)
            where(f, e->pc[j]);
        fputc('\n', f);
    }
    if (lost)
        fprintf(f, "lost %d\n", lost);
    fclose(f);
}

void host_fbw_init(void) {
    const char *e = getenv("PORT_FB_WATCH");
    if (!e || !*e)
        return;
    path = e;
    page = (uintptr_t)sysconf(_SC_PAGESIZE);
    rec = calloc(NREC, sizeof *rec);
    void *bt[4];
    backtrace(bt, 4);                   /* (its first call loads libgcc: not in the handler) */
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sa.sa_sigaction = on_segv;
    sigaction(SIGSEGV, &sa, NULL);
    sa.sa_sigaction = on_trap;
    sigaction(SIGTRAP, &sa, NULL);
    host_fbw_on = 1;
    atexit(finish);
}

#else
void host_fbw_image(const uint8_t *p, size_t n, int z) { (void)p, (void)n, (void)z; }
void host_fbw_task(int begin) { (void)begin; }
void host_fbw_init(void) {
    if (getenv("PORT_FB_WATCH"))
        host_log("PORT_FB_WATCH: Linux on x86 only\n");
}
#endif
