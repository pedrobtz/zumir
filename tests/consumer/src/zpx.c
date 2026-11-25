/* zupmxode's models through zumir (test-zupmxode.R): the IR zupmxode
 * compiles (tests/testthat/fixtures/zpx-models.R), lowered by zupmxode's
 * own lowering (zpx_lower.c) on one tier, and the same IR run by
 * zupmxode's reference evaluator (zpx_ir.c), the oracle. */
#define R_NO_REMAP
#include <stdlib.h>
#include <string.h>
#include <R.h>
#include <Rinternals.h>

#include <zumir.h>

#include "zpx_ir.h"
#include "zpx_lower.h"

typedef struct {
  zpx_lowered lowered;
  int ny, npar, nwork, nout[ZPX_IR_NBLOCKS];
} zpx_model;

static void verify(SEXP code, SEXP consts, zpx_ir *ir) {
  int b, i;
  int e = zpx_ir_verify(INTEGER(code), (size_t) XLENGTH(code), REAL(consts),
                        (size_t) XLENGTH(consts), ir, &b, &i);
  if (e != ZPX_IR_OK) Rf_error("zpx: %s (block %d, insn %d)", zpx_ir_error_message(e), b, i);
}

static void model_free(SEXP x) {
  zpx_model *m = R_ExternalPtrAddr(x);
  if (m == NULL) return;
  zpx_lower_free(&m->lowered);
  free(m);
  R_ClearExternalPtr(x);
}

/* list(model, status, message, block): the IR lowered on `tier` ("native"
 * or "interp"); model is NULL unless status is ZPX_LOWER_OK. */
SEXP zc_zpx_compile_c(SEXP code, SEXP consts, SEXP s_tier) {
  zpx_ir ir;
  verify(code, consts, &ir);
  const char *why = zpx_lower_api();
  if (why != NULL) Rf_error("zumir: %s", why);
  int tier = strcmp(CHAR(STRING_ELT(s_tier, 0)), "native") == 0 ? ZPX_TIER_NATIVE
                                                                 : ZPX_TIER_INTERP;
  const char *names[] = {"model", "status", "message", "block", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SEXP x = PROTECT(R_MakeExternalPtr(NULL, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(x, model_free, TRUE);
  zpx_model *m = calloc(1, sizeof(*m));
  void *scratch = malloc(zpx_lower_scratch_size(&ir) + 1);
  if (m == NULL || scratch == NULL) {
    free(m);
    free(scratch);
    Rf_error("zpx: no memory");
  }
  R_SetExternalPtrAddr(x, m);
  m->ny = ir.ny;
  m->npar = ir.npar;
  m->nwork = ir.nwork;
  for (int b = 0; b < ZPX_IR_NBLOCKS; b++) m->nout[b] = ir.block[b].nout;
  const char *message = NULL;
  int where = -1;
  int st = zpx_lower(&ir, tier, &m->lowered, scratch, &message, &where);
  free(scratch);
  /* The message lives in the context: copied before anything frees it. */
  SET_VECTOR_ELT(res, 2, Rf_mkString(message != NULL ? message : ""));
  SET_VECTOR_ELT(res, 1, Rf_ScalarInteger(st));
  SET_VECTOR_ELT(res, 3, Rf_ScalarInteger(where));
  if (st == ZPX_LOWER_OK) SET_VECTOR_ELT(res, 0, x);
  else model_free(x);
  UNPROTECT(2);
  return res;
}

typedef void (*block_runner)(void *self, int block, double t, const double *y, double *out,
                             const double *par, double *work);

/* A (nout + nwork) x n matrix: column j is block's out, then work, after
 * one call at t[j], y[, j], par[, j], with work[, j] on entry and out 0. */
static SEXP run_columns(void *self, block_runner run, int ny, int npar, int nwork, int nout,
                        int block, SEXP t, SEXP y, SEXP par, SEXP work) {
  R_xlen_t n = XLENGTH(t);
  if (XLENGTH(y) != n * ny || XLENGTH(par) != n * npar || XLENGTH(work) != n * nwork)
    Rf_error("zpx: inputs are not ny, npar and nwork rows by length(t) columns");
  int rows = nout + nwork;
  SEXP res = PROTECT(Rf_allocMatrix(REALSXP, rows, (int) n));
  for (R_xlen_t j = 0; j < n; j++) {
    double *col = REAL(res) + j * rows;
    memset(col, 0, (size_t) nout * sizeof(double));
    memcpy(col + nout, REAL(work) + j * nwork, (size_t) nwork * sizeof(double));
    run(self, block, REAL(t)[j], REAL(y) + j * ny, col, REAL(par) + j * npar, col + nout);
  }
  UNPROTECT(1);
  return res;
}

static void run_lowered(void *self, int block, double t, const double *y, double *out,
                        const double *par, double *work) {
  ((zpx_model *) self)->lowered.fn[block](t, y, out, par, work);
}

typedef struct {
  zpx_ir ir;
  double *regs;
} evaluator;

static void run_reference(void *self, int block, double t, const double *y, double *out,
                          const double *par, double *work) {
  evaluator *e = self;
  zpx_ir_eval(&e->ir, block, t, y, out, par, work, e->regs);
}

/* Block `block` (0-based) of a lowered model, column by column. */
SEXP zc_zpx_run_c(SEXP x, SEXP s_block, SEXP t, SEXP y, SEXP par, SEXP work) {
  zpx_model *m = R_ExternalPtrAddr(x);
  int b = Rf_asInteger(s_block);
  if (m == NULL || b < 0 || b >= ZPX_IR_NBLOCKS) Rf_error("zpx: no model, or no such block");
  return run_columns(m, run_lowered, m->ny, m->npar, m->nwork, m->nout[b], b, t, y, par, work);
}

/* The same through zupmxode's reference evaluator. */
SEXP zc_zpx_eval_c(SEXP code, SEXP consts, SEXP s_block, SEXP t, SEXP y, SEXP par, SEXP work) {
  evaluator e;
  verify(code, consts, &e.ir);
  int b = Rf_asInteger(s_block);
  if (b < 0 || b >= ZPX_IR_NBLOCKS) Rf_error("zpx: no such block");
  e.regs = (double *) R_alloc((size_t) e.ir.block[b].nreg + 1, sizeof(double));
  return run_columns(&e, run_reference, e.ir.ny, e.ir.npar, e.ir.nwork, e.ir.block[b].nout, b, t,
                     y, par, work);
}
