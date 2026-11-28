/* Host-side internals (port/host). */
#ifndef HOST_H
#define HOST_H

#include <stddef.h>
#include <stdio.h>

#include "port.h"

/* main.c's options */
extern int host_headless;               /* no window, no sound device */
extern int host_max_frames;             /* stop after so many retraces (0: never) */
extern const char *host_screenshot_prefix;  /* --screenshot: PREFIX_<frame>.bmp */
extern int host_screenshot_every;       /* ... every so many retraces (0: at the end only) */
extern const char *host_save_dir;       /* where the EEPROM and pak files go */

/* the ROM: ROM to .z64's byte order, 1 if it is this build's (main.c; ERR,
   if not NULL, says why not); the ROM to load when none is given
   (romfind.c: the player's, asked for once and kept, SAVE_DIR then its
   directory); a message box */
int host_rom_ours(uint8_t *rom, uint32_t size, char *err, size_t errn);
const char *host_find_rom(const char *def, const char **save_dir);
void host_error_box(const char *msg);
extern const char *host_input_script;   /* --input: a scripted controller */
extern const char *host_replay_path;    /* --replay: a digest's pads, by the game's frame */
extern unsigned host_vi_count;          /* retraces so far */

/* video.c */
void host_video_init(void);
void host_video_frame(void);            /* at each retrace: present (and screenshot) */
void host_video_shutdown(void);
int host_quit_requested(void);
uint32_t host_frame_hash(void);         /* FNV-1a of the shown picture */
void host_frame_shot(const char *path); /* the shown picture to a BMP */
/* the window's OpenGL context is current, the OpenGL renderer's (gfx_gl.c
   then has no EGL context of its own) */
int host_video_gl(void);

/* vi.c: the VI's picture */
extern int host_vi_raw;                 /* --vi-raw: the color image as it is, no VI */
/* the picture the VI shows at this retrace, XRGB (*w by *h); with
   --vi-raw the color image (320x240), or NULL while there is none */
const uint32_t *host_vi_picture(int *w, int *h);
/* with --vi-raw: the framebuffer shown (big-endian RGBA5551) */
const uint8_t *host_vi_raw_fb(void);
void host_vi_frame(void);               /* at each retrace (PORT_VI_REF) */
/* the picture to the window (its OpenGL context the renderer's: video.c),
   its drawable dw by dh */
void host_vi_show(int dw, int dh);

/* audio.c */
void host_audio_init(void);
void host_audio_shutdown(void);
void host_digest_close(void);
extern const char *host_wav_path;

/* input.c */
void host_input_init(void);
void host_input_frame(void);            /* once a retrace: the script's next line, SDL's events */

/* gfx.c: the graphics tasks (gfx.h has the renderer's parts) */
int host_gfx_renderer(const char *name);    /* --renderer: 0 if there is none of that name */
int host_gfx_gl(void);                      /* whether the renderer has a GL context (gfx_gl.c) */
void host_gfx_init(void);
void host_gfx_retrace(void);                /* each retrace (main.c) */
/* gfx_soft.c: RDRAM's hidden bits, a byte a halfword (HB_CLEAN: never
   written by the RDP, its hidden bits its low bit twice) */
#define HB_CLEAN 4
const uint8_t *host_gfx_hidden(void);
/* the hidden bits of the halfword at p, wherever the image is (gfx.c) */
uint8_t host_gfx_hb(const uint8_t *p);
uint64_t host_gfx_tris(void);           /* triangles drawn so far */

/* gfxcensus.c: PORT_GFX_CENSUS=FILE, what the graphics tasks use, counted */
extern int host_census_on;
void host_census_init(void);
void host_census(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* fbwatch.c: PORT_FB_WATCH=FILE, the CPU's reads and writes of the color
   and depth images between graphics tasks */
extern int host_fbw_on;
void host_fbw_init(void);
void host_fbw_image(const uint8_t *p, size_t n, int z);    /* an image the RDP drew to */
void host_fbw_task(int begin);          /* a task's start (1) and end (0) */

/* the reset button (main.c; docs/PORT.md, "The reset button"): PRE-NMI
   now, the NMI half a second later, when every part below is reset and
   the game boots again */
void host_reset_button(void);
void host_threads_reset(void);          /* threads.c: every thread's frames dropped */
int port_coro_resettable(void);         /* coro.c */
void port_coro_reset(void);
void port_alloc_reset(void);            /* port/src/memory.c: port_alloc's arena emptied */
void host_vi_reset(void);               /* vi.c */
void host_ai_reset(void);               /* audio.c */
void host_asp_reset(void);              /* aspmain.c */
/* asp_lle.c: --audio lle, the audio microcode on the RSP interpreter */
extern int host_audio_lle;
void host_asp_lle_reset(void);
void host_asp_lle_task(const void *cmd, uint32_t data_size, const void *boot, uint32_t boot_size, const void *text,
                       const void *data);
void host_gfx_reset(uint32_t clean);    /* gfx.c: the hidden bits below clean too */
void host_input_reset(void);            /* input.c: --replay's next part */
int host_replay_seed(unsigned *seed);   /* input.c: --replay's part's `# seed N` */
void port_digest_mark(const char *what);    /* digest.c: `# what VI` */

/* N64 buttons */
enum {
    B_A = 0x8000, B_B = 0x4000, B_Z = 0x2000, B_START = 0x1000, B_DU = 0x0800, B_DD = 0x0400,
    B_DL = 0x0200, B_DR = 0x0100, B_L = 0x0020, B_R = 0x0010, B_CU = 0x0008, B_CD = 0x0004,
    B_CL = 0x0002, B_CR = 0x0001,
};

#endif
