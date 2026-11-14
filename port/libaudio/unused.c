/*
 * The uncompressed sequence and its player (alSeq*, alSeqp*): boot_code's
 * module 0x13 table names them (2350.c), the game never calls them.  They
 * stop the port if it does.  (No libaudio.h here: they take nothing.)
 */
#include "port.h"


#define UNUSED(name) void name(void) { host_fatal("libaudio: %s isn't in the port (the game doesn't call it)", #name); }
UNUSED(alSeqNew)
UNUSED(alSeqNextEvent)
UNUSED(alSeqGetTicks)
UNUSED(alSeqTicksToSec)
UNUSED(alSeqSecToTicks)
UNUSED(alSeqNewMarker)
UNUSED(alSeqSetLoc)
UNUSED(alSeqGetLoc)
UNUSED(alSeqpNew)
UNUSED(alSeqpDelete)
UNUSED(alSeqpSetSeq)
UNUSED(alSeqpGetSeq)
UNUSED(alSeqpPlay)
UNUSED(alSeqpStop)
UNUSED(alSeqpGetState)
UNUSED(alSeqpSetBank)
UNUSED(alSeqpSetTempo)
UNUSED(alSeqpGetTempo)
UNUSED(alSeqpGetVol)
UNUSED(alSeqpSetVol)
UNUSED(alSeqpLoop)
UNUSED(alSeqpSetChlProgram)
UNUSED(alSeqpGetChlProgram)
UNUSED(alSeqpSetChlFXMix)
UNUSED(alSeqpGetChlFXMix)
UNUSED(alSeqpSetChlVol)
UNUSED(alSeqpGetChlVol)
UNUSED(alSeqpSetChlPan)
UNUSED(alSeqpGetChlPan)
UNUSED(alSeqpSetChlPriority)
UNUSED(alSeqpGetChlPriority)
UNUSED(alSeqpSendMidi)
