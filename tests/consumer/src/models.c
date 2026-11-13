/* The spike's three model right-hand sides, written with zumir.h's emitter
 * (D13) through the C API table, and the same functions in C built with
 * -ffp-contract=off (Makevars), so a fused multiply-add cannot make C differ
 * from MIR (#18). Also each comparison and branch helper of the emitter
 * against C. */
#define R_NO_REMAP
#include <math.h>
#include <string.h>
#include <R.h>
#include <Rinternals.h>

#include <zumir.h>

typedef void (*rhs_fn)(double t, const double *y, double *dydt, const double *par, double *work);

static const zumir_api_v1 *api(void) {
  const char *why;
  if (zumir_api_ == NULL && zumir_api_load(&why) == NULL) Rf_error("zumir: %s", why);
  return zumir_api_;
}

/* ---- the models: emitter ---- */

typedef struct {
  zumir_emitter e;
  MIR_reg_t y, dy, par;
} M;

#define LD(b, i) zumir_emit_load(&m->e, b, i)
#define ST(b, i, r) zumir_emit_store(&m->e, b, i, r)
#define ADD(a, c) zumir_emit_add(&m->e, a, c)
#define SUB(a, c) zumir_emit_sub(&m->e, a, c)
#define MUL(a, c) zumir_emit_mul(&m->e, a, c)
#define DIV(a, c) zumir_emit_div(&m->e, a, c)
#define NEG(a) zumir_emit_neg(&m->e, a)

static void emit_m1(M *m) {
  MIR_reg_t ka = LD(m->par, 0), cl = LD(m->par, 1), v = LD(m->par, 2);
  MIR_reg_t y0 = LD(m->y, 0), y1 = LD(m->y, 1);
  MIR_reg_t a = MUL(ka, y0);
  ST(m->dy, 0, NEG(a));
  ST(m->dy, 1, SUB(a, MUL(DIV(cl, v), y1)));
}

static void emit_m2(M *m) {
  MIR_reg_t ka = LD(m->par, 0), vmax = LD(m->par, 1), km = LD(m->par, 2), v1 = LD(m->par, 3),
            q = LD(m->par, 4), v2 = LD(m->par, 5);
  MIR_reg_t y0 = LD(m->y, 0), y1 = LD(m->y, 1), y2 = LD(m->y, 2);
  MIR_reg_t c = DIV(y1, v1);
  MIR_reg_t elim = DIV(MUL(vmax, c), ADD(km, c));
  MIR_reg_t abs = MUL(ka, y0);
  MIR_reg_t qc = MUL(q, c), qp = MUL(q, DIV(y2, v2));
  ST(m->dy, 0, NEG(abs));
  ST(m->dy, 1, ADD(SUB(SUB(abs, elim), qc), qp));
  ST(m->dy, 2, SUB(qc, qp));
}

static void emit_m3(M *m) {
  MIR_reg_t ktr = LD(m->par, 0), cl = LD(m->par, 1), v = LD(m->par, 2), q = LD(m->par, 3),
            v2 = LD(m->par, 4), kin = LD(m->par, 5), kout = LD(m->par, 6),
            imax = LD(m->par, 7), ic50 = LD(m->par, 8);
  MIR_reg_t y[10];
  for (int i = 0; i < 10; i++) y[i] = LD(m->y, i);
  ST(m->dy, 0, NEG(MUL(ktr, y[0])));
  for (int i = 1; i < 7; i++) ST(m->dy, i, MUL(ktr, SUB(y[i - 1], y[i])));
  MIR_reg_t k10 = DIV(cl, v), k12 = DIV(q, v), k21 = DIV(q, v2);
  MIR_reg_t a12 = MUL(k12, y[7]), a21 = MUL(k21, y[8]);
  ST(m->dy, 7, ADD(SUB(SUB(MUL(ktr, y[6]), MUL(k10, y[7])), a12), a21));
  ST(m->dy, 8, SUB(a12, a21));
  MIR_reg_t c = DIV(y[7], v);
  MIR_reg_t inh = DIV(MUL(imax, c), ADD(ic50, c));
  ST(m->dy, 9, SUB(MUL(kin, SUB(zumir_emit_const(&m->e, 1.0), inh)), MUL(kout, y[9])));
}

/* ---- the models: C ---- */

static void c_m1(double t, const double *y, double *dy, const double *p, double *w) {
  (void) t;
  (void) w;
  double a = p[0] * y[0];
  dy[0] = -a;
  dy[1] = a - (p[1] / p[2]) * y[1];
}

static void c_m2(double t, const double *y, double *dy, const double *p, double *w) {
  (void) t;
  (void) w;
  double c = y[1] / p[3];
  double elim = (p[1] * c) / (p[2] + c);
  double abs = p[0] * y[0];
  double qc = p[4] * c, qp = p[4] * (y[2] / p[5]);
  dy[0] = -abs;
  dy[1] = ((abs - elim) - qc) + qp;
  dy[2] = qc - qp;
}

static void c_m3(double t, const double *y, double *dy, const double *p, double *w) {
  (void) t;
  (void) w;
  double ktr = p[0];
  dy[0] = -(ktr * y[0]);
  for (int i = 1; i < 7; i++) dy[i] = ktr * (y[i - 1] - y[i]);
  double k10 = p[1] / p[2], k12 = p[3] / p[2], k21 = p[3] / p[4];
  double a12 = k12 * y[7], a21 = k21 * y[8];
  dy[7] = (((ktr * y[6]) - (k10 * y[7])) - a12) + a21;
  dy[8] = a12 - a21;
  double c = y[7] / p[2];
  double inh = (p[7] * c) / (p[8] + c);
  dy[9] = (p[5] * (1.0 - inh)) - (p[6] * y[9]);
}

static const struct {
  int neq, npar;
  void (*emit)(M *);
  rhs_fn c;
} MODELS[3] = {{2, 3, emit_m1, c_m1}, {3, 6, emit_m2, c_m2}, {10, 9, emit_m3, c_m3}};

static int tier_code(SEXP tier) {
  const char *t = CHAR(STRING_ELT(tier, 0));
  return strcmp(t, "native") == 0 ? ZUMIR_TIER_NATIVE
         : strcmp(t, "interp") == 0 ? ZUMIR_TIER_INTERP
                                    : ZUMIR_TIER_AUTO;
}

/* list(mir = dydt from MIR, c = dydt from C, tier) at t = 0.5. */
SEXP zc_rhs_c(SEXP s_model, SEXP s_tier, SEXP y, SEXP par) {
  api();
  int k = Rf_asInteger(s_model) - 1;
  if (k < 0 || k > 2) Rf_error("model must be 1, 2 or 3");
  if (XLENGTH(y) != MODELS[k].neq || XLENGTH(par) != MODELS[k].npar) Rf_error("wrong lengths");
  int status, tier;
  zumir_ctx *z = zumir_ctx_new(&status);
  if (z == NULL) Rf_error("zumir_ctx_new: status %d", status);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t mod = MIR_new_module(ctx, "m");
  MIR_var_t args[5] = {{MIR_T_D, "t", 0}, {MIR_T_P, "y", 0}, {MIR_T_P, "dydt", 0},
                       {MIR_T_P, "par", 0}, {MIR_T_P, "work", 0}};
  MIR_item_t fi = MIR_new_func_arr(ctx, "rhs", 0, NULL, 5, args);
  M m;
  zumir_emit_begin(&m.e, ctx, fi);
  m.y = MIR_reg(ctx, "y", fi->u.func);
  m.dy = MIR_reg(ctx, "dydt", fi->u.func);
  m.par = MIR_reg(ctx, "par", fi->u.func);
  MODELS[k].emit(&m);
  zumir_emit_return(&m.e);
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  void *fn = zumir_compile(z, mod, "rhs", tier_code(s_tier), &tier, &status);
  if (fn == NULL) {
    const char *msg;
    zumir_status(z, &msg);
    char copy[512];
    snprintf(copy, sizeof copy, "%s", msg);
    zumir_ctx_free(z);
    Rf_error("zumir_compile: status %d: %s", status, copy);
  }
  SEXP a = PROTECT(Rf_allocVector(REALSXP, MODELS[k].neq));
  SEXP b = PROTECT(Rf_allocVector(REALSXP, MODELS[k].neq));
  double work[16];
  memset(REAL(a), 0, sizeof(double) * (size_t) MODELS[k].neq);
  memset(REAL(b), 0, sizeof(double) * (size_t) MODELS[k].neq);
  ((rhs_fn) fn)(0.5, REAL(y), REAL(a), REAL(par), work);
  MODELS[k].c(0.5, REAL(y), REAL(b), REAL(par), work);
  zumir_ctx_free(z);
  const char *names[] = {"mir", "c", "tier", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, a);
  SET_VECTOR_ELT(res, 1, b);
  SET_VECTOR_ELT(res, 2, Rf_mkString(tier == ZUMIR_TIER_NATIVE ? "native" : "interp"));
  UNPROTECT(3);
  return res;
}

/* ---- comparisons and branches ---- */

static const struct {
  const char *name;
  MIR_insn_code_t cmp, branch;
} CMPS[6] = {{"eq", MIR_DEQ, MIR_DBEQ}, {"ne", MIR_DNE, MIR_DBNE}, {"lt", MIR_DLT, MIR_DBLT},
             {"le", MIR_DLE, MIR_DBLE}, {"gt", MIR_DGT, MIR_DBGT}, {"ge", MIR_DGE, MIR_DBGE}};

static int c_cmp(int k, double a, double b) {
  switch (k) {
  case 0: return a == b;
  case 1: return a != b;
  case 2: return a < b;
  case 3: return a <= b;
  case 4: return a > b;
  default: return a >= b;
  }
}

/* For `what` "cmp" (zumir_emit_cmp) or "branch" (zumir_emit_branch, then
 * zumir_emit_branch_true on the cmp result), the emitted function's
 * answers and C's on each pair (a[i], b[i]): list(mir, c), integer 0/1. */
SEXP zc_cmp_c(SEXP what, SEXP code, SEXP a, SEXP b) {
  api();
  int k = -1;
  for (int i = 0; i < 6; i++)
    if (strcmp(CMPS[i].name, CHAR(STRING_ELT(code, 0))) == 0) k = i;
  if (k < 0) Rf_error("unknown comparison");
  int branch = strcmp(CHAR(STRING_ELT(what, 0)), "branch") == 0;
  int bt = strcmp(CHAR(STRING_ELT(what, 0)), "branch_true") == 0;
  int status;
  zumir_ctx *z = zumir_ctx_new(&status);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t mod = MIR_new_module(ctx, "m");
  MIR_type_t rt = MIR_T_D;
  MIR_var_t args[2] = {{MIR_T_D, "a", 0}, {MIR_T_D, "b", 0}};
  MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 2, args);
  zumir_emitter e;
  zumir_emit_begin(&e, ctx, fi);
  MIR_reg_t ra = MIR_reg(ctx, "a", fi->u.func), rb = MIR_reg(ctx, "b", fi->u.func);
  if (branch || bt) {
    MIR_label_t yes = zumir_emit_label(&e);
    if (branch)
      zumir_emit_branch(&e, CMPS[k].branch, ra, rb, yes);
    else
      zumir_emit_branch_true(&e, zumir_emit_cmp(&e, CMPS[k].cmp, ra, rb), yes);
    zumir_emit_return_value(&e, zumir_emit_const(&e, 0.0));
    zumir_emit_place(&e, yes);
    zumir_emit_return_value(&e, zumir_emit_const(&e, 1.0));
  } else {
    MIR_reg_t c = zumir_emit_cmp(&e, CMPS[k].cmp, ra, rb);
    MIR_reg_t d = zumir_emit_reg(&e, MIR_T_D);
    zumir_emit_insn(&e, MIR_new_insn(ctx, MIR_I2D, zumir_emit_r(&e, d), zumir_emit_r(&e, c)));
    zumir_emit_return_value(&e, d);
  }
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  double (*f)(double, double) =
    (double (*)(double, double)) zumir_compile(z, mod, "f", ZUMIR_TIER_AUTO, NULL, &status);
  if (f == NULL) {
    zumir_ctx_free(z);
    Rf_error("zumir_compile: status %d", status);
  }
  R_xlen_t n = XLENGTH(a);
  SEXP mir = PROTECT(Rf_allocVector(INTSXP, n));
  SEXP c = PROTECT(Rf_allocVector(INTSXP, n));
  for (R_xlen_t i = 0; i < n; i++) {
    INTEGER(mir)[i] = (int) f(REAL(a)[i], REAL(b)[i]);
    INTEGER(c)[i] = c_cmp(k, REAL(a)[i], REAL(b)[i]);
  }
  zumir_ctx_free(z);
  const char *names[] = {"mir", "c", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, mir);
  SET_VECTOR_ELT(res, 1, c);
  UNPROTECT(3);
  return res;
}
