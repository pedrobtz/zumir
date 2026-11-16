#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

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
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
