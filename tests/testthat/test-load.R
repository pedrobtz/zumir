test_that("the package loads, registers its routines and unloads its DLL", {
  lib <- installed_lib()
  skip_if(is.null(lib), "zumir is not installed: run under R CMD check")

  res <- run_child(
    lib,
    c(
      sprintf("library(zumir, lib.loc = %s)", deparse(lib)),
      "dll <- getLoadedDLLs()[['zumir']]",
      "cat('loaded:', !is.null(dll), '\\n')",
      "cat('dynamic:', unclass(dll)[['dynamicLookup']], '\\n')",
      "unloadNamespace('zumir')",
      "cat('namespace:', 'zumir' %in% loadedNamespaces(), '\\n')",
      "cat('dll:', 'zumir' %in% names(getLoadedDLLs()), '\\n')"
    )
  )

  expect_identical(res$status, 0L)
  expect_true("loaded: TRUE " %in% res$output)
  expect_true("dynamic: FALSE " %in% res$output)
  expect_true("namespace: FALSE " %in% res$output)
  expect_true("dll: FALSE " %in% res$output)
})

test_that("unloading with a live compiled function keeps the DLL, and exits cleanly (#62)", {
  lib <- installed_lib()
  skip_if(is.null(lib), "zumir is not installed: run under R CMD check")
  skip_if(zumir_tier() == "none", "no tier runs here")
  # The finalizers of live contexts are C in the DLL: unmapping it under
  # them crashed R at the next gc or at exit (Linux).
  res <- run_child(
    lib,
    c(
      sprintf("library(zumir, lib.loc = %s)", deparse(lib)),
      "base <- zumir_info()$memory$contexts",
      "f <- mir_compile(function(x) x + 1)",
      "cat('before:', f(1), '\\n')",
      "unloadNamespace('zumir')",
      "cat('dll:', 'zumir' %in% names(getLoadedDLLs()), '\\n')",
      "invisible(gc())",
      sprintf("library(zumir, lib.loc = %s)", deparse(lib)),
      "cat('again:', f(2), '\\n')",
      "before <- zumir_info()$memory$contexts",
      "rm(f); invisible(gc()); invisible(gc())",
      "cat('contexts:', zumir_info()$memory$contexts == before - 1L, '\\n')",
      "g <- mir_compile(function(x) x * 2)",
      "cat('new:', g(3), '\\n')"
    ),
    timeout = 120
  )
  expect_identical(res$status, 0L)
  expect_true("before: 2 " %in% res$output)
  expect_true("dll: TRUE " %in% res$output)
  expect_true("again: 3 " %in% res$output)
  expect_true("contexts: TRUE " %in% res$output)
  expect_true("new: 6 " %in% res$output)
})
