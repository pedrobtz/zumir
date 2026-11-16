void f(int n, const double *x, double *y) {
unsigned mask = 0xff; unsigned r = mask & (unsigned)n; y[0] = r;
}
