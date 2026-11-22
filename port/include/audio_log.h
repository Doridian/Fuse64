/*
 * The libaudio oracle's log (docs/PORT.md, "The libaudio oracle"; after
 * DozerEncore's): what port/src/audio_record.c and port/host/audiolog.c
 * write and port/tools/audio_oracle/replay.c reads.
 *
 * A stream of little-endian 32-bit words.  It starts with ALOG_MAGIC and
 * ALOG_VERSION; then records, each a tag word and its fields:
 *
 *   TRACK addr len          memory the library and the game share
 *   MEM addr len bytes      the game changed these bytes (padded to 4)
 *   CALL fn a0..a5          the game calls fn (ALOG_* below)
 *   IN addr len bytes       ... handing it this structure
 *   OUT addr len bytes      ... which gave back this structure
 *   ACMD len hash_lo hash_hi  ... which wrote this command list (FNV-1a 64)
 *   RET value               ... which returned
 *   CBENTER kind a0 a1 a2   the library calls the game back (ALOG_CB_*)
 *   CBRET value x0 x1       ... which returned, and what it gave back through
 *                           pointers (DMANEW: *state; INITOSC: *oscState,
 *                           *initVal; UPDATEOSC: *updateVal)
 *   UNTRACK addr len        no longer shared (the RSP's and the AI's buffers)
 *   VAR id addr             the game's variable id (ALOG_VAR_*) is at addr
 *   END
 *
 * MEM records come where control passes from the game to the library (before
 * a CALL, before a CBRET): what the game, the RSP and the PI changed in the
 * tracked memory since the library last had it.  Addresses are the
 * recorder's (an i386 build, its image at a fixed address: CMakeLists.txt).
 */
#ifndef AUDIO_LOG_H
#define AUDIO_LOG_H

#include <stdint.h>

#define ALOG_MAGIC 0x474F4C41u      /* "ALOG" */
#define ALOG_VERSION 3

enum {
    ALOG_TRACK = 1, ALOG_MEM, ALOG_CALL, ALOG_IN, ALOG_OUT, ALOG_ACMD, ALOG_RET,
    ALOG_CBENTER, ALOG_CBRET, ALOG_END, ALOG_UNTRACK, ALOG_VAR
};

/*
 * The functions (CALL's fn): every libaudio function the game calls
 * (gen/zerojump.c's stubs), with its parameters and result, each a letter:
 * p a pointer, i an int, h a short, b a byte, f a float, v nothing.
 */
#define ALOG_FUNCS(X)                                                                         \
    X(alUnlink, "v", "p") X(alLink, "v", "pp") X(alHeapInit, "v", "ppi")                       \
    X(alHeapDBAlloc, "p", "pipii") X(alBnkfNew, "v", "pp") X(alSeqFileNew, "v", "pp")          \
    X(alInit, "v", "pp") X(alClose, "v", "p") X(alAudioFrame, "p", "pppi")                     \
    X(alCSeqNew, "v", "pp") X(alCSeqSetLoc, "v", "pp") X(alCSeqGetLoc, "v", "pp")              \
    X(alCents2Ratio, "f", "i") X(alCSPNew, "v", "pp") X(alCSPDelete, "v", "p")                 \
    X(alCSPSetSeq, "v", "pp") X(alCSPPlay, "v", "p") X(alCSPStop, "v", "p")                    \
    X(alCSPGetState, "i", "p") X(alCSPSetBank, "v", "pp") X(alCSPSetTempo, "v", "pi")          \
    X(alCSPGetTempo, "i", "p") X(alCSPSetVol, "v", "ph") X(alCSPSetChlProgram, "v", "pbb")     \
    X(alCSPGetChlProgram, "i", "pb") X(alCSPSetChlFXMix, "v", "pbb")                           \
    X(alCSPGetChlFXMix, "b", "pb") X(alCSPSetChlPan, "v", "pbb") X(alCSPGetChlPan, "b", "pb")  \
    X(alCSPSetChlVol, "v", "pbb") X(alCSPGetChlVol, "b", "pb")                                 \
    X(alCSPSetChlPriority, "v", "pbb") X(alCSPGetChlPriority, "b", "pb")                       \
    X(alSndpNew, "v", "pp") X(alSndpDelete, "v", "p") X(alSndpAllocate, "h", "pp")             \
    X(alSndpDeallocate, "v", "ph") X(alSndpSetSound, "v", "ph") X(alSndpPlay, "v", "p")        \
    X(alSndpStop, "v", "p") X(alSndpSetVol, "v", "ph") X(alSndpSetPitch, "v", "pf")            \
    X(alSndpSetPan, "v", "pb") X(alSndpSetPriority, "v", "phb") X(alSndpSetFXMix, "v", "pb")  \
    X(alSndpGetState, "i", "p")
#define ALOG_ENUM(f, r, a) ALOG_##f,
enum { ALOG_FN_NONE, ALOG_FUNCS(ALOG_ENUM) ALOG_FN_COUNT };
#undef ALOG_ENUM

/* the callbacks (CBENTER's kind) */
enum { ALOG_CB_HANDLER = 1, ALOG_CB_DMANEW, ALOG_CB_DMA, ALOG_CB_INITOSC, ALOG_CB_UPDATEOSC, ALOG_CB_STOPOSC };

/* the game's variables the library reads by name (VAR's id) */
enum { ALOG_VAR_MONO = 1 };     /* D_8001C4B0 (synstartvoiceparam.c) */

/* the structures' sizes as the N64 (and i386) has them (port/libaudio/orig/
   shim.c checks them against ultralib's headers) */
#define ALOG_SIZEOF_ALSYNCONFIG 0x24
#define ALOG_SIZEOF_ALSEQPCONFIG 0x1C
#define ALOG_SIZEOF_ALSNDPCONFIG 0xC
#define ALOG_SIZEOF_ALCSEQMARKER 0xEC

/*
 * The bank objects' map (for the replay): a library that builds its own
 * objects for a bank file's (each ALBank, ALInstrument, ALSound and
 * ALWaveTable, instead of relocating the file's in place) exports, under
 * PORT_AUDIO_ORACLE,
 *
 *   uint32_t alog_bank_map(const AlogBankObj **objs);
 *
 * which points *objs at every object it has built so far and returns how
 * many: each one's address in the file (what the original's pointers
 * hold, so the log's) and its own.  After each alBnkfNew the replay takes
 * the map and translates the log's words that are a mapped object's
 * address (call arguments, the game's memory writes) to the library's,
 * and the library's back where it compares them.  A library without the
 * symbol relocates in place, and nothing is translated.
 */
typedef struct {
    uint32_t file;      /* the object in the bank file */
    uint32_t native;    /* the library's own */
} AlogBankObj;
#define ALOG_BANK_MAP "alog_bank_map"

#ifndef ALOG_NO_HOST
/* port/host/audiolog.c */
int host_alog_started(void);
void host_alog_track(const void *p, uint32_t len);
void host_alog_untrack(const void *p, uint32_t len);
void host_alog_start(void);
void host_alog_var(uint32_t id, const void *p);
void host_alog_call(uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
                    uint32_t a5);
void host_alog_in(const void *p, uint32_t len);
void host_alog_out(const void *p, uint32_t len);
void host_alog_acmd(const void *p, uint32_t len);
void host_alog_ret(uint32_t value);
void host_alog_cb_enter(uint32_t kind, uint32_t a0, uint32_t a1, uint32_t a2);
void host_alog_cb_leave(uint32_t value, uint32_t x0, uint32_t x1);
#endif

#endif
