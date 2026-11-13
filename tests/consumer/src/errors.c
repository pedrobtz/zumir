/* Errors through the table (D20): an invalid instruction mid-module, then
 * more MIR_ calls (no effect, no crash), then zumir_compile() (NULL and the
 * recorded status), then zumir_ctx_free(); n times. Also the getter with
 * other versions and MIR ABIs, the frozen v1 header, and a consumer
 * import. */
#define R_NO_REMAP
#include <math.h>
#include <string.h>
#include <R.h>
#include <Rinternals.h>

#include <zumir.h>

static void load(void) {
  const char *why;
  if (zumir_api_ == NULL && zumir_api_load(&why) == NULL) Rf_error("zumir: %s", why);
}

/* One failing build: list of what was seen, as counts over n. */
SEXP zc_errors_c(SEXP s_n) {
  load();
  int n = Rf_asInteger(s_n);
  int null_compile = 0, recorded = 0, failed_status = 0, placeholders = 0, msg = 0;
  for (int i = 0; i < n; i++) {
    int status;
    zumir_ctx *z = zumir_ctx_new(&status);
    if (z == NULL) Rf_error("zumir_ctx_new: %d", status);
    MIR_context_t ctx = zumir_mir(z);
    MIR_module_t mod = MIR_new_module(ctx, "bad");
    MIR_type_t rt = MIR_T_D;
    MIR_var_t a = {MIR_T_D, "a", 0};
    MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 1, &a);
    MIR_reg_t ra = MIR_reg(ctx, "a", fi->u.func);
    /* An integer add on double registers: MIR rejects it at finish_func. */
    MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, ra),
                                          MIR_new_reg_op(ctx, ra), MIR_new_reg_op(ctx, ra)));
    MIR_append_insn(ctx, fi, MIR_new_ret_insn(ctx, 1, MIR_new_reg_op(ctx, ra)));
    MIR_finish_func(ctx);
    /* More calls on the failed context: placeholders, never NULL items. */
    MIR_item_t g = MIR_new_func_arr(ctx, "g", 1, &rt, 1, &a);
    MIR_reg_t rg = MIR_reg(ctx, "a", g->u.func);
    MIR_append_insn(ctx, g, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, rg),
                                         MIR_new_double_op(ctx, 1.0)));
    if (g != NULL && g->u.func != NULL && rg == 0) placeholders++;
    MIR_finish_module(ctx);
    const char *m1;
    if (zumir_status(z, &m1) == ZUMIR_E_MIR && strstr(m1, "operand") != NULL) msg++;
    int tier;
    void *fn = zumir_compile(z, mod, "f", ZUMIR_TIER_AUTO, &tier, &status);
    if (fn == NULL) null_compile++;
    if (status == ZUMIR_E_FAILED || status == ZUMIR_E_MIR) recorded++;
    if (status == ZUMIR_E_FAILED) failed_status++;
    zumir_ctx_free(z);
  }
  zumir_ctx_free(NULL); /* does nothing */
  const char *names[] = {"null_compile", "recorded", "failed_status", "placeholders",
                         "message", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_ScalarInteger(null_compile));
  SET_VECTOR_ELT(res, 1, Rf_ScalarInteger(recorded));
  SET_VECTOR_ELT(res, 2, Rf_ScalarInteger(failed_status));
  SET_VECTOR_ELT(res, 3, Rf_ScalarInteger(placeholders));
  SET_VECTOR_ELT(res, 4, Rf_ScalarInteger(msg));
  UNPROTECT(1);
  return res;
}

/* The getter as a consumer built against another header would call it:
 * the reason, or "" when it gives a table. */
SEXP zc_getter_c(SEXP version, SEXP mir_abi) {
  zumir_api_get_t get = (zumir_api_get_t) R_GetCCallable("zumir", "zumir_api_get");
  const char *why = NULL;
  const zumir_api_v1 *t =
    get((uint32_t) Rf_asInteger(version), (uint32_t) Rf_asInteger(mir_abi), &why);
  return Rf_mkString(t != NULL ? "" : (why != NULL ? why : "(no reason)"));
}

/* A consumer import (D14): expit registered in this context resolves;
 * without registering it, the link fails. Returns expit(x) or NA. */
static double expit(double x) {
  return 1.0 / (1.0 + exp(-x));
}

SEXP zc_expit_c(SEXP x, SEXP s_register) {
  load();
  int status;
  zumir_ctx *z = zumir_ctx_new(&status);
  MIR_context_t ctx = zumir_mir(z);
  if (Rf_asLogical(s_register) == TRUE) zumir_import(z, "expit", (void *) expit);
  MIR_module_t mod = MIR_new_module(ctx, "m");
  MIR_type_t rt = MIR_T_D;
  MIR_var_t a = {MIR_T_D, "x", 0};
  MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 1, &a);
  zumir_emitter e;
  zumir_emit_begin(&e, ctx, fi);
  MIR_reg_t rx = MIR_reg(ctx, "x", fi->u.func);
  zumir_emit_return_value(&e, zumir_emit_call1(&e, "expit", rx));
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  double (*f)(double) =
    (double (*)(double)) zumir_compile(z, mod, "f", ZUMIR_TIER_AUTO, NULL, &status);
  double r = f != NULL ? f(Rf_asReal(x)) : NA_REAL;
  const char *msg;
  zumir_status(z, &msg);
  SEXP why = PROTECT(Rf_mkString(msg));
  zumir_ctx_free(z);
  SEXP out = PROTECT(Rf_ScalarReal(r));
  Rf_setAttrib(out, Rf_install("status"), Rf_ScalarInteger(status));
  Rf_setAttrib(out, Rf_install("message"), why);
  UNPROTECT(2);
  return out;
}

/* The table's own size and version against this unit's header (D21): a
 * consumer reads a field only when `size` covers it. */
SEXP zc_table_c(void) {
  load();
  SEXP out = PROTECT(Rf_allocVector(REALSXP, 4));
  REAL(out)[0] = (double) zumir_api_->size;
  REAL(out)[1] = (double) sizeof(zumir_api_v1);
  REAL(out)[2] = (double) zumir_api_->version;
  REAL(out)[3] = (double) zumir_api_->mir_abi;
  UNPROTECT(1);
  return out;
}
