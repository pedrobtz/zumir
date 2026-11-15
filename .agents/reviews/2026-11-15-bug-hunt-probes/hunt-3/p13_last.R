source("common.R")
# 1. long identifier in a diagnostic: is the message cut, are line/column kept?
for (n in c(900, 1100, 5000)) {
  id <- strrep("q", n)
  e <- tryCatch(cfunction(sprintf("double f(double x) {\n return x + %s;\n}\ndouble g(double x) { return zz; }", id)), error = function(e) e)
  cat(sprintf("ident %d: class %s line %s col %s nchar(msg) %d; mentions zz: %s; lines in msg: %d\n", n, class(e)[1], e$line, e$column, nchar(conditionMessage(e)), grepl("zz", conditionMessage(e)), length(strsplit(conditionMessage(e), "\n")[[1]])))
}
# 2. nested call of the SAME interpreted function from a finalizer during its own poll
for (tier in c("native", "interp")) {
  f <- mir_compile(function(n) { s <- 0; i <- 0; while (i < n) { s <- s + i; i <- i + 1 }; s }, tier = tier)
  want <- f(3e5); inner <- NULL; count <- 0
  chain <- function() { e <- new.env(); reg.finalizer(e, function(e) { count <<- count + 1; inner <<- c(inner, f(7e4)); chain() }); NULL }
  chain(); gctorture(TRUE); r <- f(3e5); gctorture(FALSE)
  cat(tier, "nested same fn: outer ok", identical(r, want), "inner ok", all(inner == f(7e4)), "finalizers", count, "\n")
}
# 3. interp: OOM on first call, then reuse (ASan watches)
for (kind in c("mirf", "cfun", "rf")) {
  f <- mk(kind, "interp"); .Call(zumir:::zumir_test_fail_alloc, 1)
  r1 <- tryCatch(f(1, 2), error = function(e) class(e)[1]); .Call(zumir:::zumir_test_fail_alloc, 0)
  cat(kind, "interp first-call OOM:", r1, "; then", f(1, 2), f(5, 6), "\n"); rm(f); gc()
}
cat("live", live(), "\n")
# 4. wrapper-name collision
t2 <- "m: module\n export _zumir_call\n_zumir_call: func d, d:a\n ret a\n endfunc\n endmodule"
chk("func named _zumir_call", mir_function(t2, "_zumir_call")(5))
gc(); cat("live", live(), "\n")
