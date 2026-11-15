.libPaths(c("/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt/lib", .libPaths()))
suppressMessages(library(zumir))
invisible(compiler::enableJIT(0))
run <- function(f, ...) {
  w <- character()
  v <- withCallingHandlers(
    tryCatch(list(ok = TRUE, v = f(...)), error = function(e) list(ok = FALSE, v = paste0("ERR<", paste(class(e)[1], collapse="/"), "> ", conditionMessage(e)))),
    warning = function(w2) { w <<- c(w, conditionMessage(w2)); invokeRestart("muffleWarning") })
  v$w <- unique(w); v
}
# compare f and its compilation on arg lists
cmp <- function(label, f, ..., routes = TRUE) {
  argsl <- list(...)
  g <- tryCatch(mir_compile(f), error = function(e) e)
  if (inherits(g, "error")) { cat(sprintf("[%s] REFUSED: %s\n", label, conditionMessage(g))); return(invisible()) }
  g2 <- tryCatch(zumir:::rf_make(f, route = "text"), error = function(e) e)
  g3 <- tryCatch(mir_compile(f, tier = "interp"), error = function(e) e)
  for (a in argsl) {
    if (!is.list(a)) a <- list(a)
    r <- do.call(run, c(list(f), a)); z <- do.call(run, c(list(g), a))
    same <- identical(r$v, z$v, num.eq = FALSE) && r$ok == z$ok && identical(r$w, z$w)
    tag <- if (same) "same" else if (r$ok && !z$ok) "zerr" else if (!r$ok && !z$ok) "botherr" else "DIFF"
    if (tag %in% c("DIFF")) {
      cat(sprintf("[%s] %s args=%s\n   R: %s w=%s\n   Z: %s w=%s\n", label, tag, paste(deparse(a), collapse=""), paste(deparse(r$v), collapse=""), paste(r$w, collapse="|"), paste(deparse(z$v), collapse=""), paste(z$w, collapse="|")))
    } else if (tag == "zerr") cat(sprintf("[%s] zerr args=%s: %s (R: %s)\n", label, paste(deparse(a), collapse=""), z$v, paste(deparse(r$v), collapse="")))
    else if (tag == "botherr") cat(sprintf("[%s] botherr args=%s: Z %s | R %s\n", label, paste(deparse(a), collapse=""), z$v, r$v))
    else cat(sprintf("[%s] same args=%s -> %s\n", label, paste(deparse(a), collapse=""), paste(deparse(z$v), collapse="")))
    for (h in list(g2, g3)) if (is.function(h)) {
      z2 <- do.call(run, c(list(h), a))
      if (!identical(z2, z)) cat(sprintf("[%s] ROUTE/TIER DIFF args=%s: %s vs %s\n", label, paste(deparse(a), collapse=""), paste(deparse(z2), collapse=""), paste(deparse(z), collapse="")))
    }
  }
  invisible(g)
}
