/*
 * What the port needs beside the decompiled original libaudio
 * (src/libultra/audio) in a -DPORT_LIBAUDIO_ORIGINAL=ON build (the libaudio
 * oracle, docs/PORT.md "Audio"): what the ROM has in the asm data, and
 * what the game's C asks port/libaudio for.  Compiled with ultralib's
 * headers, as the library is.
 */
#include <libaudio.h>

/* (boot_code's asm data on the N64) */
ALGlobals *alGlobals;

/* the players' sizes 80580.c allocates (port_game.h) */
const int port_sizeof_ALCSPlayer = sizeof(ALCSPlayer);
const int port_sizeof_ALCSeq = sizeof(ALCSeq);
const int port_sizeof_ALSndPlayer = sizeof(ALSndPlayer);

/* boot_code's D4D0 (after sndpsetsound.o), handwritten: alSndpGetSound */
s32 func_8000C8D0(ALSndPlayer *sndp) {
    return sndp->target;
}

#ifdef PORT_AUDIO_RECORD
#include "audio/synthInternals.h"

/* the oracle's recorder (port/src/audio_record.c): what of the library's
   own memory only the RSP writes, which the log leaves out (the reverb's
   delay lines) */
void alog_rsp_memory(void (*untrack)(const void *p, u32 len)) {
    ALSynth *s = &alGlobals->drvr;
    s32 i;

    for (i = 0; i < s->maxAuxBusses; i++)
        if (s->auxBus[i].fx[0].base != NULL)
            untrack(s->auxBus[i].fx[0].base, s->auxBus[i].fx[0].length * sizeof(s16));
}
#endif

/* the sizes the oracle's recorder logs the structures with */
#define ALOG_NO_HOST
#include "audio_log.h"
_Static_assert(sizeof(ALSynConfig) == ALOG_SIZEOF_ALSYNCONFIG, "ALSynConfig");
_Static_assert(sizeof(ALSeqpConfig) == ALOG_SIZEOF_ALSEQPCONFIG, "ALSeqpConfig");
_Static_assert(sizeof(ALSndpConfig) == ALOG_SIZEOF_ALSNDPCONFIG, "ALSndpConfig");
_Static_assert(sizeof(ALCSeqMarker) == ALOG_SIZEOF_ALCSEQMARKER, "ALCSeqMarker");
_Static_assert(sizeof(ALEvent) == 0x10, "ALEvent");
