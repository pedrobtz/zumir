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
      d <- compare_with_r(f, g, args)
      setTimeLimit()
      if (!is.null(d)) {
        path <- rf_save_reproducer(seed, f, args, d)
        fail(sprintf(
          "seed %d (ZUMIR_RF_SEED=%d): %s; saved in %s",
          seed,
          base,
          d,
          path
        ))
        return()
      }
    }
  }
  # Most draws are in the subset: a generator that drifts out of it would
  # leave the test checking nothing.
  expect_gt(compiled, rf_random_count() * 0.8)
})

test_that("native and interpreted random functions agree bit for bit", {
  skip_if(zumir_tier() != "native", "the native tier does not run here")
  base <- rf_random_seed()
  local_seed(base)
  for (k in seq_len(rf_random_count() %/% 2L)) {
    seed <- base + k
    set.seed(seed)
    f <- rf_gen_function()
    n <- tryCatch(
      rfunction(f, tier = "native"),
      zumir_compile_error = function(e) NULL
    )
    if (is.null(n)) {
      next
    }
    i <- rfunction(f, tier = "interp")
    args <- rf_gen_args()
    a <- run_capture(n, args)
    b <- run_capture(i, args)
    if (!identical(a, b, num.eq = FALSE)) {
      path <- rf_save_reproducer(seed, f, args, "native and interp differ")
      fail(sprintf(
        "seed %d (ZUMIR_RF_SEED=%d): native and interp differ; saved in %s",
        seed,
        base,
        path
      ))
      return()
    }
  }
  succeed()
})
