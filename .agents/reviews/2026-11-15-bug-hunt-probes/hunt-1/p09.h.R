options(OutDec = ",", scipen = -5, digits = 3)
source("common.R"); options(OutDec = ",", scipen = -5, digits = 3)
show <- function(label, expr) { w <- NULL; r <- withCallingHandlers(tryCatch(withVisible(expr), error = function(e) paste0("ERR<", class(e)[1], "> ", conditionMessage(e))), warning = function(x) { w <<- conditionMessage(x); invokeRestart("muffleWarning") }); vis <- ""; if (is.list(r) && identical(names(r), c("value", "visible"))) { vis <- if (r$visible) "" else " (invisible)"; r <- r$value }; if (is.function(r)) r <- "FUNCTION"; cat(sprintf("%-30s %s%s%s\n", label, paste(deparse(r, control = c("digits17", "keepNA", "keepInteger", "showAttributes")), collapse = ""), vis, if (is.null(w)) "" else paste("  WARN:", w))) }
# type confusion through a disabled preprocessor branch
g <- cfunction("#if 0\ndouble f(int n, const int *a) {\n#else\ndouble f(int n, const double *a) {\n#endif\n double s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }", n = "n")
show("ifdef type decoy", g(a = c(1, 2, 3, 4)))
show("ifdef type decoy int", g(a = 1:4))
# attributes of supplied outputs
sq <- cfunction("void sq(int n, const double *x, double *y) { for (int i = 0; i < n; i++) y[i] = x[i] * x[i]; }", out = "y", n = "n")
show("y names", sq(x = c(1, 2), y = c(a = 0, b = 0)))
show("y matrix", sq(x = c(1, 2), y = matrix(0, 1, 2)))
show("y class", sq(x = c(1, 2), y = structure(c(0, 0), class = "foo")))
show("x names -> y", sq(x = c(a = 1, b = 2)))
# several inputs
dot <- cfunction("double dot(int n, const double *x, const double *y) { double s = 0; for (int i = 0; i < n; i++) s += x[i] * y[i]; return s; }", n = "n")
show("dot", dot(x = c(1, 2, 3), y = c(4, 5, 6))); show("dot difflen", dot(x = c(1, 2, 3), y = c(4, 5))); show("dot difflen n", dot(x = c(1, 2, 3), y = c(4, 5), n = 2)); show("dot n=3 short y", dot(x = c(1, 2, 3), y = c(4, 5), n = 3)); show("dot same vec", {v <- c(1,2,3); dot(x = v, y = v)}); show("dot empty", dot(x = numeric(0), y = numeric(0))); show("dot int+dbl", dot(x = 1:3, y = c(.5, .5, .5)))
show("dot partial arg", dot(x = c(1, 2), y = c(1, 2), 2))
# n without inputs
fill <- cfunction("void fill(int n, double *y) { for (int i = 0; i < n; i++) y[i] = i; }", out = "y", n = "n")
show("fill no n", fill()); show("fill n=3", fill(n = 3)); show("fill n=0", fill(n = 0)); show("fill n y", fill(n = 2, y = c(9, 9, 9))); show("fill y only", fill(y = c(9, 9, 9)))
fx <- cfunction("void fx(int n, double *y) { for (int i = 0; i < 3; i++) y[i] = i + n; }", out = c(y = 3)); print(formals(fx))
show("fixed n=5", fx(n = 5)); show("fixed n=5 y long", fx(n = 5L, y = c(7, 7, 7, 7))); show("fixed y short", fx(n = 5L, y = c(7, 7))); show("fixed no n", fx())
fx2 <- cfunction("void fx(int n, double *y, int *z) { for (int i = 0; i < 3; i++) y[i] = i + n; for (int i = 0; i < n; i++) z[i] = i; }", out = list(y = 3, "z"), n = "n")
show("fx2 n=2", fx2(n = 2)); show("fx2 no n", fx2()); show("fx2 z short", fx2(n = 2, z = 1L)); show("fx2 y short n omitted", fx2(n=1, y = 1))
# int return + several outputs naming; output named like function
h <- cfunction("int h(int n, int *h2, double *y) { return n; }", out = c("y", "h2"), n = "n"); show("list order", h(n = 1))
h <- cfunction("int y(int n, double *y) { return n; }", out = "y", n = "n"); show("fn named as output", h(n = 1))
h <- cfunction("void v(int n, double *y, int *z) { }", out = c("y", "z"), n = "n"); show("void two outs", h(n = 1))
h <- cfunction("int one(int n, double *y) { return 3; }", out = "y", n = "n"); show("value + one out", h(n = 1))
# large / tiny doubles through
id <- cfunction("double id(double a) { return a; }")
for (v in list(-0, NA_real_, NaN, 5e-324, 1.7976931348623157e308, 2^53 + 2, 0.1, 123456789.123456789, 1L, NA_integer_, .Machine$integer.max)) show(paste("id", deparse(v)), identical(id(v), as.double(v), num.eq = FALSE))
idv <- cfunction("void idv(int n, const double *x, double *y) { for (int i = 0; i < n; i++) y[i] = x[i]; }", out = "y", n = "n")
set.seed(1); v <- c(-0, NA, NaN, 5e-324, 1e308, rnorm(5) * 10^(-150:150)[sample(301, 5)], 2^31, 2^53 + 2); show("idv identical", identical(idv(x = v), v, num.eq = FALSE))
show("idv altrep", identical(idv(x = as.double(1:1e5)), as.double(1:1e5)))
show("idv altrep int", identical(idv(x = 1:1e5), as.double(1:1e5)))
iv <- cfunction("void iv(int n, const int *x, int *y) { for (int i = 0; i < n; i++) y[i] = x[i]; }", out = "y", n = "n")
show("iv altrep int", identical(iv(x = 1:1e5), 1:1e5)); show("iv dbl whole", iv(x = c(1, 2147483647, -2147483647))); show("iv NaN", iv(x = c(NaN, 1))); show("iv -0", iv(x = c(-0, 1)))
