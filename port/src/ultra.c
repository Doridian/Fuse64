/*
 * libultra for the port: the OS (threads, messages, events, timers) and the
 * I/O the game uses (PI, SI, VI, SP/DP, AI), on top of port/host.  From
 * DozerEncore's port/src/ultra.c (the same author's, AGPL-3.0), with this
 * game's differences: four controllers, the controller pak as a file, the
 * scheduler's own SP/DP handling.
 *
 * N64 side: built like the game, so the structures the game shares with
 * libultra (OSMesgQueue, OSContPad, OSTask, OSTimer...) are its own and
 * are used as plain C.  What needs the host (fibers, the cartridge, the
 * window) goes through port.h.
 *
 * Semantics follow libultra 2.0: a send that wakes a higher-priority thread
 * switches to it at once, events are messages sent (without blocking) to
 * the queue registered for them, and I/O completes at once but reports
 * completion through the same messages the hardware would cause.
 */
#include <ultra64.h>
#include <PR/rcp.h>
#include <string.h>
#include "port.h"
#include <PR/libaudio.h>

/* the game's C keeps these as bytes (port_game.h) */
_Static_assert(sizeof(OSThread) <= PORT_SIZEOF_OSTHREAD, "PORT_SIZEOF_OSTHREAD");
_Static_assert(sizeof(ALGlobals) <= PORT_SIZEOF_ALGLOBALS, "PORT_SIZEOF_ALGLOBALS");
_Static_assert(sizeof(OSIoMesg) == PORT_SIZEOF_OSIOMESG, "PORT_SIZEOF_OSIOMESG");

/* ---- globals libultra defines ------------------------------------------ */

void *osRomBase;
s32 osTvType, osResetType, osCicId, osVersion;
u32 osMemSize;
s32 osAppNMIBuffer[16];
u64 osClockRate = OS_CLOCK_RATE;

/* ---- messages ------------------------------------------------------------ */

/* a thread waits on a queue's address (not empty) or the byte after it
   (not full); the keys are only compared */
#define KEY_NOT_EMPTY(mq) ((const void *)(mq))
#define KEY_NOT_FULL(mq) ((const void *)((const u8 *)(mq) + 1))

void osCreateMesgQueue(OSMesgQueue *mq, OSMesg *msg, s32 count) {
    mq->mtqueue = NULL;
    mq->fullqueue = NULL;
    mq->validCount = 0;
    mq->first = 0;
    mq->msgCount = count;
    mq->msg = msg;
}

static s32 send(OSMesgQueue *mq, OSMesg msg, s32 flag, int jam) {
    while (mq->validCount >= mq->msgCount) {
        if (flag != OS_MESG_BLOCK)
            return -1;
        host_block(KEY_NOT_FULL(mq));
    }
    if (jam) {
        mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
        mq->msg[mq->first] = msg;
    } else {
        mq->msg[(mq->first + mq->validCount) % mq->msgCount] = msg;
    }
    mq->validCount++;
    host_wake(KEY_NOT_EMPTY(mq));
    host_preempt();
    return 0;
}

s32 osSendMesg(OSMesgQueue *mq, OSMesg msg, s32 flag) { return send(mq, msg, flag, 0); }
s32 osJamMesg(OSMesgQueue *mq, OSMesg msg, s32 flag) { return send(mq, msg, flag, 1); }

s32 osRecvMesg(OSMesgQueue *mq, OSMesg *msg, s32 flag) {
    host_recv_mark(KEY_NOT_EMPTY(mq));
    while (mq->validCount == 0) {
        if (flag == OS_MESG_NOBLOCK)
            return -1;
        host_block(KEY_NOT_EMPTY(mq));
    }
    if (msg != NULL)
        *msg = mq->msg[mq->first];
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;
    host_wake(KEY_NOT_FULL(mq));
    host_preempt();
    return 0;
}

/* ---- events ---------------------------------------------------------------- */

static struct { OSMesgQueue *mq; OSMesg msg; } events[OS_NUM_EVENTS];

void osSetEventMesg(OSEvent e, OSMesgQueue *mq, OSMesg msg) {
    if (e < OS_NUM_EVENTS) {
        events[e].mq = mq;
        events[e].msg = msg;
    }
}

void port_irq_event(int e) {
    host_interrupt();
    if (e >= 0 && e < OS_NUM_EVENTS && events[e].mq != NULL)
        osSendMesg(events[e].mq, events[e].msg, OS_MESG_NOBLOCK);
}

/* ---- threads ----------------------------------------------------------------- */

#define THREAD_KEY(t) ((uintptr_t)(t))

void osCreateThread(OSThread *t, OSId id, void (*entry)(void *), void *arg, void *sp, OSPri pri) {
    t->next = NULL;
    t->priority = pri;
    t->queue = NULL;
    t->tlnext = NULL;
    t->state = OS_STATE_STOPPED;
    t->flags = 0;
    t->id = id;
    t->fp = 0;
    host_thread_create(THREAD_KEY(t), entry, arg, pri);
}

void osStartThread(OSThread *t) {
    t->state = OS_STATE_RUNNABLE;
    host_thread_start(THREAD_KEY(t));
}

void osStopThread(OSThread *t) {
    if (t != NULL)
        t->state = OS_STATE_STOPPED;
    host_thread_stop(THREAD_KEY(t));
}

void osDestroyThread(OSThread *t) { host_thread_destroy(THREAD_KEY(t)); }

void osSetThreadPri(OSThread *t, OSPri pri) {
    if (t != NULL)
        t->priority = pri;
    host_thread_set_pri(THREAD_KEY(t), pri);
}

OSPri osGetThreadPri(OSThread *t) { return host_thread_get_pri(THREAD_KEY(t)); }

OSId osGetThreadId(OSThread *t) {
    if (t == NULL)
        t = (OSThread *)host_thread_current();
    return t != NULL ? t->id : 0;
}

void osYieldThread(void) { host_yield(); }

/* one thread runs at a time, and nothing preempts it but its own calls into
   libultra, so the mask has nothing to hold off */
static OSIntMask int_mask = OS_IM_ALL;
OSIntMask osSetIntMask(OSIntMask m) {
    OSIntMask old = int_mask;
    int_mask = m;
    return old;
}
OSIntMask osGetIntMask(void) { return int_mask; }

OSThread *__osGetCurrFaultedThread(void) { return NULL; }

/* ---- time and timers --------------------------------------------------------- */

static OSTime time_base;

OSTime osGetTime(void) { return host_ticks() - time_base; }
void osSetTime(OSTime t) { time_base = host_ticks() - t; }
u32 osGetCount(void) { return (u32)host_ticks(); }

#define MAX_TIMERS 16
static OSTimer *timers[MAX_TIMERS];
static OSTime timer_due[MAX_TIMERS];

int osSetTimer(OSTimer *t, OSTime countdown, OSTime interval, OSMesgQueue *mq, OSMesg msg) {
    int i, free = -1;
    t->interval = interval;
    t->value = countdown != 0 ? countdown : interval;
    t->mq = mq;
    t->msg = msg;
    for (i = 0; i < MAX_TIMERS; i++) {
        if (timers[i] == t) {
            free = i;
            break;
        }
        if (timers[i] == NULL && free < 0)
            free = i;
    }
    if (free < 0)
        return -1;
    timers[free] = t;
    timer_due[free] = host_ticks() + t->value;
    return 0;
}

int osStopTimer(OSTimer *t) {
    int i;
    for (i = 0; i < MAX_TIMERS; i++)
        if (timers[i] == t) {
            timers[i] = NULL;
            return 0;
        }
    return -1;
}

/* PI DMAs complete one after another (a count tick per 8 bytes, much
   faster than the hardware); the data is copied at once, the message comes
   when the transfer would be done */
#define PI_MAX 256
static struct { OSTime due; OSMesgQueue *mq; OSMesg msg; } pi_q[PI_MAX];
static int pi_head, pi_n;
static OSTime pi_free_at;

static void pi_complete(OSMesgQueue *mq, OSMesg msg, u32 nbytes) {
    OSTime now = host_ticks();
    int i;
    if (pi_free_at < now)
        pi_free_at = now;
    pi_free_at += nbytes / 8 + 1;
    if (mq == NULL)
        return;
    if (pi_n == PI_MAX) {
        osSendMesg(pi_q[pi_head].mq, pi_q[pi_head].msg, OS_MESG_NOBLOCK);
        pi_head = (pi_head + 1) % PI_MAX;
        pi_n--;
    }
    i = (pi_head + pi_n++) % PI_MAX;
    pi_q[i].due = pi_free_at;
    pi_q[i].mq = mq;
    pi_q[i].msg = msg;
}

unsigned long long port_irq_timers(unsigned long long now) {
    u64 next = ~0ull;
    int i;
    while (pi_n > 0 && pi_q[pi_head].due <= now) {
        host_interrupt();
        osSendMesg(pi_q[pi_head].mq, pi_q[pi_head].msg, OS_MESG_NOBLOCK);
        pi_head = (pi_head + 1) % PI_MAX;
        pi_n--;
    }
    if (pi_n > 0)
        next = pi_q[pi_head].due;
    for (i = 0; i < MAX_TIMERS; i++) {
        OSTimer *t = timers[i];
        if (t == NULL)
            continue;
        if (timer_due[i] <= now) {
            if (t->interval != 0) {
                timer_due[i] += t->interval;
                if (timer_due[i] <= now)
                    timer_due[i] = now + t->interval;
            } else {
                timers[i] = NULL;
            }
            host_interrupt();
            if (t->mq != NULL)
                osSendMesg(t->mq, t->msg, OS_MESG_NOBLOCK);
        }
        if (timers[i] != NULL && timer_due[i] < next)
            next = timer_due[i];
    }
    return next;
}

/* ---- boot ---------------------------------------------------------------------- */

void osInitialize(void) {
#ifdef VERSION_EU
    osTvType = OS_TV_PAL;
#else
    osTvType = OS_TV_NTSC;
#endif
    osRomBase = (void *)(uintptr_t)0xB0000000u;
    /* (IPL3's, from the PIF: 1 after an NMI; libultra's osInitialize then
       keeps the application's NMI buffer, which a power-on clears) */
    osResetType = port_reset_count() > 0;
    if (osResetType == 0)
        memset(osAppNMIBuffer, 0, sizeof osAppNMIBuffer);
    osMemSize = 0x400000;
}

u32 osGetMemSize(void) { return 0x400000; }

/* ---- caches, TLB, addresses ------------------------------------------------------ */

void osWritebackDCache(void *p, s32 n) { }
void osWritebackDCacheAll(void) { }
void osInvalDCache(void *p, s32 n) { }
void osInvalICache(void *p, s32 n) { }
void osMapTLB(s32 index, OSPageMask pm, void *vaddr, u32 even, u32 odd, s32 asid) { }
void osMapTLBRdb(void) { }
void osUnmapTLBAll(void) { }
/* a pointer's value: the port's display lists take host pointers (port_addr) */
__UINTPTR_TYPE__ osVirtualToPhysical(void *p) { return (uintptr_t)p; }
void *osPhysicalToVirtual(__UINTPTR_TYPE__ a) { return (void *)a; }

/* ---- PI: the cartridge ------------------------------------------------------------- */

static OSMesgQueue *pi_cmdq;

void osCreatePiManager(OSPri pri, OSMesgQueue *cmdQ, OSMesg *cmdBuf, s32 cmdMsgCnt) {
    osCreateMesgQueue(cmdQ, cmdBuf, cmdMsgCnt);
    pi_cmdq = cmdQ;
}

OSMesgQueue *osPiGetCmdQueue(void) { return pi_cmdq; }

s32 osPiStartDma(OSIoMesg *mb, s32 pri, s32 dir, u32 devAddr, void *vAddr, u32 nbytes, OSMesgQueue *mq) {
    mb->hdr.type = OS_MESG_TYPE_DMAREAD;
    mb->hdr.pri = pri;
    mb->hdr.retQueue = mq;
    mb->dramAddr = vAddr;
    mb->devAddr = devAddr;
    mb->size = nbytes;
    /* (a read to NULL takes its time and copies nothing: boot_code's loads
       of the code the port has linked in, 13F0.c) */
    if (dir == OS_READ && vAddr != NULL)
        host_rom_read(port_addr((uintptr_t)vAddr), devAddr, nbytes);
    else if (dir != OS_READ && host_verbose)
        host_log("PI write to %08X ignored\n", (unsigned)devAddr);
    pi_complete(mq, (OSMesg)mb, nbytes);
    return 0;
}

s32 osPiRawStartDma(s32 dir, u32 devAddr, void *dramAddr, u32 size) {
    if (dir == OS_READ)
        host_rom_read(port_addr((uintptr_t)dramAddr), devAddr, size);
    return 0;
}

s32 osPiRawReadIo(u32 devAddr, u32 *data) {
    u8 b[4];
    host_rom_read(b, devAddr, 4);
    *data = port_be32(b);
    return 0;
}

s32 osPiReadIo(u32 devAddr, u32 *data) { return osPiRawReadIo(devAddr, data); }
s32 osPiRawWriteIo(u32 devAddr, u32 data) { return 0; }
s32 osPiWriteIo(u32 devAddr, u32 data) { return 0; }
u32 osPiGetStatus(void) { return 0; }

/* boot_code 6080, handwritten: osPiGetDeviceType */
s32 func_80005480(void) { return 0; }

/* ---- SI: controllers, EEPROM, the controller pak ---------------------------------------- */

static int max_controllers = MAXCONTROLLERS;

static void cont_status(OSContStatus *status) {
    int i;
    s8 x, y;
    u16 b;
    for (i = 0; i < MAXCONTROLLERS; i++) {
        int on = host_input(i, &b, &x, &y);
        status[i].type = on ? CONT_TYPE_NORMAL : 0;
        status[i].status = on && host_pak_present(i) ? CONT_CARD_ON : 0;
        status[i].errno = on ? 0 : CONT_NO_RESPONSE_ERROR;
    }
}

s32 osContInit(OSMesgQueue *mq, u8 *bitpattern, OSContStatus *status) {
    int i;
    cont_status(status);
    *bitpattern = 0;
    for (i = 0; i < MAXCONTROLLERS; i++)
        if (status[i].errno == 0)
            *bitpattern |= 1 << i;
    max_controllers = MAXCONTROLLERS;
    return 0;
}

s32 osContReset(OSMesgQueue *mq, OSContStatus *status) {
    cont_status(status);
    return 0;
}

s32 osContStartQuery(OSMesgQueue *mq) {
    port_irq_event(OS_EVENT_SI);
    return 0;
}

void osContGetQuery(OSContStatus *status) { cont_status(status); }

void port_digest_poll(void);

s32 osContStartReadData(OSMesgQueue *mq) {
    /* when the PIF reads the pads: the pad for this frame (--replay) and
       PORT_DIGEST's line (port/host/digest.c) */
    port_digest_poll();
    port_irq_event(OS_EVENT_SI);
    return 0;
}

void osContGetReadData(OSContPad *pad) {
    int i;
    for (i = 0; i < max_controllers; i++) {
        u16 b = 0;
        s8 x = 0, y = 0;
        int on = host_input(i, &b, &x, &y);
        pad[i].button = b;
        pad[i].stick_x = x;
        pad[i].stick_y = y;
        pad[i].errno = on ? 0 : CONT_NO_RESPONSE_ERROR;
    }
}

s32 osContSetCh(u8 ch) {
    max_controllers = ch < MAXCONTROLLERS ? ch : MAXCONTROLLERS;
    return 0;
}

s32 osEepromProbe(OSMesgQueue *mq) { return EEPROM_TYPE_4K; }

s32 osEepromRead(OSMesgQueue *mq, u8 addr, u8 *buf) {
    host_eeprom_read(addr, buf);
    return 0;
}

s32 osEepromWrite(OSMesgQueue *mq, u8 addr, u8 *buf) {
    host_eeprom_write(addr, buf);
    return 0;
}

s32 osEepromLongRead(OSMesgQueue *mq, u8 addr, u8 *buf, int n) {
    for (; n > 0; n -= 8, addr++, buf += 8)
        host_eeprom_read(addr, buf);
    return 0;
}

s32 osEepromLongWrite(OSMesgQueue *mq, u8 addr, u8 *buf, int n) {
    for (; n > 0; n -= 8, addr++, buf += 8)
        host_eeprom_write(addr, buf);
    return 0;
}

/* the rumble pak: none */
s32 osMotorInit(OSMesgQueue *mq, OSPfs *pfs, int channel) { return PFS_ERR_NOPACK; }
s32 osMotorStop(OSPfs *pfs) { return 0; }
s32 osMotorStart(OSPfs *pfs) { return 0; }

/* ---- VI -------------------------------------------------------------------------- */

s32 osViClock = PORT_VI_CLOCK;

static OSMesgQueue *vi_mq;
static OSMesg vi_msg;
static u32 vi_count, vi_retrace;
static void *vi_cur_fb, *vi_next_fb;
static OSViMode *vi_mode;
static u32 vi_control;                  /* VI_CONTROL_REG: the mode's, with the special features */
static int vi_black;

/* VI_CONTROL_REG's bits that osViSetSpecialFeatures sets */
#define VI_CTRL_GAMMA_DITHER_ON 0x00004
#define VI_CTRL_GAMMA_ON 0x00008
#define VI_CTRL_DIVOT_ON 0x00010
#define VI_CTRL_ANTIALIAS_MASK 0x00300
#define VI_CTRL_DITHER_FILTER_ON 0x10000

void osCreateViManager(OSPri pri) { }

void osViSetMode(OSViMode *m) {
    vi_mode = m;
    vi_control = m->comRegs.ctrl;
}

void osViSetEvent(OSMesgQueue *mq, OSMesg msg, u32 retraceCount) {
    vi_mq = mq;
    vi_msg = msg;
    vi_retrace = retraceCount;
    vi_count = 0;
}

void osViSwapBuffer(void *fb) { vi_next_fb = fb; }
void *osViGetCurrentFramebuffer(void) { return vi_cur_fb; }
void *osViGetNextFramebuffer(void) { return vi_next_fb; }
u32 osViGetCurrentMode(void) { return vi_mode ? vi_mode->type : 0; }
u32 osViGetStatus(void) { return 0; }
void osViBlack(u8 active) { vi_black = active; }
void osViFade(u8 active, u16 factor) { }

/* libultra's: the game turns the gamma off, the gamma dither, the divot
   and the dither filter on (40040.c), which takes the antialiasing's mode
   to 0 (resample and antialias, always fetching the extra lines) */
void osViSetSpecialFeatures(u32 func) {
    if (func & OS_VI_GAMMA_ON)
        vi_control |= VI_CTRL_GAMMA_ON;
    if (func & OS_VI_GAMMA_OFF)
        vi_control &= ~VI_CTRL_GAMMA_ON;
    if (func & OS_VI_GAMMA_DITHER_ON)
        vi_control |= VI_CTRL_GAMMA_DITHER_ON;
    if (func & OS_VI_GAMMA_DITHER_OFF)
        vi_control &= ~VI_CTRL_GAMMA_DITHER_ON;
    if (func & OS_VI_DIVOT_ON)
        vi_control |= VI_CTRL_DIVOT_ON;
    if (func & OS_VI_DIVOT_OFF)
        vi_control &= ~VI_CTRL_DIVOT_ON;
    if (func & OS_VI_DITHER_FILTER_ON) {
        vi_control |= VI_CTRL_DITHER_FILTER_ON;
        vi_control &= ~VI_CTRL_ANTIALIAS_MASK;
    }
    if ((func & OS_VI_DITHER_FILTER_OFF) && vi_mode != NULL) {
        vi_control &= ~VI_CTRL_DITHER_FILTER_ON;
        vi_control |= vi_mode->comRegs.ctrl & VI_CTRL_ANTIALIAS_MASK;
    }
}

/* (the game never scales the picture) */
void osViSetXScale(f32 x) { }
void osViSetYScale(f32 y) { }
u32 osViGetCurrentLine(void) { return 0; }
u32 osViGetCurrentField(void) { return 0; }

/* the registers as __osViSwapContext writes them at a retrace: the mode's,
   the origin the framebuffer's plus the field's offset, no picture
   (H_START 0) while black; field 0, as the game's modes aren't interlaced */
static void vi_regs(void) {
    uint32_t r[PORT_VI_NREGS] = { 0 };
    const uint8_t *fb = vi_cur_fb != NULL ? port_addr((uintptr_t)vi_cur_fb) : NULL;
    if (vi_mode != NULL && fb != NULL && fb >= port_rdram && fb < port_rdram + PORT_RDRAM_SIZE) {
        OSViMode *m = vi_mode;
        r[PORT_VI_CONTROL] = vi_control;
        r[PORT_VI_ORIGIN] = (uint32_t)(fb - port_rdram) + m->fldRegs[0].origin;
        r[PORT_VI_WIDTH] = m->comRegs.width;
        r[PORT_VI_INTR] = m->fldRegs[0].vIntr;
        r[PORT_VI_BURST] = m->comRegs.burst;
        r[PORT_VI_V_SYNC] = m->comRegs.vSync;
        r[PORT_VI_H_SYNC] = m->comRegs.hSync;
        r[PORT_VI_LEAP] = m->comRegs.leap;
        r[PORT_VI_H_START] = vi_black ? 0 : m->comRegs.hStart;
        r[PORT_VI_V_START] = m->fldRegs[0].vStart;
        r[PORT_VI_V_BURST] = m->fldRegs[0].vBurst;
        r[PORT_VI_X_SCALE] = m->comRegs.xScale;
        r[PORT_VI_Y_SCALE] = m->fldRegs[0].yScale;
    }
    host_vi_set_regs(r, vi_black || r[PORT_VI_CONTROL] == 0 ? NULL : fb);
}

void port_irq_vi(void) {
    host_interrupt();
    if (vi_next_fb != NULL)
        vi_cur_fb = vi_next_fb;
    vi_regs();
    if (vi_mq != NULL && vi_retrace != 0 && ++vi_count >= vi_retrace) {
        vi_count = 0;
        osSendMesg(vi_mq, vi_msg, OS_MESG_NOBLOCK);
    }
}

/* ---- SP/DP ---------------------------------------------------------------------------- */

static int dp_frozen;

void osDpSetStatus(u32 v) {
    if (v & DPC_SET_FREEZE)
        dp_frozen = 1;
    if (v & DPC_CLR_FREEZE)
        dp_frozen = 0;
}
u32 osDpGetStatus(void) { return dp_frozen ? DPC_STATUS_FREEZE : 0; }
void osDpGetCounters(u32 *c) { c[0] = c[1] = c[2] = c[3] = 0; }
s32 osDpSetNextBuffer(void *p, u64 n) { return 0; }

void osSpTaskLoad(OSTask *t) { }

void osSpTaskStartGo(OSTask *t) {
    if (t->t.type == M_GFXTASK) {
        host_gfx_ucode(port_addr((uintptr_t)t->t.ucode_boot), t->t.ucode_boot_size, port_addr((uintptr_t)t->t.ucode), t->t.ucode_size,
                       port_addr((uintptr_t)t->t.ucode_data), t->t.ucode_data_size, t->t.dram_stack_size);
        int sync = host_gfx_task(port_addr((uintptr_t)t->t.data_ptr), t->t.data_size);
        host_raise(OS_EVENT_SP);
        if (sync)
            host_raise(OS_EVENT_DP);
    } else {
        if (t->t.type == M_AUDTASK)
            host_audio_task(port_addr((uintptr_t)t->t.data_ptr), t->t.data_size,
                            port_addr((uintptr_t)t->t.ucode_data));
        host_raise(OS_EVENT_SP);
    }
}

void osSpTaskYield(void) { }
OSYieldResult osSpTaskYielded(OSTask *t) { return 0; }

/* ---- AI ----------------------------------------------------------------------------------- */

s32 osAiSetFrequency(u32 f) {
    u32 dacRate = (u32)((f32)osViClock / (f32)f + .5f);
    if (dacRate < 132)
        return -1;
    host_ai_set_dacrate(dacRate);
    return osViClock / (s32)dacRate;
}

s32 osAiSetNextBuffer(void *buf, u32 size) { return host_ai_submit(port_addr((uintptr_t)buf), size); }
u32 osAiGetLength(void) { return host_ai_length(); }
u32 osAiGetStatus(void) { return host_ai_status(); }
