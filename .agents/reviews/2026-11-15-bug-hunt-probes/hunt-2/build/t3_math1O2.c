#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
void f(int n, const double *x, double *y) { int k = 0; for (int i = 0; i < 40; i++) {
y[k++] = acos(x[i]);
y[k++] = asin(x[i]);
y[k++] = atan(x[i]);
y[k++] = cos(x[i]);
y[k++] = sin(x[i]);
y[k++] = tan(x[i]);
y[k++] = acosh(x[i]);
y[k++] = asinh(x[i]);
y[k++] = atanh(x[i]);
y[k++] = cosh(x[i]);
y[k++] = sinh(x[i]);
y[k++] = tanh(x[i]);
y[k++] = exp(x[i]);
y[k++] = exp2(x[i]);
y[k++] = expm1(x[i]);
y[k++] = log(x[i]);
y[k++] = log10(x[i]);
y[k++] = log1p(x[i]);
y[k++] = log2(x[i]);
y[k++] = logb(x[i]);
y[k++] = cbrt(x[i]);
y[k++] = fabs(x[i]);
y[k++] = sqrt(x[i]);
y[k++] = erf(x[i]);
y[k++] = erfc(x[i]);
y[k++] = lgamma(x[i]);
y[k++] = tgamma(x[i]);
y[k++] = ceil(x[i]);
y[k++] = floor(x[i]);
y[k++] = nearbyint(x[i]);
y[k++] = rint(x[i]);
y[k++] = round(x[i]);
y[k++] = trunc(x[i]);
} }
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
