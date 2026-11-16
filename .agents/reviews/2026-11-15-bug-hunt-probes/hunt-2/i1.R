source("common.R")
tier <- commandArgs(TRUE)[1]
loop <- function(n) { s <- 0; i <- 0; while (i < n) { s <- s + i; i <- i + 1 }; s }
f <- mir_compile(loop, tier = tier)
show("small", f(10))
lim <- function(expr, elapsed = Inf, cpu = Inf) { setTimeLimit(elapsed = elapsed, cpu = cpu, transient = TRUE); on.exit(setTimeLimit(elapsed = Inf, cpu = Inf)); expr }
cond <- function(expr) tryCatch({ expr; "finished" }, error = function(e) list(kind = "error", cls = class(e), msg = conditionMessage(e), call = deparse(conditionCall(e))[1]),
  interrupt = function(e) list(kind = "interrupt", cls = class(e)))
# elapsed time limit: compiled vs R
t0 <- proc.time()[[3]]; a <- cond(lim(f(1e12), elapsed = 0.5)); cat("took", round(proc.time()[[3]] - t0, 2), "\n")
show("compiled under setTimeLimit", a)
b <- cond(lim(loop(1e12), elapsed = 0.5))
show("R under setTimeLimit", b)
show("next call after time limit", try_(f(1000)))
show("identical to R", identical(f(1000), loop(1000)))
# cpu limit
a <- cond(lim(f(1e12), cpu = 0.5)); show("compiled under cpu limit", a)
show("next call", try_(f(100)))
# SIGINT
pid <- Sys.getpid()
system(sprintf("(sleep 1; kill -INT %d) &", pid)); a <- cond(f(1e12)); show("compiled, SIGINT", a)
show("next call after interrupt", try_(f(1000)))
system(sprintf("(sleep 1; kill -INT %d) &", pid)); b <- cond(loop(1e12)); show("R, SIGINT", b)
# interrupt pending when a short call starts
system(sprintf("(sleep 1; kill -INT %d) &", pid)); r <- cond({ f(1e12) }); r2 <- try_(f(5)); show("call right after", r2)
# nested loops, for loop, repeat
g <- mir_compile(function(n) { s <- 0; for (i in 1:n) { for (j in 1:10) { s <- s + 1 } }; s }, tier = tier)
a <- cond(lim(g(1e9), elapsed = 0.5)); show("nested for under time limit", a$kind)
h <- mir_compile(function(n) { s <- 0; repeat { s <- s + 1; if (s > n) break }; s }, tier = tier)
a <- cond(lim(h(1e12), elapsed = 0.5)); show("repeat under time limit", a$kind)
show("h after", try_(h(3)))
# warnings before the interrupt: what does the caller see?
w <- mir_compile(function(n) { s <- sqrt(-1); i <- 0; while (i < n) { i <- i + 1 }; s }, tier = tier)
setTimeLimit(elapsed = 0.5, transient = TRUE); a <- withCallingHandlers(cond(w(1e12)), warning = function(w) { cat("  [warning seen:", conditionMessage(w), "]\n"); invokeRestart("muffleWarning") }); setTimeLimit(); show("warn then limit", a$kind)
a <- withCallingHandlers(try_(w(5)), warning = function(w) { cat("  [warning seen:", conditionMessage(w), "]\n"); invokeRestart("muffleWarning") }); show("w(5) afterwards (one warning expected)", a)
# straddle the poll interval: loops of 65535, 65536, 65537 iterations, with a time limit already expired
for (n in c(65535, 65536, 65537, 131072)) { a <- cond(lim(f(n), elapsed = 1e-9)); show(paste("expired limit, n =", n), if (is.list(a)) a$kind else a) }
cat("END\n")
