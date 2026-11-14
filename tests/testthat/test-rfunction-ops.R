# Each construct of rfunction()'s subset (design §4.6) against R itself.

test_that("arithmetic, ^, %% and %/% equal R's on odd doubles", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  v <- odd_doubles()
  fs <- list(
    function(a, b) a + b,
    function(a, b) a - b,
    function(a, b) a * b,
    function(a, b) a / b,
    function(a, b) a^b,
    function(a, b) -a + b,
    function(a, b) a %% b,
    function(a, b) a %/% b,
    function(a, b) min(a, b),
    function(a, b) max(a, b)
  )
  for (f in fs) {
    g <- rfunction(f)
    for (a in v) {
      for (b in v) {
        d <- compare_with_r(f, g, list(a = a, b = b))
        expect(is.null(d), paste(deparse(body(f)), a, b, d))
      }
    }
  }
})

test_that("the math table equals R's, warnings included", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  v <- c(odd_doubles(), 1e-8, -1e-8, 4, 9, 121, 144)
  for (fn in c(
    "exp",
    "log",
    "log1p",
    "expm1",
    "sqrt",
    "abs",
    "floor",
    "ceiling",
    "trunc",
    "sin",
    "cos",
    "tan"
  )) {
    f <- eval(bquote(function(a) .(as.name(fn))(a)))
    g <- rfunction(f)
    for (a in v) {
      d <- compare_with_r(f, g, list(a = a))
      expect(is.null(d), paste(fn, a, d))
    }
  }
})

test_that("comparisons and logic follow R's three-valued rules", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  v <- c(0, 1, NA, NaN, -Inf)
  for (op in c("==", "!=", "<", "<=", ">", ">=")) {
    f <- eval(bquote(function(a, b) as.double(.(as.name(op))(a, b))))
    for (a in v) {
      for (b in v) {
        expect_same_as_r(f, a = a, b = b)
      }
    }
  }
  lv <- c(0, 1, NA)
  and <- function(a, b) as.double(a > 0 && b > 0)
  or <- function(a, b) as.double(a > 0 || b > 0)
  not <- function(a) as.double(!(a > 0))
  for (a in lv) {
    expect_same_as_r(not, a = a)
    for (b in lv) {
      expect_same_as_r(and, a = a, b = b)
      expect_same_as_r(or, a = a, b = b)
    }
  }
  # NA && FALSE is FALSE, NA && TRUE is NA, as in R.
  expect_identical(rfunction(and)(NA_real_, 0), 0)
  expect_identical(rfunction(and)(NA_real_, 1), NA_real_)
  isf <- function(a) {
    as.double(is.na(a)) + 2 * as.double(is.nan(a)) + 4 * as.double(is.finite(a))
  }
  for (a in odd_doubles()) {
    expect_same_as_r(isf, a = a)
  }
})

test_that("a logical in arithmetic is 1, 0 or NA, as in R", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- function(a) (a > 0) + 0.5
  for (a in c(-1, 1, NA)) {
    expect_same_as_r(f, a = a)
  }
})

test_that("every loop form, counting down, break and next", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  along <- function(x) {
    s <- 0
    for (i in seq_along(x)) {
      s <- s + i * x[i]
    }
    s
  }
  for (x in list(numeric(), 1, c(1, -2, 3.5), c(NA, 1), c(NaN, NA))) {
    expect_same_as_r(along, x = x)
  }
  len <- function(n) {
    s <- 0
    for (i in seq_len(n)) {
      s <- s + i
    }
    s
  }
  for (n in c(0, 1, 5, 100)) {
    expect_same_as_r(len, n = n)
  }
  colon <- function(a, b) {
    s <- 0
    for (i in a:b) {
      s <- s * 2 + i
    }
    s
  }
  for (ab in list(c(1, 4), c(4, 1), c(1, 0), c(0, 0), c(-3, 2), c(2, -3))) {
    expect_same_as_r(colon, a = ab[1], b = ab[2])
  }
  brk <- function(x) {
    s <- 0
    for (i in seq_along(x)) {
      if (x[i] < 0) {
        break
      }
      if (x[i] == 0) {
        next
      }
      s <- s + x[i]
    }
    s
  }
  expect_same_as_r(brk, x = c(1, 0, 2, -1, 5))
  wh <- function(n) {
    i <- 0
    s <- 0
    while (i < n) {
      i <- i + 1
      if (i %% 3 == 0) {
        next
      }
      s <- s + i
      if (s > 50) break
    }
    s
  }
  for (n in c(0, 5, 100)) {
    expect_same_as_r(wh, n = n)
  }
})

test_that("vector locals made by numeric() and filled by index", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  csum <- function(x) {
    y <- numeric(length(x))
    acc <- 0
    for (i in seq_along(x)) {
      acc <- acc + x[i]
      y[i] <- acc
    }
    y
  }
  for (x in list(numeric(), c(1, 2, 3), c(0.1, NA, 3), c(-0, -0))) {
    expect_same_as_r(csum, x = x)
  }
  rev2 <- function(x, k) {
    y <- numeric(length(x) + k)
    for (i in seq_along(x)) {
      y[length(x) - i + 1] <- x[i]
    }
    y
  }
  expect_same_as_r(rev2, x = c(1, 2, 3), k = 2)
})

test_that("results do not change in a hostile session", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- function(a, b) {
    s <- a / 3
    for (i in seq_len(3)) {
      s <- s + b * 1e-7 + 1234567.891
    }
    s
  }
  want <- f(1, 2)
  g <- rfunction(f)
  old <- options(OutDec = ",", scipen = -5, digits = 3)
  on.exit(options(old))
  expect_identical(g(1, 2), want)
  # And compiled under it.
  expect_identical(rfunction(f)(1, 2), want)
})

test_that("native and interpreted rfunction()s agree", {
  skip_if(zumir_tier() != "native", "the native tier does not run here")
  f <- function(x, a) {
    s <- a
    for (i in seq_along(x)) {
      s <- s * 1.5 - x[i] %% 3 + sqrt(abs(x[i]))
    }
    s
  }
  n <- rfunction(f, tier = "native")
  i <- rfunction(f, tier = "interp")
  for (x in list(c(1, 2, 3), c(-1, NA, 1e300))) {
    expect_identical(
      suppressWarnings(n(x, 0.5)),
      suppressWarnings(i(x, 0.5))
    )
  }
})

test_that("min() and max() of an integer and a double convert the integer", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  # The conversion was once emitted and then dropped, so the call read an
  # unset register (found by the random differential).
  f <- function(x) {
    s <- 0
    for (i in seq_along(x)) {
      s <- s + min(as.double(i), i) + max(i, x[i]) + min(x[i], i)
    }
    s
  }
  for (x in list(c(1, 2, 3, 4), c(-1, 10, NA, 2.5))) {
    expect_same_as_r(f, x = x)
  }
})
