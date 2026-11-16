void f(int n, const double *x, double *y) {
uint32_t mask = 0xffff; uint32_t h = 5; if (n > 3) h = 9; y[0] = mask & h;
}
