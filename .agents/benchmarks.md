# Benchmarks

Each row says what was measured, on what, and the command that measured
it, so it can be run again. Numbers come from one machine and one run.
Read them as ratios, not as absolutes.

## Design §9, every row (Stage 10)

`Rscript tools/run-benchmarks`, from the package root with the package
installed; `tools/ci/platform.sh` runs it on every runner under the full
profile (`gh workflow run platforms.yaml --ref <branch>`). Each figure is
the median of 7 rounds. The reference C is built by `R CMD SHLIB` with R's
own compiler and `CFLAGS` (`-O2`). The kernel is a degree-6 polynomial by
Horner's rule at each of 1e6 doubles, summed; every route checks it got
the same sum. Run 43631019693 (2026-11-14, commit 4010732 on
`stage-10-benchmarks`), GitHub-hosted runners, R 4.6.2; "local" is an
x86-64 Mac (R 4.6.2, Apple clang) and a Linux container on it (R 4.6.1,
gcc).

| Row (target) | Linux x86-64 | Linux aarch64 | macOS arm64 | macOS x86-64 | Windows x86-64 | local macOS | local Linux |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `cfunction()`, ten lines, end to end (< 10 ms) | 3.06 ms | 3.98 ms | 3.60 ms | 5.74 ms | 4.00 ms | 5.72 ms | 5.06 ms |
| ten-function module, generated (< 5 ms) | 2.18 ms | 2.56 ms | 2.22 ms | 4.56 ms | 2.80 ms | 4.18 ms | 3.44 ms |
| generated code against the C compiler (within 1.5×) | 1.04× | **2.75×** | **2.04×** | 1.09× | 1.14× | 1.12× | **1.76×** |
| interpreter, against generated code (recorded) | 29.6× | 18.1× | 31.0× | 128.7× | 33.3× | 36.0× | 29.9× |
| `mir_compile()` against `cfunction()` (within 1.5×) | 1.09× | 0.92× | 1.12× | 1.17× | 1.25× | 1.19× | 1.26× |
| `mir_compile()` against byte-compiled R (recorded) | 39× faster | 36× faster | 50× faster | 61× faster | 42× faster | 37× faster | 33× faster |
| `cfunction()` call against a package's `.Call` wrapper (within 1.2×) | **2.22×** | **2.03×** | **1.88×** | **2.24×** | **2.80×** | **2.2×** | **2.28×** |
| the same call against `.Call` alone | 5.69× | 4.79× | 4.13× | 4.39× | 7.00× | 4.5× | 4.68× |
| `mir_compile()` compile, emitter against text (recorded) | 1.15× | 1.16× | 1.00× | 1.01× | 1.00× | 0.87× | 1.24× |

A call is 0.46 to 0.96 µs on the runners, against 0.20 to 0.43 µs for
a byte-compiled `function(x) .Call(sym, x)`.

Bold is a missed target. What the misses are:

- **Generated code on aarch64.** The C compilers contract `a*b + c` into
  one fused multiply-add there (§7, Floating point), halving the kernel's
  floating-point operations; MIR computes the unfused IEEE result, as it
  must to agree with R and with its interpreter bitwise. On x86-64, where
  `-O2` does not fuse, generated code is within 1.04× to 1.14× of the C
  compiler on the runners. The local Linux figure (1.76×) is gcc in a
  container on a laptop, against 1.04× on the Linux runner.
- **Call overhead.** Before Stage 10 a call cost 25.7 µs (85× `.Call`),
  nearly all of it R checking and converting the arguments over a data
  frame; Stage 10 moved that into C (`zumir_cfun_invoke()`), and the
  closure's body is the `.Call` alone. What is left is R's: R's JIT does
  not compile a small closure made at run time (an uncompiled closure
  call costs about what the `.Call` does), and the arguments go to C as a
  list. Embedding `.Call` and the symbol in the body saved 12% but would
  break a closure restored by `readRDS()` before zumir's own check, so it
  was not done. 1.2× of a wrapper needs the closure compiled, which needs
  `compiler` in `Imports`; the design has none.

### The macOS x86-64 runner's interpreter (one look, Stage 10)

The spike saw the interpreter 34 to 60× slower than generated code on the
macOS Intel runner, against 3 to 13× elsewhere. Stage 10 finds the cause
outside zumir and outside MIR's code: on the `macos-15-intel` runner MIR's
own interpreter benchmarks, built without R, run 7 to 11 times slower
than on every other runner (`loop-interp` 0.27 s against 0.03 to 0.04 s;
`sieve-interp` 23 s against 2 to 5 s), while its generated code, the C
and R itself run at the others' speed. On an x86-64 Mac outside CI the
same builds run at the others' speed too (`loop-interp` 0.05 s,
`sieve-interp` 3.7 s), and so does a Linux container on that Mac. An
interpreter is indirect branches, which binary translation handles worst,
so the runner may be x86-64 under Rosetta on Apple silicon: since Stage
10, `platform.sh` prints the CPU and `sysctl.proc_translated` on macOS,
and the next scheduled run says which. The ratio on the kernel elsewhere,
18 to 36×, is the interpreter on a tight floating-point loop; the spike's
3 to 13× was per solver call, where compiled C dilutes it.

### Interpreter dispatch: `MIR_DIRECT_DISPATCH` against computed goto

`platform.sh`, full profile: MIR's own `loop-interp` and `sieve-interp`
built twice, with R's compiler and flags, with the switch the package uses
and with computed goto, MIR's default. Seconds; the ratio is switch over
goto. Windows' `sieve-interp` runs a smaller sieve (MIR's own `FLAGS` for
Windows), so its times are not comparable with the others'.

| Runner | Compiler | loop: switch | loop: goto | sieve: switch | sieve: goto | sieve switch / goto |
| --- | --- | --- | --- | --- | --- | --- |
| Linux x86-64 | gcc | 0.035 | 0.031 | 3.02 | 2.07 | 1.46 |
| Linux aarch64 | gcc | 0.044 | 0.037 | 4.07 | 3.10 | 1.31 |
| macOS arm64 | Apple clang | 0.036 | 0.036 | 5.41 | 2.80 | 1.93 |
| macOS x86-64 | Apple clang | 0.291 | 0.270 | 26.03 | 22.94 | 1.13 |
| Windows x86-64 | gcc (Rtools) | 0.032 | 0.030 | 0.033 | 0.023 | 1.43 |
| local macOS x86-64 | Apple clang | 0.052 | 0.055 | 3.72 | 3.88 | 0.96 |
| local Linux container | gcc | 0.041 | 0.035 | 3.10 | 2.68 | 1.15 |
| local Linux container | clang | 0.056 | 0.035 | 5.00 | 2.45 | 2.04 |

The switch costs 1.1 to 2× on the sieve, most under clang. It is kept:
computed goto is a GNU extension that `-pedantic` reports, which CRAN's
checks use (design §4.2), and the interpreter is the fallback tier.

## mir_compile() compile time, by route (Stage 7, D17)

`Rscript tools/bench-mir-compile-routes.R 200`, from the package root with
the package installed. It uses 200 random functions from the tests'
generator: median 156 lines of MIR text, at most 617. The time is
`rf_context()` alone: the module built by the emitter, or read by MIR's
scanner, then linked. It is the median of 5 rounds, per function.

| Date | Build | Machine | Tier | Emitter | Text | Emitter / text |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-11-14 | lines split in R | x86_64 macOS, R 4.6.2 | native | 1.90 ms | 1.93 ms | 0.99 |
| 2026-11-14 | lines split in R | x86_64 macOS, R 4.6.2 | interp | 0.38 ms | 0.46 ms | 0.84 |
| 2026-11-14 | lines split in C | x86_64 macOS, R 4.6.2 | native | 1.94 ms | 2.04 ms | 0.95 |
| 2026-11-14 | lines split in C | x86_64 macOS, R 4.6.2 | interp | 0.41 ms | 0.45 ms | 0.90 |

On the native tier, MIR's generator and linking take almost all the time,
so the route changes little there: 1% to 5% between runs. On the
interpreter, skipping the scanner saves 10% to 16%. Between the two
builds the times differ by about as much as between runs, so the move to
C is not measured to change speed.
