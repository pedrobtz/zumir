# The workload for alloc-failure.yaml: each invalid-module kind through the
# error trap, once. With one allocation failed, MIR must report it through
# the trap ("Not enough memory", "no memory"), which reaches R as a
# zumir_error and is printed here, or R must raise its own allocation error;
# the process must not die.
library(zumir)
for (kind in c("syntax", "operand", "import")) {
  tryCatch(
    zumir:::test_invalid_module(kind),
    zumir_error = function(e) cat("zumir_error:", conditionMessage(e), "\n")
  )
}
cat("done\n")
