/*
 * The controllers: a script (--input FILE, tools/inputs' format, which
 * tools/shift_check.py plays in mupen64plus: `frame BUTTONS` lines, the
 * word held from that retrace on, in mupen64plus's BUTTONS layout), and
 * with a window the keyboard and SDL's game controllers (video.c).
 *
 * --replay FILE (implies --deterministic) gives controller 1 a digest's
 * pads by the game's frame instead (docs/PORT.md, "The gameplay digest"):
 * FILE is a PORT_DIGEST file, the port's or port/tools/emu_digest.py's
 * from mupen64plus, and each read gets the pad of the digest's read at the
 * same frame (the k-th read of a frame the digest's k-th, or its last),
 * none where the digest has no read of that frame.  The port loads
 * without the N64's waits, so the same retrace is another frame there;
 * the same frame is the same moment of the game.  A movie's digest has a
 * part per soft reset (`# reset VI` lines): the reset button (`# prenmi
 * VI`, some frames before the part's end) is pressed (main.c's
 * host_reset_button) at the read of the last frame read before it, and
 * after the NMI the next part's pads go with the new boot's frames.
 * PORT_REPLAY_PART=N starts at the N-th part (from 0) instead, on a
 * power-on (with the save the part before wrote: a cold boot where the
 * movie had a soft reset).
 *
 * PORT_MONKEY=SEED[:FROM[:START]] drives controller 1 at random from
 * retrace FROM (default 0; before it the script, if any): the stick held
 * in one of 16 directions (or let go) for 8 to 90 retraces at a time,
 * with A tapped and held (bombs, carrying one), B (kicks, throws), Z,
 * R, L and the C buttons pressed now and then; START too if START is 1
 * (the pause menu, which can leave the stage: off by default).  The
 * generator is stepped a fixed number of times every retrace, in integer
 * arithmetic only, so a seed gives the same run on every host
 * (docs/PORT.md, "Every stage").  With --replay, the replay owns
 * controller 1 for as long as it has frames; past its last frame the
 * monkey takes it, from retrace FROM on.
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

const char *host_replay_path;

/* a part of the digest: from the power-on or a reset to the next reset */
typedef struct {
    uint32_t *pads;                     /* the digest's pads, in its order */
    unsigned *vis;                      /* and the retrace of each read */
    int *first, *count;                 /* by frame: where its reads start, how many */
    unsigned frames;
    long prenmi;                        /* the button came after this frame's read (-1: before
                                           any; -2: no button in this part) */
} Part;

static Part *parts;
static int nparts, part;
static unsigned replay_frames;          /* parts[part]'s */
static unsigned replay_frame = ~0u;     /* the last read's frame, and its reads so far */
static int replay_k;
static long replay_prenmi = -2;
static int replay_ended;                /* past the last part's frames: the replay is done */
static int lag_have_offset;             /* PORT_REPLAY_LAG's offset, per part */
static long lag_offset;

static void part_begin(int k) {
    part = k;
    replay_frames = parts[k].frames;
    replay_prenmi = parts[k].prenmi;
    replay_frame = ~0u;
    replay_k = 0;
    lag_have_offset = 0;
}

static void replay_init(void) {
    FILE *f = fopen(host_replay_path, "r");
    char line[256];
    int n = 0, cap = 0, fcap = 0;
    unsigned *frames = NULL;
    if (!f)
        host_fatal("--replay %s: can't open it", host_replay_path);
    const char *e = getenv("PORT_REPLAY_PART");
    int want = e && *e ? atoi(e) : 0;
    Part *p = NULL;
    /* POLL VI SCENE SUB FRAME PAD ... (lines can be long: only the head);
       `# reset VI` ends a part (emu_digest.py --movie's soft resets) */
    for (int more = 1; more;) {
        unsigned poll, vi, fr;
        int sc, sub;
        unsigned long pad;
        more = fgets(line, sizeof line, f) != NULL;
        if (!p) {
            parts = realloc(parts, (size_t)(nparts + 1) * sizeof *parts);
            p = &parts[nparts++];
            memset(p, 0, sizeof *p);
            p->prenmi = -2;
            n = cap = 0;
        }
        if (!more || !strncmp(line, "# reset", 7)) {
            /* the part's reads by frame */
            p->first = calloc(p->frames + 1, sizeof *p->first);
            p->count = calloc(p->frames + 1, sizeof *p->count);
            for (int i = n - 1; i >= 0; i--) {
                p->first[frames[i]] = i;
                p->count[frames[i]]++;
            }
            p = NULL;
            continue;
        }
        size_t l = strlen(line);
        int whole = l && line[l - 1] == '\n';
        if (!strncmp(line, "# prenmi", 8)) {
            p->prenmi = n ? (long)frames[n - 1] : -1;
        } else if (sscanf(line, "%u %u %d %d %u %lx", &poll, &vi, &sc, &sub, &fr, &pad) == 6) {
            if (n == cap) {
                cap = cap ? cap * 2 : 4096;
                p->pads = realloc(p->pads, (size_t)cap * sizeof *p->pads);
                p->vis = realloc(p->vis, (size_t)cap * sizeof *p->vis);
            }
            if (n == fcap) {
                fcap = fcap ? fcap * 2 : 4096;
                frames = realloc(frames, (size_t)fcap * sizeof *frames);
            }
            p->pads[n] = (uint32_t)pad;
            p->vis[n] = vi;
            frames[n++] = fr;
            if (fr + 1 > p->frames)
                p->frames = fr + 1;
        }
        while (!whole && fgets(line, sizeof line, f)) {
            l = strlen(line);
            whole = l && line[l - 1] == '\n';
        }
    }
    fclose(f);
    free(frames);
    if (want < 0 || want >= nparts)
        host_fatal("--replay %s: no part %d (PORT_REPLAY_PART)", host_replay_path, want);
    part_begin(want);
    have_script = 1;
}

/* the NMI (main.c): the next part's pads for the new boot's frames */
void host_input_reset(void) {
    if (!host_replay_path || replay_ended)
        return;
    if (part + 1 < nparts)
        part_begin(part + 1);
    else
        replay_ended = 1;       /* (a reset the digest doesn't have: the replay is over) */
}

/* PORT_REPLAY_LAG=1 (with --replay): the game's thread at a read of the
   pads waits (spinning: port_spin_wait, the interrupts and the threads of
   higher priority go on) until as many retraces have passed since the
   first read as had in the digest at the same read, so that the port lags
   as the digest's run did.  The audio thread then makes as much sound
   between two frames as the N64's did (docs/PORT.md, "Audio"). */
void port_spin_wait(void);

void host_replay_wait(unsigned frame) {
    static int on = -1;
    if (on < 0) {
        const char *e = getenv("PORT_REPLAY_LAG");
        on = host_replay_path && e && *e && *e != '0';
    }
    const Part *p = parts ? &parts[part] : NULL;
    if (!on || replay_ended || frame >= replay_frames || !p->count[frame])
        return;
    int k = frame == replay_frame ? replay_k + 1 : 0;
    if (k >= p->count[frame])
        k = p->count[frame] - 1;
    unsigned vi = p->vis[p->first[frame] + k];
    if (!lag_have_offset) {
        lag_offset = (long)vi - (long)host_vi_count;
        lag_have_offset = 1;
    }
    while ((long)host_vi_count < (long)vi - lag_offset)
        port_spin_wait();
}

/* --replay: the pad word of this read of the frame */
static void replay_poll(unsigned frame) {
    const Part *p = &parts[part];
    script_word = 0;
    if (replay_ended)
        return;
    if (frame != replay_frame) {
        replay_frame = frame;
        /* the last part's frames done: the replay is over */
        if (frame >= replay_frames && part + 1 == nparts)
            replay_ended = 1;
        /* the reset button, which the game hears before the NMI */
        if (replay_prenmi >= -1 && (long)frame >= replay_prenmi) {
            replay_prenmi = -2;
            host_reset_button();
        }
        replay_k = 0;
    } else
        replay_k++;
    if (frame < replay_frames && p->count[frame]) {
        int k = replay_k < p->count[frame] ? replay_k : p->count[frame] - 1;
        script_word = p->pads[p->first[frame] + k];
    }
}

static struct {
    int on, start;
    unsigned from;
    uint64_t s;
    unsigned seg_left, hold_a, hold_b, hold_o;
    uint16_t other;
    int8_t x, y;
    uint16_t word;
} monkey;

/* xorshift64*: a 32-bit number */
static uint32_t monkey_next(void) {
    monkey.s ^= monkey.s >> 12;
    monkey.s ^= monkey.s << 25;
    monkey.s ^= monkey.s >> 27;
    return (uint32_t)((monkey.s * 0x2545F4914F6CDD1Dull) >> 32);
}

/* 0 to n-1 */
static unsigned monkey_below(unsigned n) { return (unsigned)(((uint64_t)monkey_next() * n) >> 32); }

static void monkey_init(void) {
    const char *e = getenv("PORT_MONKEY");
    if (!e || !*e)
        return;
    char *end;
    uint64_t seed = strtoull(e, &end, 0);
    if (*end == ':') {
        monkey.from = (unsigned)strtoul(end + 1, &end, 0);
        if (*end == ':')
            monkey.start = atoi(end + 1);
    }
    monkey.on = 1;
    /* splitmix64 of the seed, never 0 */
    uint64_t z = seed + 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    monkey.s = (z ^ (z >> 31)) | 1;
}

/* one retrace: always the same number of draws */
static void monkey_frame(void) {
    /* 16 directions at full tilt (80 * cos, 80 * sin) */
    static const int8_t dir[16][2] = {
        { 80, 0 }, { 74, 31 }, { 57, 57 }, { 31, 74 }, { 0, 80 }, { -31, 74 }, { -57, 57 }, { -74, 31 },
        { -80, 0 }, { -74, -31 }, { -57, -57 }, { -31, -74 }, { 0, -80 }, { 31, -74 }, { 57, -57 }, { 74, -31 },
    };
    static const uint16_t others[] = { B_Z, B_R, B_L, B_CU, B_CD, B_CL, B_CR, B_START };
    uint32_t r0 = monkey_next(), r1 = monkey_next(), r2 = monkey_next(), r3 = monkey_next();
    if (monkey.seg_left == 0) {
        unsigned d = r0 % 16, kind = (r0 >> 4) % 10;
        monkey.seg_left = 8 + (r0 >> 8) % 83;
        if (kind == 0) {        /* let go */
            monkey.x = monkey.y = 0;
        } else if (kind == 1) { /* half tilt: a walk */
            monkey.x = (int8_t)(dir[d][0] / 2);
            monkey.y = (int8_t)(dir[d][1] / 2);
        } else {
            monkey.x = dir[d][0];
            monkey.y = dir[d][1];
        }
    }
    monkey.seg_left--;
    /* A: a tap (a bomb) about every 40 retraces, a long hold every 300 */
    if (monkey.hold_a)
        monkey.hold_a--;
    else if (r1 % 300 == 0)
        monkey.hold_a = 30 + (r1 >> 16) % 61;
    else if (r1 % 40 == 1)
        monkey.hold_a = 2 + (r1 >> 16) % 5;
    /* B: a tap about every 50 */
    if (monkey.hold_b)
        monkey.hold_b--;
    else if (r2 % 50 == 0)
        monkey.hold_b = 2 + (r2 >> 16) % 8;
    /* the rest: one of them about every 120 */
    if (monkey.hold_o)
        monkey.hold_o--;
    else {
        monkey.other = 0;
        if (r3 % 120 == 0) {
            uint16_t b = others[(r3 >> 8) % (monkey.start ? 8 : 7)];
            monkey.other = b;
            monkey.hold_o = 2 + (r3 >> 16) % 20;
        }
    }
    monkey.word = (uint16_t)((monkey.hold_a ? B_A : 0) | (monkey.hold_b ? B_B : 0) | (monkey.hold_o ? monkey.other : 0));
}

/* the live pads' state (video.c fills them from SDL) */
uint16_t host_live_buttons[4];
int8_t host_live_x[4], host_live_y[4];
int host_live_present[4] = { 1, 0, 0, 0 };

void host_input_init(void) {
    monkey_init();
    if (host_replay_path) {
        replay_init();
        return;
    }
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
    if (!host_replay_path)
        while (at < nsteps && steps[at].frame <= host_vi_count)
            script_word = steps[at++].word;
    if (monkey.on && host_vi_count >= monkey.from)
        monkey_frame();
}

/* --replay still has the pad: until a read's frame is past the digest's
   last (then the replay has ended, for good) */
static int replay_owns(void) {
    return host_replay_path && !replay_ended;
}

/* mupen64plus's BUTTONS bits and the N64's */
static const struct {
    uint32_t m;
    uint16_t n;
} m64p_map[] = {
    { 0x0001, B_DR }, { 0x0002, B_DL }, { 0x0004, B_DD }, { 0x0008, B_DU }, { 0x0010, B_START },
    { 0x0020, B_Z }, { 0x0040, B_B }, { 0x0080, B_A }, { 0x0100, B_CR }, { 0x0200, B_CL },
    { 0x0400, B_CD }, { 0x0800, B_CU }, { 0x1000, B_R }, { 0x2000, B_L },
};

/* mupen64plus's BUTTONS word to the N64's */
static void from_m64p(uint32_t w, uint16_t *b, int8_t *x, int8_t *y) {
    uint16_t r = 0;
    for (unsigned i = 0; i < sizeof m64p_map / sizeof m64p_map[0]; i++)
        if (w & m64p_map[i].m)
            r |= m64p_map[i].n;
    *b = r;
    *x = (int8_t)(w >> 16);
    *y = (int8_t)(w >> 24);
}

/* the monkey has controller 1 at this retrace */
static int monkey_has_pad(void) {
    return monkey.on && host_vi_count >= monkey.from && host_vi_count > 0 && !replay_owns();
}

/* the N64's buttons and stick to mupen64plus's BUTTONS word */
static uint32_t to_m64p(uint16_t b, int8_t x, int8_t y) {
    uint32_t r = 0;
    for (unsigned i = 0; i < sizeof m64p_map / sizeof m64p_map[0]; i++)
        if (b & m64p_map[i].n)
            r |= m64p_map[i].m;
    return r | (uint32_t)(uint8_t)x << 16 | (uint32_t)(uint8_t)y << 24;
}

/* the read of a frame (osContStartReadData, through port/host/digest.c):
   the pad word controller 1 gets, in mupen64plus's layout (the digest's
   PAD), which --replay picks by the frame, or the monkey's once it has
   the pad */
uint32_t host_input_poll(unsigned frame) {
    if (host_replay_path)
        replay_poll(frame);
    if (monkey_has_pad())
        return to_m64p(monkey.word, monkey.x, monkey.y);
    return script_word;
}

int host_input(int pad, uint16_t *buttons, int8_t *x, int8_t *y) {
    *buttons = 0;
    *x = *y = 0;
    if (pad < 0 || pad > 3)
        return 0;
    if (pad == 0 && monkey_has_pad()) {
        *buttons = monkey.word;
        *x = monkey.x;
        *y = monkey.y;
        return 1;
    }
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
