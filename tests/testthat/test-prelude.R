# cfunction()'s prelude (design §4.5, D5, D6): the names it declares are
# the names the resolver links, and a C call to each returns what the C
# library, or R, returns.

test_that("the prelude declares exactly the resolver's D6 names", {
  p <- .Call(zumir_test_prelude)
  declared <- c(declared_names(p[["math.h"]]), declared_names(p[["string.h"]]))
  expect_setequal(declared, d6_names())
  expect_false(anyDuplicated(declared) > 0)
})

test_that("every D6 math name links, and returns what the C library returns", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  two <- c(
    "atan2",
    "hypot",
    "pow",
    "fmod",
    "remainder",
    "copysign",
    "nextafter",
    "fdim",
    "fmax",
    "fmin"
  )
  special <- c(
    "fma",
    "ldexp",
    "frexp",
    "modf",
    "memcpy",
    "memmove",
    "memset",
    "memcmp",
    "strlen"
  )
  one <- setdiff(d6_names(), c(two, special))
  x <- c(-2.5, -1, -0, 0, 0.3, 1, 2, 7.5, 1e-300, Inf, NaN)
  y <- c(2, -3, 1, -0.5, 4, 0.5, -2, 3, 2, 1, 0.25)
  for (fn in one) {
    f <- c2mir_fn(sprintf("double f(double x) { return %s(x); }", fn))
    # The oracle: the same C function called through MIR's own import, with
    # the right prototype (test-imports.R holds those to R).
    ref <- mir_function(call1_text(fn), "f")
    expect_identical(vapply(x, f, 0), vapply(x, ref, 0), info = fn)
  }
  for (fn in two) {
    f <- c2mir_fn(sprintf(
      "double f(double x, double y) { return %s(x, y); }",
      fn
    ))
    ref <- mir_function(call2_text(fn), "f")
    expect_identical(mapply(f, x, y), mapply(ref, x, y), info = fn)
  }
})

test_that("a C call returns what R's own function returns", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  x <- c(-2.5, -0, 0.3, 1, 7.5, 1e-300, -Inf, NaN)
  same <- list(
    exp = exp,
    log = log,
    sqrt = sqrt,
    sin = sin,
    atan = atan,
    floor = floor,
    ceil = ceiling,
    trunc = trunc,
    fabs = abs
  )
  for (fn in names(same)) {
    f <- c2mir_fn(sprintf("double f(double x) { return %s(x); }", fn))
    want <- suppressWarnings(same[[fn]](x))
    expect_identical(vapply(x, f, 0), want, info = fn)
  }
  a2 <- c2mir_fn("double f(double x, double y) { return atan2(x, y); }")
  expect_identical(mapply(a2, x, rev(x)), atan2(x, rev(x)))
})

test_that("the pointer and int signatures are declared right", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  # fma rounds once: 0.1 * 10 - 1 is 2^-54 + ..., not the twice-rounded 0.
  fma3 <- c2mir_fn(
    "double f(double a, double b, double c) { return fma(a, b, c); }"
  )
  expect_identical(fma3(2, 3, 4), 10)
  expect_identical(fma3(0.1, 10, -1), 5.551115123125783e-17)
  ld <- c2mir_fn("double f(double x, double e) { return ldexp(x, (int) e); }")
  expect_identical(ld(0.75, 4), 12)
  expect_identical(ld(1, -1074), 5e-324)
  fr <- c2mir_fn(
    "double f(double x) { int e; double m = frexp(x, &e); return m * 1000 + e; }"
  )
  expect_identical(fr(8), 504)
  expect_identical(fr(-0.375), -750 - 1)
  mf <- c2mir_fn(
    "double f(double x) { double i; double fr = modf(x, &i); return i * 10 + fr; }"
  )
  expect_identical(mf(3.25), 30.25)
  expect_identical(mf(-2.5), -20.5)
})

test_that("the string functions copy, move, fill, compare and count", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- c2mir_fn(
    "
    double f(void) {
      double a[4] = {1, 2, 3, 4}, b[4];
      char s[8];
      memcpy(b, a, sizeof a);
      memmove(a + 1, a, 3 * sizeof(double));
      memset(s, 'x', 5);
      s[5] = 0;
      /* The bytes memset wrote, not only how many: a wrong declaration
         can fill with another byte and still give strlen 5. */
      int filled = s[0] == 'x' && s[4] == 'x';
      return b[3] * 1000 + a[1] * 100 + strlen(s) * 10 + (memcmp(a, b, sizeof a) != 0) +
        0.5 * filled;
    }"
  )
  expect_identical(f(), 4000 + 100 + 50 + 1 + 0.5)
})

test_that("a name outside D6's list is a link error, R's internals included", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  for (fn in c(
    "printf",
    "puts",
    "abort",
    "exit",
    "system",
    "Rf_error",
    "R_pow",
    "malloc"
  )) {
    expect_error(
      c2mir_compile(
        sprintf(
          "double %s(double); double f(double x) { return %s(x); }",
          fn,
          fn
        ),
        "f"
      ),
      class = "zumir_mir_error",
      info = fn
    )
  }
})

test_that("both tiers give the same answers", {
  skip_if(zumir_tier() != "native", "the native tier does not run here")
  src <- "double f(double x, double y) { return hypot(x, y) + fmod(x, y) * exp(-x); }"
  n <- c2mir_fn(src, tier = "native")
  i <- c2mir_fn(src, tier = "interp")
  for (v in list(c(3, 4), c(-2.5, 0.5), c(1e300, 1e300), c(NaN, 1))) {
    expect_identical(n(v), i(v))
  }
})
