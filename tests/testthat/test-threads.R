# Threads (design §4.3 "Threads", D10): native code compiled once is shared
# by 8 threads; on the interpreter each thread compiles its own context.
# Both give the serial run's final states, bitwise.

test_that("native code compiled once gives the serial answer on 8 threads", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  skip_if(zumir_tier() == "none", "no tier runs here")
  for (model in 1:3) {
    res <- .Call(zumir_test_threads, model, 8L, 64L, 200L, "native")
    expect_true(res$ok, info = model)
    expect_true(res$identical, info = model)
  }
})

test_that("the interpreter with a context per thread gives the serial answer", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  skip_if(zumir_tier() == "none", "no tier runs here")
  gc()
  base <- .Call(zumir_test_live_arenas)
  for (model in 1:3) {
    res <- .Call(zumir_test_threads, model, 8L, 64L, 200L, "interp_per_thread")
    expect_true(res$ok, info = model)
    expect_true(res$identical, info = model)
  }
  expect_identical(.Call(zumir_test_live_arenas), base)
})
