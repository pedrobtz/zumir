#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
void f(int n, const double *x, double *y) { int flags = 0x300; bool has = flags & 0x100; y[0] = has; y[1] = has ? 10 : 20; }
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
