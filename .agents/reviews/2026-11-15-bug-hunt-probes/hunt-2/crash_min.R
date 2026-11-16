# Minimal: cfunction() segfaults while compiling, native tier only.
.libPaths("/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt/lib")
library(zumir)
src <- "double f(double x) { unsigned v = 127; int t = 5; if (x < 1.0) t = 7; return v & t; }"
tier <- commandArgs(TRUE)[1]
cat("compiling on", tier, "\n")
g <- cfunction(src, tier = tier)
cat("compiled\n"); print(g(0)); print(g(2))
