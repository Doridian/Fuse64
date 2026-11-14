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

jmp_buf *port_task_save(int *buf);
void port_task_jump(int *buf, int v) __attribute__((noreturn));

#undef func_80297C10
#undef func_80297C7C
#define func_80297C10(buf) ((u32)setjmp(*port_task_save(buf)))
#define func_80297C7C(buf, v) port_task_jump((buf), (v))

#endif
