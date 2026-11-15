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
  y[k++] = _Alignof(max_align_t); y[k++] = sizeof(max_align_t); y[k++] = sizeof(_Bool); y[k++] = \'\\377\'; y[k++] = \'\\n\'; y[k++] = \'\\0\'; y[k++] = "\\101\\x41"[1]; y[k++] = \'\\\'\'; y[k++] = u\'a\'; y[k++] = sizeof(u\'a\'); y[k++] = sizeof(U\'a\'); y[k++] = sizeof(u8"a"); y[k++] = sizeof(L"ab"); y[k++] = sizeof(u"ab"); y[k++] = sizeof(U"ab");
  y[k++] = INT8_MIN; y[k++] = UINT8_MAX; y[k++] = INT16_MIN; y[k++] = UINT16_MAX; y[k++] = INT32_MIN; y[k++] = UINT32_MAX; y[k++] = (double)INT64_MIN; y[k++] = (double)UINT64_MAX; y[k++] = (double)INT64_MAX; y[k++] = (double)SIZE_MAX; y[k++] = (double)INTPTR_MAX; y[k++] = (double)PTRDIFF_MIN; y[k++] = sizeof(INT64_C(1)); y[k++] = sizeof(UINT8_C(1)); y[k++] = sizeof(intmax_t); y[k++] = sizeof(int_fast16_t); y[k++] = sizeof(int_least8_t); y[k++] = sizeof(uintptr_t); y[k++] = sizeof(int_fast8_t); y[k++] = sizeof(int_fast32_t); y[k++] = (double)INTMAX_MAX; y[k++] = (double)UINTMAX_MAX; y[k++] = WCHAR_MAX; y[k++] = WCHAR_MIN; y[k++] = SIG_ATOMIC_MAX; y[k++] = INT_FAST16_MAX; y[k++] = (double)UINT_FAST16_MAX; y[k++] = true; y[k++] = sizeof(true); y[k++] = sizeof(bool); y[k++] = (double)LONG_MIN; y[k++] = MB_LEN_MAX; y[k++] = sizeof(NULL);
  y[k++] = FLT_EVAL_METHOD; y[k++] = DECIMAL_DIG; y[k++] = FLT_MANT_DIG; y[k++] = FLT_MAX_EXP; y[k++] = FLT_MIN_EXP; y[k++] = DBL_MAX_10_EXP; y[k++] = DBL_MIN_10_EXP; y[k++] = FLT_MAX_10_EXP; y[k++] = FLT_MIN_10_EXP; y[k++] = LDBL_MANT_DIG; y[k++] = LDBL_DIG; y[k++] = LDBL_MAX_EXP; y[k++] = (double)LDBL_EPSILON; y[k++] = LDBL_MAX > DBL_MAX; y[k++] = FLT_ROUNDS; y[k++] = DBL_DECIMAL_DIG; y[k++] = FLT_DECIMAL_DIG; y[k++] = DBL_TRUE_MIN; y[k++] = FLT_TRUE_MIN; y[k++]=DBL_HAS_SUBNORM;
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
  y[k++] = ((V){.n = 4, .v = {1, 2}}).v[1]; y[k++] = (int[]){5, 6, 7}[2]; y[k++] = sumv((V){1, {1, 1, 1, 1}}); 
  y[k++] = sf(mkf(1.25f)); y[k++] = sdi(mkdi(1.5), mkdi(2.5)); y[k++] = sc3(mkc(), mkc()); y[k++] = sl3(1, mkl(10), 0.5, mkl(100)); y[k++] = sd2(mkd(1), 2, mkd(3), mkd(4), mkd(5), mkd(6), mkd(7)); y[k++] = sif(mkif(1), mkif(2), mkif(3), mkif(4), mkif(5), mkif(6), mkif(7), mkif(8));
  char str[] = "hello"; y[k++] = sizeof str; y[k++] = strlen(str); y[k++] = str[1]; y[k++] = "abc"[1]; y[k++] = sizeof("ab" "cd"); const char *cs = "x\\0y"; y[k++] = strlen(cs); y[k++] = sizeof("x\\0y"); 
  enum E { A, B = 5, C, D = -1, Z = 0x7fffffff }; y[k++] = C; y[k++] = D; y[k++] = sizeof(enum E); y[k++] = Z; enum E e = D; y[k++] = e < 0;
  int m2[2][2] = {1, 2, 3, 4}; y[k++] = m2[1][0]; int m3[][2] = {{1}, {2, 3}, 4}; y[k++] = sizeof m3 / sizeof m3[0]; y[k++] = m3[2][0] + m3[0][1]; struct { int a; struct { int b, c; } in; int d[2]; } nest = {1, 2, 3, 4, 5}; y[k++] = nest.in.c * 10 + nest.d[1];
  struct { int a; struct { int b, c; } in; int d[2]; } nest2 = {.in.c = 7, 8, .a = 2}; y[k++] = nest2.in.c * 100 + nest2.d[0] * 10 + nest2.a; int des[6] = {[4] = 1, 2, [1] = 3, 4}; y[k++] = des[5] * 1000 + des[2] * 100 + des[1] * 10 + des[4]; char cs2[4] = "abcd"; y[k++] = cs2[3]; char cs3[8] = "ab"; y[k++] = cs3[5] + cs3[1];
  static const double tbl[] = {1.5, 2.5, 3.5}; y[k++] = tbl[2]; static int cnt[3]; cnt[1]++; y[k++] = cnt[1]; static const char *names[] = {"a", "bcd"}; y[k++] = strlen(names[1]); static int *sp = &cnt[2]; y[k++] = sp - cnt; static double sd = 1.0 / 3; y[k++] = sd; static long long sll = -1; y[k++] = (double)sll; static unsigned char suc = -1; y[k++] = suc; static float sfl = 0.1; y[k++] = sfl; static short ssh[2] = {-1, 70000}; y[k++] = ssh[1]; static _Bool sb = 5; y[k++] = sb; static double sdi2 = 7; y[k++] = sdi2 / 2; static int sid = 7.9; y[k++] = sid; static unsigned su = -1.0 + 2; y[k++] = su;
}',
bool = '
static _Bool id(_Bool b) { return b; }
void f(int n, const double *x, double *y) {
  int k = 0; y[k++] = (_Bool)0.5; y[k++] = (_Bool)256; y[k++] = (_Bool)x[15]; y[k++] = (_Bool)x[4]; y[k++] = (_Bool)0x100000000LL; 
  int i256 = 256; double h = 0.5; long long L = 0x100000000LL; float fh = 0.25f; const double *p = x;
  y[k++] = (_Bool)i256; y[k++] = (_Bool)h; y[k++] = (_Bool)L; y[k++] = (_Bool)fh; y[k++] = (_Bool)p; y[k++] = (bool)i256; 
  _Bool b1 = i256; _Bool b2 = h; _Bool b3 = L; _Bool b4 = 256; _Bool b5 = 0.5; y[k++] = b1; y[k++] = b2; y[k++] = b3; y[k++] = b4; y[k++] = b5; 
  _Bool b6; b6 = i256; y[k++] = b6; b6 = h; y[k++] = b6; b6 = L; y[k++] = b6; b6 = 2; y[k++] = b6; b6 = 0.5; y[k++] = b6;
  y[k++] = id(i256); y[k++] = id(h); y[k++] = id(256); y[k++] = id(0.5); y[k++] = id(L);
  _Bool b7 = 1; b7++; y[k++] = b7; b7 += 5; y[k++] = b7; b7 = 1; b7--; y[k++] = b7; b7--; y[k++] = b7; b7 = 0; b7 |= 6; y[k++] = b7; b7 = 1; b7 <<= 3; y[k++] = b7; b7 = 1; b7 += 255; y[k++] = b7; b7 = 0; b7 += 0.3; y[k++] = b7;
  struct { _Bool a : 1; _Bool b; } s; s.a = 2; s.b = 256; y[k++] = s.a; y[k++] = s.b; _Bool arr[2] = {256, 0.1}; y[k++] = arr[0]; y[k++] = arr[1];
  y[k++] = (_Bool)(char)256; y[k++] = (_Bool)(i256 + 0); y[k++] = (_Bool)(h * 1); y[k++] = (_Bool)-1; y[k++] = (_Bool)(unsigned char)i256; y[k++] = (int)(_Bool)2; y[k++] = (_Bool)2 + (_Bool)2; y[k++] = !!i256; y[k++] = (_Bool)1e-300; y[k++] = (_Bool)x[10];
}',
ull = '
static unsigned long long conv(double d) { return d; }
void f(int n, const double *x, double *y) {
  int k = 0; double v = 18446744073709549568.0, w = 1e19, z = 9223372036854775808.0, q = 9223372036854777856.0, r = 4294967296.5, s = 9223372036854774784.0;
  y[k++] = (double)(unsigned long long)v; y[k++] = (double)(unsigned long long)w; y[k++] = (double)(unsigned long long)z; y[k++] = (double)(unsigned long long)q; y[k++] = (double)(unsigned long long)r; y[k++] = (double)(unsigned long long)s;
  unsigned long long u = v; y[k++] = (double)u; y[k++] = (double)(u >> 32); u = w; y[k++] = (double)(u % 1000); y[k++] = (double)conv(q); y[k++] = (double)(uint64_t)x[23]; y[k++] = (double)(size_t)x[23]; y[k++] = (double)(unsigned long)x[23];
  float fv = 1.8446743e19f, fw = 9.223373e18f; y[k++] = (double)(unsigned long long)fv; y[k++] = (double)(unsigned long long)fw; y[k++] = (double)(unsigned long long)3e9f;
  y[k++] = (double)(unsigned long long)18446744073709549568.0; y[k++] = (double)(unsigned long long)1e19; y[k++] = (unsigned)3e9; double t = 3e9; y[k++] = (unsigned)t; y[k++] = (unsigned)x[21]; y[k++] = (unsigned short)x[28]; y[k++] = (unsigned char)x[26]; float tf = 3e9f; y[k++] = (unsigned)tf; y[k++] = (double)(long long)x[23 - 1]; y[k++]=(double)(unsigned long long)x[22];
  long double ld = v; y[k++] = (double)(unsigned long long)ld; y[k++] = (double)ld; y[k++] = sizeof(ld); ld = 1e19L; y[k++] = (double)(unsigned long long)ld; ld = 0.1L; y[k++] = (double)ld; y[k++] = (double)(ld * 3); y[k++] = (double)(1.0L / 3); y[k++] = (double)(ld + 0.2L); y[k++] = 0.1L == 0.1; y[k++] = (double)(1e308L * 10); y[k++] = (double)(1e308L * 10 / 10); y[k++] = (double)0x1p-1074L / 2 * 2; y[k++] = (double)(0x1p-1074L / 2 * 2); long double big = 18446744073709551615ull; y[k++] = (double)((unsigned long long)(big) - 18446744073709551614ull); y[k++] = (double)(long long)(9007199254740993.0L + 0);
  long long sl = 9007199254740993LL; long double l2 = sl; y[k++] = (double)((long long)l2 - sl); 
}'
)
run_tests(T, "t2")
