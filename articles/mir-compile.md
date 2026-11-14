# What mir_compile() accepts

``` r

library(zumir)
```

[`mir_compile()`](https://pedrobtz.github.io/zumir/reference/mir_compile.md)
compiles an R function written in a small numeric subset of R. Its
promise is simple: the compiled function returns exactly what R returns,
bit for bit, or it errors. It never gives a different answer. Where
matching R would not be cheap, it refuses: at compile time when the
function itself is the problem, at the call when an argument is.

## The subset

**Arguments** are doubles without attributes. One that is indexed
(`x[i]`), passed to [`length()`](https://rdrr.io/r/base/length.html) or
to [`seq_along()`](https://rdrr.io/r/base/seq.html) is a vector; any
other that the body reads must have length one. An argument the body
never reads is never evaluated, as in R. Arguments have no defaults,
there is no `...`, and the body never assigns to one.

**Locals** are double or logical scalars, `for` loop counters, and
double vectors made by one `y <- numeric(size)` at the top of the body,
changed only by `y[i] <- value`.

**Code**: `{`, `<-`, `=`, `if`/`else`, `while`, `for` over `seq_len(n)`,
`seq_along(x)` and `a:b` (counting down when `a > b`),
[`break`](https://rdrr.io/r/base/Control.html),
[`next`](https://rdrr.io/r/base/Control.html),
[`return()`](https://rdrr.io/r/base/function.html).

**Operations**: `+ - * / ^ %% %/%`, unary `-`, comparisons, `!`, `&&`
and `||` with R’s three-valued logic; `exp`, `log`, `log1p`, `expm1`,
`sqrt`, `abs`, `floor`, `ceiling`, `trunc`, `sin`, `cos`, `tan`;
two-argument `min` and `max`; `length`, `is.na`, `is.nan`, `is.finite`,
`as.double`.

**The result** is a double scalar, or a vector local.

``` r

csum <- mir_compile(function(x) {
  y <- numeric(length(x))
  acc <- 0
  for (i in seq_along(x)) {
    acc <- acc + x[i]
    y[i] <- acc
  }
  y
})
csum(c(1, 2, NA, 4))
#> [1]  1  3 NA NA
```

## Where it errors and R would not

Each of these is a place where R gives an answer the compiled code could
only match at a cost, so it errors instead.

| R | [`mir_compile()`](https://pedrobtz.github.io/zumir/reference/mir_compile.md) | Why |
|----|----|----|
| `x[0]`, `x[length(x) + 1]`, `x[NA]`, `x[1.5]` read `numeric(0)`, `NA`, the truncated index | an error | none of them is the element at that index |
| `seq_len(2.5)` and `numeric(2.5)` truncate; `1.5:3` steps from 1.5 | an error | a size or bound must be a whole number |
| integer arithmetic past `.Machine$integer.max` gives `NA` with a warning | an error | the loop counter is a machine integer |
| an integer, logical or character argument computes in its own type | an error | the compiled code computes in doubles |
| an argument with names, `dim` or a class keeps or dispatches on them | an error | the compiled code sees only the numbers |
| a scalar argument of length 0 or 3 recycles | an error | it is used as one number |

``` r

first <- mir_compile(function(x) x[1])
first(numeric())
#> Error:
#> ! subscript out of bounds, NA or not a whole number
```

And at compile time, each with its reason:

| Refused | Why |
|----|----|
| a call to any other function, `<<-`, `repeat`, strings, lists | outside the subset |
| a result R would type as integer or logical (`length(x)`, `a > b`, `abs(i)` of a counter) | R would return an integer or logical; write [`as.double()`](https://rdrr.io/r/base/double.html) |
| `exp` or `+` rebound in the function’s environment | the compiled code keeps base R’s meaning |
| a local with the name of a function the body calls | R would look the function up past the local |
| a default, `...`, or an assignment to an argument | R’s lazy defaults and copies are not modelled |
| [`numeric()`](https://rdrr.io/r/base/numeric.html) inside a loop or branch, `z <- y`, a second [`numeric()`](https://rdrr.io/r/base/numeric.html) for one name | R’s copy-on-modify is not modelled |
| a local that may be read before it is assigned | R would error, or not, depending on the path |
| the loop variable read after its loop | R leaves it at the last value, or `NULL` |
| `min(x)` or `max(a, b, c)` | only two-argument `min` and `max` are supported |

``` r

mir_compile(function(x) sum(x))
#> Error:
#> ! mir_compile(): `sum` is not in the subset in `sum(x)`.
```

## Warnings

Where R warns, the compiled function gives the same warning: “NaNs
produced” from `log(-1)` or `sqrt(-1)`, and “probable complete loss of
accuracy in modulus” from `%%`. It gives each once per call, where a
loop in R may warn many times.

## NA and NaN together

Where an `NA` and a `NaN` meet in one operation, R leaves open which
comes out (see `?NA`), and so does
[`mir_compile()`](https://pedrobtz.github.io/zumir/reference/mir_compile.md):
the answer can differ between R, the two tiers and different builds.
[`is.na()`](https://rdrr.io/r/base/NA.html) is `TRUE` either way, but
[`is.nan()`](https://rdrr.io/r/base/is.finite.html) of such a value can
take a different branch.

## Interrupts

Every loop counts its iterations, and every 2^16 of them checks for an
interrupt. So Ctrl-C and
[`setTimeLimit()`](https://rdrr.io/r/base/setTimeLimit.html) stop a
compiled loop, as they do an R one, with R’s own condition.

``` r

spin <- mir_compile(function(a) {
  s <- 0
  while (TRUE) s <- s + a
  s
})
setTimeLimit(elapsed = 1, transient = TRUE)
spin(1)
#> Error: reached elapsed time limit
```

[`cfunction()`](https://pedrobtz.github.io/zumir/reference/cfunction.md)
code is the user’s own C and is not interrupted.
