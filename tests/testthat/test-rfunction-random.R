# Random functions in rfunction()'s subset against R itself (design §4.6,
# D16), and native against the interpreter. Each function k is drawn from
# seed + k; a difference prints the seed and saves the function and its
# arguments under tempdir(). ZUMIR_RF_SEED replays a run.

test_that("random functions return what R returns", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  base <- rf_random_seed()
  local_seed(base)
  compiled <- 0L
  for (k in seq_len(rf_random_count())) {
    seed <- base + k
    set.seed(seed)
    f <- rf_gen_function()
    g <- tryCatch(rfunction(f), zumir_compile_error = function(e) NULL)
    if (is.null(g)) {
      next
    }
    compiled <- compiled + 1L
    for (j in 1:3) {
      args <- rf_gen_args()
      setTimeLimit(elapsed = 30, transient = TRUE)
      d <- compare_with_r(f, g, args, na_nan = "any")
      setTimeLimit()
      if (!is.null(d)) {
        path <- rf_save_reproducer(seed, f, args, d)
        fail(sprintf(
          "seed %d (ZUMIR_RF_SEED=%d): %s; saved in %s\nf: %s\nargs: %s",
          seed,
          base,
          d,
          path,
          rf_show(f),
          rf_show(args)
        ))
        return()
      }
    }
  }
  # Most draws are in the subset: a generator that drifts out of it would
  # leave the test checking nothing.
  expect_gt(compiled, rf_random_count() * 0.8)
})

test_that("native and interpreted random functions agree", {
  skip_if(zumir_tier() != "native", "the native tier does not run here")
  base <- rf_random_seed()
  local_seed(base)
  for (k in seq_len(rf_random_count() %/% 2L)) {
    seed <- base + k
    set.seed(seed)
    f <- rf_gen_function(na = FALSE)
    n <- tryCatch(
      rfunction(f, tier = "native"),
      zumir_compile_error = function(e) NULL
    )
    if (is.null(n)) {
      next
    }
    i <- rfunction(f, tier = "interp")
    args <- rf_gen_args(na = FALSE)
    a <- run_capture(n, args)
    b <- run_capture(i, args)
    # Bit for bit but for NaN payloads. No NA is ever made here (na =
    # FALSE): where an NA and a NaN meet, which comes out is the operand
    # order the compiler chose (an unoptimised gcc build of the interpreter
    # differs from generated code), R leaves it open (?NA), and is.nan()
    # can turn that into a different answer.
    if (!identical(a, b, num.eq = FALSE) && !same_capture(a, b)) {
      path <- rf_save_reproducer(seed, f, args, "native and interp differ")
      # The runner's tempdir() is gone after the job, so the message
      # carries the reproducer and both results.
      fail(sprintf(
        "seed %d (ZUMIR_RF_SEED=%d): native and interp differ; saved in %s\nf: %s\nargs: %s\nnative: %s\ninterp: %s",
        seed,
        base,
        path,
        rf_show(f),
        rf_show(args),
        rf_show(a),
        rf_show(b)
      ))
      return()
    }
  }
  succeed()
})
