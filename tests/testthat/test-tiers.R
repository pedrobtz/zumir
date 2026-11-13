# Native code against the interpreter (design §4.4): the spike's three model
# right-hand sides give the same doubles on both tiers, bitwise.

test_that("the three right-hand sides agree bitwise across tiers", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  skip_if(zumir_tier() == "none", "no tier runs here")
  # A fixed seed, restored after (the shared rules' withr::local_seed(), with
  # no withr dependency).
  old <- if (exists(".Random.seed", globalenv())) {
    get(".Random.seed", globalenv())
  }
  on.exit(
    if (is.null(old)) {
      rm(".Random.seed", envir = globalenv())
    } else {
      assign(".Random.seed", old, globalenv())
    }
  )
  set.seed(20261113)
  sizes <- list(c(2, 3), c(3, 6), c(10, 9))
  for (model in 1:3) {
    native <- compile_model(model, "native")
    interp <- compile_model(model, "interp")
    neq <- sizes[[model]][1]
    npar <- sizes[[model]][2]
    for (k in 1:50) {
      y <- runif(neq, 0, 100)
      par <- runif(npar, 0.1, 50)
      a <- context_call_rhs(native, model, y, par)
      b <- context_call_rhs(interp, model, y, par)
      expect_identical(a, b, info = paste("model", model, "draw", k))
    }
    context_free(native)
    context_free(interp)
  }
})

test_that("the tiers agree on non-finite and signed-zero inputs", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  skip_if(zumir_tier() == "none", "no tier runs here")
  native <- compile_model(2, "native")
  interp <- compile_model(2, "interp")
  on.exit({
    context_free(native)
    context_free(interp)
  })
  odd <- c(0, -0, Inf, -Inf, NaN, NA, 5e-324, .Machine$double.xmax)
  for (v in odd) {
    y <- c(v, 1, -v)
    par <- c(1.2, 40, v, 30, 4, 60)
    expect_identical(
      context_call_rhs(native, 2, y, par),
      context_call_rhs(interp, 2, y, par),
      info = format(v)
    )
  }
})
