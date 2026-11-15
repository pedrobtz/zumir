#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
void f(int n, const double *x, double *y) { int k = 0; for (int i = 0; i < 12; i++) for (int j = 0; j < 12; j++) for (int l = 0; l < 12; l++) y[k++] = fma(x[i], x[j], x[l]);
  for (int i = 0; i < 40; i++) { int e = 77; double ip = 77; y[k++] = frexp(x[i], &e); y[k++] = e; y[k++] = modf(x[i], &ip); y[k++] = ip; for (int j = -1080; j <= 1080; j += 540) y[k++] = ldexp(x[i], j); y[k++] = ldexp(x[i], INT_MAX); y[k++] = ldexp(x[i], INT_MIN); } }
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
