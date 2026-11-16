# c1.R id tier thread x : one C-API probe in this process
H <- "/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt"
.libPaths(c(file.path(H, "hunt-2", "zlib"), file.path(H, "lib")))
suppressMessages(library(zc2))
a <- commandArgs(TRUE)
r <- probe(as.integer(a[1]), as.integer(a[2]), as.logical(a[3]), as.numeric(a[4]))
cat(sprintf("id=%s tier=%s thr=%s x=%s | fn_null=%d status=%d zst=%d tier_out=%d extra=%d value=%.17g value2=%.17g msg=[%s]\n", a[1], a[2], a[3], a[4], r$fn_null, r$status, r$zst, r$tier, r$extra, r$value, r$value2, r$msg))
invisible(gc())
