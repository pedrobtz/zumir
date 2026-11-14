/* rfunction()'s two C entries (design §4.6): compile the MIR text the R
 * walker emits, with rfunction()'s import list (D18), and call it. The
 * generated function is
 *
 *   int64 rf (double *K, double *S, double **VP, int64 *VL, int64 *R)
 *
 * K the constant pool (R passes the doubles, so no number is ever printed
 * into MIR text), S the scalar arguments, VP and VL the vectors (arguments,
 * then the vector locals R allocated) and their lengths, R[0] the result
 * (a double, or the index of the vector returned). The return value is a
 * status: 0, or a run-time error or interrupt for the R wrapper to raise. */
#include <stdint.h>
#include <string.h>

#include "zumir_ctx.h"

#if ZUMIR_HAVE_MIR
SEXP zumir_rf_new(SEXP text, SEXP tier) {
  int t = strcmp(CHAR(STRING_ELT(tier, 0)), "native") == 0 ? ZUMIR_ITIER_NATIVE
                                                            : ZUMIR_ITIER_INTERP;
  SEXP x = PROTECT(zumir_rctx_new_sexp(t, ZUMIR_IMPORTS_RFUNCTION));
  if (TYPEOF(x) != EXTPTRSXP) {
    UNPROTECT(1);
    return x;
  }
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  SEXP res = PROTECT(zumir_ctx_scan(x, text));
  if (res != R_NilValue) {
    UNPROTECT(2);
    return res;
  }
  int status;
  zumir_rctx_compile(z, NULL, "rf", t, NULL, &status);
  if (status != ZUMIR_ST_OK) {
    SEXP e = status == ZUMIR_ST_MIR ? zumir_status("mir", z->type, z->message)
                                    : zumir_status("context", 0, "cannot compile on this tier");
    UNPROTECT(2);
    return e;
  }
  UNPROTECT(2);
  return x;
}

typedef int64_t (*rf_fn)(const double *K, const double *S, double *const *VP, const int64_t *VL,
                         int64_t *R);

/* Calls the compiled function. Returns list(status, value, nan, fmod):
 * value is R[0] as a double (the result) or as an index (a vector result),
 * nan and fmod say where R would have warned. */
SEXP zumir_rf_call(SEXP x, SEXP consts, SEXP scalars, SEXP vectors, SEXP vector_result) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  if (z->fn == NULL) return zumir_status("context", 0, "not compiled");
  R_xlen_t nv = XLENGTH(vectors);
  double **vp = (double **) R_alloc((size_t) nv + 1, sizeof(double *));
  int64_t *vl = (int64_t *) R_alloc((size_t) nv + 1, sizeof(int64_t));
  for (R_xlen_t i = 0; i < nv; i++) {
    SEXP v = VECTOR_ELT(vectors, i);
    vl[i] = (int64_t) XLENGTH(v);
    vp[i] = XLENGTH(v) > 0 ? REAL(v) : NULL;
  }
  int64_t r[2] = {0, 0};
  zumir_r_nan_warned = 0;
  zumir_r_fmod_warned = 0;
  int64_t status = ((rf_fn) z->fn)(REAL(consts), REAL(scalars), vp, vl, r);
  int nan = zumir_r_nan_warned, fmod = zumir_r_fmod_warned;
  zumir_r_nan_warned = 0;
  zumir_r_fmod_warned = 0;
  double value;
  if (Rf_asLogical(vector_result) == TRUE)
    value = (double) r[0];
  else
    memcpy(&value, &r[0], sizeof value);
  const char *names[] = {"status", "value", "nan", "fmod", "poll", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_ScalarInteger((int) status));
  SET_VECTOR_ELT(res, 1, Rf_ScalarReal(value));
  SET_VECTOR_ELT(res, 2, Rf_ScalarLogical(nan));
  SET_VECTOR_ELT(res, 3, Rf_ScalarLogical(fmod));
  /* The poll's error message (a time limit), "" for an interrupt. */
  SET_VECTOR_ELT(res, 4, Rf_mkString(zumir_r_poll_message));
  zumir_r_poll_message[0] = '\0';
  UNPROTECT(1);
  return res;
}
#else
SEXP zumir_rf_new(SEXP text, SEXP tier) {
  (void) text;
  (void) tier;
  return zumir_status("context", 0, "zumir was built without MIR: no target");
}
SEXP zumir_rf_call(SEXP x, SEXP consts, SEXP scalars, SEXP vectors, SEXP vector_result) {
  (void) x;
  (void) consts;
  (void) scalars;
  (void) vectors;
  (void) vector_result;
  return zumir_status("context", 0, "zumir was built without MIR");
}
#endif
