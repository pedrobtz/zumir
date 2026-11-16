source("common.R")
a <- commandArgs(TRUE); kind <- a[1]; tier <- a[2]; nmax <- as.integer(a[3]); step <- if (length(a) > 3) as.integer(a[4]) else 1L
fa <- function(n) .Call(zumir:::zumir_test_fail_alloc, n)
depth <- function() { r <- tryCatch(zumir:::test_invalid_module("syntax"), error = function(e) e$trap$depth); r }
nofail <- 0; nfail <- 0; bad <- 0; msgs <- character()
ns <- seq(as.integer(Sys.getenv("START", "1")), nmax, by = step)
for (n in ns) {
  fa(n)
  r1 <- tryCatch({ f <- mk(kind, tier); f(1, 2) }, error = function(e) e)
  fa(0)
  if (inherits(r1, "error")) {
    nofail <- 0; nfail <- nfail + 1
    key <- paste(class(r1)[1], conditionMessage(r1))
    msgs <- c(msgs, key)
    if (!inherits(r1, "zumir_error")) { bad <- bad + 1; cat(sprintf("n=%d UNCLASSED <%s> %s\n", n, class(r1)[1], conditionMessage(r1))) }
    rm(r1); if (exists("f")) rm(f); gc()
    if (live() != 0) { bad <- bad + 1; cat(sprintf("n=%d LEAK live=%g\n", n, live())) }
    d <- depth(); if (!identical(d, 0L)) { bad <- bad + 1; cat(sprintf("n=%d trap depth %s\n", n, format(d))) }
    g <- mk(kind, tier); v <- g(1, 2); if (!identical(v, 3)) { bad <- bad + 1; cat(sprintf("n=%d next compile WRONG %s\n", n, format(v))) }
    rm(g); gc()
  } else {
    if (!identical(r1, 3)) { bad <- bad + 1; cat(sprintf("n=%d WRONG RESULT (no error) %s\n", n, format(r1))) }
    rm(f); gc()
    nofail <- nofail + 1
    if (nofail > 5) { cat("no failure from n =", n - 5 * step, "\n"); break }
  }
}
cat("failures injected:", nfail, " bad:", bad, " live:", live(), "\n")
print(table(msgs))
