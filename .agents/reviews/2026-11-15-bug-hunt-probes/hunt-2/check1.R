# check1.R file.c : exit 0 = agree with cc, 1 = differ, 2 = zumir error, 3 = cc failed / UB; crash = signal
source("harness.R"); a <- commandArgs(TRUE)
src <- paste(readLines(a[1]), collapse = "\n")
dir <- file.path(H, "hunt-2", "build")
r0 <- cc_run("chk", src, "O0", dir); r2 <- cc_run("chk", src, "O2", dir)
if (is.null(r0) || !identical(bits(r0), bits(r2))) q(status = 3)
zn <- zu_run(src, "native"); zi <- zu_run(src, "interp")
if (inherits(zn, "err") || inherits(zi, "err")) { cat(zn, "\n"); q(status = 2) }
d <- which(bits(r0) != bits(zn) | bits(r0) != bits(zi))
if (length(d)) { for (i in d) cat(sprintf("y[%d] cc=%.17g native=%.17g interp=%.17g\n", i - 1L, r0[i], zn[i], zi[i])); q(status = 1) }
q(status = 0)
