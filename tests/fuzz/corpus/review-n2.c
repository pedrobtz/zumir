void f(int n, const double *x, double *y) {
unsigned mask = 0xff; for (int i = 0; i < n; i++) { unsigned h = (unsigned)x[i]; y[i] = mask & h; }
}
