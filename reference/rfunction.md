# Compile a numeric R function to native code

`rfunction()` takes an R function written in a small numeric subset of R
and returns a function with the same arguments that runs compiled code.
It returns exactly what the R function returns, or an error: never a
different answer.

## Usage

``` r
rfunction(f, tier = NULL)
```

## Arguments

- f:

  A function.

- tier:

  `"native"` or `"interp"`; by default
  [`zumir_tier()`](https://pedrobtz.github.io/zumir/reference/zumir_info.md).

## Value

A function of class `zumir_rfunction`, with `f`'s arguments.

## Details

The subset is scalar doubles, double vectors indexed in loops,
conditions and a table of maths. In detail:

- Arguments are doubles without attributes. One that is indexed
  (`x[i]`), passed to [`length()`](https://rdrr.io/r/base/length.html)
  or to [`seq_along()`](https://rdrr.io/r/base/seq.html) is a vector;
  any other must have length one when called. Arguments are never
  assigned to, have no defaults, and there is no `...`.

- Locals are double or logical scalars, `for` loop counters, and double
  vectors made by one `y <- numeric(size)` at the top of the body,
  before any loop or branch, changed only by `y[i] <- value`. `size` may
  use constants, arguments,
  [`length()`](https://rdrr.io/r/base/length.html) of arguments and
  arithmetic. Every local is assigned on every path before it is read.

- `{`, `<-`, `=`, `if`/`else`, `while`, `for` over `seq_len(n)`,
  `seq_along(x)` and `a:b`,
  [`break`](https://rdrr.io/r/base/Control.html),
  [`next`](https://rdrr.io/r/base/Control.html),
  [`return()`](https://rdrr.io/r/base/function.html).

- `+ - * / ^ %% %/%`, unary `-`, comparisons, `!`, `&&`, `||`; `exp`,
  `log`, `log1p`, `expm1`, `sqrt`, `abs`, `floor`, `ceiling`, `trunc`,
  `sin`, `cos`, `tan`; two-argument `min` and `max`; `length`, `is.na`,
  `is.nan`, `is.finite`, `as.double`.

- The result is a double scalar or a vector local.

Where matching R exactly would not be cheap, the compiled function
errors instead: an index out of range, `NA` or not a whole number; a
[`seq_len()`](https://rdrr.io/r/base/seq.html) or `a:b` bound that is
`NA` or not whole; integer overflow; an argument that is not a double,
has attributes, or is a scalar of another length. Names must mean base
R's functions in `environment(f)`. Loops can be interrupted, as R's can.
Code outside the subset is a compile error naming the construct.

## See also

Other compilers:
[`mir_function()`](https://pedrobtz.github.io/zumir/reference/mir_function.md)

## Examples

``` r
ss <- rfunction(function(x) {
  s <- 0
  for (i in seq_along(x)) s <- s + x[i] * x[i]
  s
})
ss(c(1, 2, 3))
#> [1] 14
```
