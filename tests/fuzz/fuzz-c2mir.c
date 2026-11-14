/* libFuzzer target over cfunction()'s compile path (design §8, Stage 8),
 * built without R by fuzz.yaml: each input is C source, compiled by c2mir
 * from memory after cfunction()'s prelude, with no header read from a
 * file, then linked on the interpreter against D6's list. The code is
 * never run.
 *
 * Oracles beyond "no crash":
 *   - the same input compiled twice gives the same verdict, and when it
 *     compiles, the same module as MIR prints it (c2mir keeps no state
 *     between contexts);
 *   - afterwards no arena is live and the trap stack is empty, whether the
 *     input compiled, failed to compile, or failed to link (D4).
 * A broken oracle is __builtin_trap(), which libFuzzer reports. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *memstream(char **buf, size_t *len) {
  return open_memstream(buf, len);
}

#include "zumir_imports.h"
#include "zumir_mir_api.h"
#include "zumir_mir_sink.h"
#include "zumir_prelude.h"
#include "zumir_trap.h"
#include "vendor/mir/c2mir/c2mir.h"

#define SOURCE_PARTS 5
typedef struct {
  const char *part[SOURCE_PARTS];
  int k;
  size_t pos;
} source_reader;

static int source_getc(void *data) {
  source_reader *r = data;
  while (r->k < SOURCE_PARTS) {
    unsigned char c = (unsigned char) r->part[r->k][r->pos];
    if (c != '\0') {
      r->pos++;
      return c;
    }
    r->k++;
    r->pos = 0;
  }
  return EOF;
}

enum { V_COMPILED = 0, V_COMPILE_ERROR = 1, V_TRAPPED = 2 };

/* One compile; *text is the module as MIR prints it (malloc'd) when the
 * verdict is V_COMPILED. */
static int compile_once(const char *src, char **text) {
  MIR_context_t volatile ctx = NULL;
  zumir_arena *volatile arena = NULL;
  volatile int verdict = V_TRAPPED;
  *text = NULL;
  zumir_trap t;
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    ctx = zumir_mir_new(&arena);
    zumir_mir_message_clear();
    c2mir_init(ctx);
    struct c2mir_options options;
    memset(&options, 0, sizeof options);
    options.message_file = ZUMIR_MIR_STDERR;
    source_reader r = {{zumir_prelude_prefix, "fuzz", zumir_prelude_line0, src, "\n"}, 0, 0};
    int ok = c2mir_compile(ctx, &options, source_getc, &r, "fuzz", NULL);
    c2mir_finish(ctx);
    if (ok) {
      char *buf = NULL;
      size_t len = 0;
      FILE *f = memstream(&buf, &len);
      if (f != NULL) {
        MIR_output(ctx, f);
        fclose(f);
        *text = buf;
      }
      for (MIR_module_t m = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx)); m != NULL;
           m = DLIST_NEXT(MIR_module_t, m))
        MIR_load_module(ctx, m);
      size_t n;
      const zumir_import_entry *d6 = zumir_import_d6(&n);
      for (size_t i = 0; i < n; i++) MIR_load_external(ctx, d6[i].name, d6[i].addr);
      MIR_link(ctx, MIR_set_interp_interface, NULL);
    }
    verdict = ok ? V_COMPILED : V_COMPILE_ERROR;
    zumir_trap_pop(&t);
  } else {
    /* MIR raised (a link error, an allocation): the context is not
     * finished, its arena is released whole. */
    if (arena != NULL) zumir_arena_mark_failed(arena);
    verdict = V_TRAPPED;
  }
  if (ctx != NULL || arena != NULL) zumir_mir_free(ctx, arena, 0);
  return verdict;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size > 65536) return 0;
  char *src = malloc(size + 1);
  if (src == NULL) return 0;
  memcpy(src, data, size);
  src[size] = '\0';
  char *a = NULL, *b = NULL;
  int va = compile_once(src, &a);
  int vb = compile_once(src, &b);
  if (va != vb) __builtin_trap();
  if (va == V_COMPILED && (a == NULL) != (b == NULL)) __builtin_trap();
  if (a != NULL && b != NULL && strcmp(a, b) != 0) __builtin_trap();
  if (zumir_mir_live_arenas() != 0 || zumir_trap_depth() != 0) __builtin_trap();
  free(a);
  free(b);
  free(src);
  return 0;
}
