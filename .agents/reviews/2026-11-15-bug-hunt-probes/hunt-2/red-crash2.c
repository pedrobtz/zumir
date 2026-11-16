void f(int n, const double *x, double *y) {
  int k = 0;
  unsigned short v2 = (unsigned short)0x80000000u;
  unsigned short v3 = (unsigned short)(long long)x[42];
  unsigned long long v4 = (unsigned long long)(-9223372036854775807LL-1);
  unsigned v8 = (unsigned)0x7fffLL;
  short v10 = (short)(-9223372036854775807LL-1);
  v10 &= (((v4 & v3)) + (((double)1 != (double)v8))); y[k++] = (double)v10;
  v2 = v8; y[k++] = (double)v2;
  y[k++] = (double)(((double)v2 == (double)((((((v2 * 13u)) << ((((-129) & (-7))) & 31))) & ((127, ((v2 ? v10 / v2 : 3)))))))); y[k++] = sizeof(((double)v2 == (double)((((((v2 * 13u)) << ((((-129) & (-7))) & 31))) & ((127, ((v2 ? v10 / v2 : 3))))))));
}
