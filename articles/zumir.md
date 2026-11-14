# Get started with zumir

Making R code fast usually means rewriting it in C or C++ and installing
a toolchain: Rtools on Windows, the Xcode command-line tools on macOS,
`gcc` elsewhere. Locked-down machines, Shiny servers and slim containers
often have none. zumir vendors MIR, a small JIT compiler, so a numeric R
function, or a short C function, can be compiled to native code from R
with nothing else installed.

``` r

library(zumir)
```

## Compile an R function

[`mir_compile()`](https://pedrobtz.github.io/zumir/reference/mir_compile.md)
takes a numeric R function and compiles it to native code. The result
has the same arguments and returns exactly what the R function returns.
It accepts doubles, loops over indices, conditions and the usual maths;
anything else is refused with the construct named. [What `mir_compile()`
accepts](https://pedrobtz.github.io/zumir/articles/mir-compile.md) lists
the subset, and every place it errors where R would answer differently.

``` r

csum_r <- function(x) {
  y <- numeric(length(x))
  acc <- 0
  for (i in seq_along(x)) {
    acc <- acc + x[i]
    y[i] <- acc
  }
  y
}
csum <- mir_compile(csum_r)

x <- c(1, 2, NA, 4)
csum(x)
#> [1]  1  3 NA NA
identical(csum(x), csum_r(x))
#> [1] TRUE
```

## Compile a C function

[`cfunction()`](https://pedrobtz.github.io/zumir/reference/cfunction.md)
takes the source of one C function and returns an R function that calls
it. The source defines one non-`static` function; `static` helpers
beside it are fine. Arguments named in `out` are allocated for you and
returned, and your own vectors are never changed. Lengths come from the
`int` argument you name.

``` r

sq <- cfunction("
  void sq(int n, const double *x, double *y) {
    for (int i = 0; i < n; i++) y[i] = x[i] * x[i];
  }", out = "y", n = "n")

sq(x = c(1, 2, 3))
#> [1] 1 4 9
```

## Return a value

A function that returns `int` or `double` gives that value back
directly.

``` r

dot <- cfunction("
  double dot(int n, const double *x, const double *y) {
    double s = 0;
    for (int i = 0; i < n; i++) s += x[i] * y[i];
    return s;
  }", n = "n")

dot(x = c(1, 2, 3), y = c(4, 5, 6))
#> [1] 32
```

Arguments can be `int`, `double`, `const int *`, `const double *`,
`int *` and `double *`. The source may use the C maths library and a few
memory and string functions, but it can’t include system headers or call
R’s API.

## Write MIR directly

[`mir_function()`](https://pedrobtz.github.io/zumir/reference/mir_function.md)
takes MIR’s textual IR instead of C. It is useful for tests, for
learning MIR, and for checking a lowering by hand.

``` r

f <- mir_function("
  m: module
     export add
  add: func d, d:a, d:b
     local d:r
     dadd r, a, b
     ret r
     endfunc
     endmodule", "add")

f(1, 2)
#> [1] 3
```

## Native code or the interpreter

zumir checks generated code when it loads. If that check fails, it falls
back to MIR’s interpreter: results are the same, only slower. Where MIR
has no code generator for the CPU, or the system refuses memory that is
both writable and executable, no tier runs and nothing can be compiled.
[`zumir_tier()`](https://pedrobtz.github.io/zumir/reference/zumir_info.md)
says which tier this session uses, and
[`zumir_info()`](https://pedrobtz.github.io/zumir/reference/zumir_info.md)
says why. You can also ask for the interpreter yourself.

``` r

zumir_tier()
#> [1] "native"
zumir_info()
#> $mir_commit
#> [1] "dc2c763f1c8bc746102ce87933d866335f9e6c06"
#> 
#> $mir_patches
#>  [1] "ea7f1643 Fix two warnings Rtools gcc 14 reports"                                      
#>  [2] "773a4834 Release code memory with a zero size on Windows"                             
#>  [3] "dafb8e1b Remove variables clang -Wall reports as unused"                              
#>  [4] "f35a0021 Drop the semicolons after DEF_* macros at file scope"                        
#>  [5] "6edb5ce2 Give mem_page_size a prototype"                                              
#>  [6] "1571f134 Use snprintf with the buffer's size instead of sprintf"                      
#>  [7] "0b8063f7 Clear curr_func and curr_module first in MIR_finish"                         
#>  [8] "4c27a18f Return NULL from MIR_init2 and _MIR_init instead of exiting"                 
#>  [9] "5803194c Load unaligned hash and reduce keys with memcpy"                             
#> [10] "1c4b1cca Declare MIR_insn's operands as a flexible array member"                      
#> [11] "c65e7044 Report code-memory failures through the error function"                      
#> [12] "3e643e83 Spill float arguments in the Win64 interpreter shim"                         
#> [13] "ca84f3e4 End MIR_output_item's expr data case"                                        
#> [14] "0068c2fd Drop the semicolons after DEF_* macros at file scope in c2mir"               
#> [15] "df2df6f5 Remove a variable c2mir declares and never uses"                             
#> [16] "3762da20 Use snprintf with the buffer's size in c2mir"                                
#> [17] "3bfb8d45 Let an embedder give headers in memory and forbid header files"              
#> [18] "7f6c4a03 Allocate and free all of c2mir's memory with MIR's allocator"                
#> [19] "f7672f28 Fold constant shifts without undefined behaviour in c2mir"                   
#> [20] "96763195 Read a declaration's decl_spec only when the node is a declaration"          
#> [21] "b6e92b74 Fold integer constants by MIR's wrapping rules, without undefined behaviour" 
#> [22] "2ad09397 Read call offsets and addresses in x86-64 code with memcpy"                  
#> [23] "55047839 Interpret integer operations by MIR's wrapping rules, and memory with memcpy"
#> [24] "feacf4fe Stop at the end of the unit after a ## concatenation"                        
#> [25] "4fb6f621 Fix what mingw gcc reports on c2mir's Windows x86-64 path"                   
#> [26] "06104552 Empty the buffer when a macro argument has no start marker"                  
#> [27] "46ed8157 Generate no code from a source with parse errors"                            
#> [28] "a603752b Keep the current stream valid after the end of the unit"                     
#> 
#> $target
#> [1] TRUE
#> 
#> $tier
#> [1] "native"
#> 
#> $capabilities
#>     tier capability   ok reason
#> 1 native    compile TRUE   <NA>
#> 2 native      entry TRUE   <NA>
#> 3 native        run TRUE   <NA>
#> 4 native   external TRUE   <NA>
#> 5 interp    compile TRUE   <NA>
#> 6 interp      entry TRUE   <NA>
#> 7 interp        run TRUE   <NA>
#> 8 interp   external TRUE   <NA>
#> 
#> $memory
#> $memory$contexts
#> [1] 4
#> 
#> $memory$bytes
#> [1] 2004718

csum_interp <- mir_compile(csum_r, tier = "interp")
```

## Next

See the [reference
index](https://pedrobtz.github.io/zumir/reference/index.md) for every
function. Packages that generate code, such as zupmxode, use zumir’s C
API instead of these R functions.
