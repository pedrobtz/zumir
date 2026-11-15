.libPaths("/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/devlib")
suppressMessages(library(zumir))
try_ <- function(expr) tryCatch(expr, error = function(e) structure(list(cls = class(e), msg = conditionMessage(e)), class = "caught"),
  warning = function(w) structure(list(cls = class(w), msg = conditionMessage(w)), class = "caught"))
show <- function(label, v) { cat("##", label, "\n"); if (inherits(v, "caught")) cat("   <", v$cls[1], "> ", v$msg, "\n", sep = "") else { print(v) } }
