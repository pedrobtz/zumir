#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
void f(int n, const double *x, double *y) { _Bool p = x; y[0] = p; y[1] = (_Bool)256; _Bool q = 1; q += 1; y[2] = q; }
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
