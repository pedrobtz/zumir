void f(int n, const double *x, double *y) {
unsigned v10 = n; double d = x[0];
 y[0] = v10 & ((d < 1e9 ? (int)d : 5) >> 3);
}
