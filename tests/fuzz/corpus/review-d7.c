void f(int n, const double *x, double *y) {
unsigned v = 127; double d = x[0];
 y[0] = v & (d < 1.0 ? (int)d : 5);
}
