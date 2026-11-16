source("common.R")
so <- if (nzchar(Sys.getenv("ZUMIR_LIB"))) "capi/capi_asan.so" else "capi/capi.so"
dll <- dyn.load(so)
a <- commandArgs(TRUE)
cc <- function(name, ...) .Call(getNativeSymbolInfo(name, dll)$address, ...)
switch(a[1],
  free_twice = print(cc("capi_free_twice")),
  compile_twice = { for (t in 1:2) for (ex in 0:1) { cat("tier", t, "export", ex, ": "); r <- cc("capi_compile_twice", t, ex); cat(sprintf("st1=%d f(10)=%g st2=%d g(10)=%g status=%d '%s' f again=%g\n", r[[1]], r[[2]], r[[3]], r[[4]], r[[5]], r[[6]], r[[7]])) } },
  two_modules = { for (t in 1:2) for (ex in 0:1) { cat("tier", t, "export", ex, ": "); r <- cc("capi_two_modules", t, ex); cat(sprintf("st1=%d f(10)=%g st2=%d g(10)=%g status=%d '%s' f again=%g\n", r[[1]], r[[2]], r[[3]], r[[4]], r[[5]], r[[6]], r[[7]])) } },
  tier_switch = { for (p in list(c(2,1), c(1,2), c(0,2), c(2,0), c(0,1), c(1,0))) { cat("tiers", p, ": "); r <- cc("capi_tier_switch", p[1], p[2]); cat(sprintf("st1=%d f=%g st2=%d g=%g status=%d '%s' f again=%g out=%d,%d\n", r[[1]], r[[2]], r[[3]], r[[4]], r[[5]], r[[6]], r[[7]], r[[8]], r[[9]])) } },
  misc = { for (t in 0:2) print(cc("capi_misc", t)) },
  failed = str(cc("capi_failed")),
  oom_call = { t <- as.integer(a[2]); print(cc("capi_prepare", t)); cc("capi_reset")
    .Call(zumir:::zumir_test_fail_alloc, 1)
    r <- tryCatch(cc("capi_call_held"), error = function(e) paste("R ERROR crossed consumer frame:", conditionMessage(e)))
    .Call(zumir:::zumir_test_fail_alloc, 0)
    print(r); cat("entered, cleaned:", cc("capi_flags"), "\n"); print(tryCatch(cc("capi_call_held"), error = function(e) conditionMessage(e))); cc("capi_free_held") },
  oom_thread = { t <- as.integer(a[2]); print(cc("capi_prepare", t))
    .Call(zumir:::zumir_test_fail_alloc, 1)
    r <- tryCatch(cc("capi_call_held_thread"), error = function(e) paste("R ERROR:", conditionMessage(e)))
    print(r) }
)
cat("live", live(), "END\n")
