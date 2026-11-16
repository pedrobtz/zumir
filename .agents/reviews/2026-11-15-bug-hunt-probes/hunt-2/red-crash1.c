void f(int n, const double *x, double *y) {
  int k = 0;
  long long v1 = (long long)(long long)x[40];
  long long v2 = (long long)(long long)x[41];
  unsigned long long v3 = (unsigned long long)(long long)x[42];
  long long v4 = (long long)0xffffffffu;
  int v5 = (int)(long long)x[44];
  unsigned long v6 = (unsigned long)(long long)x[45];
  int v7 = (int)(long long)x[46];
  unsigned char v8 = (unsigned char)'a';
  unsigned char v9 = (unsigned char)(long long)x[48];
  unsigned v10 = (unsigned)127;
  v8 = (((7 * 1u)) * (-v3)); y[k++] = (double)v8;
  v7 = v4; y[k++] = (double)v7;
  y[k++] = (double)((unsigned short)v2); y[k++] = sizeof((unsigned short)v2);
  v2 = v5; y[k++] = (double)v2;
  y[k++] = (double)(((-v10) & ((((((double)255 + (double)((v8, v4))) > -1e9 && ((double)255 + (double)((v8, v4))) < 1e9 ? (int)((double)255 + (double)((v8, v4))) : 5)) >> ((((unsigned)((unsigned short)v9) * (((unsigned)v1 * v1)))) & 31))))); y[k++] = sizeof(((-v10) & ((((((double)255 + (double)((v8, v4))) > -1e9 && ((double)255 + (double)((v8, v4))) < 1e9 ? (int)((double)255 + (double)((v8, v4))) : 5)) >> ((((unsigned)((unsigned short)v9) * (((unsigned)v1 * v1)))) & 31)))));
  y[k++] = (double)((unsigned)(((((unsigned)32767 + v8)) << (((v6 + 9223372036854775807LL)) & 31)))); y[k++] = sizeof((unsigned)(((((unsigned)32767 + v8)) << (((v6 + 9223372036854775807LL)) & 31))));
  y[k++] = (double)((((v7 ? v10 % v7 : 3)) - ((v5 - v10)))); y[k++] = sizeof((((v7 ? v10 % v7 : 3)) - ((v5 - v10))));
  v3 = (((((v8 || 13u)) ^ ((v4 || ('a'))))) + ((unsigned)((v1 ? (unsigned long long)v2 % v1 : 3)))); y[k++] = (double)v3;
}
