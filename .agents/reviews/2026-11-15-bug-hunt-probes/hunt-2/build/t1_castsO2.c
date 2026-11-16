#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

void f(int n, const double *x, double *y) {
  int k = 0;
  double v[] = {0.0, -0.0, 0.9, -0.9, 1.5, -1.5, 2147483647.0, -2147483648.0, 2147483647.9, -2147483648.9, 255.9, -128.9, 65535.5};
  for (int i = 0; i < 13; i++) { y[k++] = (int)v[i]; }
  for (int i = 0; i < 13; i++) { y[k++] = (long long)v[i]; }
  double w[] = {0.0, 0.9, 1.5, 4294967295.0, 4294967295.9, 2147483648.0, 9223372036854775808.0, 18446744073709549568.0, 9007199254740993.0, 1e19};
  for (int i = 0; i < 6; i++) { y[k++] = (unsigned)w[i]; }
  for (int i = 0; i < 10; i++) { y[k++] = (double)(unsigned long long)w[i]; }
  y[k++] = (float)0.1; y[k++] = (float)1e39; y[k++] = (float)-1e39; y[k++] = (float)1e-46; y[k++] = (float)16777217.0; y[k++] = (double)(float)x[24];
  long long L = 9007199254740993LL; y[k++] = (double)L; y[k++] = (float)L; unsigned long long U = 18446744073709551615ull; y[k++] = (double)U; y[k++] = (float)U;
  U = 9223372036854775809ull; y[k++] = (double)U; y[k++] = (float)U; U = 9223372036854776833ull; y[k++] = (double)U; U = 9223372036854776832ull; y[k++] = (double)U;
  U = 9223372036854777857ull; y[k++] = (double)U; U= 18446744073709550592ull; y[k++] = (double)U; U = 18446744073709550591ull; y[k++] = (double)U;
  y[k++] = (int)(char)300; y[k++] = (int)(unsigned char)300; y[k++] = (int)(short)70000; y[k++] = (int)(long long)0x1ffffffffLL; y[k++] = (unsigned)(0x1ffffffffLL);
  y[k++] = (_Bool)0.5; y[k++] = (_Bool)256; y[k++] = (_Bool)x[15]; y[k++] = (_Bool)x[1]; y[k++] = (bool)0x100000000LL; y[k++] = (bool)(char)256;
  unsigned u = 3000000000u; y[k++] = (double)u; y[k++] = (float)u; int i = -5; y[k++] = (double)(unsigned)i; y[k++] = (float)i; y[k++] = (double)(unsigned char)i; y[k++] = (long long)(unsigned)i; y[k++] = (long long)i; y[k++] = (unsigned long long)i > 0;
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
