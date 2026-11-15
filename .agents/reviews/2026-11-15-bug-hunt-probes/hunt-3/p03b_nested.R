source("common.R")
tier <- commandArgs(TRUE)[1]
outer_src <- function(x, n) { y <- sqrt(x); s <- 0; i <- 0; while (i < n) { s <- s + 1; i <- i + 1 }; y + s }
inner_src <- function(a) a + 1
fo <- mir_compile(outer_src, tier = tier)
fi <- mir_compile(inner_src, tier = tier)
count <- 0; depth_seen <- integer()
chain <- function() { e <- new.env(); reg.finalizer(e, function(e) { count <<- count + 1; depth_seen <<- c(depth_seen, sys.nframe()); fi(1); chain() }); NULL }
chain()
w <- NULL
gctorture(TRUE)
r <- withCallingHandlers(fo(-1, 3e5), warning = function(c) { w <<- c(w, conditionMessage(c)); invokeRestart("muffleWarning") })
gctorture(FALSE)
cat("torture result", r, "warnings:", if (is.null(w)) "<NONE - LOST>" else w, " finalizers run:", count, "\n")
print(table(depth_seen))
