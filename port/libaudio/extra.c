/*
 * The rest of the libaudio interface Bomberman 64 has (boot_code's module
 * 0x13 table, 2350.c): the compact sequence player's channel calls, the
 * synthesizer's effect and priority calls, and the uncompressed sequence
 * and its player, which the game's table names but the game never calls
 * (they stop the port if it does).  The port's own, from the library's
 * documented interface (the N64 programming manual's "Audio Library").
 */
#include "audio_private.h"
#include "port.h"

/* ---- the compact sequence player's channels -------------------------------------------- */

void alCSPSendMidi(ALCSPlayer *seqp, s32 ticks, u8 status, u8 byte1, u8 byte2) {
    ALEvent evt;

    evt.type = AL_SEQP_MIDI_EVT;
    evt.msg.midi.ticks = 0;
    evt.msg.midi.status = status;
    evt.msg.midi.byte1 = byte1;
    evt.msg.midi.byte2 = byte2;
    evt.msg.midi.duration = 0;
    alEvtqPostEvent(&seqp->evtq, &evt, (ALMicroTime)(ticks * seqp->uspt));
}

void alCSPSetChlProgram(ALCSPlayer *seqp, u8 chan, u8 prog) {
    alCSPSendMidi(seqp, 0, AL_MIDI_ProgramChange | chan, prog, 0);
}

s32 alCSPGetChlProgram(ALCSPlayer *seqp, u8 chan) {
    ALInstrument *inst = seqp->chanState[chan].instrument;
    s32 i;

    if (seqp->bank == NULL)
        return -1;
    for (i = 0; i < seqp->bank->instCount; i++)
        if (seqp->bank->instArray[i] == inst)
            return i;
    return -1;
}

void alCSPSetChlFXMix(ALCSPlayer *seqp, u8 chan, u8 fxmix) {
    alCSPSendMidi(seqp, 0, AL_MIDI_ControlChange | chan, AL_MIDI_FX1_CTRL, fxmix);
}
u8 alCSPGetChlFXMix(ALCSPlayer *seqp, u8 chan) { return seqp->chanState[chan].fxmix; }

void alCSPSetChlVol(ALCSPlayer *seqp, u8 chan, u8 vol) {
    alCSPSendMidi(seqp, 0, AL_MIDI_ControlChange | chan, AL_MIDI_VOLUME_CTRL, vol);
}
u8 alCSPGetChlVol(ALCSPlayer *seqp, u8 chan) { return seqp->chanState[chan].vol; }

void alCSPSetChlPan(ALCSPlayer *seqp, u8 chan, u8 pan) {
    alCSPSendMidi(seqp, 0, AL_MIDI_ControlChange | chan, AL_MIDI_PAN_CTRL, pan);
}
u8 alCSPGetChlPan(ALCSPlayer *seqp, u8 chan) { return seqp->chanState[chan].pan; }

void alCSPSetChlPriority(ALCSPlayer *seqp, u8 chan, u8 priority) {
    ALEvent evt;

    evt.type = AL_SEQP_PRIORITY_EVT;
    evt.msg.sppriority.chan = chan;
    evt.msg.sppriority.priority = priority;
    alEvtqPostEvent(&seqp->evtq, &evt, 0);
}
u8 alCSPGetChlPriority(ALCSPlayer *seqp, u8 chan) { return seqp->chanState[chan].priority; }

ALCSeq *alCSPGetSeq(ALCSPlayer *seqp) { return seqp->target; }

void alCSPDelete(ALCSPlayer *seqp) { alSynRemovePlayer(seqp->drvr, &seqp->node); }

/* ---- the synthesizer ----------------------------------------------------------------------- */

void alSynRemovePlayer(ALSynth *s, ALPlayer *client) {
    ALPlayer **p;

    for (p = &s->head; *p != NULL; p = &(*p)->next)
        if (*p == client) {
            *p = client->next;
            client->next = NULL;
            return;
        }
}

s16 alSynGetPriority(ALSynth *s, ALVoice *voice) { return voice->priority; }

/* the game sets its effects through alInit's config; these are the
   library's for a second bus, which it never asks for */
ALFxRef *alSynAllocFX(ALSynth *s, s16 bus, ALSynConfig *c, ALHeap *hp) { return NULL; }
ALFxRef alSynGetFXRef(ALSynth *s, s16 bus, s16 index) { return NULL; }
void alSynFreeFX(ALSynth *s, ALFxRef *fx) { }
void alSynSetFXParam(ALSynth *s, ALFxRef fx, s16 paramID, void *param) { }

s32 alHeapCheck(ALHeap *hp) { return 0; }

/* ---- bank files --------------------------------------------------------------------------- */

void alBnkfNew(ALBankFile *f, u8 *table) {
    s32 i;

    port_alBnkfParse(f, (u32)(uintptr_t)table, 0x7FFFFFFF);
    for (i = 0; i < f->bankCount; i++)
        f->bankArray[i] = alBnkfBank(f, i);
}
