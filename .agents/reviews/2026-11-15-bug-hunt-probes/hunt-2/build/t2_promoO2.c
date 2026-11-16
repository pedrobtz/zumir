#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

void f(int n, const double *x, double *y) {
  unsigned char uc = 200; signed char sc = -56; char c = (char)200; short s = -1; unsigned short us = 65535;
  int k = 0;
  y[k++] = uc + uc; y[k++] = sc * 2; y[k++] = c; y[k++] = (c < 0); y[k++] = s + us; y[k++] = us * us / 3.0 > 0;
  y[k++] = (unsigned char)(uc + 100); y[k++] = (signed char)(sc - 100); y[k++] = ~uc; y[k++] = -uc; y[k++] = !uc;
  y[k++] = (short)(us + 1); y[k++] = (unsigned short)(s); y[k++] = uc << 24; y[k++] = (uc << 24) >> 24; y[k++] = sizeof(c + c);
  y[k++] = sizeof(char); y[k++] = sizeof(long); y[k++] = sizeof(long long); y[k++] = sizeof(long double); y[k++] = sizeof(void*);
  y[k++] = sizeof(int); y[k++] = sizeof(short); y[k++] = sizeof(float); y[k++] = sizeof(size_t); y[k++] = sizeof('a');
  y[k++] = CHAR_MIN; y[k++] = CHAR_MAX; y[k++] = (char)-1 < 0; y[k++] = '\xff'; y[k++] = "\xff"[0]; y[k++] = L'a';
  y[k++] = sizeof(wchar_t); y[k++] = (wchar_t)-1 < 0; y[k++] = sizeof(L'a'); y[k++]=sizeof(ptrdiff_t); y[k++] = _Alignof(double); y[k++] = _Alignof(long double);
  y[k++] = _Alignof(max_align_t); y[k++] = sizeof(max_align_t); y[k++] = sizeof(_Bool); y[k++] = '\377'; y[k++] = '\n'; y[k++] = '\0'; y[k++] = "\101\x41"[1]; y[k++] = '\''; y[k++] = u'a'; y[k++] = sizeof(u'a'); y[k++] = sizeof(U'a'); y[k++] = sizeof(u8"a"); y[k++] = sizeof(L"ab"); y[k++] = sizeof(u"ab"); y[k++] = sizeof(U"ab");
  y[k++] = INT8_MIN; y[k++] = UINT8_MAX; y[k++] = INT16_MIN; y[k++] = UINT16_MAX; y[k++] = INT32_MIN; y[k++] = UINT32_MAX; y[k++] = (double)INT64_MIN; y[k++] = (double)UINT64_MAX; y[k++] = (double)INT64_MAX; y[k++] = (double)SIZE_MAX; y[k++] = (double)INTPTR_MAX; y[k++] = (double)PTRDIFF_MIN; y[k++] = sizeof(INT64_C(1)); y[k++] = sizeof(UINT8_C(1)); y[k++] = sizeof(intmax_t); y[k++] = sizeof(int_fast16_t); y[k++] = sizeof(int_least8_t); y[k++] = sizeof(uintptr_t); y[k++] = sizeof(int_fast8_t); y[k++] = sizeof(int_fast32_t); y[k++] = (double)INTMAX_MAX; y[k++] = (double)UINTMAX_MAX; y[k++] = WCHAR_MAX; y[k++] = WCHAR_MIN; y[k++] = SIG_ATOMIC_MAX; y[k++] = INT_FAST16_MAX; y[k++] = (double)UINT_FAST16_MAX; y[k++] = true; y[k++] = sizeof(true); y[k++] = sizeof(bool); y[k++] = (double)LONG_MIN; y[k++] = MB_LEN_MAX; y[k++] = sizeof(NULL);
  y[k++] = FLT_EVAL_METHOD; y[k++] = DECIMAL_DIG; y[k++] = FLT_MANT_DIG; y[k++] = FLT_MAX_EXP; y[k++] = FLT_MIN_EXP; y[k++] = DBL_MAX_10_EXP; y[k++] = DBL_MIN_10_EXP; y[k++] = FLT_MAX_10_EXP; y[k++] = FLT_MIN_10_EXP; y[k++] = LDBL_MANT_DIG; y[k++] = LDBL_DIG; y[k++] = LDBL_MAX_EXP; y[k++] = (double)LDBL_EPSILON; y[k++] = LDBL_MAX > DBL_MAX; y[k++] = FLT_ROUNDS; y[k++] = DBL_DECIMAL_DIG; y[k++] = FLT_DECIMAL_DIG; y[k++] = DBL_TRUE_MIN; y[k++] = FLT_TRUE_MIN; y[k++]=DBL_HAS_SUBNORM;
}
void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }
