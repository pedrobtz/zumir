source("common.R")
show <- function(label, expr) { w <- NULL; r <- withCallingHandlers(tryCatch(withVisible(expr), error = function(e) paste0("ERR<", class(e)[1], "> ", conditionMessage(e))), warning = function(x) { w <<- conditionMessage(x); invokeRestart("muffleWarning") }); vis <- ""; if (is.list(r) && identical(names(r), c("value", "visible"))) { vis <- if (r$visible) "" else " (invisible)"; r <- r$value }; if (is.function(r)) r <- "FUNCTION"; cat(sprintf("%-30s %s%s%s\n", label, paste(deparse(r, control = c("digits17", "keepNA", "keepInteger", "showAttributes")), collapse = ""), vis, if (is.null(w)) "" else paste("  WARN:", w))) }
for (h in c("stddef.h", "stdint.h", "stdbool.h", "stdarg.h", "float.h", "limits.h", "math.h", "string.h", "stdlib.h")) show(paste("include", h), cfunction(sprintf("#include <%s>\ndouble f(double a) { return a; }", h))(1))
show("int ret INT_MIN", cfunction("int f(int a) { return -2147483647 - 1 + a; }")(0L))
show("mir_function INT_MIN", mir_function("m: module\n export f\nf: func i32\n ret -2147483648\n endfunc\n endmodule", "f")())
show("math", cfunction("double f(double a) { return exp(a) + sin(a) + pow(a, 2) + fma(a, a, a) + lgamma(a) + fabs(a) + hypot(a, a); }")(1.5) - (exp(1.5) + sin(1.5) + 1.5^2 + (1.5 * 1.5 + 1.5) + lgamma(1.5) + 1.5 + sqrt(2 * 1.5^2)))
show("uint64", cfunction("double f(double a) { uint64_t u = (uint64_t) a; size_t s = sizeof(u); bool b = true; return (double) u + s + b + DBL_EPSILON + INT_MAX; }")(3))
show("printf", cfunction("double f(double a) { printf(\"x\"); return a; }"))
show("malloc", cfunction("void *malloc(unsigned long); double f(double a) { malloc(1); return a; }"))
show("R api", cfunction("void Rf_error(const char *, ...); double f(double a) { Rf_error(\"x\"); return a; }"))
for (tier in c("native", "interp")) {
  g <- cfunction("int f(int n, const double *x, const int *k, double *y, int *z, double s) { int t = 0; for (int i = 0; i < n; i++) { y[i] = x[i] * s + k[i]; z[i] = k[i] * 2; t += k[i]; } return t; }", out = c("y", "z"), n = "n", tier = tier)
  show(paste("cf", tier), g(x = c(1.5, -0, NA), k = c(1L, -7L, 2147483647L), s = 0.1))
  m <- mir_function("m: module\n export f\nf: func i64, i8:a, u16:b, i64:c, f:d, d:e\n local i64:r, d:t\n add r, a, b\n add r, r, c\n f2d t, d\n dadd t, t, e\n d2i c, t\n add r, r, c\n ret r\n endfunc\n endmodule", "f", tier = tier)
  show(paste("mf", tier), m(-128, 65535, -2^40, 1.5, 2.25))
}
# contexts are freed with their functions
invisible(gc()); base <- zumir_info()$memory$contexts
fs <- lapply(1:20, function(i) list(mir_compile(function(x) x + 1), cfunction("double f(double a) { return a; }"), mir_function("m: module\n export f\nf: func d, d:a\n ret a\n endfunc\n endmodule", "f")))
cat("contexts:", base, "->", zumir_info()$memory$contexts); rm(fs); invisible(gc()); invisible(gc()); cat(" ->", zumir_info()$memory$contexts, "\n")
# failed compiles leak?
for (i in 1:20) { try(cfunction("double f(double a) { return a +; }"), silent = TRUE); try(cfunction("long double f(double a) { return a; }"), silent = TRUE); try(cfunction("int f(void) { return 1; }"), silent = TRUE); try(mir_function("garbage", "f"), silent = TRUE) }
invisible(gc()); cat("after failures:", zumir_info()$memory$contexts, "\n")
# copy of compiled function shares state?; body/formals modification
g <- mir_compile(function(x, y) x - y); h <- g; formals(h) <- formals(h)[2:1]; show("formals swapped", h(5, 1)); show("orig", g(5, 1))
show("do.call names", do.call(g, list(y = 1, x = 5))); show("partial", g(5, y = 1)); show("extra", g(1, 2, 3))
c1 <- cfunction("double f(double alpha, double beta) { return alpha - beta; }"); show("cf partial match", c1(al = 5, be = 1)); show("cf reorder", c1(beta = 1, alpha = 5))
print(c1); print(unclass(c1))
