void f(int n, const double *x, double *y) {
unsigned v = 127; double d = 2.0; int t = (d < 1.0 ? (int)d : 5);
 y[0] = v & t;
}
