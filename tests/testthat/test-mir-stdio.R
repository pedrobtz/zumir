# MIR's exit, abort and stderr are redirected to zumir (src/zumir_mir_stdio.h,
# design §4.2, D4). zumir_test_sink is compiled under the same macros as
# MIR's sources.

test_that("writes to stderr reach zumir's buffer, not the process's stderr", {
  path <- tempfile()
  on.exit(unlink(path))
  got <- .Call(zumir_test_sink, "héllo %d", path)
  # fprintf, fputc and perror each reach the buffer, in order, and the
  # format string is the text, not a format applied twice.
  # Compared as bytes: the buffer's bytes come back unmarked, and the
  # literal is UTF-8, so as strings they differ outside a UTF-8 locale.
  expect_identical(
    charToRaw(got),
    charToRaw(enc2utf8(paste0("héllo %d!p: ", strerror_enoent(), "\n")))
  )
})

test_that("a FILE that is not stderr is written as stdio writes it", {
  path <- tempfile()
  on.exit(unlink(path))
  .Call(zumir_test_sink, "héllo %d", path)
  expect_identical(readBin(path, "raw", 100), charToRaw("héllo %d!"))
})

test_that("exit with no trap is an R error carrying what MIR wrote", {
  expect_error(
    .Call(zumir_test_exit, "bad module", FALSE),
    "MIR stopped (status 3): bad module",
    fixed = TRUE
  )
  # The buffer belongs to the next failure only.
  path <- tempfile()
  on.exit(unlink(path))
  expect_false(grepl("bad module", .Call(zumir_test_sink, "x", path)))
})

test_that("exit and abort under a trap land in it, with what MIR wrote", {
  # abort() is exit status 134; the trailing newline is dropped.
  expect_identical(.Call(zumir_test_exit, "boom", TRUE), c("134", "boom"))
})
