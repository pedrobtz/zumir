#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#define STR(x) #x
#define XSTR(x) STR(x)
#define CAT(a, b) a##b
#define VA(...) cnt(__VA_ARGS__, 0)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SQ(x) ((x) * (x))
#define EMPTY
#define F(x) G(x)
#define G(x) (x + 1)
#define OBJ F
#define TEN 10
#define REC REC + 1
static int cnt(int a, ...) { return a; }
void f(int n, const double *x, double *y) { int k = 0;
  y[k++] = sizeof(STR(a  b   c)); y[k++] = sizeof(XSTR(TEN)); y[k++] = sizeof(STR(TEN)); y[k++] = CAT(1, 2); y[k++] = CAT(TE, N); y[k++] = VA(7, 8, 9); y[k++] = MAX(3, 4.5); int i = 3; y[k++] = SQ(i + 1); y[k++] = OBJ(5) EMPTY;
  y[k++] = sizeof(STR("q\n")); y[k++] = sizeof(STR('"')); y[k++] = __LINE__; y[k++] = sizeof(__func__); y[k++] = __STDC__; y[k++] = __STDC_VERSION__ >= 199901L; y[k++] = sizeof(__DATE__);
#if -1 < 0u
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if 0x8000000000000000 > 0
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if (2147483647 + 1) > 0
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if -1 >> 1 == -1
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if (-7 / 2 == -3) && (-7 % 2 == -1) && (7 / -2 == -3)
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if defined(TEN) && !defined NOPE && TEN == 10 && UNDEFINED_THING == 0
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if 'a' == 97 && '\377' < 0
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if 18446744073709551615u == -1
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if (1 ? -1 : 0u) < 0
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if 1 << 40 > 0 && ~0u == 18446744073709551615u && (0 - 1u) > 0
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#ifdef __x86_64__
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#if defined(__LP64__) || defined(_LP64)
  y[k++] = 1;
#else
  y[k++] = 2;
#endif
#line 100
  y[k++] = __LINE__;
  _Static_assert(sizeof(int) == 4, "int");
  y[k++] = _Generic(1.0f, float: 1, double: 2, default: 3); y[k++] = _Generic(1, int: 1, long: 2, default: 3); y[k++] = _Generic('a', char: 1, int: 2, default: 3); y[k++] = _Generic(1u + 1ll, long long: 1, unsigned long long: 2, default: 3); y[k++] = _Generic((char)1 + (char)1, char: 1, int: 2, default: 3); y[k++] = _Generic(1 ? 1 : 2.0, int: 1, double: 2); y[k++] = _Generic(sizeof(int), unsigned long: 1, unsigned long long: 2, default: 3); y[k++] = _Generic(1.0f + 1, float: 1, double: 2); y[k++] = _Generic((short)1 << 1, short: 1, int: 2); y[k++] = _Generic(-(unsigned char)1, unsigned char: 1, int: 2, unsigned: 3); y[k++] = _Generic(1 == 1, int: 1, _Bool: 2); y[k++] = _Generic(0x7fffffff + 0u, unsigned: 1, default: 2); y[k++] = _Generic(2147483648, long: 1, long long: 2, unsigned: 3); y[k++] = _Generic(0xffffffff, unsigned: 1, long: 2, int: 3); y[k++] = _Generic((int *)0 - (int *)0, long: 1, long long: 2, int: 3); y[k++] = _Generic(L'a', int: 1, unsigned short: 2, default: 3); y[k++] = _Generic(1.0L, long double: 1, double: 2); y[k++] = _Generic(1.f * 1.0, float: 1, double: 2); y[k++] = _Generic(!1.5, int: 1, double: 2); y[k++] = _Generic(~(unsigned short)1, int: 1, unsigned short: 2, unsigned: 3);
  struct bf { unsigned a : 3; long long b : 40; unsigned long long c : 31; int d : 31; unsigned e : 32; } q = {7, 1, 1, 1, 1}; y[k++] = _Generic(q.a + 0, int: 1, unsigned: 2); y[k++] = (q.a - 8 < 0); y[k++] = (q.c - 2 < 0); y[k++] = (q.e - 2 < 0); y[k++] = _Generic(q.c + 0, int: 1, unsigned long long: 2, long long: 3, unsigned: 4); y[k++] = _Generic(q.b + 0, long long: 1, int: 2, long: 3);
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
