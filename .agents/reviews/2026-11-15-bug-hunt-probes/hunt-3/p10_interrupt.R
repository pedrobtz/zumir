source("common.R")
tier <- commandArgs(TRUE)[1]
depth <- function() tryCatch(zumir:::test_invalid_module("syntax"), error = function(e) e$trap$depth)
loop <- mir_compile(function(x, n) { y <- sqrt(x); s <- 0; i <- 0; while (i < n) { s <- s + i; i <- i + 1 }; s + y }, tier = tier)
want <- loop(4, 1e5)
pid <- Sys.getpid()
for (k in 1:3) {
  system(sprintf("(sleep 0.4; kill -INT %d) &", pid))
  r <- tryCatch(loop(-1, 1e13), interrupt = function(c) "INTERRUPT condition", error = function(e) paste("ERR", conditionMessage(e)), warning = function(w) paste("WARN", conditionMessage(w)))
  cat(k, "->", r, "\n")
  w <- 0; v <- withCallingHandlers(loop(4, 1e5), warning = function(c) { w <<- w + 1; invokeRestart("muffleWarning") })
  cat("  next call identical:", identical(v, want), "warnings:", w, "depth:", depth(), "\n")
}
# interrupt during compile of a big cfunction
src <- paste0("double f(double x) { double s = 0; ", paste(sprintf("s += x * %d.0 + sin(x + %d);", 1:3000, 1:3000), collapse = " "), " return s; }")
system(sprintf("(sleep 0.3; kill -INT %d) &", pid))
t0 <- Sys.time()
r <- tryCatch({ g <- cfunction(src, tier = tier); "compiled" }, interrupt = function(c) "INTERRUPT during compile", error = function(e) paste("ERR", conditionMessage(e)))
cat("compile:", r, "took", format(Sys.time() - t0), "\n")
r2 <- tryCatch({ Sys.sleep(1); "no pending interrupt" }, interrupt = function(c) "INTERRUPT delivered after compile")
cat(r2, "\n")
gc(); cat("live", live(), "; fresh:", mk("cfun", tier)(1, 2), "\n")
