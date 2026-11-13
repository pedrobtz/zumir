/* What every zumir MIR context gets (design §4.3, D4): MIR's error
 * function, with MIR's exact signature, and an allocator that raises the
 * trap instead of returning NULL. MIR checks some of its allocations and
 * not others; with this allocator it never sees a NULL, so a failed
 * allocation is a trapped MIR_alloc_error, never a NULL dereference inside
 * MIR (alloc-failure.yaml, Stage 3). A raise inside the allocator leaves
 * the field MIR was about to assign as it was, so the context can still be
 * finished. Use zumir_mir_new() under a trap; no R API here. */
#include "zumir_mir.h"

#if ZUMIR_HAVE_MIR
#include <stdlib.h>

#include "zumir_mir_api.h"
#include "zumir_trap.h"

void zumir_mir_error_func(MIR_error_type_t type, const char *format, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  zumir_mir_message_clear();
  zumir_trap_raise((int) type, 0, buf);
}

static void out_of_memory(void) {
  zumir_trap_raise(MIR_alloc_error, 0, "Not enough memory");
}

static void *zm_malloc(size_t size, void *user_data) {
  (void) user_data;
  void *p = malloc(size == 0 ? 1 : size);
  if (p == NULL) out_of_memory();
  return p;
}

static void *zm_calloc(size_t num, size_t size, void *user_data) {
  (void) user_data;
  void *p = calloc(num == 0 ? 1 : num, size == 0 ? 1 : size);
  if (p == NULL) out_of_memory();
  return p;
}

static void *zm_realloc(void *ptr, size_t old_size, size_t new_size, void *user_data) {
  (void) old_size;
  (void) user_data;
  void *p = realloc(ptr, new_size == 0 ? 1 : new_size);
  /* On failure ptr is still valid and still owned by MIR. */
  if (p == NULL) out_of_memory();
  return p;
}

static void zm_free(void *ptr, void *user_data) {
  (void) user_data;
  free(ptr);
}

static struct MIR_alloc zumir_alloc = {zm_malloc, zm_calloc, zm_realloc, zm_free, NULL};

MIR_context_t zumir_mir_new(void) {
  MIR_context_t ctx = MIR_init2(&zumir_alloc, NULL);
  /* NULL only on MIR_init2's version check, which one tree cannot fail. */
  if (ctx == NULL) zumir_trap_raise(MIR_alloc_error, 0, "MIR_init failed");
  MIR_set_error_func(ctx, zumir_mir_error_func);
  return ctx;
}
#else
/* ISO C forbids an empty translation unit. */
typedef int zumir_mir_glue_c_unused;
#endif
