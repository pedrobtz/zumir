/* Included by the wrapper files just before MIR's sources: MIR's exit,
 * abort and writes to stderr go to zumir instead of ending or printing from
 * the R process (design §4.2 "No exit, no abort, no printing", D4).
 *
 * Every system header MIR includes is included first, so the macros below
 * reach MIR's own code and no declaration in a system header; sys/mman.h is
 * the exception, left to MIR's own include order because mir-code-alloc.h
 * defines MAP_FAILED before it and declares none of the names below. Then:
 *
 *   exit (s), abort ()   call zumir_mir_exit (), in zumir_mir_stdio.c;
 *   stderr               is a sink object that is not a FILE, and the
 *                        stdio writers MIR uses (fprintf, vfprintf, fputc,
 *                        perror) are zumir's, which append what is written
 *                        to the sink to the thread's message buffer and pass
 *                        any other FILE (MIR_output's caller's) to stdio.
 *
 * MIR's sources are unchanged by this; the redirection lives here and in
 * zumir_mir_stdio.c alone. sprintf is not redirected: MIR's calls are
 * snprintf on the fork (src/vendor/PROVENANCE). */
#ifndef ZUMIR_MIR_STDIO_H
#define ZUMIR_MIR_STDIO_H

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#if defined(__APPLE__) && defined(__aarch64__)
#include <libkern/OSCacheControl.h>
#include <pthread.h>
#endif

#include "zumir_mir_sink.h"

#undef stderr
#define stderr ZUMIR_MIR_STDERR
#define exit(status) zumir_mir_exit(status)
#define abort() zumir_mir_exit(134)
#define fprintf(...) zumir_mir_fprintf(__VA_ARGS__)
#define vfprintf(f, format, ap) zumir_mir_vfprintf(f, format, ap)
#define fputc(c, f) zumir_mir_fputc(c, f)
#define perror(s) zumir_mir_perror(s)

#endif
