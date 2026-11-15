void f(int n, const double *x, double *y) {
double d = 4294967295.0;
 y[0] = (d < 1e9 ? (int)d : 5);
}
