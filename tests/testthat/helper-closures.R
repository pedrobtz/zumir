# The three compilers' closures, for test-closures.R (#23): each case
# makes a closure and gives an argument and the answer.
closure_cases <- function() {
  list(
    cfunction = list(
      make = function() cfunction("double f(double a) { return a * 2 + 1; }"),
      arg = 3,
      want = 7
    ),
    mir_compile = list(
      make = function() mir_compile(function(a) a * 2 + 1),
      arg = 3,
      want = 7
    ),
    mir_function = list(
      make = function() {
        mir_function(
          "m: module\n export f\nf: func d, d:a\n local d:r\n dmul r, a, a\n ret r\n endfunc\nendmodule\n",
          "f"
        )
      },
      arg = 3,
      want = 9
    )
  )
}
