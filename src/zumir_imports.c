/* The import resolver (design D6, D14, D18). What a context's code may call
 * by name is a fixed set, loaded with MIR_load_external just before
 * linking; any other name is "import of undefined item", a link error:
 *
 *   ZUMIR_IMPORTS_D6         the C math library, memcpy, memmove, memset,
 *                            memcmp, strlen (cfunction(), mir_function());
 *   ZUMIR_IMPORTS_CONSUMER   D6's list, plus what the consumer registered
 *                            in that context with zumir_import() (D14);
 *   ZUMIR_IMPORTS_RFUNCTION  rfunction()'s internal list: part of the math
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
 * modulus"; rfunction()'s wrapper reads and clears it (Stage 6). */
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

/* The interrupt poll (D23): 1 when R has an interrupt or a time limit
 * pending, 0 otherwise. R_CheckUserInterrupt runs inside R_ToplevelExec,
 * so nothing jumps through generated code. */
static void check_interrupt(void *data) {
  (void) data;
  R_CheckUserInterrupt();
}

int zumir_r_poll(void) {
  return R_ToplevelExec(check_interrupt, NULL) ? 0 : 1;
}

/* ---- The lists ---- */

#define MATH1(f) {#f, (void *) (double (*)(double)) f}
#define MATH2(f) {#f, (void *) (double (*)(double, double)) f}

static const zumir_import_entry d6_list[] = {
  MATH1(acos), MATH1(asin), MATH1(atan), MATH2(atan2), MATH1(cos), MATH1(sin), MATH1(tan),
  MATH1(acosh), MATH1(asinh), MATH1(atanh), MATH1(cosh), MATH1(sinh), MATH1(tanh),
  MATH1(exp), MATH1(exp2), MATH1(expm1), MATH1(log), MATH1(log10), MATH1(log1p),
  MATH1(log2), MATH1(logb), MATH1(cbrt), MATH1(fabs), MATH2(hypot), MATH2(pow),
  MATH1(sqrt), MATH1(erf), MATH1(erfc), MATH1(lgamma), MATH1(tgamma), MATH1(ceil),
  MATH1(floor), MATH1(nearbyint), MATH1(rint), MATH1(round), MATH1(trunc), MATH2(fmod),
  MATH2(remainder), MATH2(copysign), MATH2(nextafter), MATH2(fdim), MATH2(fmax),
  MATH2(fmin),
  {"fma", (void *) (double (*)(double, double, double)) fma},
  {"ldexp", (void *) (double (*)(double, int)) ldexp},
  {"frexp", (void *) (double (*)(double, int *)) frexp},
  {"modf", (void *) (double (*)(double, double *)) modf},
  {"memcpy", (void *) memcpy},
  {"memmove", (void *) memmove},
  {"memset", (void *) memset},
  {"memcmp", (void *) memcmp},
  {"strlen", (void *) strlen},
};

static const zumir_import_entry rfunction_list[] = {
  MATH1(exp), MATH1(log), MATH1(log1p), MATH1(expm1), MATH1(sqrt), MATH1(fabs),
  MATH1(floor), MATH1(ceil), MATH1(trunc), MATH1(sin), MATH1(cos), MATH1(tan),
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
  *n = LEN(d6_list);
  return d6_list;
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

/* The names in a list, for tests: kind 0 D6, 2 rfunction. */
SEXP zumir_test_import_names(SEXP kind) {
  size_t n;
  const zumir_import_entry *list = zumir_import_list(Rf_asInteger(kind), &n);
  SEXP out = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t) n));
  for (size_t i = 0; i < n; i++) SET_STRING_ELT(out, (R_xlen_t) i, Rf_mkChar(list[i].name));
  UNPROTECT(1);
  return out;
}
