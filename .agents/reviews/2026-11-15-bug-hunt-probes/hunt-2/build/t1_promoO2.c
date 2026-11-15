#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

void f(int n, const double *x, double *y) {
  unsigned char uc = 200; signed char sc = -56; char c = (char)200; short s = -1; unsigned short us = 65535;
  int k = 0;
  y[k++] = uc + uc; y[k++] = sc * 2; y[k++] = c; y[k++] = (c < 0); y[k++] = s + us; y[k++] = us * us / 3.0 > 0;
  y[k++] = (unsigned char)(uc + 100); y[k++] = (signed char)(sc - 100); y[k++] = ~uc; y[k++] = -uc; y[k++] = !uc;
  y[k++] = (short)(us + 1); y[k++] = (unsigned short)(s); y[k++] = uc << 24; y[k++] = (uc << 24) >> 24; y[k++] = sizeof(c + c);
  y[k++] = sizeof(char); y[k++] = sizeof(long); y[k++] = sizeof(long long); y[k++] = sizeof(long double); y[k++] = sizeof(void*);
  y[k++] = sizeof(int); y[k++] = sizeof(short); y[k++] = sizeof(float); y[k++] = sizeof(size_t); y[k++] = sizeof('a');
  y[k++] = CHAR_MIN; y[k++] = CHAR_MAX; y[k++] = (char)-1 < 0; y[k++] = '\xff'; y[k++] = "\xff"[0]; y[k++] = L'a';
  y[k++] = sizeof(wchar_t); y[k++] = (wchar_t)-1 < 0; y[k++] = sizeof(L'a'); y[k++]=sizeof(ptrdiff_t); y[k++] = _Alignof(double); y[k++] = _Alignof(long double);
  y[k++] = _Alignof(max_align_t); y[k++] = sizeof(max_align_t); y[k++] = sizeof(_Bool); y[k++] = 'ab'; 
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
