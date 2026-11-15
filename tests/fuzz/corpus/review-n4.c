void f(int n, const double *x, double *y) {
unsigned mask = 0xff; unsigned h = 0; for (int i = 0; i < n; i++) { h = h * 31u + i; } y[0] = h & mask;
}
