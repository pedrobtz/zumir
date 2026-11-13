/* The other end of zumir_mir_stdio.h: what MIR writes to stderr is kept in
 * a per-thread buffer, and MIR's exit and abort go to the error trap
 * (zumir_trap.h) with it as the message. This file uses the real stdio,
 * never sees the redirecting macros, and uses no R API. */
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "zumir_mir_sink.h"
#include "zumir_trap.h"

char zumir_mir_stderr_sink;

#define ZUMIR_MIR_MESSAGE_MAX 4096
static _Thread_local char message[ZUMIR_MIR_MESSAGE_MAX];
static _Thread_local size_t message_len;

static void message_append(const char *s, size_t n) {
  size_t room = sizeof(message) - 1 - message_len;
  if (n > room) n = room;
  memcpy(message + message_len, s, n);
  message_len += n;
  message[message_len] = '\0';
}

const char *zumir_mir_message(void) {
  message[message_len] = '\0';
  return message;
}

void zumir_mir_message_clear(void) {
  message_len = 0;
  message[0] = '\0';
}

int zumir_mir_vfprintf(FILE *f, const char *format, va_list ap) {
  if (f != ZUMIR_MIR_STDERR) return vfprintf(f, format, ap);
  char buf[1024];
  int n = vsnprintf(buf, sizeof(buf), format, ap);
  if (n > 0) message_append(buf, (size_t) n < sizeof(buf) ? (size_t) n : sizeof(buf) - 1);
  return n;
}

int zumir_mir_fprintf(FILE *f, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int n = zumir_mir_vfprintf(f, format, ap);
  va_end(ap);
  return n;
}

int zumir_mir_fputc(int c, FILE *f) {
  if (f != ZUMIR_MIR_STDERR) return fputc(c, f);
  char ch = (char) c;
  message_append(&ch, 1);
  return (unsigned char) c;
}

void zumir_mir_perror(const char *s) {
  const char *why = strerror(errno);
  if (s != NULL && *s != '\0') {
    message_append(s, strlen(s));
    message_append(": ", 2);
  }
  message_append(why, strlen(why));
  message_append("\n", 1);
}

void zumir_mir_exit(int status) {
  char buf[ZUMIR_MIR_MESSAGE_MAX];
  snprintf(buf, sizeof(buf), "%s", zumir_mir_message());
  /* Drop a trailing newline: the message becomes an R condition's. */
  size_t n = strlen(buf);
  while (n > 0 && buf[n - 1] == '\n') buf[--n] = '\0';
  zumir_mir_message_clear();
  zumir_trap_raise(ZUMIR_TRAP_EXIT, status, buf);
}
