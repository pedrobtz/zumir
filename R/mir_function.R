#' Compile MIR text to an R function
#'
#' `mir_function()` reads a module in MIR's textual form, compiles the
#' function `name` from it on `tier`, and returns an R function that calls
#' the result. It is for tests, teaching, and checking a code generator's
#' output by hand; MIR's text format is described in MIR's `MIR.md`.
#'
#' The function must be exported by its module (`export name`), and take
#' and return numbers only: the integer types `i8` to `u64`, `f` and `d`,
#' with at most one result. The R function has one argument per MIR
#' argument, by the same names, each a single number. An integer argument
#' must be a whole number that fits its type. Results of 8 to 32 bits are R
#' integers; 64-bit results and `u32` are doubles, and one beyond 2^53 is
#' an error, since a double could not hold it exactly.
#'
#' The code may call the C math library and `memcpy`, `memmove`, `memset`,
#' `memcmp` and `strlen` by name (`import exp`); any other import is a link
#' error. Compiled code runs with the rights of `.Call()` code: a wrong
#' memory access crashes R, as it would in a package's C code.
#'
#' @param text MIR text: one string with one or more modules.
#' @param name The name of the function to call.
#' @param tier `"native"` or `"interp"`; by default [zumir_tier()].
#' @returns A function of class `zumir_function`. Its context lives as long
#'   as the function does.
#' @family compilers
#' @export
#' @examplesIf zumir_tier() != "none"
#' add <- mir_function("
#'   m: module
#'      export add
#'   add: func d, d:a, d:b
#'      local d:r
#'      dadd r, a, b
#'      ret r
#'      endfunc
#'      endmodule", "add")
#' add(1, 2)
mir_function <- function(text, name, tier = NULL) {
  check_string(text, "text", "mir_function")
  check_string(name, "name", "mir_function")
  tier <- resolve_tier(tier, "mir_function")
  build_mir_function(text, name, tier, imports = 0L)
}

# Internal: mir_function() with another import set (2: mir_compile()'s), for
# the tests of zumir's ports of R's arithmetic.
build_mir_function <- function(text, name, tier, imports) {
  # MIR's scanner needs the last line to end in a newline.
  if (!endsWith(text, "\n")) {
    text <- paste0(text, "\n")
  }
  res <- zumir_check(.Call(zumir_mirf_new, text, name, tier, imports))
  ctx <- res$context
  arg_names <- res$arg_names
  f <- function() NULL
  formals(f) <- stats_setNames(
    rep(list(quote(expr = )), length(arg_names)),
    arg_names
  )
  args <- as.call(c(quote(list), lapply(arg_names, as.name)))
  # The routine comes from a call in a call's head, where R skips
  # non-function bindings: a MIR argument named zumir_mirf_call shadowed
  # the routine's name. Not the routine object itself, whose address
  # saveRDS() cannot keep.
  body(f) <- bquote(zumir_check(.Call(mirf_routine(), .(ctx), .(args))))
  environment(f) <- new.env(parent = asNamespace("zumir"))
  structure(
    f,
    class = c("zumir_function", "function"),
    zumir = list(
      name = name,
      tier = tier,
      arg_types = stats_setNames(res$arg_types, arg_names),
      res_type = res$res_type
    )
  )
}

#' @export
print.zumir_function <- function(x, ...) {
  a <- attr(x, "zumir")
  args <- paste(names(a$arg_types), a$arg_types, sep = ": ", collapse = ", ")
  cat(sprintf(
    "<zumir_function %s(%s)%s, %s>\n",
    a$name,
    args,
    if (nzchar(a$res_type)) paste(":", a$res_type) else "",
    a$tier
  ))
  invisible(x)
}

mirf_routine <- function() zumir_mirf_call

# A local setNames(): zumir imports nothing, stats included.
stats_setNames <- function(object, nm) {
  names(object) <- nm
  object
}

check_string <- function(x, arg, fn) {
  if (!is.character(x) || length(x) != 1L || is.na(x)) {
    zumir_stop("argument", sprintf("%s(): `%s` must be one string.", fn, arg))
  }
}

# The tier to compile on: `tier` as given, or zumir_tier() for NULL. A
# value other than "native" or "interp" is an argument error; a tier that
# does not run here (the default included, when it is "none") is a tier
# error.
resolve_tier <- function(tier, fn) {
  if (is.null(tier)) {
    tier <- zumir_tier()
  } else if (
    !is.character(tier) ||
      length(tier) != 1L ||
      is.na(tier) ||
      !tier %in% c("native", "interp")
  ) {
    zumir_stop(
      "argument",
      sprintf("%s(): `tier` must be \"native\" or \"interp\".", fn)
    )
  }
  caps <- capabilities_frame()
  if (tier == "none" || !all(caps$ok[caps$tier == tier])) {
    zumir_stop(
      "tier",
      sprintf(
        "%s(): the %s tier does not run here; see zumir_info()$capabilities.",
        fn,
        if (tier == "none") "default" else tier
      )
    )
  }
  tier
}
