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
extern const char *host_input_script;   /* --input: a scripted controller */
extern const char *host_replay_path;    /* --replay: a digest's pads, by the game's frame */
extern int host_replay_over;             /* --replay: a part before a reset played out */
extern unsigned host_vi_count;          /* retraces so far */

/* video.c */
void host_video_init(void);
void host_video_frame(void);            /* at each retrace: present (and screenshot) */
void host_video_shutdown(void);
int host_quit_requested(void);
uint32_t host_frame_hash(void);         /* FNV-1a of the shown picture */
void host_frame_shot(const char *path); /* the shown picture to a BMP */

/* vi.c: the VI's picture */
extern int host_vi_raw;                 /* --vi-raw: the color image as it is, no VI */
/* the picture the VI shows at this retrace, XRGB (*w by *h); with
   --vi-raw the color image (320x240), or NULL while there is none */
const uint32_t *host_vi_picture(int *w, int *h);
/* with --vi-raw: the framebuffer shown (big-endian RGBA5551) */
const uint8_t *host_vi_raw_fb(void);
void host_vi_frame(void);               /* at each retrace (PORT_VI_REF) */

/* audio.c */
void host_audio_init(void);
void host_audio_shutdown(void);
extern const char *host_wav_path;

/* input.c */
void host_input_init(void);
void host_input_frame(void);            /* once a retrace: the script's next line, SDL's events */

/* gfx.c */
void host_gfx_init(void);
/* RDRAM's hidden bits, a byte a halfword (HB_CLEAN: never written by the
   RDP, its hidden bits its low bit twice) */
#define HB_CLEAN 4
const uint8_t *host_gfx_hidden(void);
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

/* N64 buttons */
enum {
    B_A = 0x8000, B_B = 0x4000, B_Z = 0x2000, B_START = 0x1000, B_DU = 0x0800, B_DD = 0x0400,
    B_DL = 0x0200, B_DR = 0x0100, B_L = 0x0020, B_R = 0x0010, B_CU = 0x0008, B_CD = 0x0004,
    B_CL = 0x0002, B_CR = 0x0001,
};

#endif
