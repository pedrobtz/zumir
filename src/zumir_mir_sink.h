/* Where MIR's exit, abort and stderr go (zumir_mir_stdio.h). Declared
 * apart from the redirecting macros so zumir's own files can use the
 * message buffer without them. */
#ifndef ZUMIR_MIR_SINK_H
#define ZUMIR_MIR_SINK_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

/* Not a FILE: only its address is used, to tell MIR's stderr from a FILE
 * that a caller of MIR_output passed. */
extern char zumir_mir_stderr_sink;
#define ZUMIR_MIR_STDERR ((FILE *) (void *) &zumir_mir_stderr_sink)

#if defined(__GNUC__)
#define ZUMIR_NORETURN __attribute__((noreturn))
#else
#define ZUMIR_NORETURN
#endif

void zumir_mir_exit(int status) ZUMIR_NORETURN;
int zumir_mir_fprintf(FILE *f, const char *format, ...);
int zumir_mir_vfprintf(FILE *f, const char *format, va_list ap);
int zumir_mir_fputc(int c, FILE *f);
void zumir_mir_perror(const char *s);

/* What MIR wrote to stderr on this thread since the last clear, truncated
 * to the buffer, NUL-terminated. */
const char *zumir_mir_message(void);
void zumir_mir_message_clear(void);

#endif
