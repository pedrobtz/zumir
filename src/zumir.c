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

SEXP zumir_test_sink(SEXP text, SEXP path);
SEXP zumir_test_exit(SEXP text, SEXP trapped);
SEXP zumir_test_invalid(SEXP kind, SEXP finish);
SEXP zumir_test_rss(void);

static const R_CallMethodDef zumir_call_methods[] = {
  {"zumir_has_target", (DL_FUNC) &zumir_has_target, 0},
  {"zumir_test_sink", (DL_FUNC) &zumir_test_sink, 2},
  {"zumir_test_exit", (DL_FUNC) &zumir_test_exit, 2},
  {"zumir_test_invalid", (DL_FUNC) &zumir_test_invalid, 2},
  {"zumir_test_rss", (DL_FUNC) &zumir_test_rss, 0},
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
}
