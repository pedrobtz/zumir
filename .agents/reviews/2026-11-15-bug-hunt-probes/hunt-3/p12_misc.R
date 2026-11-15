source("common.R")
tier <- commandArgs(TRUE)[1]
# 1. vector argument ownership: compiled code must not write the caller's vector
f <- tryCatch(mir_compile(function(x) { x[1] <- 99; x[1] + x[2] }, tier = tier), error = function(e) e)
if (is.function(f)) { x <- c(1, 2, 3); y <- x; r <- f(x); cat("vec arg write: result", r, "caller x:", x, "alias y:", y, "\n") } else cat("vec arg write refused:", conditionMessage(f), "\n")
# vector local returned twice: results independent?
g <- mir_compile(function(n) { y <- numeric(n); i <- 1; while (i <= n) { y[i] <- i; i <- i + 1 }; y }, tier = tier)
a <- g(3); b <- g(3); b2 <- b; a[1] <- 100; cat("vector results independent:", identical(b, c(1,2,3)), identical(g(3), c(1,2,3)), "\n")
# cfunction const input not copied (D24): by design; output copies
h <- cfunction("void fill(double *out, const double *x, int n) { for (int i = 0; i < n; i++) out[i] = 2 * x[i]; }", out = "out", n = "n", tier = tier)
x <- c(1, 2, 3); o <- c(9, 9, 9); o_alias <- o
r <- h(out = o, x = x); cat("cfun out:", r, " caller's o:", o, "\n")
# 2. stdio sink straddle
tf <- tempfile()
for (n in c(0, 1, 1022, 1023, 1024, 1025, 4000, 4090, 4094, 4095, 4096, 4097, 10000)) {
  s <- strrep("a", n); r <- .Call(zumir:::zumir_test_sink, s, tf)
  cat(sprintf("sink n=%5d -> nchar %d; file %d\n", n, nchar(r), file.size(tf)))
}
r <- .Call(zumir:::zumir_test_sink, "x", tf); cat("after big: '", r, "'\n", sep = "")
chk("exit untrapped", .Call(zumir:::zumir_test_exit, strrep("b", 5000), FALSE))
chk("exit trapped", nchar(.Call(zumir:::zumir_test_exit, strrep("b", 5000), TRUE)))
chk("depth", tryCatch(zumir:::test_invalid_module("syntax"), error = function(e) e$trap$depth))
chk("sink %", .Call(zumir:::zumir_test_sink, "%s%n%d", tf))
# 3. gctorture short sequences
gctorture(TRUE)
for (kind in c("mirf", "cfun", "rf")) { f <- mk(kind, tier); r <- f(1, 2); g <- f; rm(f); r2 <- g(3, 4); rm(g); cat(kind, "torture:", r, r2, "\n") }
e1 <- tryCatch(mir_function("garbage", "x", tier = tier), error = function(e) class(e)[1])
e2 <- tryCatch(cfunction("double f(double x) { return y; }", tier = tier), error = function(e) class(e)[1])
e3 <- tryCatch(mir_function(mirtext, "nope", tier = tier), error = function(e) class(e)[1])
r <- h(out = NULL, x = c(1, 2)); 
gctorture(FALSE)
cat("torture errors:", e1, e2, e3, "| h:", r, "\n")
gc(); cat("live", live(), "bytes", zumir_info()$memory$bytes, "\n")
rm(list = c("f", "g", "h")[c("f","g","h") %in% ls()]); gc(); cat("live", live(), "bytes", zumir_info()$memory$bytes, "\n")
# 4. many contexts create/free: counters return to zero
fs <- lapply(1:200, function(i) mk("mirf", tier)); cat("200 live:", live(), "\n"); rm(fs); gc(); cat("after:", live(), zumir_info()$memory$bytes, "\n")
