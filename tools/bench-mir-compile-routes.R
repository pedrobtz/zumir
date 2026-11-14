# Compile time of mir_compile()'s two routes (design §4.6, D17), for
# .agents/benchmarks.md. Run from the package root with the package
# installed: Rscript tools/bench-mir-compile-routes.R [n]. Each of n random
# functions from the tests' generator is lowered once, then built by each
# route on each tier; the times are of rf_context() alone (MIR's scanner or
# the emitter, then linking), median of 5 rounds.
library(zumir)
ns <- asNamespace("zumir")
for (f in list.files("tests/testthat", "^helper-(mir-compile|child)", full.names = TRUE)) {
  sys.source(f, environment())
}
n <- as.integer(commandArgs(TRUE)[1])
if (is.na(n)) n <- 200L
set.seed(20261114)
progs <- list()
while (length(progs) < n) {
  p <- tryCatch(ns$rf_compile(rf_gen_function()), error = function(e) NULL)
  if (!is.null(p)) progs[[length(progs) + 1L]] <- p
}
lines <- vapply(progs, function(p) length(strsplit(p$text, "\n")[[1]]), 1L)
time_route <- function(route, tier) {
  rounds <- vapply(1:5, function(r) {
    gc()
    system.time(for (p in progs) ns$rf_context(p, tier, route))[["elapsed"]]
  }, 0)
  1000 * stats::median(rounds) / length(progs)
}
tiers <- if (zumir_tier() == "native") c("native", "interp") else zumir_tier()
cat(sprintf("%d functions, median %d lines of MIR text (max %d); R %s, %s\n",
  n, as.integer(stats::median(lines)), max(lines), getRversion(), R.version$platform))
for (tier in tiers) {
  e <- time_route("emitter", tier)
  t <- time_route("text", tier)
  cat(sprintf("%-6s emitter %.2f ms, text %.2f ms per function (emitter/text %.2f)\n",
    tier, e, t, e / t))
}
