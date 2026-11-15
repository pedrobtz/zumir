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

# The vendored c2mir fixes of the critical review (src/vendor/PROVENANCE),
# each against the value C11 and the platform's cc give.

test_that("a conversion to _Bool compares with zero (#65)", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  src <- "
    static _Bool id(_Bool b) { return b; }
    static _Bool sb = 5;
    void f(int n, double *y) {
      int flags = 0x300, i256 = 256; double h = 0.5; long long L = 0x100000000LL;
      int k = 0;
      _Bool has = flags & 0x100; y[k++] = has;
      y[k++] = (_Bool) 0.5; y[k++] = (_Bool) 256; y[k++] = (_Bool) i256;
      y[k++] = (_Bool) h; y[k++] = (_Bool) L; y[k++] = (_Bool) &flags;
      _Bool b = 1; b += 1; y[k++] = b;
      y[k++] = sb; y[k++] = id(i256); y[k++] = id(0.5);
      struct { _Bool a : 1; } s; s.a = 2; y[k++] = s.a;
      _Bool arr[2] = {256, 0.1}; y[k++] = arr[0] + arr[1];
      y[k++] = (_Bool) 0; y[k++] = (_Bool) 0.0;
    }"
  want <- c(rep(1, 12), 2, 0, 0, 0)
  for (tier in tiers_here()) {
    f <- cfunction(src, out = c(y = 16), tier = tier)
    expect_identical(f(n = 16L), want, info = tier)
  }
})

test_that("a floating value from 2^63 up converts to unsigned long long (#66)", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  src <- "
    double f(double x) { unsigned long long u = x; return (double) u; }"
  xs <- c(2^63 - 1024, 2^63, 1e19, 2^64 - 2048, 0, 1)
  for (tier in tiers_here()) {
    f <- cfunction(src, tier = tier)
    expect_identical(vapply(xs, f, 0), xs, info = tier)
  }
})

test_that("float.h, NULL, character constants and _Generic are C11's (#67)", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  src <- "
    void f(int n, double *y) {
      int k = 0, *p = NULL;
      y[k++] = FLT_DIG; y[k++] = DBL_DIG; y[k++] = DBL_MANT_DIG;
      y[k++] = p == NULL;
      y[k++] = sizeof ('a') == sizeof (int);
      y[k++] = sizeof (L\"ab\") == 3 * sizeof (wchar_t);
      y[k++] = _Generic (u'a', unsigned short: 1, default: 2);
    #if (1 ? -1 : 0u) < 0
      y[k++] = 1;
    #else
      y[k++] = 2;
    #endif
      /* The LDBL_ macros describe the long double c2mir computes with. */
      long double one = 1;
      y[k++] = one + LDBL_EPSILON != one && one + LDBL_EPSILON / 2 == one;
      y[k++] = LDBL_MANT_DIG >= DBL_MANT_DIG;
    }"
  want <- c(6, 15, 53, 1, 1, 1, 1, 2, 1, 1)
  for (tier in tiers_here()) {
    f <- cfunction(src, out = c(y = 10), tier = tier)
    expect_identical(f(n = 10L), want, info = tier)
  }
})

test_that("a narrow bit-field reads what was stored, on every tier (#68)", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  src <- "
    void f(int n, double *y) {
      struct { unsigned a : 3; long long b : 40; unsigned long long c : 31;
               int d : 31; unsigned e : 32; } q = {7, 1, 1, 1, 1};
      long long t = q.c;
      y[0] = t; y[1] = (double) (q.c + 1); y[2] = q.a; y[3] = q.d;
    }"
  for (tier in tiers_here()) {
    f <- cfunction(src, out = c(y = 4), tier = tier)
    expect_identical(f(n = 4L), c(1, 2, 7, 1), info = tier)
  }
})

test_that("an and feeding a uext compiles natively, in a child process (#56)", {
  skip_if(zumir_tier() != "native", "native code does not run here")
  lib <- installed_lib()
  skip_if(is.null(lib), "zumir is not installed: run under R CMD check")
  res <- run_child(
    lib,
    c(
      "library(zumir)",
      paste0(
        "f <- cfunction('double f(int n) { unsigned mask = 0xff; ",
        "unsigned r = mask & (unsigned) n; return r; }', tier = 'native')"
      ),
      "cat('value:', f(300L), f(-1L), '\\n')"
    ),
    timeout = 120
  )
  expect_identical(res$status, 0L)
  expect_true("value: 44 255 " %in% res$output)
})
