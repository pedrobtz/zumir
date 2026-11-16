# runc.R file.c [tier]: compile and run one harness-style source with cfunction
source("harness.R"); a <- commandArgs(TRUE)
src <- paste(readLines(a[1]), collapse = "\n")
for (t in if (length(a) > 1) a[2] else c("native", "interp")) { r <- zu_run(src, t); print(if (inherits(r, "err")) r else head(r, 20)) }
