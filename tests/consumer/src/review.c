/* The C API's guarantees the critical review found broken (#59, #60, #61,
 * #63, #76, #78, #90, #92), each as a consumer sees it: every call returns,
 * an error is a status on the context, never an R error or a longjmp, on
 * R's thread or another. */
#define R_NO_REMAP
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <R.h>
#include <Rinternals.h>

#include <zumir.h>

static void load(void) {
  const char *why;
  if (zumir_api_ == NULL && zumir_api_load(&why) == NULL) Rf_error("zumir: %s", why);
}

static int tier_code(SEXP tier) {
  return strcmp(CHAR(STRING_ELT(tier, 0)), "native") == 0 ? ZUMIR_TIER_NATIVE
                                                           : ZUMIR_TIER_INTERP;
}

/* ---- #59: an invalid opcode is a status, on any thread ---- */

static void *bad_opcode(void *arg) {
  int *out = arg, status;
  zumir_ctx *z = zumir_ctx_new(&status);
  if (z == NULL) {
    *out = -1;
    return NULL;
  }
  MIR_context_t ctx = zumir_mir(z);
  MIR_new_module(ctx, "m");
  MIR_op_t op = MIR_new_int_op(ctx, 1);
  (void) MIR_new_insn(ctx, (MIR_insn_code_t) 1000000, op, op, op);
  (void) MIR_new_insn(ctx, (MIR_insn_code_t) -5, op, op, op);
  const char *msg;
  *out = zumir_status(z, &msg);
  zumir_ctx_free(z);
  return NULL;
}

/* list(main, thread): zumir_status after the bad opcodes. */
SEXP zc_bad_opcode_c(void) {
  load();
  int main_status = -2, thread_status = -2;
  bad_opcode(&main_status);
  pthread_t th;
  if (pthread_create(&th, NULL, bad_opcode, &thread_status) == 0) pthread_join(th, NULL);
  SEXP res = PROTECT(Rf_allocVector(INTSXP, 2));
  INTEGER(res)[0] = main_status;
  INTEGER(res)[1] = thread_status;
  UNPROTECT(1);
  return res;
}

/* ---- #61, #76: the emitter ---- */

static double plus1(double x) {
  return x + 1;
}

/* f(a) = the sum over k of name_k(a), for `ncalls` distinct names (each a
 * registered plus1), and one call of `nargs` arguments when nargs >= 0.
 * Returns c(status, value). */
SEXP zc_emit_limits_c(SEXP s_ncalls, SEXP s_nargs, SEXP s_tier) {
  load();
  int ncalls = Rf_asInteger(s_ncalls), nargs = Rf_asInteger(s_nargs), status;
  zumir_ctx *z = zumir_ctx_new(&status);
  if (z == NULL) Rf_error("zumir_ctx_new: %d", status);
  char names[40][8];
  for (int k = 0; k < ncalls && k < 40; k++) {
    snprintf(names[k], sizeof names[k], "p%d", k);
    zumir_import(z, names[k], (void *) plus1);
  }
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t mod = MIR_new_module(ctx, "m");
  MIR_type_t rt = MIR_T_D;
  MIR_var_t a = {MIR_T_D, "a", 0};
  MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 1, &a);
  zumir_emitter e;
  zumir_emit_begin(&e, ctx, fi);
  MIR_reg_t ra = MIR_reg(ctx, "a", fi->u.func);
  MIR_reg_t s = zumir_emit_const(&e, 0);
  for (int k = 0; k < ncalls && k < 40; k++) s = zumir_emit_add(&e, s, zumir_emit_call1(&e, names[k], ra));
  if (nargs >= 0) {
    MIR_reg_t args[6] = {ra, ra, ra, ra, ra, ra};
    s = zumir_emit_add(&e, s, zumir_emit_call(&e, "hypot", nargs, args));
  }
  zumir_emit_return_value(&e, s);
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  double (*fn)(double) =
    (double (*)(double)) zumir_compile(z, mod, "f", tier_code(s_tier), NULL, &status);
  double value = fn != NULL ? fn(0.5) : NA_REAL;
  zumir_ctx_free(z);
  SEXP res = PROTECT(Rf_allocVector(REALSXP, 2));
  REAL(res)[0] = status;
  REAL(res)[1] = value;
  UNPROTECT(1);
  return res;
}

/* Two functions in one module, each calling sin and cos through its own
 * emitter, the names from one reused buffer: c(status_f, f(x), status_g,
 * g(x)), f(x) = sin(x) + cos(x), g(x) = sin(x) * cos(x). */
SEXP zc_emit_names_c(SEXP s_x, SEXP s_tier) {
  load();
  int status, tier = tier_code(s_tier);
  zumir_ctx *z = zumir_ctx_new(&status);
  if (z == NULL) Rf_error("zumir_ctx_new: %d", status);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t mod = MIR_new_module(ctx, "m");
  MIR_type_t rt = MIR_T_D;
  MIR_var_t a = {MIR_T_D, "a", 0};
  char buf[16];
  for (int which = 0; which < 2; which++) {
    MIR_item_t fi = MIR_new_func_arr(ctx, which == 0 ? "f" : "g", 1, &rt, 1, &a);
    zumir_emitter e;
    zumir_emit_begin(&e, ctx, fi);
    MIR_reg_t ra = MIR_reg(ctx, "a", fi->u.func);
    strcpy(buf, "sin");
    MIR_reg_t s1 = zumir_emit_call1(&e, buf, ra);
    strcpy(buf, "cos");
    MIR_reg_t c1 = zumir_emit_call1(&e, buf, ra);
    zumir_emit_return_value(&e, which == 0 ? zumir_emit_add(&e, s1, c1) : zumir_emit_mul(&e, s1, c1));
    MIR_finish_func(ctx);
  }
  MIR_new_export(ctx, "f");
  MIR_new_export(ctx, "g");
  MIR_finish_module(ctx);
  double x = Rf_asReal(s_x);
  int st_f, st_g;
  double (*f)(double) = (double (*)(double)) zumir_compile(z, mod, "f", tier, NULL, &st_f);
  double (*g)(double) = (double (*)(double)) zumir_compile(z, mod, "g", tier, NULL, &st_g);
  SEXP res = PROTECT(Rf_allocVector(REALSXP, 4));
  REAL(res)[0] = st_f;
  REAL(res)[1] = f != NULL ? f(x) : NA_REAL;
  REAL(res)[2] = st_g;
  REAL(res)[3] = g != NULL ? g(x) : NA_REAL;
  zumir_ctx_free(z);
  UNPROTECT(1);
  return res;
}

/* ---- #92: one tier per context, after a link that found nothing ---- */

/* c(first status, second status): compile "nope" natively, then "f" on
 * the interpreter. */
SEXP zc_tier_lock_c(void) {
  load();
  int status;
  zumir_ctx *z = zumir_ctx_new(&status);
  if (z == NULL) Rf_error("zumir_ctx_new: %d", status);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t mod = MIR_new_module(ctx, "m");
  MIR_type_t rt = MIR_T_D;
  MIR_var_t a = {MIR_T_D, "a", 0};
  MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 1, &a);
  MIR_reg_t ra = MIR_reg(ctx, "a", fi->u.func);
  MIR_append_insn(ctx, fi, MIR_new_ret_insn(ctx, 1, MIR_new_reg_op(ctx, ra)));
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  int first, second;
  (void) zumir_compile(z, mod, "nope", ZUMIR_TIER_NATIVE, NULL, &first);
  (void) zumir_compile(z, mod, "f", ZUMIR_TIER_INTERP, NULL, &second);
  zumir_ctx_free(z);
  SEXP res = PROTECT(Rf_allocVector(INTSXP, 2));
  INTEGER(res)[0] = first;
  INTEGER(res)[1] = second;
  UNPROTECT(1);
  return res;
}

/* ---- #78: NULL arguments are statuses ---- */

/* The status after each misuse, each on a fresh context. */
SEXP zc_null_args_c(void) {
  load();
  SEXP res = PROTECT(Rf_allocVector(INTSXP, 6));
  for (int which = 0; which < 6; which++) {
    int status;
    zumir_ctx *z = zumir_ctx_new(&status);
    if (z == NULL) Rf_error("zumir_ctx_new: %d", status);
    MIR_context_t ctx = zumir_mir(z);
    MIR_new_module(ctx, "m");
    MIR_type_t rt = MIR_T_D;
    MIR_var_t a = {MIR_T_D, "a", 0};
    MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 1, &a);
    MIR_reg_t ra = MIR_reg(ctx, "a", fi->u.func);
    MIR_insn_t ret = MIR_new_ret_insn(ctx, 1, MIR_new_reg_op(ctx, ra));
    switch (which) {
    case 0: MIR_append_insn(ctx, fi, NULL); break;
    case 1: MIR_append_insn(ctx, NULL, ret); break;
    case 2: (void) MIR_new_func_reg(ctx, NULL, MIR_T_D, "x"); break;
    case 3: MIR_insert_insn_after(ctx, fi, NULL, ret); break;
    case 4: (void) MIR_new_ref_op(ctx, NULL); break;
    default: (void) MIR_get_module_item(ctx, NULL, "f"); break;
    }
    INTEGER(res)[which] = zumir_status(z, NULL);
    zumir_ctx_free(z);
  }
  UNPROTECT(1);
  return res;
}

/* ---- #60: an interpreted function's first call allocates nothing ---- */

typedef struct {
  zumir_ctx *z;
  double (*fn)(double);
} compiled;

static void compiled_free(SEXP x) {
  compiled *c = R_ExternalPtrAddr(x);
  if (c == NULL) return;
  zumir_ctx_free(c->z);
  free(c);
  R_ClearExternalPtr(x);
}

/* f(a) = sin(a) + 1 on the interpreter, not yet called. */
SEXP zc_interp_compile_c(void) {
  load();
  int status;
  compiled *c = calloc(1, sizeof *c);
  if (c == NULL) Rf_error("no memory");
  c->z = zumir_ctx_new(&status);
  MIR_context_t ctx = zumir_mir(c->z);
  MIR_module_t mod = MIR_new_module(ctx, "m");
  MIR_type_t rt = MIR_T_D;
  MIR_var_t a = {MIR_T_D, "a", 0};
  MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 1, &a);
  zumir_emitter e;
  zumir_emit_begin(&e, ctx, fi);
  MIR_reg_t ra = MIR_reg(ctx, "a", fi->u.func);
  zumir_emit_return_value(&e, zumir_emit_add(&e, zumir_emit_call1(&e, "sin", ra), zumir_emit_const(&e, 1)));
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  c->fn = (double (*)(double)) zumir_compile(c->z, mod, "f", ZUMIR_TIER_INTERP, NULL, &status);
  SEXP x = PROTECT(R_MakeExternalPtr(c, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(x, compiled_free, TRUE);
  UNPROTECT(1);
  return x;
}

/* c(entered, returned, value): a longjmp out of fn would leave returned 0
 * (and, as an R error, never reach here). */
SEXP zc_interp_call_c(SEXP x, SEXP s_a) {
  compiled *c = R_ExternalPtrAddr(x);
  SEXP res = PROTECT(Rf_allocVector(REALSXP, 3));
  REAL(res)[0] = 1;
  REAL(res)[1] = 0;
  double v = c != NULL && c->fn != NULL ? c->fn(Rf_asReal(s_a)) : NA_REAL;
  REAL(res)[1] = 1;
  REAL(res)[2] = v;
  UNPROTECT(1);
  return res;
}
