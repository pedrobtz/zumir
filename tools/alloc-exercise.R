# The workload for alloc-failure.yaml, which runs it once per injected
# allocation failure. With one allocation failed, MIR must report it through
# the trap ("Not enough memory", "no memory"), which reaches R as a
# zumir_error and is printed here, or R must raise its own allocation error;
# the process must not die. ZUMIR_ALLOC_EXERCISE picks the section, one job
# each, since the sweep is capped by allocation count:
#   trap       each invalid-module kind through the error trap (Stage 3)
#   rfunction  rfunction() through the emitter (Stage 7): the .Call bridge
#              reads R's values, allocates with R_alloc and builds a MIR
#              module, then the function runs
library(zumir)
section <- Sys.getenv("ZUMIR_ALLOC_EXERCISE", "trap")
if (section == "trap") {
  for (kind in c("syntax", "operand", "import")) {
    tryCatch(
      zumir:::test_invalid_module(kind),
      zumir_error = function(e) cat("zumir_error:", conditionMessage(e), "\n")
    )
  }
} else if (section == "rfunction" && zumir_tier() != "none") {
  tryCatch(
    {
      g <- rfunction(function(x) {
        s <- 0
        for (i in seq_along(x)) {
          if (x[i] > 2) next
          s <- s + sqrt(x[i])
        }
        s
      })
      # The answer, when it comes, is R's.
      stopifnot(identical(g(c(1, 4, 9, 2)), 1 + sqrt(2)))
      cat("rfunction: ok\n")
    },
    zumir_error = function(e) cat("zumir_error:", conditionMessage(e), "\n")
  )
}
cat("done\n")
