/*
 * The controllers: a script (--input FILE, tools/inputs' format, which
 * tools/shift_check.py plays in mupen64plus: `frame BUTTONS` lines, the
 * word held from that retrace on, in mupen64plus's BUTTONS layout), and
 * with a window the keyboard and SDL's game controllers (video.c).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

typedef struct {
    unsigned frame;
    uint32_t word;
} Step;

static Step *steps;
static int nsteps, at;
static uint32_t script_word;
static int have_script;

/* the live pads' state (video.c fills them from SDL) */
uint16_t host_live_buttons[4];
int8_t host_live_x[4], host_live_y[4];
int host_live_present[4] = { 1, 0, 0, 0 };

void host_input_init(void) {
    if (!host_input_script)
        return;
    FILE *f = fopen(host_input_script, "r");
    if (!f)
        host_fatal("--input %s: can't open it", host_input_script);
    char line[256];
    int cap = 0;
    while (fgets(line, sizeof line, f)) {
        unsigned fr;
        unsigned long w;
        if (line[0] == '#' || sscanf(line, "%u %lx", &fr, &w) != 2)
            continue;
        if (nsteps == cap) {
            cap = cap ? cap * 2 : 256;
            steps = realloc(steps, (size_t)cap * sizeof *steps);
        }
        steps[nsteps].frame = fr;
        steps[nsteps].word = (uint32_t)w;
        nsteps++;
    }
    fclose(f);
    have_script = 1;
}

void host_input_frame(void) {
    while (at < nsteps && steps[at].frame <= host_vi_count)
        script_word = steps[at++].word;
}

/* mupen64plus's BUTTONS word to the N64's */
static void from_m64p(uint32_t w, uint16_t *b, int8_t *x, int8_t *y) {
    static const struct { uint32_t m; uint16_t n; } map[] = {
        { 0x0001, B_DR }, { 0x0002, B_DL }, { 0x0004, B_DD }, { 0x0008, B_DU }, { 0x0010, B_START },
        { 0x0020, B_Z }, { 0x0040, B_B }, { 0x0080, B_A }, { 0x0100, B_CR }, { 0x0200, B_CL },
        { 0x0400, B_CD }, { 0x0800, B_CU }, { 0x1000, B_R }, { 0x2000, B_L },
    };
    uint16_t r = 0;
    for (unsigned i = 0; i < sizeof map / sizeof map[0]; i++)
        if (w & map[i].m)
            r |= map[i].n;
    *b = r;
    *x = (int8_t)(w >> 16);
    *y = (int8_t)(w >> 24);
}

int host_input(int pad, uint16_t *buttons, int8_t *x, int8_t *y) {
    *buttons = 0;
    *x = *y = 0;
    if (pad < 0 || pad > 3)
        return 0;
    if (have_script) {
        if (pad != 0)
            return 0;
        from_m64p(script_word, buttons, x, y);
        return 1;
    }
    if (!host_live_present[pad])
        return 0;
    *buttons = host_live_buttons[pad];
    *x = host_live_x[pad];
    *y = host_live_y[pad];
    return 1;
}
