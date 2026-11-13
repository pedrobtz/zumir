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
    # A context leaked per failure is about 97 KB, so 10^3 of them would be
    # 97 MB (measured by not finishing them). Finished, the second 10^3
    # grew by 24 to 504 KB, and by 5.9 MB under ASan, whose quarantine holds
    # freed memory; 16 MB keeps a factor of six from a leak.
    growth <- as.numeric(f[5])
    if (!is.na(growth)) expect_lt(growth, 16 * 1024^2, label = line)
  }
})

test_that("failing each of MIR's allocations in turn: an error, never a crash", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  # zumir's allocator fails its n-th allocation; past the last one MIR
  # makes, the module's own error comes back. Each run is an error of one
  # of those two types, and every context's arena is freed.
  types <- c(syntax = 1L, operand = 26L, import = 20L)
  alloc_error <- 3L # MIR_alloc_error
  on.exit(.Call(zumir_test_fail_alloc, 0))
  # Contexts other tests hold stay counted until collected.
  gc()
  base <- .Call(zumir_test_live_arenas)
  for (kind in names(types)) {
    seen <- integer()
    for (n in 1:260) {
      .Call(zumir_test_fail_alloc, n)
      cnd <- tryCatch(test_invalid_module(kind), zumir_error = identity)
      .Call(zumir_test_fail_alloc, 0)
      expect_s3_class(cnd, "zumir_mir_error")
      seen <- c(seen, cnd$mir_type)
    }
    expect_setequal(unique(seen), c(alloc_error, types[[kind]]))
    # The sweep passed MIR's last allocation, so the run ends on the
    # module's own error.
    expect_identical(seen[[260]], types[[kind]], info = kind)
  }
  expect_identical(.Call(zumir_test_live_arenas), base)
})
