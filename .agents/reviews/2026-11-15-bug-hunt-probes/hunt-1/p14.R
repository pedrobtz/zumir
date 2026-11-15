source("common.R")
g <- mir_compile(function(x) { if (x > 0) {} else x * 7 })
print(g(-3)); print(g(1)); print(g(-5)); print(g(2))
f <- function(n) { y <- numeric(2); z <- numeric(3); z[1] <- 9; if (n > 0) z else {} }
g <- mir_compile(f); print(g(1)); print(tryCatch(g(-1), error = function(e) conditionMessage(e))); print(f(-1))
f2 <- function(x) { s <- 0; if (x > 0) { s <- 1 } else return(5); {} }
g2 <- tryCatch(mir_compile(f2), error = function(e) conditionMessage(e)); if (is.function(g2)) { print(g2(1)); print(f2(1)) } else print(g2)
