#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

/* No native routines yet; Stage 2 vendors MIR and Stage 3 adds the first
 * entry. Registration is on and dynamic lookup off from the start, so a
 * routine that is not in this table can never be reached by name. */
static const R_CallMethodDef zumir_call_methods[] = {
  {NULL, NULL, 0}
};

void R_init_zumir(DllInfo *dll) {
  R_registerRoutines(dll, NULL, zumir_call_methods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}
