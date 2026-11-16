source("common.R")
set.seed(as.integer(Sys.getenv("SEED", "11")))
leaf <- function() sample(list(quote(x), quote(y), quote(i), quote(j), quote(length(v)), quote(v[j]), quote(v[i]), 2L, 3L, -1L, 0L, 2147483647L, 46341L, 0, -0, 0.5, 2, -3, 1e308, NA_real_, NaN, Inf, TRUE, FALSE, NA), 1)[[1]]
gen <- function(d) {
  if (d <= 0 || runif(1) < 0.25) return(leaf())
  k <- sample(12, 1)
  if (k <= 5) return(call(sample(c("+", "-", "*", "/", "^", "%%", "%/%"), 1), gen(d - 1), gen(d - 1)))
  if (k == 6) return(call(sample(c("==", "!=", "<", "<=", ">", ">="), 1), gen(d - 1), gen(d - 1)))
  if (k == 7) return(call(sample(c("&&", "||"), 1), gen(d - 1), gen(d - 1)))
  if (k == 8) return(call(sample(c("!", "-", "+", "is.na", "is.nan", "is.finite", "as.double", "("), 1), gen(d - 1)))
  if (k == 9) return(call(sample(c("min", "max"), 1), gen(d - 1), gen(d - 1)))
  if (k == 10) return(call(sample(c("exp", "log", "sqrt", "abs", "floor", "ceiling", "trunc", "sin", "log1p", "expm1", "cos", "tan"), 1), gen(d - 1)))
  call("as.double", gen(d - 1))
}
ncomp <- 0; nbad <- 0
for (k in 1:4000) {
  e <- gen(3); cnd <- gen(2)
  f <- eval(bquote(function(x, y, v) { s <- 0; for (i in 1:3) { for (j in seq_len(2)) { if (.(cnd)) next; s <- s + as.double(.(e)) } }; s }))
  g <- tryCatch(mir_compile(f), zumir_compile_error = function(e) NULL, error = function(e) { cat("NONCLASSED compile error:", conditionMessage(e), deparse(e), "\n"); NULL })
  if (is.null(g)) next
  ncomp <- ncomp + 1
  for (a in list(list(1.5, -2, c(1, 2, 3)), list(NA_real_, 0, c(NaN, -0, 4)), list(-0, Inf, c(0.5, NA, -1e300)), list(3, 2, c(2, 2, 2)))) {
    r <- do.call(run, c(list(f), a)); z <- do.call(run, c(list(g), a))
    if (!z$ok) next   # zumir may error where R answers
    ok <- r$ok && identical(r$w, z$w) && (identical(r$v, z$v, num.eq = FALSE) || (is.double(r$v) && is.nan(r$v) && is.nan(z$v)))
    if (!ok) { nbad <- nbad + 1; if (nbad <= 12) cat("DIFF\n  e:", deparse(e), "\n  cnd:", deparse(cnd), "\n  args:", deparse(a), "\n  R:", deparse(r), "\n  Z:", deparse(z), "\n") }
  }
}
cat("compiled", ncomp, "bad", nbad, "\n")
