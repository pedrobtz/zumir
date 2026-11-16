# Compile a C function and call it from R

`cfunction()` compiles one C function with MIR's own C compiler,
`c2mir`, in memory, and returns an R function that calls it. No
toolchain is needed: no Rtools, Xcode tools or `gcc`.

## Usage

``` r
cfunction(code, out = character(), n = NULL, tier = NULL)
```

## Arguments

- code:

  One string of C.

- out:

  Names of the output (non-`const` pointer) parameters, or a named
  integer vector giving each a fixed length;
  [`character()`](https://rdrr.io/r/base/character.html) or `NULL` for
  none.

- n:

  The name of an `int` parameter that receives the length, or `NULL`.

- tier:

  `"native"` or `"interp"`; by default
  [`zumir_tier()`](https://pedrobtz.github.io/zumir/reference/zumir_info.md).

## Value

A function of class `zumir_cfunction`, whose arguments are the C
function's parameters.

## Details

The source is C11 after a small prelude, which declares the C maths
library (`exp`, `sin`, `pow`, `fma`, `lgamma`, …) and `memcpy`,
`memmove`, `memset`, `memcmp` and `strlen`, and gives `stddef.h`,
`stdint.h`, `stdbool.h`, `stdarg.h`, `float.h` and `limits.h`. No header
is read from a file, so `#include <stdio.h>` is an error, and the same
source compiles the same everywhere. Calls to any other function are
link errors. The function is generated on the session's tier
([`zumir_tier()`](https://pedrobtz.github.io/zumir/reference/zumir_info.md))
unless `tier` says otherwise.

The source must define exactly one non-`static` function; `static`
helpers beside it are fine. Its parameters may be `int`, `double`,
`const int *`, `const double *`, `int *` and `double *`, and it returns
`void`, `int` or `double`.

- A `const` pointer is an input, passed without a copy: an integer or
  double vector (an integer vector is converted for a `const double *`).
  A factor, `Date`, `POSIXct` or `difftime` is refused, for a pointer as
  for a scalar.

- A non-`const` pointer is an output and must be named in `out`. When
  the call supplies a value, the C function gets a copy of it; when it
  does not, a zero-filled vector. Either way the caller's vector is
  never changed, and the written copy is returned, without the supplied
  value's names, dimensions or class.

- `n` names an `int` parameter that receives a length. When the call
  does not give it, it is the common length of the pointer inputs; when
  it does, it may not exceed any of them. Outputs have length `n`,
  unless `out` gives them a fixed length, as in `out = c(y = 3)`.

The R function returns the C function's value, or its one output, or,
when there are several, a named list: the value first, named after the
function, then the outputs by parameter name.

`NA` crosses as R's C API carries it: an `NA` element of an integer
input arrives in C as `INT_MIN`, and `INT_MIN` in an `int` result or
output comes back as `NA`. An `NA` double is a NaN in C.

The signature is read from the source as c2mir's preprocessor leaves it,
so comments, `#if` branches and macros cannot change what is read. Any
other parameter or return type (`float`, `long double`, `unsigned`,
`char`, a typedef) is a compile error.

Code compiled by `cfunction()` runs with
[`.Call()`](https://rdrr.io/r/base/CallExternal.html)'s rights: it can
read and write any memory a C function can, and a loop that never ends
is not interrupted.

## See also

Other compilers:
[`mir_compile()`](https://pedrobtz.github.io/zumir/reference/mir_compile.md),
[`mir_function()`](https://pedrobtz.github.io/zumir/reference/mir_function.md)

## Examples

``` r
sq <- cfunction("
  void sq(int n, const double *x, double *y) {
    for (int i = 0; i < n; i++) y[i] = x[i] * x[i];
  }", out = "y", n = "n")
sq(x = c(1, 2, 3))
#> [1] 1 4 9

dot <- cfunction("
  double dot(int n, const double *x, const double *y) {
    double s = 0;
    for (int i = 0; i < n; i++) s += x[i] * y[i];
    return s;
  }", n = "n")
dot(x = c(1, 2, 3), y = c(4, 5, 6))
#> [1] 32
```
