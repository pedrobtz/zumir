source("common.R")
tier <- commandArgs(TRUE)[1]
loop <- function(n) { s <- 0; i <- 0; while (i < n) { s <- s + i; i <- i + 1 }; s }
f <- mir_compile(loop, tier = tier)
show("small", f(10))
lim <- function(expr, elapsed = Inf, cpu = Inf) { setTimeLimit(elapsed = elapsed, cpu = cpu, transient = TRUE); on.exit(setTimeLimit(elapsed = Inf, cpu = Inf)); expr }
cond <- function(expr) tryCatch({ expr; "finished" }, error = function(e) list(kind = "error", cls = class(e), msg = conditionMessage(e), call = deparse(conditionCall(e))[1]),
  interrupt = function(e) list(kind = "interrupt", cls = class(e)))
# nested loops, for loop, repeat
g <- mir_compile(function(n) { s <- 0; for (i in 1:n) { for (j in 1:10) { s <- s + 1 } }; s }, tier = tier)
a <- cond(lim(g(1e9), elapsed = 0.5)); show("nested for under time limit", a$kind)
h <- mir_compile(function(n) { s <- 0; while (TRUE) { s <- s + 1; if (s > n) break }; s }, tier = tier)
a <- cond(lim(h(1e12), elapsed = 0.5)); show("repeat under time limit", a$kind)
show("h after", try_(h(3)))
# warnings before the interrupt: what does the caller see?
w <- mir_compile(function(n) { s <- sqrt(-1); i <- 0; while (i < n) { i <- i + 1 }; s }, tier = tier)
a <- withCallingHandlers(cond(lim(w(1e12), elapsed = 0.5)), warning = function(w) { cat("  [warning seen:", conditionMessage(w), "]\n"); invokeRestart("muffleWarning") }); show("warn then limit", a$kind)
a <- withCallingHandlers(try_(w(5)), warning = function(w) { cat("  [warning seen:", conditionMessage(w), "]\n"); invokeRestart("muffleWarning") }); show("w(5) afterwards (one warning expected)", a)
# straddle the poll interval: loops of 65535, 65536, 65537 iterations, with a time limit already expired
for (n in c(65535, 65536, 65537, 131072)) { a <- cond(lim(f(n), elapsed = 1e-9)); show(paste("expired limit, n =", n), if (is.list(a)) a$kind else a) }
# time limit caught silently? stderr must stay clean inside try(silent = TRUE)
r <- try(lim(f(1e12), elapsed = 0.3), silent = TRUE); cat("try class:", class(r), "\n")
# options(error=) must not run for a caught time limit
options(error = function() cat("  [options(error) handler RAN]\n")); r <- cond(lim(f(1e12), elapsed = 0.3)); options(error = NULL); show("with options(error)", r$kind)
# on.exit / finally around the compiled call
r <- tryCatch(lim(f(1e12), elapsed = 0.3), error = function(e) "caught", finally = cat("  [finally ran]\n")); show("finally", r)
# calling handler sees the condition once
k <- 0; r <- tryCatch(withCallingHandlers(lim(f(1e12), elapsed = 0.3), error = function(e) k <<- k + 1), error = function(e) "caught"); show("calling handler count", k)
# an interrupt while a time limit is also set; then both cleared
show("limits after", try_(f(1e6)))
cat("END\n")
