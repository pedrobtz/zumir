source("harness.R")
T <- list(
pp = '
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
  y[k++] = sizeof(STR("q\\n")); y[k++] = sizeof(STR(\'"\')); y[k++] = __LINE__; y[k++] = sizeof(__func__); y[k++] = __STDC__; y[k++] = __STDC_VERSION__ >= 199901L; y[k++] = sizeof(__DATE__);
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
#if \'a\' == 97 && \'\\377\' < 0
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
  y[k++] = _Generic(1.0f, float: 1, double: 2, default: 3); y[k++] = _Generic(1, int: 1, long: 2, default: 3); y[k++] = _Generic(\'a\', char: 1, int: 2, default: 3); y[k++] = _Generic(1u + 1ll, long long: 1, unsigned long long: 2, default: 3); y[k++] = _Generic((char)1 + (char)1, char: 1, int: 2, default: 3); y[k++] = _Generic(1 ? 1 : 2.0, int: 1, double: 2); y[k++] = _Generic(sizeof(int), unsigned long: 1, unsigned long long: 2, default: 3); y[k++] = _Generic(1.0f + 1, float: 1, double: 2); y[k++] = _Generic((short)1 << 1, short: 1, int: 2); y[k++] = _Generic(-(unsigned char)1, unsigned char: 1, int: 2, unsigned: 3); y[k++] = _Generic(1 == 1, int: 1, _Bool: 2); y[k++] = _Generic(0x7fffffff + 0u, unsigned: 1, default: 2); y[k++] = _Generic(2147483648, long: 1, long long: 2, unsigned: 3); y[k++] = _Generic(0xffffffff, unsigned: 1, long: 2, int: 3); y[k++] = _Generic((int *)0 - (int *)0, long: 1, long long: 2, int: 3); y[k++] = _Generic(L\'a\', int: 1, unsigned short: 2, default: 3); y[k++] = _Generic(1.0L, long double: 1, double: 2); y[k++] = _Generic(1.f * 1.0, float: 1, double: 2); y[k++] = _Generic(!1.5, int: 1, double: 2); y[k++] = _Generic(~(unsigned short)1, int: 1, unsigned short: 2, unsigned: 3);
  struct bf { unsigned a : 3; long long b : 40; unsigned long long c : 31; int d : 31; unsigned e : 32; } q = {7, 1, 1, 1, 1}; y[k++] = _Generic(q.a + 0, int: 1, unsigned: 2); y[k++] = (q.a - 8 < 0); y[k++] = (q.c - 2 < 0); y[k++] = (q.e - 2 < 0); y[k++] = _Generic(q.c + 0, int: 1, unsigned long long: 2, long long: 3, unsigned: 4); y[k++] = _Generic(q.b + 0, long long: 1, int: 2, long: 3);
}',
misc = '
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
  char buf[16] = "abc"; char *cp = buf; while (*cp) cp++; y[k++] = cp - buf; *cp++ = \'d\'; *cp = 0; y[k++] = strlen(buf); y[k++] = buf[3]; y[k++] = (buf[0] < buf[1]) + (\'a\' < \'b\') + ("x"[0] == \'x\');
  long double ld = 1.0L; for (int i = 0; i < 70; i++) ld /= 2; y[k++] = (double)(ld * 1e21L); y[k++] = (double)(1.0L + ld) == 1.0; y[k++] = (1.0L + ld) == 1.0L; y[k++] = sizeof(ld + 1.0); y[k++] = (double)(ld + x[2]);
}'
)
run_tests(T, "t4")
