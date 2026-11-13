# Contexts are freed with the R objects that hold them (design §4.3
# "Memory"): what zumir_info() reports returns to its baseline after gc().

test_that("a context's memory is freed when its R object is collected", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  gc()
  base <- zumir_info()$memory
  ctxs <- lapply(1:5, function(i) compile_model(1 + i %% 3, "native"))
  held <- zumir_info()$memory
  expect_identical(held$contexts, base$contexts + 5)
  expect_gt(held$bytes, base$bytes)
  rm(ctxs)
  gc()
  expect_identical(zumir_info()$memory, base)
})

test_that("context_free() frees at once, and the finalizer then does nothing", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  gc()
  base <- zumir_info()$memory
  ctx <- compile_model(1, "interp")
  context_free(ctx)
  expect_identical(zumir_info()$memory, base)
  rm(ctx)
  gc()
  expect_identical(zumir_info()$memory, base)
})

test_that("contexts are freed under gctorture too", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  skip_on_cran()
  gc()
  base <- zumir_info()$memory
  gctorture(TRUE)
  ctx <- compile_model(1, "native")
  out <- context_call_rhs(ctx, 1, c(100, 1), c(1.2, 2.5, 30))
  rm(ctx)
  gctorture(FALSE)
  gc()
  expect_length(out, 2L)
  expect_identical(zumir_info()$memory, base)
})
