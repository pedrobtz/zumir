#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

#include "zumir_mir.h"
#include "zumir_trap.h"

/* TRUE when this build compiled MIR in (zumir_mir.h): FALSE on a CPU MIR
 * does not target, on Windows aarch64, and in a build with ZUMIR_NO_MIR.
 * Internal; zumir_info() reports it from Stage 4. */
static SEXP zumir_has_target(void) {
  return Rf_ScalarLogical(ZUMIR_HAVE_MIR ? TRUE : FALSE);
}

/* list(commit, patches) for zumir_info(). */
static SEXP zumir_mir_build(void) {
  static const char *patches[] = {ZUMIR_MIR_PATCHES};
  int n = (int) (sizeof(patches) / sizeof(patches[0]));
  const char *names[] = {"commit", "patches", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_mkString(ZUMIR_MIR_COMMIT));
  SEXP p = PROTECT(Rf_allocVector(STRSXP, n));
  for (int i = 0; i < n; i++) SET_STRING_ELT(p, i, Rf_mkChar(patches[i]));
  SET_VECTOR_ELT(res, 1, p);
  UNPROTECT(2);
  return res;
}

SEXP zumir_test_sink(SEXP text, SEXP path);
SEXP zumir_test_exit(SEXP text, SEXP trapped);
SEXP zumir_test_invalid(SEXP kind, SEXP finish);
SEXP zumir_test_rss(void);
SEXP zumir_test_fail_alloc(SEXP n);
SEXP zumir_test_live_arenas(void);
SEXP zumir_ctx_new(SEXP tier);
SEXP zumir_ctx_free(SEXP x);
SEXP zumir_ctx_state(SEXP x);
SEXP zumir_ctx_begin(SEXP x, SEXP name);
SEXP zumir_ctx_end(SEXP x);
SEXP zumir_ctx_scan(SEXP x, SEXP text);
SEXP zumir_ctx_link(SEXP x, SEXP name);
SEXP zumir_ctx_call_d(SEXP x, SEXP arg);
SEXP zumir_memory(void);
SEXP zumir_probe_run(void);
SEXP zumir_probe_result(void);
SEXP zumir_test_emit_model(SEXP x, SEXP model);
SEXP zumir_test_call_rhs(SEXP x, SEXP model, SEXP y, SEXP par);
SEXP zumir_test_threads(SEXP model, SEXP nthreads, SEXP nsub, SEXP nsteps, SEXP mode);
SEXP zumir_test_deny_wx(void);
SEXP zumir_set_long_double(SEXP on);
SEXP zumir_test_port(SEXP which, SEXP x, SEXP y);
SEXP zumir_test_import_names(SEXP kind);
SEXP zumir_test_api_get(SEXP version, SEXP mir_abi);
SEXP zumir_mirf_new(SEXP text, SEXP name, SEXP tier, SEXP imports);
SEXP zumir_mirf_call(SEXP x, SEXP args);
SEXP zumir_mir_roundtrip(SEXP text, SEXP path);
void zumir_api_register(void);
SEXP zumir_set_r46(SEXP on);
SEXP zumir_test_math(SEXP which, SEXP x);
SEXP zumir_rf_new(SEXP text, SEXP tier, SEXP path);
SEXP zumir_rf_build(SEXP spec, SEXP tier, SEXP path);
SEXP zumir_c2mir_compile(SEXP source, SEXP fname, SEXP tier, SEXP name);
SEXP zumir_test_c2mir_call(SEXP x, SEXP args);
SEXP zumir_test_prelude(void);
SEXP zumir_rf_call(SEXP x, SEXP consts, SEXP scalars, SEXP vectors, SEXP vector_result);

static const R_CallMethodDef zumir_call_methods[] = {
  {"zumir_has_target", (DL_FUNC) &zumir_has_target, 0},
  {"zumir_test_sink", (DL_FUNC) &zumir_test_sink, 2},
  {"zumir_test_exit", (DL_FUNC) &zumir_test_exit, 2},
  {"zumir_test_invalid", (DL_FUNC) &zumir_test_invalid, 2},
  {"zumir_test_rss", (DL_FUNC) &zumir_test_rss, 0},
  {"zumir_test_fail_alloc", (DL_FUNC) &zumir_test_fail_alloc, 1},
  {"zumir_test_live_arenas", (DL_FUNC) &zumir_test_live_arenas, 0},
  {"zumir_mir_build", (DL_FUNC) &zumir_mir_build, 0},
  {"zumir_ctx_new", (DL_FUNC) &zumir_ctx_new, 1},
  {"zumir_ctx_free", (DL_FUNC) &zumir_ctx_free, 1},
  {"zumir_ctx_state", (DL_FUNC) &zumir_ctx_state, 1},
  {"zumir_ctx_begin", (DL_FUNC) &zumir_ctx_begin, 2},
  {"zumir_ctx_end", (DL_FUNC) &zumir_ctx_end, 1},
  {"zumir_ctx_scan", (DL_FUNC) &zumir_ctx_scan, 2},
  {"zumir_ctx_link", (DL_FUNC) &zumir_ctx_link, 2},
  {"zumir_ctx_call_d", (DL_FUNC) &zumir_ctx_call_d, 2},
  {"zumir_memory", (DL_FUNC) &zumir_memory, 0},
  {"zumir_probe_run", (DL_FUNC) &zumir_probe_run, 0},
  {"zumir_probe_result", (DL_FUNC) &zumir_probe_result, 0},
  {"zumir_test_emit_model", (DL_FUNC) &zumir_test_emit_model, 2},
  {"zumir_test_call_rhs", (DL_FUNC) &zumir_test_call_rhs, 4},
  {"zumir_test_threads", (DL_FUNC) &zumir_test_threads, 5},
  {"zumir_test_deny_wx", (DL_FUNC) &zumir_test_deny_wx, 0},
  {"zumir_set_long_double", (DL_FUNC) &zumir_set_long_double, 1},
  {"zumir_test_port", (DL_FUNC) &zumir_test_port, 3},
  {"zumir_test_import_names", (DL_FUNC) &zumir_test_import_names, 1},
  {"zumir_test_api_get", (DL_FUNC) &zumir_test_api_get, 2},
  {"zumir_mirf_new", (DL_FUNC) &zumir_mirf_new, 4},
  {"zumir_mirf_call", (DL_FUNC) &zumir_mirf_call, 2},
  {"zumir_mir_roundtrip", (DL_FUNC) &zumir_mir_roundtrip, 2},
  {"zumir_set_r46", (DL_FUNC) &zumir_set_r46, 1},
  {"zumir_test_math", (DL_FUNC) &zumir_test_math, 2},
  {"zumir_rf_new", (DL_FUNC) &zumir_rf_new, 3},
  {"zumir_rf_build", (DL_FUNC) &zumir_rf_build, 3},
  {"zumir_c2mir_compile", (DL_FUNC) &zumir_c2mir_compile, 4},
  {"zumir_test_c2mir_call", (DL_FUNC) &zumir_test_c2mir_call, 2},
  {"zumir_test_prelude", (DL_FUNC) &zumir_test_prelude, 0},
  {"zumir_rf_call", (DL_FUNC) &zumir_rf_call, 5},
  {NULL, NULL, 0}
};

/* MIR reported an error, or exited, with no zumir entry's trap on this
 * thread (zumir_trap.h). Every entry sets one, so this is a zumir bug; it
 * is reported as an R error rather than letting MIR continue. Only R's main
 * thread calls zumir entries that call MIR outside a trap. */
static void zumir_unhandled(int type, int status, const char *message) {
  if (type == ZUMIR_TRAP_EXIT)
    Rf_error("MIR stopped (status %d): %s", status, message);
  Rf_error("MIR error %d outside a zumir entry: %s", type, message);
}

/* The only symbol the shared object exports: every object is built with
 * $(C_VISIBILITY), and zumir-win.def lists this one alone on Windows, so
 * MIR's symbols cannot clash with another package that vendors MIR. */
void attribute_visible R_init_zumir(DllInfo *dll) {
  R_registerRoutines(dll, NULL, zumir_call_methods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
  zumir_trap_unhandled = zumir_unhandled;
  /* The C API table, for packages with LinkingTo: zumir (design §4.3). */
  zumir_api_register();
}
