#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

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
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
