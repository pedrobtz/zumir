/* What every zumir MIR context gets (design §4.3, D4): MIR's error
 * function, with MIR's exact signature, and an arena: a heap allocator and
 * a code allocator that record everything they hand MIR.
 *
 * The heap allocator raises the trap instead of returning NULL. MIR checks
 * some of its allocations and not others, so with a NULL it would
 * dereference it; with this allocator a failed allocation is always a
 * trapped MIR_alloc_error (alloc-failure.yaml, Stage 3).
 *
 * After such a failure the context is not finished with MIR_finish: MIR
 * may have published an object it had not finished building (new_func_arr
 * adds the function to its module before allocating its variables), and
 * MIR_finish would walk it. zumir_mir_free() releases the arena's blocks
 * and code mappings directly instead. A context that failed with any other
 * MIR error is finished by MIR_finish, under a trap; if that raises too, the
 * arena releases what is left.
 *
 * The arena also counts what each context holds, for zumir_info()
 * (Stage 4). No R API here. */
#include "zumir_mir.h"

#if ZUMIR_HAVE_MIR
#include <stddef.h>
#include <stdlib.h>

#include "zumir_mir_api.h"
#include "vendor/mir/mir-gen.h"
#include "zumir_trap.h"

/* zumir_mir.c: MIR's own code allocator, static in mir.c's unit. */
MIR_code_alloc_t zumir_mir_default_code_alloc(void);

void zumir_mir_error_func(MIR_error_type_t type, const char *format, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  zumir_mir_message_clear();
  zumir_trap_raise((int) type, 0, buf);
}

/* A heap block: this header, then MIR's bytes. The union keeps MIR's part
 * aligned as malloc would. */
typedef union zumir_block {
  struct {
    union zumir_block *prev, *next;
    size_t size;
  } h;
  max_align_t align;
} zumir_block;

typedef struct zumir_mapping {
  struct zumir_mapping *next;
  void *addr;
  size_t len;
} zumir_mapping;

struct zumir_arena {
  struct MIR_alloc alloc; /* first: MIR_get_alloc (ctx) is the arena */
  struct MIR_code_alloc code_alloc;
  MIR_code_alloc_t real_code_alloc;
  zumir_block head; /* circular list of live blocks */
  zumir_mapping *mappings;
  size_t bytes, blocks;
  int failed;    /* an allocation failed: never MIR_finish */
  int map_errno; /* errno of the last code mapping refused, or 0 */
};

int zumir_arena_map_errno(const zumir_arena *a) {
  return a->map_errno;
}

#include <errno.h>
#include <stdatomic.h>
static _Atomic long live_arenas;
static _Atomic long long live_bytes;

long zumir_mir_live_arenas(void) {
  return live_arenas;
}

double zumir_mir_live_bytes(void) {
  return (double) live_bytes;
}

size_t zumir_arena_bytes(const zumir_arena *a) {
  return a->bytes;
}

/* Test hook (zumir_test_fail_alloc): when fail_at is n > 0, the n-th heap
 * allocation from here on fails, as if malloc had returned NULL. One
 * thread's tests use it at a time. */
static long fail_at, alloc_count;

void zumir_mir_fail_alloc_at(long n) {
  fail_at = n;
  alloc_count = 0;
}

static int injected_failure(void) {
  return fail_at > 0 && ++alloc_count == fail_at;
}

static void out_of_memory(zumir_arena *a) {
  a->failed = 1;
  zumir_trap_raise(MIR_alloc_error, 0, "Not enough memory");
}

static void link_block(zumir_arena *a, zumir_block *b, size_t size) {
  b->h.size = size;
  b->h.next = a->head.h.next;
  b->h.prev = &a->head;
  a->head.h.next->h.prev = b;
  a->head.h.next = b;
  a->bytes += size;
  a->blocks++;
  live_bytes += (long long) size;
}

static void unlink_block(zumir_arena *a, zumir_block *b) {
  b->h.prev->h.next = b->h.next;
  b->h.next->h.prev = b->h.prev;
  a->bytes -= b->h.size;
  a->blocks--;
  live_bytes -= (long long) b->h.size;
}

static void *zm_malloc(size_t size, void *user_data) {
  zumir_arena *a = user_data;
  zumir_block *b = injected_failure() ? NULL : malloc(sizeof(zumir_block) + size);
  if (b == NULL) out_of_memory(a);
  link_block(a, b, size);
  return b + 1;
}

static void *zm_calloc(size_t num, size_t size, void *user_data) {
  zumir_arena *a = user_data;
  if (size != 0 && num > (SIZE_MAX - sizeof(zumir_block)) / size) out_of_memory(a);
  zumir_block *b = injected_failure() ? NULL : calloc(1, sizeof(zumir_block) + num * size);
  if (b == NULL) out_of_memory(a);
  link_block(a, b, num * size);
  return b + 1;
}

static void *zm_realloc(void *ptr, size_t old_size, size_t new_size, void *user_data) {
  zumir_arena *a = user_data;
  (void) old_size;
  if (ptr == NULL) return zm_malloc(new_size, user_data);
  zumir_block *b = (zumir_block *) ptr - 1;
  /* Unlinked while realloc may move it; on failure the old block is still
   * valid and still MIR's, so it goes back on the list before the raise. */
  unlink_block(a, b);
  zumir_block *nb = injected_failure() ? NULL : realloc(b, sizeof(zumir_block) + new_size);
  if (nb == NULL) {
    link_block(a, b, b->h.size);
    out_of_memory(a);
  }
  link_block(a, nb, new_size);
  return nb + 1;
}

static void zm_free(void *ptr, void *user_data) {
  zumir_arena *a = user_data;
  if (ptr == NULL) return;
  zumir_block *b = (zumir_block *) ptr - 1;
  unlink_block(a, b);
  free(b);
}

/* Code memory: MIR's own allocator, with each mapping recorded. A mapping
 * that cannot be recorded is released and reported as failed, so nothing
 * mapped is ever untracked. */
static void *zm_mem_map(size_t len, void *user_data) {
  zumir_arena *a = user_data;
  MIR_code_alloc_t real = a->real_code_alloc;
  zumir_mapping *m = malloc(sizeof(*m));
  if (m == NULL) return MAP_FAILED;
  errno = 0;
  void *addr = real->mem_map(len, real->user_data);
  if (addr == MAP_FAILED) {
    a->map_errno = errno;
    free(m);
    return MAP_FAILED;
  }
  m->addr = addr;
  m->len = len;
  m->next = a->mappings;
  a->mappings = m;
  return addr;
}

static int zm_mem_unmap(void *addr, size_t len, void *user_data) {
  zumir_arena *a = user_data;
  MIR_code_alloc_t real = a->real_code_alloc;
  for (zumir_mapping **p = &a->mappings; *p != NULL; p = &(*p)->next) {
    if ((*p)->addr == addr) {
      zumir_mapping *m = *p;
      *p = m->next;
      free(m);
      break;
    }
  }
  return real->mem_unmap(addr, len, real->user_data);
}

static int zm_mem_protect(void *addr, size_t len, MIR_mem_protect_t prot, void *user_data) {
  zumir_arena *a = user_data;
  MIR_code_alloc_t real = a->real_code_alloc;
  errno = 0;
  int r = real->mem_protect(addr, len, prot, real->user_data);
  if (r != 0) a->map_errno = errno;
  return r;
}

MIR_context_t zumir_mir_new(zumir_arena *volatile *arena) {
  zumir_arena *a = calloc(1, sizeof(*a));
  if (a == NULL) zumir_trap_raise(MIR_alloc_error, 0, "Not enough memory");
  a->alloc = (struct MIR_alloc){zm_malloc, zm_calloc, zm_realloc, zm_free, a};
  a->real_code_alloc = zumir_mir_default_code_alloc();
  a->code_alloc = (struct MIR_code_alloc){zm_mem_map, zm_mem_unmap, zm_mem_protect, a};
  a->head.h.next = a->head.h.prev = &a->head;
  live_arenas++;
  /* Visible to the caller before MIR runs, so a raise inside MIR_init2
   * still leaves the arena to release. */
  *arena = a;
  MIR_context_t ctx = MIR_init2(&a->alloc, &a->code_alloc);
  /* NULL only on MIR_init2's version check, which one tree cannot fail. */
  if (ctx == NULL) zumir_trap_raise(MIR_alloc_error, 0, "MIR_init failed");
  MIR_set_error_func(ctx, zumir_mir_error_func);
  return ctx;
}

int zumir_mir_free(MIR_context_t ctx, zumir_arena *a, int gen) {
  volatile int finished = 1;
  if (a == NULL) return 1;
  if (ctx != NULL && !a->failed) {
    zumir_trap t;
    zumir_trap_push(&t);
    if (setjmp(t.env) == 0) {
      if (gen) MIR_gen_finish(ctx);
      MIR_finish(ctx);
      zumir_trap_pop(&t);
    } else {
      finished = 0;
    }
  } else if (ctx != NULL) {
    finished = 0;
  }
  /* Whatever MIR did not free: everything, after a failed allocation. */
  while (a->mappings != NULL) {
    zumir_mapping *m = a->mappings;
    a->mappings = m->next;
    a->real_code_alloc->mem_unmap(m->addr, m->len, a->real_code_alloc->user_data);
    free(m);
  }
  while (a->head.h.next != &a->head) {
    zumir_block *b = a->head.h.next;
    unlink_block(a, b);
    free(b);
  }
  free(a);
  live_arenas--;
  return finished;
}
#else
/* ISO C forbids an empty translation unit. */
typedef int zumir_mir_glue_c_unused;
#endif
