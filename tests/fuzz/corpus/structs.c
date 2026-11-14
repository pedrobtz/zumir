#include <string.h>
#include <stdint.h>
typedef struct { double re, im; } cplx;
static cplx mul(cplx a, cplx b) { cplx r = {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re}; return r; }
double f(int n, double *out) {
  cplx z = {1, 0}, w = {0.5, 0.5};
  for (int i = 0; i < n; i++) z = mul(z, w);
  memcpy(out, &z, sizeof z);
  return z.re + (double) (uint32_t) n;
}
