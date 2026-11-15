source("common.R")
depth <- function() tryCatch(zumir:::test_invalid_module("syntax"), error = function(e) e$trap$depth)
for (tier in c("native", "interp")) {
  cat("=====", tier, "\n")
  # later-call allocation on interp?
  f <- mk("mirf", tier); f(1,2); .Call(zumir:::zumir_test_fail_alloc, 1); chk("2nd call with fail@1", f(1,2)); .Call(zumir:::zumir_test_fail_alloc, 0)
  loop <- mir_compile(function(n) { s <- 0; i <- 0; while (i < n) { s <- s + i; i <- i + 1 }; s }, tier = tier)
  nanf <- mir_compile(function(x) sqrt(x) + 1, tier = tier)
  modf <- mir_compile(function(x, y) x %% y, tier = tier)
  want <- loop(1e5)
  # time limit
  r <- tryCatch({ setTimeLimit(elapsed = 0.3, transient = TRUE); loop(1e12) }, error = function(e) paste0("ERR<", class(e)[1], "> ", conditionMessage(e)))
  setTimeLimit()
  cat("time limit:", r, "\n")
  chk("after time limit loop", identical(loop(1e5), want)); chk("depth", depth())
  chk("nan no stale poll", nanf(4))
  # time limit then a function w/o loops: poll message stale?
  # warn=2
  op <- options(warn = 2)
  chk("warn2 nan", nanf(-1)); chk("warn2 next ok call", nanf(4)); chk("warn2 fmod", modf(1e300, 3)); chk("after fmod", modf(7, 3))
  options(op)
  w <- NULL; r <- withCallingHandlers(nanf(4), warning = function(c) { w <<- c(w, conditionMessage(c)); invokeRestart("muffleWarning") }); cat("after warn2: result", r, "warnings", length(w), "\n")
  # error from warning handler (non-local exit from warning()), then next
  r <- tryCatch(withCallingHandlers(nanf(-1), warning = function(c) stop("handler bails")), error = function(e) conditionMessage(e)); cat("handler:", r, "\n")
  w <- NULL; r <- withCallingHandlers(modf(7, 3), warning = function(c) { w <<- c(w, conditionMessage(c)); invokeRestart("muffleWarning") }); cat("next call: result", r, "warnings", length(w), "\n")
  # runtime error status then next call
  idx <- mir_compile(function(x, i) x[i], tier = tier)
  chk("index error", idx(c(1,2,3), 7)); chk("after index error", idx(c(1,2,3), 2))
  # runtime error after a NaN warning was flagged: flag must not leak into next call
  both <- mir_compile(function(x, i) { y <- sqrt(x[1]); y + x[i] }, tier = tier)
  chk("nan then index error", both(c(-1,2,3), 9))
  w <- NULL; r <- withCallingHandlers(both(c(4,2,3), 2), warning = function(c) { w <<- c(w, conditionMessage(c)); invokeRestart("muffleWarning") }); cat("next call: result", r, "warnings", length(w), "\n")
  # interrupt flagged then time: nan flag + interrupt
  slow <- mir_compile(function(x, n) { y <- sqrt(x); i <- 0; while (i < n) i <- i + 1; y }, tier = tier)
  r <- tryCatch({ setTimeLimit(elapsed = 0.3, transient = TRUE); slow(-1, 1e12) }, error = function(e) paste0("ERR<", class(e)[1], "> ", conditionMessage(e)), warning = function(w) paste("WARN", conditionMessage(w)))
  setTimeLimit(); cat("nan+limit:", r, "\n")
  w <- NULL; r <- withCallingHandlers(slow(4, 10), warning = function(c) { w <<- c(w, conditionMessage(c)); invokeRestart("muffleWarning") }); cat("next call: result", r, "warnings", length(w), "\n")
  cat("live", live(), "\n")
}
