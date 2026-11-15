source("common.R")
tier <- commandArgs(TRUE)[1]
# outer: produces NaN warning before a long loop with polls
outer_src <- function(x, n) { y <- sqrt(x); s <- 0; i <- 0; while (i < n) { s <- s + 1; i <- i + 1 }; y + s }
inner_src <- function(a) a + 1
fo <- mir_compile(outer_src, tier = tier)
fi <- mir_compile(inner_src, tier = tier)
# baseline
w <- NULL
r <- withCallingHandlers(fo(-1, 3e5), warning = function(c) { w <<- c(w, conditionMessage(c)); invokeRestart("muffleWarning") })
cat("baseline result", r, "warnings:", w, "\n")
# now with finalizers firing during the poll
count <- 0
mkfin <- function() { e <- new.env(); reg.finalizer(e, function(e) { count <<- count + 1; fi(1) }); NULL }
w <- NULL
for (k in 1:50) mkfin()
gctorture(TRUE)
r <- withCallingHandlers(fo(-1, 3e5), warning = function(c) { w <<- c(w, conditionMessage(c)); invokeRestart("muffleWarning") })
gctorture(FALSE)
cat("torture result", r, "warnings:", w, " finalizers run:", count, "\n")
