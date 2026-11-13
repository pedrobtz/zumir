#' Compile a numeric R function to native code
#'
#' `rfunction()` takes an R function written in a small numeric subset of R
#' and returns a function with the same arguments that runs compiled code.
#' It returns exactly what the R function returns, or an error: never a
#' different answer.
#'
#' The subset is scalar doubles, double vectors indexed in loops, conditions
#' and a table of maths. In detail:
#'
#' * Arguments are doubles without attributes. One that is indexed
#'   (`x[i]`), passed to `length()` or to `seq_along()` is a vector; any
#'   other must have length one when called. Arguments are never assigned
#'   to, have no defaults, and there is no `...`.
#' * Locals are double or logical scalars, `for` loop counters, and double
#'   vectors made by one `y <- numeric(size)` at the top of the body, before
#'   any loop or branch, changed only by `y[i] <- value`. `size` may use
#'   constants, arguments, `length()` of arguments and arithmetic. Every
#'   local is assigned on every path before it is read.
#' * `{`, `<-`, `=`, `if`/`else`, `while`, `for` over `seq_len(n)`,
#'   `seq_along(x)` and `a:b`, `break`, `next`, `return()`.
#' * `+ - * / ^ %% %/%`, unary `-`, comparisons, `!`, `&&`, `||`; `exp`,
#'   `log`, `log1p`, `expm1`, `sqrt`, `abs`, `floor`, `ceiling`, `trunc`,
#'   `sin`, `cos`, `tan`; two-argument `min` and `max`; `length`, `is.na`,
#'   `is.nan`, `is.finite`, `as.double`.
#' * The result is a double scalar or a vector local.
#'
#' Where matching R exactly would not be cheap, the compiled function
#' errors instead: an index out of range, `NA` or not a whole number; a
#' `seq_len()` or `a:b` bound that is `NA` or not whole; integer overflow;
#' an argument that is not a double, has attributes, or is a scalar of
#' another length. Names must mean base R's functions in `environment(f)`.
#' Loops can be interrupted, as R's can. Code outside the subset is a
#' compile error naming the construct.
#'
#' @param f A function.
#' @param tier `"native"` or `"interp"`; by default [zumir_tier()].
#' @returns A function of class `zumir_rfunction`, with `f`'s arguments.
#' @family compilers
#' @export
#' @examplesIf zumir_tier() != "none"
#' ss <- rfunction(function(x) {
#'   s <- 0
#'   for (i in seq_along(x)) s <- s + x[i] * x[i]
#'   s
#' })
#' ss(c(1, 2, 3))
rfunction <- function(f, tier = NULL) {
  if (!is.function(f) || is.primitive(f)) {
    rf_refuse("`f` must be an R function, not a primitive", NULL)
  }
  tier <- resolve_tier(tier)
  prog <- rf_compile(f)
  ctx <- zumir_check(.Call(zumir_rf_new, prog$text, tier))
  argn <- names(formals(f))
  g <- function() NULL
  formals(g) <- stats_setNames(rep(list(quote(expr = )), length(argn)), argn)
  # Only the arguments the body reads are forced: R never evaluates the
  # others.
  used <- argn[argn %in% c(prog$sargs, prog$vargs)]
  call_args <- as.call(c(
    quote(list),
    stats_setNames(lapply(used, as.name), used)
  ))
  body(g) <- bquote(rf_call(.(prog), .(ctx), .(call_args)))
  environment(g) <- new.env(parent = asNamespace("zumir"))
  structure(
    g,
    class = c("zumir_rfunction", "function"),
    tier = tier,
    mir = prog$text
  )
}

#' @export
print.zumir_rfunction <- function(x, ...) {
  cat(sprintf(
    "<zumir_rfunction(%s), %s>\n",
    paste(names(formals(x)), collapse = ", "),
    attr(x, "tier")
  ))
  invisible(x)
}

# ---- Errors ----

rf_refuse <- function(what, expr) {
  where <- if (is.null(expr)) "" else paste0(" in `", rf_deparse(expr), "`")
  stop(structure(
    class = c("zumir_compile_error", "zumir_error", "error", "condition"),
    list(message = paste0("rfunction(): ", what, where, "."), call = NULL)
  ))
}

rf_deparse <- function(e) {
  d <- paste(deparse(e, width.cutoff = 60L, nlines = 2L), collapse = " ")
  if (nchar(d) > 80L) paste0(substr(d, 1L, 77L), "...") else d
}

rf_runtime_error <- function(message) {
  stop(structure(
    class = c("zumir_runtime_error", "zumir_error", "error", "condition"),
    list(message = message, call = NULL)
  ))
}

# ---- The subset ----

rf_math1 <- c(
  exp = "zumir_r_exp",
  log = "zumir_r_log",
  log1p = "zumir_r_log1p",
  expm1 = "zumir_r_expm1",
  sqrt = "zumir_r_sqrt",
  abs = "zumir_r_abs",
  floor = "zumir_r_floor",
  ceiling = "zumir_r_ceil",
  trunc = "zumir_r_trunc",
  sin = "zumir_r_sin",
  cos = "zumir_r_cos",
  tan = "zumir_r_tan"
)

rf_calls <- c(
  "{",
  "(",
  "<-",
  "=",
  "if",
  "for",
  "while",
  "break",
  "next",
  "return",
  "[",
  "+",
  "-",
  "*",
  "/",
  "^",
  "%%",
  "%/%",
  "==",
  "!=",
  "<",
  "<=",
  ">",
  ">=",
  "!",
  "&&",
  "||",
  names(rf_math1),
  "min",
  "max",
  "length",
  "is.na",
  "is.nan",
  "is.finite",
  "as.double",
  "seq_len",
  "seq_along",
  ":",
  "numeric"
)

rf_status <- c(
  ok = 0L,
  na_cond = 1L,
  nan_cond = 2L,
  index = 3L,
  overflow = 4L,
  interrupt = 5L,
  seq_len = 6L,
  colon = 7L
)

# Every call head in e.
rf_heads <- function(e) {
  if (!is.call(e)) {
    return(character())
  }
  h <- if (is.symbol(e[[1]])) as.character(e[[1]]) else "<a computed function>"
  c(h, unlist(lapply(as.list(e)[-1], rf_heads), use.names = FALSE))
}

# The first call in e whose head is h, for an error's position.
rf_find_call <- function(e, h) {
  if (!is.call(e)) {
    return(NULL)
  }
  head <- if (is.symbol(e[[1]])) {
    as.character(e[[1]])
  } else {
    "<a computed function>"
  }
  if (identical(head, h)) {
    return(e)
  }
  for (a in as.list(e)[-1]) {
    found <- rf_find_call(a, h)
    if (!is.null(found)) {
      return(found)
    }
  }
  NULL
}

# Symbols used as values (not call heads) in e.
rf_symbols <- function(e) {
  if (is.symbol(e)) {
    s <- as.character(e)
    return(if (nzchar(s)) s else character())
  }
  if (!is.call(e)) {
    return(character())
  }
  unlist(lapply(as.list(e)[-1], rf_symbols), use.names = FALSE)
}

# Arguments that are vectors: indexed, or given to length() or seq_along().
rf_vector_args <- function(e, args) {
  out <- character()
  walk <- function(e) {
    if (!is.call(e)) {
      return()
    }
    h <- if (is.symbol(e[[1]])) as.character(e[[1]]) else ""
    if (
      h %in%
        c("[", "length", "seq_along") &&
        length(e) >= 2 &&
        is.symbol(e[[2]])
    ) {
      out <<- c(out, as.character(e[[2]]))
    }
    lapply(as.list(e)[-1], walk)
  }
  walk(e)
  intersect(unique(out), args)
}

# ---- The compiler ----

rf_compile <- function(f) {
  fm <- formals(f)
  argn <- names(fm)
  if ("..." %in% argn) {
    rf_refuse("`...` is not supported", NULL)
  }
  for (a in argn) {
    if (!identical(fm[[a]], quote(expr = ))) {
      rf_refuse(sprintf("argument `%s` has a default", a), NULL)
    }
  }
  body <- body(f)
  env <- environment(f)
  heads <- unique(rf_heads(body))
  for (h in heads) {
    if (!h %in% rf_calls) {
      rf_refuse(sprintf("`%s` is not in the subset", h), rf_find_call(body, h))
    }
    if (
      !identical(
        get0(h, envir = env, mode = "function"),
        get(h, envir = baseenv())
      )
    ) {
      rf_refuse(
        sprintf("`%s` does not mean base R's `%s` in environment(f)", h, h),
        NULL
      )
    }
  }
  clash <- intersect(c(argn, rf_assigned_names(body)), heads)
  if (length(clash)) {
    rf_refuse(
      sprintf(
        "`%s` names both a variable and a function the body calls",
        clash[1]
      ),
      NULL
    )
  }
  vargs <- rf_vector_args(body, argn)
  st <- new.env(parent = emptyenv())
  st$lines <- character()
  st$regs <- character() # name -> "d" or "i64"
  st$ntmp <- 0L
  st$nlab <- 0L
  st$consts <- numeric()
  st$loops <- list()
  st$locals <- list() # name -> list(t, r)
  st$assigned <- character()
  st$dead <- character()
  st$args <- argn
  st$vargs <- vargs
  # An argument the body never reads has no shape, and R never looks at
  # it (Q6): it is not checked and not passed.
  st$sargs <- setdiff(intersect(argn, rf_symbols(body)), vargs)
  st$vlocals <- character()
  st$sizes <- list()
  st$result <- NULL
  st$top <- TRUE # no loop or branch yet: numeric() allowed
  st$uses_poll <- FALSE
  rf_reg(st, "d", "na_d")
  rf_reg(st, "d", "zero_d")
  rf_reg(st, "d", "one_d")
  rf_reg(st, "i64", "pc")
  rf_reg(st, "i64", "pt")
  rf_reg(st, "i64", "pr")
  rf_ins(st, sprintf("dmov na_d, %s", rf_konst(st, NA_real_)))
  rf_ins(st, sprintf("dmov zero_d, %s", rf_konst(st, 0)))
  rf_ins(st, sprintf("dmov one_d, %s", rf_konst(st, 1)))
  rf_ins(st, "mov pc, 0")
  for (k in seq_along(st$sargs)) {
    r <- rf_reg(st, "d", sprintf("s%d", k))
    rf_ins(st, sprintf("dmov %s, d:%d(S)", r, 8L * (k - 1L)))
  }
  body_stmts <- if (is.call(body) && identical(body[[1]], as.symbol("{"))) {
    as.list(body)[-1]
  } else {
    list(body)
  }
  if (length(body_stmts) == 0L) {
    rf_refuse("the body is empty", NULL)
  }
  # Vector locals, from top-level numeric() before any loop or branch.
  rf_block(st, body_stmts, result = TRUE)
  rf_finish(st)
}

# Names assigned anywhere in e (`<-` and `=` targets).
rf_assigned_names <- function(e) {
  if (!is.call(e)) {
    return(character())
  }
  h <- if (is.symbol(e[[1]])) as.character(e[[1]]) else ""
  own <- if (h %in% c("<-", "=") && is.symbol(e[[2]])) {
    as.character(e[[2]])
  } else {
    character()
  }
  c(own, unlist(lapply(as.list(e)[-1], rf_assigned_names), use.names = FALSE))
}

rf_reg <- function(st, type, name = NULL) {
  if (is.null(name)) {
    st$ntmp <- st$ntmp + 1L
    # Not t<n>: MIR names its own temporaries so.
    name <- sprintf("z%d", st$ntmp)
  }
  st$regs[[name]] <- type
  name
}

rf_label <- function(st) {
  st$nlab <- st$nlab + 1L
  sprintf("L%d", st$nlab)
}

# `line` is forced before st$lines is read: building it may emit
# instructions of its own (a conversion in rf_as_double()), which must come
# first and must not be lost when st$lines is reassigned.
rf_ins <- function(st, line) {
  line <- paste0("  ", line)
  st$lines <- c(st$lines, line)
}

rf_place <- function(st, label) {
  st$lines <- c(st$lines, paste0(label, ":"))
}

# The constant pool: a memory operand for x, identical bits shared.
rf_konst <- function(st, x) {
  hit <- which(vapply(st$consts, identical, NA, y = x, num.eq = FALSE))
  if (length(hit)) {
    k <- hit[1]
  } else {
    st$consts <- c(st$consts, x)
    k <- length(st$consts)
  }
  sprintf("d:%d(K)", 8L * (k - 1L))
}

rf_status_exit <- function(st, name) {
  sprintf("Lst%d", rf_status[[name]])
}

# ---- Values: list(t = "d" | "i" | "l" | "v", r = register, slot = vector) ----

rf_as_double <- function(st, v, e) {
  if (v$t == "d") {
    return(v$r)
  }
  if (v$t == "i") {
    r <- rf_reg(st, "d")
    rf_ins(st, sprintf("i2d %s, %s", r, v$r))
    return(r)
  }
  if (v$t == "l") {
    r <- rf_reg(st, "d")
    done <- rf_label(st)
    rf_ins(st, sprintf("i2d %s, %s", r, v$r))
    rf_ins(st, sprintf("bne %s, %s, 2", done, v$r))
    rf_ins(st, sprintf("dmov %s, na_d", r))
    rf_place(st, done)
    return(r)
  }
  rf_refuse("a vector where a number is needed", e)
}

# A logical register (0, 1, 2 = NA) from a scalar value, as R's coercion.
rf_as_logical <- function(st, v, e) {
  if (v$t == "l") {
    return(v$r)
  }
  r <- rf_reg(st, "i64")
  if (v$t == "i") {
    rf_ins(st, sprintf("ne %s, %s, 0", r, v$r))
    return(r)
  }
  if (v$t == "d") {
    done <- rf_label(st)
    nan <- rf_reg(st, "i64")
    rf_ins(st, sprintf("dne %s, %s, %s", r, v$r, "zero_d"))
    rf_ins(st, sprintf("dne %s, %s, %s", nan, v$r, v$r))
    rf_ins(st, sprintf("bf %s, %s", done, nan))
    rf_ins(st, sprintf("mov %s, 2", r))
    rf_place(st, done)
    return(r)
  }
  rf_refuse("a vector where a logical value is needed", e)
}

rf_expr <- function(st, e) {
  if (is.numeric(e) && length(e) == 1L && is.null(attributes(e))) {
    if (is.integer(e)) {
      if (is.na(e)) {
        rf_refuse("NA_integer_ is not supported", e)
      }
      r <- rf_reg(st, "i64")
      rf_ins(st, sprintf("mov %s, %d", r, e))
      return(list(t = "i", r = r))
    }
    r <- rf_reg(st, "d")
    rf_ins(st, sprintf("dmov %s, %s", r, rf_konst(st, e)))
    return(list(t = "d", r = r))
  }
  if (is.logical(e) && length(e) == 1L) {
    r <- rf_reg(st, "i64")
    rf_ins(st, sprintf("mov %s, %d", r, if (is.na(e)) 2L else as.integer(e)))
    return(list(t = "l", r = r))
  }
  if (is.symbol(e)) {
    return(rf_name(st, as.character(e), e))
  }
  if (!is.call(e)) {
    rf_refuse("a value of a type outside the subset", e)
  }
  h <- as.character(e[[1]])
  n <- length(e) - 1L
  if (h == "(") {
    return(rf_expr(st, e[[2]]))
  }
  if (h %in% c("+", "-") && n == 1L) {
    return(rf_unary(st, h, e))
  }
  if (h %in% c("+", "-", "*", "/", "^", "%%", "%/%")) {
    return(rf_arith(st, h, e))
  }
  if (h %in% c("==", "!=", "<", "<=", ">", ">=")) {
    return(rf_compare(st, h, e))
  }
  if (h == "!") {
    return(rf_not(st, e))
  }
  if (h %in% c("&&", "||")) {
    return(rf_andor(st, h, e))
  }
  if (h %in% names(rf_math1)) {
    if (n != 1L || !is.null(names(e))) {
      rf_refuse(sprintf("`%s` takes one unnamed argument here", h), e)
    }
    x <- rf_as_double(st, rf_expr(st, e[[2]]), e)
    r <- rf_reg(st, "d")
    rf_ins(st, sprintf("call p_d1, %s, %s, %s", rf_math1[[h]], r, x))
    return(list(t = "d", r = r))
  }
  if (h %in% c("min", "max")) {
    if (n != 2L || !is.null(names(e))) {
      rf_refuse(sprintf("only two-argument `%s()` is supported", h), e)
    }
    a <- rf_expr(st, e[[2]])
    b <- rf_expr(st, e[[3]])
    if (a$t != "d" && b$t != "d") {
      rf_refuse(sprintf("`%s()` of two non-doubles is an integer in R", h), e)
    }
    r <- rf_reg(st, "d")
    rf_ins(
      st,
      sprintf(
        "call p_d2, %s, %s, %s, %s",
        if (h == "min") "zumir_r_min2" else "zumir_r_max2",
        r,
        rf_as_double(st, a, e),
        rf_as_double(st, b, e)
      )
    )
    return(list(t = "d", r = r))
  }
  if (h == "length") {
    v <- rf_vector(st, e[[2]], e)
    return(list(t = "i", r = sprintf("vl%d", v$slot)))
  }
  if (h %in% c("is.na", "is.nan", "is.finite")) {
    return(rf_is(st, h, e))
  }
  if (h == "as.double") {
    if (n != 1L) {
      rf_refuse("`as.double()` takes one argument here", e)
    }
    r <- rf_as_double(st, rf_expr(st, e[[2]]), e)
    return(list(t = "d", r = r))
  }
  if (h == "[") {
    return(rf_index_read(st, e))
  }
  rf_refuse(sprintf("`%s` cannot be used as a value", h), e)
}

rf_name <- function(st, s, e) {
  if (s %in% c("T", "F")) {
    rf_refuse("write TRUE and FALSE, not T and F", e)
  }
  if (s %in% st$sargs) {
    return(list(t = "d", r = sprintf("s%d", match(s, st$sargs))))
  }
  if (s %in% st$vargs || s %in% st$vlocals) {
    return(list(t = "v", slot = rf_slot(st, s)))
  }
  if (s %in% st$dead) {
    rf_refuse(sprintf("loop variable `%s` is read after its loop", s), e)
  }
  if (!is.null(st$locals[[s]])) {
    if (!s %in% st$assigned) {
      rf_refuse(sprintf("`%s` may be read before it is assigned", s), e)
    }
    l <- st$locals[[s]]
    return(list(t = l$t, r = l$r))
  }
  rf_refuse(sprintf("`%s` is not an argument or a local", s), e)
}

rf_slot <- function(st, s) {
  k <- match(s, c(st$vargs, st$vlocals))
  rf_slot_regs(st, k)
  k
}

# The data and length registers of vector slot k, loaded once at entry.
rf_slot_regs <- function(st, k) {
  p <- sprintf("vp%d", k)
  if (is.na(st$regs[p])) {
    rf_reg(st, "i64", p)
    rf_reg(st, "i64", sprintf("vl%d", k))
  }
}

rf_vector <- function(st, a, e) {
  if (!is.symbol(a)) {
    rf_refuse("only a vector argument or local can be given here", e)
  }
  v <- rf_name(st, as.character(a), e)
  if (v$t != "v") {
    rf_refuse(sprintf("`%s` is not a vector", as.character(a)), e)
  }
  v
}

rf_unary <- function(st, h, e) {
  v <- rf_expr(st, e[[2]])
  if (v$t == "l") {
    rf_refuse("unary minus or plus of a logical is an integer in R", e)
  }
  if (h == "+") {
    return(v)
  }
  r <- rf_reg(st, if (v$t == "d") "d" else "i64")
  rf_ins(st, sprintf("%s %s, %s", if (v$t == "d") "dneg" else "neg", r, v$r))
  list(t = v$t, r = r)
}

rf_arith <- function(st, h, e) {
  a <- rf_expr(st, e[[2]])
  b <- rf_expr(st, e[[3]])
  if (a$t == "v" || b$t == "v") {
    rf_refuse("vector arithmetic is not supported", e)
  }
  if (a$t == "l" && b$t != "d" || b$t == "l" && a$t != "d") {
    rf_refuse("arithmetic on a logical without a double is an integer in R", e)
  }
  if (a$t == "i" && b$t == "i") {
    if (h %in% c("+", "-", "*")) {
      r <- rf_reg(st, "i64")
      rf_ins(
        st,
        sprintf(
          "%s %s, %s, %s",
          c("+" = "add", "-" = "sub", "*" = "mul")[[h]],
          r,
          a$r,
          b$r
        )
      )
      # R's integers stop at +-(2^31 - 1); beyond is NA in R, an error here.
      rf_ins(
        st,
        sprintf("bgt %s, %s, 2147483647", rf_status_exit(st, "overflow"), r)
      )
      rf_ins(
        st,
        sprintf("blt %s, %s, -2147483647", rf_status_exit(st, "overflow"), r)
      )
      return(list(t = "i", r = r))
    }
    if (h %in% c("%%", "%/%")) {
      rf_refuse(sprintf("`%s` of two integers is not supported", h), e)
    }
  }
  x <- rf_as_double(st, a, e)
  y <- rf_as_double(st, b, e)
  r <- rf_reg(st, "d")
  if (h %in% c("+", "-", "*", "/")) {
    rf_ins(
      st,
      sprintf(
        "%s %s, %s, %s",
        c("+" = "dadd", "-" = "dsub", "*" = "dmul", "/" = "ddiv")[[h]],
        r,
        x,
        y
      )
    )
  } else if (h == "^") {
    # R's R_POW: x * x when y is 2, else R_pow.
    two <- rf_reg(st, "d")
    sq <- rf_label(st)
    done <- rf_label(st)
    rf_ins(st, sprintf("dmov %s, %s", two, rf_konst(st, 2)))
    rf_ins(st, sprintf("dbeq %s, %s, %s", sq, y, two))
    rf_ins(st, sprintf("call p_d2, R_pow, %s, %s, %s", r, x, y))
    rf_ins(st, sprintf("jmp %s", done))
    rf_place(st, sq)
    rf_ins(st, sprintf("dmul %s, %s, %s", r, x, x))
    rf_place(st, done)
  } else {
    rf_ins(
      st,
      sprintf(
        "call p_d2, %s, %s, %s, %s",
        if (h == "%%") "zumir_r_fmod" else "zumir_r_intdiv",
        r,
        x,
        y
      )
    )
  }
  list(t = "d", r = r)
}

rf_compare <- function(st, h, e) {
  a <- rf_expr(st, e[[2]])
  b <- rf_expr(st, e[[3]])
  if (a$t == "v" || b$t == "v") {
    rf_refuse("vector comparison is not supported", e)
  }
  op <- c(
    "==" = "eq",
    "!=" = "ne",
    "<" = "lt",
    "<=" = "le",
    ">" = "gt",
    ">=" = "ge"
  )[[h]]
  r <- rf_reg(st, "i64")
  if (a$t == "i" && b$t == "i") {
    rf_ins(st, sprintf("%s %s, %s, %s", op, r, a$r, b$r))
    return(list(t = "l", r = r))
  }
  x <- rf_as_double(st, a, e)
  y <- rf_as_double(st, b, e)
  na <- rf_label(st)
  done <- rf_label(st)
  n1 <- rf_reg(st, "i64")
  n2 <- rf_reg(st, "i64")
  rf_ins(st, sprintf("dne %s, %s, %s", n1, x, x))
  rf_ins(st, sprintf("dne %s, %s, %s", n2, y, y))
  rf_ins(st, sprintf("or %s, %s, %s", n1, n1, n2))
  rf_ins(st, sprintf("bt %s, %s", na, n1))
  rf_ins(st, sprintf("d%s %s, %s, %s", op, r, x, y))
  rf_ins(st, sprintf("jmp %s", done))
  rf_place(st, na)
  rf_ins(st, sprintf("mov %s, 2", r))
  rf_place(st, done)
  list(t = "l", r = r)
}

rf_not <- function(st, e) {
  l <- rf_as_logical(st, rf_expr(st, e[[2]]), e)
  r <- rf_reg(st, "i64")
  done <- rf_label(st)
  rf_ins(st, sprintf("mov %s, 2", r))
  rf_ins(st, sprintf("beq %s, %s, 2", done, l))
  rf_ins(st, sprintf("eq %s, %s, 0", r, l))
  rf_place(st, done)
  list(t = "l", r = r)
}

rf_andor <- function(st, h, e) {
  la <- rf_as_logical(st, rf_expr(st, e[[2]]), e)
  r <- rf_reg(st, "i64")
  done <- rf_label(st)
  short <- if (h == "&&") 0L else 1L
  rf_ins(st, sprintf("mov %s, %d", r, short))
  rf_ins(st, sprintf("beq %s, %s, %d", done, la, short))
  lb <- rf_as_logical(st, rf_expr(st, e[[3]]), e)
  rf_ins(st, sprintf("beq %s, %s, %d", done, lb, short))
  # Neither decided it: NA if either is NA, else the other value.
  rf_ins(st, sprintf("mov %s, %d", r, 1L - short))
  rf_ins(st, sprintf("bne %s, %s, 2", sprintf("%s_b", done), la))
  rf_ins(st, sprintf("mov %s, 2", r))
  rf_place(st, sprintf("%s_b", done))
  rf_ins(st, sprintf("bne %s, %s, 2", done, lb))
  rf_ins(st, sprintf("mov %s, 2", r))
  rf_place(st, done)
  list(t = "l", r = r)
}

rf_is <- function(st, h, e) {
  v <- rf_expr(st, e[[2]])
  r <- rf_reg(st, "i64")
  if (v$t == "v") {
    rf_refuse(sprintf("`%s()` of a vector is not supported", h), e)
  }
  if (v$t == "i") {
    rf_ins(st, sprintf("mov %s, %d", r, if (h == "is.finite") 1L else 0L))
    return(list(t = "l", r = r))
  }
  if (v$t == "l") {
    if (h == "is.nan") {
      rf_ins(st, sprintf("mov %s, 0", r))
    } else {
      rf_ins(
        st,
        sprintf("%s %s, %s, 2", if (h == "is.na") "eq" else "ne", r, v$r)
      )
    }
    return(list(t = "l", r = r))
  }
  if (h == "is.na") {
    rf_ins(st, sprintf("dne %s, %s, %s", r, v$r, v$r))
  } else if (h == "is.finite") {
    d <- rf_reg(st, "d")
    rf_ins(st, sprintf("dsub %s, %s, %s", d, v$r, v$r))
    rf_ins(st, sprintf("deq %s, %s, zero_d", r, d))
  } else {
    isna <- rf_reg(st, "i64")
    done <- rf_label(st)
    rf_ins(st, sprintf("dne %s, %s, %s", r, v$r, v$r))
    rf_ins(st, sprintf("bf %s, %s", done, r))
    rf_ins(st, sprintf("call p_id, zumir_r_isna, %s, %s", isna, v$r))
    rf_ins(st, sprintf("eq %s, %s, 0", r, isna))
    rf_place(st, done)
  }
  list(t = "l", r = r)
}

# The 1-based index register of x[i] in vector slot k, checked.
rf_index <- function(st, k, ie, e) {
  bad <- rf_status_exit(st, "index")
  len <- sprintf("vl%d", k)
  v <- rf_expr(st, ie)
  if (v$t == "i") {
    rf_ins(st, sprintf("blt %s, %s, 1", bad, v$r))
    rf_ins(st, sprintf("bgt %s, %s, %s", bad, v$r, len))
    return(v$r)
  }
  if (v$t != "d") {
    rf_refuse("an index must be a number", e)
  }
  n <- rf_reg(st, "i64")
  ld <- rf_reg(st, "d")
  i <- rf_reg(st, "i64")
  back <- rf_reg(st, "d")
  rf_ins(st, sprintf("dne %s, %s, %s", n, v$r, v$r))
  rf_ins(st, sprintf("bt %s, %s", bad, n))
  rf_ins(st, sprintf("dblt %s, %s, one_d", bad, v$r))
  rf_ins(st, sprintf("i2d %s, %s", ld, len))
  rf_ins(st, sprintf("dbgt %s, %s, %s", bad, v$r, ld))
  rf_ins(st, sprintf("d2i %s, %s", i, v$r))
  rf_ins(st, sprintf("i2d %s, %s", back, i))
  rf_ins(st, sprintf("dbne %s, %s, %s", bad, back, v$r))
  i
}

rf_index_read <- function(st, e) {
  if (length(e) != 3L) {
    rf_refuse("only x[i] indexing is supported", e)
  }
  v <- rf_vector(st, e[[2]], e)
  i <- rf_index(st, v$slot, e[[3]], e)
  r <- rf_reg(st, "d")
  rf_ins(st, sprintf("dmov %s, d:-8(vp%d, %s, 8)", r, v$slot, i))
  list(t = "d", r = r)
}

# ---- Statements ----

rf_block <- function(st, stmts, result = FALSE) {
  for (k in seq_along(stmts)) {
    rf_stmt(st, stmts[[k]], result = result && k == length(stmts))
  }
}

rf_stmt <- function(st, e, result = FALSE) {
  if (is.call(e)) {
    h <- as.character(e[[1]])
    if (h == "{") {
      return(rf_block(st, as.list(e)[-1], result))
    }
    if (h %in% c("<-", "=")) {
      rf_assign(st, e)
      if (result) {
        rf_refuse("the function must end in its result, not an assignment", e)
      }
      return(invisible())
    }
    if (h == "if") {
      return(rf_if(st, e, result))
    }
    if (h == "while") {
      rf_while(st, e)
      if (result) {
        rf_refuse("the function must end in its result, not a loop", e)
      }
      return(invisible())
    }
    if (h == "for") {
      rf_for(st, e)
      if (result) {
        rf_refuse("the function must end in its result, not a loop", e)
      }
      return(invisible())
    }
    if (h %in% c("break", "next")) {
      if (!length(st$loops)) {
        rf_refuse(sprintf("`%s` outside a loop", h), e)
      }
      loop <- st$loops[[length(st$loops)]]
      rf_ins(st, sprintf("jmp %s", if (h == "break") loop$brk else loop$nxt))
      return(invisible())
    }
    if (h == "return") {
      if (length(e) != 2L) {
        rf_refuse("`return()` needs one value", e)
      }
      return(rf_result(st, e[[2]]))
    }
  }
  if (result) {
    return(rf_result(st, e))
  }
  # An expression statement whose value is not used: compiled for its
  # errors and warnings, as R evaluates it.
  rf_expr(st, e)
  invisible()
}

rf_result <- function(st, e) {
  if (is.symbol(e) && as.character(e) %in% st$vlocals) {
    kind <- "vector"
    k <- rf_slot(st, as.character(e))
    rf_ins(st, sprintf("mov i64:0(R), %d", k - 1L - length(st$vargs)))
  } else if (is.symbol(e) && as.character(e) %in% st$vargs) {
    rf_refuse(
      "an argument vector cannot be the result; copy it into a local",
      e
    )
  } else {
    v <- rf_expr(st, e)
    if (v$t != "d") {
      rf_refuse(
        if (v$t == "v") {
          "a vector result must be a vector local"
        } else {
          "the result would be an integer or logical in R; use as.double()"
        },
        e
      )
    }
    kind <- "scalar"
    rf_ins(st, sprintf("dmov d:0(R), %s", v$r))
  }
  if (!is.null(st$result) && st$result != kind) {
    rf_refuse(
      "the function returns a scalar on one path and a vector on another",
      e
    )
  }
  st$result <- kind
  rf_ins(st, "ret 0")
}

rf_assign <- function(st, e) {
  lhs <- e[[2]]
  rhs <- e[[3]]
  if (is.call(lhs) && identical(lhs[[1]], as.symbol("["))) {
    return(rf_assign_index(st, lhs, rhs, e))
  }
  if (!is.symbol(lhs)) {
    rf_refuse("only a name or y[i] can be assigned", e)
  }
  s <- as.character(lhs)
  if (s %in% st$args) {
    rf_refuse(sprintf("argument `%s` is assigned to", s), e)
  }
  if (s %in% st$vlocals) {
    rf_refuse(sprintf("vector `%s` is assigned whole", s), e)
  }
  if (any(vapply(st$loops, function(l) identical(l$var, s), NA))) {
    rf_refuse(sprintf("loop variable `%s` is assigned to", s), e)
  }
  if (is.call(rhs) && identical(rhs[[1]], as.symbol("numeric"))) {
    return(rf_numeric(st, s, rhs, e))
  }
  if (s %in% st$dead) {
    st$dead <- setdiff(st$dead, s)
  }
  v <- rf_expr(st, rhs)
  if (v$t == "v") {
    rf_refuse("vectors cannot be assigned whole", e)
  }
  if (v$t == "i") {
    rf_refuse("an integer local; use as.double()", e)
  }
  l <- st$locals[[s]]
  if (is.null(l)) {
    r <- rf_reg(
      st,
      if (v$t == "d") "d" else "i64",
      sprintf("v_%d", length(st$locals) + 1L)
    )
    st$locals[[s]] <- list(t = v$t, r = r)
    l <- st$locals[[s]]
  } else if (l$t != v$t) {
    rf_refuse(
      sprintf(
        "`%s` is assigned a %s and a %s",
        s,
        rf_type_name(l$t),
        rf_type_name(v$t)
      ),
      e
    )
  }
  rf_ins(st, sprintf("%s %s, %s", if (v$t == "d") "dmov" else "mov", l$r, v$r))
  st$assigned <- union(st$assigned, s)
}

rf_type_name <- function(t) {
  c(d = "double", l = "logical", i = "integer", v = "vector")[[t]]
}

rf_numeric <- function(st, s, rhs, e) {
  if (!st$top) {
    rf_refuse("`numeric()` inside a loop or branch", e)
  }
  if (s %in% c(names(st$locals), st$vlocals)) {
    rf_refuse(sprintf("`%s` is made twice", s), e)
  }
  if (length(rhs) != 2L) {
    rf_refuse("`numeric()` needs one size", e)
  }
  size <- rhs[[2]]
  bad <- setdiff(
    all.names(size),
    c("length", "+", "-", "*", "/", "(", "%/%", st$args)
  )
  if (length(bad)) {
    rf_refuse(
      sprintf(
        "a vector size may use only constants, arguments, length() and arithmetic; not `%s`",
        bad[1]
      ),
      e
    )
  }
  st$vlocals <- c(st$vlocals, s)
  st$sizes[[s]] <- size
  rf_slot(st, s)
}

rf_assign_index <- function(st, lhs, rhs, e) {
  if (length(lhs) != 3L || !is.symbol(lhs[[2]])) {
    rf_refuse("only y[i] <- v is supported", e)
  }
  s <- as.character(lhs[[2]])
  if (s %in% st$vargs || s %in% st$sargs) {
    rf_refuse(sprintf("argument `%s` is assigned to", s), e)
  }
  if (!s %in% st$vlocals) {
    rf_refuse(sprintf("`%s` is not a vector local made by numeric()", s), e)
  }
  k <- rf_slot(st, s)
  i <- rf_index(st, k, lhs[[3]], e)
  v <- rf_as_double(st, rf_expr(st, rhs), e)
  rf_ins(st, sprintf("dmov d:-8(vp%d, %s, 8), %s", k, i, v))
}

# Jumps to `false` when the condition is FALSE, to the status exits when it
# is NA or NaN, with R's message for the condition's type.
rf_condition <- function(st, ce, false, e) {
  v <- rf_expr(st, ce)
  if (v$t == "l") {
    rf_ins(st, sprintf("beq %s, %s, 2", rf_status_exit(st, "na_cond"), v$r))
    rf_ins(st, sprintf("beq %s, %s, 0", false, v$r))
  } else if (v$t == "i") {
    rf_ins(st, sprintf("beq %s, %s, 0", false, v$r))
  } else if (v$t == "d") {
    # R's asLogicalNoNA: a double NA or NaN is "not interpretable as
    # logical"; only a logical NA is a "missing value".
    n <- rf_reg(st, "i64")
    rf_ins(st, sprintf("dne %s, %s, %s", n, v$r, v$r))
    rf_ins(st, sprintf("bt %s, %s", rf_status_exit(st, "nan_cond"), n))
    rf_ins(st, sprintf("dbeq %s, %s, zero_d", false, v$r))
  } else {
    rf_refuse("a condition must be one value", e)
  }
}

rf_if <- function(st, e, result) {
  st$top <- FALSE
  els <- rf_label(st)
  done <- rf_label(st)
  rf_condition(st, e[[2]], els, e)
  before <- st$assigned
  rf_stmt(st, e[[3]], result)
  after_then <- st$assigned
  rf_ins(st, sprintf("jmp %s", done))
  rf_place(st, els)
  st$assigned <- before
  if (length(e) == 4L) {
    rf_stmt(st, e[[4]], result)
    st$assigned <- intersect(after_then, st$assigned)
  } else {
    if (result) {
      rf_refuse("a result `if` needs an `else`", e)
    }
    st$assigned <- before
  }
  rf_place(st, done)
  if (result) rf_ins(st, "ret 0")
}

# Every 2^16 iterations of any loop, the interrupt poll (D23).
rf_poll <- function(st) {
  skip <- rf_label(st)
  rf_ins(st, "add pc, pc, 1")
  rf_ins(st, "and pt, pc, 65535")
  rf_ins(st, sprintf("bne %s, pt, 0", skip))
  rf_ins(st, "call p_i0, zumir_r_poll, pr")
  rf_ins(st, sprintf("bt %s, pr", rf_status_exit(st, "interrupt")))
  rf_place(st, skip)
}

rf_loop_body <- function(st, body, loop) {
  st$loops <- c(st$loops, list(loop))
  before <- st$assigned
  rf_stmt(st, body)
  st$assigned <- before
  st$loops <- st$loops[-length(st$loops)]
}

rf_while <- function(st, e) {
  st$top <- FALSE
  top <- rf_label(st)
  nxt <- rf_label(st)
  brk <- rf_label(st)
  rf_place(st, top)
  rf_condition(st, e[[2]], brk, e)
  rf_loop_body(st, e[[3]], list(brk = brk, nxt = nxt, var = NULL))
  rf_place(st, nxt)
  rf_poll(st)
  rf_ins(st, sprintf("jmp %s", top))
  rf_place(st, brk)
}

# A whole-number count or bound from a scalar value, or a jump to `bad`.
rf_whole <- function(st, v, bad, nonneg, e) {
  if (v$t == "i") {
    if (nonneg) {
      rf_ins(st, sprintf("blt %s, %s, 0", bad, v$r))
    }
    return(v$r)
  }
  if (v$t != "d") {
    rf_refuse("a loop bound must be a number", e)
  }
  n <- rf_reg(st, "i64")
  lim <- rf_reg(st, "d")
  i <- rf_reg(st, "i64")
  back <- rf_reg(st, "d")
  rf_ins(st, sprintf("dne %s, %s, %s", n, v$r, v$r))
  rf_ins(st, sprintf("bt %s, %s", bad, n))
  rf_ins(st, sprintf("dmov %s, %s", lim, rf_konst(st, 2147483647)))
  rf_ins(st, sprintf("dbgt %s, %s, %s", bad, v$r, lim))
  rf_ins(
    st,
    sprintf("dmov %s, %s", lim, rf_konst(st, if (nonneg) 0 else -2147483647))
  )
  rf_ins(st, sprintf("dblt %s, %s, %s", bad, v$r, lim))
  rf_ins(st, sprintf("d2i %s, %s", i, v$r))
  rf_ins(st, sprintf("i2d %s, %s", back, i))
  rf_ins(st, sprintf("dbne %s, %s, %s", bad, back, v$r))
  i
}

rf_for <- function(st, e) {
  st$top <- FALSE
  if (!is.symbol(e[[2]])) {
    rf_refuse("the loop variable must be a name", e)
  }
  var <- as.character(e[[2]])
  if (var %in% c(st$args, st$vlocals)) {
    rf_refuse(sprintf("`%s` is already an argument or vector", var), e)
  }
  if (!is.null(st$locals[[var]]) && st$locals[[var]]$t != "i") {
    rf_refuse(
      sprintf(
        "`%s` is a %s and a loop variable",
        var,
        rf_type_name(st$locals[[var]]$t)
      ),
      e
    )
  }
  sq <- e[[3]]
  if (
    !is.call(sq) || !as.character(sq[[1]]) %in% c("seq_len", "seq_along", ":")
  ) {
    rf_refuse("`for` runs over seq_len(n), seq_along(x) or a:b", e)
  }
  kind <- as.character(sq[[1]])
  start <- rf_reg(st, "i64")
  count <- rf_reg(st, "i64")
  step <- rf_reg(st, "i64")
  rf_ins(st, sprintf("mov %s, 1", step))
  rf_ins(st, sprintf("mov %s, 1", start))
  if (kind == "seq_len") {
    n <- rf_whole(
      st,
      rf_expr(st, sq[[2]]),
      rf_status_exit(st, "seq_len"),
      TRUE,
      e
    )
    rf_ins(st, sprintf("mov %s, %s", count, n))
  } else if (kind == "seq_along") {
    v <- rf_vector(st, sq[[2]], e)
    rf_ins(st, sprintf("mov %s, vl%d", count, v$slot))
  } else {
    bad <- rf_status_exit(st, "colon")
    a <- rf_whole(st, rf_expr(st, sq[[2]]), bad, FALSE, e)
    b <- rf_whole(st, rf_expr(st, sq[[3]]), bad, FALSE, e)
    up <- rf_label(st)
    go <- rf_label(st)
    rf_ins(st, sprintf("mov %s, %s", start, a))
    rf_ins(st, sprintf("bge %s, %s, %s", up, b, a))
    rf_ins(st, sprintf("mov %s, -1", step))
    rf_ins(st, sprintf("sub %s, %s, %s", count, a, b))
    rf_ins(st, sprintf("jmp %s", go))
    rf_place(st, up)
    rf_ins(st, sprintf("sub %s, %s, %s", count, b, a))
    rf_place(st, go)
    rf_ins(st, sprintf("add %s, %s, 1", count, count))
  }
  l <- st$locals[[var]]
  if (is.null(l)) {
    st$locals[[var]] <- list(
      t = "i",
      r = rf_reg(st, "i64", sprintf("v_%d", length(st$locals) + 1L))
    )
    l <- st$locals[[var]]
  }
  ctr <- rf_reg(st, "i64")
  k <- rf_reg(st, "i64")
  top <- rf_label(st)
  nxt <- rf_label(st)
  brk <- rf_label(st)
  rf_ins(st, sprintf("mov %s, %s", ctr, start))
  rf_ins(st, sprintf("mov %s, 0", k))
  rf_place(st, top)
  rf_ins(st, sprintf("bge %s, %s, %s", brk, k, count))
  rf_ins(st, sprintf("mov %s, %s", l$r, ctr))
  st$dead <- setdiff(st$dead, var)
  was <- var %in% st$assigned
  st$assigned <- union(st$assigned, var)
  rf_loop_body(st, e[[4]], list(brk = brk, nxt = nxt, var = var))
  if (!was) {
    st$assigned <- setdiff(st$assigned, var)
  }
  rf_place(st, nxt)
  rf_ins(st, sprintf("add %s, %s, %s", ctr, ctr, step))
  rf_ins(st, sprintf("add %s, %s, 1", k, k))
  rf_poll(st)
  rf_ins(st, sprintf("jmp %s", top))
  rf_place(st, brk)
  st$dead <- union(st$dead, var)
}

rf_finish <- function(st) {
  if (is.null(st$result)) {
    rf_refuse("the function has no result", NULL)
  }
  nslots <- length(st$vargs) + length(st$vlocals)
  entry <- character()
  for (k in seq_len(nslots)) {
    if (!is.na(st$regs[sprintf("vp%d", k)])) {
      entry <- c(
        entry,
        sprintf("  mov vp%d, i64:%d(VP)", k, 8L * (k - 1L)),
        sprintf("  mov vl%d, i64:%d(VL)", k, 8L * (k - 1L))
      )
    }
  }
  exits <- unlist(lapply(names(rf_status)[-1], function(nm) {
    c(sprintf("Lst%d:", rf_status[[nm]]), sprintf("  ret %d", rf_status[[nm]]))
  }))
  regs <- st$regs
  decl <- vapply(names(regs), function(n) sprintf("%s:%s", regs[[n]], n), "")
  locals <- if (length(decl)) {
    vapply(
      split(decl, ceiling(seq_along(decl) / 8)),
      function(d) paste0("  local ", paste(d, collapse = ", ")),
      ""
    )
  } else {
    character()
  }
  text <- c(
    "rfm: module",
    "  export rf",
    "p_d1: proto d, d:x",
    "p_d2: proto d, d:x, d:y",
    "p_id: proto i64, d:x",
    "p_i0: proto i64",
    paste0(
      "  import ",
      paste(
        c(
          unname(rf_math1),
          "zumir_r_isna",
          "R_pow",
          "zumir_r_fmod",
          "zumir_r_intdiv",
          "zumir_r_min2",
          "zumir_r_max2",
          "zumir_r_poll"
        ),
        collapse = ", "
      )
    ),
    "rf: func i64, p:K, p:S, p:VP, p:VL, p:R",
    unname(locals),
    entry,
    st$lines,
    "  ret 0",
    exits,
    "  endfunc",
    "endmodule",
    ""
  )
  list(
    text = paste(text, collapse = "\n"),
    consts = st$consts,
    sargs = st$sargs,
    vargs = st$vargs,
    vlocals = st$vlocals,
    sizes = st$sizes,
    result = st$result
  )
}

# ---- Calling ----

rf_call <- function(prog, ctx, args) {
  for (a in c(prog$sargs, prog$vargs)) {
    x <- args[[a]]
    if (typeof(x) != "double") {
      stop(structure(
        class = c("zumir_argument_error", "zumir_error", "error", "condition"),
        list(
          message = sprintf(
            "argument `%s` must be a double, not %s: R would compute in another type",
            a,
            typeof(x)
          ),
          call = NULL
        )
      ))
    }
    if (!is.null(attributes(x))) {
      stop(structure(
        class = c("zumir_argument_error", "zumir_error", "error", "condition"),
        list(
          message = sprintf(
            "argument `%s` has attributes (%s): R would keep or dispatch on them",
            a,
            paste(names(attributes(x)), collapse = ", ")
          ),
          call = NULL
        )
      ))
    }
    if (a %in% prog$sargs && length(x) != 1L) {
      stop(structure(
        class = c("zumir_argument_error", "zumir_error", "error", "condition"),
        list(
          message = sprintf(
            "argument `%s` is used as a scalar and has length %s",
            a,
            format(length(x))
          ),
          call = NULL
        )
      ))
    }
    if (a %in% prog$vargs && length(x) > .Machine$integer.max) {
      stop(structure(
        class = c("zumir_argument_error", "zumir_error", "error", "condition"),
        list(
          message = sprintf(
            "argument `%s` is longer than .Machine$integer.max",
            a
          ),
          call = NULL
        )
      ))
    }
  }
  env <- list2env(args[prog$vargs], parent = baseenv())
  for (a in prog$sargs) {
    assign(a, args[[a]], envir = env)
  }
  locals <- lapply(prog$vlocals, function(s) {
    size <- eval(prog$sizes[[s]], env)
    if (
      !is.numeric(size) ||
        length(size) != 1L ||
        is.na(size) ||
        size < 0 ||
        size != floor(size) ||
        size > .Machine$integer.max
    ) {
      rf_runtime_error(sprintf(
        "the size of `%s` must be a whole number from 0, not %s",
        s,
        format(size)
      ))
    }
    numeric(size)
  })
  scalars <- vapply(prog$sargs, function(a) args[[a]], 0)
  vectors <- c(unname(args[prog$vargs]), locals)
  res <- .Call(
    zumir_rf_call,
    ctx,
    prog$consts,
    as.double(scalars),
    vectors,
    prog$result == "vector"
  )
  res <- zumir_check(res)
  st <- res$status
  if (st == rf_status[["interrupt"]]) {
    # The poll stopped the loop. A time limit (or another error in R's
    # event processing) is raised with R's own message; an interrupt is
    # signalled as R signals one, then unwinds to the top level.
    if (nzchar(res$poll)) {
      stop(simpleError(res$poll, call = NULL))
    }
    signalCondition(structure(class = c("interrupt", "condition"), list()))
    invokeRestart("abort")
  }
  if (st != 0L) {
    rf_runtime_error(switch(
      names(rf_status)[match(st, rf_status)],
      na_cond = "missing value where TRUE/FALSE needed",
      nan_cond = "argument is not interpretable as logical",
      index = "subscript out of bounds, NA or not a whole number",
      overflow = "integer overflow",
      seq_len = "seq_len() needs a whole number from 0",
      colon = "a:b needs whole numbers that are not NA"
    ))
  }
  if (isTRUE(res$nan)) {
    warning("NaNs produced", call. = FALSE)
  }
  if (isTRUE(res$fmod)) {
    warning("probable complete loss of accuracy in modulus", call. = FALSE)
  }
  if (prog$result == "vector") {
    locals[[res$value + 1]]
  } else {
    res$value
  }
}
