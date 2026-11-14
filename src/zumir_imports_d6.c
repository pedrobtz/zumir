/* D6's list (design D6): the C math library and five string functions,
 * the names cfunction()'s code may call. Apart from zumir_imports.c, which
 * needs R, so tests/c's c-tests runner links the same list. The prelude
 * (zumir_prelude.c) declares exactly these names; test-prelude.R holds the
 * two to each other. */
#include <math.h>
#include <string.h>

#include "zumir_imports.h"

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

const zumir_import_entry *zumir_import_d6(size_t *n) {
  *n = sizeof(d6_list) / sizeof(d6_list[0]);
  return d6_list;
}
