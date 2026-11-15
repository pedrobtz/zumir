# Minimal repros of cfunction() wrong results, each beside cc's answer (computed by harness.R's cc_run).
source("harness.R")
T <- list(
bool_from_int = 'void f(int n, const double *x, double *y) { int flags = 0x300; bool has = flags & 0x100; y[0] = has; y[1] = has ? 10 : 20; }',
bool_from_double = 'void f(int n, const double *x, double *y) { bool nz = x[4]; /* 0.5 */ y[0] = nz; bool b2 = (bool)x[24]; /* 0.1 */ y[1] = b2; }',
bool_from_pointer = 'void f(int n, const double *x, double *y) { _Bool p = x; y[0] = p; y[1] = (_Bool)256; _Bool q = 1; q += 1; y[2] = q; }',
u64_from_double = 'void f(int n, const double *x, double *y) { unsigned long long u = 1e19; y[0] = (double)u; uint64_t v = (uint64_t)x[23]; /* 1e19 */ y[1] = (double)v; y[2] = (double)(size_t)18446744073709549568.0; }',
float_h = 'void f(int n, const double *x, double *y) { y[0] = DBL_DIG; y[1] = FLT_DIG; y[2] = LDBL_MANT_DIG; y[3] = sizeof(long double); y[4] = LDBL_EPSILON > 1e-17; y[5] = DECIMAL_DIG; }',
char_const = 'void f(int n, const double *x, double *y) { y[0] = sizeof(\'a\'); y[1] = sizeof(L"ab"); }'
)
run_tests(T, "fc")
cat("== NULL on this platform\n"); print(tryCatch(cfunction("int f(const double *p) { return p == NULL; }"), error = function(e) conditionMessage(e)))
