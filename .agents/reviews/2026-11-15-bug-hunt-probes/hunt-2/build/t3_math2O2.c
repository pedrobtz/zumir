#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
void f(int n, const double *x, double *y) { int k = 0; for (int i = 0; i < 16; i++) for (int j = 0; j < 16; j++) {
y[k++] = atan2(x[i], x[j]);
y[k++] = hypot(x[i], x[j]);
y[k++] = pow(x[i], x[j]);
y[k++] = fmod(x[i], x[j]);
y[k++] = remainder(x[i], x[j]);
y[k++] = copysign(x[i], x[j]);
y[k++] = nextafter(x[i], x[j]);
y[k++] = fdim(x[i], x[j]);
y[k++] = fmax(x[i], x[j]);
y[k++] = fmin(x[i], x[j]);
} }
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
