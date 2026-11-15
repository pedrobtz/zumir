void f(int n, const double *x, double *y) {
unsigned char mask = 0x0f; unsigned char c = 5; if (n > 3) c = 9; unsigned char r = mask & c; y[0] = r;
}
