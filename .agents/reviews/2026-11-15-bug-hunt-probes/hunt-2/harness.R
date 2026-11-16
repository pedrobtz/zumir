# Differential harness: cfunction() (native, interp) against cc -O0/-O2.
# Each test: C source defining  void f(int n, const double *x, double *y)
# y has NY doubles, zero-filled.
H <- "/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt"
.libPaths(file.path(H, "lib"))
suppressMessages(library(zumir))
NY <- 400L
X <- c(0, -0, 1, -1, 0.5, -0.5, 2.5, -2.5, 1e308, -1e308, 5e-324, -5e-324,
  2.2250738585072014e-308, Inf, -Inf, NaN, 3, 7, -7, 2147483647, -2147483648,
  4294967295, 9007199254740993, 1e19, 0.1, 100, 255, 256, 65535, 32768, 1e-10,
  123456789.123, 1.5, -1.5, 3.5, -3.5, 1e-320, 710, -745.2, 0.75)
bits <- function(v) {
  r <- vapply(v, function(d) paste(rev(as.character(writeBin(d, raw()))), collapse = ""), "")
  r[is.nan(v)] <- "NaN"
  r
}
cc_run <- function(name, src, opt, dir) {
  cfile <- file.path(dir, paste0(name, opt, ".c"))
  so <- file.path(dir, paste0(name, opt, ".dylib"))
  writeLines(c("#include <stddef.h>", "#include <stdint.h>", "#include <stdbool.h>",
    "#include <stdarg.h>", "#include <float.h>", "#include <limits.h>",
    "#include <math.h>", "#include <string.h>", src,
    "void f_wrap(int *n, double *x, double *y) { f(*n, x, y); }"), cfile)
  rc <- system2("cc", c(paste0("-", opt), "-w", "-ffp-contract=off", "-shared", "-o", so, cfile),
    stdout = FALSE, stderr = file.path(dir, paste0(name, opt, ".err")))
  if (rc != 0) return(NULL)
  dll <- dyn.load(so)
  on.exit(dyn.unload(so))
  .C(getNativeSymbolInfo("f_wrap", dll)$address, length(X), as.double(X), y = double(NY), NAOK = TRUE)$y
}
zu_run <- function(src, tier) {
  tryCatch({
    f <- cfunction(src, out = c(y = NY), n = "n", tier = tier)
    f(x = X)
  }, error = function(e) structure(paste(class(e)[1], conditionMessage(e)), class = "err"))
}
run_tests <- function(tests, tag) {
  dir <- file.path(H, "hunt-2", "build"); dir.create(dir, showWarnings = FALSE)
  for (nm in names(tests)) {
    src <- tests[[nm]]
    cat("==", nm, "\n"); flush(stdout())
    r0 <- cc_run(paste0(tag, "_", nm), src, "O0", dir)
    r2 <- cc_run(paste0(tag, "_", nm), src, "O2", dir)
    if (is.null(r0)) { cat("   CC FAILED TO COMPILE\n"); }
    zn <- zu_run(src, "native"); zi <- zu_run(src, "interp")
    if (inherits(zn, "err")) { cat("   ZUMIR native ERR:", zn, "\n") }
    if (inherits(zi, "err")) { cat("   ZUMIR interp ERR:", zi, "\n") }
    if (is.null(r0) || inherits(zn, "err") || inherits(zi, "err")) next
    if (!identical(bits(r0), bits(r2))) cat("   note: cc O0 != O2 (UB / fp?) at", which(bits(r0) != bits(r2)) - 1, "\n")
    for (p in list(list("native", zn), list("interp", zi))) {
      d <- which(bits(r0) != bits(p[[2]]))
      if (length(d)) {
        cat("   DIFF", p[[1]], "vs cc -O0:\n")
        for (i in d) cat(sprintf("     y[%d] cc=%.17g (%s) zumir=%.17g (%s) ccO2=%.17g\n", i - 1L, r0[i], bits(r0[i]), p[[2]][i], bits(p[[2]][i]), r2[i]))
      }
    }
  }
}
