#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

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
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
