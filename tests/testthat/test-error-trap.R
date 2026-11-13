# The error trap (design §4.2, §6, D4): MIR's errors come back to R as
# zumir_error conditions, the session survives, and memory does not grow.
# The in-process tests run under valgrind and ASan in CI; the 10^3 loops run
# in a child R process, so a crash there fails a test, not the run.

test_that("each invalid module is a zumir_mir_error of MIR's error type", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  # MIR_error_type_t: MIR_syntax_error, MIR_op_mode_error,
  # MIR_undeclared_op_ref_error (mir.h).
  mir_types <- c(syntax = 1L, operand = 26L, import = 20L)
  kinds <- names(mir_types)
  for (kind in kinds) {
    cnd <- expect_error(
      test_invalid_module(kind),
      class = "zumir_mir_error"
    )
    expect_s3_class(cnd, "zumir_error")
    expect_identical(cnd$mir_type, mir_types[[kind]], info = kind)
    expect_true(nzchar(conditionMessage(cnd)), info = kind)
    # The context was finished, and no trap was left on the stack.
    expect_true(cnd$trap$finished, info = kind)
    expect_identical(cnd$trap$depth, 0L, info = kind)
  }
})

test_that("an error inside MIR_finish_func, then MIR_finish, does not crash", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  # "operand" fails inside MIR_finish_func, leaving curr_func set; finishing
  # the context then needs the fork's MIR_finish patch.
  for (i in 1:50) {
    cnd <- expect_error(test_invalid_module("operand"), class = "zumir_error")
    expect_true(cnd$trap$finished)
  }
})

test_that("50 failures of each kind in this process leave the trap usable", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  kinds <- c("syntax", "operand", "import")
  for (kind in kinds) {
    n <- 0L
    for (i in 1:50) {
      tryCatch(test_invalid_module(kind), zumir_error = function(e) {
        n <<- n + 1L
      })
    }
    expect_identical(n, 50L, info = kind)
  }
})

test_that("10^3 failures of each kind: every one caught, session alive, RSS flat", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  lib <- installed_lib()
  skip_if(is.null(lib), "zumir is not installed: run under R CMD check")
  res <- run_child(
    lib,
    c(
      "library(zumir)",
      "rss <- function() { gc(); .Call(zumir:::zumir_test_rss) }",
      "loop <- function(kind) { n <- 0L; for (i in 1:1000) tryCatch(zumir:::test_invalid_module(kind), zumir_error = function(e) n <<- n + 1L); n }",
      "for (kind in c('syntax', 'operand', 'import')) {",
      "  n1 <- loop(kind); r1 <- rss(); n2 <- loop(kind); r2 <- rss()",
      "  cat('kind:', kind, n1, n2, r2 - r1, '\\n')",
      "}",
      "cat('alive\\n')"
    )
  )
  expect_identical(res$status, 0L)
  expect_true("alive" %in% res$output)
  lines <- grep("^kind:", res$output, value = TRUE)
  expect_length(lines, 3L)
  for (line in lines) {
    f <- strsplit(line, " +")[[1]]
    expect_identical(f[3:4], c("1000", "1000"), info = line)
    # A context leaked per failure is at least 50 KB, so 10^3 of them would
    # be 50 MB; the spike measured 0 to 16 KB of growth for the second 10^3.
    growth <- as.numeric(f[5])
    if (!is.na(growth)) expect_lt(growth, 4 * 1024^2, label = line)
  }
})
