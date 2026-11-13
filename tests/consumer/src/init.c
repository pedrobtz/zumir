#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

SEXP zc_rhs_c(SEXP model, SEXP tier, SEXP y, SEXP par);
SEXP zc_cmp_c(SEXP what, SEXP code, SEXP a, SEXP b);
SEXP zc_errors_c(SEXP n);
SEXP zc_errors_cpp_c(SEXP n);
SEXP zc_frozen_c(SEXP x);
SEXP zc_getter_c(SEXP version, SEXP mir_abi);
SEXP zc_expit_c(SEXP x, SEXP reg);
SEXP zc_table_c(void);

static const R_CallMethodDef calls[] = {
  {"zc_rhs_c", (DL_FUNC) &zc_rhs_c, 4},
  {"zc_cmp_c", (DL_FUNC) &zc_cmp_c, 4},
  {"zc_errors_c", (DL_FUNC) &zc_errors_c, 1},
  {"zc_errors_cpp_c", (DL_FUNC) &zc_errors_cpp_c, 1},
  {"zc_frozen_c", (DL_FUNC) &zc_frozen_c, 1},
  {"zc_getter_c", (DL_FUNC) &zc_getter_c, 2},
  {"zc_expit_c", (DL_FUNC) &zc_expit_c, 2},
  {"zc_table_c", (DL_FUNC) &zc_table_c, 0},
  {NULL, NULL, 0}};

void R_init_zumirconsumer(DllInfo *dll) {
  R_registerRoutines(dll, NULL, calls, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
}
