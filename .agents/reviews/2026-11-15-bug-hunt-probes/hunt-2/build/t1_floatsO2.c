#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

void f(int n, const double *x, double *y) {
  int k = 0; float a = 0.1f, b = 0.2f; y[k++] = a + b; y[k++] = a * b; y[k++] = a / b; y[k++] = a - b; y[k++] = (a + b == 0.3f); y[k++] = (a + b == 0.3);
  y[k++] = 1.0f / 3; y[k++] = 1 / 3.0f; y[k++] = 16777216.0f + 1; y[k++] = 16777216.0f + 1.0f; y[k++] = sizeof(a + 1); y[k++] = sizeof(a + 1.0); y[k++] = sizeof(1.0f); y[k++] = sizeof(1.0L);
  float big = 3.4e38f; y[k++] = big * 10; y[k++] = big * 10.0; y[k++] = big + big; y[k++] = -big - big; float tiny = 1e-45f; y[k++] = tiny; y[k++] = tiny / 2; y[k++] = tiny * 0.5;
  for (int i = 0; i < 16 && i < n; i++) { float t = (float)x[i]; y[k++] = t; y[k++] = t * t; }
  y[k++] = FLT_MAX; y[k++] = FLT_MIN; y[k++] = FLT_EPSILON; y[k++] = DBL_MAX; y[k++] = DBL_MIN; y[k++] = DBL_EPSILON; y[k++] = DBL_DIG; y[k++] = DBL_MANT_DIG; y[k++] = FLT_DIG; y[k++] = DBL_MAX_EXP; y[k++] = DBL_MIN_EXP; y[k++] = FLT_RADIX;
  y[k++] = HUGE_VAL; y[k++] = INFINITY; y[k++] = -INFINITY; y[k++] = NAN; y[k++] = M_PI; y[k++] = M_E; y[k++] = INT_MAX; y[k++] = 0x1p-1074; y[k++] = 0x1.8p1; y[k++] = 1e400 > 1; y[k++] = 4.9406564584124654e-324; y[k++] = 2.4703282292062328e-324; y[k++] = 1.7976931348623158e308;y[k++] = 1.7976931348623159e308; y[k++]=.1e1; y[k++] = 1.e-1; y[k++] = 0x.8p0; y[k++] = 1e-400; y[k++] = 0.1 + 0.2; y[k++] = 1.0 - 0.9; y[k++] = 9007199254740993.0; y[k++] = 0.30000000000000004; y[k++] = 123456789012345678901234567890.0; y[k++]=5e-324; y[k++] = 2.2250738585072011e-308; y[k++] = 0.1f; y[k++] = 1e23; y[k++]=8.41e21; y[k++] = 9.5e-5; y[k++] = 1e22+1;
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
