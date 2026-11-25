# zupmxode's models through zumir: the IR zupmxode compiles for three
# models (fixtures/zpx-models.R, from tools/zpx-fixture.R), lowered by a
# copy of zupmxode's lowering (src/zpx_lower.c) on each tier, against a
# copy of zupmxode's reference evaluator of the same IR (src/zpx_ir.c).

zpx_models <- function() {
  source(test_path("fixtures", "zpx-models.R"), local = TRUE)$value
}

zpx_tiers <- function() {
  switch(
    zumir::zumir_tier(),
    native = c("native", "interp"),
    interp = "interp",
    character()
  )
}

zpx_dims <- function(m) {
  h <- m$code
  # The header (6 ints), then nreg, nout, ninsn for each of the 5 blocks.
  list(ny = h[3], npar = h[4], nwork = h[5], nout = h[8 + 3 * (0:4)])
}

zpx_ops <- function(m) {
  matrix(m$code[-(1:21)], ncol = 5, byrow = TRUE)[, 1]
}

# n columns of inputs: mostly values a model sees, with zeros, negatives,
# tiny and huge magnitudes, signed zeros and non-finite values mixed in.
zpx_values <- function(rows, n) {
  k <- rows * n
  x <- rnorm(k)
  pick <- sample(6L, k, replace = TRUE, prob = c(60, 10, 10, 10, 5, 5))
  x[pick == 2] <- 0
  x[pick == 3] <- -abs(x[pick == 3]) * 10
  x[pick == 4] <- sign(x[pick == 4]) * 10^runif(sum(pick == 4), -300, 300)
  x[pick == 5] <- sample(
    c(-0, Inf, -Inf, NaN, NA),
    sum(pick == 5),
    replace = TRUE
  )
  x[pick == 6] <- runif(sum(pick == 6), 0, 48)
  matrix(x, rows, n)
}

# Bitwise where neither is NaN; NaN at the same places, payload aside (C
# may evaluate a commutative operation's operands in either order).
expect_same_bits <- function(object, expected, info) {
  nan <- is.nan(expected)
  expect_identical(is.nan(object), nan, info = info)
  expect_identical(object[!nan], expected[!nan], num.eq = FALSE, info = info)
}

test_that("the fixture is zupmxode's IR, and it branches, calls and loads time", {
  models <- zpx_models()
  expect_named(models, c("lib_PK_2cmt_des", "lib_PK_2cmt_tdcl_des", "lag_emax"))
  for (nm in names(models)) {
    m <- models[[nm]]
    expect_identical(m$code[1:2], c(0x5a50584fL, 1L), info = nm)
    expect_length(m$states, m$code[3])
  }
  # What the three cover between them: CALL1, CALL2, LOADT, GT, AND, JMP, JMPZ.
  ops <- unique(unlist(lapply(models, zpx_ops)))
  expect_true(all(c(2L, 20L, 21L, 32L, 36L, 40L, 41L) %in% ops))
})

test_that("each block of zupmxode's models equals its reference evaluator, bitwise, on every tier", {
  skip_if(length(zpx_tiers()) == 0, "no tier runs here")
  withr::local_seed(20261125)
  models <- zpx_models()
  n <- 400L
  for (nm in names(models)) {
    m <- models[[nm]]
    d <- zpx_dims(m)
    t <- c(0, -0, Inf, NaN, runif(n - 4L, 0, 48))
    y <- zpx_values(d$ny, n)
    par <- zpx_values(d$npar, n)
    for (tier in zpx_tiers()) {
      lowered <- zumirconsumer:::zc_zpx_compile(m, tier)
      expect_identical(
        lowered$status,
        0L,
        info = paste(nm, tier, lowered$message)
      )
      # Each block alone, on arbitrary work; then the blocks in zupmxode's
      # order, each on the work the one before left.
      work <- zpx_values(d$nwork, n)
      chained <- work
      for (b in 0:4) {
        info <- paste(nm, tier, "block", b)
        expect_same_bits(
          zumirconsumer:::zc_zpx_run(lowered$model, b, t, y, par, work),
          zumirconsumer:::zc_zpx_eval(m, b, t, y, par, work),
          info
        )
        got <- zumirconsumer:::zc_zpx_run(lowered$model, b, t, y, par, chained)
        want <- zumirconsumer:::zc_zpx_eval(m, b, t, y, par, chained)
        expect_same_bits(got, want, paste(info, "chained"))
        chained <- want[d$nout[b + 1L] + seq_len(d$nwork), , drop = FALSE]
      }
    }
  }
})

test_that("the two-compartment model's right-hand side is the model's, as R computes it", {
  skip_if(length(zpx_tiers()) == 0, "no tier runs here")
  withr::local_seed(1)
  m <- zpx_models()$lib_PK_2cmt_des
  d <- zpx_dims(m)
  n <- 50L
  t <- runif(n, 0, 24)
  y <- matrix(runif(d$ny * n, 0, 100), d$ny)
  par <- matrix(rnorm(d$npar * n), d$npar)
  rownames(par) <- m$params
  lowered <- zumirconsumer:::zc_zpx_compile(m, zpx_tiers()[1])
  work <- matrix(0, d$nwork, n)
  for (b in 0:1) {
    work <- zumirconsumer:::zc_zpx_run(lowered$model, b, t, y, par, work)
    work <- work[d$nout[b + 1L] + seq_len(d$nwork), , drop = FALSE]
  }
  rhs <- zumirconsumer:::zc_zpx_run(lowered$model, 2L, t, y, par, work)[
    seq_len(d$ny),
  ]
  ka <- exp(par["lka", ])
  kel <- exp(par["lcl", ]) / exp(par["lvc", ])
  k12 <- exp(par["lq", ]) / exp(par["lvc", ])
  k21 <- exp(par["lq", ]) / exp(par["lvp", ])
  want <- rbind(
    -ka * y[1, ],
    ka * y[1, ] - kel * y[2, ] - k12 * y[2, ] + k21 * y[3, ],
    k12 * y[2, ] - k21 * y[3, ]
  )
  expect_equal(rhs, want, tolerance = 1e-13)
})
