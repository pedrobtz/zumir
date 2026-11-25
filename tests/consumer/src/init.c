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
SEXP zc_bad_opcode_c(void);
SEXP zc_emit_limits_c(SEXP ncalls, SEXP nargs, SEXP tier);
SEXP zc_emit_names_c(SEXP x, SEXP tier);
SEXP zc_tier_lock_c(void);
SEXP zc_null_args_c(void);
SEXP zc_interp_compile_c(void);
SEXP zc_interp_call_c(SEXP x, SEXP a);
SEXP zc_alloc_status_c(SEXP tier);

static const R_CallMethodDef calls[] = {
  {"zc_rhs_c", (DL_FUNC) &zc_rhs_c, 4},
  {"zc_cmp_c", (DL_FUNC) &zc_cmp_c, 4},
  {"zc_errors_c", (DL_FUNC) &zc_errors_c, 1},
  {"zc_errors_cpp_c", (DL_FUNC) &zc_errors_cpp_c, 1},
  {"zc_frozen_c", (DL_FUNC) &zc_frozen_c, 1},
  {"zc_getter_c", (DL_FUNC) &zc_getter_c, 2},
  {"zc_expit_c", (DL_FUNC) &zc_expit_c, 2},
  {"zc_table_c", (DL_FUNC) &zc_table_c, 0},
  {"zc_bad_opcode_c", (DL_FUNC) &zc_bad_opcode_c, 0},
  {"zc_emit_limits_c", (DL_FUNC) &zc_emit_limits_c, 3},
  {"zc_emit_names_c", (DL_FUNC) &zc_emit_names_c, 2},
  {"zc_tier_lock_c", (DL_FUNC) &zc_tier_lock_c, 0},
  {"zc_null_args_c", (DL_FUNC) &zc_null_args_c, 0},
  {"zc_interp_compile_c", (DL_FUNC) &zc_interp_compile_c, 0},
  {"zc_interp_call_c", (DL_FUNC) &zc_interp_call_c, 2},
  {"zc_alloc_status_c", (DL_FUNC) &zc_alloc_status_c, 1},
  {NULL, NULL, 0}};

void R_init_zumirconsumer(DllInfo *dll) {
  R_registerRoutines(dll, NULL, calls, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
}
