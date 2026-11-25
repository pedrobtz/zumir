# Compile time against call count for zupmxode's models (design §11,
# roadmap Stage 13), through the consumer's copy of zupmxode's lowering:
# what one model costs to lower on each tier, what one call of its
# right-hand side costs there and in zupmxode's reference evaluator, and
# the calls after which compiling has paid for itself. For
# .agents/benchmarks.md.
#
#   R CMD INSTALL . && R CMD INSTALL tests/consumer && Rscript tools/bench-zpx.R
models <- source(
  "tests/consumer/tests/testthat/fixtures/zpx-models.R",
  local = TRUE
)$value
tiers <- switch(
  zumir::zumir_tier(),
  native = c("native", "interp"),
  interp = "interp",
  stop("no tier runs here")
)
ns <- asNamespace("zumirconsumer")
per <- function(f, reps, k = 1) {
  # Median seconds of one call of f: reps rounds of k calls each (a
  # round must be well above system.time()'s millisecond).
  median(vapply(
    seq_len(reps),
    function(i) {
      system.time(
        for (j in seq_len(k)) {
          f()
        }
      )[["elapsed"]]
    },
    0
  )) /
    k
}
n <- 1e6
rows <- list()
for (nm in names(models)) {
  m <- models[[nm]]
  h <- m$code
  ny <- h[3]
  npar <- h[4]
  nwork <- h[5]
  set.seed(1)
  t <- runif(n, 0, 24)
  y <- matrix(runif(ny * n, 0, 100), ny)
  par <- matrix(rnorm(npar * n, sd = 0.5), npar)
  work <- matrix(0, nwork, n)
  ref <- per(function() ns$zc_zpx_eval(m, 2L, t, y, par, work), 5) / n
  for (tier in tiers) {
    compile <- per(function() ns$zc_zpx_compile(m, tier), 5, k = 200)
    x <- ns$zc_zpx_compile(m, tier)$model
    call <- per(function() ns$zc_zpx_run(x, 2L, t, y, par, work), 5) / n
    rows[[length(rows) + 1L]] <- data.frame(
      model = nm,
      insns = (length(h) - 21L) / 5L,
      tier = tier,
      compile_ms = 1e3 * compile,
      call_ns = 1e9 * call,
      reference_ns = 1e9 * ref,
      break_even = if (call < ref) ceiling(compile / (ref - call)) else NA
    )
  }
}
res <- do.call(rbind, rows)
print(res, digits = 3, row.names = FALSE)
