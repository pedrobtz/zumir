# c2mir's diagnostics (design §4.5 step 2, §6): a zumir_compile_error that
# points at the user's line and column, not the prelude's; no system
# header is read (D5).

test_that("a syntax error on line 3 is reported at line 3", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  e <- expect_error(
    c2mir_compile(
      "double f(double a) {\n  double b = a;\n  return b +;\n}",
      "f"
    ),
    class = "zumir_compile_error"
  )
  expect_identical(e$line, 3L)
  expect_identical(e$column, 13L)
  # The same error one line lower is one line lower.
  e2 <- expect_error(
    c2mir_compile(
      "\ndouble f(double a) {\n  double b = a;\n  return b +;\n}",
      "f"
    ),
    class = "zumir_compile_error"
  )
  expect_identical(e2$line, 4L)
})

test_that("an error on line 1 is line 1, after the prelude's own lines", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  e <- expect_error(
    c2mir_compile("double f(double a) { return a + undeclared_name; }", "f"),
    class = "zumir_compile_error"
  )
  expect_identical(e$line, 1L)
  expect_true(e$column > 0L)
})

test_that("a warning before the error does not move the position", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  # c2mir warns about the empty initializer on line 1, then the error is on
  # line 3: the position is the error's.
  e <- expect_error(
    c2mir_compile(
      "int a[2] = {};\ndouble f(double x) {\n  return x +;\n}",
      "f"
    ),
    class = "zumir_compile_error"
  )
  expect_identical(e$line, 3L)
  expect_identical(e$column, 13L)
})

test_that("no system header is read: stdio.h is not found", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  for (h in c(
    "<stdio.h>",
    "<stdlib.h>",
    "\"stdio.h\"",
    "</usr/include/stdio.h>",
    "\"zumir_no_such_file.h\""
  )) {
    e <- expect_error(
      c2mir_compile(
        sprintf("#include %s\ndouble f(double a) { return a; }", h),
        "f"
      ),
      class = "zumir_compile_error",
      info = h
    )
    expect_identical(e$line, 1L, info = h)
  }
  # A header file that exists in the working directory is not read either.
  dir <- tempfile()
  dir.create(dir)
  old <- setwd(dir)
  on.exit(setwd(old))
  writeLines("#define ZUMIR_FROM_A_FILE 1", "local.h")
  expect_error(
    c2mir_compile(
      "#include \"local.h\"\ndouble f(double a) { return a; }",
      "f"
    ),
    class = "zumir_compile_error"
  )
  f <- c2mir_fn(
    "#if __has_include(\"local.h\") || __has_include(<stdio.h>)\ndouble f(void) { return 1; }\n#else\ndouble f(void) { return 2; }\n#endif"
  )
  expect_identical(f(), 2)
})

test_that("the prelude's headers and c2mir's own are found", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- c2mir_fn(
    "
    #include <math.h>
    #include <string.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <float.h>
    #include <limits.h>
    double f(void) {
      bool ok = INT_MAX == 2147483647 && sizeof(int64_t) == 8 && DBL_MANT_DIG == 53;
      size_t n = strlen(\"abcd\");
      return ok ? (double) n + floor(M_PI) : -1;
    }"
  )
  expect_identical(f(), 7)
  inf <- c2mir_fn("double f(void) { return INFINITY + HUGE_VAL; }")
  expect_identical(inf(), Inf)
  nan <- c2mir_fn("double f(void) { return NAN; }")
  expect_true(is.nan(nan()))
})

test_that("a function the source does not define is an argument error", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  expect_error(
    c2mir_compile("double f(double a) { return a; }", "g"),
    class = "zumir_argument_error"
  )
  expect_error(c2mir_compile(c("a", "b"), "f"), class = "zumir_argument_error")
  expect_error(
    c2mir_compile(NA_character_, "f"),
    class = "zumir_argument_error"
  )
})

test_that("a failed compile leaves no context behind", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  gc()
  base <- .Call(zumir_test_live_arenas)
  for (k in 1:50) {
    tryCatch(
      c2mir_compile("double f(double a) { return a +; }", "f"),
      zumir_error = function(e) NULL
    )
    tryCatch(
      c2mir_compile(
        "double puts(double); double f(double a) { return puts(a); }",
        "f"
      ),
      zumir_error = function(e) NULL
    )
  }
  gc()
  expect_lte(.Call(zumir_test_live_arenas), base)
})

test_that("blank and comment lines at the top are counted", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  # c2mir does not count lines that follow a #line directive directly;
  # the prelude ends with a line of its own so the user's are counted.
  for (k in 0:3) {
    for (top in c("\n", "// a comment\n", "/* a\n comment */\n")) {
      src <- paste0(strrep(top, k), "double f(double a) {\n  return a +;\n}")
      e <- expect_error(c2mir_compile(src, "f"), class = "zumir_compile_error")
      expect_identical(
        e$line,
        2L + k * (1L + (top == "/* a\n comment */\n")),
        info = paste(k, top)
      )
    }
  }
})
