# showfail.R <seed>: print the statement(s) whose outputs differ
a <- commandArgs(TRUE)[1]
src <- readLines(sprintf("fuzzfail/fz%s.c", a)); r <- readRDS(sprintf("fuzzfail/fz%s.rds", a))
if (is.character(r$zn)) { cat(r$zn, "\n"); cat(src[1:13], sep = "\n"); q() }
d <- which(!(r$cc == r$zn | (is.nan(r$cc) & is.nan(r$zn)))) - 1
cat("X[41:50]:", format(r$X[41:50], digits = 17), "\n"); cat(src[1:12], sep = "\n"); cat("\n")
k <- 0
for (l in src[13:(length(src) - 1)]) {
  nk <- lengths(regmatches(l, gregexpr("y\\[k\\+\\+\\]", l)))
  idx <- k + seq_len(nk) - 1; k <- k + nk
  if (any(idx %in% d)) cat(sprintf("y[%s] cc=%s zumir=%s\n%s\n", paste(idx, collapse = ","), paste(format(r$cc[idx + 1], digits = 17), collapse = ","), paste(format(r$zn[idx + 1], digits = 17), collapse = ","), l))
}
