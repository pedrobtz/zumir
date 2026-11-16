#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

void f(int n, const double *x, double *y) {
  int k = 0; int a = -16; unsigned b = 0x80000001u; long long c = -1024; unsigned long long d = 0x8000000000000001ull;
  for (int s = 0; s < 32; s += 7) { y[k++] = a >> s; y[k++] = b >> s; y[k++] = b << s; y[k++] = (double)(c >> s); y[k++] = (double)(d >> s); y[k++] = (double)(d << s); y[k++] = (double)(1LL << (s + 31)); y[k++] = 1u << s;}
  unsigned char uc = 0x81; y[k++] = uc << 1; y[k++] = uc >> 1; y[k++] = (unsigned char)(uc << 1); signed char sc = -127; y[k++] = sc >> 1; y[k++] = (sc & 0xff) >> 1;
  short sh = -32768; y[k++] = sh >> 15; y[k++] = (unsigned short)sh >> 15; y[k++] = 1 << 30; y[k++] = (double)(1ull << 63 >> 63); y[k++] = (double)(-1LL >> 63); y[k++] = a ^ b; y[k++] = a & b; y[k++] = a | b; y[k++] = ~a; y[k++] = (double)~d; y[k++] = (double)(c ^ d);
  long sl = 40; y[k++] = (double)(1ul << sl); y[k++] = 1 << (char)3; y[k++] = sizeof(1 << 3LL); y[k++] = sizeof((char)1 << 3); 
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
