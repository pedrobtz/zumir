#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

#include "zumir_mir.h"

/* TRUE when this build compiled MIR in (zumir_mir.h): FALSE on a CPU MIR
 * does not target, on Windows aarch64, and in a build with ZUMIR_NO_MIR.
 * Internal; zumir_info() reports it from Stage 4. */
static SEXP zumir_has_target(void) {
  return Rf_ScalarLogical(ZUMIR_HAVE_MIR ? TRUE : FALSE);
}

SEXP zumir_test_sink(SEXP text, SEXP path);
SEXP zumir_test_exit(SEXP text);

static const R_CallMethodDef zumir_call_methods[] = {
  {"zumir_has_target", (DL_FUNC) &zumir_has_target, 0},
  {"zumir_test_sink", (DL_FUNC) &zumir_test_sink, 2},
  {"zumir_test_exit", (DL_FUNC) &zumir_test_exit, 1},
  {NULL, NULL, 0}
};

/* The only symbol the shared object exports: every object is built with
 * $(C_VISIBILITY), and zumir-win.def lists this one alone on Windows, so
 * MIR's symbols cannot clash with another package that vendors MIR. */
void attribute_visible R_init_zumir(DllInfo *dll) {
  R_registerRoutines(dll, NULL, zumir_call_methods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}
