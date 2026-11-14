void rhs(double t, const double *y, double *dy, const double *p) {
  dy[0] = -p[0] * y[0] + p[1] * y[1] * y[2];
  dy[1] = p[0] * y[0] - p[1] * y[1] * y[2] - p[2] * y[1] * y[1];
  dy[2] = p[2] * y[1] * y[1] + 0 * t;
}
