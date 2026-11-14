#include <stdarg.h>
double sum(int n, ...) {
  va_list ap;
  double s = 0;
  va_start(ap, n);
  for (int i = 0; i < n; i++) s += va_arg(ap, double);
  va_end(ap);
  return s;
}
