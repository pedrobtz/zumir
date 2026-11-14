/* The prelude's text (zumir_prelude.h). Each declaration is one D6 name;
 * a name here that the resolver does not link is a link error, and one the
 * resolver links that is not here is undeclared in C: test-prelude.R
 * compiles a call to every name in both lists. */
#include "zumir_prelude.h"

const char zumir_prelude_math_h[] =
  "#ifndef _ZUMIR_MATH_H\n"
  "#define _ZUMIR_MATH_H 1\n"
  "#define HUGE_VAL (1.0 / 0.0)\n"
  "#define INFINITY (1.0f / 0.0f)\n"
  "#define NAN (0.0f / 0.0f)\n"
  "#define M_PI 3.14159265358979323846\n"
  "#define M_E 2.7182818284590452354\n"
  "double acos (double); double asin (double); double atan (double);\n"
  "double atan2 (double, double); double cos (double); double sin (double);\n"
  "double tan (double); double acosh (double); double asinh (double);\n"
  "double atanh (double); double cosh (double); double sinh (double);\n"
  "double tanh (double); double exp (double); double exp2 (double);\n"
  "double expm1 (double); double log (double); double log10 (double);\n"
  "double log1p (double); double log2 (double); double logb (double);\n"
  "double cbrt (double); double fabs (double); double hypot (double, double);\n"
  "double pow (double, double); double sqrt (double); double erf (double);\n"
  "double erfc (double); double lgamma (double); double tgamma (double);\n"
  "double ceil (double); double floor (double); double nearbyint (double);\n"
  "double rint (double); double round (double); double trunc (double);\n"
  "double fmod (double, double); double remainder (double, double);\n"
  "double copysign (double, double); double nextafter (double, double);\n"
  "double fdim (double, double); double fmax (double, double);\n"
  "double fmin (double, double); double fma (double, double, double);\n"
  "double ldexp (double, int); double frexp (double, int *);\n"
  "double modf (double, double *);\n"
  "#endif\n";

const char zumir_prelude_string_h[] =
  "#ifndef _ZUMIR_STRING_H\n"
  "#define _ZUMIR_STRING_H 1\n"
  "#include <stddef.h>\n"
  "void *memcpy (void *, const void *, size_t);\n"
  "void *memmove (void *, const void *, size_t);\n"
  "void *memset (void *, int, size_t);\n"
  "int memcmp (const void *, const void *, size_t);\n"
  "size_t strlen (const char *);\n"
  "#endif\n";

const char zumir_prelude_prefix[] =
  "#include <stddef.h>\n"
  "#include <stdint.h>\n"
  "#include <stdbool.h>\n"
  "#include <stdarg.h>\n"
  "#include <float.h>\n"
  "#include <limits.h>\n"
  "#include <math.h>\n"
  "#include <string.h>\n"
  "#line 0 \"";

/* After the #line directive and the source's name: one line of no
 * consequence, numbered 0, so the user's source starts at line 1. c2mir
 * does not count the blank or comment-only lines that follow a #line
 * directive directly (they are read ahead with the directive); after
 * this line it counts every line. */
const char zumir_prelude_line0[] = "\"\nextern int __zumir_prelude_end;\n";
