/* The error trap (design §4.2, §6, D4, D20): MIR never ends, prints from or
 * crashes the R process on its own account.
 *
 * Every zumir entry that calls MIR is a recovery point. It pushes a trap on
 * this thread's stack, calls setjmp() on it in its own frame, calls MIR,
 * and pops the trap before returning:
 *
 *   zumir_trap t;
 *   zumir_trap_push(&t);
 *   if (setjmp(t.env) == 0) {
 *     ... MIR calls ...
 *     zumir_trap_pop(&t);
 *   } else {
 *     ... t.type, t.status and t.message say what MIR reported; the trap
 *     is already popped, and the context is failed: it may only be
 *     finished, under a trap of its own ...
 *   }
 *
 * MIR's error function (zumir_mir_error_func in zumir_mir_glue.c, installed
 * on every context by zumir_mir_new()) and MIR's exit and abort (zumir_mir_exit, by the
 * macros in zumir_mir_stdio.h) end in zumir_trap_raise(), which copies the
 * message into the innermost trap, pops it and longjmps there. The stack is
 * thread-local, so an error on one thread lands in that thread's trap.
 *
 * Nothing between the setjmp() and the longjmp may allocate R memory or
 * call R: the jump skips R's own frames otherwise. Locals the else branch
 * reads after being changed under the trap must be volatile.
 *
 * This file and zumir_trap.c use no R API, so the C tests under tests/c
 * link them without R. */
#ifndef ZUMIR_TRAP_H
#define ZUMIR_TRAP_H

#include <setjmp.h>
#include <stdarg.h>

#include "zumir_mir_sink.h"

/* t.type for MIR's exit or abort, which carry no MIR_error_type_t. */
#define ZUMIR_TRAP_EXIT (-1)

typedef struct zumir_trap {
  jmp_buf env;
  struct zumir_trap *prev;
  int type;   /* MIR_error_type_t, or ZUMIR_TRAP_EXIT */
  int status; /* exit's status; 0 for an error */
  char message[1024];
} zumir_trap;

void zumir_trap_push(zumir_trap *t);
void zumir_trap_pop(zumir_trap *t);
int zumir_trap_depth(void);

/* Records type, status and message in the innermost trap, pops it and
 * longjmps to it. With no trap on this thread, calls zumir_trap_unhandled,
 * which the package sets to raise an R error (R_init_zumir); it must not
 * return. */
void zumir_trap_raise(int type, int status, const char *message) ZUMIR_NORETURN;
extern void (*zumir_trap_unhandled)(int type, int status, const char *message);

#endif
