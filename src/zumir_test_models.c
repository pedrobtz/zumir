/* Test-only: the spike's three model right-hand sides (pedrobtz/mirspike),
 * emitted through MIR's API as zupmxode's lowering would, for the tier and
 * thread tests (tests/testthat/test-tiers.R, test-threads.R). Stage 5
 * moves the emitter into inst/include/zumir.h. The signature is RFC 0047
 * §4.7's: void rhs(double t, const double *y, double *dydt,
 * const double *par, double *work). */
#include <math.h>
#include <pthread.h>
#include <string.h>

#include "zumir_ctx.h"

#if ZUMIR_HAVE_MIR
#include "vendor/mir/mir-gen.h"
#include "zumir_trap.h"

typedef void (*rhs_fn)(double t, const double *y, double *dydt, const double *par,
                       double *work);

typedef struct {
  MIR_context_t ctx;
  MIR_item_t fi;
  MIR_func_t f;
  MIR_reg_t t, y, dy, par, work;
  int ntmp;
} B;

static MIR_op_t OR(B *b, MIR_reg_t r) { return MIR_new_reg_op(b->ctx, r); }
static MIR_op_t OM(B *b, MIR_reg_t base, int i) {
  return MIR_new_mem_op(b->ctx, MIR_T_D, (MIR_disp_t) i * 8, base, 0, 1);
}
static void app(B *b, MIR_insn_t insn) { MIR_append_insn(b->ctx, b->fi, insn); }
static MIR_reg_t tmp(B *b) {
  char nm[32];
  snprintf(nm, sizeof nm, "r%d", b->ntmp++);
  return MIR_new_func_reg(b->ctx, b->f, MIR_T_D, nm);
}
static MIR_reg_t ld(B *b, MIR_reg_t base, int i) {
  MIR_reg_t r = tmp(b);
  app(b, MIR_new_insn(b->ctx, MIR_DMOV, OR(b, r), OM(b, base, i)));
  return r;
}
static void st(B *b, MIR_reg_t base, int i, MIR_reg_t r) {
  app(b, MIR_new_insn(b->ctx, MIR_DMOV, OM(b, base, i), OR(b, r)));
}
static MIR_reg_t op2(B *b, MIR_insn_code_t code, MIR_reg_t x, MIR_reg_t y) {
  MIR_reg_t r = tmp(b);
  app(b, MIR_new_insn(b->ctx, code, OR(b, r), OR(b, x), OR(b, y)));
  return r;
}
static MIR_reg_t neg(B *b, MIR_reg_t x) {
  MIR_reg_t r = tmp(b);
  app(b, MIR_new_insn(b->ctx, MIR_DNEG, OR(b, r), OR(b, x)));
  return r;
}
static MIR_reg_t cst(B *b, double v) {
  MIR_reg_t r = tmp(b);
  app(b, MIR_new_insn(b->ctx, MIR_DMOV, OR(b, r), MIR_new_double_op(b->ctx, v)));
  return r;
}
#define ADD(a, c) op2(b, MIR_DADD, a, c)
#define SUB(a, c) op2(b, MIR_DSUB, a, c)
#define MUL(a, c) op2(b, MIR_DMUL, a, c)
#define DIV(a, c) op2(b, MIR_DDIV, a, c)

/* One compartment, first-order absorption. y = (depot, central);
 * par = (ka, cl, v). */
static void emit_m1(B *b) {
  MIR_reg_t ka = ld(b, b->par, 0), cl = ld(b, b->par, 1), v = ld(b, b->par, 2);
  MIR_reg_t y0 = ld(b, b->y, 0), y1 = ld(b, b->y, 1);
  MIR_reg_t a = MUL(ka, y0);
  st(b, b->dy, 0, neg(b, a));
  st(b, b->dy, 1, SUB(a, MUL(DIV(cl, v), y1)));
}

/* Two compartments, Michaelis-Menten elimination. */
static void emit_m2(B *b) {
  MIR_reg_t ka = ld(b, b->par, 0), vmax = ld(b, b->par, 1), km = ld(b, b->par, 2),
            v1 = ld(b, b->par, 3), q = ld(b, b->par, 4), v2 = ld(b, b->par, 5);
  MIR_reg_t y0 = ld(b, b->y, 0), y1 = ld(b, b->y, 1), y2 = ld(b, b->y, 2);
  MIR_reg_t c = DIV(y1, v1);
  MIR_reg_t elim = DIV(MUL(vmax, c), ADD(km, c));
  MIR_reg_t abs = MUL(ka, y0);
  MIR_reg_t qc = MUL(q, c), qp = MUL(q, DIV(y2, v2));
  st(b, b->dy, 0, neg(b, abs));
  st(b, b->dy, 1, ADD(SUB(SUB(abs, elim), qc), qp));
  st(b, b->dy, 2, SUB(qc, qp));
}

/* Ten states: seven transit compartments, two-compartment disposition and
 * an indirect response with Imax inhibition. */
static void emit_m3(B *b) {
  MIR_reg_t ktr = ld(b, b->par, 0), cl = ld(b, b->par, 1), v = ld(b, b->par, 2),
            q = ld(b, b->par, 3), v2 = ld(b, b->par, 4), kin = ld(b, b->par, 5),
            kout = ld(b, b->par, 6), imax = ld(b, b->par, 7), ic50 = ld(b, b->par, 8);
  MIR_reg_t y[10];
  for (int i = 0; i < 10; i++) y[i] = ld(b, b->y, i);
  st(b, b->dy, 0, neg(b, MUL(ktr, y[0])));
  for (int i = 1; i < 7; i++) st(b, b->dy, i, MUL(ktr, SUB(y[i - 1], y[i])));
  MIR_reg_t k10 = DIV(cl, v), k12 = DIV(q, v), k21 = DIV(q, v2);
  MIR_reg_t a12 = MUL(k12, y[7]), a21 = MUL(k21, y[8]);
  st(b, b->dy, 7, ADD(SUB(SUB(MUL(ktr, y[6]), MUL(k10, y[7])), a12), a21));
  st(b, b->dy, 8, SUB(a12, a21));
  MIR_reg_t c = DIV(y[7], v);
  MIR_reg_t inh = DIV(MUL(imax, c), ADD(ic50, c));
  st(b, b->dy, 9, SUB(MUL(kin, SUB(cst(b, 1.0), inh)), MUL(kout, y[9])));
}

typedef struct {
  const char *name;
  int neq, npar;
  void (*emit)(B *);
  double par[9];
  double y0[10];
} model_t;

static const model_t MODELS[3] = {
  {"one_cmt", 2, 3, emit_m1, {1.2, 2.5, 30}, {100, 0}},
  {"two_cmt_mm", 3, 6, emit_m2, {1.2, 40, 2.5, 30, 4, 60}, {100, 0, 0}},
  {"ten_state", 10, 9, emit_m3, {2, 2.5, 30, 4, 60, 10, 0.1, 0.8, 1.5},
   {100, 0, 0, 0, 0, 0, 0, 0, 0, 100}},
};

static const model_t *get_model(SEXP s) {
  int m = Rf_asInteger(s);
  if (m < 1 || m > 3) Rf_error("model must be 1, 2 or 3");
  return &MODELS[m - 1];
}

/* Emits function "rhs" for model m into the current module. */
static void emit_rhs(MIR_context_t ctx, const model_t *m) {
  B b;
  memset(&b, 0, sizeof b);
  b.ctx = ctx;
  MIR_var_t args[5] = {{MIR_T_D, "t", 0},
                       {MIR_T_P, "y", 0},
                       {MIR_T_P, "dydt", 0},
                       {MIR_T_P, "par", 0},
                       {MIR_T_P, "work", 0}};
  b.fi = MIR_new_func_arr(ctx, "rhs", 0, NULL, 5, args);
  b.f = b.fi->u.func;
  b.t = MIR_reg(ctx, "t", b.f);
  b.y = MIR_reg(ctx, "y", b.f);
  b.dy = MIR_reg(ctx, "dydt", b.f);
  b.par = MIR_reg(ctx, "par", b.f);
  b.work = MIR_reg(ctx, "work", b.f);
  m->emit(&b);
  app(&b, MIR_new_ret_insn(ctx, 0));
  MIR_finish_func(ctx);
}

static void body_emit(zumir_rctx *z, void *arg) {
  emit_rhs(z->ctx, (const model_t *) arg);
}

/* Emits model `model`'s "rhs" into the module the context has begun. */
SEXP zumir_test_emit_model(SEXP x, SEXP model) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  const model_t *m = get_model(model);
  if (!zumir_rctx_run(z, body_emit, (void *) m)) return zumir_status("mir", z->type, z->message);
  return R_NilValue;
}

/* Calls the linked "rhs" once at t = 0.5 with y and par from R; returns
 * dydt. */
SEXP zumir_test_call_rhs(SEXP x, SEXP model, SEXP y, SEXP par) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  const model_t *m = get_model(model);
  if (z->fn == NULL) return zumir_status("context", 0, "nothing linked yet");
  if (XLENGTH(y) != m->neq || XLENGTH(par) != m->npar) Rf_error("wrong lengths");
  SEXP dy = PROTECT(Rf_allocVector(REALSXP, m->neq));
  double work[16];
  memset(REAL(dy), 0, sizeof(double) * (size_t) m->neq);
  ((rhs_fn) z->fn)(0.5, REAL(y), REAL(dy), REAL(par), work);
  UNPROTECT(1);
  return dy;
}

/* ---- Threads (design §4.3 "Threads", D10). ---- */

typedef struct {
  MIR_context_t ctx;
  zumir_arena *arena;
  int gen;
  rhs_fn fn;
  int ok;
} compiled;

/* Compiles model m in a fresh context on `tier`, without R. */
static compiled compile_model(const model_t *m, int tier) {
  compiled c;
  memset(&c, 0, sizeof c);
  MIR_context_t volatile ctx = NULL;
  zumir_arena *volatile arena = NULL;
  volatile int gen = 0;
  zumir_trap t;
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    ctx = zumir_mir_new(&arena);
    if (tier == ZUMIR_TIER_NATIVE) {
      MIR_gen_init(ctx);
      gen = 1;
      MIR_gen_set_optimize_level(ctx, 2);
    }
    MIR_new_module(ctx, "m");
    emit_rhs(ctx, m);
    MIR_finish_module(ctx);
    MIR_module_t mod = DLIST_TAIL(MIR_module_t, *MIR_get_module_list(ctx));
    MIR_load_module(ctx, mod);
    MIR_link(ctx, tier == ZUMIR_TIER_NATIVE ? MIR_set_gen_interface : MIR_set_interp_interface,
             NULL);
    for (MIR_item_t it = DLIST_HEAD(MIR_item_t, mod->items); it != NULL;
         it = DLIST_NEXT(MIR_item_t, it))
      if (it->item_type == MIR_func_item) c.fn = (rhs_fn) it->addr;
    zumir_trap_pop(&t);
    c.ok = c.fn != NULL;
  }
  c.ctx = ctx;
  c.arena = arena;
  c.gen = gen;
  return c;
}

static void free_compiled(compiled *c) {
  zumir_mir_free(c->ctx, c->arena, c->gen);
  memset(c, 0, sizeof(*c));
}

/* Euler steps from y0 with par; the final state goes to out. */
static void traj(rhs_fn f, const model_t *m, const double *par, int nsteps, double *out) {
  double y[10], dy[10], work[16];
  memcpy(y, m->y0, sizeof y);
  double t = 0, h = 1e-3;
  for (int k = 0; k < nsteps; k++) {
    f(t, y, dy, par, work);
    for (int j = 0; j < m->neq; j++) y[j] += h * dy[j];
    t += h;
  }
  memcpy(out, y, sizeof(double) * (size_t) m->neq);
}

typedef struct {
  rhs_fn f;
  const model_t *m;
  const double *pars;
  double *out;
  int nsub, nsteps, start, step;
  int own_tier; /* -1: run f; else compile a context of this tier here */
  int ok;
} job;

static void run_job(job *a) {
  rhs_fn f = a->f;
  compiled c;
  memset(&c, 0, sizeof c);
  a->ok = 1;
  if (a->own_tier >= 0) {
    c = compile_model(a->m, a->own_tier);
    if (!c.ok) {
      a->ok = 0;
      free_compiled(&c);
      return;
    }
    f = c.fn;
  }
  for (int s = a->start; s < a->nsub; s += a->step)
    traj(f, a->m, a->pars + (size_t) s * a->m->npar, a->nsteps,
         a->out + (size_t) s * a->m->neq);
  if (a->own_tier >= 0) free_compiled(&c);
}

static void *thread_main(void *p) {
  run_job((job *) p);
  return NULL;
}

/* Runs `nsub` subjects of model `model` on `nthreads` threads and compares
 * the final states with a serial run, bitwise. `mode`: "native" (compiled
 * once, shared), "interp_shared" (one interpreted context, shared: D10
 * says this is wrong), "interp_per_thread" (one context per thread, D10).
 * Returns list(identical, ok). */
SEXP zumir_test_threads(SEXP model, SEXP s_nthreads, SEXP s_nsub, SEXP s_nsteps, SEXP s_mode) {
  const model_t *m = get_model(model);
  int nth = Rf_asInteger(s_nthreads), nsub = Rf_asInteger(s_nsub),
      nsteps = Rf_asInteger(s_nsteps);
  const char *mode = CHAR(STRING_ELT(s_mode, 0));
  if (nth < 1 || nth > 64 || nsub < 1 || nsteps < 1) Rf_error("bad sizes");
  int tier = strcmp(mode, "native") == 0 ? ZUMIR_TIER_NATIVE : ZUMIR_TIER_INTERP;
  int own = strcmp(mode, "interp_per_thread") == 0 ? ZUMIR_TIER_INTERP : -1;
  double *pars = (double *) R_alloc((size_t) nsub * m->npar, sizeof(double));
  double *serial = (double *) R_alloc((size_t) nsub * m->neq, sizeof(double));
  double *par = (double *) R_alloc((size_t) nsub * m->neq, sizeof(double));
  for (int s = 0; s < nsub; s++)
    for (int k = 0; k < m->npar; k++)
      pars[(size_t) s * m->npar + k] = m->par[k] * (1.0 + 0.01 * (s % 17) - 0.003 * k);
  memset(par, 0, sizeof(double) * (size_t) nsub * m->neq);

  compiled c = compile_model(m, tier);
  if (!c.ok) {
    free_compiled(&c);
    Rf_error("compiling the model failed");
  }
  job one = {c.fn, m, pars, serial, nsub, nsteps, 0, 1, -1, 1};
  run_job(&one);
  job jobs[64];
  pthread_t th[64];
  for (int k = 0; k < nth; k++)
    jobs[k] = (job){c.fn, m, pars, par, nsub, nsteps, k, nth, own, 1};
  for (int k = 0; k < nth; k++) pthread_create(&th[k], NULL, thread_main, &jobs[k]);
  for (int k = 0; k < nth; k++) pthread_join(th[k], NULL);
  free_compiled(&c);
  int ok = 1;
  for (int k = 0; k < nth; k++) ok = ok && jobs[k].ok;
  int same = ok && memcmp(serial, par, sizeof(double) * (size_t) nsub * m->neq) == 0;
  const char *names[] = {"identical", "ok", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_ScalarLogical(same));
  SET_VECTOR_ELT(res, 1, Rf_ScalarLogical(ok));
  UNPROTECT(1);
  return res;
}
#else
SEXP zumir_test_emit_model(SEXP x, SEXP model) {
  (void) x;
  (void) model;
  return zumir_status("context", 0, "zumir was built without MIR");
}
SEXP zumir_test_call_rhs(SEXP x, SEXP model, SEXP y, SEXP par) {
  (void) x;
  (void) model;
  (void) y;
  (void) par;
  return zumir_status("context", 0, "zumir was built without MIR");
}
SEXP zumir_test_threads(SEXP model, SEXP a, SEXP b, SEXP c, SEXP d) {
  (void) model;
  (void) a;
  (void) b;
  (void) c;
  (void) d;
  Rf_error("zumir was built without MIR");
}
#endif
