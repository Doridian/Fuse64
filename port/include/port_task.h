/*
 * 530C0's cooperative tasks in the port (docs/PORT.md, "The tasks").
 *
 * On the N64 a task is a setjmp buffer (B2410's func_80297C10, setjmp, and
 * func_80297C7C, longjmp): func_80238A10 makes one by writing its entry and
 * the top of its stack into the buffer as $ra and $sp, and the first
 * longjmp to it starts the entry on that stack.  The scheduler
 * (func_802392F8) and the tasks then switch with setjmp and longjmp.
 *
 * In the port each task is a coroutine with a host stack of its own
 * (port/host/coro.c).  func_80297C10 is the host's setjmp, in the caller's
 * frame as the original is (a macro: port_task_save records which
 * coroutine the buffer is now, and gives the host's jmp_buf for it);
 * func_80297C7C(buf, v) switches to the coroutine that saved buf, which
 * longjmps there with v, or, for a buffer func_80238A10 has just set up,
 * starts a new coroutine at its entry.  The game's buffer keeps what the
 * game put there (the entry and the stack top, which func_80239208's stack
 * check reads); word 0, which func_80238A10 clears, is the port's mark of
 * a buffer saved since.
 */
#ifndef PORT_TASK_H
#define PORT_TASK_H

#include <setjmp.h>

#if defined(_WIN32) && defined(__x86_64__)
/* x86-64 Windows: the CRT's setjmp with no frame, whose longjmp restores
   the registers without unwinding (MinGW's __USE_MINGW_SETJMP_NON_SEH
   setjmp).  Not the compiler's: on Win64 clang's __builtin_longjmp puts
   back the frame pointer less the offset the prologue gave it (rbp is
   rsp plus the frame's size there), so the setjmp's function restored
   xmm6-xmm15 and its locals from the wrong place: a float the game kept
   across a task switch (9BFD0's fades) came back 0. */
typedef jmp_buf port_jmp_buf;
#define PORT_SETJMP(b) _setjmp((b), NULL)
#define PORT_LONGJMP(b, v) longjmp((b), (v))
#elif defined(_WIN32)
/* MinGW's i386 longjmp unwinds the SEH chain, which a switch of fibers
   leaves as another stack's: the compiler's own setjmp there, whose
   longjmp passes 1 (the value goes beside it) */
typedef void *port_jmp_buf[5];
extern int port_task_v;
#define PORT_SETJMP(b) (__builtin_setjmp(b) ? port_task_v : 0)
#define PORT_LONGJMP(b, v) (port_task_v = (v) ? (v) : 1, __builtin_longjmp((b), 1))
#else
typedef jmp_buf port_jmp_buf;
#define PORT_SETJMP(b) setjmp(b)
#define PORT_LONGJMP(b, v) longjmp((b), (v))
#endif

port_jmp_buf *port_task_save(intptr_t *buf);
void port_task_jump(intptr_t *buf, int v) __attribute__((noreturn));

#undef func_80297C10
#undef func_80297C7C
#define func_80297C10(buf) ((u32)PORT_SETJMP(*port_task_save(buf)))
#define func_80297C7C(buf, v) port_task_jump((buf), (v))

#endif
