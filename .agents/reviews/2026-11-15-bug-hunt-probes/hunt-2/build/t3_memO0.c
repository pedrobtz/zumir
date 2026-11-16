#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

void f(int n, const double *x, double *y) { int k = 0;
  double a[10]; for (int i = 0; i < 10; i++) a[i] = i + 0.5;
  memmove(a + 2, a, 6 * sizeof(double)); for (int i = 0; i < 10; i++) y[k++] = a[i];
  memmove(a, a + 3, 6 * sizeof(double)); for (int i = 0; i < 10; i++) y[k++] = a[i];
  char b[40]; memset(b, 0x41, 39); b[39] = 0; y[k++] = strlen(b); b[17] = 0; y[k++] = strlen(b); y[k++] = strlen(""); y[k++] = strlen(b + 18);
  y[k++] = memcmp("abc", "abd", 3) < 0; y[k++] = memcmp("abc", "abd", 2); y[k++] = memcmp("\xff", "\x01", 1) > 0; y[k++] = memcmp(a, a, 0);
  void *r = memcpy(y + 100, x, 8 * sizeof(double)); y[k++] = (r == y + 100); r = memset(b, 300, 3); y[k++] = (unsigned char)b[0]; y[k++] = (r == b); y[k++] = (memmove(b + 1, b, 5) == b + 1);
  unsigned char raw[8]; double one = 1.0; memcpy(raw, &one, 8); y[k++] = raw[7]; y[k++] = raw[6]; unsigned long long u; memcpy(&u, &x[1], 8); y[k++] = (double)(u >> 63); float fl = 1.5f; unsigned ui; memcpy(&ui, &fl, 4); y[k++] = ui;
  int big[1000]; memset(big, 0xff, sizeof big); y[k++] = big[999]; memset(big, 0, sizeof big); long s = 0; for (int i = 0; i < 1000; i++) { big[i] = i * i; } for (int i = 0; i < 1000; i++) s += big[i]; y[k++] = s;
  size_t sz = strlen("hello") - 6; y[k++] = sz > 0; y[k++] = (double)sz; y[k++] = sizeof(strlen("")) ; y[k++] = sizeof(size_t);
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
