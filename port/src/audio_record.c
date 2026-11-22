/*
 * The libaudio oracle's recorder, N64 side (docs/PORT.md, "The libaudio
 * oracle"; DozerEncore's, for this game): built with -DPORT_AUDIO_RECORD=ON
 * only, with the original library (PORT_LIBAUDIO_ORIGINAL).
 *
 * Every stub of gen/zerojump.c that reaches libaudio calls the function
 * of the same name here instead (port/include/audio_record.h), which tells
 * port/host/audiolog.c about the call: its arguments, the structures it
 * hands over or gets back, its result.  The library's calls to itself
 * don't go through a stub.  Where the library calls back into the game,
 * the DMA routine (alInit's config) and the sequence player's oscillators
 * (alCSPNew's), it goes through the trampolines below, which log the
 * game's side of it.  The host keeps a copy of the
 * memory the library and the game share and logs what the game changed in
 * it each time control passes from the game to the library, so that
 * port/tools/audio_oracle can replay the library's whole input.
 */
#ifdef PORT_AUDIO_RECORD
#include <stddef.h>
#include <stdint.h>
#include "audio_log.h"

/* (the library's own declarations are another header's: these are the
   stubs', by the functions' names) */
#define REAL(ret, name, ...) extern ret real_##name(__VA_ARGS__) __asm__(#name)

typedef void *(*DmaNew)(void *state);
typedef intptr_t (*DmaProc)(int32_t addr, int32_t len, void *state);

extern unsigned char D_8001C4B0;
void host_fatal(const char *fmt, ...);
int host_alog_track_name(const char *name);
void *host_alog_symbol(const char *name, uint32_t *size);
void alog_rsp_memory(void (*untrack)(const void *p, uint32_t len));     /* port/libaudio/orig/shim.c */

/* a variable of the game's, by name */
static void *sym(const char *name) {
    void *p = host_alog_symbol(name, NULL);
    if (p == NULL)
        host_fatal("audio_record: no symbol %s", name);
    return p;
}

/* an address as a word of the log (the recorder is an i386 build) */
#define A(p) ((uint32_t)(uintptr_t)(p))

static uint32_t f2u(float f) {
    union { float f; uint32_t u; } x;
    x.f = f;
    return x.u;
}

/* the game's side of the library's callback */
static DmaNew game_dmanew;
static DmaProc game_dma;

#define CALL(fn, a0, a1, a2, a3, a4) host_alog_call(ALOG_##fn, a0, a1, a2, a3, a4, 0)

static intptr_t rec_dma(int32_t addr, int32_t len, void *state) {
    intptr_t r;

    host_alog_cb_enter(ALOG_CB_DMA, (uint32_t)addr, (uint32_t)len, A(state));
    r = game_dma(addr, len, state);
    host_alog_cb_leave((uint32_t)r, 0, 0);
    return r;
}

static void *rec_dmanew(void *state) {
    void *p;

    host_alog_cb_enter(ALOG_CB_DMANEW, A(state), 0, 0);
    p = game_dmanew(state);
    if (game_dma != NULL && game_dma != (DmaProc)p)
        host_fatal("audio_record: a second DMA routine");
    game_dma = (DmaProc)p;
    host_alog_cb_leave(A(p), *(uint32_t *)state, 0);
    return (void *)rec_dma;
}

/* ---- the heap and the files ---------------------------------------------- */

/* what the library and the game share besides the heap: the game's audio
   objects (85A10.c's audio manager and DMA buffers' list, 80580.c's
   players, configs, sequences and markers), by name (the symbols' sizes) */
static const char *const shared[] = {
    NULL
};

REAL(void, alHeapInit, void *, void *, int);
void rec_alHeapInit(void *hp, void *base, int len) {
    if (!host_alog_started()) {
        int i;
        host_alog_track(base, (uint32_t)len);
        host_alog_track(hp, 16);
        host_alog_track(&D_8001C4B0, 1);
        for (i = 0; shared[i] != NULL; i++)
            if (!host_alog_track_name(shared[i]))
                host_fatal("audio_record: no symbol %s", shared[i]);
        host_alog_start();
        host_alog_var(ALOG_VAR_MONO, &D_8001C4B0);
    }
    CALL(alHeapInit, A(hp), A(base), (uint32_t)len, 0, 0);
    real_alHeapInit(hp, base, len);
    host_alog_ret(0);
}

REAL(void *, alHeapDBAlloc, void *, int, void *, int, int);
void *rec_alHeapDBAlloc(void *file, int line, void *hp, int num, int size) {
    void *r;
    CALL(alHeapDBAlloc, A(file), (uint32_t)line, A(hp), (uint32_t)num, (uint32_t)size);
    r = real_alHeapDBAlloc(file, line, hp, num, size);
    host_alog_ret(A(r));
    return r;
}

/* ---- the driver ------------------------------------------------------------- */

REAL(void, alInit, void *, void *);
void rec_alInit(void *g, void *c) {
    uint8_t *sc = c;
    int32_t *params = *(int32_t **)(sc + 0x20);

    CALL(alInit, A(g), A(c), 0, 0, 0);
    host_alog_in(c, ALOG_SIZEOF_ALSYNCONFIG);
    if (sc[0x1C] == 6 /* AL_FX_CUSTOM */ && params != NULL)
        host_alog_in(params, (2 + 8 * params[0]) * 4);
    game_dmanew = *(DmaNew *)(sc + 0x10);
    *(DmaNew *)(sc + 0x10) = rec_dmanew;
    real_alInit(g, c);
    *(DmaNew *)(sc + 0x10) = game_dmanew;
    /* (what only the RSP writes and the library never reads: the reverb's
       delay lines) */
    alog_rsp_memory(host_alog_untrack);
    host_alog_ret(0);
}

/* what only the RSP and the PI write and the library never reads (it hands
   their addresses on): 85A10.c's output buffers (D_802AD6A0's audioInfo[3],
   D_802AF950 samples each) and DMA buffers (D_802AF930's lists, D_802AF944
   bytes each) */
static void untrack_buffers(void) {
    uint8_t *am = sym("D_802AD6A0");
    uint32_t frame = *(uint32_t *)sym("D_802AF950"), dmalen = *(int32_t *)sym("D_802AF944");
    uint8_t **lists = sym("D_802AF930");
    int i;

    for (i = 0; i < 3; i++) {
        uint8_t *info = ((uint8_t **)(am + 8))[i];
        host_alog_untrack(*(void **)info, frame * 4);
    }
    for (i = 0; i < 2; i++) {
        uint8_t *b;
        for (b = lists[i]; b != NULL; b = *(uint8_t **)b)
            host_alog_untrack(*(void **)(b + 0x10), dmalen);
    }
}

REAL(void *, alAudioFrame, void *, void *, void *, int);
void *rec_alAudioFrame(void *cmdList, void *cmdLen, void *outBuf, int outLen) {
    static int untracked;
    void *r;
    if (!untracked) {
        untracked = 1;
        untrack_buffers();
    }
    CALL(alAudioFrame, A(cmdList), A(cmdLen), A(outBuf), (uint32_t)outLen, 0);
    r = real_alAudioFrame(cmdList, cmdLen, outBuf, outLen);
    host_alog_out(cmdLen, 4);
    host_alog_acmd(cmdList, (uint32_t)((uint8_t *)r - (uint8_t *)cmdList));
    host_alog_ret(A(r));
    return r;
}

REAL(void, alCSeqSetLoc, void *, void *);
void rec_alCSeqSetLoc(void *seq, void *m) {
    CALL(alCSeqSetLoc, A(seq), A(m), 0, 0, 0);
    host_alog_in(m, ALOG_SIZEOF_ALCSEQMARKER);
    real_alCSeqSetLoc(seq, m);
    host_alog_ret(0);
}

REAL(void, alCSeqGetLoc, void *, void *);
void rec_alCSeqGetLoc(void *seq, void *m) {
    CALL(alCSeqGetLoc, A(seq), A(m), 0, 0, 0);
    real_alCSeqGetLoc(seq, m);
    host_alog_out(m, ALOG_SIZEOF_ALCSEQMARKER);
    host_alog_ret(0);
}

REAL(float, alCents2Ratio, int);
float rec_alCents2Ratio(int cents) {
    float r;
    CALL(alCents2Ratio, (uint32_t)cents, 0, 0, 0, 0);
    r = real_alCents2Ratio(cents);
    host_alog_ret(f2u(r));
    return r;
}

/* the oscillators (ALSeqpConfig's initOsc, updateOsc, stopOsc): the
   game's, one set for every player */
typedef int32_t (*InitOsc)(void **state, float *value, uint8_t type, uint8_t rate, uint8_t depth,
                           uint8_t delay);
typedef int32_t (*UpdateOsc)(void *state, float *value);
typedef void (*StopOsc)(void *state);
static InitOsc game_initosc;
static UpdateOsc game_updateosc;
static StopOsc game_stoposc;

static int32_t rec_initosc(void **state, float *value, uint8_t type, uint8_t rate, uint8_t depth,
                           uint8_t delay) {
    int32_t r;

    host_alog_cb_enter(ALOG_CB_INITOSC, type | rate << 8 | depth << 16 | (uint32_t)delay << 24, 0, 0);
    r = game_initosc(state, value, type, rate, depth, delay);
    host_alog_cb_leave((uint32_t)r, A(*state), f2u(*value));
    return r;
}

static int32_t rec_updateosc(void *state, float *value) {
    int32_t r;

    host_alog_cb_enter(ALOG_CB_UPDATEOSC, A(state), 0, 0);
    r = game_updateosc(state, value);
    host_alog_cb_leave((uint32_t)r, f2u(*value), 0);
    return r;
}

static void rec_stoposc(void *state) {
    host_alog_cb_enter(ALOG_CB_STOPOSC, A(state), 0, 0);
    game_stoposc(state);
    host_alog_cb_leave(0, 0, 0);
}

REAL(void, alCSPNew, void *, void *);
void rec_alCSPNew(void *p, void *c) {
    void **osc = (void **)((char *)c + 0x10);
    void *was[3];

    CALL(alCSPNew, A(p), A(c), 0, 0, 0);
    host_alog_in(c, ALOG_SIZEOF_ALSEQPCONFIG);
    was[0] = osc[0];
    was[1] = osc[1];
    was[2] = osc[2];
    if ((game_initosc != NULL && (void *)game_initosc != was[0]) ||
        (game_updateosc != NULL && (void *)game_updateosc != was[1]) ||
        (game_stoposc != NULL && (void *)game_stoposc != was[2]))
        host_fatal("audio_record: a second set of oscillators");
    game_initosc = (InitOsc)was[0];
    game_updateosc = (UpdateOsc)was[1];
    game_stoposc = (StopOsc)was[2];
    if (was[0] != NULL)
        osc[0] = (void *)rec_initosc;
    if (was[1] != NULL)
        osc[1] = (void *)rec_updateosc;
    if (was[2] != NULL)
        osc[2] = (void *)rec_stoposc;
    real_alCSPNew(p, c);
    osc[0] = was[0];
    osc[1] = was[1];
    osc[2] = was[2];
    host_alog_ret(0);
}

REAL(void, alSndpNew, void *, void *);
void rec_alSndpNew(void *p, void *c) {
    CALL(alSndpNew, A(p), A(c), 0, 0, 0);
    host_alog_in(c, ALOG_SIZEOF_ALSNDPCONFIG);
    real_alSndpNew(p, c);
    host_alog_ret(0);
}

/* ---- the rest: arguments and a result ------------------------------------- */

#define V_P(name)                                                            \
    REAL(void, name, void *);                                                \
    void rec_##name(void *a) {                                               \
        CALL(name, A(a), 0, 0, 0, 0);                                        \
        real_##name(a);                                                      \
        host_alog_ret(0);                                                    \
    }
#define V_PP(name)                                                           \
    REAL(void, name, void *, void *);                                        \
    void rec_##name(void *a, void *b) {                                      \
        CALL(name, A(a), A(b), 0, 0, 0);                                     \
        real_##name(a, b);                                                   \
        host_alog_ret(0);                                                    \
    }
#define V_PX(name, T)                                                        \
    REAL(void, name, void *, T);                                             \
    void rec_##name(void *a, T b) {                                          \
        CALL(name, A(a), (uint32_t)(int32_t)b, 0, 0, 0);                     \
        real_##name(a, b);                                                   \
        host_alog_ret(0);                                                    \
    }
#define V_PXY(name, T, U)                                                    \
    REAL(void, name, void *, T, U);                                          \
    void rec_##name(void *a, T b, U c) {                                     \
        CALL(name, A(a), (uint32_t)(int32_t)b, (uint32_t)(int32_t)c, 0, 0);  \
        real_##name(a, b, c);                                                \
        host_alog_ret(0);                                                    \
    }
#define R_P(R, name)                                                         \
    REAL(R, name, void *);                                                   \
    R rec_##name(void *a) {                                                  \
        R r;                                                                 \
        CALL(name, A(a), 0, 0, 0, 0);                                        \
        r = real_##name(a);                                                  \
        host_alog_ret((uint32_t)(int32_t)r);                                 \
        return r;                                                            \
    }
#define R_PX(R, name, T)                                                     \
    REAL(R, name, void *, T);                                                \
    R rec_##name(void *a, T b) {                                             \
        R r;                                                                 \
        CALL(name, A(a), (uint32_t)(int32_t)b, 0, 0, 0);                     \
        r = real_##name(a, b);                                               \
        host_alog_ret((uint32_t)(int32_t)r);                                 \
        return r;                                                            \
    }

V_P(alUnlink)
V_PP(alLink)
V_PP(alBnkfNew)
V_PP(alSeqFileNew)
V_P(alClose)
V_PP(alCSeqNew)
V_P(alCSPDelete)
V_PP(alCSPSetSeq)
V_P(alCSPPlay)
V_P(alCSPStop)
R_P(int, alCSPGetState)
V_PP(alCSPSetBank)
V_PX(alCSPSetTempo, int)
R_P(int, alCSPGetTempo)
V_PX(alCSPSetVol, short)
V_PXY(alCSPSetChlProgram, unsigned char, unsigned char)
R_PX(int, alCSPGetChlProgram, unsigned char)
V_PXY(alCSPSetChlFXMix, unsigned char, unsigned char)
R_PX(unsigned char, alCSPGetChlFXMix, unsigned char)
V_PXY(alCSPSetChlPan, unsigned char, unsigned char)
R_PX(unsigned char, alCSPGetChlPan, unsigned char)
V_PXY(alCSPSetChlVol, unsigned char, unsigned char)
R_PX(unsigned char, alCSPGetChlVol, unsigned char)
V_PXY(alCSPSetChlPriority, unsigned char, unsigned char)
R_PX(unsigned char, alCSPGetChlPriority, unsigned char)
V_P(alSndpDelete)
R_PX(short, alSndpAllocate, void *)
V_PX(alSndpDeallocate, short)
V_PX(alSndpSetSound, short)
V_P(alSndpPlay)
V_P(alSndpStop)
V_PX(alSndpSetVol, short)
V_PX(alSndpSetPan, unsigned char)
V_PXY(alSndpSetPriority, short, unsigned char)
V_PX(alSndpSetFXMix, unsigned char)
R_P(int, alSndpGetState)

REAL(void, alSndpSetPitch, void *, float);
void rec_alSndpSetPitch(void *a, float pitch) {
    CALL(alSndpSetPitch, A(a), f2u(pitch), 0, 0, 0);
    real_alSndpSetPitch(a, pitch);
    host_alog_ret(0);
}
#endif
