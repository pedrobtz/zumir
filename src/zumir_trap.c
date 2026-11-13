/* The error trap; see zumir_trap.h. No R API here. */
#include <stdio.h>
#include <string.h>

#include "zumir_trap.h"

static _Thread_local zumir_trap *top;

void (*zumir_trap_unhandled)(int type, int status, const char *message);

void zumir_trap_push(zumir_trap *t) {
  t->prev = top;
  t->type = 0;
  t->status = 0;
  t->message[0] = '\0';
  top = t;
}

void zumir_trap_pop(zumir_trap *t) {
  /* Pops t and anything pushed above it that was not popped. */
  if (top == t) top = t->prev;
}

int zumir_trap_depth(void) {
  int n = 0;
  for (zumir_trap *t = top; t != NULL; t = t->prev) n++;
  return n;
}

void zumir_trap_raise(int type, int status, const char *message) {
  zumir_trap *t = top;
  if (t == NULL) {
    if (zumir_trap_unhandled != NULL) zumir_trap_unhandled(type, status, message);
    /* Unreachable in the package, whose handler raises an R error. Spinning
     * keeps MIR from running on with a context it has given up on; ending
     * the process is the thing this file exists to prevent. */
    for (;;) {
    }
  }
  t->type = type;
  t->status = status;
  snprintf(t->message, sizeof(t->message), "%s", message == NULL ? "" : message);
  top = t->prev;
  longjmp(t->env, 1);
}

void zumir_mir_error_func(int type, const char *format, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  zumir_mir_message_clear();
  zumir_trap_raise(type, 0, buf);
}
