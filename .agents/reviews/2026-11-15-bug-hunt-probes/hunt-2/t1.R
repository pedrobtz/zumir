source("harness.R")
T <- list(
promo = '
void f(int n, const double *x, double *y) {
  unsigned char uc = 200; signed char sc = -56; char c = (char)200; short s = -1; unsigned short us = 65535;
  int k = 0;
  y[k++] = uc + uc; y[k++] = sc * 2; y[k++] = c; y[k++] = (c < 0); y[k++] = s + us; y[k++] = us * us / 3.0 > 0;
  y[k++] = (unsigned char)(uc + 100); y[k++] = (signed char)(sc - 100); y[k++] = ~uc; y[k++] = -uc; y[k++] = !uc;
  y[k++] = (short)(us + 1); y[k++] = (unsigned short)(s); y[k++] = uc << 24; y[k++] = (uc << 24) >> 24; y[k++] = sizeof(c + c);
  y[k++] = sizeof(char); y[k++] = sizeof(long); y[k++] = sizeof(long long); y[k++] = sizeof(long double); y[k++] = sizeof(void*);
  y[k++] = sizeof(int); y[k++] = sizeof(short); y[k++] = sizeof(float); y[k++] = sizeof(size_t); y[k++] = sizeof(\'a\');
  y[k++] = CHAR_MIN; y[k++] = CHAR_MAX; y[k++] = (char)-1 < 0; y[k++] = \'\\xff\'; y[k++] = "\\xff"[0]; y[k++] = L\'a\';
  y[k++] = sizeof(wchar_t); y[k++] = (wchar_t)-1 < 0; y[k++] = sizeof(L\'a\'); y[k++]=sizeof(ptrdiff_t); y[k++] = _Alignof(double); y[k++] = _Alignof(long double);
  y[k++] = _Alignof(max_align_t); y[k++] = sizeof(max_align_t); y[k++] = sizeof(_Bool); y[k++] = \'ab\'; 
}',
unsig = '
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
}',
casts = '
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
}',
shifts = '
void f(int n, const double *x, double *y) {
  int k = 0; int a = -16; unsigned b = 0x80000001u; long long c = -1024; unsigned long long d = 0x8000000000000001ull;
  for (int s = 0; s < 32; s += 7) { y[k++] = a >> s; y[k++] = b >> s; y[k++] = b << s; y[k++] = (double)(c >> s); y[k++] = (double)(d >> s); y[k++] = (double)(d << s); y[k++] = (double)(1LL << (s + 31)); y[k++] = 1u << s;}
  unsigned char uc = 0x81; y[k++] = uc << 1; y[k++] = uc >> 1; y[k++] = (unsigned char)(uc << 1); signed char sc = -127; y[k++] = sc >> 1; y[k++] = (sc & 0xff) >> 1;
  short sh = -32768; y[k++] = sh >> 15; y[k++] = (unsigned short)sh >> 15; y[k++] = 1 << 30; y[k++] = (double)(1ull << 63 >> 63); y[k++] = (double)(-1LL >> 63); y[k++] = a ^ b; y[k++] = a & b; y[k++] = a | b; y[k++] = ~a; y[k++] = (double)~d; y[k++] = (double)(c ^ d);
  long sl = 40; y[k++] = (double)(1ul << sl); y[k++] = 1 << (char)3; y[k++] = sizeof(1 << 3LL); y[k++] = sizeof((char)1 << 3); 
}',
divmod = '
void f(int n, const double *x, double *y) {
  int k = 0; int a[] = {7, -7, 2147483647, -2147483647, 1, -1, 100}; int b[] = {2, -2, 3, -3, 2147483647, -2147483647, 7};
  for (int i = 0; i < 7; i++) for (int j = 0; j < 7; j++) { if (k < 90) { y[k++] = a[i] / b[j] * 1000.0 + a[i] % b[j] % 1000; } }
  long long A = -9223372036854775807LL; y[k++] = (double)(A / 10); y[k++] = (double)(A % 10); unsigned long long B = 18446744073709551615ull; y[k++] = (double)(B / 10); y[k++] = (double)(B % 10);
  short s = -7; unsigned char u = 2; y[k++] = s / u; y[k++] = s % u;
}',
floats = '
void f(int n, const double *x, double *y) {
  int k = 0; float a = 0.1f, b = 0.2f; y[k++] = a + b; y[k++] = a * b; y[k++] = a / b; y[k++] = a - b; y[k++] = (a + b == 0.3f); y[k++] = (a + b == 0.3);
  y[k++] = 1.0f / 3; y[k++] = 1 / 3.0f; y[k++] = 16777216.0f + 1; y[k++] = 16777216.0f + 1.0f; y[k++] = sizeof(a + 1); y[k++] = sizeof(a + 1.0); y[k++] = sizeof(1.0f); y[k++] = sizeof(1.0L);
  float big = 3.4e38f; y[k++] = big * 10; y[k++] = big * 10.0; y[k++] = big + big; y[k++] = -big - big; float tiny = 1e-45f; y[k++] = tiny; y[k++] = tiny / 2; y[k++] = tiny * 0.5;
  for (int i = 0; i < 16 && i < n; i++) { float t = (float)x[i]; y[k++] = t; y[k++] = t * t; }
  y[k++] = FLT_MAX; y[k++] = FLT_MIN; y[k++] = FLT_EPSILON; y[k++] = DBL_MAX; y[k++] = DBL_MIN; y[k++] = DBL_EPSILON; y[k++] = DBL_DIG; y[k++] = DBL_MANT_DIG; y[k++] = FLT_DIG; y[k++] = DBL_MAX_EXP; y[k++] = DBL_MIN_EXP; y[k++] = FLT_RADIX;
  y[k++] = HUGE_VAL; y[k++] = INFINITY; y[k++] = -INFINITY; y[k++] = NAN; y[k++] = M_PI; y[k++] = M_E; y[k++] = INT_MAX; y[k++] = 0x1p-1074; y[k++] = 0x1.8p1; y[k++] = 1e400 > 1; y[k++] = 4.9406564584124654e-324; y[k++] = 2.4703282292062328e-324; y[k++] = 1.7976931348623158e308;y[k++] = 1.7976931348623159e308; y[k++]=.1e1; y[k++] = 1.e-1; y[k++] = 0x.8p0; y[k++] = 1e-400; y[k++] = 0.1 + 0.2; y[k++] = 1.0 - 0.9; y[k++] = 9007199254740993.0; y[k++] = 0.30000000000000004; y[k++] = 123456789012345678901234567890.0; y[k++]=5e-324; y[k++] = 2.2250738585072011e-308; y[k++] = 0.1f; y[k++] = 1e23; y[k++]=8.41e21; y[k++] = 9.5e-5; y[k++] = 1e22+1;
}',
fpcmp = '
void f(int n, const double *x, double *y) {
  int k = 0; double nan = x[15], inf = x[13], z = x[0], nz = x[1];
  y[k++] = nan == nan; y[k++] = nan != nan; y[k++] = nan < 1; y[k++] = nan > 1; y[k++] = nan <= nan; y[k++] = nan >= nan; y[k++] = !(nan < 1); y[k++] = !nan; y[k++] = nan ? 1 : 2; y[k++] = nan && 1; y[k++] = nan || 0;
  y[k++] = z == nz; y[k++] = 1 / z; y[k++] = 1 / nz; y[k++] = -z; y[k++] = z - z; y[k++] = nz + z; y[k++] = nz + nz; y[k++] = nz * 1; y[k++] = z * -1; y[k++] = nz ? 1 : 2; y[k++] = !nz; y[k++] = 0 - z; y[k++] = -nz;
  y[k++] = inf - inf; y[k++] = inf * z; y[k++] = inf > 1e308; y[k++] = -inf < -1e308; y[k++] = inf == inf; y[k++] = z / z; y[k++] = inf / inf;
  if (nan) y[k++] = 1; else y[k++] = 2; if (nan < 1) y[k++] = 1; else y[k++] = 2; if (!(nan >= 1)) y[k++] = 1; else y[k++] = 2; if (nan != 1) y[k++] = 1; else y[k++] = 2; if (nan == nan) y[k++] = 1; else y[k++] = 2;
  if (nan > 1 || nan < 1) y[k++] = 1; else y[k++] = 2; while (nan < 1) { y[k] = 5; break; } k++; for (int i = 0; !(i >= nan) && i < 3; i++) y[k] += 1; k++;
  float fn = (float)nan; y[k++] = fn == fn; y[k++] = fn < 1.0f; y[k++] = !(fn > 1.0f); if (fn <= 1.0f) y[k++] = 1; else y[k++] = 2; if (fn != fn) y[k++] = 1; else y[k++] = 2;
  y[k++] = x[10] > 0; y[k++] = x[10] / 2; y[k++] = x[10] * 0.5; y[k++] = x[12] / 2; y[k++] = x[12] * x[12]; y[k++] = x[8] * 10; y[k++] = x[8] + x[8]; y[k++] = x[24] + 0.2; y[k++] = x[24] * 3; y[k++] = x[8] - x[8] * 1.0000001;
  double m = nan; y[k++] = (m < 1) ? 1 : (m >= 1) ? 2 : 3; y[k++] = (m != m) + (m == m) * 2; int cnt = 0; for (int i = 0; i < n; i++) cnt += (x[i] < x[(i + 1) % n]) + 2 * (x[i] <= x[(i + 1) % n]) + 4 * (x[i] == x[(i + 1) % n]) + 8 * (x[i] != x[(i+1)%n]); y[k++] = cnt;
  for (int i = 0; i < 20; i++) { double u = x[i], v = x[(i + 5) % n]; y[k++] = (u < v) | (u > v) << 1 | (u <= v) << 2 | (u >= v) << 3 | (u == v) << 4 | (u != v) << 5 | (!(u < v)) << 6; }
}',
control = '
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
}',
structs = '
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
  char str[] = "hello"; y[k++] = sizeof str; y[k++] = strlen(str); y[k++] = str[1]; y[k++] = "abc"[1]; y[k++] = sizeof("ab" "cd"); const char *cs = "x\\0y"; y[k++] = strlen(cs); y[k++] = sizeof("x\\0y"); 
  int vla_n = 3 + (n > 0); double vla[vla_n]; for (int i = 0; i < vla_n; i++) vla[i] = i * 1.5; y[k++] = vla[3]; y[k++] = sizeof vla;
  enum E { A, B = 5, C, D = -1, Z = 0x7fffffff }; y[k++] = C; y[k++] = D; y[k++] = sizeof(enum E); y[k++] = Z; enum E e = D; y[k++] = e < 0;
}',
fnptr = '
static double add(double a, double b) { return a + b; } static double mul(double a, double b) { return a * b; }
static int cmpi(int a, int b) { return a - b; }
typedef double (*binop)(double, double);
static double apply(binop g, double a, double b) { return g(a, b); }
static binop pick(int i) { return i ? add : mul; }
static double va(int cnt, ...) { va_list ap; va_start(ap, cnt); double s = 0; for (int i = 0; i < cnt; i++) s += va_arg(ap, double); va_end(ap); return s; }
static double vi(int cnt, ...) { va_list ap; va_start(ap, cnt); double s = 0; for (int i = 0; i < cnt; i++) { if (i % 2) s += va_arg(ap, double); else s += va_arg(ap, int); } va_end(ap); return s; }
static long long vl(int cnt, ...) { va_list ap; va_start(ap, cnt); long long s = 0; for (int i = 0; i < cnt; i++) s += va_arg(ap, long long); va_end(ap); return s; }
static double many(double a, double b, double c, double d, double e, double f1, double g, double h, double i, double j, int k, int l, int m, int nn, int o, int p, int q, int r, float s, char t) { return a + 2*b + 3*c + 4*d + 5*e + 6*f1 + 7*g + 8*h + 9*i + 10*j + 11*k + 12*l + 13*m + 14*nn + 15*o + 16*p + 17*q + 18*r + 19*s + 20*t; }
static float ff(float a, float b) { return a / b; } static short fs(short a) { return a + 1; } static unsigned char fu(unsigned char a) { return a + 1; } static signed char fsc(int a) { return a; } static _Bool fb(int a) { return a; } static unsigned fuu(long long a) { return a; } static long long fll(unsigned a) { return a; } static long long fll2(int a) { return a; } static char fc(int a) { return a; }
void f(int n, const double *x, double *y) {
  int k = 0; binop t[2] = {add, mul}; y[k++] = t[0](3, 4); y[k++] = t[1](3, 4); y[k++] = apply(mul, 5, 6); y[k++] = pick(1)(2, 3); y[k++] = (*pick(0))(2, 3); y[k++] = (pick(0) == mul); int (*c)(int, int) = cmpi; y[k++] = c(3, 10); y[k++] = (*****c)(10, 3);
  double (*m1)(double) = sqrt; y[k++] = m1(2.0); double (*mt[3])(double) = {sin, cos, exp}; for (int i = 0; i < 3; i++) y[k++] = mt[i](1.0); y[k++] = apply(pow, 2, 10); y[k++] = apply(fmod, 7.5, 2);
  y[k++] = va(3, 1.5, 2.5, 3.5); y[k++] = va(0); y[k++] = va(10, 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.); y[k++] = vi(6, 1, 2.5, 3, 4.5, 5, 6.5); y[k++] = vi(12, 1, 2.5, 3, 4.5, 5, 6.5, 7, 8.5, 9, 10.5, 11, 12.5); y[k++] = (double)vl(9, 1LL, 2LL, 3LL, 4LL, 5LL, 6LL, 1LL << 40, -8LL, 9LL);
  y[k++] = va(2, 1.5f, 2.25f); y[k++] = vi(3, (char)-3, 0.5f, (short)-7);
  y[k++] = many(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19.5f, 20);
  y[k++] = ff(1, 3); y[k++] = fs(32767); y[k++] = fu(255); y[k++] = fsc(200); y[k++] = fb(256); y[k++] = fuu(-1); y[k++] = (double)fll(4000000000u); y[k++] = (double)fll2(-5); y[k++] = fc(200); y[k++] = fs(-32769 + 0); y[k++] = fu(511); y[k++] = fb(0.5); y[k++] = ff(16777217, 1);
  y[k++] = fabs(-0.0); y[k++] = 1 / fabs(x[1]); y[k++] = sqrt(x[1]); y[k++] = 1 / sqrt(x[1]); y[k++] = floor(-0.5); y[k++] = 1 / ceil(-0.5); y[k++] = 1/trunc(-0.5); y[k++] = 1/round(-0.4); y[k++] = round(2.5); y[k++] = rint(2.5); y[k++] = nearbyint(3.5); y[k++] = round(-2.5); y[k++] = fmax(x[15], 1); y[k++] = fmin(1, x[15]); y[k++] = 1 / fmax(x[0], x[1]); y[k++] = 1 / fmin(x[0], x[1]); y[k++] = 1/copysign(0, -1); y[k++] = copysign(x[15], -1) != 0; y[k++] = fma(0.1, 10, -1); y[k++] = 0.1 * 10 - 1; y[k++] = ldexp(1, -1074); y[k++] = ldexp(1, 1024); y[k++] = ldexp(0.75, -1075); int ex; y[k++] = frexp(x[10], &ex); y[k++] = ex; y[k++] = frexp(x[13], &ex); double ip; y[k++] = modf(-3.75, &ip); y[k++] = ip; y[k++] = modf(x[14], &ip); y[k++] = ip; y[k++] = 1 / modf(-3.0, &ip); y[k++] = hypot(x[13], x[15]); y[k++] = pow(x[15], 0); y[k++] = pow(1, x[15]); y[k++] = pow(-8, 1.0/3); y[k++] = pow(x[1], -1); y[k++] = pow(x[1], -2); y[k++] = nextafter(0, 1); y[k++] = nextafter(1, 2) - 1; y[k++] = nextafter(x[8]*1.7976931348623157, x[13]); y[k++] = fdim(1, 3); y[k++] = remainder(5, 3); y[k++] = remainder(7.5, 2); y[k++] = fmod(-7.5, 2); y[k++] = 1/fmod(-4, 2); y[k++] = logb(8); y[k++] = logb(x[0]); y[k++] = tgamma(0.5); y[k++] = tgamma(-1); y[k++] = lgamma(-0.5); y[k++] = tgamma(172); y[k++] = erf(x[13]); y[k++] = erfc(-x[13]); y[k++] = cbrt(-27); y[k++] = exp2(1024); y[k++] = exp2(-1074); y[k++] = expm1(1e-20); y[k++] = log1p(1e-20); y[k++] = atan2(x[1], -1); y[k++] = atan2(x[1], x[1]); y[k++] = atan2(x[13], x[14]); y[k++] = acosh(1); y[k++] = atanh(1); y[k++] = asinh(x[1]) == 0; y[k++] = log(x[0]); y[k++] = log(-1) != log(-1); y[k++] = log10(1e15); y[k++] = log2(x[10]); y[k++] = tanh(1e10); y[k++]=sinh(710); y[k++] = cosh(711);
}'
)
run_tests(T, "t1")
