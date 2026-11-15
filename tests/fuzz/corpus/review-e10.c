void f(int n, const double *x, double *y) {
unsigned v = 127; int t = 5; if (x[0] < 1.0) t = 7; unsigned r = v & t; y[0] = 1; y[1] = r;
}
