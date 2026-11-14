# The rfunction() oracle (#21): R itself. run_r() and run_z() evaluate a
# function on the same arguments and return what happened; same_as_r()
# compares them: the value with identical(num.eq = FALSE), which tells 0
# from -0 and NA from NaN, allowing only R's own NA-or-NaN freedom (?NA:
# where a NaN is involved, either may result); the warnings as a set of messages (R may warn more
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

# Where R's NA-or-NaN freedom applies. "inputs": the arguments hold both an
# NA and a NaN, for chosen inputs, where an NA alone (NA + 1) must stay NA.
# "any": on any element, for random functions, where a NaN can be made
# inside (0/0) and a comparison with it makes an NA, so the inputs do not
# show whether the two met.
na_nan_free <- function(args, rule) {
  if (rule == "any") {
    return(TRUE)
  }
  v <- unlist(args, use.names = FALSE)
  any(is.na(v) & !is.nan(v)) && any(is.nan(v))
}

# Two run_capture() results agree: the same warnings, and values equal but
# for NA-or-NaN.
same_capture <- function(a, b) {
  identical(a$warnings, b$warnings) &&
    !inherits(a$value, "error") &&
    !inherits(b$value, "error") &&
    same_value(a$value, b$value, TRUE)
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

compare_with_r <- function(f, g, args, na_nan = "inputs") {
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
  if (!identical(typeof(r$value), typeof(z$value))) {
    return(sprintf("type %s, R's %s", typeof(z$value), typeof(r$value)))
  }
  if (!same_value(r$value, z$value, na_nan_free(args, na_nan))) {
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
  # A logical in arithmetic, as R's 0, 1 or NA. Not without NA (env$na):
  # a comparison with a NaN is NA, and this is how it becomes a number.
  if (!env$na) {
    return(rf_gen_leaf(env))
  }
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

# With na = FALSE no value is ever NA: no arithmetic on a logical (with
# rf_gen_args(na = FALSE), no NA argument). An NA and a NaN then never
# meet, where R leaves open which comes out (?NA), and where is.nan() can
# turn that choice into different control flow: native against
# interpreter is bitwise only there.
rf_gen_function <- function(na = TRUE) {
  locals <- paste0("s", seq_len(sample(1:3, 1)))
  # The scope is a list, copied into each loop; the while counter is shared.
  env <- list(
    locals = character(),
    loopvars = character(),
    alongvars = character(),
    count = new.env(),
    na = na
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

rf_gen_args <- function(na = TRUE) {
  pick <- function(n) {
    v <- c(odd_doubles(), rnorm(4), round(rnorm(4) * 10), 1e-310)
    if (!na) {
      v <- v[!(is.na(v) & !is.nan(v))]
    }
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

# One line of R that rebuilds x, every double to 17 digits.
rf_show <- function(x) {
  paste(
    deparse(x, control = c("keepNA", "digits17", "niceNames")),
    collapse = " "
  )
}

# ---- The two routes (D17) ----

# Functions covering every construct of the subset, for the route tests:
# each operator and math function, the logic, every loop form, vector
# locals, and each run-time check.
rf_construct_cases <- function() {
  ops <- c("+", "-", "*", "/", "^", "%%", "%/%", "min", "max")
  binary <- lapply(ops, function(op) {
    eval(bquote(function(a, b) .(as.name(op))(a, b)))
  })
  math <- c(
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
  )
  unary <- lapply(math, function(fn) {
    eval(bquote(function(a) .(as.name(fn))(a)))
  })
  cmp <- lapply(c("==", "!=", "<", "<=", ">", ">="), function(op) {
    eval(bquote(function(a, b) as.double(.(as.name(op))(a, b))))
  })
  c(
    binary,
    unary,
    cmp,
    list(
      function(a) -a,
      function(a, b) as.double(a > 0 && b > 0) + as.double(a > 0 || b > 0),
      function(a) as.double(!(a > 0)) + (a > 0) + 0.5,
      function(a) {
        as.double(is.na(a)) +
          2 * as.double(is.nan(a)) +
          4 * as.double(is.finite(a))
      },
      function(a) {
        s <- 0
        if (a > 0) {
          s <- 1
        } else {
          s <- 2
        }
        s
      },
      function(x) {
        s <- 0
        for (i in seq_along(x)) {
          s <- s + i * x[i]
        }
        s
      },
      function(n) {
        s <- 0
        for (i in seq_len(n)) {
          s <- s + i
        }
        s
      },
      function(a, b) {
        s <- 0
        for (i in a:b) {
          s <- s * 2 + i
        }
        s
      },
      function(n) {
        i <- 0
        s <- 0
        while (i < n) {
          i <- i + 1
          if (i %% 3 == 0) {
            next
          }
          s <- s + i
          if (s > 50) break
        }
        s
      },
      function(x) {
        y <- numeric(length(x))
        acc <- 0
        for (i in seq_along(x)) {
          acc <- acc + x[i]
          y[i] <- acc
        }
        y
      },
      function(x, i) x[i],
      function(n) {
        s <- 0
        for (i in seq_len(n)) {
          if (i == n) s <- s + i * i
        }
        s
      },
      function(x) {
        s <- 0
        for (i in seq_along(x)) {
          s <- s + min(as.double(i), i) + max(i, x[i])
        }
        s
      }
    )
  )
}

# The module a route builds for f, as MIR prints it before linking. The
# text does not depend on the tier, and the interpreter links fastest.
rf_module_text <- function(f, route) {
  prog <- rf_compile(f)
  path <- tempfile(fileext = ".mir")
  on.exit(unlink(path))
  rf_context(prog, "interp", route, path)
  readLines(path)
}
