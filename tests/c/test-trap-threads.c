/* The error trap on two threads at once (design §4.3 "Threads", D4): each
 * thread fails its own module 500 times, both at the same time, and every
 * error must land in the thread that raised it, with that thread's MIR
 * error type. Built without R by tools/ci/platform.sh from MIR (through
 * src/zumir_mir.c) and zumir's R-free trap files:
 *
 *   cc -Isrc -DNDEBUG tests/c/test-trap-threads.c src/zumir_mir.c \
 *     src/zumir_mir_gen.c src/zumir_mir_glue.c src/zumir_mir_stdio.c src/zumir_trap.c \
 *     -lpthread -lm
 *
 * Exit status 0 when every check holds. */
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "zumir_trap.h"
#include "zumir_mir_api.h"

#define N 500

static const char *bad_text =
  "m: module\n  f: func i64, i64:a\n  bogus a\n endfunc\nendmodule\n";

/* A syntax error (MIR_syntax_error) or an undefined import at link time
 * (MIR_undeclared_op_ref_error). */
static void fail(MIR_context_t ctx, int kind) {
  if (kind == 0) {
    MIR_scan_string(ctx, bad_text);
    return;
  }
  MIR_new_module(ctx, "bad");
  MIR_item_t imp = MIR_new_import(ctx, "no_such_symbol_zz");
  MIR_type_t rt = MIR_T_I64;
  MIR_item_t pr = MIR_new_proto_arr(ctx, "p", 1, &rt, 0, NULL);
  MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 0, NULL);
  MIR_reg_t r = MIR_new_func_reg(ctx, fi->u.func, MIR_T_I64, "r");
  MIR_append_insn(ctx, fi,
                  MIR_new_call_insn(ctx, 3, MIR_new_ref_op(ctx, pr), MIR_new_ref_op(ctx, imp),
                                    MIR_new_reg_op(ctx, r)));
  MIR_append_insn(ctx, fi, MIR_new_ret_insn(ctx, 1, MIR_new_reg_op(ctx, r)));
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  MIR_load_module(ctx, DLIST_TAIL(MIR_module_t, *MIR_get_module_list(ctx)));
  MIR_link(ctx, MIR_set_interp_interface, NULL);
}

typedef struct {
  int kind;
  int caught, wrong, depth;
} job;

/* A start gate for both threads, so their failures overlap: macOS has no
 * pthread_barrier_t. */
static pthread_mutex_t gate_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_cv = PTHREAD_COND_INITIALIZER;
static int gate_n = 0;

static void gate_wait(void) {
  pthread_mutex_lock(&gate_mu);
  if (++gate_n == 2)
    pthread_cond_broadcast(&gate_cv);
  else
    while (gate_n < 2) pthread_cond_wait(&gate_cv, &gate_mu);
  pthread_mutex_unlock(&gate_mu);
}

static void *run(void *arg) {
  job *j = arg;
  gate_wait();
  for (int i = 0; i < N; i++) {
    MIR_context_t volatile ctx = NULL;
    zumir_arena *volatile arena = NULL;
    zumir_trap t;
    zumir_trap_push(&t);
    if (setjmp(t.env) == 0) {
      ctx = zumir_mir_new(&arena);
      fail(ctx, j->kind);
      zumir_trap_pop(&t);
    } else {
      int want = j->kind == 0 ? MIR_syntax_error : MIR_undeclared_op_ref_error;
      const char *text = j->kind == 0 ? "bogus" : "no_such_symbol_zz";
      if (t.type == want && strstr(t.message, text) != NULL)
        j->caught++;
      else
        j->wrong++;
    }
    if (!zumir_mir_free(ctx, arena, 0)) j->wrong++;
  }
  j->depth = zumir_trap_depth();
  return NULL;
}

int main(void) {
  job jobs[2] = {{0, 0, 0, 0}, {1, 0, 0, 0}};
  pthread_t th[2];
  for (int k = 0; k < 2; k++) pthread_create(&th[k], NULL, run, &jobs[k]);
  for (int k = 0; k < 2; k++) pthread_join(th[k], NULL);
  int ok = 1;
  for (int k = 0; k < 2; k++) {
    printf("thread %d: caught %d of %d, wrong %d, trap depth %d\n", k, jobs[k].caught, N,
           jobs[k].wrong, jobs[k].depth);
    ok = ok && jobs[k].caught == N && jobs[k].wrong == 0 && jobs[k].depth == 0;
  }
  printf("live arenas %ld\n", zumir_mir_live_arenas());
  ok = ok && zumir_mir_live_arenas() == 0;
  printf("%s\n", ok ? "ok" : "FAIL");
  return ok ? 0 : 1;
}
