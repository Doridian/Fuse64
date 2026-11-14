/*
 * The sound player (alSndp*): one-shot and looping sounds from a bank's
 * ALSound records, each on a synthesizer voice, with the sound's envelope
 * (attack to attackVolume over attackTime, then decay to decayVolume over
 * decayTime, then the end, unless decayTime is -1: held until stopped),
 * stopped with the envelope's release.  The port's own, from the
 * library's documented interface and behaviour (the N64 programming
 * manual's "Sound Player"); Bomberman 64 plays its sound effects through
 * it (80580.c, 85A10.c).
 *
 * Like the sequence player it is a synthesizer client: the calls post
 * events to its queue, and its handler acts on them at the synthesizer's
 * next frame.
 */
#include "audio_private.h"
#include "port.h"

enum {
    SNDP_PLAY_EVT = 1, SNDP_STOP_EVT, SNDP_PAN_EVT, SNDP_VOL_EVT, SNDP_PITCH_EVT, SNDP_API_EVT,
    SNDP_DECAY_EVT, SNDP_END_EVT, SNDP_FX_EVT,
};

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
    u8 voiced;          /* it holds a synthesizer voice */
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

/* the voice's volume: the sound's, the game's and the envelope's, out of
   AL_VOL_FULL each, to the synthesizer's 0x7FFF */
static s16 volume(SndState *st, s32 env) {
    s32 v = (s32)st->sound->sampleVolume * st->vol / 0x7FFF;
    v = v * env / AL_VOL_FULL;
    v = v * v;                          /* the library's squared scale */
    return (s16)(v > 0x7FFF ? 0x7FFF : v);
}

static ALPan pan(SndState *st) {
    s32 p = st->pan + st->sound->samplePan - AL_PAN_CENTER;
    return (ALPan)(p < 0 ? 0 : p > 127 ? 127 : p);
}

static void release(ALSndPlayer *sndp, SndState *st) {
    if (st->voiced) {
        alSynStopVoice(sndp->drvr, &st->voice);
        alSynFreeVoice(sndp->drvr, &st->voice);
        st->voiced = 0;
    }
    st->state = AL_STOPPED;
}

static void handle(ALSndPlayer *sndp, SndEvent *e) {
    SndState *st = e->msg.snd.state;
    ALSound *snd;
    ALVoiceConfig vc;

    switch (e->type) {
    case SNDP_API_EVT:
        post(sndp, SNDP_API_EVT, NULL, 0, 0, sndp->frameTime);
        return;
    case SNDP_PLAY_EVT:
        snd = st->sound;
        if (st->state != AL_STOPPED || snd == NULL)
            return;
        vc.priority = st->priority;
        vc.fxBus = 0;
        vc.unityPitch = 0;
        if (!alSynAllocVoice(sndp->drvr, &st->voice, &vc))
            return;
        st->voiced = 1;
        st->voice.clientPrivate = st;
        alSynStartVoiceParams(sndp->drvr, &st->voice, snd->wavetable,
                              st->pitch * alCents2Ratio(snd->keyMap->detune),
                              volume(st, snd->envelope->attackVolume), pan(st), st->fxMix,
                              snd->envelope->attackTime);
        st->state = AL_PLAYING;
        if (snd->envelope->decayTime != -1)
            post(sndp, SNDP_DECAY_EVT, st, 0, 0, snd->envelope->attackTime);
        return;
    case SNDP_DECAY_EVT:
        if (st->state != AL_PLAYING || !st->voiced)
            return;
        snd = st->sound;
        alSynSetVol(sndp->drvr, &st->voice, volume(st, snd->envelope->decayVolume), snd->envelope->decayTime);
        post(sndp, SNDP_STOP_EVT, st, 0, 0, snd->envelope->decayTime);
        return;
    case SNDP_STOP_EVT:
        if (st->state != AL_PLAYING || !st->voiced) {
            if (st->state == AL_PLAYING)
                st->state = AL_STOPPED;
            return;
        }
        alSynSetVol(sndp->drvr, &st->voice, 0, st->sound->envelope->releaseTime);
        st->state = AL_STOPPING;
        post(sndp, SNDP_END_EVT, st, 0, 0, st->sound->envelope->releaseTime);
        return;
    case SNDP_END_EVT:
        if (st->state == AL_STOPPING)
            release(sndp, st);
        return;
    case SNDP_VOL_EVT:
        st->vol = (s16)e->msg.snd.v;
        if (st->state == AL_PLAYING && st->voiced)
            alSynSetVol(sndp->drvr, &st->voice, volume(st, st->sound->envelope->decayVolume), 1000);
        return;
    case SNDP_PAN_EVT:
        st->pan = (ALPan)e->msg.snd.v;
        if (st->voiced)
            alSynSetPan(sndp->drvr, &st->voice, pan(st));
        return;
    case SNDP_PITCH_EVT:
        st->pitch = e->msg.snd.f;
        if (st->voiced)
            alSynSetPitch(sndp->drvr, &st->voice, st->pitch * alCents2Ratio(st->sound->keyMap->detune));
        return;
    case SNDP_FX_EVT:
        st->fxMix = (u8)e->msg.snd.v;
        if (st->voiced)
            alSynSetFXMix(sndp->drvr, &st->voice, st->fxMix);
        return;
    }
}

static ALMicroTime handler(void *node) {
    ALSndPlayer *sndp = node;

    do {
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
    sndp->curTime = 0;
    sndp->drvr = &alGlobals->drvr;
    st = alHeapAlloc(c->heap, c->maxSounds, sizeof(SndState));
    for (i = 0; i < c->maxSounds; i++)
        st[i].sound = NULL;
    sndp->sndState = st;
    items = alHeapAlloc(c->heap, c->maxEvents, sizeof(ALEventListItem));
    alEvtqNew(&sndp->evtq, items, c->maxEvents);
    sndp->nextEvent.type = SNDP_API_EVT;
    sndp->nextDelta = 0;
    sndp->node.next = NULL;
    sndp->node.handler = handler;
    sndp->node.clientData = sndp;
    alSynAddPlayer(sndp->drvr, &sndp->node);
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
            st[i].fxMix = 0;
            st[i].vol = 0x7FFF;
            st[i].voiced = 0;
            return (ALSndId)i;
        }
    return -1;
}

void alSndpDeallocate(ALSndPlayer *sndp, ALSndId id) {
    if (id >= 0 && id < sndp->maxSounds)
        STATES(sndp)[id].sound = NULL;
}

void alSndpSetSound(ALSndPlayer *sndp, ALSndId id) { sndp->target = id; }
ALSndId alSndpGetSound(ALSndPlayer *sndp) { return (ALSndId)sndp->target; }

/* boot_code D4D0, handwritten: alSndpGetSound */
s32 func_8000C8D0(ALSndPlayer *sndp) { return sndp->target; }

static SndState *target(ALSndPlayer *sndp) {
    if (sndp->target < 0 || sndp->target >= sndp->maxSounds)
        return NULL;
    return &STATES(sndp)[sndp->target];
}

void alSndpPlay(ALSndPlayer *sndp) {
    SndState *st = target(sndp);
    if (st)
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
