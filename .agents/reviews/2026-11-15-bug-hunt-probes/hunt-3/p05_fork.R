source("common.R")
library(parallel)
for (tier in c("native","interp")) {
  fs <- lapply(c(mirf="mirf", cfun="cfun", rf="rf"), mk, tier = tier)
  loop <- mir_compile(function(n) { s <- 0; i <- 0; while (i < n) { s <- s + i; i <- i + 1 }; s }, tier = tier)
  want <- loop(2e5)
  l0 <- live()
  res <- mclapply(c(1,2,3,4), function(i) {
    r <- vapply(fs, function(f) f(i, 2), 0)
    gc()
    g <- mk("cfun", tier); r2 <- g(i, 100)
    rm(g); gc()
    # free parent's objects in child
    lv1 <- live()
    c(r, new = r2, loop = loop(2e5) - want, tierok = zumir_tier() == "native", live = lv1, probe = { zumir:::probe_tiers(); zumir_tier() == "native" })
  }, mc.cores = 4)
  print(do.call(rbind, res))
  cat(tier, "parent after:", vapply(fs, function(f) f(1, 2), 0), "live", live() - l0, "\n")
  # child frees everything and exits via finalizers; parent still fine
  p <- mcparallel({ b <- live(); rm(fs, loop, envir = globalenv()); gc(); c(before = b, after = live()) })
  print(mccollect(p))
  cat(tier, "parent after child rm:", vapply(fs, function(f) f(1, 2), 0), loop(2e5) == want, "\n")
  # child gets error in call (alloc failure on interp), parent unaffected
  p <- mcparallel({ f <- mk("rf", tier); .Call(zumir:::zumir_test_fail_alloc, 1); tryCatch(f(1,2), error = function(e) class(e)[1]) })
  print(mccollect(p))
  cat(tier, "parent compile after child's failure:", mk("rf", tier)(1,2), "\n")
  # fork while a compile is half-way: internal context wrappers
  ctx <- zumir:::context_new(tier); zumir:::context_begin(ctx, "m")
  p <- mcparallel({ zumir:::context_emit_model(ctx, 1L); zumir:::context_end(ctx); zumir:::context_link(ctx, "rhs"); str(zumir:::context_state(ctx)); "child linked" })
  print(mccollect(p))
  print(zumir:::context_state(ctx)$linked)
  zumir:::context_emit_model(ctx, 1L); zumir:::context_end(ctx); zumir:::context_link(ctx, "rhs")
  print(zumir:::context_state(ctx)$linked)
}
