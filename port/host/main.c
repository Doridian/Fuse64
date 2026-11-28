/*
 * The host program: the cartridge and the "interrupt" loop (DozerEncore's
 * port/host/main.c, cut down).
 *
 * The game runs as fibers (threads.c) on the main OS thread.  This loop
 * runs whenever no game thread can: it delivers the events the hardware
 * would raise (the VI retrace at 60 Hz, 50 in eu; timers; SP and DP task
 * completion) and picks the next thread by libultra's priority rules.
 *
 * Time: --deterministic (and headless runs by default) keeps a virtual
 * clock that moves only from one event to the next when every thread
 * waits, so a run is the same on every host and every time; otherwise the
 * host's clock paces the retraces.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "fiber.h"
#include "host.h"

void port_spin_wait(void);

#if defined(VERSION_JP)
#define ROM_DEFAULT "baserom.jp.z64"
#define ROM_CODE "NBMJ"
#define ROM_NAME "Baku Bomberman (Japan)"
#elif defined(VERSION_EU)
#define ROM_DEFAULT "baserom.eu.z64"
#define ROM_CODE "NBMP"
#define ROM_NAME "Bomberman 64 (Europe)"
#else
#define ROM_DEFAULT "baserom.us.z64"
#define ROM_CODE "NBME"
#define ROM_NAME "Bomberman 64 (USA)"
#endif

int host_verbose;
int host_headless;
int host_max_frames;
const char *host_screenshot_prefix;
int host_screenshot_every;
const char *host_save_dir = ".";
const char *host_input_script;
unsigned host_vi_count;
static int deterministic = -1;
static uint8_t *rom;
static uint32_t rom_size;

void host_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

/* host_fatal shows its message in a box too while the ROM is found and
   loaded with a window to come (a package started from its icon has no
   terminal) */
static int fatal_box;

void host_fatal(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "fuse64: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    if (fatal_box) {
        char msg[1024];
        va_start(ap, fmt);
        vsnprintf(msg, sizeof msg, fmt, ap);
        va_end(ap);
        host_error_box(msg);
    }
    host_threads_dump();
    fflush(stderr);
    exit(1);
}

int host_is_deterministic(void) { return deterministic; }

/* ---- the cartridge ---------------------------------------------------------- */

uint32_t host_rom_size(void) { return rom_size; }
const uint8_t *host_rom(void) { return rom; }

void host_rom_read(void *dst, uint32_t addr, uint32_t len) {
    addr &= 0x0FFFFFFF;
    if (host_verbose > 2)
        host_log("rom %06X %X -> %p\n", (unsigned)addr, (unsigned)len, dst);
    if (addr >= rom_size) {
        memset(dst, 0, len);
        return;
    }
    if (addr + len > rom_size) {
        memset((uint8_t *)dst + (rom_size - addr), 0, addr + len - rom_size);
        len = rom_size - addr;
    }
    memcpy(dst, rom + addr, len);
    port_wc_mark(dst, len, 0);
}

/* ROM to .z64's byte order (from .n64's little-endian words or .v64's
   swapped halves); 1 if it is this build's ROM: an N64 ROM, this version's
   game code, and the data this build takes from it (host_rom_fill_check) */
int host_rom_ours(uint8_t *r, uint32_t n, char *err, size_t errn) {
    if (n < 0x1000) {
        if (err)
            snprintf(err, errn, "not a ROM");
        return 0;
    }
    if (r[0] == 0x40 && r[1] == 0x12) {
        for (uint32_t i = 0; i + 3 < n; i += 4) {
            uint8_t a = r[i], b = r[i + 1];
            r[i] = r[i + 3]; r[i + 1] = r[i + 2]; r[i + 2] = b; r[i + 3] = a;
        }
    } else if (r[0] == 0x37 && r[1] == 0x80) {
        for (uint32_t i = 0; i + 1 < n; i += 2) {
            uint8_t a = r[i];
            r[i] = r[i + 1]; r[i + 1] = a;
        }
    }
    if (r[0] != 0x80 || r[1] != 0x37) {
        if (err)
            snprintf(err, errn, "not an N64 ROM");
        return 0;
    }
    if (memcmp(r + 0x3B, ROM_CODE, 4) != 0) {
        if (err)
            snprintf(err, errn, "game code %.4s, this port is built for %s", (const char *)r + 0x3B, ROM_CODE);
        return 0;
    }
    if (!host_rom_fill_check(r, n)) {
        if (err)
            snprintf(err, errn, "this ROM's data isn't what this build was made from (give the %s ROM)", ROM_NAME);
        return 0;
    }
    return 1;
}

static void load_rom(const char *path) {
    char err[160];
    FILE *f = fopen(path, "rb");
    if (!f)
        host_fatal("can't open the ROM %s (give the path of your own %s ROM)", path, ROM_NAME);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0x1000 || n > 64 * 1024 * 1024)
        host_fatal("%s: not a ROM", path);
    rom = malloc((size_t)n);
    if (!rom || fread(rom, 1, (size_t)n, f) != (size_t)n)
        host_fatal("%s: read error", path);
    fclose(f);
    rom_size = (uint32_t)n;
    if (!host_rom_ours(rom, rom_size, err, sizeof err))
        host_fatal("%s: %s", path, err);
    /* the game's data, before anything reads it */
    host_rom_fill(rom, rom_size);
#ifdef PORT_SCATTER_LAYOUT
    port_scatter_copy();
#endif
}

/* ---- time -------------------------------------------------------------------- */

static uint64_t virtual_ns;
static struct timespec t0;

static uint64_t real_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)(t.tv_sec - t0.tv_sec) * 1000000000ull + (uint64_t)(t.tv_nsec - t0.tv_nsec);
}

uint64_t host_now_ns(void) { return deterministic ? virtual_ns : real_ns(); }

/* the CPU's count runs at 46.875 MHz: 3 ticks every 64 ns */
uint64_t host_ticks(void) { return host_now_ns() * 3 / 64; }

/* ---- events raised for the loop ------------------------------------------------ */

static int pending[32];
static int npending;

void host_raise(int event) {
    if (npending < (int)(sizeof pending / sizeof pending[0]))
        pending[npending++] = event;
}

/* ---- the reset button ---------------------------------------------------------------- */

/* The N64's reset button (docs/PORT.md, "The reset button"): the PIF raises
   PRE-NMI at once (OS_EVENT_PRENMI: the game wipes the screen, then
   spins), and the NMI half a second later, which restarts the CPU: the
   boot as after a power-on, but with RDRAM as it was (port/src/reset.c).
   Here the NMI comes from the loop, no thread running: each part of the
   console is reset, then port_boot runs again.  F9, PORT_RESET=N[,N...]
   (at retrace N), --replay's resets and the page's button press it. */
#define NMI_DELAY_NS 500000000ull
static int nmi_pending;
static uint64_t nmi_at;
static int nmi_end;             /* a build without the soft reset: the run ends at the NMI */

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void host_reset_button(void) {
    if (nmi_pending)
        return;
    nmi_pending = 1;
    nmi_at = host_now_ns() + NMI_DELAY_NS;
    if (host_verbose)
        host_log("%u: the reset button\n", host_vi_count);
    port_digest_mark("prenmi");
    host_raise(14);             /* OS_EVENT_PRENMI */
}

/* the game's spin after the reset's wipe (6BBE0.c): until the NMI */
void port_nmi_wait(void) { port_spin_wait(); }

static void soft_reset(void) {
    nmi_pending = 0;
    if (!port_reset_supported() || !port_coro_resettable()) {
        host_log("fuse64: no soft reset in this build (docs/PORT.md, \"The reset button\"): the run ends\n");
        nmi_end = 1;
        return;
    }
    if (host_verbose)
        host_log("%u: the NMI\n", host_vi_count);
    host_gfx_sync();            /* (RDRAM keeps what the RDP drew) */
    npending = 0;
    host_threads_reset();
    port_coro_reset();
    port_alloc_reset();
    port_reset_game();
    host_gfx_reset(0x100400);   /* (what IPL3 writes: port/src/reset.c) */
    host_vi_reset();
    host_ai_reset();
    host_asp_reset();
    host_input_reset();
    port_digest_mark("reset");
    port_boot();
}

/* PORT_RESET=N[,N...]: the button at those retraces */
static const char *reset_at;

static void reset_script(void) {
    for (const char *p = reset_at; p && *p;) {
        char *e;
        unsigned long n = strtoul(p, &e, 0);
        if (e == p)
            break;
        if (n == host_vi_count)
            host_reset_button();
        p = *e == ',' ? e + 1 : e;
    }
}

static void deliver_pending(void) {
    while (npending > 0) {
        int e = pending[0];
        memmove(pending, pending + 1, (size_t)(npending - 1) * sizeof pending[0]);
        npending--;
        port_irq_event(e);
    }
}

/* ---- the program ----------------------------------------------------------------- */

/* the release (CMake's PORT_RELEASE: CI's v* tag), or a build of no release */
#ifndef FUSE64_VERSION
#define FUSE64_VERSION "(no release)"
#endif

static void usage(const char *argv0) {
    fprintf(stderr,
            "Fuse64 " FUSE64_VERSION ", a port of Bomberman 64\n"
            "usage: %s [options] [ROM]   (default ROM: %s; with a window and none there, it asks)\n"
            "  --headless          no window and no sound (implies --deterministic)\n"
            "  --deterministic     a virtual clock: the same run every time\n"
            "  --frames N          stop after N retraces\n"
            "  --screenshot PREFIX write PREFIX_<retrace>.bmp at the end (and with --every N, every N)\n"
            "  --every N           ...\n"
            "  --input FILE        a scripted controller (port/tools/inputs)\n"
            "  --replay FILE       controller 1 from a PORT_DIGEST file, by the game's frame\n"
            "  --save-dir DIR      where the EEPROM and controller pak files go (default ., or the\n"
            "                      player's data directory with the ROM it asked for)\n"
            "  --wav FILE          the sound to a WAV file\n"
            "  --hash              print each retrace's picture hash\n"
            "  --vi-raw            show the color image as it is, without the VI's filters\n"
            "  --renderer NAME     the renderer: soft (the game's microcode on the RSP, the RDP in software:\n"
            "                      exact, the default), hle-sw (the microcode's commands in C, the same RDP),\n"
            "                      gl (hle-sw's commands drawn by OpenGL, exact: docs/PORT.md, \"The OpenGL renderer\"),\n"
            "                      gl-fast (the same with the GPU's rasterizer, depth test and blending: approximate)\n"
            "  --audio NAME        the audio microcode: hle (in C, the default) or lle (the game's own on the\n"
            "                      RSP interpreter: docs/PORT.md, \"Audio\"; also PORT_AUDIO_LLE=1)\n"
            "  -v                  more log\n",
            argv0, ROM_DEFAULT);
    exit(2);
}

static int print_hash;

#if defined(__EMSCRIPTEN__) && defined(PORT_WASM_WEB)
/* the next animation frame, or a timer if none comes (a hidden page has
   none): the browser draws the canvas and takes input in between (Blast
   Corps' main.c, wait_display) */
EM_ASYNC_JS(void, wait_display, (void), {
    return new Promise(function (resolve) {
        var done = false;
        function fin() { if (!done) { done = true; resolve(); } }
        requestAnimationFrame(fin);
        setTimeout(fin, 50);
    });
});
static double last_yield;
#endif

/* PORT_DUMP_RDRAM=N[,N...]:FILE writes the RDRAM buffer (the work area,
   the regions) at retrace N (several: FILE.N each), to compare two hosts'
   or renderers' runs (a debugging aid; a GPU renderer's images synced
   first, as for anything that reads RDRAM) */
extern uint8_t port_rdram[];
static void dump_rdram(void) {
    static int init, many;
    static const char *list, *path;
    if (!init) {
        const char *e = getenv("PORT_DUMP_RDRAM");
        init = 1;
        if (e && strchr(e, ':')) {
            list = e;
            path = strchr(e, ':') + 1;
            many = strchr(e, ',') && strchr(e, ',') < path;
        }
    }
    for (const char *p = list; p;) {
        char *end;
        unsigned long n = strtoul(p, &end, 10);
        if (end == p)
            break;
        if (n == host_vi_count) {
            char name[1100];
            snprintf(name, sizeof name, many ? "%s.%u" : "%s", path, host_vi_count);
            host_gfx_sync();
            FILE *f = fopen(name, "wb");
            if (f) {
                fwrite(port_rdram, 1, 8 << 20, f);
                fclose(f);
            }
            /* (and its hidden bits, a byte a halfword: FILE.hb) */
            strncat(name, ".hb", sizeof name - strlen(name) - 1);
            f = fopen(name, "wb");
            if (f) {
                fwrite(host_gfx_hidden(), 1, PORT_RDRAM_SIZE / 2, f);
                fclose(f);
            }
        }
        if (*end != ',')
            break;
        p = end + 1;
    }
}

#ifdef PORT_AUDIO_RECORD
#include <sys/personality.h>
#endif

int main(int argc, char **argv) {
    const char *rom_path = NULL;
    int save_dir_given = 0;
#ifdef PORT_AUDIO_RECORD
    /* (the libaudio oracle's recorder: the same addresses every run, the
       heap's too, for the replay to map) */
    if (!(personality(0xFFFFFFFF) & ADDR_NO_RANDOMIZE)) {
        personality(ADDR_NO_RANDOMIZE);
        execv("/proc/self/exe", argv);
    }
#endif
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--headless"))
            host_headless = 1;
        else if (!strcmp(a, "--deterministic"))
            deterministic = 1;
        else if (!strcmp(a, "--realtime"))
            deterministic = 0;
        else if (!strcmp(a, "--frames") && i + 1 < argc)
            host_max_frames = atoi(argv[++i]);
        else if (!strcmp(a, "--screenshot") && i + 1 < argc)
            host_screenshot_prefix = argv[++i];
        else if (!strcmp(a, "--every") && i + 1 < argc)
            host_screenshot_every = atoi(argv[++i]);
        else if (!strcmp(a, "--input") && i + 1 < argc)
            host_input_script = argv[++i];
        else if (!strcmp(a, "--replay") && i + 1 < argc) {
            host_replay_path = argv[++i];
            deterministic = 1;
        }
        else if (!strcmp(a, "--save-dir") && i + 1 < argc) {
            host_save_dir = argv[++i];
            save_dir_given = 1;
        }
        else if (!strcmp(a, "--wav") && i + 1 < argc)
            host_wav_path = argv[++i];
        else if (!strcmp(a, "--hash"))
            print_hash = 1;
        else if (!strcmp(a, "--vi-raw"))
            host_vi_raw = 1;
        else if (!strcmp(a, "--renderer") && i + 1 < argc) {
            if (!host_gfx_renderer(argv[++i])) {
                fprintf(stderr, "fuse64: --renderer %s: no such renderer (there are soft, hle-sw and, in a build with OpenGL, gl and gl-fast)\n", argv[i]);
                exit(2);
            }
        }
        else if (!strcmp(a, "--audio") && i + 1 < argc) {
            const char *n = argv[++i];
            if (!strcmp(n, "hle") || !strcmp(n, "lle"))
                host_audio_lle = n[0] == 'l';
            else {
                fprintf(stderr, "fuse64: --audio %s: no such audio path (there are hle and lle)\n", n);
                exit(2);
            }
        }
        else if (!strcmp(a, "-v"))
            host_verbose++;
        else if (a[0] == '-')
            usage(argv[0]);
        else
            rom_path = a;
    }
    if (deterministic < 0)
        deterministic = host_headless;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    fatal_box = !host_headless;
    if (!rom_path && !host_headless)
        rom_path = host_find_rom(ROM_DEFAULT, save_dir_given ? NULL : &host_save_dir);
    load_rom(rom_path ? rom_path : ROM_DEFAULT);
    fatal_box = 0;
    fiber_init();
    host_video_init();
    host_audio_init();
    host_input_init();
    host_gfx_init();
    port_boot();

    const uint64_t vi_period = 1000000000ull / PORT_RETRACE_HZ;
    uint64_t next_vi = host_now_ns() + vi_period;

    reset_at = getenv("PORT_RESET");
    for (;;) {
        deliver_pending();
        uint64_t now = host_now_ns();
        if (nmi_pending && now >= nmi_at) {
            soft_reset();
            if (nmi_end)
                break;
            continue;
        }
        if (now >= next_vi) {
            next_vi += vi_period;
            if (!deterministic && now > next_vi + 4 * vi_period)
                next_vi = now + vi_period;      /* fell behind: don't catch up */
            host_vi_count++;
            host_gfx_retrace();
            dump_rdram();
            host_input_frame();
            host_video_frame();
            reset_script();
            if (print_hash)
                printf("%u %08x\n", host_vi_count, host_frame_hash());
            if (host_quit_requested() || (host_max_frames && (int)host_vi_count >= host_max_frames))
                break;
            port_irq_vi();
#if defined(__EMSCRIPTEN__) && defined(PORT_WASM_WEB)
            /* the page: the browser's turn (input, sound, the picture shown)
               after a retrace when it hasn't had one for 12 ms, even when
               the game is behind and never sleeps, or the tab hangs */
            if (emscripten_get_now() - last_yield >= 12.0) {
                wait_display();
                last_yield = emscripten_get_now();
            }
#endif
            continue;
        }
        uint64_t deadline = port_irq_timers(host_ticks());
        uint64_t wake = next_vi;
        /* (rounded up: at deadline * 64 / 3 the counter may not be there yet) */
        if (deadline != ~0ull && (deadline * 64 + 2) / 3 < wake)
            wake = (deadline * 64 + 2) / 3;
        if (nmi_pending && nmi_at < wake)
            wake = nmi_at;
        if (npending)
            continue;
        if (host_run_one())
            continue;
        if (deterministic) {
            if (wake > virtual_ns)
                virtual_ns = wake;
            continue;
        }
        now = host_now_ns();
        if (wake > now + 50000) {
#ifdef __EMSCRIPTEN__
            emscripten_sleep((unsigned)((wake - now) / 1000000));
#ifdef PORT_WASM_WEB
            last_yield = emscripten_get_now();
#endif
#else
            struct timespec ts = { (time_t)((wake - now) / 1000000000ull), (long)((wake - now) % 1000000000ull) };
            nanosleep(&ts, NULL);
#endif
        }
    }
    if (host_screenshot_prefix && !host_screenshot_every) {
        char path[1024];
        snprintf(path, sizeof path, "%s_%05u.bmp", host_screenshot_prefix, host_vi_count);
        host_frame_shot(path);
    }
    if (host_verbose) {
        host_log("%u retraces, %llu triangles\n", host_vi_count, (unsigned long long)host_gfx_tris());
        host_threads_dump();
    }
    host_audio_shutdown();
    host_video_shutdown();
    host_digest_close();
    /* (and any other file still open, PORT_DUMP_DL's: under node nothing
       flushes them at the exit) */
    fflush(NULL);
    return 0;
}
