/* The import resolver (design D6, D14, D18). What a context's code may call
 * by name is a fixed set, loaded with MIR_load_external just before
 * linking; any other name is "import of undefined item", a link error:
 *
 *   ZUMIR_IMPORTS_D6         the C math library, memcpy, memmove, memset,
 *                            memcmp, strlen (cfunction(), mir_function());
 *   ZUMIR_IMPORTS_CONSUMER   D6's list, plus what the consumer registered
 *                            in that context with zumir_import() (D14);
 *   ZUMIR_IMPORTS_RFUNCTION  mir_compile()'s internal list: part of the math
 *                            library, R_pow, zumir's C ports of R's %%,
 *                            %/%, min and max, and the interrupt poll (D18,
 *                            D23). User source never names these.
 *
 * The ports follow R's own code (src/main/arithmetic.c myfmod and myfloor,
 * src/main/summary.c rmin/rmax and the do_summary combine step), so their
 * results are R's bit for bit; R uses long double there only when it was
 * built with it, which .onLoad passes in (zumir_set_long_double). */
#define R_NO_REMAP
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Arith.h>

#include "zumir_imports.h"

/* Rmath.h's declaration, without Rmath.h's macros. */
double R_pow(double x, double y);

/* ---- R's arithmetic, ported ---- */

static int use_long_double = 1;

SEXP zumir_set_long_double(SEXP on) {
  use_long_double = Rf_asLogical(on) == TRUE && sizeof(long double) > sizeof(double);
  return R_NilValue;
}

/* Set by zumir_r_fmod where R warns "probable complete loss of accuracy in
 * modulus"; mir_compile()'s wrapper reads and clears it (Stage 6). */
_Thread_local int zumir_r_fmod_warned;

static double c_eps(void) {
  return use_long_double ? (double) LDBL_EPSILON : DBL_EPSILON;
}

/* R's myfmod: x1 %% x2. */
double zumir_r_fmod(double x1, double x2) {
  if (x2 == 0.0) return R_NaN;
  if (fabs(x2) * c_eps() > 1 && R_FINITE(x1) && fabs(x1) <= fabs(x2)) {
    return (fabs(x1) == fabs(x2)) ? 0
           : ((x1 < 0 && x2 > 0) || (x2 < 0 && x1 > 0)) ? x1 + x2
                                                          : x1;
  }
  double q = x1 / x2;
  if (R_FINITE(q) && (fabs(q) * c_eps() > 1)) zumir_r_fmod_warned = 1;
  if (use_long_double) {
    long double tmp = (long double) x1 - floor(q) * (long double) x2;
    return (double) (tmp - floorl(tmp / x2) * x2);
  }
  double tmp = x1 - floor(q) * x2;
  return tmp - floor(tmp / x2) * x2;
}

/* R's myfloor: x1 %/% x2. */
double zumir_r_intdiv(double x1, double x2) {
  double q = x1 / x2;
  if (x2 == 0.0 || fabs(q) * c_eps() > 1 || !R_FINITE(q)) return q;
  if (fabs(q) < 1) return (q < 0) ? -1 : ((x1 < 0 && x2 > 0) || (x1 > 0 && x2 < 0) ? -1 : 0);
  if (use_long_double) {
    long double tmp = (long double) x1 - floor(q) * (long double) x2;
    return (double) (floor(q) + floorl(tmp / x2));
  }
  double tmp = x1 - floor(q) * x2;
  return floor(q) + floor(tmp / x2);
}

/* do_summary's combine step for doubles: NA trumps anything, NaN
 * accumulates by +=, otherwise the smaller (min) or larger (max). */
static void combine(double *z, double t, int max) {
  if (R_IsNA(*z)) return;
  if (ISNAN(t)) {
    if (R_IsNA(t))
      *z = t;
    else
      *z += t;
  } else if ((!max && t < *z) || (max && t > *z)) {
    *z = t;
  }
}

/* R's min(x1, x2) and max(x1, x2) for two doubles. */
double zumir_r_min2(double x1, double x2) {
  double z = R_PosInf;
  combine(&z, x1, 0);
  combine(&z, x2, 0);
  return z;
}

double zumir_r_max2(double x1, double x2) {
  double z = R_NegInf;
  combine(&z, x1, 1);
  combine(&z, x2, 1);
  return z;
}

/* ---- R's math1 for mir_compile()'s math table ----
 *
 * R 4.6 wraps several functions with small-argument shortcuts (src/main/
 * arithmetic.c: Rexp, Rexpm1, Rlog1p, Rsin, Rcos, Rtan, Rsqrt) and calls
 * them through math1_ari, which returns a fixed value at one argument;
 * R 4.5 calls the C library through math1. Both then keep an incoming NaN
 * as it was and flag a NaN made from a number ("NaNs produced"). These
 * follow the R the session runs: .onLoad passes getRversion() >= 4.6.0. */

static int r46 = 1;

SEXP zumir_set_r46(SEXP on) {
  r46 = Rf_asLogical(on) == TRUE;
  return R_NilValue;
}

/* Set where R would warn "NaNs produced"; mir_compile()'s wrapper reads and
 * clears it. */
_Thread_local int zumir_r_nan_warned;

static double nan_rule(double x, double y) {
  if (ISNAN(y)) {
    if (ISNAN(x)) return x; /* the incoming NaN is preserved */
    zumir_r_nan_warned = 1;
  }
  return y;
}

static double f_x_x(double x, double (*f)(double), double m) {
  return (fabs(x) <= m) ? x : f(x);
}

static double r_exp(double x) {
  return (fabs(x) <= sqrt(DBL_EPSILON)) ? 1. + x : exp(x);
}
static double r_expm1(double x) {
  return f_x_x(x, expm1, DBL_EPSILON);
}
static double r_log1p(double x) {
  return f_x_x(x, log1p, DBL_EPSILON);
}
static double r_sin(double x) {
  return f_x_x(x, sin, sqrt(3. * DBL_EPSILON));
}
static double r_tan(double x) {
  return f_x_x(x, tan, sqrt(1.5 * DBL_EPSILON));
}
static double r_cos(double x) {
  if (fabs(x) < sqrt(sqrt(12. * DBL_EPSILON))) return (1. - x * x * 0.5);
  return cos(x);
}
static double r_sqrt(double x) {
  if (x == 0.) return x;
  for (int i = 1; i < 12; i++)
    if (x == i * i) return i;
  return sqrt(x);
}

/* math1_ari: res at x == arg, else f(x); then the NaN rule. */
static double ari(double x, double (*f)(double), double arg, double res) {
  return nan_rule(x, x == arg ? res : f(x));
}

double zumir_r_exp(double x) {
  return r46 ? ari(x, r_exp, 0., 1.) : nan_rule(x, exp(x));
}
double zumir_r_expm1(double x) {
  return r46 ? ari(x, r_expm1, 0., 0.) : nan_rule(x, expm1(x));
}
double zumir_r_log1p(double x) {
  return r46 ? ari(x, r_log1p, 0., 0.) : nan_rule(x, log1p(x));
}
double zumir_r_sin(double x) {
  return r46 ? ari(x, r_sin, 0., 0.) : nan_rule(x, sin(x));
}
double zumir_r_cos(double x) {
  return r46 ? ari(x, r_cos, 0., 1.) : nan_rule(x, cos(x));
}
double zumir_r_tan(double x) {
  return r46 ? ari(x, r_tan, 0., 0.) : nan_rule(x, tan(x));
}
double zumir_r_sqrt(double x) {
  return nan_rule(x, r46 ? r_sqrt(x) : sqrt(x));
}
/* R_log (src/main/arithmetic.h), through math1. */
double zumir_r_log(double x) {
  return nan_rule(x, x > 0 ? log(x) : x == 0 ? R_NegInf : R_NaN);
}
double zumir_r_floor(double x) {
  return nan_rule(x, floor(x));
}
double zumir_r_ceil(double x) {
  return nan_rule(x, ceil(x));
}
double zumir_r_trunc(double x) {
  return nan_rule(x, trunc(x));
}
double zumir_r_abs(double x) {
  return fabs(x);
}

/* 1 when x is R's NA (not another NaN): R_IsNA. */
int64_t zumir_r_isna(double x) {
  return R_IsNA(x) ? 1 : 0;
}

/* The interrupt poll (D23): 1 when R has an interrupt or a time limit
 * pending, 0 otherwise. R_CheckUserInterrupt runs under R_tryCatchError,
 * which keeps an error's message (a time limit is an error, and R resets
 * the limit as it raises it), inside R_ToplevelExec, which stops an
 * interrupt; both jumps end in frames below the generated code. */
_Thread_local char zumir_r_poll_message[512];

static SEXP poll_body(void *data) {
  (void) data;
  R_CheckUserInterrupt();
  return R_NilValue;
}

static SEXP poll_error(SEXP cond, void *data) {
  (void) data;
  SEXP msg = R_NilValue;
  if (TYPEOF(cond) == VECSXP && XLENGTH(cond) > 0) msg = VECTOR_ELT(cond, 0);
  snprintf(zumir_r_poll_message, sizeof zumir_r_poll_message, "%s",
           TYPEOF(msg) == STRSXP && XLENGTH(msg) > 0 ? CHAR(STRING_ELT(msg, 0)) : "error");
  return R_NilValue;
}

static void poll_toplevel(void *data) {
  (void) data;
  R_tryCatchError(poll_body, NULL, poll_error, NULL);
}

int zumir_r_poll(void) {
  zumir_r_poll_message[0] = '\0';
  int ok = R_ToplevelExec(poll_toplevel, NULL);
  return (!ok || zumir_r_poll_message[0] != '\0') ? 1 : 0;
}

/* ---- The lists ---- */

static const zumir_import_entry rfunction_list[] = {
  {"zumir_r_exp", (void *) zumir_r_exp},
  {"zumir_r_expm1", (void *) zumir_r_expm1},
  {"zumir_r_log1p", (void *) zumir_r_log1p},
  {"zumir_r_sin", (void *) zumir_r_sin},
  {"zumir_r_cos", (void *) zumir_r_cos},
  {"zumir_r_tan", (void *) zumir_r_tan},
  {"zumir_r_sqrt", (void *) zumir_r_sqrt},
  {"zumir_r_log", (void *) zumir_r_log},
  {"zumir_r_floor", (void *) zumir_r_floor},
  {"zumir_r_ceil", (void *) zumir_r_ceil},
  {"zumir_r_trunc", (void *) zumir_r_trunc},
  {"zumir_r_abs", (void *) zumir_r_abs},
  {"zumir_r_isna", (void *) zumir_r_isna},
  {"R_pow", (void *) R_pow},
  {"zumir_r_fmod", (void *) zumir_r_fmod},
  {"zumir_r_intdiv", (void *) zumir_r_intdiv},
  {"zumir_r_min2", (void *) zumir_r_min2},
  {"zumir_r_max2", (void *) zumir_r_max2},
  {"zumir_r_poll", (void *) zumir_r_poll},
};

#define LEN(a) (sizeof(a) / sizeof((a)[0]))

const zumir_import_entry *zumir_import_list(int kind, size_t *n) {
  if (kind == ZUMIR_IMPORTS_RFUNCTION) {
    *n = LEN(rfunction_list);
    return rfunction_list;
  }
  return zumir_import_d6(n);
}

#if ZUMIR_HAVE_MIR
void zumir_imports_load(MIR_context_t ctx, int kind, const zumir_import_entry *extra,
                        size_t nextra) {
  size_t n;
  const zumir_import_entry *list = zumir_import_list(kind, &n);
  for (size_t i = 0; i < n; i++) MIR_load_external(ctx, list[i].name, list[i].addr);
  if (kind == ZUMIR_IMPORTS_CONSUMER)
    for (size_t i = 0; i < nextra; i++) MIR_load_external(ctx, extra[i].name, extra[i].addr);
}
#endif

/* Test entry: one of mir_compile()'s math wrappers called directly on x,
 * returning list(value, warned) for comparison with R's function. */
SEXP zumir_test_math(SEXP which, SEXP x) {
  const char *w = CHAR(STRING_ELT(which, 0));
  static const struct {
    const char *name;
    double (*f)(double);
  } fns[] = {{"exp", zumir_r_exp},     {"expm1", zumir_r_expm1}, {"log1p", zumir_r_log1p},
             {"sin", zumir_r_sin},     {"cos", zumir_r_cos},     {"tan", zumir_r_tan},
             {"sqrt", zumir_r_sqrt},   {"log", zumir_r_log},     {"floor", zumir_r_floor},
             {"ceiling", zumir_r_ceil}, {"trunc", zumir_r_trunc}, {"abs", zumir_r_abs}};
  double (*f)(double) = NULL;
  for (size_t i = 0; i < sizeof(fns) / sizeof(fns[0]); i++)
    if (strcmp(fns[i].name, w) == 0) f = fns[i].f;
  if (f == NULL) Rf_error("unknown function");
  R_xlen_t n = XLENGTH(x);
  SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
  zumir_r_nan_warned = 0;
  for (R_xlen_t i = 0; i < n; i++) REAL(out)[i] = f(REAL(x)[i]);
  const char *names[] = {"value", "warned", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, out);
  SET_VECTOR_ELT(res, 1, Rf_ScalarLogical(zumir_r_nan_warned));
  zumir_r_nan_warned = 0;
  UNPROTECT(2);
  return res;
}

/* Test entry: the C ports called directly, for comparison with R
 * (tests/testthat/test-imports.R). `which`: "fmod", "intdiv", "min2",
 * "max2". Vectorised over x and y of one length. */
SEXP zumir_test_port(SEXP which, SEXP x, SEXP y) {
  const char *w = CHAR(STRING_ELT(which, 0));
  double (*f)(double, double) = strcmp(w, "fmod") == 0     ? zumir_r_fmod
                                : strcmp(w, "intdiv") == 0 ? zumir_r_intdiv
                                : strcmp(w, "min2") == 0   ? zumir_r_min2
                                : strcmp(w, "max2") == 0   ? zumir_r_max2
                                                           : NULL;
  if (f == NULL) Rf_error("unknown port");
  R_xlen_t n = XLENGTH(x);
  if (XLENGTH(y) != n) Rf_error("x and y differ in length");
  SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
  for (R_xlen_t i = 0; i < n; i++) REAL(out)[i] = f(REAL(x)[i], REAL(y)[i]);
  UNPROTECT(1);
  return out;
}

/* The names in a list, for tests: kind 0 D6, 2 mir_compile(). */
SEXP zumir_test_import_names(SEXP kind) {
  size_t n;
  const zumir_import_entry *list = zumir_import_list(Rf_asInteger(kind), &n);
  SEXP out = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t) n));
  for (size_t i = 0; i < n; i++) SET_STRING_ELT(out, (R_xlen_t) i, Rf_mkChar(list[i].name));
  UNPROTECT(1);
  return out;
}
