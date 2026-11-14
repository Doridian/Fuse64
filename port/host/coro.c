/*
 * 530C0's tasks as coroutines (port/include/port_task.h has the model).
 *
 * The scheduler and its tasks all run on the game's main thread: the
 * scheduler is coroutine 0, that thread's own context, and each task one
 * with a host stack of its own.  A switch saves the running context and
 * resumes another (ucontext's swapcontext; Windows fibers; emscripten's
 * fibers), and the one resumed longjmps (the host's) to the buffer it was
 * asked for.  A switch happens inside the thread's fiber (threads.c), and
 * a task can block in libultra like any code of its thread: the thread's
 * fiber then yields from the task's stack, and resumes there.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

#if defined(_WIN32)
#include <windows.h>
#define CORO_WIN32 1
#elif defined(__EMSCRIPTEN__)
#include <emscripten/fiber.h>
#define CORO_EMSCRIPTEN 1
#elif defined(PORT_HAVE_UCONTEXT)
#include <ucontext.h>
#define CORO_UCONTEXT 1
#else
/* no ucontext (musl): a host thread per task, exactly one of them (or the
   thread that runs the scheduler) going at a time, as fiber_pthread.c's */
#include <pthread.h>
#define CORO_PTHREAD 1
#endif

#define CORO_STACK (256 * 1024)
#define MAX_BUFS 128
#define MARK 0x434F524Fu        /* "CORO": the buffer was saved since func_80238A10 set it up */

typedef struct Coro {
#if CORO_UCONTEXT
    ucontext_t uc;
#elif CORO_WIN32
    void *fiber;
#elif CORO_EMSCRIPTEN
    emscripten_fiber_t f;
    void *astack;
#elif CORO_PTHREAD
    pthread_t th;
    pthread_cond_t cv;
    int go, started;
    unsigned gen;               /* coro_start's count: a new run of the entry */
    jmp_buf top;                /* the thread's start, where a new run begins */
#endif
    void *stack;
    void (*entry)(void);
    int live;
} Coro;

typedef struct {
    int *buf;                   /* the game's buffer */
    jmp_buf jb;                 /* the host's, for it */
    Coro *owner;                /* the coroutine that saved it (main: coroutine 0) */
} Buf;

static Coro main_coro;
static Coro *cur = &main_coro;
static Buf bufs[MAX_BUFS];
static int nbufs;
static Buf *resume_buf;         /* what the coroutine switched to longjmps to */
static int resume_v;
static Coro *free_coros[MAX_BUFS];
static int nfree;

static Buf *find(int *buf, int create) {
    for (int i = 0; i < nbufs; i++)
        if (bufs[i].buf == buf)
            return &bufs[i];
    if (!create)
        return NULL;
    if (nbufs == MAX_BUFS)
        host_fatal("tasks: more than %d setjmp buffers", MAX_BUFS);
    bufs[nbufs].buf = buf;
    bufs[nbufs].owner = NULL;
    return &bufs[nbufs++];
}

jmp_buf *port_task_save(int *buf) {
    Buf *b = find(buf, 1);
    b->owner = cur;
    buf[0] = (int)MARK;
    return &b->jb;
}

/* ---- the switch ---------------------------------------------------------------- */

static void after_switch(void) {
    if (resume_buf) {
        Buf *b = resume_buf;
        resume_buf = NULL;
        longjmp(b->jb, resume_v);
    }
}

static void coro_main(void) {
    Coro *c = cur;
    /* a task that returns from its entry starts it again: on the N64 the
       entry's own $ra is the entry (func_80238A10 put it there) */
    for (;;)
        c->entry();
}

#if CORO_UCONTEXT
static void trampoline(void) { coro_main(); }

static void coro_start(Coro *c) {
    getcontext(&c->uc);
    c->uc.uc_stack.ss_sp = c->stack;
    c->uc.uc_stack.ss_size = CORO_STACK;
    c->uc.uc_link = NULL;
    makecontext(&c->uc, trampoline, 0);
}

static void coro_switch(Coro *to) {
    Coro *from = cur;
    cur = to;
    swapcontext(&from->uc, &to->uc);
}
#elif CORO_WIN32
static void CALLBACK fiber_proc(void *p) {
    (void)p;
    coro_main();
}

static void coro_start(Coro *c) {
    c->fiber = CreateFiberEx(64 * 1024, CORO_STACK, 0, fiber_proc, c);
    if (!c->fiber)
        host_fatal("tasks: CreateFiberEx failed");
}

static void coro_switch(Coro *to) {
    if (!main_coro.fiber)
        main_coro.fiber = GetCurrentFiber();
    cur = to;
    SwitchToFiber(to->fiber);
}
#elif CORO_EMSCRIPTEN
static void fiber_proc(void *p) {
    (void)p;
    coro_main();
}

static void coro_start(Coro *c) {
    if (!c->astack)
        c->astack = malloc(64 * 1024);
    emscripten_fiber_init(&c->f, fiber_proc, c, c->stack, CORO_STACK, c->astack, 64 * 1024);
}

static void coro_switch(Coro *to) {
    static char main_astack[64 * 1024];
    static int main_init;
    Coro *from = cur;
    if (!main_init) {
        emscripten_fiber_init_from_current_context(&main_coro.f, main_astack, sizeof main_astack);
        main_init = 1;
    }
    cur = to;
    emscripten_fiber_swap(&from->f, &to->f);
}
#elif CORO_PTHREAD
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;

/* with mu held: wait for the baton; a coroutine started again meanwhile
   (coro_start of a killed task's) begins its entry afresh */
static void wait_go(Coro *self) {
    unsigned gen = self->gen;
    while (!self->go)
        pthread_cond_wait(&self->cv, &mu);
    self->go = 0;
    if (self != &main_coro && self->gen != gen) {
        pthread_mutex_unlock(&mu);
        longjmp(self->top, 1);
    }
}

static void *thread_start(void *p) {
    Coro *c = p;
    pthread_mutex_lock(&mu);
    while (!c->go)
        pthread_cond_wait(&c->cv, &mu);
    c->go = 0;
    pthread_mutex_unlock(&mu);
    setjmp(c->top);
    cur = c;
    coro_main();
    return NULL;
}

static void coro_start(Coro *c) {
    if (!c->started) {
        pthread_attr_t a;
        pthread_cond_init(&c->cv, NULL);
        pthread_attr_init(&a);
        pthread_attr_setstacksize(&a, CORO_STACK);
        if (pthread_create(&c->th, &a, thread_start, c) != 0)
            host_fatal("tasks: pthread_create failed");
        pthread_attr_destroy(&a);
        c->started = 1;
    } else {
        c->gen++;
    }
}

static void coro_switch(Coro *to) {
    static int main_init;
    Coro *from = cur;
    pthread_mutex_lock(&mu);
    if (!main_init) {
        pthread_cond_init(&main_coro.cv, NULL);
        main_init = 1;
    }
    cur = to;
    to->go = 1;
    pthread_cond_signal(&to->cv);
    wait_go(from);
    cur = from;
    pthread_mutex_unlock(&mu);
}
#endif

void port_task_jump(int *buf, int v) {
    {
        /* PORT_HEAPCHECK=1: the game's heap checked at each switch */
        static int check = -1;
        int port_heap_check(void);
        if (check < 0) {
            const char *e = getenv("PORT_HEAPCHECK");
            check = e && *e && *e != '0';
        }
        if (check && port_heap_check())
            host_fatal("the heap went bad before a switch to task buffer %p (entry %p)", (void *)buf,
                       (void *)(uintptr_t)(uint32_t)buf[3]);
    }
    Buf *b = find(buf, 0);
    if (b == NULL || (unsigned)buf[0] != MARK) {
        /* a buffer func_80238A10 set up: a new task at its entry (word 3)
           on a stack of its own; the coroutine the buffer had (a task
           that was killed) is done with */
        Coro *c;
        if (b == NULL)
            b = find(buf, 1);
        if (b->owner && b->owner != &main_coro && b->owner != cur) {
            b->owner->live = 0;
            free_coros[nfree++] = b->owner;
        }
        if (nfree > 0) {
            c = free_coros[--nfree];
        } else {
            c = calloc(1, sizeof *c);
            if (!c)
                host_fatal("tasks: no memory");
            c->stack = malloc(CORO_STACK);
            if (!c->stack)
                host_fatal("tasks: no memory for a stack");
        }
        c->entry = (void (*)(void))(uintptr_t)(uint32_t)buf[3];
        if (host_verbose > 1)
            host_log("task %p: entry %p\n", (void *)buf, (void *)c->entry);
        c->live = 1;
        coro_start(c);
        b->owner = c;
        buf[0] = (int)MARK;
        resume_buf = NULL;
        coro_switch(c);
        after_switch();
        host_fatal("tasks: a coroutine resumed with nothing to do");
    }
    if (b->owner == cur)
        longjmp(b->jb, v);
    resume_buf = b;
    resume_v = v;
    coro_switch(b->owner);
    after_switch();
    host_fatal("tasks: a coroutine resumed with nothing to do");
}

/* whether p is on a task's stack (port/src/memory.c: a display list built
   in a local) */
int port_coro_stack(const void *p) {
    for (int i = 0; i < nbufs; i++) {
        Coro *c = bufs[i].owner;
        if (c && c->stack && (const char *)p >= (const char *)c->stack &&
            (const char *)p < (const char *)c->stack + CORO_STACK)
            return 1;
    }
    return 0;
}
