#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include <pthread.h>
#include <string.h>
#include <zumir.h>

static void load(void) { const char *why; if (zumir_api_load(&why) == NULL) Rf_error("zumir: %s", why); }

/* module with two exported functions f(x)=x+1, g(x)=x*2 */
static MIR_module_t build(MIR_context_t ctx, const char *mname, int do_export) {
  MIR_module_t mod = MIR_new_module(ctx, mname);
  MIR_type_t d = MIR_T_D;
  const char *names[2] = {"f", "g"};
  for (int k = 0; k < 2; k++) {
    MIR_var_t a = {MIR_T_D, "x", 0};
    if (do_export) MIR_new_export(ctx, names[k]);
    MIR_item_t fi = MIR_new_func_arr(ctx, names[k], 1, &d, 1, &a);
    MIR_reg_t x = MIR_reg(ctx, "x", fi->u.func);
    MIR_reg_t r = MIR_new_func_reg(ctx, fi->u.func, MIR_T_D, "r");
    MIR_append_insn(ctx, fi, MIR_new_insn(ctx, k == 0 ? MIR_DADD : MIR_DMUL, MIR_new_reg_op(ctx, r), MIR_new_reg_op(ctx, x),
                                          MIR_new_double_op(ctx, k == 0 ? 1.0 : 2.0)));
    MIR_append_insn(ctx, fi, MIR_new_ret_insn(ctx, 1, MIR_new_reg_op(ctx, r)));
    MIR_finish_func(ctx);
  }
  MIR_finish_module(ctx);
  return mod;
}

SEXP capi_free_twice(void) {
  load(); int st; zumir_ctx *z = zumir_ctx_new(&st);
  zumir_ctx_free(z);
  zumir_ctx_free(z); /* header: "NULL, or twice: does nothing" */
  return Rf_ScalarInteger(st);
}

/* two functions from one module: zumir_compile() twice. tier: 1 native 2 interp. */
SEXP capi_compile_twice(SEXP s_tier, SEXP s_export) {
  load(); int st1 = -1, st2 = -1, t1 = -1, t2 = -1; zumir_ctx *z = zumir_ctx_new(&st1);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t mod = build(ctx, "m", Rf_asInteger(s_export));
  double (*f)(double) = (double (*)(double)) zumir_compile(z, mod, "f", Rf_asInteger(s_tier), &t1, &st1);
  double rf = f ? f(10) : NA_REAL;
  double (*g)(double) = (double (*)(double)) zumir_compile(z, mod, "g", Rf_asInteger(s_tier), &t2, &st2);
  const char *msg = ""; int zs = zumir_status(z, &msg);
  double rg = g ? g(10) : NA_REAL;
  double rf2 = (f && zs == 0) ? f(10) : NA_REAL;
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 7));
  SET_VECTOR_ELT(out, 0, Rf_ScalarInteger(st1)); SET_VECTOR_ELT(out, 1, Rf_ScalarReal(rf));
  SET_VECTOR_ELT(out, 2, Rf_ScalarInteger(st2)); SET_VECTOR_ELT(out, 3, Rf_ScalarReal(rg));
  SET_VECTOR_ELT(out, 4, Rf_ScalarInteger(zs)); SET_VECTOR_ELT(out, 5, Rf_mkString(msg)); SET_VECTOR_ELT(out, 6, Rf_ScalarReal(rf2));
  zumir_ctx_free(z);
  UNPROTECT(1); return out;
}

/* two modules in one context, compiled one after another; first pointer still good? */
SEXP capi_two_modules(SEXP s_tier, SEXP s_export) {
  load(); int st1 = -1, st2 = -1; zumir_ctx *z = zumir_ctx_new(&st1);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t m1 = build(ctx, "m1", Rf_asInteger(s_export));
  double (*f)(double) = (double (*)(double)) zumir_compile(z, m1, "f", Rf_asInteger(s_tier), NULL, &st1);
  double a = f ? f(10) : NA_REAL;
  MIR_module_t m2 = build(ctx, "m2", Rf_asInteger(s_export));
  double (*g)(double) = (double (*)(double)) zumir_compile(z, m2, "g", Rf_asInteger(s_tier), NULL, &st2);
  const char *msg = ""; int zs = zumir_status(z, &msg);
  double b = g ? g(10) : NA_REAL;
  double a2 = (f && zs == 0) ? f(10) : NA_REAL;
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 7));
  SET_VECTOR_ELT(out, 0, Rf_ScalarInteger(st1)); SET_VECTOR_ELT(out, 1, Rf_ScalarReal(a));
  SET_VECTOR_ELT(out, 2, Rf_ScalarInteger(st2)); SET_VECTOR_ELT(out, 3, Rf_ScalarReal(b));
  SET_VECTOR_ELT(out, 4, Rf_ScalarInteger(zs)); SET_VECTOR_ELT(out, 5, Rf_mkString(msg)); SET_VECTOR_ELT(out, 6, Rf_ScalarReal(a2));
  zumir_ctx_free(z);
  UNPROTECT(1); return out;
}

/* tier switch on one context: interp first then native, and reverse */
SEXP capi_tier_switch(SEXP s_t1, SEXP s_t2) {
  load(); int st1 = -1, st2 = -1, o1 = -1, o2 = -1; zumir_ctx *z = zumir_ctx_new(&st1);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t m1 = build(ctx, "m1", 0);
  double (*f)(double) = (double (*)(double)) zumir_compile(z, m1, "f", Rf_asInteger(s_t1), &o1, &st1);
  double a = f ? f(10) : NA_REAL;
  MIR_module_t m2 = build(ctx, "m2", 0);
  double (*g)(double) = (double (*)(double)) zumir_compile(z, m2, "g", Rf_asInteger(s_t2), &o2, &st2);
  const char *msg = ""; int zs = zumir_status(z, &msg);
  double b = g ? g(10) : NA_REAL;
  double a2 = (f && zs == 0) ? f(10) : NA_REAL;
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 9));
  SET_VECTOR_ELT(out, 0, Rf_ScalarInteger(st1)); SET_VECTOR_ELT(out, 1, Rf_ScalarReal(a));
  SET_VECTOR_ELT(out, 2, Rf_ScalarInteger(st2)); SET_VECTOR_ELT(out, 3, Rf_ScalarReal(b));
  SET_VECTOR_ELT(out, 4, Rf_ScalarInteger(zs)); SET_VECTOR_ELT(out, 5, Rf_mkString(msg)); SET_VECTOR_ELT(out, 6, Rf_ScalarReal(a2));
  SET_VECTOR_ELT(out, 7, Rf_ScalarInteger(o1)); SET_VECTOR_ELT(out, 8, Rf_ScalarInteger(o2));
  zumir_ctx_free(z);
  UNPROTECT(1); return out;
}

/* notfound then good; NULL args; compile on unfinished module */
SEXP capi_misc(SEXP s_tier) {
  load(); int st[6] = {-1,-1,-1,-1,-1,-1}; int tier = Rf_asInteger(s_tier);
  zumir_ctx *z = zumir_ctx_new(&st[0]);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t m = build(ctx, "m", 0);
  void *p1 = zumir_compile(z, m, "nope", tier, NULL, &st[1]);
  double (*f)(double) = (double (*)(double)) zumir_compile(z, m, "f", tier, NULL, &st[2]);
  double a = f ? f(10) : NA_REAL;
  void *p3 = zumir_compile(z, m, NULL, tier, NULL, &st[3]);
  void *p4 = zumir_compile(z, m, "f", 7, NULL, &st[4]);
  void *p5 = zumir_compile(NULL, m, "f", tier, NULL, &st[5]);
  int tout = 99; void *p6 = zumir_compile(z, m, "f", tier, &tout, NULL);
  int imp = zumir_import(z, NULL, NULL);
  SEXP out = PROTECT(Rf_allocVector(REALSXP, 13));
  for (int i = 0; i < 6; i++) REAL(out)[i] = st[i];
  REAL(out)[6] = a; REAL(out)[7] = p1 != NULL; REAL(out)[8] = p3 != NULL; REAL(out)[9] = p4 != NULL; REAL(out)[10] = p5 != NULL;
  REAL(out)[11] = p6 != NULL ? ((double (*)(double)) p6)(10) : NA_REAL; REAL(out)[12] = imp;
  zumir_ctx_free(z);
  UNPROTECT(1); return out;
}

/* A consumer frame with cleanup that must run: compile on interp, keep a
 * native resource, call fn; did we get back here? */
static int cleaned = 0, entered = 0;
SEXP capi_reset(void) { cleaned = entered = 0; return R_NilValue; }
SEXP capi_flags(void) { SEXP o = Rf_allocVector(INTSXP, 2); INTEGER(o)[0] = entered; INTEGER(o)[1] = cleaned; return o; }
static zumir_ctx *held;
static double (*held_fn)(double);
SEXP capi_prepare(SEXP s_tier) {
  load(); int st; held = zumir_ctx_new(&st);
  MIR_module_t m = build(zumir_mir(held), "m", 0);
  held_fn = (double (*)(double)) zumir_compile(held, m, "f", Rf_asInteger(s_tier), NULL, &st);
  return Rf_ScalarInteger(st);
}
SEXP capi_call_held(void) {
  entered = 1;
  double r = held_fn(10);   /* D20: no longjmp crosses this frame */
  cleaned = 1;
  return Rf_ScalarReal(r);
}
SEXP capi_free_held(void) { zumir_ctx_free(held); held = NULL; return R_NilValue; }
static void *thr(void *p) { *(double *) p = held_fn(10); return NULL; }
SEXP capi_call_held_thread(void) {
  pthread_t t; double r = -1; pthread_create(&t, NULL, thr, &r); pthread_join(t, NULL);
  return Rf_ScalarReal(r);
}

/* failed context: build error then everything is inert */
SEXP capi_failed(void) {
  load(); int st = -1, st2 = -1; zumir_ctx *z = zumir_ctx_new(&st);
  MIR_context_t ctx = zumir_mir(z);
  MIR_new_module(ctx, "a");
  MIR_module_t m2 = MIR_new_module(ctx, "b"); /* fails: previous not finished */
  const char *msg = ""; int zs = zumir_status(z, &msg);
  MIR_item_t fi = MIR_new_func_arr(ctx, "f", 0, NULL, 0, NULL);
  const char *nm = fi->u.func->name; (void) nm;
  void *p = zumir_compile(z, m2, "f", ZUMIR_TIER_AUTO, NULL, &st2);
  int imp = zumir_import(z, "x", (void *) &capi_reset);
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 5));
  SET_VECTOR_ELT(out, 0, Rf_ScalarInteger(zs)); SET_VECTOR_ELT(out, 1, Rf_mkString(msg));
  SET_VECTOR_ELT(out, 2, Rf_ScalarInteger(st2)); SET_VECTOR_ELT(out, 3, Rf_ScalarLogical(p != NULL)); SET_VECTOR_ELT(out, 4, Rf_ScalarInteger(imp));
  zumir_ctx_free(z);
  UNPROTECT(1); return out;
}
