source("common.R")
for (tier in c("native", "interp")) { cat("=====", tier, "\n")
# 1. commented-out old definition (multi-line comment) with const; real one non-const
src <- "/* old version:
void scale(int n, const double *x) {
  }
*/
void scale(int n, double *x) { for (int i = 0; i < n; i++) x[i] *= 2; }"
f <- try_(cfunction(src, n = "n", tier = tier)); show("decoy in multi-line comment: compiled without `out`?", if (inherits(f, "caught")) f else "compiled")
if (!inherits(f, "caught")) { v <- c(1, 2, 3); w <- v; r <- f(x = v); show("caller's v after call (must be 1 2 3 by D24)", v); show("alias w", w); show("result", r) }
# single-line comment variant works?
src2 <- "/* void scale(int n, const double *x) { */\nvoid scale(int n, double *x) { for (int i = 0; i < n; i++) x[i] *= 2; }"
show("single-line comment decoy", try_(cfunction(src2, n = "n", tier = tier)))
# #if 0 decoy
src3 <- "#if 0\nvoid scale(int n, const double *x) { }\n#endif\nvoid scale(int n, double *x) { for (int i = 0; i < n; i++) x[i] *= 2; }"
f <- try_(cfunction(src3, n = "n", tier = tier)); show("#if 0 decoy compiled without out?", if (inherits(f, "caught")) f else "compiled")
if (!inherits(f, "caught")) { v <- c(1, 2, 3); f(x = v); show("caller's v after call", v) }
# string literal decoy
src4 <- "static const char *doc = \"void scale(int n, const double *x) {\";\nvoid scale(int n, double *x) { for (int i = 0; i < n; i++) x[i] *= 2; }"
f <- try_(cfunction(src4, n = "n", tier = tier)); show("string decoy", if (inherits(f, "caught")) f else "compiled")
# #define const
src5 <- "#define const\nvoid scale(int n, const double *x) { for (int i = 0; i < n; i++) x[i] *= 2; }"
f <- try_(cfunction(src5, n = "n", tier = tier)); show("#define const", if (inherits(f, "caught")) f else "compiled")
if (!inherits(f, "caught")) { v <- c(1, 2, 3); f(x = v); show("caller's v after call", v) }
# return types
for (rt in c("short int", "unsigned int", "signed int", "long int", "const int", "static int", "volatile double", "long double", "_Bool", "char", "float", "unsigned", "long", "size_t")) {
  f <- try_(cfunction(sprintf("%s g(int a) { return a; }", rt), tier = tier))
  show(paste("return type", rt), if (inherits(f, "caught")) f else f(70000L))
}
show("int returning INT_MIN", try_(cfunction("int g(void) { return -2147483647 - 1; }", tier = tier)()))
show("int out holding INT_MIN", try_(cfunction("void g(int *y) { y[0] = -2147483647 - 1; }", out = c(y = 1), tier = tier)()))
# static local across calls and between two compiles
src <- "int cnt(void) { static int c = 0; return ++c; }"
a <- cfunction(src, tier = tier); b <- cfunction(src, tier = tier)
show("static counter a,a,b,a", c(a(), a(), b(), a()))
# numbers
sq <- cfunction("double h(int k, double d) { return k + d; }", tier = tier)
for (k in list(NA_integer_, NA_real_, NaN, Inf, 2147483647, 2147483648, -2147483647, -2147483648, 1.5, -0, integer(0), c(1L, 2L), "1", TRUE, NULL, 3+0i, factor("a"), 1e-320, structure(1L, class = "foo"), Sys.Date()))
  show(paste("int k =", deparse(k)), try_(sq(k, 1)))
for (d in list(NA_integer_, NA_real_, NaN, Inf, -0, numeric(0), c(1, 2), "1", TRUE, NULL, factor("a"), .Machine$integer.max, structure(2.5, class = "bar")))
  show(paste("double d =", deparse(d)), try_(sq(1L, d)))
show("missing arg", try_(sq(1L)))
show("extra arg", try_(sq(1L, 2, 3)))
show("named swapped", try_(sq(d = 0.5, k = 2L)))
}
