#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

static int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
static double ack(double m, double n) { return m == 0 ? n + 1 : n == 0 ? ack(m - 1, 1) : ack(m - 1, ack(m, n - 1)); }
static int sw(int v) { int r = 0; switch (v) { case 1: r += 1; case 2: r += 10; break; case -3: r = -3; break; case 100000: r = 5; break; default: r = 99; case 7: r += 1000; } return r; }
static int sw2(long long v) { switch (v) { case 0x100000001LL: return 1; case 1: return 2; case -1: return 3; default: return 4; } }
static int sw3(unsigned char c) { switch (c) { case 255: return 1; case 0: return 2; case 128: return 3; } return 0; }
static int sw4(int v) { switch (v) { case 0 ... 9: return 1; case 10 ... 20: return 2; } return 3; }
static int counter(void) { static int c = 0; return ++c; }
static int duff(int count) { int n = (count + 7) / 8, r = 0; switch (count % 8) { case 0: do { r++; case 7: r++; case 6: r++; case 5: r++; case 4: r++; case 3: r++; case 2: r++; case 1: r++; } while (--n > 0); } return r; }
void f(int n, const double *x, double *y) {
  int k = 0; y[k++] = fib(20); y[k++] = ack(2, 3);
  int vs[] = {1, 2, -3, 100000, 7, 0, 3}; for (int i = 0; i < 7; i++) y[k++] = sw(vs[i]);
  y[k++] = sw2(0x100000001LL); y[k++] = sw2(1); y[k++] = sw2(-1); y[k++] = sw2(0x200000001LL); y[k++] = sw3(255); y[k++] = sw3(128); y[k++] = sw3(0); y[k++] = sw3(1);
  y[k++] = sw4(5); y[k++] = sw4(10); y[k++] = sw4(20); y[k++] = sw4(21); y[k++] = sw4(-1);
  y[k++] = counter(); y[k++] = counter(); y[k++] = duff(1); y[k++] = duff(8); y[k++] = duff(19);
  int i = 0, s = 0; again: s += i; if (++i < 10) goto again; y[k++] = s;
  for (i = 0; i < 10; i++) { if (i == 3) continue; if (i == 7) break; s += i * 100; } y[k++] = s; y[k++] = i;
  i = 5; do { s--; } while (i-- > 0); y[k++] = s; y[k++] = i; i = 0; while (i++ < 5) ; y[k++] = i;
  int a = 0, b = 0; (a = 1) || (b = 1); y[k++] = a * 10 + b; a = b = 0; (a = 0) && (b = 1); y[k++] = a * 10 + b; a = 0; b = (a++, a++, a + 5); y[k++] = a * 10 + b;
  a = 3; b = a++ + ++a; y[k++] = a; int c = a > 3 ? a < 10 ? 1 : 2 : 3; y[k++] = c; y[k++] = (a, b, c); y[k++] = 1 ? 2 : 3.5; y[k++] = sizeof(1 ? 2 : 3.5); y[k++] = sizeof(1 ? (char)1 : (short)2);
  a = 5; a += a -= a *= 2; a = 7; a <<= 2; a |= 3; a ^= 0xff; a %= 13; a >>= 1; y[k++] = a; a = -17; a /= 4; y[k++] = a; a = -17; a %= 4; y[k++] = a; unsigned char q = 250; q += 10; y[k++] = q; q -= 20; y[k++] = q; q *= 30; y[k++] = q; q = 1; q <<= 9; y[k++] = q; q = 200; q++; q = q + 100; y[k++]=q; signed char sc = 127; sc++; y[k++] = sc; short sh = 32767; sh += 1; y[k++] = sh; sh = 100; sh *= 1000; y[k++] = sh; int ii = 7; ii *= 1.5; y[k++] = ii; ii = 7; ii /= 0.3; y[k++] = ii; float ff = 1; ff += 0.1; y[k++] = ff; char cc = 10; cc *= 2.6; y[k++] = cc; unsigned uu = 10; uu -= 11; y[k++] = uu; uu = 1; uu -= 2.5; y[k++] = 0; long long big = 1; big <<= 40; big += 0.5; y[k++] = (double)big; ii = 1000000; ii *= 3.5f; y[k++] = ii;
  for (int j = 0, m = 10; j < m; j++, m--) y[k] += j * m; k++;
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
