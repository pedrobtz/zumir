# mir_function() (design §5): MIR text in, an R function out.

test_that("the design's add example adds, on both tiers", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  for (tier in c("native", "interp")) {
    add <- mir_function(add_text(), "add", tier = tier)
    expect_s3_class(add, "zumir_function")
    expect_identical(names(formals(add)), c("a", "b"))
    expect_identical(add(1, 2), 3)
    # IEEE addition, the same double R computes.
    expect_identical(add(0.1, 0.2), 0.1 + 0.2)
    expect_identical(add(-0, -0), -0)
    expect_identical(add(Inf, -Inf), Inf + -Inf)
  }
})

test_that("integer arguments are refused outside their type's range", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  limits <- list(
    i8 = c(-128, 127),
    u8 = c(0, 255),
    i16 = c(-32768, 32767),
    u16 = c(0, 65535),
    i32 = c(-2147483647, 2147483647)
  )
  for (type in names(limits)) {
    f <- mir_function(int_text(type), "f")
    lo <- limits[[type]][1]
    hi <- limits[[type]][2]
    # limit - 1 (inside), limit, limit + 1 (outside), at both ends.
    expect_identical(f(hi - 1, 0), as.integer(hi - 1), info = type)
    expect_identical(f(hi, 0), as.integer(hi), info = type)
    expect_error(f(hi + 1, 0), class = "zumir_argument_error", info = type)
    expect_identical(f(lo, 0), as.integer(lo), info = type)
    expect_error(f(lo - 1, 0), class = "zumir_argument_error", info = type)
    expect_error(f(1.5, 0), class = "zumir_argument_error", info = type)
    expect_error(f(NA, 0), class = "zumir_argument_error", info = type)
    expect_error(f(NaN, 0), class = "zumir_argument_error", info = type)
    expect_error(f(c(1, 2), 0), class = "zumir_argument_error", info = type)
    expect_error(f("1", 0), class = "zumir_argument_error", info = type)
  }
})

test_that("an i32 result is sign-extended, and INT_MIN is refused as NA", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- mir_function(int_text("i32", "sub"), "f")
  expect_identical(f(0, 1), -1L)
  expect_identical(f(-2147483647, 0), -2147483647L)
  expect_error(f(-2147483647, 1), class = "zumir_argument_error")
  u <- mir_function(int_text("u8", "add"), "f")
  # 200 + 100 wraps in 8 bits; the result is zero-extended from u8.
  expect_identical(u(200, 100), 44L)
})

test_that("64-bit results are doubles, exact up to 2^53 and refused beyond", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- mir_function(int_text("i64"), "f")
  expect_identical(f(2^53 - 1, 0), 2^53 - 1)
  expect_identical(f(2^53, 0), 2^53)
  expect_error(f(2^53, 1), class = "zumir_argument_error")
  expect_identical(f(-2^53, 0), -2^53)
  expect_error(f(-2^53, -1), class = "zumir_argument_error")
})

test_that("f arguments and results go through float", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  f <- mir_function(
    "m: module\n export f\nf: func f, f:a\n local f:r\n fadd r, a, a\n ret r\n endfunc\nendmodule",
    "f"
  )
  # 0.1 rounds to the nearest float, then doubles exactly.
  expect_identical(f(0.1), 2 * 0.100000001490116119384765625)
})

test_that("functions that cannot be called from R are refused when compiled", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  no_export <- "m: module\nf: func d, d:a\n ret a\n endfunc\nendmodule"
  expect_error(mir_function(no_export, "f"), class = "zumir_argument_error")
  expect_error(mir_function(add_text(), "nope"), class = "zumir_argument_error")
  ptr <- "m: module\n export f\nf: func i64, p:a\n local i64:r\n mov r, 0\n ret r\n endfunc\nendmodule"
  expect_error(mir_function(ptr, "f"), class = "zumir_argument_error")
  two <- "m: module\n export f\nf: func i64, i64, i64:a\n ret a, a\n endfunc\nendmodule"
  expect_error(mir_function(two, "f"), class = "zumir_argument_error")
  # ld is a long double where MIR has one; where long double is a double
  # (Windows, macOS on arm64), MIR reads it as d, and it is a number.
  ld <- "m: module\n export f\nf: func ld, ld:a\n ret a\n endfunc\nendmodule"
  ld_is_d <- .Platform$OS.type == "windows" || .Machine$sizeof.longdouble <= 8
  if (ld_is_d) {
    expect_identical(mir_function(ld, "f")(1.5), 1.5)
  } else {
    expect_error(mir_function(ld, "f"), class = "zumir_argument_error")
  }
})

test_that("bad text and bad arguments to mir_function() are classed errors", {
  expect_error(mir_function(1, "f"), class = "zumir_argument_error")
  expect_error(mir_function(c("a", "b"), "f"), class = "zumir_argument_error")
  expect_error(mir_function(NA_character_, "f"), class = "zumir_argument_error")
  expect_error(mir_function(add_text(), NA), class = "zumir_argument_error")
  expect_error(
    mir_function(add_text(), "add", tier = "jit"),
    class = "zumir_argument_error"
  )
  skip_if(zumir_tier() == "none", "no tier runs here")
  expect_error(
    mir_function("m: module\n bogus\nendmodule\n", "f"),
    class = "zumir_mir_error"
  )
})

test_that("an unavailable tier is a zumir_tier_error", {
  skip_if(zumir_tier() != "none", "a tier runs here")
  expect_error(mir_function(add_text(), "add"), class = "zumir_tier_error")
})

test_that("MIR's own test programs read and write back to a fixed point", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  # run-test's round trip, strengthened: text read and written by MIR, read
  # and written again, is the same text.
  files <- list.files(
    test_path("mir-tests"),
    pattern = "\\.mir$",
    full.names = TRUE
  )
  expect_length(files, 17L)
  out1 <- tempfile()
  out2 <- tempfile()
  on.exit(unlink(c(out1, out2)))
  for (f in files) {
    text <- paste(readLines(f, warn = FALSE), collapse = "\n")
    zumir_check(.Call(zumir_mir_roundtrip, paste0(text, "\n"), out1))
    once <- paste(readLines(out1, warn = FALSE), collapse = "\n")
    expect_true(nzchar(once), info = basename(f))
    zumir_check(.Call(zumir_mir_roundtrip, paste0(once, "\n"), out2))
    twice <- paste(readLines(out2, warn = FALSE), collapse = "\n")
    expect_identical(twice, once, info = basename(f))
  }
})

test_that("MIR's own test functions give their hand-computed values", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  read_test <- function(name, fn) {
    text <- readLines(test_path("mir-tests", name), warn = FALSE)
    at <- grep("module", text)[1]
    paste(
      c(text[seq_len(at)], paste(" export", fn), text[-seq_len(at)]),
      collapse = "\n"
    )
  }
  for (tier in c("native", "interp")) {
    # test1: l = 2i; j or k = 2i; i = l + j + k = 4i.
    cse <- mir_function(read_test("test1.mir", "cse"), "cse", tier = tier)
    expect_identical(cse(3), 12)
    expect_identical(cse(-3), -12)
    expect_identical(cse(0), 0)
    # test2: both branches set c to 5.
    ccp <- mir_function(read_test("test2.mir", "ccp"), "ccp", tier = tier)
    expect_identical(ccp(), 5)
    # test6 and test7: an i64 read as unsigned, to float and double.
    ui2d <- mir_function(read_test("test7.mir", "ui2d"), "ui2d", tier = tier)
    expect_identical(ui2d(-1), 2^64)
    expect_identical(ui2d(12345), 12345)
    ui2f <- mir_function(read_test("test6.mir", "ui2f"), "ui2f", tier = tier)
    expect_identical(ui2f(-1), 2^64)
    expect_identical(ui2f(16777217), 16777216)
  }
})
