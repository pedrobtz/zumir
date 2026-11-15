#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

static unsigned long long conv(double d) { return d; }
void f(int n, const double *x, double *y) {
  int k = 0; double v = 18446744073709549568.0, w = 1e19, z = 9223372036854775808.0, q = 9223372036854777856.0, r = 4294967296.5, s = 9223372036854774784.0;
  y[k++] = (double)(unsigned long long)v; y[k++] = (double)(unsigned long long)w; y[k++] = (double)(unsigned long long)z; y[k++] = (double)(unsigned long long)q; y[k++] = (double)(unsigned long long)r; y[k++] = (double)(unsigned long long)s;
  unsigned long long u = v; y[k++] = (double)u; y[k++] = (double)(u >> 32); u = w; y[k++] = (double)(u % 1000); y[k++] = (double)conv(q); y[k++] = (double)(uint64_t)x[23]; y[k++] = (double)(size_t)x[23]; y[k++] = (double)(unsigned long)x[23];
  float fv = 1.8446743e19f, fw = 9.223373e18f; y[k++] = (double)(unsigned long long)fv; y[k++] = (double)(unsigned long long)fw; y[k++] = (double)(unsigned long long)3e9f;
  y[k++] = (double)(unsigned long long)18446744073709549568.0; y[k++] = (double)(unsigned long long)1e19; y[k++] = (unsigned)3e9; double t = 3e9; y[k++] = (unsigned)t; y[k++] = (unsigned)x[21]; y[k++] = (unsigned short)x[28]; y[k++] = (unsigned char)x[26]; float tf = 3e9f; y[k++] = (unsigned)tf; y[k++] = (double)(long long)x[23 - 1]; y[k++]=(double)(unsigned long long)x[22];
  long double ld = v; y[k++] = (double)(unsigned long long)ld; y[k++] = (double)ld; y[k++] = sizeof(ld); ld = 1e19L; y[k++] = (double)(unsigned long long)ld; ld = 0.1L; y[k++] = (double)ld; y[k++] = (double)(ld * 3); y[k++] = (double)(1.0L / 3); y[k++] = (double)(ld + 0.2L); y[k++] = 0.1L == 0.1; y[k++] = (double)(1e308L * 10); y[k++] = (double)(1e308L * 10 / 10); y[k++] = (double)0x1p-1074L / 2 * 2; y[k++] = (double)(0x1p-1074L / 2 * 2); long double big = 18446744073709551615ull; y[k++] = (double)((unsigned long long)(big) - 18446744073709551614ull); y[k++] = (double)(long long)(9007199254740993.0L + 0);
  long long sl = 9007199254740993LL; long double l2 = sl; y[k++] = (double)((long long)l2 - sl); 
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
