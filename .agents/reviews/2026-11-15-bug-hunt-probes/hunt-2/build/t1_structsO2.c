#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

typedef struct { char c; double d; short s; int i; } S;
typedef struct { unsigned a : 3; int b : 5; unsigned c : 12; unsigned long long d : 40; int e : 1; } BF;
typedef union { double d; unsigned long long u; unsigned char b[8]; float fl; } U;
typedef struct { int n; double v[4]; } V;
static V mk(int n) { V r; r.n = n; for (int i = 0; i < 4; i++) r.v[i] = n * (i + 1) + 0.5; return r; }
static double sumv(V v) { double s = v.n; for (int i = 0; i < 4; i++) s += v.v[i]; v.v[0] = 999; return s; }
static S mks(double d) { S s = { .d = d, .i = 4, .c = 7 }; return s; }
typedef struct { float a, b; } F2; static F2 mkf(float a) { F2 r = {a, a * 2}; return r; } static float sf(F2 f) { return f.a + f.b; }
typedef struct { double a; int b; } DI; static DI mkdi(double a) { DI r = {a, 3}; return r; } static double sdi(DI f, DI g) { return f.a * f.b + g.a; }
typedef struct { char a[3]; } C3; static C3 mkc(void) { C3 r = {{1, 2, 3}}; return r; } static int sc3(C3 a, C3 b) { return a.a[0] + a.a[2] * 10 + b.a[1] * 100; }
typedef struct { long long a, b, c; } L3; static L3 mkl(long long a) { L3 r = {a, a + 1, a + 2}; return r; } static double sl3(int z, L3 a, double d, L3 b) { return z + a.a + a.c * 2 + d + b.b; }
typedef struct { double a, b; } D2; static D2 mkd(double a) { D2 r = {a, -a}; return r; } static double sd2(D2 a, int i, D2 b, D2 c, D2 d, D2 e, D2 f2) { return a.a + i + b.b * 2 + c.a * 3 + d.b * 4 + e.a * 5 + f2.b * 6; }
typedef struct { int a; float b; } IF; static IF mkif(int a) { IF r = {a, a + 0.5f}; return r; } static double sif(IF a, IF b, IF c, IF d, IF e, IF g, IF h, IF i) { return a.a + b.b + c.a + d.b + e.a + g.b + h.a + i.b; }
void f(int n, const double *x, double *y) {
  int k = 0; y[k++] = sizeof(S); y[k++] = offsetof(S, d); y[k++] = offsetof(S, s); y[k++] = offsetof(S, i); y[k++] = sizeof(BF); y[k++] = sizeof(U); y[k++] = sizeof(V); y[k++] = _Alignof(S);
  BF b = {0}; b.a = 9; b.b = -3; b.c = 5000; b.d = 0xfffffffffffull; b.e = 1; y[k++] = b.a; y[k++] = b.b; y[k++] = b.c; y[k++] = (double)b.d; y[k++] = b.e; b.b = 17; y[k++] = b.b; b.a++; y[k++] = b.a; y[k++] = b.a - 5 < 0; y[k++] = sizeof(b.a + 0);
  b.b = 15; b.b++; y[k++] = b.b; b.a = 7; b.a += 3; y[k++] = b.a; y[k++] = (b.a = 13); y[k++] = (b.b = 200); y[k++] = b.d + 1 > b.d; y[k++] = (double)(b.d << 30);
  U u; u.d = 1.0; y[k++] = (double)(u.u >> 52); y[k++] = u.b[7]; y[k++] = u.b[6]; u.u = 0x7ff0000000000000ull; y[k++] = u.d; u.fl = 1.5f; y[k++] = (double)(u.u & 0xffffffffu);
  V v = mk(3); y[k++] = sumv(v); y[k++] = v.v[0]; V w = v; w.v[1] = -1; y[k++] = v.v[1]; y[k++] = w.v[1]; v = mk(7); y[k++] = v.v[3]; y[k++] = mk(9).v[2]; y[k++] = sumv(mk(2));
  S s = mks(2.5); y[k++] = s.d; y[k++] = s.i; y[k++] = s.c; y[k++] = s.s; S t[3] = {{1, 2, 3, 4}, [2] = {.i = 9}}; y[k++] = t[0].d + t[1].d + t[2].i; y[k++] = sizeof t / sizeof t[0];
  int arr[5] = {1, 2}; y[k++] = arr[1] + arr[4]; int m[2][3] = {{1, 2, 3}, {4, 5, 6}}; y[k++] = m[1][2] + *(*(m + 1) + 1) + (*m)[2]; y[k++] = sizeof m; y[k++] = &m[1][0] - &m[0][0]; int *p = arr + 4; y[k++] = p - arr; y[k++] = p[-3]; y[k++] = *--p; y[k++] = (char*)p - (char*)arr;
  y[k++] = ((V){.n = 4, .v = {1, 2}}).v[1]; y[k++] = (int[]){5, 6, 7}[2]; y[k++] = sumv((V){1, {1, 1, 1, 1}}); double *dp = (double[]){1.5, 2.5}; y[k++] = dp[0] + dp[1];
  y[k++] = sf(mkf(1.25f)); y[k++] = sdi(mkdi(1.5), mkdi(2.5)); y[k++] = sc3(mkc(), mkc()); y[k++] = sl3(1, mkl(10), 0.5, mkl(100)); y[k++] = sd2(mkd(1), 2, mkd(3), mkd(4), mkd(5), mkd(6), mkd(7)); y[k++] = sif(mkif(1), mkif(2), mkif(3), mkif(4), mkif(5), mkif(6), mkif(7), mkif(8));
  char str[] = "hello"; y[k++] = sizeof str; y[k++] = strlen(str); y[k++] = str[1]; y[k++] = "abc"[1]; y[k++] = sizeof("ab" "cd"); const char *cs = "x\0y"; y[k++] = strlen(cs); y[k++] = sizeof("x\0y"); 
  int vla_n = 3 + (n > 0); double vla[vla_n]; for (int i = 0; i < vla_n; i++) vla[i] = i * 1.5; y[k++] = vla[3]; y[k++] = sizeof vla;
  enum E { A, B = 5, C, D = -1, Z = 0x7fffffff }; y[k++] = C; y[k++] = D; y[k++] = sizeof(enum E); y[k++] = Z; enum E e = D; y[k++] = e < 0;
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
