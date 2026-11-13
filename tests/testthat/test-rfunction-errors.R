# rfunction()'s run-time errors and warnings (design §4.6, D16, D23): R's
# own message where R errors, and an error, never a different answer, where
# R would answer in a way the compiled code does not reproduce.

test_that("if (NA), while (NA) and if (NaN) raise R's errors", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f_if <- function(a) {
    s <- 0
    if (a > 0) {
      s <- 1
    }
    s
  }
  f_while <- function(a) {
    s <- 0
    while (a > s) {
      s <- s + 1
    }
    s
  }
  f_nan <- function(a) {
    s <- 0
    if (a) {
      s <- 1
    }
    s
  }
  for (f in list(f_if, f_while)) {
    expect_same_as_r(f, a = NA_real_)
    expect_error(rfunction(f)(NA_real_), class = "zumir_runtime_error")
  }
  expect_same_as_r(f_nan, a = NaN)
  expect_same_as_r(f_nan, a = NA_real_)
  expect_error(rfunction(f_nan)(NaN), class = "zumir_runtime_error")
  expect_identical(rfunction(f_nan)(2), 1)
})

test_that("log(-1), sqrt(-1) and %%'s loss of accuracy warn once per call", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  count_warnings <- function(g, ...) {
    n <- 0L
    withCallingHandlers(
      g(...),
      warning = function(w) {
        n <<- n + 1L
        invokeRestart("muffleWarning")
      }
    )
    n
  }
  loop <- function(body) {
    eval(bquote(function(a) {
      s <- 0
      for (i in seq_len(10)) {
        s <- s + .(body)
      }
      s
    }))
  }
  for (body in list(quote(log(a)), quote(sqrt(a)))) {
    f <- loop(body)
    expect_same_as_r(f, a = -1)
    expect_identical(count_warnings(rfunction(f), -1), 1L)
    expect_identical(count_warnings(rfunction(f), 1), 0L)
  }
  f <- loop(quote(a %% 3))
  expect_same_as_r(f, a = 1e300)
  expect_identical(count_warnings(rfunction(f), 1e300), 1L)
  expect_identical(count_warnings(rfunction(f), 1e10), 0L)
  # Both kinds in one call give one of each.
  g <- function(a, b) log(a) + b %% 3
  expect_same_as_r(g, a = -1, b = 1e300)
  expect_identical(count_warnings(rfunction(g), -1, 1e300), 2L)
})

test_that("an index is used only where R reads the element", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- function(x, i) x[i]
  g <- rfunction(f)
  x <- c(10, 20, 30)
  for (i in c(1, 3)) {
    expect_same_as_r(f, x = x, i = i)
  }
  # R gives numeric(0), NA, NA and x[1]: none is the element at i.
  for (i in c(0, 4, NA, 1.5, -1, Inf, NaN)) {
    expect_error(g(x, i), class = "zumir_runtime_error")
  }
  h <- function(n, i) {
    y <- numeric(n)
    y[i] <- 1
    y
  }
  expect_same_as_r(h, n = 3, i = 3)
  expect_error(rfunction(h)(3, 4), class = "zumir_runtime_error")
  expect_error(rfunction(h)(3, 0), class = "zumir_runtime_error")
})

test_that("seq_len() and numeric() sizes must be whole numbers from 0", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  sl <- function(n) {
    s <- 0
    for (i in seq_len(n)) {
      s <- s + i
    }
    s
  }
  nm <- function(n) {
    y <- numeric(n)
    y
  }
  for (n in c(0, 1, 3)) {
    expect_same_as_r(sl, n = n)
    expect_same_as_r(nm, n = n)
  }
  # R errors at -1 and NA, and truncates or rounds 2.5.
  for (n in c(-1, NA, 2.5, Inf)) {
    expect_error(rfunction(sl)(n), class = "zumir_runtime_error")
    expect_error(rfunction(nm)(n), class = "zumir_runtime_error")
  }
})

test_that("a:b needs whole bounds that are not NA", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- function(a, b) {
    s <- 0
    for (i in a:b) {
      s <- s + i
    }
    s
  }
  expect_same_as_r(f, a = 1, b = 3)
  # R errors on NA, and steps from 1.5 by 1.
  for (ab in list(c(NA, 3), c(1, NA), c(1.5, 3), c(1, 2.5), c(1, Inf))) {
    expect_error(rfunction(f)(ab[1], ab[2]), class = "zumir_runtime_error")
  }
})

test_that("an integer counter that would overflow is an error, not NA", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- function(a) {
    s <- 0
    for (i in seq_len(a)) {
      if (i == a) s <- s + i * i
    }
    s
  }
  expect_same_as_r(f, a = 46340)
  expect_identical(rfunction(f)(46340), 46340 * 46340)
  # R gives NA with a warning: i * i is integer arithmetic there.
  expect_warning(expect_identical(f(46341), NA_real_))
  expect_error(rfunction(f)(46341), class = "zumir_runtime_error")
})

test_that("the overflow guard straddles +-.Machine$integer.max exactly", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  up <- function(n) {
    s <- 0
    for (i in seq_len(n)) {
      if (i == n) s <- as.double(i + 2147483645L)
    }
    s
  }
  down <- function(n) {
    s <- 0
    for (i in seq_len(n)) {
      if (i == n) s <- as.double(-i - 2147483645L)
    }
    s
  }
  for (f in list(up, down)) {
    # limit - 1 and limit: R's answer.
    for (n in c(1, 2)) {
      expect_same_as_r(f, n = n)
    }
    # limit + 1: R's NA with a warning, an error here.
    expect_warning(expect_identical(f(3), NA_real_))
    expect_error(rfunction(f)(3), class = "zumir_runtime_error")
  }
})

test_that("arguments must be plain doubles of the right length", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  g <- rfunction(function(a) a + 1)
  bad <- list(
    1L,
    TRUE,
    "1",
    c(a = 1),
    matrix(1),
    Sys.Date(),
    as.difftime(1, units = "secs"),
    numeric(),
    c(1, 2, 3)
  )
  for (v in bad) {
    expect_error(g(v), class = "zumir_argument_error")
  }
  expect_identical(g(1), 2)
})

test_that("a vector longer than .Machine$integer.max is refused", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  # A compact ALTREP sequence: no data is allocated, and none may be read.
  x <- 1:(2^31)
  expect_identical(typeof(x), "double")
  f <- rfunction(function(x) {
    s <- 0
    for (i in seq_along(x)) {
      s <- s + x[i]
    }
    s
  })
  expect_error(f(x), class = "zumir_argument_error")
})

test_that("loops of 2^16 - 1, 2^16 and 2^16 + 1 iterations are exact", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- function(n) {
    s <- 0
    for (i in seq_len(n)) {
      s <- s + 1
    }
    s
  }
  w <- function(n) {
    s <- 0
    while (s < n) {
      s <- s + 1
    }
    s
  }
  for (n in c(2^16 - 1, 2^16, 2^16 + 1)) {
    expect_same_as_r(f, n = n)
    expect_same_as_r(w, n = n)
  }
})

test_that("a time limit stops compiled loops, and the session goes on", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  lib <- installed_lib()
  skip_if(is.null(lib), "needs the installed package")
  out <- run_child(
    lib,
    c(
      "library(zumir)",
      "inf <- rfunction(function(a) { s <- 0; while (TRUE) s <- s + a; s })",
      "big <- rfunction(function(n) { s <- 0; for (i in seq_len(n)) s <- s + 1; s })",
      "run <- function(g, x) { setTimeLimit(elapsed = 1, transient = TRUE); g(x) }",
      "for (g in list(inf, big)) {",
      "  t0 <- proc.time()[[3]]",
      "  e <- tryCatch(run(g, 1e9), error = function(e) e)",
      "  cat('stopped', inherits(e, 'error'), proc.time()[[3]] - t0 < 30, '\\n')",
      "  cat('message', identical(conditionMessage(e),",
      "    tryCatch(run(function(x) repeat {}, 0), error = conditionMessage)), '\\n')",
      "}",
      "cat('again', big(10), big(65537), '\\n')"
    )
  )
  expect_identical(out$status, 0L)
  expect_identical(sum(out$output == "stopped TRUE TRUE "), 2L)
  expect_identical(sum(out$output == "message TRUE "), 2L)
  expect_true("again 10 65537 " %in% out$output)
})
