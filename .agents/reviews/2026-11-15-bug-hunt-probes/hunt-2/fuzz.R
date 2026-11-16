# Random UB-free integer/float expression fuzzer: cfunction() vs cc.
source("harness.R")
args <- commandArgs(TRUE); seed0 <- as.integer(args[1]); nprog <- as.integer(args[2])
TY <- list(
  sc = list(c = "signed char", size = 1, sg = TRUE), uc = list(c = "unsigned char", size = 1, sg = FALSE),
  ch = list(c = "char", size = 1, sg = TRUE),
  s = list(c = "short", size = 2, sg = TRUE), us = list(c = "unsigned short", size = 2, sg = FALSE),
  i = list(c = "int", size = 4, sg = TRUE), u = list(c = "unsigned", size = 4, sg = FALSE),
  l = list(c = "long", size = 8, sg = TRUE), ul = list(c = "unsigned long", size = 8, sg = FALSE),
  ll = list(c = "long long", size = 8, sg = TRUE), ull = list(c = "unsigned long long", size = 8, sg = FALSE))
prom <- function(t) if (TY[[t]]$size < 4) "i" else t
uac <- function(a, b) {
  a <- prom(a); b <- prom(b); A <- TY[[a]]; B <- TY[[b]]
  if (a == b) return(a)
  if (A$size == B$size) {
    if (A$sg == B$sg) { # long vs long long: higher rank
      r <- c(i = 1, u = 1, l = 2, ul = 2, ll = 3, ull = 3)
      return(if (r[[a]] >= r[[b]]) a else b)
    }
    # unsigned of the higher rank
    r <- c(i = 1, u = 1, l = 2, ul = 2, ll = 3, ull = 3)
    hi <- if (r[[a]] >= r[[b]]) a else b
    return(switch(hi, i = "u", u = "u", l = "ul", ul = "ul", ll = "ull", ull = "ull"))
  }
  if (A$size > B$size) a else b
}
consts <- c("0", "1", "-1", "2", "7", "127", "128", "255", "256", "-128", "-129", "32767", "32768", "65535", "65536",
  "2147483647", "(-2147483647-1)", "0x80000000u", "0xffffffffu", "4294967296LL", "9223372036854775807LL",
  "(-9223372036854775807LL-1)", "0x8000000000000000ull", "18446744073709551615ull", "1u", "3ll", "100", "-7", "13u", "0x7fffLL", "31", "63")
ctype <- function(k) {
  if (grepl("ull$", k)) "ull" else if (grepl("LL-1\\)$|LL$|ll$", k)) "ll" else if (grepl("u$", k)) "u" else if (grepl("^'", k)) "i" else "i"
}
vars <- NULL
gen <- function(d) {
  r <- runif(1)
  if (d <= 0 || r < 0.15) {
    if (runif(1) < 0.6) { v <- sample(nrow(vars), 1); return(list(code = vars$name[v], type = vars$type[v])) }
    k <- sample(consts, 1); return(list(code = k, type = ctype(k)))
  }
  op <- sample(c("arith", "arith", "arith", "cmp", "cmp", "bit", "bit", "shift", "shift", "logic", "tern", "cast", "cast", "not", "neg", "div", "div", "fp", "comma"), 1)
  a <- gen(d - 1)
  if (op == "cast") { t <- sample(names(TY), 1); return(list(code = sprintf("(%s)%s", TY[[t]]$c, par(a)), type = t)) }
  if (op == "not") { o <- sample(c("~", "!"), 1); return(list(code = paste0(o, par(a)), type = if (o == "!") "i" else prom(a$type))) }
  if (op == "neg") {
    pt <- prom(a$type)
    if (TY[[pt]]$sg && TY[[a$type]]$size >= 4) return(a)
    return(list(code = paste0("-", par(a)), type = pt))
  }
  if (op == "fp") { # through double and back, guarded
    t <- sample(c("float", "double"), 1); b <- gen(d - 1)
    o <- sample(c("+", "-", "*", "<", "<=", "==", "!=", ">", ">="), 1)
    e <- sprintf("((%s)%s %s (%s)%s)", t, par(a), o, t, par(b))
    if (o %in% c("+", "-", "*")) {
      return(list(code = sprintf("(%s > -1e9 && %s < 1e9 ? (int)%s : 5)", e, e, e), type = "i"))
    }
    return(list(code = e, type = "i"))
  }
  b <- gen(d - 1)
  rt <- uac(a$type, b$type)
  sa <- TY[[a$type]]$size; sb <- TY[[b$type]]$size
  if (op == "arith") {
    o <- sample(c("+", "-", "*"), 1)
    ok <- !TY[[rt]]$sg || (o != "*" && sa <= 2 && sb <= 2) || (o == "*" && sa == 1 && sb == 1)
    if (!ok) { # force unsigned
      ut <- sample(c("u", "ull", "ul"), 1)
      return(list(code = sprintf("((%s)%s %s %s)", TY[[ut]]$c, par(a), o, par(b)), type = uac(ut, b$type) -> z, z = NULL)[c("code", "type")])
    }
    return(list(code = sprintf("(%s %s %s)", par(a), o, par(b)), type = rt))
  }
  if (op == "div") {
    o <- sample(c("/", "%"), 1)
    ok <- !TY[[rt]]$sg || (sa <= 2 && sb <= 2)
    if (!ok) return(list(code = sprintf("(%s ? (unsigned long long)%s %s %s : 3)", par(b), par(a), o, par(b)), type = uac("ull", uac("ull", b$type))))
    return(list(code = sprintf("(%s ? %s %s %s : 3)", par(b), par(a), o, par(b)), type = uac(rt, "i")))
  }
  if (op == "cmp") { o <- sample(c("<", "<=", ">", ">=", "==", "!="), 1); return(list(code = sprintf("(%s %s %s)", par(a), o, par(b)), type = "i")) }
  if (op == "bit") { o <- sample(c("&", "|", "^"), 1); return(list(code = sprintf("(%s %s %s)", par(a), o, par(b)), type = rt)) }
  if (op == "logic") { o <- sample(c("&&", "||"), 1); return(list(code = sprintf("(%s %s %s)", par(a), o, par(b)), type = "i")) }
  if (op == "comma") { return(list(code = sprintf("(%s, %s)", par(a), par(b)), type = b$type)) }
  if (op == "shift") {
    pt <- prom(a$type); m <- if (TY[[pt]]$size == 8) 63 else 31
    if (runif(1) < 0.5) return(list(code = sprintf("(%s >> (%s & %d))", par(a), par(b), m), type = pt))
    if (TY[[pt]]$sg) { ut <- if (TY[[pt]]$size == 8) "ull" else "u"; return(list(code = sprintf("((%s)%s << (%s & %d))", TY[[ut]]$c, par(a), par(b), m), type = ut)) }
    return(list(code = sprintf("(%s << (%s & %d))", par(a), par(b), m), type = pt))
  }
  if (op == "tern") { cnd <- gen(d - 1); return(list(code = sprintf("(%s ? %s : %s)", par(cnd), par(a), par(b)), type = rt)) }
}
par <- function(e) if (grepl("^[A-Za-z0-9_]+$", e$code)) e$code else paste0("(", e$code, ")")
make_prog <- function(seed) {
  set.seed(seed)
  nv <- 10
  tys <- sample(names(TY), nv, replace = TRUE)
  vars <<- data.frame(name = paste0("v", seq_len(nv)), type = tys, stringsAsFactors = FALSE)
  lines <- "void f(int n, const double *x, double *y) {\n  int k = 0;"
  for (j in seq_len(nv)) {
    init <- if (runif(1) < 0.5) sprintf("(%s)(long long)x[%d]", TY[[tys[j]]]$c, 40L + j - 1L) else sprintf("(%s)%s", TY[[tys[j]]]$c, sample(consts, 1))
    lines <- c(lines, sprintf("  %s v%d = %s;", TY[[tys[j]]]$c, j, init))
  }
  for (s in 1:25) {
    e <- gen(sample(2:4, 1))
    if (runif(1) < 0.3) { # assign to a variable (implicit conversion)
      j <- sample(nv, 1)
      lines <- c(lines, sprintf("  v%d = %s; y[k++] = (double)v%d;", j, e$code, j))
    } else if (runif(1) < 0.2) {
      j <- sample(nv, 1); o <- sample(c("&=", "|=", "^=", ">>="), 1)
      if (o == ">>=") lines <- c(lines, sprintf("  v%d >>= (%s & 7); y[k++] = (double)v%d;", j, par(e), j))
      else lines <- c(lines, sprintf("  v%d %s %s; y[k++] = (double)v%d;", j, o, e$code, j))
    } else {
      lines <- c(lines, sprintf("  y[k++] = (double)%s; y[k++] = sizeof(%s);", par(e), e$code))
    }
  }
  paste(c(lines, "}"), collapse = "\n")
}
dir <- file.path(H, "hunt-2", "fuzzbuild"); dir.create(dir, showWarnings = FALSE)
X <- c(X, sample(c(-1, 0, 1, 127, 128, 255, 256, -128, -129, 32767, 32768, 65535, 65536, 2147483647, -2147483648, 2147483648, 4294967295, 4294967296, -4294967297, 9007199254740992, -1e15, 12345, -99999), 10))
set.seed(seed0); X[41:50] <- sample(c(-1, 0, 1, 127, 128, 255, 256, -128, -129, 32767, 32768, 65535, 65536, 2147483647, -2147483648, 2147483648, 4294967295, 4294967296, -4294967297, 9007199254740992, -1e15, 12345, -99999), 10)
nbad <- 0
p0 <- if (length(args) > 2) as.integer(args[3]) else 1L
for (p in p0:nprog) {
  cat("p", p, "\n", file = file.path(H, "hunt-2", sprintf("fuzz-progress-%d", seed0)))
  seed <- seed0 * 100000L + p
  src <- make_prog(seed)
  nm <- paste0("fz", seed)
  r0 <- cc_run(nm, src, "O0", dir); r2 <- cc_run(nm, src, "O2", dir)
  if (is.null(r0)) { cat("cc failed", seed, "\n"); next }
  unlink(Sys.glob(file.path(dir, paste0(nm, "*"))))
  if (!identical(bits(r0), bits(r2))) { cat("skip (cc O0!=O2)", seed, "\n"); next }
  writeLines(src, file.path(H, "hunt-2", sprintf("fuzz-current-%d.c", seed0)))
  zi <- zu_run(src, "interp")
  if (inherits(zi, "err") && grepl("Division by zero", zi)) next
  if (inherits(zi, "err") || !identical(bits(r0), bits(zi))) { cat("FAIL-interp", seed, if (inherits(zi, "err")) zi else paste(which(bits(r0) != bits(zi)) - 1, collapse = ","), "\n"); dir.create(file.path(H, "hunt-2", "fuzzfail"), showWarnings = FALSE); writeLines(src, file.path(H, "hunt-2", "fuzzfail", paste0(nm, ".interp.c"))) }
  zn <- zu_run(src, "native")
  bad <- inherits(zn, "err") || inherits(zi, "err") || !identical(bits(r0), bits(zn)) || !identical(bits(r0), bits(zi))
  if (bad) {
    nbad <- nbad + 1
    f <- file.path(H, "hunt-2", "fuzzfail", paste0(nm, ".c")); dir.create(dirname(f), showWarnings = FALSE)
    writeLines(src, f)
    cat("FAIL", seed, if (inherits(zn, "err")) zn else paste("native diff at", paste(which(bits(r0) != bits(zn)) - 1, collapse = ",")),
      if (inherits(zi, "err")) zi else paste("interp diff at", paste(which(bits(r0) != bits(zi)) - 1, collapse = ",")), "\n")
    saveRDS(list(X = X, cc = r0, zn = zn, zi = zi), sub(".c$", ".rds", f))
  }
}
cat("done", nprog, "programs,", nbad, "failures\n")
