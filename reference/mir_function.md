# Compile MIR text to an R function

`mir_function()` reads a module in MIR's textual form, compiles the
function `name` from it on `tier`, and returns an R function that calls
the result. It is for tests, teaching, and checking a code generator's
output by hand; MIR's text format is described in MIR's `MIR.md`.

## Usage

``` r
mir_function(text, name, tier = NULL)
```

## Arguments

- text:

  MIR text: one string with one or more modules.

- name:

  The name of the function to call.

- tier:

  `"native"` or `"interp"`; by default
  [`zumir_tier()`](https://pedrobtz.github.io/zumir/reference/zumir_info.md).

## Value

A function of class `zumir_function`. Its context lives as long as the
function does.

## Details

The function must be exported by its module (`export name`), and take
and return numbers only: the integer types `i8` to `u64`, `f` and `d`,
with at most one result. The R function has one argument per MIR
argument, by the same names, each a single number. An integer argument
must be a whole number that fits its type. Results of 8 to 32 bits are R
integers; 64-bit results and `u32` are doubles, and one beyond 2^53 is
an error, since a double could not hold it exactly.

The code may call the C math library and `memcpy`, `memmove`, `memset`,
`memcmp` and `strlen` by name (`import exp`); any other import is a link
error. Compiled code runs with the rights of
[`.Call()`](https://rdrr.io/r/base/CallExternal.html) code: a wrong
memory access crashes R, as it would in a package's C code.

## See also

Other compilers:
[`rfunction()`](https://pedrobtz.github.io/zumir/reference/rfunction.md)

## Examples

``` r
add <- mir_function("
  m: module
     export add
  add: func d, d:a, d:b
     local d:r
     dadd r, a, b
     ret r
     endfunc
     endmodule", "add")
add(1, 2)
#> [1] 3
```
