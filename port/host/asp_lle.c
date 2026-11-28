/*
 * The audio microcode, low level (--audio lle, PORT_AUDIO_LLE=1): each
 * audio task runs the game's own aspMain (its text D_8029E750, its data
 * D_802A5040, booted by rspboot D_8029D250) on the port's RSP interpreter
 * (rsp.c), as the graphics microcode does (docs/PORT.md, "Audio").
 *
 * The microcode sees a 16 MiB DRAM through its DMAs (24-bit addresses):
 * the low 8 MiB are port_rdram, as it is (the audio heap, the AI's
 * buffers, the reverb's delay lines, the samples the game's DMA routine
 * brought in, the microcode's state between frames: all bytes only the
 * RSP reads or the N64's order already); above, a window this task maps:
 * the microcode's text and data, the command list as the N64 has it
 * (libaudio's Acmd words are pointer-wide on a host: HostGfx), and what
 * the game keeps in the host's order or outside port_rdram (the ADPCM
 * codebooks and loop states, which native.c converted: copied with their
 * halfwords swapped on a little-endian host; anything else outside
 * port_rdram mapped where it is).
 *
 * DMEM is the audio's own between tasks (as aspmain.c's, zero at the
 * power-on), whatever renderer runs: of what a task didn't write itself,
 * only one thing reaches the sound, POLEF with A_INIT, which takes two
 * samples of history from DMEM 0xF94-0xF97, the state scratch the last
 * RESAMPLE, ENVMIXER or POLEF left (PORT_RSP_WATCH and PORT_AUDIO_LLE_FILL,
 * docs/PORT.md "Audio"); where none ran yet in the task, on the N64 that
 * is what the graphics task in between left.
 *
 * Checked against cxd4's interpreter task by task (PORT_ASP_REF,
 * port/tools/asp_ref), and command by command against aspmain.c
 * (PORT_ASP_DIAG).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "host.h"
#include "rsp.h"

typedef struct {
    uintptr_t w0, w1;
} HostGfx;

/* the sizes the N64 has (85A10.c's arrays; rspboot loads 0xF80 bytes of
   text and the task's ucode_data_size, 0x800, of data: what follows the
   arrays in the N64's memory, main_code's .data and .bss, never runs and
   doesn't change the sound: PORT_AUDIO_LLE_DATAFILL) */
#define ASP_TEXT 0xE20
#define ASP_DATA 0x2C0
#define BOOT_TEXT 0xF80

#define WIN_BASE 0x800000u
#define WIN_SIZE 0x200000u
#define NREG 1024

static uint8_t win[WIN_SIZE];
static uint32_t win_top;
static struct {
    uint32_t virt, len;
    uint8_t *host;
} reg[NREG];
static int nreg;
static uint8_t asp_dmem[4096];          /* DMEM between audio tasks */
int host_audio_lle = -1;             /* (-1: PORT_AUDIO_LLE decides) */

static int lle_stats;
static uint64_t st_tasks, st_insns, st_ns, st_cmds;

static uint64_t wall_ns(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void stats_exit(void) {
    if (st_tasks)
        fprintf(stderr, "audio lle: %llu tasks, %llu commands, %llu RSP instructions (%.0f a task), %.3f ms a task\n",
                (unsigned long long)st_tasks, (unsigned long long)st_cmds, (unsigned long long)st_insns,
                (double)st_insns / (double)st_tasks, (double)st_ns / 1e6 / (double)st_tasks);
}

#ifdef PORT_RSP_WATCH
extern uint8_t rsp_watch_written[4096];
extern uint32_t rsp_watch_stale[4096];
extern uint16_t rsp_watch_pc[4096];
void rsp_watch_clear(void);
static void watch_exit(void) {
    int any = 0;
    for (int a = 0; a < 4096;) {
        if (!rsp_watch_stale[a]) {
            a++;
            continue;
        }
        int b = a;
        uint64_t n = 0;
        while (b < 4096 && rsp_watch_stale[b])
            n += rsp_watch_stale[b++];
        fprintf(stderr, "audio lle: DMEM %03X-%03X read before written in the task, %llu times (first by %03X)\n", a, b - 1,
                (unsigned long long)n, rsp_watch_pc[a] & 0xFFF);
        any = 1;
        a = b;
    }
    if (!any)
        fprintf(stderr, "audio lle: no DMEM byte read before the task wrote it\n");
}
#endif

/* (the image: port_rdram in the N64 layout, the fixed-address names at
   their offsets there in the others) */
static int is_rdram(const void *p) { return port_image_off(p) >= 0; }

/* a place in the window: n bytes, at the same offset in 16 as `align` */
static uint32_t win_alloc(uint32_t n, uintptr_t align) {
    uint32_t at = ((win_top + 15) & ~15u) + (uint32_t)(align & 15);
    if (at + n > WIN_SIZE)
        host_fatal("audio lle: the task's window is full\n");
    win_top = at + n;
    return at;
}

static uint32_t map_region(uint8_t *host, uint32_t len) {
    if (nreg == NREG)
        host_fatal("audio lle: too many regions in a task\n");
    uint32_t v = WIN_BASE + win_alloc(0, (uintptr_t)host);
    win_top += len;
    if (win_top > WIN_SIZE)
        host_fatal("audio lle: the task's window is full\n");
    reg[nreg].virt = v;
    reg[nreg].len = len;
    reg[nreg].host = host;
    nreg++;
    return v;
}

/* the DRAM address of n bytes at host (a copy, halfword-swapped, where
   `swap`) */
static uint32_t dram_of(uintptr_t a, uint32_t n, int swap) {
    uint8_t *p = port_addr(a);
    if (!swap && is_rdram(p) && port_image_room(p) >= n)
        return (uint32_t)port_image_off(p);
    if (!swap) {
        static int said;
        if (!said && host_verbose)
            host_log("audio lle: %u bytes outside port_rdram at %p, mapped\n", n, (void *)p);
        said = 1;
        return map_region(p, n);
    }
    uint32_t at = win_alloc(n, (uintptr_t)p);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    for (uint32_t i = 0; i + 1 < n; i += 2) {
        win[at + i] = p[i + 1];
        win[at + i + 1] = p[i];
    }
#else
    memcpy(win + at, p, n);
#endif
    return WIN_BASE + at;
}

static void lle_dma(int write, uint32_t dram, uint8_t *mem, uint32_t len) {
    dram &= 0xFFFFFF;
    for (uint32_t i = 0; i < len;) {
        uint32_t d = dram + i, k;
        uint8_t *h = NULL;
        if (d < PORT_RDRAM_SIZE) {
            size_t room = 0;
            h = port_image_host(d, &room);
            k = (uint32_t)room;
        } else {
            k = 0;
            for (int r = 0; r < nreg; r++)
                if (d - reg[r].virt < reg[r].len) {
                    k = reg[r].len - (d - reg[r].virt);
                    h = reg[r].host + (d - reg[r].virt);
                    break;
                }
            if (!h && d >= WIN_BASE && d < WIN_BASE + win_top) {
                k = WIN_BASE + win_top - d;
                h = win + (d - WIN_BASE);
            }
        }
        if (k > len - i)
            k = len - i;
        if (!h) {
            /* (nothing there: reads as 0, writes go nowhere) */
            static int said;
            if (!said)
                host_log("audio lle: a DMA %s %06X, nothing mapped there\n", write ? "to" : "from", d);
            said = 1;
            if (!write)
                mem[i] = 0;
            i++;
            continue;
        }
        if (write)
            memcpy(h, mem + i, k);
        else
            memcpy(mem + i, h, k);
        i += k;
    }
}

#ifdef PORT_ASP_REF
/* (a checking build: every task on cxd4's RSP interpreter too, from the
   same memory, port/tools/asp_ref; the DRAM it writes and its DMEM after
   compared with rsp.c's) */
int aspref_task(uint8_t *img, uint32_t n, uint8_t *dmem, uint8_t *imem);
#define REF_SIZE (WIN_BASE + WIN_SIZE)
static uint8_t *ref_img, ref_dmem[4096], ref_imem[4096];
static uint64_t ref_tasks, ref_bad, ref_bytes, ref_dmem_bad;

static void ref_exit(void) {
    fprintf(stderr, "audio lle: against cxd4: %llu tasks, %llu differ (%llu bytes of DRAM; DMEM in %llu)\n",
            (unsigned long long)ref_tasks, (unsigned long long)ref_bad, (unsigned long long)ref_bytes,
            (unsigned long long)ref_dmem_bad);
}

static void ref_begin(void) {
    if (!ref_img) {
        ref_img = malloc(REF_SIZE);
        atexit(ref_exit);
    }
    port_image_copy(ref_img);
    memset(ref_img + WIN_BASE, 0, WIN_SIZE);
    memcpy(ref_img + WIN_BASE, win, win_top);
    for (int r = 0; r < nreg; r++)
        memcpy(ref_img + reg[r].virt, reg[r].host, reg[r].len);
    memcpy(ref_dmem, rsp_dmem, sizeof ref_dmem);
    memcpy(ref_imem, rsp_imem, sizeof ref_imem);
}

static void ref_end(uint32_t ncmd) {
    if (!aspref_task(ref_img, REF_SIZE, ref_dmem, ref_imem))
        host_log("audio lle: cxd4 didn't end the task\n");
    ref_tasks++;
    uint32_t bad = 0, first = 0;
    static uint8_t *cur;
    if (!cur && !(cur = malloc(PORT_RDRAM_SIZE)))
        host_fatal("audio lle: no memory");
    port_image_copy(cur);
    for (uint32_t a = 0; a < PORT_RDRAM_SIZE; a++)
        if (ref_img[a] != cur[a]) {
            if (!bad)
                first = a;
            bad++;
        }
    for (int r = 0; r < nreg; r++)
        for (uint32_t i = 0; i < reg[r].len; i++)
            if (ref_img[reg[r].virt + i] != reg[r].host[i]) {
                if (!bad)
                    first = reg[r].virt + i;
                bad++;
            }
    int dbad = memcmp(ref_dmem, rsp_dmem, sizeof ref_dmem) != 0;
    if (bad || dbad) {
        if (ref_bad < 20) {
            host_log("audio lle: %u: task %llu (%u commands) differs from cxd4: %u bytes of DRAM from %06X, DMEM %s\n",
                     host_vi_count, (unsigned long long)ref_tasks, ncmd, bad, first, dbad ? "differs" : "the same");
            if (dbad)
                for (int a = 0; a < 4096; a++)
                    if (ref_dmem[a] != rsp_dmem[a]) {
                        host_log("  DMEM %03X: %02X cxd4 %02X\n", a, rsp_dmem[a], ref_dmem[a]);
                        break;
                    }
        }
        ref_bad++;
        ref_bytes += bad;
        ref_dmem_bad += dbad;
    }
}
#endif

enum {
    A_SPNOOP, A_ADPCM, A_CLEARBUFF, A_ENVMIXER, A_LOADBUFF, A_RESAMPLE, A_SAVEBUFF, A_SEGMENT,
    A_SETBUFF, A_SETVOL, A_DMEMMOVE, A_LOADADPCM, A_MIXER, A_INTERLEAVE, A_POLEF, A_SETLOOP,
};

#ifdef PORT_ASP_DIAG
/* (a debugging build: each command against aspmain.c's: the microcode
   stops at its dispatch, 0x0E4, before each command; aspmain.c runs the
   command from the same DMEM, vector registers, accumulators and flags,
   and a copy of the state block the command keeps in RDRAM, as they were
   there; then DMEM (all but the command list at 0x380-0x4BF, which the
   microcode refills after a chunk's last command, before it stops again),
   the registers and the state block are compared with the microcode's
   after it.  The table at the exit.) */
void host_asp_hle_one(uint8_t *dm, uintptr_t w0, uintptr_t w1, uint8_t *state, uint16_t v[32][8], uint16_t a[3][8],
                      uint16_t fl[4]);
static const char *op_name[16] = {
    "SPNOOP", "ADPCM", "CLEARBUFF", "ENVMIXER", "LOADBUFF", "RESAMPLE", "SAVEBUFF", "SEGMENT",
    "SETBUFF", "SETVOL", "DMEMMOVE", "LOADADPCM", "MIXER", "INTERLEAVE", "POLEF", "SETLOOP",
};
static struct {
    uint64_t n, bad, dmem_cmds, dmem_bytes, state_cmds, state_bytes, reg_cmds;
    int max;
} dg[16][2];
static int dg_shown;

static void diag_exit(void) {
    uint64_t n = 0, bad = 0;
    fprintf(stderr, "audio lle against aspmain.c, by command (init: the F_INIT flag):\n"
                    "  command   init    count  differ  DMEM:cmds   bytes  max|d|  state:cmds   bytes  regs:cmds\n");
    for (int op = 0; op < 16; op++)
        for (int in = 0; in < 2; in++) {
            if (!dg[op][in].n)
                continue;
            n += dg[op][in].n;
            bad += dg[op][in].bad;
            fprintf(stderr, "  %-10s %d %8llu %7llu %10llu %7llu %7d %11llu %7llu %10llu\n", op_name[op], in,
                    (unsigned long long)dg[op][in].n, (unsigned long long)dg[op][in].bad,
                    (unsigned long long)dg[op][in].dmem_cmds, (unsigned long long)dg[op][in].dmem_bytes,
                    dg[op][in].max, (unsigned long long)dg[op][in].state_cmds,
                    (unsigned long long)dg[op][in].state_bytes, (unsigned long long)dg[op][in].reg_cmds);
        }
    fprintf(stderr, "audio lle against aspmain.c: %llu commands, %llu differ\n", (unsigned long long)n,
            (unsigned long long)bad);
}

static void diag_cmd(uintptr_t w0a, uintptr_t w1a, const uint8_t *pre, const uint8_t *st_pre, uint16_t v_pre[32][8],
                     uint16_t a_pre[3][8], uint16_t f_pre[4]) {
    static uint8_t hle[4096], st[96];
    static uint16_t v[32][8], a[3][8], f[4], lv[32][8], la[3][8], lf[4];
    uint32_t w0 = (uint32_t)w0a;
    int op = w0 >> 24, flags = (w0 >> 16) & 0xFF;
    int st_len = op == A_ADPCM || op == A_RESAMPLE ? 32 : op == A_POLEF ? 8 : op == A_ENVMIXER ? 80 : 0;
    int init = st_len && (flags & 1);
    memcpy(hle, pre, sizeof hle);
    memcpy(st, st_pre, sizeof st);
    memcpy(v, v_pre, sizeof v);
    memcpy(a, a_pre, sizeof a);
    memcpy(f, f_pre, sizeof f);
    host_asp_hle_one(hle, w0a, w1a, st, v, a, f);
    /* (SEGMENT and SETLOOP: the microcode keeps the address in DMEM,
       aspmain.c a pointer) */
    if (op == A_SEGMENT || op == A_SETLOOP || op > A_SETLOOP)
        return;
    rsp_vstate(lv, la, lf);
    uint64_t dbytes = 0, sbytes = 0;
    int mx = 0, first = -1;
    for (int i = 0; i < 4096; i++)
        if (hle[i] != rsp_dmem[i] && (i < 0x380 || i >= 0x4C0)) {
            dbytes++;
            if (first < 0)
                first = i;
            int h = i & ~1;
            int d = (int16_t)(rsp_dmem[h] << 8 | rsp_dmem[h + 1]) - (int16_t)(hle[h] << 8 | hle[h + 1]);
            if (d < 0)
                d = -d;
            if (d > mx)
                mx = d;
        }
    const uint8_t *st_post = st_len ? port_addr(w1a) : NULL;
    for (int i = 0; i < st_len; i++)
        sbytes += st[i] != st_post[i];
    int regs = memcmp(v, lv, sizeof v) || memcmp(a, la, sizeof a) || memcmp(f, lf, 3 * sizeof f[0]);
    dg[op][init].n++;
    if (dbytes || sbytes || regs)
        dg[op][init].bad++;
    dg[op][init].dmem_cmds += dbytes != 0;
    dg[op][init].dmem_bytes += dbytes;
    dg[op][init].state_cmds += sbytes != 0;
    dg[op][init].state_bytes += sbytes;
    dg[op][init].reg_cmds += regs;
    if (mx > dg[op][init].max)
        dg[op][init].max = mx;
    if ((dbytes || sbytes || regs) && dg_shown < 40) {
        dg_shown++;
        host_log("audio diag: %u: %s %08X %08X: %llu DMEM bytes differ (first %03X: microcode %02X, aspmain.c %02X), "
                 "%llu of the state's, registers %s\n",
                 host_vi_count, op_name[op], w0, (uint32_t)w1a, (unsigned long long)dbytes, first & 0xFFF,
                 first < 0 ? 0 : rsp_dmem[first], first < 0 ? 0 : hle[first], (unsigned long long)sbytes,
                 regs ? "differ" : "the same");
        if (regs)
            for (int r = 0; r < 32; r++)
                if (memcmp(v[r], lv[r], sizeof v[r]))
                    host_log("audio diag:   v%d: microcode %04X %04X %04X %04X %04X %04X %04X %04X, aspmain.c %04X %04X "
                             "%04X %04X %04X %04X %04X %04X\n",
                             r, lv[r][0], lv[r][1], lv[r][2], lv[r][3], lv[r][4], lv[r][5], lv[r][6], lv[r][7], v[r][0],
                             v[r][1], v[r][2], v[r][3], v[r][4], v[r][5], v[r][6], v[r][7]);
    }
}

/* the task a command at a time (the microcode's dispatch at 0x0E4) */
static int diag_run(const HostGfx *cmd, uint32_t n) {
    static uint8_t pre[4096], st_pre[96];
    static uint16_t v[32][8], a[3][8], f[4];
    static const uint32_t stop[] = { 0x0E4 };
    static int said;
    if (!said) {
        said = 1;
        atexit(diag_exit);
    }
    long total = 0;
    int pc = rsp_run(0x000, stop, 1, 50000000L);
    total += rsp_ran;
    for (uint32_t k = 0; pc == 0x0E4; k++) {
        uint32_t w0 = port_be32(rsp_dmem + (rsp_r[29] & 0xFFF));
        if (k >= n || w0 != (uint32_t)cmd[k].w0) {
            host_log("audio diag: command %u isn't the list's (%08X)\n", k, w0);
            return rsp_run(0x0E4, NULL, 0, 50000000L);
        }
        memcpy(pre, rsp_dmem, sizeof pre);
        rsp_vstate(v, a, f);
        int op = w0 >> 24;
        if (op == A_ADPCM || op == A_RESAMPLE || op == A_POLEF || op == A_ENVMIXER)
            memcpy(st_pre, port_addr(cmd[k].w1), op == A_ENVMIXER ? 80 : 32);
        rsp_run(0x0E4, NULL, 0, 1);
        pc = rsp_run(0x0E8, stop, 1, 50000000L);
        total += rsp_ran + 1;
        diag_cmd(cmd[k].w0, cmd[k].w1, pre, st_pre, v, a, f);
    }
    rsp_ran = total;
    return pc;
}
#endif

void host_asp_lle_reset(void) { memset(asp_dmem, 0, sizeof asp_dmem); }

void host_asp_lle_task(const void *cmdp, uint32_t data_size, const void *bootp, uint32_t boot_size,
                       const void *textp, const void *datap) {
    const HostGfx *cmd = cmdp;
    const uint8_t *boot = bootp, *text = textp, *data = datap;
    static int init;
    if (!init) {
        init = 1;
        const char *e = getenv("PORT_AUDIO_LLE_STATS");
        lle_stats = e && atoi(e) != 0;
        if (lle_stats || host_verbose)
            atexit(stats_exit);
#ifdef PORT_RSP_WATCH
        atexit(watch_exit);
#endif
    }
    uint64_t t0 = lle_stats ? wall_ns() : 0;
    win_top = 0;
    nreg = 0;
    /* the microcode: rspboot's 0xF80 bytes of text, the task's 0x800 of data */
    uint32_t v_text = WIN_BASE + win_alloc(BOOT_TEXT, 0);
    memcpy(win + (v_text - WIN_BASE), text, ASP_TEXT);
    memset(win + (v_text - WIN_BASE) + ASP_TEXT, 0, BOOT_TEXT - ASP_TEXT);
    uint32_t v_data = WIN_BASE + win_alloc(0x800, 0);
    memcpy(win + (v_data - WIN_BASE), data, ASP_DATA);
    memset(win + (v_data - WIN_BASE) + ASP_DATA, 0, 0x800 - ASP_DATA);
#ifdef PORT_RSP_WATCH
    /* (PORT_AUDIO_LLE_DATAFILL=N: what follows the data, byte N instead of 0) */
    if (getenv("PORT_AUDIO_LLE_DATAFILL"))
        memset(win + (v_data - WIN_BASE) + ASP_DATA, atoi(getenv("PORT_AUDIO_LLE_DATAFILL")), 0x800 - ASP_DATA);
#endif
    /* the command list, as the N64's: two words a command, its addresses
       in the DRAM above */
    uint32_t n = data_size / sizeof(HostGfx);
    uint32_t v_list = WIN_BASE + win_alloc(8 * n, 0);
    uint8_t *out = win + (v_list - WIN_BASE);
    uint32_t count = 0;
    for (uint32_t i = 0; i < n; i++) {
        uintptr_t w1a = cmd[i].w1;
        uint32_t w0 = (uint32_t)cmd[i].w0, w1 = (uint32_t)w1a;
        int op = w0 >> 24, flags = (w0 >> 16) & 0xFF;
        switch (op) {
        case A_SETBUFF:
            if (!(flags & 0x08))
                count = w1 & 0xFFFF;
            break;
        case A_ADPCM:
        case A_RESAMPLE:
        case A_POLEF:
            w1 = dram_of(w1a, 32, 0);
            break;
        case A_ENVMIXER:
            w1 = dram_of(w1a, 80, 0);
            break;
        case A_LOADBUFF:
        case A_SAVEBUFF:
            w1 = dram_of(w1a & ~(uintptr_t)7, (count + 7) & ~7u, 0) | (uint32_t)(w1a & 7);
            break;
        case A_SEGMENT:
            w1 = w1a ? dram_of(w1a, 1, 0) : 0;
            break;
        case A_LOADADPCM:
            w1 = dram_of(w1a, w0 & 0xFFFFFF, 1);
            break;
        case A_SETLOOP:
            w1 = dram_of(w1a, 32, 1);
            break;
        }
        port_wbe32(out + 8 * i, w0);
        port_wbe32(out + 8 * i + 4, w1);
    }
    /* the task, as osSpTaskLoad leaves it at the top of DMEM */
#ifdef PORT_RSP_WATCH
    /* (PORT_AUDIO_LLE_GFXDMEM=1: log what the graphics task left at F90,
       and whether a graphics task wrote any of F90-F97 since the last
       audio task) */
    if (getenv("PORT_AUDIO_LLE_GFXDMEM")) {
        int w = 0;
        for (int i = 0xF90; i < 0xF98; i++)
            w |= rsp_watch_written[i];
        host_log("audio lle: %u: DMEM F90 before the task %08X %08X%s\n", host_vi_count, port_be32(rsp_dmem + 0xF90),
                 port_be32(rsp_dmem + 0xF94), w ? ", written by a graphics task" : "");
    }
#endif
    rsp_reset();
    memcpy(rsp_dmem, asp_dmem, sizeof rsp_dmem);
#ifdef PORT_RSP_WATCH
    /* (PORT_AUDIO_LLE_FILL=N[:LO-HI[:T]]: each task (the first T) finds DMEM (LO to HI) filled with byte N,
       to show the sound doesn't depend on what the last task left) */
    if (getenv("PORT_AUDIO_LLE_FILL")) {
        static unsigned tasks;
        unsigned v = 0, lo = 0, hi = 0xFFF, upto = ~0u;
        sscanf(getenv("PORT_AUDIO_LLE_FILL"), "%u:%x-%x:%u", &v, &lo, &hi, &upto);
        if (tasks++ < upto)
            memset(rsp_dmem + lo, (int)v, hi + 1 - lo);
    }
#endif
    rsp_dma = lle_dma;
    rsp_trace = NULL;
    memcpy(rsp_imem, boot, boot_size < 0x1000 ? boot_size : 0x1000);
    uint32_t task[16] = {
        2, 0, 0, boot_size, v_text, ASP_TEXT, v_data, 0x800, 0, 0, 0, 0, v_list, 8 * n, 0, 0,
    };
    for (int i = 0; i < 16; i++)
        port_wbe32(rsp_dmem + 0xFC0 + 4 * i, task[i]);
#ifdef PORT_RSP_WATCH
    rsp_watch_clear();
    memset(rsp_watch_written + 0xFC0, 1, 64);   /* (the task, osSpTaskLoad's) */
#endif
#ifdef PORT_ASP_REF
    ref_begin();
#endif
#ifdef PORT_ASP_DIAG
    int pc = diag_run(cmd, n);
#else
    int pc = rsp_run(0x000, NULL, 0, 50000000L);
#endif
    if (pc != -1)
        host_log("audio lle: the microcode didn't end (pc %d) in a list of %u commands\n", pc, n);
#ifdef PORT_ASP_REF
    ref_end(n);
#endif
    memcpy(asp_dmem, rsp_dmem, sizeof asp_dmem);
#ifdef PORT_RSP_WATCH
    rsp_watch_clear();      /* (for PORT_AUDIO_LLE_GFXDMEM: what the graphics tasks write) */
#endif
    if (lle_stats) {
        st_tasks++;
        st_cmds += n;
        st_insns += (uint64_t)rsp_ran;
        st_ns += wall_ns() - t0;
    }
}
