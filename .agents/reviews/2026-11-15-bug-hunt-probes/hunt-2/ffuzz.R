# Random float/double expression fuzzer (no int conversions out of range): cfunction() vs cc.
source("harness.R")
args <- commandArgs(TRUE); seed0 <- as.integer(args[1]); nprog <- as.integer(args[2])
consts <- c("0.1f", "0.1", "1e30f", "3.0", "1.5", "16777217.0f", "0.3f", "1e-40f", "2.5e-310", "1e300", "0.5f", "-0.0", "-0.0f", "7.0f", "1.0f", "3.4e38f", "1e-30f", "0x1p-149f", "0x1.fffffep127f", "33554433.0", "(float)2147483647", "(double)4294967295u", "(double)9007199254740993LL", "(float)18446744073709551615ULL", "(float)4294967295u", "(float)9007199254740993LL", "1e-5f", "0.7", "(1.0f/3)", "(1.0/3)", "(float)100u", "-5.0")
vars <- c(paste0("f", 1:4), paste0("d", 1:4), "(float)i1", "(double)u1", "(float)l1", "(double)l1", "(float)u1")
gen <- function(d) {
  if (d <= 0 || runif(1) < 0.2) return(if (runif(1) < 0.6) sample(vars, 1) else sample(consts, 1))
  op <- sample(c("bin", "bin", "bin", "bin", "neg", "castf", "castd", "cmp", "tern", "call"), 1)
  a <- gen(d - 1)
  switch(op,
    bin = sprintf("(%s %s %s)", a, sample(c("+", "-", "*", "/"), 1), gen(d - 1)),
    neg = sprintf("(-%s)", a), castf = sprintf("((float)%s)", a), castd = sprintf("((double)%s)", a),
    cmp = sprintf("((float)(%s %s %s))", a, sample(c("<", "<=", ">", ">=", "==", "!="), 1), gen(d - 1)),
    tern = sprintf("(%s %s %s ? %s : %s)", a, sample(c("<", ">", "==", "!="), 1), gen(d - 1), gen(d - 1), gen(d - 1)),
    call = sprintf("%s(%s)", sample(c("sqrt", "fabs", "floor", "ceil", "trunc"), 1), a))
}
make_prog <- function(seed) {
  set.seed(seed)
  l <- c("void f(int n, const double *x, double *y) {", "  int k = 0;",
    sprintf("  float f%d = (float)x[%d];", 1:4, sample(0:39, 4)), sprintf("  double d%d = x[%d];", 1:4, sample(0:39, 4)),
    sprintf("  int i1 = %d; unsigned u1 = %su; long long l1 = %sLL;", sample(c(-7, 3, 16777217, 2147483647, -2147483647), 1), sample(c("3", "4000000000", "16777217"), 1), sample(c("-9007199254740993", "123456789012", "5"), 1)))
  for (s in 1:30) {
    e <- gen(sample(2:4, 1)); r <- runif(1)
    if (r < 0.25) { v <- sample(vars[1:8], 1); l <- c(l, sprintf("  %s = %s; y[k++] = %s;", v, e, v)) }
    else if (r < 0.4) { v <- sample(vars[1:8], 1); l <- c(l, sprintf("  %s %s %s; y[k++] = %s;", v, sample(c("+=", "-=", "*=", "/="), 1), e, v)) }
    else l <- c(l, sprintf("  y[k++] = %s; y[k++] = sizeof(%s);", e, e))
  }
  paste(c(l, "}"), collapse = "\n")
}
dir <- file.path(H, "hunt-2", "fuzzbuild"); dir.create(dir, showWarnings = FALSE)
nbad <- 0
for (p in seq_len(nprog)) {
  seed <- seed0 * 100000L + p; src <- make_prog(seed); nm <- paste0("ff", seed)
  r0 <- cc_run(nm, src, "O0", dir); r2 <- cc_run(nm, src, "O2", dir)
  unlink(Sys.glob(file.path(dir, paste0(nm, "*"))))
  if (is.null(r0)) { cat("cc failed", seed, "\n"); next }
  if (!identical(bits(r0), bits(r2))) { cat("skip (cc O0!=O2)", seed, "\n"); next }
  writeLines(src, file.path(H, "hunt-2", sprintf("ffuzz-current-%d.c", seed0)))
  zi <- zu_run(src, "interp"); zn <- zu_run(src, "native")
  if (inherits(zn, "err") || inherits(zi, "err") || !identical(bits(r0), bits(zn)) || !identical(bits(r0), bits(zi))) {
    nbad <- nbad + 1; f <- file.path(H, "hunt-2", "fuzzfail", paste0(nm, ".c")); dir.create(dirname(f), showWarnings = FALSE); writeLines(src, f)
    saveRDS(list(X = X, cc = r0, zn = zn, zi = zi), sub(".c$", ".rds", f))
    cat("FAIL", seed, if (inherits(zn, "err")) zn else paste("native diff at", paste(which(bits(r0) != bits(zn)) - 1, collapse = ",")), if (inherits(zi, "err")) zi else paste("interp diff at", paste(which(bits(r0) != bits(zi)) - 1, collapse = ",")), "\n")
  }
}
cat("done", nprog, "programs,", nbad, "failures\n")
