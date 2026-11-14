# The rfunction() oracle (#21): R itself. run_r() and run_z() evaluate a
# function on the same arguments and return what happened; same_as_r()
# compares them: the value with identical(num.eq = FALSE), which tells 0
# from -0 and NA from NaN, allowing only R's own NA-or-NaN freedom where
# the inputs hold both; the warnings as a set of messages (R may warn more
# than once in a loop, rfunction() once per kind); the error by message.

run_capture <- function(fun, args) {
  warnings <- character()
  value <- tryCatch(
    withCallingHandlers(
      do.call(fun, args),
      warning = function(w) {
        warnings <<- c(warnings, conditionMessage(w))
        invokeRestart("muffleWarning")
      }
    ),
    error = function(e) e
  )
  list(value = value, warnings = unique(warnings))
}

na_nan_mixed <- function(args) {
  v <- unlist(args, use.names = FALSE)
  any(is.na(v) & !is.nan(v)) && any(is.nan(v))
}

same_value <- function(a, b, mixed) {
  if (identical(a, b, num.eq = FALSE)) {
    return(TRUE)
  }
  if (!mixed || !is.double(a) || !is.double(b) || length(a) != length(b)) {
    return(FALSE)
  }
  diff <- !mapply(identical, a, b, MoreArgs = list(num.eq = FALSE))
  all(is.na(a[diff]) & is.na(b[diff]))
}

# NULL when they agree, else a description of the difference.
#
# R's side runs in R's AST evaluator: an uncompiled copy of f, with the JIT
# off. R 4.6's byte compiler folds `s <- 0; -(0 * 0)` to the constant 0
# from its pool, which cannot tell 0 from -0, so the JIT makes R's answer
# change after a few calls.
run_r <- function(f, args) {
  plain <- f
  body(plain) <- body(f)
  old <- compiler::enableJIT(0)
  on.exit(compiler::enableJIT(old))
  run_capture(plain, args)
}

compare_with_r <- function(f, g, args) {
  r <- run_r(f, args)
  z <- run_capture(g, args)
  if (inherits(r$value, "error")) {
    if (!inherits(z$value, "error")) {
      return(sprintf(
        "R errors (%s), rfunction() returns",
        conditionMessage(r$value)
      ))
    }
    if (!inherits(z$value, "zumir_error")) {
      return(sprintf(
        "rfunction()'s error is not a zumir_error: %s",
        conditionMessage(z$value)
      ))
    }
    if (!identical(conditionMessage(z$value), conditionMessage(r$value))) {
      return(sprintf(
        "error message %s, R's %s",
        conditionMessage(z$value),
        conditionMessage(r$value)
      ))
    }
    return(NULL)
  }
  if (inherits(z$value, "error")) {
    return(sprintf(
      "rfunction() errors (%s), R returns",
      conditionMessage(z$value)
    ))
  }
  if (!same_value(r$value, z$value, na_nan_mixed(args))) {
    return(sprintf(
      "value %s, R's %s",
      paste(format(z$value, digits = 17), collapse = " "),
      paste(format(r$value, digits = 17), collapse = " ")
    ))
  }
  if (!identical(sort(z$warnings), sort(r$warnings))) {
    return(sprintf(
      "warnings [%s], R's [%s]",
      paste(z$warnings, collapse = "; "),
      paste(r$warnings, collapse = "; ")
    ))
  }
  NULL
}

expect_same_as_r <- function(f, ..., tier = zumir_tier(), info = NULL) {
  g <- rfunction(f, tier = tier)
  args <- list(...)
  d <- compare_with_r(f, g, args)
  expect(
    is.null(d),
    sprintf("%s\n  on %s", d %||% "", paste(deparse(args), collapse = " "))
  )
  invisible(g)
}

# Doubles that exercise R's rules: signed zeros, subnormals, non-finite
# values, NA and NaN, and ordinary numbers.
odd_doubles <- function() {
  c(
    0,
    -0,
    5e-324,
    -5e-324,
    1e-300,
    1,
    -1,
    2,
    0.5,
    -2.5,
    3,
    7,
    1e15,
    1e300,
    Inf,
    -Inf,
    NaN,
    NA
  )
}

`%||%` <- function(a, b) if (is.null(a)) b else a

# withr::local_seed() without the dependency: set the seed for the calling
# frame and restore the session's stream when it exits.
local_seed <- function(seed, frame = parent.frame()) {
  had <- exists(".Random.seed", globalenv())
  old <- if (had) get(".Random.seed", globalenv())
  do.call(
    on.exit,
    list(
      substitute(
        if (had) {
          assign(".Random.seed", old, globalenv())
        } else if (exists(".Random.seed", globalenv())) {
          rm(".Random.seed", envir = globalenv())
        },
        list(had = had, old = old)
      ),
      add = TRUE
    ),
    envir = frame
  )
  set.seed(seed)
}

# ---- Random functions in the subset (the differential test) ----
#
# rf_gen_function() draws a function of scalars a and b and a vector x:
# a few locals assigned at the top, then statements (assignments, if/else,
# loops over seq_along(x) and seq_len(), while loops that are bounded), and
# a double result. Every local is assigned before any statement, so each is
# definitely assigned; loop variables are read only inside their loops.

rf_gen_leaf <- function(env) {
  k <- sample(4, 1)
  if (k == 1) {
    return(sample(c(odd_doubles()[-18], 0.25, 10, 100), 1))
  }
  # A loop variable is an integer in R. Mostly it is made a double, so a
  # local it is assigned to stays one (an integer local is refused).
  if (k == 2 && length(env$loopvars)) {
    v <- as.name(sample(env$loopvars, 1))
    return(if (runif(1) < 0.8) call("as.double", v) else v)
  }

  # x is indexed only by a seq_along(x) variable: any other index can fall
  # outside x, where R reads NA and rfunction() refuses (D16).
  if (k == 3 && length(env$alongvars)) {
    return(call("[", quote(x), as.name(sample(env$alongvars, 1))))
  }
  as.name(sample(c("a", "b", env$locals), 1))
}

rf_gen_expr <- function(depth, env) {
  if (depth <= 0 || runif(1) < 0.25) {
    return(rf_gen_leaf(env))
  }
  k <- sample(10, 1)
  if (k <= 5) {
    op <- sample(
      c("+", "-", "*", "/", "^", "%%", "%/%"),
      1,
      prob = c(4, 4, 4, 2, 1, 1, 1)
    )
    return(call(op, rf_gen_expr(depth - 1, env), rf_gen_expr(depth - 1, env)))
  }
  if (k == 6) {
    return(call("-", rf_gen_expr(depth - 1, env)))
  }
  if (k <= 8) {
    fn <- sample(
      c(
        "exp",
        "log",
        "log1p",
        "expm1",
        "sqrt",
        "abs",
        "floor",
        "ceiling",
        "trunc",
        "sin",
        "cos",
        "tan"
      ),
      1
    )
    return(call(fn, rf_gen_expr(depth - 1, env)))
  }
  if (k == 9) {
    fn <- sample(c("min", "max"), 1)
    return(call(fn, rf_gen_expr(depth - 1, env), rf_gen_expr(depth - 1, env)))
  }
  # A logical in arithmetic, as R's 0, 1 or NA.
  call("+", call("(", rf_gen_cond(depth - 1, env)), rf_gen_expr(depth - 1, env))
}

rf_gen_cond <- function(depth, env) {
  k <- sample(6, 1)
  if (k <= 3) {
    op <- sample(c("<", "<=", ">", ">=", "==", "!="), 1)
    return(call(op, rf_gen_expr(depth, env), rf_gen_expr(depth, env)))
  }
  if (k == 4) {
    return(call(
      sample(c("&&", "||"), 1),
      rf_gen_cond(depth - 1, env),
      rf_gen_cond(depth - 1, env)
    ))
  }
  if (k == 5) {
    return(call("!", rf_gen_cond(depth - 1, env)))
  }
  call(sample(c("is.na", "is.nan", "is.finite"), 1), rf_gen_expr(depth, env))
}

rf_gen_stmt <- function(depth, env) {
  k <- sample(10, 1)
  target <- as.name(sample(env$locals, 1))
  if (k <= 5 || depth <= 0) {
    return(call("<-", target, rf_gen_expr(3, env)))
  }
  if (k <= 7) {
    s <- call("if", rf_gen_cond(2, env), rf_gen_block(depth - 1, env))
    if (runif(1) < 0.5) {
      s[[4]] <- rf_gen_block(depth - 1, env)
    }
    return(s)
  }
  if (k <= 9 && length(env$loopvars) < 2) {
    v <- paste0("i", length(env$loopvars) + 1)
    along <- runif(1) < 0.7
    seq <- if (along) {
      quote(seq_along(x))
    } else {
      call("seq_len", sample(0:4, 1) + 0)
    }
    inner <- env
    inner$loopvars <- c(v, env$loopvars)
    inner$alongvars <- c(if (along) v, env$alongvars)
    body <- rf_gen_block(depth - 1, inner)
    if (runif(1) < 0.2) {
      body <- as.call(c(
        as.list(body),
        call("if", rf_gen_cond(1, inner), quote(break))
      ))
    }
    return(call("for", as.name(v), seq, body))
  }
  # A while loop bounded by a counter local of its own.
  w <- paste0("w", env$count$n)
  env$count$n <- env$count$n + 1L
  bound <- sample(0:5, 1) + 0
  body <- rf_gen_block(depth - 1, env)
  body <- as.call(c(
    as.list(body),
    call("<-", as.name(w), call("+", as.name(w), 1))
  ))
  call(
    "{",
    call("<-", as.name(w), 0),
    call("while", call("<", as.name(w), bound), body)
  )
}

rf_gen_block <- function(depth, env) {
  n <- sample(1:2, 1)
  as.call(c(
    as.name("{"),
    lapply(seq_len(n), function(i) rf_gen_stmt(depth, env))
  ))
}

rf_gen_function <- function() {
  locals <- paste0("s", seq_len(sample(1:3, 1)))
  # The scope is a list, copied into each loop; the while counter is shared.
  env <- list(
    locals = character(),
    loopvars = character(),
    alongvars = character(),
    count = new.env()
  )
  env$count$n <- 1L
  init <- list()
  for (l in locals) {
    init[[length(init) + 1]] <- call("<-", as.name(l), rf_gen_expr(2, env))
    env$locals <- c(env$locals, l)
  }
  stmts <- lapply(seq_len(sample(1:4, 1)), function(i) rf_gen_stmt(2, env))
  result <- rf_gen_expr(2, env)
  # A while statement's counters are locals too: give them a value first.
  ws <- if (env$count$n > 1) {
    lapply(paste0("w", seq_len(env$count$n - 1)), function(w) {
      call("<-", as.name(w), 0)
    })
  }
  body <- as.call(c(as.name("{"), ws, init, stmts, result))
  f <- function(a, b, x) NULL
  body(f) <- body
  environment(f) <- baseenv()
  f
}

rf_gen_args <- function() {
  pick <- function(n) {
    v <- c(odd_doubles(), rnorm(4), round(rnorm(4) * 10), 1e-310)
    sample(v, n, replace = TRUE)
  }
  list(a = pick(1), b = pick(1), x = pick(sample(0:5, 1)))
}

# ---- The random differential's run: count, seed and reproducer ----

# 10^2 functions, or 10^4 on the full profile's R CMD check legs. Under a
# memory tool (valgrind, gctorture, the sanitizers), which runs R 20 to 100
# times slower, the full profile keeps 10^2: those legs check memory, and
# the check legs carry the differential's breadth.
rf_random_count <- function() {
  full <- identical(Sys.getenv("R_ACTIONS_PROFILE"), "full")
  tool <- Sys.getenv("R_ACTIONS_WORKFLOW") %in%
    c("valgrind", "gctorture", "sanitizers")
  if (full && !tool) 10000L else 100L
}

rf_random_seed <- function() {
  s <- suppressWarnings(as.integer(Sys.getenv("ZUMIR_RF_SEED")))
  if (is.na(s)) sample.int(1e6, 1) else s
}

rf_save_reproducer <- function(seed, f, args, what) {
  path <- file.path(tempdir(), sprintf("rfunction-random-%d.R", seed))
  writeLines(
    c(
      sprintf("# %s; seed %d", what, seed),
      paste("f <-", paste(deparse(f), collapse = "\n")),
      paste("args <-", paste(deparse(args), collapse = "\n"))
    ),
    path
  )
  path
}
