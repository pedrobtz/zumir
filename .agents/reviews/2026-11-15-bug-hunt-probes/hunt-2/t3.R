source("harness.R")
NY <- 4000L
m1 <- c("acos","asin","atan","cos","sin","tan","acosh","asinh","atanh","cosh","sinh","tanh","exp","exp2","expm1","log","log10","log1p","log2","logb","cbrt","fabs","sqrt","erf","erfc","lgamma","tgamma","ceil","floor","nearbyint","rint","round","trunc")
m2 <- c("atan2","hypot","pow","fmod","remainder","copysign","nextafter","fdim","fmax","fmin")
T <- list()
T$math1 <- paste0("void f(int n, const double *x, double *y) { int k = 0; for (int i = 0; i < 40; i++) {\n",
  paste(sprintf("y[k++] = %s(x[i]);", m1), collapse = "\n"), "\n} }")
T$math1const <- paste0("void f(int n, const double *x, double *y) { int k = 0;\n",
  paste(as.vector(outer(m1, c("0.0", "-0.0", "1.0", "-1.0", "0.5", "1e308", "5e-324", "INFINITY", "-INFINITY", "NAN", "710.0", "1e-320", "2.5", "-2.5", "HUGE_VAL", "-HUGE_VAL", "(double)INFINITY", "3", "-7"), function(f, v) sprintf("y[k++] = %s(%s);", f, v))), collapse = "\n"), "\n}")
T$math2 <- paste0("void f(int n, const double *x, double *y) { int k = 0; for (int i = 0; i < 16; i++) for (int j = 0; j < 16; j++) {\n",
  paste(sprintf("y[k++] = %s(x[i], x[j]);", m2), collapse = "\n"), "\n} }")
T$math3 <- "void f(int n, const double *x, double *y) { int k = 0; for (int i = 0; i < 12; i++) for (int j = 0; j < 12; j++) for (int l = 0; l < 12; l++) y[k++] = fma(x[i], x[j], x[l]);
  for (int i = 0; i < 40; i++) { int e = 77; double ip = 77; y[k++] = frexp(x[i], &e); y[k++] = e; y[k++] = modf(x[i], &ip); y[k++] = ip; for (int j = -1080; j <= 1080; j += 540) y[k++] = ldexp(x[i], j); y[k++] = ldexp(x[i], INT_MAX); y[k++] = ldexp(x[i], INT_MIN); } }"
T$mem <- '
void f(int n, const double *x, double *y) { int k = 0;
  double a[10]; for (int i = 0; i < 10; i++) a[i] = i + 0.5;
  memmove(a + 2, a, 6 * sizeof(double)); for (int i = 0; i < 10; i++) y[k++] = a[i];
  memmove(a, a + 3, 6 * sizeof(double)); for (int i = 0; i < 10; i++) y[k++] = a[i];
  char b[40]; memset(b, 0x41, 39); b[39] = 0; y[k++] = strlen(b); b[17] = 0; y[k++] = strlen(b); y[k++] = strlen(""); y[k++] = strlen(b + 18);
  y[k++] = memcmp("abc", "abd", 3) < 0; y[k++] = memcmp("abc", "abd", 2); y[k++] = memcmp("\\xff", "\\x01", 1) > 0; y[k++] = memcmp(a, a, 0);
  void *r = memcpy(y + 100, x, 8 * sizeof(double)); y[k++] = (r == y + 100); r = memset(b, 300, 3); y[k++] = (unsigned char)b[0]; y[k++] = (r == b); y[k++] = (memmove(b + 1, b, 5) == b + 1);
  unsigned char raw[8]; double one = 1.0; memcpy(raw, &one, 8); y[k++] = raw[7]; y[k++] = raw[6]; unsigned long long u; memcpy(&u, &x[1], 8); y[k++] = (double)(u >> 63); float fl = 1.5f; unsigned ui; memcpy(&ui, &fl, 4); y[k++] = ui;
  int big[1000]; memset(big, 0xff, sizeof big); y[k++] = big[999]; memset(big, 0, sizeof big); long s = 0; for (int i = 0; i < 1000; i++) { big[i] = i * i; } for (int i = 0; i < 1000; i++) s += big[i]; y[k++] = s;
  size_t sz = strlen("hello") - 6; y[k++] = sz > 0; y[k++] = (double)sz; y[k++] = sizeof(strlen("")) ; y[k++] = sizeof(size_t);
}'
T$globals <- '
static int gi = 5; static double gd = 2.5; static unsigned char gb[4] = {1, 2, 3, 250}; static long long gl = -3; static double garr[3]; static const int gc[] = {10, 20, 30};
static struct { int a; double b; char c; } gs = {1, 2.5, 3}; static int *gp = &gi; static const char *gstr = "hey"; static float gf = 0.1f; static short gsh = -2; static unsigned gu = 4000000000u; static _Bool gbool = 1;
static int uninit; static double ud; static int counter(void) { return ++uninit; }
static double (*gfn)(double) = sqrt; static int tab[2][2] = {{1, 2}, {3, 4}}; static unsigned long long gull = 18446744073709551615ull; static signed char gsc = -100; static double gneg = -0.0; static double ginf = 1.0 / 0.0; static int gsz = sizeof(double) * 3; static int gneg2 = -(3 + 4) * 2; static unsigned gcast = (unsigned char)300; static double gdiv = 7 / 2; static double gdiv2 = 7 / 2.0; static int gsh2 = 1 << 10 | 3; static int gt = 5 > 3 ? 11 : 22; static char gch = \'A\' + 2; static long long gbig = 1LL << 40; static int gmod = -7 % 3; static unsigned gund = 0u - 1; static int gfl = (int)3.99; static int gfl2 = (int)-3.99; static float gff = 16777217; static double gexp = 1e308 * 10; static int gand = 0xff & ~0x0f; static long long gmix = 2147483647 + 1LL; static unsigned gmix2 = 2147483647 + 1u; static int gcmp = -1 < 0u; static int gcmp2 = -1 < 0ll; static long long gsx = (int)0xffffffffu; static long long gzx = (unsigned)-1; static long long gshr = -8 >> 1; static unsigned long long gshr2 = -8ull >> 1; static int gchar = (char)200; static int guchar = (unsigned char)-56; static short gshort = (short)40000; static double gflt = (float)0.1; static double gflt2 = 0.1f + 0.2f; static double gflt3 = 0.1f * 3; static int gidiv = 2147483647 / -1; static long long glmod = 9223372036854775807LL % 1000; static unsigned long long gumul = 4294967296ull * 4294967295ull; static unsigned gumul2 = 65536u * 65536u; static unsigned gumul3 = 65537u * 65537u; static int gnot = !5 + !0 + ~0; static double gneg3 = -(0.0); static double gd2i = (double)(1ull << 63); static double gu2d = 18446744073709551615ull; static double gl2d = 9007199254740993ll; static float gl2f = 16777217ll; static double gull2f = (float)18446744073709551615ull; 
void f(int n, const double *x, double *y) { int k = 0;
  y[k++] = gi; y[k++] = gd; y[k++] = gb[3]; y[k++] = gl; y[k++] = garr[2]; y[k++] = gc[1]; y[k++] = gs.b + gs.c; y[k++] = *gp; y[k++] = strlen(gstr); y[k++] = gf; y[k++] = gsh; y[k++] = gu; y[k++] = gbool;
  y[k++] = counter(); y[k++] = counter(); y[k++] = ud; y[k++] = gfn(16); y[k++] = tab[1][0]; y[k++] = (double)gull; y[k++] = gsc; y[k++] = 1 / gneg; y[k++] = ginf; y[k++] = gsz; y[k++] = gneg2; y[k++] = gcast; y[k++] = gdiv; y[k++] = gdiv2; y[k++] = gsh2; y[k++] = gt; y[k++] = gch; y[k++] = (double)gbig; y[k++] = gmod; y[k++] = gund; y[k++] = gfl; y[k++] = gfl2; y[k++] = gff; y[k++] = gexp; y[k++] = gand; y[k++] = (double)gmix; y[k++] = gmix2; y[k++] = gcmp; y[k++] = gcmp2; y[k++] = (double)gsx; y[k++] = (double)gzx; y[k++] = (double)gshr; y[k++] = (double)gshr2; y[k++] = gchar; y[k++] = guchar; y[k++] = gshort; y[k++] = gflt; y[k++] = gflt2; y[k++] = gflt3; y[k++] = gidiv; y[k++] = (double)glmod; y[k++] = (double)gumul; y[k++] = gumul2; y[k++] = gumul3; y[k++] = gnot; y[k++] = 1 / gneg3; y[k++] = gd2i; y[k++] = gu2d; y[k++] = gl2d; y[k++] = gl2f; y[k++] = gull2f;
  gi += 10; y[k++] = gi; *gp = 99; y[k++] = gi; gb[0]--; gb[0]--; y[k++] = gb[0]; gl *= gl; y[k++] = gl; garr[1] = 7; y[k++] = garr[0] + garr[1];
}'
run_tests(T, "t3")
