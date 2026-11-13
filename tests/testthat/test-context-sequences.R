# A context used out of order ends in its documented state (design §4.3
# "Lifetimes", D20, #22).

test_that("freeing twice does nothing the second time", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  ctx <- context_new("interp")
  context_free(ctx)
  expect_silent(context_free(ctx))
  expect_false(context_state(ctx)$live)
})

test_that("use after free is a zumir_context_error", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  ctx <- compile_model(1, "native")
  context_free(ctx)
  expect_error(
    context_call_rhs(ctx, 1, c(1, 1), c(1, 1, 1)),
    class = "zumir_context_error"
  )
  expect_error(context_begin(ctx), class = "zumir_context_error")
})

test_that("a context restored by readRDS is a zumir_context_error, not a crash", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  ctx <- compile_model(1, "native")
  path <- tempfile(fileext = ".rds")
  on.exit(unlink(path))
  saveRDS(ctx, path)
  back <- readRDS(path)
  expect_false(context_state(back)$live)
  expect_error(
    context_call_rhs(back, 1, c(1, 1), c(1, 1, 1)),
    class = "zumir_context_error"
  )
  expect_silent(context_free(back))
  # The original is untouched.
  expect_length(context_call_rhs(ctx, 1, c(100, 1), c(1.2, 2.5, 30)), 2L)
})

test_that("a forked child gets the parent's answer from the same context", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  skip_on_os("windows")
  skip_if_not_installed("parallel")
  ctx <- compile_model(3, "native")
  y <- c(100, 1:9)
  par <- c(2, 2.5, 30, 4, 60, 10, 0.1, 0.8, 1.5)
  parent <- context_call_rhs(ctx, 3, y, par)
  job <- parallel::mcparallel(context_call_rhs(ctx, 3, y, par))
  child <- parallel::mccollect(job)[[1]]
  expect_identical(child, parent)
})

test_that("an R error between two MIR calls leaves the context usable", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  ctx <- context_new("native")
  context_begin(ctx, "m")
  expect_error(stop("an R error mid-build"), "mid-build")
  context_emit_model(ctx, 1)
  context_end(ctx)
  context_link(ctx, "rhs")
  ref <- compile_model(1, "native")
  y <- c(100, 3)
  par <- c(1.2, 2.5, 30)
  expect_identical(
    context_call_rhs(ctx, 1, y, par),
    context_call_rhs(ref, 1, y, par)
  )
})

test_that("after a MIR error the context is failed: later calls error, free works", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  gc()
  base <- zumir_info()$memory
  ctx <- context_new("interp")
  expect_error(
    context_scan(ctx, "m: module\n bogus\nendmodule\n"),
    class = "zumir_mir_error"
  )
  expect_true(context_state(ctx)$failed)
  expect_error(context_begin(ctx), class = "zumir_context_error")
  expect_error(context_link(ctx, "f"), class = "zumir_context_error")
  context_free(ctx)
  expect_identical(zumir_info()$memory, base)
})
