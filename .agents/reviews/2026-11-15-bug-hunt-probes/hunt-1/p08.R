source("common.R")
show <- function(label, expr) { w <- NULL; r <- withCallingHandlers(tryCatch(withVisible(expr), error = function(e) paste0("ERR<", class(e)[1], "> ", conditionMessage(e))), warning = function(x) { w <<- conditionMessage(x); invokeRestart("muffleWarning") }); vis <- ""; if (is.list(r) && identical(names(r), c("value", "visible"))) { vis <- if (r$visible) "" else " (invisible)"; r <- r$value }; if (is.function(r)) r <- "FUNCTION"; cat(sprintf("%-30s %s%s%s\n", label, paste(deparse(r, control = c("digits17", "keepNA", "keepInteger", "showAttributes")), collapse = ""), vis, if (is.null(w)) "" else paste("  WARN:", w))) }
# zero-parameter functions
show("int f(void)", cfunction("int f(void) { return 7; }")())
show("double f(void)", cfunction("double f(void) { return 7; }")())
show("double f()", cfunction("double f() { return 7; }")())
show("void f(void)", cfunction("void f(void) { }")())
sig <- zumir:::cf_signature("int f(void) { return 7; }", "f"); str(sig$params)
str(ifelse(sig$params$kind == "scalar", ifelse(sig$params$type == "int", "i32", "d"), "p"))
# signature forms
src <- function(sig, body = "return 0;") sprintf("%s { %s }", sig, body)
for (s in c("double f(double a)", "double\nf(\n double a\n)", "double f(double a) /* c */", "double f(const double a)", "double f(double const *a)", "double f(const double * const a)", "double f(const double a[])", "double f(double *restrict a)", "unsigned int f(int a)", "unsigned f(int a)", "long f(int a)", "long int f(int a)", "short int f(int a)", "signed int f(int a)", "const int f(int a)", "static int f(int a)", "float f(double a)", "long double f(double a)", "int *f(int a)", "double f(unsigned int a)", "double f(long a)", "double f(float a)", "double f(char a)", "double f(int a, int a2)", "double f(double a, ...)", "double f(const void *a)", "double f(const char *a)", "double f(double (*g)(double))", "extern double f(double a)", "inline double f(double a)", "double f(register double a)", "double (f)(double a)", "double f(int16_t a)", "double f(const double*a,const int*b)", "double f(int n, const double x[n])", "_Bool f(int a)", "double f(volatile double a)", "double f(double a, double b, double c, double d, double e, double g, double h, double i, double j, double k)"))
  show(gsub("\n", "\\\\n", s), cfunction(src(s)))
show("two fns", cfunction("double f(double a) { return a; } double g(double a) { return a; }"))
show("static helper", cfunction("static double h(double a) { return a * 2; } double f(double a) { return h(a); }")(2))
show("helper same prefix", cfunction("static double ff(const double *a) { return a[0]; } double f(double a) { return ff(&a); }")(2))
show("proto first", cfunction("double f(const double *b); double f(const double *a) { return a[0]; }")(a = 2))
show("comment decoy", cfunction("/* double f(double *a) { */ double f(const double *a) { return a[0]; }")(a = 2))
show("string decoy", cfunction("static const char *s = \"int f(double *z) {\"; double f(const double *a) { return a[0]; }")(a = 2))
show("line-comment in sig", cfunction("double f(const double *a, // first\n int n) { return a[0] + n; }")(a = 2, n = 1L))
show("macro const", {x <- c(1, 2); g <- cfunction("#define P double *\n void f(const P a) { a[0] = 99; }"); g(a = x); x})
show("ifdef decoy", {x <- c(1, 2); g <- cfunction("#if 0\nvoid f(const double *a) {\n#else\nvoid f(double *a) {\n#endif\n a[0] = 99; }"); r <- g(a = x); x})
show("cast away const", {x <- c(1, 2); g <- cfunction("void f(const double *a) { ((double *) a)[0] = 99; }"); g(a = x); x})
show("no fn", cfunction("int x = 1;"))
show("only static", cfunction("static double f(double a) { return a; }"))
show("include", cfunction("#include <stdio.h>\ndouble f(double a) { return a; }"))
show("syntax", tryCatch(cfunction("double f(double a) { return a +; }"), error = function(e) unlist(e[c("line", "column")])))
show("code vec", cfunction(c("a", "b"))); show("code NA", cfunction(NA_character_)); show("code num", cfunction(1)); show("code empty", cfunction(""))
show("code latin1", cfunction(iconv("double f(double a) { /* é */ return a; }", "UTF-8", "latin1"))(1))
show("code utf8 ident", cfunction("double f(double é) { return é; }"))
# R-reserved parameter names
for (nm in c("function", "TRUE", "next", "repeat", "in", "NULL", "NA", "Inf", "T", "zumir_cfun_invoke", "list", "_x", "x_", "x1")) show(paste("param", nm), do.call(cfunction(sprintf("double f(double %s) { return %s + 1; }", nm, nm)), stats::setNames(list(1), nm)))
# out forms
s2 <- "void f(int n, const double *x, double *y, double *z) { for (int i = 0; i < n; i++) { y[i] = x[i]; z[i] = -x[i]; } }"
for (o in list(c("y", "z"), c("z", "y"), "y", c(y = 3, z = 3), list("y", z = 3), list("y", "z"), list(y = 3, "z"), NULL, character(), c(a = "y", b = "z"), c(y = 2.5, z = 3), c(y = NA, z = 3), c(y = NA_real_, z = 3), c(y = -1, z = 3), c(y = Inf, z = 3), c(y = 2^31, z = 3), c(y = 2147483647, z = 3), c(y = 3, y = 3, z = 3), c("y", "y", "z"), c(y = 3, 3), c("y", NA), factor(c("y", "z")), c(x = 3, y = 3, z = 3), c("n", "y", "z"), list(y = 3:4, "z"), list(y = "3", "z"), c(y = TRUE, z = TRUE), list("y", z = NA), list("y", z = 2.5), c("y", "z", "w"))) {
  show(paste("out=", paste(deparse(o), collapse = "")), { g <- cfunction(s2, out = o, n = "n"); g(x = c(1, 2, 3)) })
}
for (n in list("n", "x", "y", "q", NA_character_, c("n", "n"), 1, TRUE, "", factor("n"), NULL)) show(paste("n=", deparse(n)), { g <- cfunction(s2, out = c("y", "z"), n = n); g(x = c(1, 2, 3)) })
