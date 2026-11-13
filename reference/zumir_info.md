# What zumir has, and what runs here

`zumir_info()` reports the vendored MIR, whether it has a code generator
for this CPU, which tiers run in this session and why the others do not,
and the memory MIR contexts hold. `zumir_tier()` is the tier code runs
on by default.

## Usage

``` r
zumir_info()

zumir_tier()
```

## Value

`zumir_info()`: a list with

- `mir_commit`: the upstream MIR commit vendored;

- `mir_patches`: the local patches, one per element;

- `target`: `TRUE` when MIR has a code generator for this CPU and was
  compiled in;

- `tier`: as `zumir_tier()`;

- `capabilities`: a data frame with columns `tier`, `capability`, `ok`
  and `reason` (`NA` where `ok`);

- `memory`: a list with `contexts`, the MIR contexts alive, and `bytes`,
  the memory they hold.

`zumir_tier()`: `"native"`, `"interp"` or `"none"`.

## Details

At load, zumir compiles and calls a small function natively and on MIR's
interpreter. Each tier is reported as four capabilities, which fail
separately: `compile` (MIR builds the function on this tier), `entry`
(it has an address C can call), `run` (calling it gives the right
answer) and `external` (it can call a C library function, `exp`). A tier
is available when all four hold.

Both tiers need memory that is writable and executable, the interpreter
too, since MIR generates small call bridges for it. Where the system
refuses that memory (SELinux `deny_execmem`, some seccomp policies and
hardened containers), neither tier runs, and `zumir_tier()` is `"none"`.
The `reason` column says what was refused. On macOS, R needs the
`com.apple.security.cs.allow-jit` entitlement that CRAN's build carries.

## Examples

``` r
zumir_tier()
#> [1] "native"
info <- zumir_info()
info$capabilities
#>     tier capability   ok reason
#> 1 native    compile TRUE   <NA>
#> 2 native      entry TRUE   <NA>
#> 3 native        run TRUE   <NA>
#> 4 native   external TRUE   <NA>
#> 5 interp    compile TRUE   <NA>
#> 6 interp      entry TRUE   <NA>
#> 7 interp        run TRUE   <NA>
#> 8 interp   external TRUE   <NA>
```
