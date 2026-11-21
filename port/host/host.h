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
extern unsigned host_vi_count;          /* retraces so far */

/* video.c */
void host_video_init(void);
void host_video_frame(void);            /* at each retrace: present (and screenshot) */
void host_video_shutdown(void);
int host_quit_requested(void);
uint32_t host_frame_hash(void);         /* FNV-1a of the shown framebuffer */
void host_frame_shot(const char *path); /* the shown framebuffer to a BMP */

/* audio.c */
void host_audio_init(void);
void host_audio_shutdown(void);
extern const char *host_wav_path;

/* input.c */
void host_input_init(void);
void host_input_frame(void);            /* once a retrace: the script's next line, SDL's events */

/* gfx.c */
void host_gfx_init(void);
uint64_t host_gfx_tris(void);           /* triangles drawn so far */

/* N64 buttons */
enum {
    B_A = 0x8000, B_B = 0x4000, B_Z = 0x2000, B_START = 0x1000, B_DU = 0x0800, B_DD = 0x0400,
    B_DL = 0x0200, B_DR = 0x0100, B_L = 0x0020, B_R = 0x0010, B_CU = 0x0008, B_CD = 0x0004,
    B_CL = 0x0002, B_CR = 0x0001,
};

#endif
