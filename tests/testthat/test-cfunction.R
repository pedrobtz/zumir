# cfunction() (design §4.5 step 5, §5, Q2, D24): every example and type
# against a reference written in R, the refusals by class, outputs as
# copies, and the length limits.

test_that("the design's examples return what R computes", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  sq <- cfunction(
    "
    void sq(int n, const double *x, double *y) {
      for (int i = 0; i < n; i++) y[i] = x[i] * x[i];
    }",
    out = "y",
    n = "n"
  )
  x <- c(1, -2.5, 3, 0, NA, Inf)
  expect_identical(sq(x = x), x * x)
  dot <- cfunction(
    "
    double dot(int n, const double *x, const double *y) {
      double s = 0;
      for (int i = 0; i < n; i++) s += x[i] * y[i];
      return s;
    }",
    n = "n"
  )
  expect_identical(dot(x = c(1, 2, 3), y = c(4, 5, 6)), 32)
  expect_identical(dot(x = numeric(), y = numeric()), 0)
})

test_that("every argument and return type against R", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  mul <- cfunction("int mul(int a, int b) { return a * b; }")
  expect_identical(mul(6L, 7L), 42L)
  expect_identical(mul(6, -7), -42L)
  half <- cfunction("double half(double a, int k) { return a / k; }")
  expect_identical(half(1, 4L), 0.25)
  isum <- cfunction(
    "
    int isum(int n, const int *x) {
      int s = 0;
      for (int i = 0; i < n; i++) s += x[i];
      return s;
    }",
    n = "n"
  )
  expect_identical(isum(x = 1:10), 55L)
  # A whole double vector is accepted for const int *.
  expect_identical(isum(x = c(1, 2, 3)), 6L)
  icum <- cfunction(
    "
    void icum(int n, const int *x, int *y) {
      int s = 0;
      for (int i = 0; i < n; i++) { s += x[i]; y[i] = s; }
    }",
    out = "y",
    n = "n"
  )
  expect_identical(icum(x = c(3L, -1L, 4L)), cumsum(c(3L, -1L, 4L)))
  # An integer vector is converted for const double *.
  dcum <- cfunction(
    "
    void dcum(int n, const double *x, double *y) {
      double s = 0;
      for (int i = 0; i < n; i++) { s += x[i]; y[i] = s; }
    }",
    out = "y",
    n = "n"
  )
  expect_identical(dcum(x = 1:4), cumsum(c(1, 2, 3, 4)))
  # void with no output returns invisible NULL.
  nothing <- cfunction("void nothing(double a) { (void) a; }")
  expect_invisible(nothing(1))
  expect_null(nothing(1))
  # The prelude's maths, against R where R's function is the C library's.
  ex <- cfunction("double ex(double a) { return exp(a) + sqrt(a) - floor(a); }")
  expect_identical(ex(2.5), exp(2.5) + sqrt(2.5) - floor(2.5))
})

test_that("arguments arrive in their order, outputs before and after scalars", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  # The wrapper once moved a void function's arguments down twice.
  f <- cfunction(
    "void f(double *y, int n) { for (int i = 0; i < n; i++) y[i] = 10 * n + i; }",
    out = "y",
    n = "n"
  )
  expect_identical(f(y = c(0, 0, 0), n = 2L), c(20, 21, 0))
  g <- cfunction(
    "void g(double a, double *y, double b) { y[0] = a; y[1] = b; }",
    out = c(y = 2)
  )
  expect_identical(g(1.5, b = -2), c(1.5, -2))
  h <- cfunction(
    "int h(int a, double *y, int b, double c) { y[0] = c; return a - b; }",
    out = c(y = 1)
  )
  expect_identical(h(10L, b = 3L, c = 0.5), list(h = 7L, y = 0.5))
})

test_that("outputs are copies: the caller's vector never changes (D24)", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  inc <- cfunction(
    "
    void inc(int n, double *y) { for (int i = 0; i < n; i++) y[i] += 1; }",
    out = "y",
    n = "n"
  )
  y0 <- c(1, 2, 3)
  y1 <- y0
  bits <- serialize(y0, NULL)
  expect_identical(inc(n = 3L, y = y0), c(2, 3, 4))
  expect_identical(serialize(y0, NULL), bits)
  expect_identical(y1, c(1, 2, 3))
  # An integer output seeded from a double is converted, and still a copy.
  iinc <- cfunction(
    "
    void iinc(int n, int *y) { for (int i = 0; i < n; i++) y[i] += 1; }",
    out = "y",
    n = "n"
  )
  yi <- c(5, 6)
  expect_identical(iinc(n = 2L, y = yi), c(6L, 7L))
  expect_identical(yi, c(5, 6))
  # Not supplied: zero-filled.
  expect_identical(inc(n = 4L), c(1, 1, 1, 1))
  # A value and two outputs: a named list, the value first.
  three <- cfunction(
    "
    double three(int n, const double *x, double *lo, int *pos) {
      double s = 0;
      for (int i = 0; i < n; i++) { s += x[i]; lo[i] = x[i] - 1; pos[i] = x[i] > 0; }
      return s;
    }",
    out = c("lo", "pos"),
    n = "n"
  )
  expect_identical(
    three(x = c(-1, 2)),
    list(three = 1, lo = c(-2, 1), pos = c(0L, 1L))
  )
})

test_that("an argument that is not code is refused on any build", {
  # Checked before a tier is needed, so a build without MIR refuses it too.
  expect_error(cfunction(c("a", "b")), class = "zumir_argument_error")
  expect_error(cfunction(NA_character_), class = "zumir_argument_error")
  expect_error(
    cfunction("double f(void) { return 1; }", out = 1),
    class = "zumir_argument_error"
  )
  expect_error(
    cfunction("double f(void) { return 1; }", n = c("a", "b")),
    class = "zumir_argument_error"
  )
})

test_that("refusals carry their classes", {
  # The signature is read from what c2mir compiled: these need a tier.
  skip_if(zumir_tier() == "none", "no tier runs here")
  # At compile: the signature, the outputs, the source.
  expect_error(
    cfunction("float f(float a) { return a; }"),
    class = "zumir_compile_error"
  )
  expect_error(
    cfunction("double f(char *s) { return 0; }"),
    class = "zumir_compile_error"
  )
  expect_error(
    cfunction("double *f(double *y) { return y; }", out = "y"),
    class = "zumir_compile_error"
  )
  expect_error(
    cfunction("unsigned f(unsigned a) { return a; }"),
    class = "zumir_compile_error"
  )
  # D24: a non-const pointer not named in out.
  expect_error(
    cfunction("void f(double *y) { y[0] = 1; }"),
    class = "zumir_compile_error"
  )
  expect_error(
    cfunction("void f(double *y) { y[0] = 1; }", out = "z"),
    class = "zumir_argument_error"
  )
  expect_error(
    cfunction("double f(const double *x) { return x[0]; }", out = "x"),
    class = "zumir_argument_error"
  )
  # An output with no length: no n, no fixed length.
  expect_error(
    cfunction("void f(double *y) { y[0] = 1; }", out = "y"),
    class = "zumir_argument_error"
  )
  expect_error(
    cfunction("void f(double a, double *y) { y[0] = a; }", out = "y", n = "a"),
    class = "zumir_argument_error"
  )
  # Q2: one non-static function.
  expect_error(
    cfunction("double f(void) { return 1; } double g(void) { return 2; }"),
    class = "zumir_argument_error"
  )
  expect_error(
    cfunction("static double f(void) { return 1; }"),
    class = "zumir_argument_error"
  )
  expect_error(
    cfunction("double f(double a) { return a +; }"),
    class = "zumir_compile_error"
  )
  # At the call.
  half <- cfunction("double half(double a, int k) { return a / k; }")
  expect_error(half("1", 2L), class = "zumir_argument_error")
  expect_error(half(c(1, 2), 2L), class = "zumir_argument_error")
  expect_error(half(1, 2.5), class = "zumir_argument_error")
  expect_error(half(1, NA_integer_), class = "zumir_argument_error")
  expect_error(half(1, 2^31), class = "zumir_argument_error")
  isum <- cfunction(
    "int isum(int n, const int *x) { return n ? x[0] : 0; }",
    n = "n"
  )
  expect_error(isum(x = c(1.5, 2)), class = "zumir_argument_error")
  expect_error(isum(x = "a"), class = "zumir_argument_error")
})

test_that("each value is checked and converted as R's as.integer() and as.double() would", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  half <- cfunction("double half(double a, int k) { return a / k; }")
  # Scalars: is.numeric() and one element; an int is whole and fits.
  expect_error(half(factor("a"), 1L), class = "zumir_argument_error")
  expect_error(half(TRUE, 1L), class = "zumir_argument_error")
  expect_error(half(numeric(), 1L), class = "zumir_argument_error")
  expect_error(half(1, Inf), class = "zumir_argument_error")
  expect_error(half(1, NaN), class = "zumir_argument_error")
  expect_error(half(1, -2^31), class = "zumir_argument_error")
  expect_identical(half(1, -(2^31 - 1)), 1 / -(2^31 - 1))
  expect_identical(half(1, .Machine$integer.max), 1 / .Machine$integer.max)
  expect_identical(half(3L, 2), 1.5)
  expect_identical(half(NA_integer_, 1L), NA_real_)
  # A const int * gets as.integer() of a double vector: NA and NaN become
  # NA_integer_; a const double * gets as.double() of an integer one.
  first_i <- cfunction("int first_i(const int *x) { return x[0]; }")
  expect_identical(first_i(c(NaN, 1)), NA_integer_)
  expect_identical(first_i(c(NA, 1)), NA_integer_)
  expect_identical(first_i(c(-3, 1)), -3L)
  expect_error(first_i(c(1, 2^31)), class = "zumir_argument_error")
  first_d <- cfunction("double first_d(const double *x) { return x[0]; }")
  expect_identical(first_d(c(NA_integer_, 1L)), NA_real_)
  expect_identical(first_d(c(7L, 1L)), 7)
  # A given output comes back as a plain copy: its values, not its
  # attributes, and converted to the parameter's type.
  inc <- cfunction(
    "void inc(int n, double *y, int *k) {
       for (int i = 0; i < n; i++) { y[i] += 1; k[i] += 1; }
     }",
    out = c("y", "k"),
    n = "n"
  )
  y <- c(a = 1, b = 2)
  k <- c(5, 6)
  r <- inc(y = y, k = k, n = 2L)
  expect_identical(r, list(y = c(2, 3), k = c(6L, 7L)))
  expect_identical(y, c(a = 1, b = 2))
  expect_error(
    inc(y = y, k = c(1.5, 2), n = 2L),
    class = "zumir_argument_error"
  )
  expect_error(inc(y = y, k = "a", n = 2L), class = "zumir_argument_error")
})

test_that("a name outside D6 is a link error, mir_compile()'s and R's included", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  for (fn in c("R_pow", "zumir_r_fmod", "expit", "printf", "Rf_error")) {
    expect_error(
      cfunction(sprintf(
        "double %s(double, ...); double f(double a) { return %s(a); }",
        fn,
        fn
      )),
      class = "zumir_mir_error",
      info = fn
    )
  }
})

test_that("n straddles the inputs' length; outputs straddle n", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  sq <- cfunction(
    "void sq(int n, const double *x, double *y) {
    for (int i = 0; i < n; i++) y[i] = x[i] * x[i]; }",
    out = "y",
    n = "n"
  )
  x <- c(1, 2, 3)
  expect_identical(sq(x = x, n = 3L), c(1, 4, 9))
  expect_identical(sq(x = x, n = 2L), c(1, 4))
  expect_error(sq(x = x, n = 4L), class = "zumir_argument_error")
  expect_error(sq(x = x, n = -1L), class = "zumir_argument_error")
  expect_identical(sq(x = numeric(), n = 0L), numeric())
  # A supplied output shorter than n is refused; longer keeps its tail.
  expect_error(sq(x = x, y = c(0, 0)), class = "zumir_argument_error")
  expect_identical(sq(x = x, y = c(0, 0, 0, 7)), c(1, 4, 9, 7))
  # Inputs of different lengths need n.
  dot <- cfunction(
    "double dot(int n, const double *x, const double *y) {
    double s = 0; for (int i = 0; i < n; i++) s += x[i] * y[i]; return s; }",
    n = "n"
  )
  expect_error(dot(x = 1, y = c(1, 2)), class = "zumir_argument_error")
  expect_identical(dot(x = 1, y = c(1, 2), n = 1L), 1)
})

test_that("a vector longer than .Machine$integer.max is refused unread", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  # A compact ALTREP sequence: refused before its data would be expanded.
  x <- 1:(2^31)
  first <- cfunction(
    "double first(int n, const double *x) { return n ? x[0] : 0; }",
    n = "n"
  )
  expect_error(first(x = x), class = "zumir_argument_error")
  expect_error(first(x = x, n = 1L), class = "zumir_argument_error")
})

test_that("native and interpreted cfunction()s give identical results", {
  skip_if(zumir_tier() != "native", "the native tier does not run here")
  src <- "double poly(int n, const double *c, double x, double *terms) {
    double s = 0, p = 1;
    for (int i = 0; i < n; i++) { terms[i] = c[i] * p; s += terms[i]; p *= x; }
    return s; }"
  n <- cfunction(src, out = "terms", n = "n", tier = "native")
  i <- cfunction(src, out = "terms", n = "n", tier = "interp")
  for (x in c(0.5, -3, 1e10, NaN)) {
    expect_identical(n(c = c(1, -2, 3.5), x = x), i(c = c(1, -2, 3.5), x = x))
  }
})

test_that("results do not change in a hostile session", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  src <- "double f(double a) { return a / 3 + 1234567.891; }"
  want <- cfunction(src)(1)
  old <- options(OutDec = ",", scipen = -5, digits = 3)
  on.exit(options(old))
  expect_identical(cfunction(src)(1), want)
  expect_identical(want, 1 / 3 + 1234567.891)
})
