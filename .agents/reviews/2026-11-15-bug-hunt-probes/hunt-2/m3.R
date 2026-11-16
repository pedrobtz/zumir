# m3.R <nargs> <tier> <type>: many-argument function, sum with weights, vs R
.libPaths("/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/devlib"); source("common.R")
a <- commandArgs(TRUE); n <- as.integer(a[1]); tier <- a[2]; ty <- a[3]
isf <- ty %in% c("d", "f")
args <- paste0(ty, ":a", 1:n, collapse = ", ")
acc <- if (isf) "d" else "i64"
body <- c(sprintf(" local %s:s, %s:t", acc, acc), if (isf) " dmov s, 0.0" else " mov s, 0")
for (i in 1:n) body <- c(body, if (ty == "d") sprintf(" dmul t, a%d, %d.0\n dadd s, s, t", i, i) else if (ty == "f") sprintf(" f2d t, a%d\n dmul t, t, %d.0\n dadd s, s, t", i, i) else sprintf(" mul t, a%d, %d\n add s, s, t", i, i))
txt <- paste0("m: module\n export f\nf: func ", acc, ", ", args, "\n", paste(body, collapse = "\n"), "\n ret s\n endfunc\n endmodule\n")
g <- try_(mir_function(txt, "f", tier = tier))
if (inherits(g, "caught")) { show("compile", g); q() }
vals <- as.list(((1:n) %% 7) - 3)
r <- try_(do.call(g, vals)); want <- sum(unlist(vals) * (1:n))
cat(sprintf("n=%d %s %s: got %s want %s %s\n", n, tier, ty, if (inherits(r, "caught")) r$msg else format(r), want, if (!inherits(r, "caught") && identical(as.numeric(r), as.numeric(want))) "OK" else "MISMATCH"))
rm(g); invisible(gc()); cat("gc ok\n")
