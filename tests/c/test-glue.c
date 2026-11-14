/* zumir's R-free C, each part against what it must give (Stage 11): the
 * error trap's stack, MIR's stderr buffer and exit, the arena's counts
 * through a context's life and through every allocation failure in turn,
 * code generated and run on both tiers, and D6's import list. Built
 * without R by tools/ci/platform.sh, and by mutation.yaml with Mull, for
 * which it is the test that kills a mutant by exiting non-zero:
 *
 *   cc -Isrc -Isrc/vendor/mir -DNDEBUG tests/c/test-glue.c src/zumir_mir.c \
 *     src/zumir_mir_gen.c src/zumir_c2mir.c src/zumir_prelude.c \
 *     src/zumir_mir_glue.c src/zumir_mir_stdio.c src/zumir_trap.c \
 *     src/zumir_imports_d6.c -lm
 *
 * Exit status 0 when every check holds; each failed check is printed. */
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zumir_imports.h"
#include "zumir_prelude.h"
#include "zumir_mir_api.h"
#include "zumir_trap.h"
#include "vendor/mir/mir-gen.h"
#include "vendor/mir/c2mir/c2mir.h"

static int failures;

#define CHECK(cond)                                                    \
  do {                                                                 \
    if (!(cond)) {                                                     \
      failures++;                                                      \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
    }                                                                  \
  } while (0)

/* ---- the trap's stack ---------------------------------------------- */

static jmp_buf unhandled_env;
static int unhandled_type, unhandled_status;
static char unhandled_message[64];

static void unhandled(int type, int status, const char *message) {
  unhandled_type = type;
  unhandled_status = status;
  snprintf(unhandled_message, sizeof unhandled_message, "%s", message);
  longjmp(unhandled_env, 1);
}

static void test_trap(void) {
  CHECK(zumir_trap_depth() == 0);
  zumir_trap outer, inner;
  zumir_trap_push(&outer);
  zumir_trap_push(&inner);
  CHECK(zumir_trap_depth() == 2);
  volatile int landed = 0;
  if (setjmp(inner.env) == 0) {
    zumir_trap_raise(7, 3, "inner \n\t\r ");
  } else {
    landed = 1;
  }
  /* The raise lands in the innermost trap, pops it, and records what was
   * raised, trailing whitespace off. */
  CHECK(landed == 1);
  CHECK(inner.type == 7 && inner.status == 3);
  CHECK(strcmp(inner.message, "inner") == 0);
  CHECK(zumir_trap_depth() == 1);
  zumir_trap_pop(&outer);
  CHECK(zumir_trap_depth() == 0);

  /* A long message is cut to the buffer, NUL-terminated; NULL is "". */
  char big[3000];
  memset(big, 'm', sizeof big - 1);
  big[sizeof big - 1] = '\0';
  zumir_trap t;
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) zumir_trap_raise(1, 0, big);
  CHECK(strlen(t.message) == sizeof t.message - 1);
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) zumir_trap_raise(1, 0, NULL);
  CHECK(t.message[0] == '\0');

  /* Popping a trap pops what was pushed above it and not popped, so no
   * trap left on the stack outlives its frame; popping one that is not
   * on the stack changes nothing. */
  zumir_trap a, b, c;
  zumir_trap_push(&a);
  zumir_trap_push(&b);
  zumir_trap_push(&c);
  zumir_trap_pop(&b);
  CHECK(zumir_trap_depth() == 1);
  zumir_trap_pop(&c);
  CHECK(zumir_trap_depth() == 1);
  zumir_trap_pop(&a);
  CHECK(zumir_trap_depth() == 0);
  /* A raise with no trap goes to zumir_trap_unhandled (the package's
   * raises an R error), with what was raised. */
  zumir_trap_unhandled = unhandled;
  if (setjmp(unhandled_env) == 0) zumir_trap_raise(5, 2, "nowhere");
  CHECK(unhandled_type == 5 && unhandled_status == 2);
  CHECK(strcmp(unhandled_message, "nowhere") == 0);
  zumir_trap_unhandled = NULL;
  /* A push resets what an earlier use left in the trap. */
  zumir_trap_push(&t);
  CHECK(t.type == 0 && t.status == 0 && t.message[0] == '\0');
  zumir_trap_pop(&t);
}

/* ---- MIR's stderr and exit ------------------------------------------ */

static void test_stdio(void) {
  zumir_mir_message_clear();
  CHECK(strcmp(zumir_mir_message(), "") == 0);
  CHECK(zumir_mir_fprintf(ZUMIR_MIR_STDERR, "a%d", 5) == 2);
  CHECK(zumir_mir_fputc('x', ZUMIR_MIR_STDERR) == 'x');
  errno = ENOENT;
  zumir_mir_perror("ctx");
  char want[512];
  snprintf(want, sizeof want, "a5xctx: %s\n", strerror(ENOENT));
  CHECK(strcmp(zumir_mir_message(), want) == 0);
  errno = ENOENT;
  zumir_mir_perror("");
  snprintf(want + strlen(want), sizeof want - strlen(want), "%s\n", strerror(ENOENT));
  CHECK(strcmp(zumir_mir_message(), want) == 0);
  zumir_mir_message_clear();
  CHECK(strcmp(zumir_mir_message(), "") == 0);

  /* One write is cut at 1023 bytes, and the buffer at 4095. */
  char line[2000];
  memset(line, 'q', sizeof line - 1);
  line[sizeof line - 1] = '\0';
  CHECK(zumir_mir_fprintf(ZUMIR_MIR_STDERR, "%s", line) == (int) sizeof line - 1);
  CHECK(strlen(zumir_mir_message()) == 1023);
  for (int i = 0; i < 5; i++) zumir_mir_fprintf(ZUMIR_MIR_STDERR, "%s", line);
  CHECK(strlen(zumir_mir_message()) == 4095);
  /* Around the cut: 1023, 1024 and 1025 bytes keep 1023, and what is
   * written next follows them. */
  for (int len = 1023; len <= 1025; len++) {
    zumir_mir_message_clear();
    zumir_mir_fprintf(ZUMIR_MIR_STDERR, "%.*s", len, line);
    zumir_mir_fputc('z', ZUMIR_MIR_STDERR);
    const char *m = zumir_mir_message();
    CHECK(strlen(m) == 1024 && m[1023] == 'z');
  }
  zumir_mir_message_clear();

  /* A FILE that is not MIR's stderr gets the bytes; the buffer does not. */
  /* tmpfile() can be refused on Windows (it writes to the drive's root);
   * the check is then skipped, not failed. */
  FILE *f = tmpfile();
  if (f == NULL) printf("skip: tmpfile() refused, the FILE check not run\n");
  if (f != NULL) {
    zumir_mir_fprintf(f, "to-file %d", 9);
    zumir_mir_fputc('!', f);
    rewind(f);
    char got[64] = {0};
    CHECK(fgets(got, sizeof got, f) != NULL);
    CHECK(strcmp(got, "to-file 9!") == 0);
    fclose(f);
  }
  CHECK(strcmp(zumir_mir_message(), "") == 0);

  /* exit() is a raise of ZUMIR_TRAP_EXIT with the buffer as its message,
   * its newlines off, and the buffer cleared. */
  zumir_mir_fprintf(ZUMIR_MIR_STDERR, "bad thing\n\n");
  zumir_trap t;
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) zumir_mir_exit(3);
  CHECK(t.type == ZUMIR_TRAP_EXIT && t.status == 3);
  CHECK(strcmp(t.message, "bad thing") == 0);
  CHECK(strcmp(zumir_mir_message(), "") == 0);
  CHECK(zumir_trap_depth() == 0);
}

/* ---- contexts, arenas and generated code ---------------------------- */

static const char *add_text =
  "m: module\n"
  "  export add\n"
  "add: func i64, i64:a, i64:b\n"
  "  local i64:r\n"
  "  add r, a, b\n"
  "  ret r\n"
  "  endfunc\n"
  "endmodule\n";

/* Builds `add`, links it on the tier and calls add(5, 7). Under a trap
 * the caller set; returns the result. */
static int64_t build_and_call(MIR_context_t ctx, int native) {
  MIR_scan_string(ctx, add_text);
  MIR_module_t m = DLIST_TAIL(MIR_module_t, *MIR_get_module_list(ctx));
  MIR_item_t f = DLIST_TAIL(MIR_item_t, m->items);
  while (f != NULL && f->item_type != MIR_func_item) f = DLIST_PREV(MIR_item_t, f);
  MIR_load_module(ctx, m);
  if (native) {
    MIR_gen_init(ctx);
    MIR_link(ctx, MIR_set_gen_interface, NULL);
    int64_t (*fn)(int64_t, int64_t);
    void *addr = MIR_gen(ctx, f);
    memcpy(&fn, &addr, sizeof fn);
    return fn(5, 7);
  }
  MIR_link(ctx, MIR_set_interp_interface, NULL);
  MIR_val_t res, args[2];
  args[0].i = 5;
  args[1].i = 7;
  MIR_interp_arr(ctx, f, &res, 2, args);
  return res.i;
}

static void test_context(int native) {
  CHECK(zumir_mir_live_arenas() == 0);
  MIR_context_t volatile ctx = NULL;
  zumir_arena *volatile arena = NULL;
  volatile int64_t r = 0;
  int owner = 0;
  zumir_trap t;
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    ctx = zumir_mir_new(&arena);
    CHECK(zumir_mir_live_arenas() == 1);
    CHECK(zumir_arena_bytes(arena) > 0);
    CHECK(zumir_mir_live_bytes() == (double) zumir_arena_bytes(arena));
    zumir_arena_set_owner(arena, &owner);
    CHECK(zumir_mir_owner(ctx) == &owner);
    size_t before = zumir_arena_bytes(arena);
    r = build_and_call(ctx, native);
    CHECK(zumir_arena_bytes(arena) > before);
    zumir_trap_pop(&t);
  } else {
    printf("raised: %d %s\n", t.type, t.message);
    CHECK(0);
  }
  CHECK(r == 12);
  CHECK(zumir_arena_map_errno(arena) == 0);
  if (native) CHECK(zumir_arena_mappings(arena) > 0 && zumir_arena_mappings(arena) < 100);
  long stray = zumir_mir_stray_mappings();
  CHECK(zumir_mir_free(ctx, arena, native) == 1);
  CHECK(zumir_mir_live_arenas() == 0);
  CHECK(zumir_mir_live_bytes() == 0);
  /* MIR_finish unmapped its code itself, through the arena, so nothing
   * was left for zumir_mir_free() to unmap. */
  CHECK(zumir_mir_stray_mappings() == stray);
  /* NULL is nothing to free. */
  CHECK(zumir_mir_free(NULL, NULL, 0) == 1);
}

/* A context MIR raised on is marked failed and freed without MIR_finish:
 * zumir_mir_free() says it did not finish, and nothing is left. */
static void test_failed_context(void) {
  MIR_context_t volatile ctx = NULL;
  zumir_arena *volatile arena = NULL;
  zumir_trap t;
  zumir_trap_push(&t);
  volatile int raised = 0;
  if (setjmp(t.env) == 0) {
    ctx = zumir_mir_new(&arena);
    MIR_scan_string(ctx, "m: module\n  f: func i64\n  bogus\n endfunc\nendmodule\n");
    zumir_trap_pop(&t);
  } else {
    raised = 1;
  }
  CHECK(raised == 1);
  CHECK(t.type == MIR_syntax_error);
  CHECK(strstr(t.message, "bogus") != NULL);
  zumir_arena_mark_failed(arena);
  CHECK(zumir_mir_free(ctx, arena, 0) == 0);
  CHECK(zumir_mir_live_arenas() == 0);
  CHECK(zumir_mir_live_bytes() == 0);
}

/* Each heap allocation of a context's life fails in turn: every failure
 * is a MIR_alloc_error raised into the trap, the context is then freed
 * without finishing, and nothing is left; the first run with no failure
 * gives the answer. */
static void test_alloc_failures(int native) {
  int seen = 0, done = 0;
  for (long n = 1; n < 100000 && !done; n++) {
    MIR_context_t volatile ctx = NULL;
    zumir_arena *volatile arena = NULL;
    volatile int64_t r = 0;
    zumir_trap t;
    zumir_mir_fail_alloc_at(n);
    zumir_trap_push(&t);
    if (setjmp(t.env) == 0) {
      ctx = zumir_mir_new(&arena);
      r = build_and_call(ctx, native);
      zumir_trap_pop(&t);
      zumir_mir_fail_alloc_at(0);
      CHECK(r == 12);
      CHECK(zumir_mir_free(ctx, arena, native) == 1);
      done = 1;
    } else {
      zumir_mir_fail_alloc_at(0);
      seen++;
      if (t.type != MIR_alloc_error) {
        printf("allocation %ld: raised %d %s\n", n, t.type, t.message);
        CHECK(t.type == MIR_alloc_error);
      }
      /* MIR_finish is skipped: 0, unless MIR_init2 itself raised and
       * there was no context to finish. */
      CHECK(zumir_mir_free(ctx, arena, native) == (ctx == NULL));
    }
    if (zumir_mir_live_arenas() != 0 || zumir_mir_live_bytes() != 0) {
      printf("allocation %ld: %ld arenas, %.0f bytes left\n", n, zumir_mir_live_arenas(),
             zumir_mir_live_bytes());
      CHECK(0);
      break;
    }
  }
  CHECK(done == 1);
  CHECK(seen > 10);
  printf("%s: %d allocations failed in turn\n", native ? "native" : "interp", seen);
}


/* ---- c2mir, as cfunction() compiles ---------------------------------- */

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

static const char *c_text =
  "static double sq(double v) { return v * v; }\n"
  "double hyp(double a, double b) { return sqrt(sq(a) + sq(b)) + strlen(\"ab\"); }\n";

/* Compiles c_text after the prelude, links it against D6 on the tier and
 * calls hyp(3, 4). Under the caller's trap. */
static double c2mir_call(MIR_context_t ctx, int native) {
  struct c2mir_options options;
  memset(&options, 0, sizeof options);
  options.message_file = ZUMIR_MIR_STDERR;
  source_reader r = {{zumir_prelude_prefix, "<glue>", zumir_prelude_line0, c_text, "\n"}, 0, 0};
  c2mir_init(ctx);
  int ok = c2mir_compile(ctx, &options, source_getc, &r, "<glue>", NULL);
  c2mir_finish(ctx);
  if (!ok) return -1;
  MIR_module_t m = DLIST_TAIL(MIR_module_t, *MIR_get_module_list(ctx));
  MIR_item_t f = NULL;
  for (MIR_item_t it = DLIST_HEAD(MIR_item_t, m->items); it != NULL; it = DLIST_NEXT(MIR_item_t, it))
    if (it->item_type == MIR_func_item && strcmp(it->u.func->name, "hyp") == 0) f = it;
  if (f == NULL) return -2;
  MIR_load_module(ctx, m);
  size_t n;
  const zumir_import_entry *d6 = zumir_import_d6(&n);
  for (size_t i = 0; i < n; i++) MIR_load_external(ctx, d6[i].name, d6[i].addr);
  if (native) {
    MIR_gen_init(ctx);
    MIR_link(ctx, MIR_set_gen_interface, NULL);
    double (*fn)(double, double);
    void *addr = MIR_gen(ctx, f);
    memcpy(&fn, &addr, sizeof fn);
    return fn(3, 4);
  }
  MIR_link(ctx, MIR_set_interp_interface, NULL);
  MIR_val_t res, args[2];
  args[0].d = 3;
  args[1].d = 4;
  MIR_interp_arr(ctx, f, &res, 2, args);
  return res.d;
}

/* The compile and its every allocation failure, as for MIR above: c2mir
 * is what calls the arena's calloc. */
static void test_c2mir(int native) {
  int seen = 0, done = 0;
  /* About 10^4 allocations: every one with ZUMIR_GLUE_FULL set (the full
   * profile), else every 53rd, which keeps a run, and each mutant's under
   * Mull, to about a second. */
  long step = getenv("ZUMIR_GLUE_FULL") != NULL ? 1 : 53;
  for (long n = 1; n < 1000000 && !done; n += step) {
    MIR_context_t volatile ctx = NULL;
    zumir_arena *volatile arena = NULL;
    volatile double r = 0;
    zumir_trap t;
    zumir_mir_fail_alloc_at(n);
    zumir_trap_push(&t);
    if (setjmp(t.env) == 0) {
      ctx = zumir_mir_new(&arena);
      r = c2mir_call(ctx, native);
      zumir_trap_pop(&t);
      zumir_mir_fail_alloc_at(0);
      CHECK(r == 7.0);
      CHECK(zumir_mir_free(ctx, arena, native) == 1);
      done = 1;
    } else {
      zumir_mir_fail_alloc_at(0);
      seen++;
      if (t.type != MIR_alloc_error) {
        printf("c2mir allocation %ld: raised %d %s\n", n, t.type, t.message);
        CHECK(t.type == MIR_alloc_error);
      }
      CHECK(zumir_mir_free(ctx, arena, native) == (ctx == NULL));
    }
    if (zumir_mir_live_arenas() != 0 || zumir_mir_live_bytes() != 0) {
      printf("c2mir allocation %ld: %ld arenas, %.0f bytes left\n", n, zumir_mir_live_arenas(),
             zumir_mir_live_bytes());
      CHECK(0);
      break;
    }
  }
  CHECK(done == 1);
  CHECK(seen > 5000 / step);
  printf("c2mir %s: %d allocations failed in turn\n", native ? "native" : "interp", seen);
}

/* ---- D6 --------------------------------------------------------------- */

/* D6 (design): the double functions of math.h and five string functions,
 * no float variants, each the platform's own. */
static const char *d6_names[] = {
  "acos", "asin", "atan", "atan2", "cos", "sin", "tan", "acosh", "asinh", "atanh", "cosh",
  "sinh", "tanh", "exp", "exp2", "expm1", "log", "log10", "log1p", "log2", "logb", "cbrt",
  "fabs", "hypot", "pow", "sqrt", "erf", "erfc", "lgamma", "tgamma", "ceil", "floor",
  "nearbyint", "rint", "round", "trunc", "fmod", "remainder", "copysign", "nextafter",
  "fdim", "fmax", "fmin", "fma", "ldexp", "frexp", "modf", "memcpy", "memmove", "memset",
  "memcmp", "strlen"};

static void *d6_addr(const zumir_import_entry *list, size_t n, const char *name) {
  for (size_t i = 0; i < n; i++)
    if (strcmp(list[i].name, name) == 0) return list[i].addr;
  return NULL;
}

static void test_d6(void) {
  size_t n;
  const zumir_import_entry *list = zumir_import_d6(&n);
  size_t want = sizeof d6_names / sizeof d6_names[0];
  CHECK(n == want);
  for (size_t i = 0; i < want; i++) {
    if (d6_addr(list, n, d6_names[i]) == NULL) {
      printf("D6 lacks %s\n", d6_names[i]);
      CHECK(0);
    }
  }
  for (size_t i = 0; i < n; i++)
    for (size_t j = i + 1; j < n; j++) CHECK(strcmp(list[i].name, list[j].name) != 0);
  /* Each name is the platform's function. */
  double (*f1)(double);
  void *p = d6_addr(list, n, "exp");
  memcpy(&f1, &p, sizeof f1);
  CHECK(p != NULL && f1(1.0) == exp(1.0));
  p = d6_addr(list, n, "lgamma");
  memcpy(&f1, &p, sizeof f1);
  CHECK(p != NULL && f1(4.5) == lgamma(4.5));
  double (*f2)(double, double);
  p = d6_addr(list, n, "atan2");
  memcpy(&f2, &p, sizeof f2);
  CHECK(p != NULL && f2(1.0, -2.0) == atan2(1.0, -2.0));
  double (*f3)(double, double, double);
  p = d6_addr(list, n, "fma");
  memcpy(&f3, &p, sizeof f3);
  CHECK(p != NULL && f3(0.1, 10.0, -1.0) == fma(0.1, 10.0, -1.0));
  size_t (*sl)(const char *);
  p = d6_addr(list, n, "strlen");
  memcpy(&sl, &p, sizeof sl);
  CHECK(p != NULL && sl("zumir") == 5);
  CHECK(d6_addr(list, n, "printf") == NULL);
  CHECK(d6_addr(list, n, "expf") == NULL);
  CHECK(d6_addr(list, n, "exit") == NULL);
}

int main(void) {
  test_trap();
  test_stdio();
  test_context(0);
  test_context(1);
  test_failed_context();
  test_alloc_failures(0);
  test_alloc_failures(1);
  test_c2mir(0);
  test_c2mir(1);
  test_d6();
  CHECK(zumir_trap_depth() == 0);
  printf("%s (%d failed)\n", failures ? "FAIL" : "ok", failures);
  return failures ? 1 : 0;
}
