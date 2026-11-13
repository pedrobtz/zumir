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
