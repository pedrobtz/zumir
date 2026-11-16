source("common.R"); options(OutDec = ",", scipen = -5, digits = 3, warn = 2)
f <- function(x, n) { y <- numeric(n * 1.0); s <- 0.1 + 1e5 + 100000L + 123456.7891234; for (i in seq_len(n)) y[i] <- x[i] * 1e-5 + s + 1e15 + 0.30000000000000004 + log(x[i]); y }
g <- mir_compile(f); g2 <- zumir:::rf_make(f, route = "text")
print(identical(g(c(1, 2, 3), 3), f(c(1, 2, 3), 3), num.eq = FALSE)); print(identical(g2(c(1, 2, 3), 3), f(c(1, 2, 3), 3), num.eq = FALSE))
print(tryCatch(g(c(-1, 2, 3), 3), error = function(e) class(e))); print(tryCatch(f(c(-1, 2, 3), 3), error = function(e) class(e)))
print(tryCatch(g(c(1, 2, 3), 2.5), error = function(e) conditionMessage(e)))
print(tryCatch(g(c(1, 2, 3), 1e5 + 0.5), error = function(e) conditionMessage(e)))
print(g); 
cf <- cfunction("void f(int n, const double *x, double *y) { for (int i = 0; i < n; i++) y[i] = x[i] * 1.5e-3; }", out = c(y = 100000), n = "n"); print(length(cf(x = c(1, 2))))
print(tryCatch(cf(x = 1, n = 100000), error = function(e) conditionMessage(e)))
cat(grep("mov z", strsplit(attr(mir_compile(function(x) { s <- x; for (i in 100000L:100001L) s <- s + i; s }), "mir"), "\n")[[1]], value = TRUE), sep = "\n")
