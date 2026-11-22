/*
 * The libaudio oracle's recorder (-DPORT_AUDIO_RECORD=ON): included
 * before gen/zerojump.c, it sends every stub that reaches libaudio through
 * the recorder's function of the same name (port/src/audio_record.c),
 * which logs the call and makes it.  The library's calls to itself don't
 * go through a stub, so only the game's are logged.
 */
#define alUnlink rec_alUnlink
#define alLink rec_alLink
#define alHeapInit rec_alHeapInit
#define alHeapDBAlloc rec_alHeapDBAlloc
#define alBnkfNew rec_alBnkfNew
#define alSeqFileNew rec_alSeqFileNew
#define alInit rec_alInit
#define alClose rec_alClose
#define alAudioFrame rec_alAudioFrame
#define alCSeqNew rec_alCSeqNew
#define alCSeqSetLoc rec_alCSeqSetLoc
#define alCSeqGetLoc rec_alCSeqGetLoc
#define alCents2Ratio rec_alCents2Ratio
#define alCSPNew rec_alCSPNew
#define alCSPDelete rec_alCSPDelete
#define alCSPSetSeq rec_alCSPSetSeq
#define alCSPPlay rec_alCSPPlay
#define alCSPStop rec_alCSPStop
#define alCSPGetState rec_alCSPGetState
#define alCSPSetBank rec_alCSPSetBank
#define alCSPSetTempo rec_alCSPSetTempo
#define alCSPGetTempo rec_alCSPGetTempo
#define alCSPSetVol rec_alCSPSetVol
#define alCSPSetChlProgram rec_alCSPSetChlProgram
#define alCSPGetChlProgram rec_alCSPGetChlProgram
#define alCSPSetChlFXMix rec_alCSPSetChlFXMix
#define alCSPGetChlFXMix rec_alCSPGetChlFXMix
#define alCSPSetChlPan rec_alCSPSetChlPan
#define alCSPGetChlPan rec_alCSPGetChlPan
#define alCSPSetChlVol rec_alCSPSetChlVol
#define alCSPGetChlVol rec_alCSPGetChlVol
#define alCSPSetChlPriority rec_alCSPSetChlPriority
#define alCSPGetChlPriority rec_alCSPGetChlPriority
#define alSndpNew rec_alSndpNew
#define alSndpDelete rec_alSndpDelete
#define alSndpAllocate rec_alSndpAllocate
#define alSndpDeallocate rec_alSndpDeallocate
#define alSndpSetSound rec_alSndpSetSound
#define alSndpPlay rec_alSndpPlay
#define alSndpStop rec_alSndpStop
#define alSndpSetVol rec_alSndpSetVol
#define alSndpSetPitch rec_alSndpSetPitch
#define alSndpSetPan rec_alSndpSetPan
#define alSndpSetPriority rec_alSndpSetPriority
#define alSndpSetFXMix rec_alSndpSetFXMix
#define alSndpGetState rec_alSndpGetState
