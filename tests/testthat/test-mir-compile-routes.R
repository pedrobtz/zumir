# mir_compile()'s two routes to MIR (design §4.6, D17): the emitter, built
# with MIR's API, against the text route, read by MIR's scanner. The text
# route is the oracle: the same module printed by MIR_output, and the same
# results; and both against R.

test_that("both routes build the same module for every construct", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  for (f in rf_construct_cases()) {
    expect_identical(
      rf_module_text(f, "emitter"),
      rf_module_text(f, "text"),
      info = paste(deparse(f), collapse = " ")
    )
  }
})

test_that("both routes build the same module for random functions", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  base <- rf_random_seed()
  local_seed(base)
  for (k in seq_len(rf_random_count())) {
    seed <- base + k
    set.seed(seed)
    f <- rf_gen_function()
    if (inherits(tryCatch(rf_compile(f), error = identity), "error")) {
      next
    }
    a <- rf_module_text(f, "emitter")
    b <- rf_module_text(f, "text")
    if (!identical(a, b)) {
      fail(sprintf(
        "seed %d (ZUMIR_RF_SEED=%d): the modules differ\nf: %s",
        seed,
        base,
        rf_show(f)
      ))
      return()
    }
  }
  succeed()
})

test_that("both routes return what R returns", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  base <- rf_random_seed()
  local_seed(base)
  for (k in seq_len(rf_random_count() %/% 2L)) {
    seed <- base + k
    set.seed(seed)
    f <- rf_gen_function()
    e <- tryCatch(
      rf_make(f, route = "emitter"),
      zumir_compile_error = function(e) NULL
    )
    if (is.null(e)) {
      next
    }
    t <- rf_make(f, route = "text")
    args <- rf_gen_args()
    for (g in list(e, t)) {
      d <- compare_with_r(f, g, args, na_nan = "any")
      if (!is.null(d)) {
        fail(sprintf(
          "seed %d (ZUMIR_RF_SEED=%d): %s\nf: %s\nargs: %s",
          seed,
          base,
          d,
          rf_show(f),
          rf_show(args)
        ))
        return()
      }
    }
  }
  # And the chosen inputs of the construct cases, by both routes.
  v <- c(0, -0, 1, -2.5, 7, Inf, NaN, NA)
  for (f in rf_construct_cases()) {
    shapes <- names(formals(f))
    ge <- rf_make(f, route = "emitter")
    gt <- rf_make(f, route = "text")
    for (a in v) {
      args <- stats::setNames(
        lapply(shapes, function(s) {
          if (s == "x") {
            c(a, 2, 3)
          } else if (s %in% c("i", "n")) {
            2
          } else {
            a
          }
        }),
        shapes
      )
      e <- run_capture(ge, args)
      t <- run_capture(gt, args)
      # The routes agree exactly, a D16 refusal included.
      same <- identical(e, t, num.eq = FALSE) ||
        (inherits(e$value, "error") &&
          identical(conditionMessage(e$value), conditionMessage(t$value)))
      expect(same, paste("routes differ:", rf_show(f), rf_show(args)))
      # Where mir_compile() answers, it answers as R does.
      if (!inherits(e$value, "zumir_runtime_error")) {
        d <- compare_with_r(f, ge, args)
        expect(is.null(d), paste(rf_show(f), rf_show(args), d))
      }
    }
  }
})

test_that("the emitter route is mir_compile()'s default", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  g <- mir_compile(function(a) a + 1)
  expect_identical(attr(g, "route"), "emitter")
  expect_identical(g(1), 2)
  expect_identical(
    attr(rf_make(function(a) a + 1, route = "text"), "route"),
    "text"
  )
})

test_that("each of MIR's allocations failed in the emitter: an error, no arena left", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- function(x) {
    s <- 0
    for (i in seq_along(x)) {
      if (x[i] > 2) {
        next
      }
      s <- s + sqrt(x[i])
    }
    s
  }
  prog <- rf_compile(f)
  on.exit(.Call(zumir_test_fail_alloc, 0))
  gc()
  base <- .Call(zumir_test_live_arenas)
  failed <- 0L
  # About 2,400 allocations build and link this function. The full profile
  # fails each in turn; the quick one the first 60 (context and module
  # set-up) and every 37th after, still until the build succeeds.
  full <- identical(Sys.getenv("R_ACTIONS_PROFILE"), "full")
  points <- if (full) 1:20000 else c(1:60, seq(61, 20000, by = 37))
  for (n in points) {
    .Call(zumir_test_fail_alloc, n)
    res <- tryCatch(
      rf_context(prog, zumir_tier(), "emitter"),
      zumir_error = identity
    )
    .Call(zumir_test_fail_alloc, 0)
    if (!inherits(res, "zumir_error")) {
      break
    }
    expect_s3_class(res, "zumir_mir_error")
    failed <- failed + 1L
  }
  # The sweep reached the build's last allocation: it then succeeded.
  expect_s3_class(res, "zumir_context")
  expect_gt(failed, 60L)
  rm(res)
  gc()
  # At most the baseline: contexts other tests left may be collected
  # meanwhile, and a leak here keeps dozens.
  expect_lte(.Call(zumir_test_live_arenas), base)
})

test_that("an R error while mir_compile() lowers leaves no context behind", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  gc()
  base <- .Call(zumir_test_live_arenas)
  big <- function(x) {
    s <- 0
    for (i in seq_along(x)) {
      s <- s + sqrt(abs(x[i])) * 2 - x[i] %% 3 + exp(x[i] / 100)
    }
    s
  }
  # An error at every point of compile and build: a time limit that
  # expires at once, each time a little later.
  for (k in 1:20) {
    run <- function() {
      setTimeLimit(elapsed = k / 1000, transient = TRUE)
      on.exit(setTimeLimit())
      mir_compile(big)
    }
    tryCatch(run(), error = function(e) NULL)
  }
  gc()
  expect_lte(.Call(zumir_test_live_arenas), base)
})
