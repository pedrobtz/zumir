#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

static inline int sq(int a) { return a * a; }
static int g1(int a[static 3]) { return a[0] + a[2]; }
static double avg(int cnt, const double *v) { double s = 0; for (int i = 0; i < cnt; i++) s += v[i]; return s / cnt; }
static int fact(int n2) { return n2 <= 1 ? 1 : n2 * fact(n2 - 1); }
static int even(int); static int odd(int v) { return v == 0 ? 0 : even(v - 1); } static int even(int v) { return v == 0 ? 1 : odd(v - 1); }
static void swap(double *a, double *b) { double t = *a; *a = *b; *b = t; }
static int cmp(const void *a, const void *b) { double x = *(const double *)a, y2 = *(const double *)b; return (x > y2) - (x < y2); }
static void isort(double *v, int cnt, int (*c)(const void *, const void *)) { for (int i = 1; i < cnt; i++) for (int j = i; j > 0 && c(&v[j - 1], &v[j]) > 0; j--) swap(&v[j - 1], &v[j]); }
static unsigned hash(const char *s) { unsigned h = 2166136261u; while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; } return h; }
static unsigned long long lcg(unsigned long long *s) { *s = *s * 6364136223846793005ULL + 1442695040888963407ULL; return *s >> 33; }
static double horner(const double *c, int cnt, double x0) { double r = 0; for (int i = cnt - 1; i >= 0; i--) r = r * x0 + c[i]; return r; }
static int popcnt(unsigned long long v) { int c = 0; while (v) { v &= v - 1; c++; } return c; }
static int collatz(long long v) { int c = 0; while (v != 1) { v = v % 2 ? 3 * v + 1 : v / 2; c++; } return c; }
static double kahan(const double *v, int cnt) { double s = 0, c = 0; for (int i = 0; i < cnt; i++) { double yy = v[i] - c; double t = s + yy; c = (t - s) - yy; s = t; } return s; }
void f(int n, const double *x, double *y) { int k = 0;
  y[k++] = sq(7); int a3[3] = {1, 2, 3}; y[k++] = g1(a3); y[k++] = avg(4, x + 2); y[k++] = fact(12); y[k++] = odd(7) + 10 * even(10);
  double v[8] = {5, -1, 3.5, 0, -0.0, 9, 2, 2}; isort(v, 8, cmp); for (int i = 0; i < 8; i++) y[k++] = v[i];
  y[k++] = hash("hello, world"); y[k++] = hash(""); unsigned long long s = 42; for (int i = 0; i < 5; i++) y[k++] = (double)lcg(&s);
  double c[] = {1, -2, 3, 0.5, -0.25}; y[k++] = horner(c, 5, 1.7); y[k++] = horner(c, 5, -1e10); y[k++] = popcnt(0xdeadbeefcafebabeULL); y[k++] = collatz(27); y[k++] = collatz(837799);
  double big[] = {1e16, 1, -1e16, 1, 1e-8, 3}; y[k++] = kahan(big, 6); double plain = 0; for (int i = 0; i < 6; i++) plain += big[i]; y[k++] = plain;
  double acc = 0; for (int i = 1; i <= 1000; i++) acc += 1.0 / i; y[k++] = acc; acc = 1; for (int i = 0; i < 60; i++) acc *= 1.1; y[k++] = acc; float fa = 0; for (int i = 1; i <= 1000; i++) fa += 1.0f / i; y[k++] = fa;
  double t = 0.1; y[k++] = t * 3 == 0.3; y[k++] = t + 0.2 == 0.3; y[k++] = (float)t == 0.1f; y[k++] = t * t * t; y[k++] = t / 3 * 3; y[k++] = 1e308 * 10 / 10; y[k++] = x[10] * x[10]; y[k++] = x[12] / 2 * 2; y[k++] = (x[8] + x[8]) - x[8];
  y[k++] = a3[1] * x[4] + a3[2] * x[6] - x[7] / a3[0]; y[k++] = x[2] * x[3] + x[4] * x[5] + x[6] * x[7] + x[24] * x[25]; y[k++] = (x[24] + x[25]) * (x[26] - x[27]) / (x[28] + 1) - x[29] * x[30] + x[31] / 7;
  int ii = n; long long ll = ii * 1000000LL; y[k++] = (double)ll; ll = (long long)ii * ii * ii * ii * ii * ii; y[k++] = (double)ll; unsigned uu = ii; uu = uu * uu * uu * uu * uu * uu * uu; y[k++] = uu;
  int idx = 0; int arr[5] = {0}; arr[idx++] = idx; arr[idx] = idx++; y[k++] = idx; const double *p = x; p += 3; y[k++] = *p++; y[k++] = *++p; y[k++] = p[-2]; y[k++] = p - x; y[k++] = *(x + n - 1); y[k++] = (p > x) + (p == x + 5) * 2 + (p != 0) * 4 + !p * 8;
  char buf[16] = "abc"; char *cp = buf; while (*cp) cp++; y[k++] = cp - buf; *cp++ = 'd'; *cp = 0; y[k++] = strlen(buf); y[k++] = buf[3]; y[k++] = (buf[0] < buf[1]) + ('a' < 'b') + ("x"[0] == 'x');
  long double ld = 1.0L; for (int i = 0; i < 70; i++) ld /= 2; y[k++] = (double)(ld * 1e21L); y[k++] = (double)(1.0L + ld) == 1.0; y[k++] = (1.0L + ld) == 1.0L; y[k++] = sizeof(ld + 1.0); y[k++] = (double)(ld + x[2]);
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
