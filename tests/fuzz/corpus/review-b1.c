void f(int n, const double *x, double *y) {
unsigned v10 = 127; double d = x[0];
 y[0] = (-v10) & ((d > -1e9 && d < 1e9 ? (int)d : 5) >> 3);
}
