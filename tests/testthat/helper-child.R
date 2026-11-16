# Child R processes for tests that must not run in the session running the
# suite: loading and unloading the package, and (from Stage 3) anything that
# could end the process. The child loads the installed package, the one
# R CMD check built; under devtools::load_all() there is no installed copy,
# and installed_lib() returns NULL so the caller can skip.
installed_lib <- function() {
  path <- find.package("zumir")
  if (!file.exists(file.path(path, "Meta", "package.rds"))) {
    return(NULL)
  }
  dirname(path)
}

# `code` runs in a fresh Rscript with `lib` first on the library path. The
# path is set inside the script, not through system2(env =): on Windows
# that prepends `R_LIBS=...` to the command line, which only commands such
# as R and make accept, and Rscript does not.
# `timeout` (seconds, 0 for none) ends a child that hangs, with status 124,
# so a regression fails instead of stalling the suite.
run_child <- function(lib, code, timeout = 0) {
  script <- tempfile(fileext = ".R")
  on.exit(unlink(script))
  writeLines(
    c(sprintf(".libPaths(c(%s, .libPaths()))", deparse(lib)), code),
    script
  )
  rscript <- file.path(R.home("bin"), "Rscript")
  out <- suppressWarnings(system2(
    rscript,
    c("--vanilla", shQuote(script)),
    stdout = TRUE,
    stderr = TRUE,
    timeout = timeout
  ))
  status <- attr(out, "status")
  list(status = if (is.null(status)) 0L else status, output = out)
}

# Whether the run is on r-actions' full profile. The callers pass
# `profile: auto`, and the R CMD check, sanitizer and valgrind legs export
# that input, not what it resolved to, so "auto" is resolved here as
# r-actions resolves it: full on a dispatch or a schedule, and on a pull
# request labelled full-ci (the label is in the event's payload). Locally,
# R_ACTIONS_PROFILE=full asks for it.
full_profile <- function() {
  p <- Sys.getenv("R_ACTIONS_PROFILE")
  if (identical(p, "full")) {
    return(TRUE)
  }
  if (!identical(p, "auto")) {
    return(FALSE)
  }
  event <- Sys.getenv("GITHUB_EVENT_NAME")
  if (event %in% c("workflow_dispatch", "schedule")) {
    return(TRUE)
  }
  path <- Sys.getenv("GITHUB_EVENT_PATH")
  if (event == "pull_request" && nzchar(path) && file.exists(path)) {
    payload <- paste(readLines(path, warn = FALSE), collapse = "\n")
    return(grepl("\"name\"[[:space:]]*:[[:space:]]*\"full-ci\"", payload))
  }
  FALSE
}

# At most the baseline after a gc(): a context another test left (kept
# alive on a leg that keeps byte-code constants) may be collected
# meanwhile, now that the suite runs shuffled; a leak here would keep ours.
expect_memory_at_most <- function(base) {
  m <- zumir_info()$memory
  expect_lte(m$contexts, base$contexts)
  expect_lte(m$bytes, base$bytes)
}
