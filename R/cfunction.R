#' Compile a C function and call it from R
#'
#' `cfunction()` compiles one C function with MIR's own C compiler, `c2mir`,
#' in memory, and returns an R function that calls it. No toolchain is
#' needed: no Rtools, Xcode tools or `gcc`.
#'
#' The source is C11 after a small prelude, which declares the C maths
#' library (`exp`, `sin`, `pow`, `fma`, `lgamma`, …) and `memcpy`,
#' `memmove`, `memset`, `memcmp` and `strlen`, and gives `stddef.h`,
#' `stdint.h`, `stdbool.h`, `stdarg.h`, `float.h` and `limits.h`. No header
#' is read from a file, so `#include <stdio.h>` is an error, and the same
#' source compiles the same everywhere. Calls to any other function are
#' link errors. The function is generated on the session's tier
#' ([zumir_tier()]) unless `tier` says otherwise.
#'
#' The source must define exactly one non-`static` function; `static`
#' helpers beside it are fine. Its parameters may be `int`, `double`,
#' `const int *`, `const double *`, `int *` and `double *`, and it returns
#' `void`, `int` or `double`.
#'
#' * A `const` pointer is an input, passed without a copy: an integer or
#'   double vector (an integer vector is converted for a `const double *`).
#' * A non-`const` pointer is an output and must be named in `out`. When
#'   the call supplies a value, the C function gets a copy of it; when it
#'   does not, a zero-filled vector. Either way the caller's vector is
#'   never changed, and the written copy is returned.
#' * `n` names an `int` parameter that receives a length. When the call
#'   does not give it, it is the common length of the pointer inputs; when
#'   it does, it may not exceed any of them. Outputs have length `n`,
#'   unless `out` gives them a fixed length, as in `out = c(y = 3)`.
#'
#' The R function returns the C function's value, or its one output, or,
#' when there are several, a named list: the value first, named after the
#' function, then the outputs by parameter name.
#'
#' Code compiled by `cfunction()` runs with `.Call()`'s rights: it can
#' read and write any memory a C function can, and a loop that never ends
#' is not interrupted.
#'
#' @param code One string of C.
#' @param out Names of the output (non-`const` pointer) parameters, or a
#'   named integer vector giving each a fixed length.
#' @param n The name of an `int` parameter that receives the length, or
#'   `NULL`.
#' @param tier `"native"` or `"interp"`; by default [zumir_tier()].
#' @returns A function of class `zumir_cfunction`, whose arguments are the
#'   C function's parameters.
#' @family compilers
#' @export
#' @examplesIf zumir_tier() != "none"
#' sq <- cfunction("
#'   void sq(int n, const double *x, double *y) {
#'     for (int i = 0; i < n; i++) y[i] = x[i] * x[i];
#'   }", out = "y", n = "n")
#' sq(x = c(1, 2, 3))
#'
#' dot <- cfunction("
#'   double dot(int n, const double *x, const double *y) {
#'     double s = 0;
#'     for (int i = 0; i < n; i++) s += x[i] * y[i];
#'     return s;
#'   }", n = "n")
#' dot(x = c(1, 2, 3), y = c(4, 5, 6))
cfunction <- function(code, out = character(), n = NULL, tier = NULL) {
  if (!is.character(code) || length(code) != 1L || is.na(code)) {
    cf_stop("argument", "`code` must be one string of C")
  }
  out <- cf_out(out)
  if (!is.null(n) && !(is.character(n) && length(n) == 1L && !is.na(n))) {
    cf_stop("argument", "`n` must be one parameter name, or NULL")
  }
  tier <- resolve_tier(tier, "cfunction")
  res <- zumir_check(.Call(zumir_cfun_new, code, tier, "cfunction"))
  sig <- cf_signature(code, res$name)
  cf_check_signature(sig, res)
  spec <- cf_spec(sig, out, n)
  g <- function() NULL
  formals(g) <- spec$formals
  call_args <- as.call(c(
    quote(list),
    stats_setNames(lapply(sig$params$name, as.name), sig$params$name)
  ))
  # The body is one .Call: zumir_cfun_invoke() checks and marshals the
  # arguments and signals what it refuses, so a call costs about what a
  # package's .Call wrapper does (design §9). R's JIT does not compile a
  # closure made here, so nothing else goes in the body.
  # The routine from a call in a call's head, where R skips non-function
  # bindings: a parameter named zumir_cfun_invoke shadowed its name.
  invoke <- bquote(
    .Call(cf_routine(), .(res$context), .(cf_cspec(spec)), .(call_args))
  )
  body(g) <- if (spec$ret == "void" && length(spec$out) == 0L) {
    call("invisible", invoke)
  } else {
    invoke
  }
  environment(g) <- new.env(parent = asNamespace("zumir"))
  structure(
    g,
    class = c("zumir_cfunction", "function"),
    tier = tier,
    name = res$name
  )
}

#' @export
print.zumir_cfunction <- function(x, ...) {
  cat(sprintf(
    "<zumir_cfunction %s(%s), %s>\n",
    attr(x, "name"),
    paste(names(formals(x)), collapse = ", "),
    attr(x, "tier")
  ))
  invisible(x)
}

# out as given (names; named lengths; or a list of both, as
# list("y", z = 3)) to list(names, fixed): fixed a named vector of the
# fixed lengths.
cf_out <- function(out) {
  if (is.null(out)) {
    out <- character()
  }
  if (is.character(out) && is.null(names(out))) {
    return(list(names = out, fixed = stats_setNames(numeric(), character())))
  }
  if (is.numeric(out) && !is.null(names(out)) && all(nzchar(names(out)))) {
    return(list(
      names = names(out),
      fixed = stats_setNames(as.numeric(out), names(out))
    ))
  }
  if (is.list(out)) {
    nms <- if (is.null(names(out))) rep("", length(out)) else names(out)
    keys <- character()
    fixed <- stats_setNames(numeric(), character())
    for (i in seq_along(out)) {
      v <- out[[i]]
      if (
        !nzchar(nms[[i]]) && is.character(v) && length(v) == 1L && !is.na(v)
      ) {
        keys <- c(keys, v)
      } else if (nzchar(nms[[i]]) && is.numeric(v) && length(v) == 1L) {
        keys <- c(keys, nms[[i]])
        fixed[[nms[[i]]]] <- v
      } else {
        cf_stop("argument", "`out` must be parameter names, or named lengths")
      }
    }
    return(list(names = keys, fixed = fixed))
  }
  cf_stop("argument", "`out` must be parameter names, or named lengths")
}

cf_routine <- function() zumir_cfun_invoke

cf_stop <- function(kind, message) {
  zumir_stop(kind, paste0("cfunction(): ", message, "."))
}

# The source as c2mir's preprocessor leaves it, after the prelude: no
# comments, conditionals resolved, macros expanded. The signature is read
# from this, so a decoy in a comment, an #if 0 branch or a macro is never
# read in place of the definition c2mir compiled.
cf_preprocessed <- function(code) {
  path <- tempfile("zumir-cfunction-", fileext = ".i")
  on.exit(unlink(path))
  zumir_check(.Call(zumir_cfun_prepro, code, "cfunction", path))
  text <- paste(readLines(path, warn = FALSE), collapse = "\n")
  text <- sub("(?s)^.*?__zumir_prelude_end\\s*;", "", text, perl = TRUE)
  gsub("(?m)^#line[^\n]*", " ", text, perl = TRUE)
}

# The definition of `name`: its return type and parameters, read from the
# preprocessed source with string and character literals removed. A
# signature the patterns do not read is refused, never guessed.
cf_signature <- function(code, name) {
  text <- gsub(
    "(?s)\"(?:[^\"\\\\\n]|\\\\.)*\"|'(?:[^'\\\\\n]|\\\\.)*'",
    " ",
    cf_preprocessed(code),
    perl = TRUE
  )
  text <- gsub("[[:space:]]+", " ", text, perl = TRUE)
  # Every word before the name is the return type (extern and inline
  # aside), so `long double` is read whole and refused by name.
  pattern <- sprintf(
    "((?:\\b[A-Za-z_][A-Za-z0-9_]* ?)+?)( ?\\*)? ?\\b%s ?\\(([^()]*)\\) ?\\{",
    name
  )
  m <- regmatches(text, regexec(pattern, text, perl = TRUE))[[1]]
  if (length(m) == 0L) {
    cf_stop("compile", sprintf("cannot read the signature of `%s`", name))
  }
  if (nzchar(m[[3]])) {
    cf_stop(
      "compile",
      sprintf(
        "`%s` returns a pointer; cfunction() supports void, int and double",
        name
      )
    )
  }
  words <- strsplit(trimws(m[[2]]), " ", fixed = TRUE)[[1]]
  ret <- paste(setdiff(words, c("extern", "inline")), collapse = " ")
  if (!ret %in% c("void", "int", "double")) {
    cf_stop(
      "compile",
      sprintf(
        "`%s` returns `%s`; cfunction() supports void, int and double",
        name,
        ret
      )
    )
  }
  params <- trimws(strsplit(m[[4]], ",", fixed = TRUE)[[1]])
  if (length(params) == 1L && params %in% c("", "void")) {
    params <- character()
  }
  one <- function(p) {
    pm <- regmatches(
      p,
      regexec(
        "^(const )?(int|double) ?(\\*)? ?([A-Za-z_][A-Za-z0-9_]*)$",
        p
      )
    )[[1]]
    if (length(pm) == 0L) {
      cf_stop(
        "compile",
        sprintf(
          "parameter `%s` of `%s`: cfunction() supports int, double, const int *, const double *, int * and double *",
          p,
          name
        )
      )
    }
    pointer <- nzchar(pm[[4]])
    if (nzchar(pm[[2]]) && !pointer) {
      cf_stop(
        "compile",
        sprintf(
          "parameter `%s`: a const scalar is not supported; drop const",
          p
        )
      )
    }
    data.frame(
      name = pm[[5]],
      type = pm[[3]],
      kind = if (!pointer) {
        "scalar"
      } else if (nzchar(pm[[2]])) {
        "input"
      } else {
        "output"
      },
      stringsAsFactors = FALSE
    )
  }
  params <- do.call(
    rbind,
    c(
      list(data.frame(
        name = character(),
        type = character(),
        kind = character()
      )),
      lapply(params, one)
    )
  )
  if (anyDuplicated(params$name)) {
    cf_stop("compile", "two parameters share a name")
  }
  list(name = name, ret = ret, params = params)
}

# What c2mir compiled has to be what the text says: one MIR argument per
# parameter, of the matching kind, and the matching result.
cf_check_signature <- function(sig, res) {
  # vapply, not ifelse(): with no parameters ifelse() gives logical(0),
  # which is not identical to character(0).
  want <- vapply(
    seq_len(nrow(sig$params)),
    function(i) {
      if (sig$params$kind[[i]] != "scalar") {
        "p"
      } else if (sig$params$type[[i]] == "int") {
        "i32"
      } else {
        "d"
      }
    },
    ""
  )
  got <- res$arg_types
  # c2mir passes a pointer as u64 (or i64, p); the text has already
  # refused unsigned and long parameters, so these are the pointers.
  got[got %in% c("i64", "u64")] <- "p"
  ret <- c(void = "", int = "i32", double = "d")[[sig$ret]]
  if (!identical(unname(want), got) || !identical(ret, res$res_type)) {
    cf_stop(
      "compile",
      sprintf(
        "the signature read for `%s` does not match what c2mir compiled; write the definition plainly, without macros",
        sig$name
      )
    )
  }
}

# The closure's formals and how each parameter is marshalled.
cf_spec <- function(sig, out, n) {
  p <- sig$params
  out_names <- out$names
  fixed <- out$fixed
  if (anyDuplicated(out_names)) {
    cf_stop("argument", "`out` names a parameter twice")
  }
  if (sig$ret != "void" && sig$name %in% out_names) {
    cf_stop(
      "argument",
      sprintf(
        "the function and output `%s` share a name, so the result would name two elements `%s`",
        sig$name,
        sig$name
      )
    )
  }
  for (o in out_names) {
    if (!o %in% p$name) {
      cf_stop(
        "argument",
        sprintf("`out` names `%s`, which is not a parameter", o)
      )
    }
    if (p$kind[p$name == o] != "output") {
      cf_stop(
        "argument",
        sprintf("`out` names `%s`, which is not a non-const pointer", o)
      )
    }
  }
  for (f in names(fixed)) {
    v <- fixed[[f]]
    if (is.na(v) || v < 0 || v != floor(v) || v > .Machine$integer.max) {
      cf_stop(
        "argument",
        sprintf("the length of `%s` must be a whole number from 0", f)
      )
    }
  }
  # D24: a non-const pointer not named in out would have its writes lost.
  lost <- p$name[p$kind == "output" & !p$name %in% out_names]
  if (length(lost)) {
    cf_stop(
      "compile",
      sprintf(
        "`%s` is a non-const pointer, an output: name it in `out`, or make it const",
        lost[[1]]
      )
    )
  }
  if (!is.null(n)) {
    if (
      !n %in% p$name ||
        p$kind[p$name == n] != "scalar" ||
        p$type[p$name == n] != "int"
    ) {
      cf_stop(
        "argument",
        sprintf("`n` names `%s`, which is not an int parameter", n)
      )
    }
  }
  needs_n <- setdiff(out_names, names(fixed))
  if (length(needs_n) && is.null(n)) {
    cf_stop(
      "argument",
      sprintf(
        "output `%s` has no length: give `n`, or a fixed length in `out`",
        needs_n[[1]]
      )
    )
  }
  formals <- lapply(p$name, function(x) {
    if (identical(x, n) || x %in% out_names) NULL else quote(expr = )
  })
  names(formals) <- p$name
  list(
    name = sig$name,
    ret = sig$ret,
    params = p,
    out = out_names,
    fixed = fixed,
    n = n,
    formals = formals
  )
}

# The spec zumir_cfun_invoke() reads: per parameter its kind (0 int, 1
# double, 2 const pointer, 3 output pointer), its type (0 int, 1 double),
# its name and an output's fixed length (-1 for none); the 1-based index of
# the length parameter (0 for none); and the value's name (NA for void).
cf_cspec <- function(spec) {
  p <- spec$params
  kinds <- ifelse(
    p$kind == "scalar",
    ifelse(p$type == "int", 0L, 1L),
    ifelse(p$kind == "input", 2L, 3L)
  )
  fixed <- vapply(
    p$name,
    function(nm) {
      if (nm %in% names(spec$fixed)) as.integer(spec$fixed[[nm]]) else -1L
    },
    1L,
    USE.NAMES = FALSE
  )
  list(
    kinds = as.integer(kinds),
    types = as.integer(p$type == "double"),
    names = p$name,
    n = if (is.null(spec$n)) 0L else match(spec$n, p$name),
    fixed = fixed,
    value = if (spec$ret == "void") NA_character_ else spec$name
  )
}
