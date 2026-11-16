#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
void f(int n, const double *x, double *y) { unsigned long long u = 1e19; y[0] = (double)u; uint64_t v = (uint64_t)x[23]; /* 1e19 */ y[1] = (double)v; y[2] = (double)(size_t)18446744073709549568.0; }
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
