#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
void f(int n, const double *x, double *y) { bool nz = x[4]; /* 0.5 */ y[0] = nz; bool b2 = (bool)x[24]; /* 0.1 */ y[1] = b2; }
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
