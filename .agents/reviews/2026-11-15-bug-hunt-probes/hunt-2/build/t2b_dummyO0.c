#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
void f(int n, const double *x, double *y) { y[0] = 1; }
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
