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
  # Under a memory tool, which runs R 20 to 100 times slower, the quick set.
  tool <- Sys.getenv("R_ACTIONS_WORKFLOW") %in%
    c("valgrind", "gctorture", "sanitizers")
  full <- full_profile() && !tool
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
  ns <- asNamespace("zumir")
  big <- function(x) {
    s <- 0
    for (i in seq_along(x)) {
      s <- s + sqrt(abs(x[i])) * 2 - x[i] %% 3 + exp(x[i] / 100)
    }
    s
  }
  # Errors injected deterministically (a time limit fired at only one of
  # twenty points): at the k-th instruction the lowering emits, for every
  # k, and once more after the module is built in C.
  at <- new.env()
  at$n <- 0L
  at$k <- 0L
  suppressMessages(trace(
    "rf_ins",
    tracer = bquote({
      assign("n", .(at)$n + 1L, envir = .(at))
      if (.(at)$n == .(at)$k) stop("injected")
    }),
    where = ns,
    print = FALSE
  ))
  on.exit(suppressMessages(untrace("rf_ins", where = ns)), add = TRUE)
  mir_compile(big)
  steps <- at$n
  expect_gt(steps, 20L)
  gc()
  base <- .Call(zumir_test_live_arenas)
  raised <- 0L
  for (k in seq_len(steps)) {
    at$n <- 0L
    at$k <- k
    raised <- raised +
      inherits(tryCatch(mir_compile(big), error = identity), "error")
  }
  at$k <- 0L
  expect_identical(raised, steps)
  suppressMessages(trace(
    "rf_context",
    exit = quote(stop(structure(
      class = c("injected_error", "error", "condition"),
      list(message = "injected after the build", call = NULL)
    ))),
    where = ns,
    print = FALSE
  ))
  on.exit(suppressMessages(untrace("rf_context", where = ns)), add = TRUE)
  expect_error(mir_compile(big), class = "injected_error")
  gc()
  expect_lte(.Call(zumir_test_live_arenas), base)
})

test_that("an operand the emitter cannot read is a MIR error, never a guess", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  ns <- asNamespace("zumir")
  prog <- ns$rf_compile(function(x) {
    s <- 0
    for (i in seq_along(x)) {
      s <- s + x[i]
    }
    s
  })
  first <- prog$spec$body[[1]]
  expect_match(first, "mov vp1, i64:0(VP)", fixed = TRUE)
  long_reg <- strrep("r", 70)
  bad <- c(
    "q:0(VP)", # a memory type other than d or i64
    "i64:0 VP", # no parenthesis after the displacement
    "i64:0(VP", # no closing parenthesis
    "i64:0(VP)x", # text after it
    "i64:0(VP, vl1)", # an index with no scale
    sprintf("i64:0(%s)", long_reg), # a register name longer than 63
    "i64:0()", # no base register
    "12x" # an integer with text after it
  )
  for (b in bad) {
    p <- prog
    p$spec$body[[1]] <- sub("i64:0(VP)", b, first, fixed = TRUE)
    for (tier in c("native", "interp")) {
      expect_error(
        ns$rf_context(p, tier, "emitter"),
        class = "zumir_mir_error",
        info = paste(b, tier)
      )
    }
  }
  # Untouched, it builds.
  ctx <- ns$rf_context(prog, "interp", "emitter")
  expect_identical(ns$rf_call(prog, ctx, list(x = c(1, 2))), 3)
})
