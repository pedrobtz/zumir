#include <math.h>
double norm(int n, const double *x) {
  double s = 0;
  for (int i = 0; i < n; i++) s = fma(x[i], x[i], s);
  return sqrt(s);
}
