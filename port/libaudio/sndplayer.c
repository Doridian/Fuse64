/*
 * The sound player (alSndp*): one-shot and looping sounds from a bank's
 * ALSound records, each on a synthesizer voice, with the sound's envelope
 * (attack to attackVolume over attackTime, then decay to decayVolume over
 * decayTime, then the release, unless decayTime is -1: held until
 * stopped).  The port's own, from the library's documented interface and
 * behaviour (the N64 programming manual's "Sound Player") and what the
 * game's library (2.0I's, src/libultra/audio) does with it, checked
 * against mupen64plus's sound (docs/PORT.md, "Audio"); Bomberman 64 plays
 * its sound effects through it (80580.c, 85A10.c).
 *
 * Like the sequence player it is a synthesizer client: the calls post
 * events to its queue, and its handler acts on them at the synthesizer's
 * next frame.  What 2.0I's does, and this one too:
 * - a sound's volume is the game's (alSndpSetVol; at first the sound's
 *   sampleVolume, as a fraction of AL_VOL_FULL, times 0x7FFF) times the
 *   envelope's attack or decay volume (out of AL_VOL_FULL); the
 *   synthesizer squares it;
 * - the voice starts silent and ramps to the attack volume over the
 *   attack time; the decay and release times are the envelope's divided
 *   by the pitch, as is the decay's start;
 * - the voice is started even when none could be allocated (the sound
 *   plays nothing but goes through its states);
 * - a stop releases over the release time (and is immediate without
 *   one); the end frees the voice and drops the sound's pending events.
 */
#include "audio_private.h"
#include "port.h"

enum {
    SNDP_PLAY_EVT, SNDP_STOP_EVT, SNDP_PAN_EVT, SNDP_VOL_EVT, SNDP_PITCH_EVT, SNDP_API_EVT,
    SNDP_DECAY_EVT, SNDP_END_EVT, SNDP_FX_EVT,
};

#define MIN_PITCH 0.0001f

/* a sound the game allocated */
typedef struct {
    ALVoice voice;
    ALSound *sound;
    s16 priority;
    f32 pitch;
    s32 state;
    s16 vol;
    ALPan pan;
    u8 fxMix;
} SndState;

#define STATES(sndp) ((SndState *)(sndp)->sndState)

/* an event as the queue holds it: which sound, and a value */
typedef struct {
    s16 type;
    union {
        struct {
            SndState *state;
            s32 v;
            f32 f;
        } snd;
        ALEvent pad;
    } msg;
} SndEvent;

static void post(ALSndPlayer *sndp, s16 type, SndState *st, s32 v, f32 f, ALMicroTime delta) {
    SndEvent e;

    e.type = type;
    e.msg.snd.state = st;
    e.msg.snd.v = v;
    e.msg.snd.f = f;
    alEvtqPostEvent(&sndp->evtq, (ALEvent *)&e, delta);
}

/* a time over the pitch (an envelope's times are at pitch 1), in a double
   and capped, as a very low pitch could overflow it */
static ALMicroTime over_pitch(ALMicroTime t, f32 pitch) {
    f64 d = t / pitch;
    return d > 2147483647.0 ? 2147483647 : (ALMicroTime)d;
}

/* the game's volume times an envelope volume */
static s16 env_vol(SndState *st, u8 env) {
    return (s16)((s32)env * st->vol / AL_VOL_FULL);
}

static ALPan pan(SndState *st) {
    s32 p = st->pan - AL_PAN_CENTER + st->sound->samplePan;
    return (ALPan)(p < AL_PAN_LEFT ? AL_PAN_LEFT : p > AL_PAN_RIGHT ? AL_PAN_RIGHT : p);
}

/* the sound's events still queued go, their time to the next one's */
static void drop_events(ALEventQueue *q, SndState *st) {
    ALLink *ln = q->allocList.next;

    while (ln != NULL) {
        ALLink *next = ln->next;
        ALEventListItem *item = (ALEventListItem *)ln;
        if (((SndEvent *)&item->evt)->msg.snd.state == st) {
            if (next != NULL)
                ((ALEventListItem *)next)->delta += item->delta;
            alUnlink(ln);
            alLink(ln, &q->freeList);
        }
        ln = next;
    }
}

static void finish(ALSndPlayer *sndp, SndState *st) {
    alSynStopVoice(sndp->drvr, &st->voice);
    alSynFreeVoice(sndp->drvr, &st->voice);
    drop_events(&sndp->evtq, st);
    st->state = AL_STOPPED;
}

static void handle(ALSndPlayer *sndp, SndEvent *e) {
    SndState *st = e->msg.snd.state;
    ALSound *snd = st->sound;
    ALVoiceConfig vc;
    ALMicroTime t;

    switch (e->type) {
    case SNDP_PLAY_EVT:
        if (st->state != AL_STOPPED || snd == NULL)
            return;
        vc.fxBus = 0;
        vc.priority = st->priority;
        vc.unityPitch = 0;
        alSynAllocVoice(sndp->drvr, &st->voice, &vc);
        alSynStartVoice(sndp->drvr, &st->voice, snd->wavetable);
        st->state = AL_PLAYING;
        alSynSetPan(sndp->drvr, &st->voice, pan(st));
        alSynSetVol(sndp->drvr, &st->voice, env_vol(st, snd->envelope->attackVolume), snd->envelope->attackTime);
        alSynSetPitch(sndp->drvr, &st->voice, st->pitch);
        alSynSetFXMix(sndp->drvr, &st->voice, st->fxMix);
        post(sndp, SNDP_DECAY_EVT, st, 0, 0, over_pitch(snd->envelope->attackTime, st->pitch));
        return;
    case SNDP_STOP_EVT:
        if (st->state != AL_PLAYING || snd == NULL)
            return;
        t = over_pitch(snd->envelope->releaseTime, st->pitch);
        alSynSetVol(sndp->drvr, &st->voice, 0, t);
        if (t != 0) {
            post(sndp, SNDP_END_EVT, st, 0, 0, t);
            st->state = AL_STOPPING;
        } else {
            finish(sndp, st);
        }
        return;
    case SNDP_PAN_EVT:
        st->pan = (ALPan)e->msg.snd.v;
        if (st->state == AL_PLAYING && snd != NULL)
            alSynSetPan(sndp->drvr, &st->voice, pan(st));
        return;
    case SNDP_PITCH_EVT:
        st->pitch = e->msg.snd.f;
        if (st->pitch < MIN_PITCH)
            st->pitch = MIN_PITCH;
        if (st->state == AL_PLAYING)
            alSynSetPitch(sndp->drvr, &st->voice, st->pitch);
        return;
    case SNDP_FX_EVT:
        st->fxMix = (u8)e->msg.snd.v;
        if (st->state == AL_PLAYING)
            alSynSetFXMix(sndp->drvr, &st->voice, st->fxMix);
        return;
    case SNDP_VOL_EVT:
        st->vol = (s16)e->msg.snd.v;
        if (st->state == AL_PLAYING && snd != NULL)
            alSynSetVol(sndp->drvr, &st->voice, env_vol(st, snd->envelope->decayVolume), 1000);
        return;
    case SNDP_DECAY_EVT:
        /* at the attack's end: the decay, and the release after it (none
           for a sound held until it is stopped) */
        if (snd->envelope->decayTime != -1) {
            t = over_pitch(snd->envelope->decayTime, st->pitch);
            alSynSetVol(sndp->drvr, &st->voice, env_vol(st, snd->envelope->decayVolume), t);
            post(sndp, SNDP_STOP_EVT, st, 0, 0, t);
        }
        return;
    case SNDP_END_EVT:
        finish(sndp, st);
        return;
    }
}

static ALMicroTime handler(void *node) {
    ALSndPlayer *sndp = node;

    do {
        if (sndp->nextEvent.type == SNDP_API_EVT)
            post(sndp, SNDP_API_EVT, NULL, 0, 0, sndp->frameTime);
        else
            handle(sndp, (SndEvent *)&sndp->nextEvent);
        sndp->nextDelta = alEvtqNextEvent(&sndp->evtq, &sndp->nextEvent);
    } while (sndp->nextDelta == 0);
    sndp->curTime += sndp->nextDelta;
    return sndp->nextDelta;
}

void alSndpNew(ALSndPlayer *sndp, ALSndpConfig *c) {
    ALEventListItem *items;
    SndState *st;
    s32 i;

    sndp->maxSounds = c->maxSounds;
    sndp->target = -1;
    sndp->frameTime = AL_USEC_PER_FRAME;
    st = alHeapAlloc(c->heap, c->maxSounds, sizeof(SndState));
    for (i = 0; i < c->maxSounds; i++)
        st[i].sound = NULL;
    sndp->sndState = st;
    items = alHeapAlloc(c->heap, c->maxEvents, sizeof(ALEventListItem));
    alEvtqNew(&sndp->evtq, items, c->maxEvents);
    sndp->drvr = &alGlobals->drvr;
    sndp->node.next = NULL;
    sndp->node.handler = handler;
    sndp->node.clientData = sndp;
    alSynAddPlayer(sndp->drvr, &sndp->node);
    /* the first API event a frame from now */
    post(sndp, SNDP_API_EVT, NULL, 0, 0, sndp->frameTime);
    sndp->nextDelta = alEvtqNextEvent(&sndp->evtq, &sndp->nextEvent);
}

void alSndpDelete(ALSndPlayer *sndp) { alSynRemovePlayer(sndp->drvr, &sndp->node); }

ALSndId alSndpAllocate(ALSndPlayer *sndp, ALSound *sound) {
    SndState *st = STATES(sndp);
    s32 i;

    for (i = 0; i < sndp->maxSounds; i++)
        if (st[i].sound == NULL) {
            st[i].sound = sound;
            st[i].priority = AL_DEFAULT_PRIORITY;
            st[i].state = AL_STOPPED;
            st[i].pitch = 1.0f;
            st[i].pan = AL_PAN_CENTER;
            st[i].fxMix = AL_DEFAULT_FXMIX;
            st[i].vol = (s16)(32767 * sound->sampleVolume / AL_VOL_FULL);
            return (ALSndId)i;
        }
    return -1;
}

void alSndpDeallocate(ALSndPlayer *sndp, ALSndId id) {
    SndState *st = STATES(sndp);

    if (st[id].state != AL_STOPPED)
        return;
    st[id].sound = NULL;
    if (sndp->target == id)
        sndp->target = -1;
}

void alSndpSetSound(ALSndPlayer *sndp, ALSndId id) { sndp->target = id; }
ALSndId alSndpGetSound(ALSndPlayer *sndp) { return (ALSndId)sndp->target; }

/* boot_code D4D0, handwritten: alSndpGetSound */
s32 func_8000C8D0(ALSndPlayer *sndp) { return sndp->target; }

/* the target's state (as the library, no check of the target: the game
   sets one first) */
static SndState *target(ALSndPlayer *sndp) {
    if (sndp->target < 0 || sndp->target >= sndp->maxSounds)
        return NULL;
    return &STATES(sndp)[sndp->target];
}

void alSndpPlay(ALSndPlayer *sndp) {
    SndState *st = target(sndp);
    if (st && st->state == AL_STOPPED)
        post(sndp, SNDP_PLAY_EVT, st, 0, 0, 0);
}

void alSndpPlayAt(ALSndPlayer *sndp, ALMicroTime delta) {
    SndState *st = target(sndp);
    if (st)
        post(sndp, SNDP_PLAY_EVT, st, 0, 0, delta);
}

void alSndpStop(ALSndPlayer *sndp) {
    SndState *st = target(sndp);
    if (st)
        post(sndp, SNDP_STOP_EVT, st, 0, 0, 0);
}

void alSndpSetVol(ALSndPlayer *sndp, s16 vol) {
    SndState *st = target(sndp);
    if (st)
        post(sndp, SNDP_VOL_EVT, st, vol, 0, 0);
}

void alSndpSetPitch(ALSndPlayer *sndp, f32 pitch) {
    SndState *st = target(sndp);
    if (st)
        post(sndp, SNDP_PITCH_EVT, st, 0, pitch, 0);
}

void alSndpSetPan(ALSndPlayer *sndp, ALPan p) {
    SndState *st = target(sndp);
    if (st)
        post(sndp, SNDP_PAN_EVT, st, p, 0, 0);
}

void alSndpSetPriority(ALSndPlayer *sndp, ALSndId id, u8 priority) {
    if (id >= 0 && id < sndp->maxSounds)
        STATES(sndp)[id].priority = priority;
}

void alSndpSetFXMix(ALSndPlayer *sndp, u8 mix) {
    SndState *st = target(sndp);
    if (st)
        post(sndp, SNDP_FX_EVT, st, mix, 0, 0);
}

s32 alSndpGetState(ALSndPlayer *sndp) {
    SndState *st = target(sndp);
    return st ? st->state : AL_STOPPED;
}
