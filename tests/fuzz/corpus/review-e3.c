void f(int n, const double *x, double *y) {
unsigned v = 127; int t = x[0] < 1.0 ? 7 : 5; y[0] = v & t;
}
