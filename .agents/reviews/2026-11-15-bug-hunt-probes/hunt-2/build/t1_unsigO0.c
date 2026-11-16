#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

void f(int n, const double *x, double *y) {
  unsigned u = 0; int i = -1; unsigned long long ull = 0; long long ll = -1; int k = 0;
  y[k++] = u - 1; y[k++] = (i < u); y[k++] = (i < 1u); y[k++] = (ll < u); y[k++] = (ll < ull); y[k++] = (double)(ull - 1);
  y[k++] = (unsigned)i; y[k++] = (unsigned)i / 2; y[k++] = i / 2; y[k++] = (unsigned)i % 7; y[k++] = i % 7; y[k++] = -7 / 2; y[k++] = -7 % 2;
  y[k++] = 7 / -2; y[k++] = 7 % -2; y[k++] = (-7) / (-2); y[k++] = (-7) % (-2);
  unsigned a = 4000000000u, b = 500000000u; y[k++] = a + b; y[k++] = a * b; y[k++] = a - b - a; y[k++] = (a > b); y[k++] = ((int)a > (int)b);
  y[k++] = (double)(a * (unsigned long long)b); y[k++] = (unsigned long long)a * b / 7; y[k++] = (long long)a << 3; y[k++] = a >> 31; y[k++] = (int)a >> 31;
  y[k++] = 1u << 31; y[k++] = (1u << 31) - 1; y[k++] = (double)(1ull << 63); y[k++] = (double)(~0ull); y[k++] = (double)(~0ull >> 1);
  y[k++] = (double)(long long)(1ull << 63); y[k++] = -1 > 0u; y[k++] = -1L > 0u; y[k++] = (unsigned char)-1 > 0; y[k++] = (short)-1 > 0u;
  y[k++] = 0x7fffffff + 1u; y[k++] = 2147483648 > 0; y[k++] = sizeof(2147483648); y[k++] = sizeof(0x80000000); y[k++] = 0x80000000 > 0; y[k++] = -0x80000000 > 0;
  y[k++] = sizeof(4294967296); y[k++] = sizeof(0xffffffff); y[k++] = sizeof(1u); y[k++] = sizeof(1ll); y[k++] = -2147483648 < 0; y[k++] = sizeof(-2147483648);
  unsigned long long big = 18446744073709551615ull; y[k++] = (double)big; y[k++] = (double)(big / 3); y[k++] = (double)(big % 1000000007ull); y[k++] = big > 0; y[k++]=(double)(big >> 1);
  long long m = LLONG_MIN; y[k++] = (double)m; y[k++] = (double)(m / 3); y[k++] = (double)(m % 1000000007); y[k++] = (double)(m >> 3); y[k++] = (double)((unsigned long long)m >> 3);
  y[k++] = (double)(m + LLONG_MAX); y[k++] = (double)LLONG_MAX; y[k++] = (double)ULLONG_MAX; y[k++] = (double)LONG_MAX; y[k++] = (double)ULONG_MAX; y[k++]=UINT_MAX; y[k++] = INT_MIN; y[k++]=SHRT_MIN; y[k++]=USHRT_MAX; y[k++]=SCHAR_MIN; y[k++]=UCHAR_MAX; y[k++] = CHAR_BIT;
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
