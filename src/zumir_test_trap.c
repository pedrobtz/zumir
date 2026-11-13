/* Internal test entries for the error trap (zumir_trap.h). No R API is
 * called between a setjmp() and the end of the MIR calls it covers; the R
 * result is built after. tests/testthat/test-error-trap.R. */
#define R_NO_REMAP
#include <string.h>
#include <R.h>
#include <Rinternals.h>

#include "zumir_mir.h"
#include "zumir_trap.h"

#if ZUMIR_HAVE_MIR
#include "zumir_mir_api.h"

static const char *bad_text =
  "m: module\n  f: func i64, i64:a\n  bogus a\n endfunc\nendmodule\n";

/* Builds a module that fails as `kind` says, in a fresh context. */
static void build_invalid(MIR_context_t ctx, const char *kind) {
  if (strcmp(kind, "syntax") == 0) {
    MIR_scan_string(ctx, bad_text);
  } else if (strcmp(kind, "operand") == 0) {
    /* An integer add on double registers: MIR_finish_func rejects it, so
     * the error is raised inside MIR_finish_func. */
    MIR_new_module(ctx, "bad");
    MIR_type_t rt = MIR_T_D;
    MIR_var_t a = {MIR_T_D, "a", 0};
    MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 1, &a);
    MIR_reg_t ra = MIR_reg(ctx, "a", fi->u.func);
    MIR_append_insn(ctx, fi,
                    MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, ra),
                                 MIR_new_reg_op(ctx, ra), MIR_new_reg_op(ctx, ra)));
    MIR_append_insn(ctx, fi, MIR_new_ret_insn(ctx, 1, MIR_new_reg_op(ctx, ra)));
    MIR_finish_func(ctx);
    MIR_finish_module(ctx);
  } else if (strcmp(kind, "import") == 0) {
    MIR_new_module(ctx, "bad");
    MIR_item_t imp = MIR_new_import(ctx, "no_such_symbol_zz");
    MIR_type_t rt = MIR_T_I64;
    MIR_item_t pr = MIR_new_proto_arr(ctx, "p", 1, &rt, 0, NULL);
    MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 0, NULL);
    MIR_reg_t r = MIR_new_func_reg(ctx, fi->u.func, MIR_T_I64, "r");
    MIR_append_insn(ctx, fi,
                    MIR_new_call_insn(ctx, 3, MIR_new_ref_op(ctx, pr),
                                      MIR_new_ref_op(ctx, imp), MIR_new_reg_op(ctx, r)));
    MIR_append_insn(ctx, fi, MIR_new_ret_insn(ctx, 1, MIR_new_reg_op(ctx, r)));
    MIR_finish_func(ctx);
    MIR_finish_module(ctx);
    MIR_load_module(ctx, DLIST_TAIL(MIR_module_t, *MIR_get_module_list(ctx)));
    MIR_link(ctx, MIR_set_interp_interface, NULL);
  }
}
#endif

/* Builds and fails a module of `kind` ("syntax", "operand" or "import")
 in a fresh context under a trap, then finishes the context under
 * zumir_mir_free() when `finish` is TRUE. Returns NULL if MIR reported
 * nothing, else list(type, status, message, finished, depth): finished is
 * FALSE when the context was released without MIR_finish (an allocation
 * failed, or MIR_finish raised), and depth is this thread's trap depth
 * afterwards, which must be 0. */
SEXP zumir_test_invalid(SEXP s_kind, SEXP s_finish) {
#if ZUMIR_HAVE_MIR
  const char *kind = CHAR(STRING_ELT(s_kind, 0));
  int finish = Rf_asLogical(s_finish) == TRUE;
  if (strcmp(kind, "syntax") && strcmp(kind, "operand") && strcmp(kind, "import"))
    Rf_error("unknown kind '%s'", kind);

  MIR_context_t volatile ctx = NULL;
  zumir_arena *volatile arena = NULL;
  volatile int raised = 0;
  int finished = 1;
  zumir_trap t;

  zumir_mir_message_clear();
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    ctx = zumir_mir_new(&arena);
    build_invalid(ctx, kind);
    zumir_trap_pop(&t);
  } else {
    raised = 1;
  }
  /* With `finish` FALSE the context is leaked on purpose, for the RSS
   * test's own check that a leak shows. */
  if (finish) finished = zumir_mir_free(ctx, arena);

  if (!raised) return R_NilValue;
  const char *names[] = {"type", "status", "message", "finished", "depth", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_ScalarInteger(t.type));
  SET_VECTOR_ELT(res, 1, Rf_ScalarInteger(t.status));
  SET_VECTOR_ELT(res, 2, Rf_mkString(t.message));
  SET_VECTOR_ELT(res, 3, Rf_ScalarLogical(finished));
  SET_VECTOR_ELT(res, 4, Rf_ScalarInteger(zumir_trap_depth()));
  UNPROTECT(1);
  return res;
#else
  (void) s_kind;
  (void) s_finish;
  Rf_error("zumir was built without MIR");
#endif
}

/* Fails the n-th allocation zumir's MIR allocator makes from now on (0:
 * none), so a test can fail each of MIR's allocations in turn. */
SEXP zumir_test_fail_alloc(SEXP n) {
#if ZUMIR_HAVE_MIR
  zumir_mir_fail_alloc_at((long) Rf_asReal(n));
#else
  (void) n;
#endif
  return R_NilValue;
}

/* Arenas not yet freed: 0 when no context leaked. */
SEXP zumir_test_live_arenas(void) {
#if ZUMIR_HAVE_MIR
  return Rf_ScalarReal((double) zumir_mir_live_arenas());
#else
  return Rf_ScalarReal(0);
#endif
}

/* The process's resident set size, in bytes, or NA where it cannot be read. */
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#elif defined(__linux__)
#include <stdio.h>
#include <unistd.h>
#endif

SEXP zumir_test_rss(void) {
  double rss = NA_REAL;
#if defined(_WIN32)
  PROCESS_MEMORY_COUNTERS pmc;
  if (K32GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    rss = (double) pmc.WorkingSetSize;
#elif defined(__APPLE__)
  mach_task_basic_info_data_t info;
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info, &count) ==
      KERN_SUCCESS)
    rss = (double) info.resident_size;
#elif defined(__linux__)
  FILE *f = fopen("/proc/self/statm", "r");
  if (f != NULL) {
    long size, resident;
    if (fscanf(f, "%ld %ld", &size, &resident) == 2)
      rss = (double) resident * (double) sysconf(_SC_PAGESIZE);
    fclose(f);
  }
#endif
  return Rf_ScalarReal(rss);
}
