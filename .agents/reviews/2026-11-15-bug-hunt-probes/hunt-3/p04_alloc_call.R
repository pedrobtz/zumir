source("common.R")
a <- commandArgs(TRUE); kind <- a[1]; tier <- a[2]; nmax <- as.integer(a[3])
fa <- function(n) .Call(zumir:::zumir_test_fail_alloc, n)
cls <- function(e) paste(class(e), collapse="/")
nofail <- 0
for (n in seq_len(nmax)) {
  f <- mk(kind, tier)
  l1 <- live()
  fa(n)
  r1 <- tryCatch(f(1, 2), error = function(e) e)
  fa(0)
  if (inherits(r1, "error")) {
    nofail <- 0
    cat(sprintf("n=%d first call: ERR <%s> %s\n", n, cls(r1), conditionMessage(r1)))
    cat("   state:", paste(names(unlist(zumir:::context_state(environment(f)$ctx %||% NULL))), collapse=","), "\n")
    r2 <- tryCatch(f(1, 2), error = function(e) e)
    cat("   second call same closure:", if (inherits(r2,"error")) sprintf("ERR <%s> %s", cls(r2), conditionMessage(r2)) else format(r2), "\n")
    r3 <- tryCatch(f(10, 20), error = function(e) e)
    cat("   third call same closure:", if (inherits(r3,"error")) sprintf("ERR <%s> %s", cls(r3), conditionMessage(r3)) else format(r3), "\n")
    cat("   trap depth probe / fresh compile:", format(tryCatch(mk(kind, tier)(1,2), error=function(e) conditionMessage(e))), "\n")
  } else {
    if (!identical(r1, 3)) cat(sprintf("n=%d WRONG RESULT %s\n", n, format(r1)))
    nofail <- nofail + 1
    if (nofail > 5) { cat("no failure from n =", n - 5, "\n"); break }
  }
  rm(f); gc()
  if (live() != 0) cat(sprintf("n=%d LEAK live=%g\n", n, live()))
}
cat("END live", live(), "\n")
