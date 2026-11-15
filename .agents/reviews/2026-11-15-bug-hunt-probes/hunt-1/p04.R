source("common.R")
try1 <- function(expr) { r <- tryCatch(expr, error = function(e) paste0("ERR<", class(e)[1], "> ", conditionMessage(e))); if (is.function(r)) "compiled" else r }
cat("minus0 class:", try1(mir_compile(function(x) `-`() + x)), "\n")
cat("isna0:", try1(mir_compile(function(x) as.double(is.na()) + x)), "\n")
cat("if0:", try1(mir_compile(function(x) { `if`(); x })), "\n")
cat("if1:", try1(mir_compile(function(x) { `if`(x > 1); x })), "\n")
cat("for2:", try1(mir_compile(function(x) { `for`(i, 1:3); x })), "\n")
cat("while1:", try1(mir_compile(function(x) { `while`(x > 1); x })), "\n")
cat("asg1:", try1(mir_compile(function(x) { `<-`(a); x })), "\n")
cat("asg3:", try1(mir_compile(function(x) { `<-`(a, 1, 2); a })), "\n")
cat("brk1:", try1(mir_compile(function(x) { while (TRUE) `break`(1); x })), "\n")
cat("len0:", try1(mir_compile(function(x) as.double(length()) + x)), "\n")
cat("idx1:", try1(mir_compile(function(x) `[`(x))), "\n")
cat("num0:", try1(mir_compile(function(x) {y <- numeric(); y})), "\n")
cat("seqlen0:", try1(mir_compile(function(x) {for (i in seq_len()) x; x})), "\n")
cat("strconst:", try1(mir_compile(function(x) {"a"; x})), "\n")
cat("null:", try1(mir_compile(function(x) {NULL; x})), "\n")
cat("cplx:", try1(mir_compile(function(x) x + 1i)), "\n")
cat("empty:", try1(mir_compile(function(x) {})), "\n")
cat("nobody:", try1(mir_compile(function(x) NULL)), "\n")
cat("noargs:", try1(mir_compile(function() 1)), "\n")
cat("prim:", try1(mir_compile(sum)), "\n")
cat("nonfun:", try1(mir_compile("a")), "\n")
cat("dflt:", try1(mir_compile(function(x = 1) x)), "\n")
cat("dots:", try1(mir_compile(function(...) 1)), "\n")
cat("global:", try1(mir_compile(function(x) x + k)), "\n")
cat("pi:", try1(mir_compile(function(x) x + pi)), "\n")
cat("T:", try1(mir_compile(function(x) x + T)), "\n")
cat("recompile:", try1(mir_compile(mir_compile(function(x) x + 1))), "\n")
for (t in list(c("native", "interp"), NA, NA_character_, 1, "none", "NATIVE", factor("native"), character(), TRUE, list("native"), "nat")) cat("tier", deparse(t), ":", try1(mir_compile(function(x) x, tier = t)), "|", try1(mir_function("m: module\n export f\nf: func d, d:a\n ret a\n endfunc\n endmodule", "f", tier = t)), "|", try1(cfunction("double f(double a) { return a; }", tier = t)), "\n")
# D22
e <- new.env(); assign("exp", function(x) 42, e); f <- function(x) exp(x); environment(f) <- e
cat("rebound exp:", try1(mir_compile(f)), "\n")
e <- new.env(); assign("exp", 3, e); f <- function(x) exp(x); environment(f) <- e
cat("var exp:", try1(mir_compile(f)), " R:", f(1), "\n")
e <- new.env(); assign("+", function(a, b) 42, e); f <- function(x) x + 1; environment(f) <- e
cat("rebound +:", try1(mir_compile(f)), "\n")
e <- new.env(); assign("{", function(...) 42, e); f <- function(x) { x }; environment(f) <- e
cat("rebound {:", try1(mir_compile(f)), "\n")
e <- new.env(); assign("(", function(...) 42, e); f <- function(x) (x); environment(f) <- e
cat("rebound (:", try1(mir_compile(f)), "\n")
e <- new.env(); assign("numeric", function(...) c(9, 9), e); f <- function(x) { y <- numeric(1); y }; environment(f) <- e
cat("rebound numeric:", try1(mir_compile(f)), "\n")
e <- new.env(); assign("length", function(...) 7L, e); f <- function(x) { y <- numeric(length(x)); y }; environment(f) <- e
cat("rebound length:", try1(mir_compile(f)), "\n")
e <- new.env(); assign("return", function(...) 7L, e); f <- function(x) { return(x) }; environment(f) <- e
cat("rebound return:", try1(mir_compile(f)), "\n")
e <- new.env(); assign("if", function(...) 7L, e); f <- function(x) { if (x > 1) 1 else 2 }; environment(f) <- e
cat("rebound if:", try1(mir_compile(f)), "\n")
e <- new.env(); assign("TRUE", 0, e); 
cat("local exp:", try1(mir_compile(function(x) { exp <- 2; exp(x) + exp })), "\n")
cat("arg exp:", try1(mir_compile(function(exp) exp(exp))), "\n")
cat("arg c:", try1(mir_compile(function(c) c + 1)), "\n")
cat("loopvar exp:", try1(mir_compile(function(x) { s <- 0; for (exp in 1:3) s <- s + exp(1); s })), "\n")
cat("loopvar length:", try1(mir_compile(function(x) { s <- 0; for (length in 1:3) s <- s + length(x); s })), "\n")
# size expr evaluated in baseenv: rebinding of * in f's env?
e <- new.env(); assign("*", function(a, b) 1, e); f <- function(n) { y <- numeric(n * 2); y }; environment(f) <- e
cat("rebound * in size:", try1(mir_compile(f)), " R:", deparse(f(2)), "\n")
e <- new.env(); assign("%/%", function(a, b) 1, e); f <- function(n) { y <- numeric(n %/% 2); y }; environment(f) <- e
cat("rebound %/% in size:", try1(mir_compile(f)), " R:", deparse(f(6)), "\n")
g <- tryCatch(mir_compile(f), error = function(e) NULL); if (is.function(g)) cat(" Z:", deparse(g(6)), "\n")
# special arg names
for (nm in c("rf_call", "list", "prog", "args", "ctx", "zumir_check", "s1", "K", "vp1", "na_d", "pc", "z1", "L1", "if", "TRUE", "é", "a b", "...1", "..1", "_x", "function")) {
  f <- eval(parse(text = sprintf("function(`%s`) `%s` + 1", nm, nm)))
  g <- tryCatch(mir_compile(f), error = function(e) conditionMessage(e))
  cat("argname", nm, ":", if (is.function(g)) deparse(tryCatch(do.call(g, stats::setNames(list(2), nm)), error = function(e) conditionMessage(e))) else g, "\n")
}
for (nm in c("s1", "K", "vp1", "na_d", "pc", "z1", "L1", "v_1", "Lst3", "rf", "zero_d", "S", "R", "VL", "TRUE.", "é", "a b")) {
  f <- eval(parse(text = sprintf("function(x) { `%s` <- x * 2; y <- 1; `%s` + y }", nm, nm)))
  cmp(paste("localname", nm), f, 3)
}
