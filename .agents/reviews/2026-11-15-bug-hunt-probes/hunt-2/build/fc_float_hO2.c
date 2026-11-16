#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
void f(int n, const double *x, double *y) { y[0] = DBL_DIG; y[1] = FLT_DIG; y[2] = LDBL_MANT_DIG; y[3] = sizeof(long double); y[4] = LDBL_EPSILON > 1e-17; y[5] = DECIMAL_DIG; }
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
