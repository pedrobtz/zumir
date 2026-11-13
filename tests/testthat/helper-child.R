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

run_child <- function(lib, code) {
  script <- tempfile(fileext = ".R")
  on.exit(unlink(script))
  writeLines(code, script)
  rscript <- file.path(R.home("bin"), "Rscript")
  out <- suppressWarnings(system2(
    rscript,
    c("--vanilla", shQuote(script)),
    stdout = TRUE,
    stderr = TRUE,
    env = paste0("R_LIBS=", shQuote(lib))
  ))
  status <- attr(out, "status")
  list(status = if (is.null(status)) 0L else status, output = out)
}
