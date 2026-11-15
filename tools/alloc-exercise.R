# The workload for alloc-failure.yaml, which runs it once per injected
# allocation failure. With one allocation failed, MIR must report it through
# the trap ("Not enough memory", "no memory"), which reaches R as a
# zumir_error and is printed here, or R must raise its own allocation error;
# the process must not die. ZUMIR_ALLOC_EXERCISE picks the section, one job
# each, since the sweep is capped by allocation count:
#   trap       each invalid-module kind through the error trap (Stage 3)
#   mir_compile  mir_compile() through the emitter (Stage 7): the .Call
#                bridge reads R's values, allocates with R_alloc and builds a
#                MIR module, then the function runs
#   cfunction    cfunction(): the prelude, c2mir, the preprocessed source
#                the signature is read from, the wrapper, then a call
#   mir_function mir_function(): MIR's scanner, the wrapper, then a call
#   interp_call  each compiler on the interpreter, then its first call,
#                which allocates nothing since the code is prepared at link
library(zumir)
section <- Sys.getenv("ZUMIR_ALLOC_EXERCISE", "trap")
if (section == "trap") {
  for (kind in c("syntax", "operand", "import")) {
    tryCatch(
      zumir:::test_invalid_module(kind),
      zumir_error = function(e) cat("zumir_error:", conditionMessage(e), "\n")
    )
  }
} else if (section == "mir_compile" && zumir_tier() != "none") {
  tryCatch(
    {
      g <- mir_compile(function(x) {
        s <- 0
        for (i in seq_along(x)) {
          if (x[i] > 2) {
            next
          }
          s <- s + sqrt(x[i])
        }
        s
      })
      # The answer, when it comes, is R's.
      stopifnot(identical(g(c(1, 4, 9, 2)), 1 + sqrt(2)))
      cat("mir_compile: ok\n")
    },
    zumir_error = function(e) cat("zumir_error:", conditionMessage(e), "\n")
  )
} else if (section == "cfunction" && zumir_tier() != "none") {
  tryCatch(
    {
      f <- cfunction(
        "double dot(int n, const double *x, const double *y) {
           double s = 0;
           for (int i = 0; i < n; i++) s += x[i] * y[i];
           return s;
         }",
        n = "n"
      )
      stopifnot(identical(f(x = c(1, 2, 3), y = c(4, 5, 6)), 32))
      cat("cfunction: ok\n")
    },
    zumir_error = function(e) cat("zumir_error:", conditionMessage(e), "\n")
  )
} else if (section == "mir_function" && zumir_tier() != "none") {
  tryCatch(
    {
      f <- mir_function(
        "m: module\nexport add\nadd: func d, d:a, d:b\nlocal d:r\ndadd r, a, b\nret r\nendfunc\nendmodule\n",
        "add"
      )
      stopifnot(identical(f(1, 2), 3))
      cat("mir_function: ok\n")
    },
    zumir_error = function(e) cat("zumir_error:", conditionMessage(e), "\n")
  )
} else if (section == "interp_call" && zumir_tier() != "none") {
  for (make in list(
    function() mir_compile(function(x) sqrt(x) + 1, tier = "interp"),
    function() {
      cfunction("double f(double x) { return sin(x) + 1; }", tier = "interp")
    }
  )) {
    tryCatch(
      {
        f <- make()
        stopifnot(is.numeric(f(2)))
        cat("interp_call: ok\n")
      },
      zumir_error = function(e) cat("zumir_error:", conditionMessage(e), "\n")
    )
  }
}
cat("done\n")
