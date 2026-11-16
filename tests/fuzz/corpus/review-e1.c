void f(int n, const double *x, double *y) {
unsigned v = 127; y[0] = v & (x[0] < 1.0 ? 7 : 5);
}
