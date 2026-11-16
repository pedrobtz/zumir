#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

void f(int n, const double *x, double *y) {
  int k = 0; int a[] = {7, -7, 2147483647, -2147483647, 1, -1, 100}; int b[] = {2, -2, 3, -3, 2147483647, -2147483647, 7};
  for (int i = 0; i < 7; i++) for (int j = 0; j < 7; j++) { if (k < 90) { y[k++] = a[i] / b[j] * 1000.0 + a[i] % b[j] % 1000; } }
  long long A = -9223372036854775807LL; y[k++] = (double)(A / 10); y[k++] = (double)(A % 10); unsigned long long B = 18446744073709551615ull; y[k++] = (double)(B / 10); y[k++] = (double)(B % 10);
  short s = -7; unsigned char u = 2; y[k++] = s / u; y[k++] = s % u;
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
