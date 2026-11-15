source("common.R")
mode <- commandArgs(TRUE)[1]
count <- 0
if (mode == "compile_msg") {
  bad <- "double f(double x) { return x + undeclared_thing; }"
  base <- tryCatch(cfunction(bad), error = function(e) e)
  cat("baseline: <", class(base)[1], "> line", base$line, "col", base$column, "msg:", conditionMessage(base), "\n")
  chain <- function() { e <- new.env(); reg.finalizer(e, function(e) { count <<- count + 1; g <- cfunction("double ok(double a) { return a; }"); chain() }); NULL }
  chain()
  gctorture(TRUE)
  r <- tryCatch(cfunction(bad), error = function(e) e)
  gctorture(FALSE)
  cat("torture:  <", class(r)[1], "> line", r$line, "col", r$column, "msg:", conditionMessage(r), "\n")
  cat("finalizers run:", count, " same message:", identical(conditionMessage(r), conditionMessage(base)), "\n")
} else if (mode == "compile_msg2") {
  bad <- "double f(double x) { return x + undeclared_thing; }"
  base <- tryCatch(cfunction(bad), error = function(e) e)
  chain <- function() { e <- new.env(); reg.finalizer(e, function(e) { count <<- count + 1; tryCatch(cfunction("double other(double a) { return a +; }"), error = function(e) NULL); chain() }); NULL }
  chain()
  gctorture(TRUE)
  r <- tryCatch(cfunction(bad), error = function(e) e)
  gctorture(FALSE)
  cat("baseline msg:", conditionMessage(base), "\n")
  cat("torture:  <", class(r)[1], "> line", r$line, "col", r$column, "msg:", conditionMessage(r), "\n")
} else if (mode == "poll_msg") {
  loop <- mir_compile(function(n) { s <- 0; i <- 0; while (i < n) { s <- s + i; i <- i + 1 }; s })
  fi <- mir_compile(function(a) a + 1)
  base <- tryCatch({ setTimeLimit(elapsed = 0.2, transient = TRUE); loop(1e13) }, error = function(e) paste("error:", conditionMessage(e)), interrupt = function(c) "INTERRUPT")
  setTimeLimit(); cat("baseline:", base, "\n")
  chain <- function() { e <- new.env(); reg.finalizer(e, function(e) { count <<- count + 1; fi(1); chain() }); NULL }
  chain()
  # torture switched on by the time the loop is running: use a task via the poll? simply torture the whole call
  r <- tryCatch({ setTimeLimit(elapsed = 1, transient = TRUE); gctorture(TRUE); loop(1e13) }, error = function(e) paste("error:", conditionMessage(e)), interrupt = function(c) "INTERRUPT (time limit turned into an interrupt)")
  gctorture(FALSE); setTimeLimit()
  cat("torture: ", r, " finalizers:", count, "\n")
}
