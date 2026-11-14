# The closures cfunction(), mir_compile() and mir_function() return
# (#23): they keep their context alive, a restored one fails with a class,
# and a forked child gets the parent's answer.

test_that("a closure keeps its context through rm() and gc()", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  for (nm in names(closure_cases())) {
    case <- closure_cases()[[nm]]
    f <- case$make()
    g <- f
    rm(f)
    gc()
    gc()
    expect_identical(g(case$arg), case$want, info = nm)
  }
})

test_that("a closure restored from a file fails with a class", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  for (nm in names(closure_cases())) {
    case <- closure_cases()[[nm]]
    path <- tempfile(fileext = ".rds")
    saveRDS(case$make(), path)
    g <- readRDS(path)
    unlink(path)
    expect_error(g(case$arg), class = "zumir_context_error", info = nm)
  }
})

test_that("a forked child gets the parent's answer", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  skip_on_os("windows")
  for (nm in names(closure_cases())) {
    case <- closure_cases()[[nm]]
    f <- case$make()
    job <- parallel::mcparallel(f(case$arg))
    got <- parallel::mccollect(job)[[1]]
    expect_identical(got, case$want, info = nm)
    expect_identical(f(case$arg), case$want, info = nm)
  }
})
