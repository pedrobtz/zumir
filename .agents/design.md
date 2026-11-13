# RFC 0073: `zumir`, native code from R without a compiler, with MIR

Adopted from [RFC 0073](https://github.com/pedrobtz/packages/blob/72859143843cf3d7407bcbcaec2fd41d677978ae/rfcs/not-started/0073-zumir-native-code-without-a-compiler.md)
on 2026-11-13. This file is the single source of truth for the design; the
RFC is now a pointer here.

- **Status:** Adopted. The package repository exists; each decision
  marked (C) stays the maintainer's to overturn, and those marked (M) are
  the maintainer's. The MIR spike ([results](https://github.com/pedrobtz/mirspike/blob/main/RESULTS.md), 2026-11-13) decided that the package is
  built; RFC 0047 `zupmxode` rests on that answer.
- **Date:** 2026-11-12.
- **Packages:** `zumir` (<https://github.com/pedrobtz/zumir>; crandb
  returned `not_found` on 2026-11-12).
  No `Imports`. `Suggests: testthat`. Vendors MIR (MIT): the core, the
  generator, the interpreter and the C-to-MIR compiler `c2mir`. One
  shared object. MIT.
- **Source:** the MIR repository, `vnmakarov/mir`, at `master` commit
  `dc2c763` (2026-10-23), read through the GitHub API on 2026-11-12:
  `README.md` (the platform disclaimer, the speed and size table, the
  porting notes), `MIR.md` (contexts and threads), `CUSTOM-ALLOCATORS.md`,
  `mir.h` (`MIR_set_error_func`), `mir.c` (the default error handler,
  which calls `exit (1)`), `mir-code-alloc-default.c` (`MAP_JIT` and
  `pthread_jit_write_protect_np` on macOS, `VirtualProtect` on Windows),
  `c2mir/README.md` and the per-target header directories, and the CI
  workflows (Windows built with CMake and MSVC only); open issues #163
  (gcc-mingw build error), #496 (Windows `VirtualFree`), #394 and #286
  (`_Thread_local`); the latest release, v1.0.0 (2024-05-27); the
  contributor list (about 4,070 of 4,200 commits by the principal
  author). `luajr` 0.3.1 on CRAN (2026-06-04) as precedent for a CRAN
  package that generates machine code at run time. PyPI, searched on
  2026-11-12: no MIR binding (`pymir` is an unrelated project). The MIR
  spike (`pedrobtz/mirspike`, 2026-11-13) built and measured MIR alone;
  nothing of this package is built yet.

## 1. What zumir is

`zumir` brings MIR, Vladimir Makarov's lightweight JIT compiler, to R in
three ways:

1. **A provider** (`LinkingTo: zumir`) for family packages that generate
   code: they build MIR functions through a versioned C API table and get
   back native function pointers, or MIR's interpreter where MIR has no
   code generator for the CPU (§4.4). `zupmxode` (RFC 0047) is the first consumer.
2. **C without a toolchain** for R users: `cfunction()` compiles a short
   C function with MIR's own C compiler and returns an R function that
   calls it. No Rtools, no Xcode command-line tools, no `gcc`.
3. **Native code from R itself**: `rfunction()` compiles a numeric R
   function, a subset of R written as R, to native code and returns an R
   function with the same arguments and the same results. R's parser
   reads it; nothing else is needed.

`cfunction()` and `rfunction()` are what the package offers R users; the
provider is what the family builds on.

MIR's README measures its generator at about 250 µs for a small C
program, about a hundredth of `gcc -O2`'s time, with generated code
within about ten per cent of `gcc -O2`'s speed, and its interpreter at
six to fourteen times slower than generated code. The spike measured
generated code at 0.7 to 1.3 times the C compiler's speed on three model
right-hand sides, and the interpreter at 3 to 13 times slower per solver
call (up to 22 on the right-hand side alone, and 34 to 60 on the macOS
Intel CI runner, not explained). It targets x86-64 and
aarch64 on Linux and macOS, and ppc64le, s390x and riscv64 on Linux.

The one-line statement that governs every decision below:

> **Native code from R without a compiler, and MIR never takes R
> down.**

What it is not:

- **Not a full C compiler.** `c2mir` implements C11 with gaps (open
  issues #394 and #286), and `cfunction()` takes plain numeric functions,
  not programs (§5, D5).
- **Not a compiler for all of R.** `rfunction()` takes numeric kernels
  (§4.6): doubles, loops, conditions and a math table, not data frames,
  closures, environments or calls into other R functions.
- **Not a sandbox.** Code compiled by `cfunction()` runs in the R process
  with the rights `.Call()` code has. A bad pointer crashes R, as it
  would in a package's C code.
- **Not `inline` or `Rcpp`.** Those drive the system compiler and accept
  any C or C++ the compiler does. This package accepts less and needs
  nothing installed.
- **Not LLVM.** MIR trades the last ten per cent of speed for a compiler
  small enough to vendor and fast enough to run per model.

## 2. Scope

In v0.1.0:

- **Vendored MIR**: core, generator, interpreter and `c2mir`, for
  x86-64 and aarch64 as tested targets, and ppc64le, s390x and riscv64
  built when the host matches (D7).
- **The C API** (§4.3): one versioned table of function pointers covering
  context creation, module and function building, linking, generation
  and interpretation, plus `zumir`'s own functions for error recovery,
  tier detection and freeing code.
- **MIR text from R**: `mir_function()` takes MIR's textual IR and returns
  an R function, for tests, teaching, and checking a lowering by hand.
- **`cfunction()`**: C source with numeric signatures, compiled by
  `c2mir` and wrapped as an R function (§5).
- **`rfunction()`**: a numeric R function compiled to MIR through the
  same emitter consumers use (§4.6), with R itself as its test oracle.
- **Two tiers**, native and interpreter, chosen automatically with
  override, reported by `zumir_info()`. Both need executable memory; where
  the system refuses it, what runs is Q5.

Out of v0.1.0 (§13): caching generated code between sessions, MIR's
parallel generator branch, a Python package, `mir2c`, C code that
includes R's headers or calls R's API, and `rfunction()` beyond §4.6's
subset (element-wise vector arithmetic, integer and logical results).

## 3. Position in the `zu*` family

| | zumir |
| --- | --- |
| Role | provider and tool |
| R prefix | `mir_`, with `cfunction()` and `rfunction()` |
| Info function | `zumir_info()` |
| Root condition class | `zumir_error` |
| Public C prefix | `zumir_`, and MIR's own `MIR_` names through the table |
| From C | yes, by `R_GetCCallable("zumir", "zumir_api")` |
| Hides symbols | yes; the table is the only way in |
| Vendored code | MIR (MIT) |
| Language | en-GB |

**Consumers.**

- **`zupmxode`** (RFC 0047): lowers its IR to MIR, one function per model
  block, and calls the generated code from its solvers on every thread.
  The first consumer and the reason for the provider half.
- **Later, by the same route:** an SBML or FMI right-hand side engine
  (RFC 0053, `base-layer-candidates.md`), and any family package that
  would gain from compiling a pattern, a filter or a kernel at run time.
- **R users**, through `cfunction()`: Windows users without Rtools,
  Shiny servers and containers with only R, and locked-down machines.

**What exists elsewhere.**

- `inline`, `Rcpp::cppFunction()`, `cpp11::cpp_source()`: need the
  system toolchain.
- `luajr` (CRAN 0.3.1): LuaJIT in R. Compiles Lua, not C, and its fast
  path is code that stays inside Lua; a C solver calling into it crosses
  a boundary per call.
- `nanowasm` (this family): a WebAssembly interpreter. Runs modules
  built elsewhere; generates no native code.
- No binding to MIR exists for R or, as far as PyPI shows, for Python.
- **TinyCC** (`tcc`, LGPL-2.1, `mob` branch, 0.9.28rc) was weighed as an
  alternative on 2026-11-13. It has a mature C99 front end, a long-standing
  Windows target, several maintainers, and a way round refused W+X memory
  (`CONFIG_SELINUX`: one file mapped read-execute and read-write). It loses
  where `zupmxode` needs most: its code runs at about half `gcc -O2`'s
  speed (MIR's README, geomean 0.50 against MIR's 0.91), it has no API for
  building code, only C text, and no interpreter; and LGPL changes the
  package's licence. It stays a fallback for `cfunction()` alone (D19),
  and its dual mapping is Q5's option (a).

**Model languages.** NONMEM's NM-TRAN, Monolix's MLXTRAN, rxode2's syntax,
mrgsolve's C++ blocks and SBML reach MIR only through `zupmxode`'s IR:
`zupmxio` and `zusbml` translate them into `zupmxode` models, and
`zupmxode` lowers its IR through the C API. None goes through `c2mir`,
which stays the front end of `cfunction()`.

## 4. Architecture

### 4.1 What is vendored

From `dc2c763`, under `src/vendor/mir/` (the portfolio's vendor path),
unchanged except for the patches in
§4.2: `mir.c`, `mir.h`, the `mir-*.h` support headers, `mir-gen.c`,
`mir-gen.h`, `mir-interp.c` (included by `mir.c`), the per-target
`mir-<target>.c`, `mir-<target>.h` and `mir-gen-<target>.c` for the five
targets, the default allocators, and `c2mir/` with its per-target
directories. Not vendored: the test and benchmark trees, `llvm2mir`,
`mir2c`, the drivers and utilities. `tools/vendor.sh` records the
commit, the file list and the patches, and re-applies them.

Sizes from the README: about 23,000 lines for the core and generator,
11,000 for the core and interpreter, and `c2mir.c` at 550 KB of source.

### 4.2 Patches

Each is a numbered patch file, offered upstream where it is not
R-specific.

- **No exit, no abort, no printing.** MIR's default error handler prints
  and calls `exit (1)`, and `MIR_init` can reach it before a handler is
  installed. `zumir` installs its own with `MIR_set_error_func` on every
  context, which `longjmp`s back to the `zumir` call that entered MIR,
  and patches the paths that run before installation. Writes to
  `stderr` and `stdout` go to a buffer that `zumir` turns into the R
  condition's message. R CMD check flags all three, and an exit would
  take the analyst's session down.
- **Rtools gcc-mingw.** MIR's CI builds Windows with MSVC only. Under
  Rtools gcc 14.3 it builds (issue #163 does not reproduce) with two
  warnings R CMD check counts as significant (`-Wreturn-type` in
  `item_name_ptr`, `-Wuninitialized` in `mir-varr.h`), and
  `default_mem_unmap` passes a size to `VirtualFree(…, MEM_RELEASE)`, so
  every unmap fails and code pages leak (#496). All three are patched.
- **Strict ISO C.** Under `-pedantic`, gcc 16 reports about 90 stray `;`
  after the `DEF_VARR`/`DEF_DLIST`/`DEF_HTAB` macro invocations and the
  interpreter's computed `goto *`, as significant install warnings. The
  semicolons are removed; the interpreter is built with
  `MIR_DIRECT_DISPATCH`.
- **Code-memory failures are errors.** `_MIR_set_code` ignores the result
  of `mem_protect` and writes into a page that may still be execute-only:
  where writable-and-executable memory is refused, the process dies with
  SIGSEGV. The patch checks every `mem_map` and `mem_protect` result and
  reports failure through the error function (D11).
- **Finishing after a trapped error.** An error trapped inside
  `MIR_finish_func` leaves `curr_func` set; `MIR_finish` then reads it
  after freeing it. The patch clears it first.
- **c2mir's output.** `c2mir` writes diagnostics to a `FILE *`
  (`c2m_options->message_file`) and calls `abort ()` in seven places. Both
  are patched: messages go to a callback that fills the R condition's
  message, and the aborts go through the error function.
- **Other CPUs.** `mir.c` and `mir-gen.c` stop with `#error "undefined or
  unsupported generation target"` on a CPU MIR does not target. The build
  leaves MIR out there (a `configure` test, or a preprocessor guard in a
  wrapper file), so the package still installs and every entry reports
  that there is no target (§7).
- **No `-fsigned-char`.** Upstream builds with it; R CMD check flags it as
  a non-portable flag. Replaced by a source fix or a `configure` test.
- **Symbol hiding.** MIR's external symbols are given hidden visibility
  so they cannot clash with another package that vendors MIR, or with a
  future version of this one loaded beside it.

### 4.3 The C API

A consumer calls `R_GetCCallable("zumir", "zumir_api")` once and gets a
pointer to a `const zumir_api_v1` struct. Its first field is the API
version; later versions only append fields. `inst/include/zumir.h`
declares it and provides inline wrappers, so consumer code reads as
ordinary MIR code:

```c
#include <zumir.h>

zumir_ctx *z = zumir_ctx_new(&status);      /* MIR context + error trap */
MIR_context_t ctx = zumir_mir(z);
MIR_module_t mod = MIR_new_module(ctx, "m");
/* ... MIR_new_func, MIR_new_insn, MIR_finish_func ... */
MIR_finish_module(ctx);
void *fn = zumir_compile(z, mod, "rhs", ZUMIR_TIER_AUTO, &tier, &status);
/* fn is a C function pointer on either tier */
zumir_ctx_free(z);                          /* frees code with the context */
```

The table carries the subset of MIR's API a code generator needs
(context, module, function, register, operand and instruction
construction, finishing, loading, linking, generator and interpreter
interfaces, external symbol loading), about forty entries, plus the
`zumir_` functions. Every `zumir_` entry returns a status and never
`longjmp`s across the consumer's frames: an error inside MIR is caught
at the `zumir` boundary and reported (D4).

**Helpers for lowering.** `zumir.h` also carries a small emitter for
straight-line `double` code (D13): new registers, loads and stores at an
offset from a pointer argument (`y[i]`, `par[k]`), the four operations,
negation, constants, comparisons and branches, and calls to the math
table by name with their prototypes made once per module. It is the
spike's emitter, made general; `zupmxode`'s lowering becomes a loop over
its IR with one helper call per instruction, and `rfunction()` (§4.6) is
built on the same helpers.

**Imports.** A consumer registers its own C functions in its context
(`zumir_import(z, "expit", &expit)`, MIR's `MIR_load_external`), and the
import resolver for that context accepts them beside D6's list (D14).
`zupmxode` needs `logit`, `expit`, `probit` and friends this way.
`cfunction()` keeps D6's fixed list.

**Threads.** MIR's own rule, from `MIR.md`: MIR functions may be used
from different threads without synchronisation if each thread works in
its own context. `zumir` keeps that rule and adds one: generated code
may be called from any thread, because it is code. A consumer that
compiles once and runs on many threads, as `zupmxode` does, needs no
lock beyond its own compile step. **Interpreted code is not shareable:**
the interpreter keeps per-call state in its context, and one interpreted
function called from eight threads gave wrong answers on every leg of the
spike. On the interpreter tier each thread gets its own context, compiled
before the parallel region (D10). The error trap is thread-local.

**Memory.** Contexts are created with `MIR_init2` and `zumir`'s
allocators, which count what each context holds so that
`zumir_info()` can report it and a finaliser can free it. Executable
memory comes from MIR's default code allocator: `mmap` with `MAP_JIT`
and `pthread_jit_write_protect_np` on macOS, `VirtualProtect` on
Windows, `mmap` and `mprotect` elsewhere.

### 4.4 Tier detection

At load, `zumir` generates and calls a one-instruction function. If
that fails, because the architecture is not one MIR targets or because
the system refuses memory that is writable and executable, native code
is reported unavailable for the session and `ZUMIR_TIER_AUTO` means the
interpreter. `zumir_info()` says which, and why.

Two facts from the spike constrain this. The probe must *fail*, not
crash, so it rests on the code-memory patch (§4.2, D11). And MIR's
interpreter needs executable memory too: `MIR_set_interp_interface`
generates a machine-code shim per function, so where W+X memory is
refused neither tier can be called through a C function pointer. What
runs there is open (Q5), decided at the start of the stage that builds
this.

The probe runs before anything else maps code, in `.onLoad`, so a refusal
is found before a consumer has compiled anything. On macOS, `MAP_JIT`
works because CRAN's R carries the `allow-jit` entitlement; an R built or
embedded without it was not tested and must be detected by the same
probe. The interpreter's figures above were measured with computed-goto
dispatch; the `-pedantic` patch switches it to `MIR_DIRECT_DISPATCH`,
whose cost is measured again with the benchmarks (§9).

### 4.5 `cfunction()`

1. The source is prefixed with `zumir`'s prelude: declarations for the
   math functions and the handful of string and memory functions
   allowed (D6), and `c2mir`'s own built-in `stddef.h`, `stdint.h`,
   `stdbool.h`, `stdarg.h`, `float.h` and `limits.h`. No system header is
   read, because a machine without a toolchain has none (D5).
2. `c2mir` compiles it in memory to a MIR module. Its diagnostics,
   with line and column, become the R error.
3. The module is linked with an import resolver that resolves only the
   prelude's names, to the C library R itself uses.
4. The function is generated on the session's tier.
5. An R closure is returned that checks argument types and lengths,
   allocates outputs, and calls the function through one `.Call`
   wrapper.

Signatures are limited to what can be marshalled without R's API (§5).

### 4.6 `rfunction()`

`rfunction(f)` takes an R function and returns one with the same formals.
R's parser has already read `f`; an R walker checks its body against the
subset below, infers each argument's shape, and lowers it to one MIR
function. Anything outside the subset is a compile error naming the
construct and its position, so a function either compiles or says why;
it never silently means something else (D15, D16).

**Two routes to MIR (D17).** The walker first emits MIR *text*, compiled
by `mir_function()`'s path: no new native code, and the text is there to
read when a lowering is wrong. A second route then drives the §4.3
emitter from R through one `.Call` bridge, without text. The text route
is its oracle: for every function, both routes must print the same MIR
module (`MIR_output`) and return the same results, and the benchmarks
record what the bridge saves in compile time.

**Imports (D18).** Generated code calls a fixed internal list, resolved
only for `rfunction()`: the C library's `exp`, `log`, `log1p`, `expm1`,
`sqrt`, `fabs`, `floor`, `ceil`, `trunc`, `sin`, `cos`, `tan`; R's
`R_pow` for `^`, a pure function of libR that neither allocates nor
raises a condition; and zumir's own C ports of R's `%%`, `%/%` and
two-argument `min`/`max` (R's `myfmod`, `myfloor` and `rmin`/`rmax`
rules). Nothing in the list calls back into R.

**Values.**

- Every argument is a `double` scalar or a `double` vector. An argument
  that is indexed (`x[i]`), passed to `length()` or to `seq_along()` is a
  vector; any other is a scalar, checked to be length one at the call.
  An argument that is not `double` (integer, logical, character) is
  refused at the call with a classed error: R's own function would
  compute in another type and return another type, so converting would
  change the answer (D16).
- Locals are `double` scalars, `logical` scalars (from comparisons,
  `is.na()`, `!`, `&&`, `||`), `integer` loop counters, or `double`
  vectors made by `numeric(<size>)`, where `<size>` uses only arguments
  and `length()`. The wrapper evaluates the size in R and allocates the
  vector before the call, so generated code never allocates.
- A logical in arithmetic is `1`, `0` or `NA`, as in R. An integer
  counter in arithmetic with another integer stays integer, as in R; an
  overflow, where R would give `NA` with a warning, is an error (D16).
  With a double it becomes double.
- The result is a `double` scalar expression or a `double` vector local,
  returned by the last expression or `return()`. A result R would
  return as integer or logical (`length(x)`, a comparison, the counter)
  is refused at compile time; `as.double()` around it is accepted.

**Code.**

- `{`, `<-` and `=` to locals, `x[i] <- v` on vector locals.
- `if`/`else`, `while`, `break`, `next`, and `for` over `seq_len(n)`,
  `seq_along(x)` and `a:b`. `a:b` counts down when `a > b`, as R's does
  (`1:0` is `1, 0`); a bound that is `NA` or not a whole number is an
  error, as is a `seq_len()` argument that is negative, `NA` or not a
  whole number (R truncates `seq_len(2.5)`; D16).
- `+ - * /`, unary `-`, `^`, `%%`, `%/%`; `==`, `!=`, `<`, `<=`, `>`,
  `>=`; `!`, `&&`, `||` with R's three-valued logic: `NA && FALSE` is
  `FALSE` and `NA && TRUE` is `NA`, as in R.
- The math table above, `length`, `is.na`, `is.nan`, `is.finite`.

**Errors and warnings.** Generated code cannot call R, so it returns a
status, and the wrapper acts on it after the call:

- `if (NA)` and `while (NA)` raise R's "missing value where TRUE/FALSE
  needed", `if (NaN)` R's "argument is not interpretable as logical",
  as `zumir_error` conditions.
- Where R warns (`log(-1)` and `sqrt(-1)`, "NaNs produced"; `%%` with
  "probable complete loss of accuracy in modulus"), the wrapper issues
  the same warning once per kind per call. R may warn more than once in
  a loop; the count is the one documented difference that is not an
  error.
- An index out of range, not a whole number or `NA`, and an integer
  overflow, are errors (D16).

**Semantics.** Arithmetic is IEEE double, unfused, as R's evaluator does
it, so every non-NaN result equals R's bit for bit, signed zeros
included, on every platform (§7 on FMA). Where a NaN results from an `NA`
and a `NaN` together, R itself documents that either may come out; the
same holds here. Every other NaN or `NA` result is the one R gives.

**Oracle.** R itself: every test calls `f` and `rfunction(f)` on the
same inputs, random and chosen (`NA`, `NaN`, `±Inf`, `-0`, subnormals,
empty vectors), and compares with `identical(num.eq = FALSE)` plus a
sign-of-zero check, allowing only the NA-or-NaN case above.

## 5. Public R API (complete v0.1.0 surface)

```r
sq <- cfunction("
  void sq(int n, const double *x, double *y) {
    for (int i = 0; i < n; i++) y[i] = x[i] * x[i];
  }", out = "y", n = "n")
sq(x = c(1, 2, 3))          # 1 4 9; n from length(x), y allocated

dot <- cfunction("
  double dot(int n, const double *x, const double *y) {
    double s = 0;
    for (int i = 0; i < n; i++) s += x[i] * y[i];
    return s;
  }", n = "n")

f <- mir_function("
  m: module
     export add
  add: func d, d:a, d:b
     local d:r
     dadd r, a, b
     ret r
     endfunc
     endmodule", "add")
f(1, 2)                      # 3

ss <- rfunction(function(x) {
  s <- 0
  for (i in seq_along(x)) s <- s + x[i] * x[i]
  s
})
ss(c(1, 2, 3))               # 14, identical to the R function's answer

csum <- rfunction(function(x) {
  y <- numeric(length(x))
  acc <- 0
  for (i in seq_along(x)) {
    acc <- acc + x[i]
    y[i] <- acc
  }
  y
})

zumir_info()                 # MIR commit, targets, tier, patches, memory
zumir_tier()                 # "native" or "interp" for this session
cfunction(src, tier = "interp")
rfunction(f, tier = "interp")
```

Argument types: `int`, `double`, `const int *`, `const double *`,
`int *` and `double *`, with lengths named by an `int` argument or fixed.
Return types: `void`, `int`, `double`. Inputs are not copied unless the
C type is non-`const`; outputs named in `out` are allocated by the
wrapper and returned, as a list when there are several.

## 6. Errors

- `rfunction()` refuses a construct outside §4.6 at compile time, with
  the construct named and its position in the source; errors at run time
  (§4.6) are `zumir_error` conditions with R's own wording where R has
  one.

- Compile errors from `c2mir` carry the line and column of the user's
  source, not of the prelude.
- MIR errors, in either API, become a `zumir_error` condition from R and
  a status code from C. The process never exits on MIR's account (D4).
- Calls through `cfunction()` validate types and lengths in R before the
  call. Inside the call the code is the user's; a crash there is a crash,
  as it is in `.Call()`.
- No native code generator for the CPU is not an error: the interpreter
  runs and `zumir_info()` says why. No executable memory at all is
  reported the same way, with what runs decided by Q5.

## 7. Limits and portability

- **Tested targets**: x86-64 on Linux, macOS and Windows; aarch64 on
  macOS and Linux. These are the r-actions legs.
- **Built, not tested here**: ppc64le, s390x and riscv64 on Linux, which
  MIR targets and its own CI tests. Debian builds R on them.
- **Elsewhere**: the package installs with MIR left out of the build
  (§4.2), reports that MIR has no target, and `cfunction()`,
  `rfunction()` and the provider API return an error condition and a
  status code.
- **Executable memory**: both tiers need it (§4.4). On macOS it works
  because CRAN's R binary carries the `allow-jit` entitlement that
  `MAP_JIT` requires. Where policy refuses W+X memory (SELinux
  `deny_execmem`, PaX, some sandboxes), §4.4 detects it and Q5 decides
  what runs.
- **Floating point**: generated code computes the unfused IEEE result.
  C compilers contract `a*b + c` into a fused multiply-add on aarch64, so
  comparisons with compiled C allow one ulp there; native and interpreted
  code agree bitwise everywhere.
- **CRAN**: generating machine code at run time has a precedent in
  `luajr`. `cran-comments.md` says what the package does and why, and
  that no code is downloaded.

## 8. Testing

- MIR's own c2mir-free tests on the vendored copy; `mir-tests/*.mir`
  through `mir_function()`, by `run-test`'s round trip and hand-written
  expected results; and the `c-tests` corpus through `c2mir` by its own
  rules (`.expect` outputs and exit status), with a test-only import list
  for `printf`, `abort` and `exit`. On every leg, on both tiers.
- Every `cfunction()` example against a reference written in R.
- `rfunction()` against R itself (§4.6): each construct of the subset,
  and a generator of random functions within the subset run on random
  and chosen inputs, compared with `identical()`; every refused construct
  refused with its name.
- Native against interpreter on the same modules: identical results.
- Threads: one context per thread compiling at once, and one compiled
  function called from eight threads, against serial results.
- Error recovery: modules that are invalid on purpose, through both APIs,
  in a loop of 10³, with the session alive and no memory growth after.
- Finalisers: contexts and code freed when R collects the objects that
  hold them.
- A fuzz target over `c2mir` input through `cfunction()`'s path, built
  standalone without R; crashes found go upstream with a reproducer.

## 9. Performance targets

Measured by `tools/run-benchmarks`, recorded in `.agents/benchmarks.md`.

| Workload | Target |
| --- | --- |
| `cfunction()` on a ten-line function, end to end | under 10 ms |
| lowering-sized module, ten functions, generated | under 5 ms |
| generated code on a numeric kernel against `gcc -O2` | within 1.5× |
| interpreter on the same kernel | recorded |
| `cfunction()` call overhead against `.Call` of compiled C | within 1.2× |
| `rfunction()` on a loop kernel against the same kernel by `cfunction()` | within 1.5× |
| `rfunction()` against R's byte-compiled function on the same kernel | recorded |
| `rfunction()` compile time, emitter bridge against MIR text | recorded |
| interpreter with `MIR_DIRECT_DISPATCH` against computed goto | recorded |

## 10. Roadmap

The order of work, the stages and their exit criteria are in
[roadmap.md](roadmap.md). The MIR spike's questions, its decision table and
its results are its Stage 0.

## 11. Acceptance criteria for v0.1.0

- Every behaviour in §5 and §6 has a passing test.
- MIR's test subset passes on every tested target, on both tiers.
- The error-recovery loop leaves the session alive and memory flat.
- No call to `exit`, `abort` or the standard streams remains in the
  shared object; R CMD check is clean on all r-actions legs.
- Every §9 row is measured and recorded.
- `zupmxode` builds and passes its Stage 2 against this package.

## 12. Decisions

Marked **(M)** when the maintainer decided it and **(C)** when this RFC
decides it and the maintainer may overturn it.

- **D1 (M), 2026-11-12; amended 2026-11-13.** One package does both
  jobs: it vendors MIR for the family and gives R users `cfunction()` and
  `rfunction()`, its two selling points. The provider alone would
  have no use outside the family; `cfunction()` is what makes the
  package worth publishing by itself, and it shares every line of the
  vendoring and the error trap.
- **D2 (M), 2026-11-13.** MIR is vendored at a commit named by its full
  hash, `dc2c763f1c8bc746102ce87933d866335f9e6c06` (`master`, 2026-10-23), never by a branch name and not from
  the v1.0.0 tag. The tag is from May 2024; that commit carries two and a
  half years of fixes. This deviates from `.claude/shared/native-code.md`
  ("pin a stable upstream release"), on purpose. `tools/vendor.sh` records
  the hash, and an update is
  a deliberate pull request that re-runs the spike's checks (the MIR
  tests, the three right-hand sides, threads, executable memory, the
  error loop) on every leg.
- **D3 (C).** MIR's API is exposed through a versioned table of function
  pointers, under MIR's own names, plus `zumir_` functions. Wrapping
  every MIR call in a `zumir_` equivalent would double the surface and
  hide MIR's documentation from consumers; exposing the raw symbols is
  not possible across R packages.
- **D4 (C).** MIR never terminates the process. The error handler is
  replaced on every context, the paths before installation are
  patched, and every entry from R or from the table is a recovery point.
- **D5 (C).** `cfunction()` reads no system header. Machines without a
  toolchain have none, and the prelude is what makes the same source
  compile on every platform.
- **D6 (C).** The import resolver resolves a fixed list: the C math
  library, `memcpy`, `memmove`, `memset`, `memcmp`, `strlen`. Any other
  external name is a link error, so a function behaves the same on
  every platform and cannot reach into R's internals by name.
- **D7 (C).** x86-64 and aarch64 are the tested targets; the other three
  MIR targets are built when the host matches and are not promised.
- **D8 (M), 2026-11-12.** The name is `zumir`: `zu` and the library's
  name, as for `zuprima`. The R prefix is `mir_`, with `cfunction()`
  unprefixed because it says what it does.
- **D9 (C).** `c2mir` ships in v0.1.0, not later. Without it there is no
  `cfunction()` and so no reason to publish; with it, `zupmxode` still
  never compiles C text, because it emits MIR directly.
- **D10 (C), 2026-11-13.** On the interpreter tier, one MIR context per
  thread: a consumer compiles the module once per thread before its
  parallel region. Native code is still compiled once and shared. From
  the spike's thread results (§4.3).
- **D11 (C), 2026-11-13.** A code-memory failure (`mem_map`,
  `mem_protect`) is an error through the context's error function, never
  a write into protected memory. Patched in MIR and offered upstream.
- **D12 (C), 2026-11-13.** MIR is kept as upstream plus a numbered patch
  queue, not a fork. Upstream merged 56 of its last 60 outside pull
  requests, in bursts months apart; each patch is offered upstream and
  dropped from the queue when a pinned commit contains it. A fork is
  reconsidered if a needed patch is refused or the queue stops rebasing.
- **D13 (C), 2026-11-13.** `zumir.h` carries a straight-line `double`
  emitter (§4.3). Lowering is the same work for every consumer. The
  spike's emitter covered loads, stores and the five arithmetic
  instructions only, and gave results bitwise equal to C; comparisons,
  branches and calls are new and tested against C in the C API's stage; `zupmxode`'s IR stays its own
  (RFC 0047 D13), only the step from IR to MIR is shared.
- **D14 (C), 2026-11-13.** Consumers register their own imports per
  context; `cfunction()` keeps D6's fixed list.
- **D15 (M), 2026-11-13.** `rfunction()` ships in v0.1.0, named without
  the `mir_` prefix for the reason D8 gives `cfunction()`. Its v0.1.0
  subset is §4.6: scalar doubles, indexed vectors in loops, conditions,
  the math table, and R's `NA` rules exactly. Element-wise vector
  arithmetic with recycling, integer and logical results wait (§13).
- **D16 (C), 2026-11-13.** Where the subset cannot match R cheaply, it
  errors rather than answering differently: out-of-range and non-integer
  indices, sizes and `seq_len()` or `a:b` bounds that are not whole
  numbers, arguments that are not `double`, results R would type as
  integer or logical, and integer overflow. An `rfunction()` result
  is either R's answer or an error.
- **D17 (M), 2026-11-13.** `rfunction()` reaches MIR by two routes: MIR
  text first, then a `.Call` bridge to the §4.3 emitter, each its own
  stage. The text route is the bridge's oracle (the same module printed,
  the same results) and its compile-time benchmark.
- **D18 (C), 2026-11-13.** `rfunction()`'s imports are a fixed internal
  list (§4.6): the C library's math, `R_pow`, and zumir's C ports of R's
  `%%`, `%/%`, `min` and `max`. The import resolver, with D6's list,
  consumer imports (D14) and this one, is built with the C API, before
  `rfunction()` and `c2mir` need it.
- **D19 (C), 2026-11-13.** `c2mir` stays the front end of `cfunction()`.
  If, in its stage, it fails the `c-tests` subset that `cfunction()`'s
  numeric functions need, or the fuzz target finds crashes upstream does
  not fix, TinyCC (§3) is weighed as the front end of `cfunction()` alone,
  with its LGPL as the cost. The provider and `rfunction()` stay on MIR.

## 13. Later versions, in the order they are expected

- **`rfunction()` beyond the subset**: element-wise vector arithmetic
  with recycling, integer and logical results, several outputs as a
  list, and R's own `Rmath` functions (`gammafn`, `dnorm`, …) where their
  warnings can be kept from unwinding through generated code.
- **Caching generated code** with MIR's binary format, for consumers
  whose modules are large enough that generation shows.
- **Signatures with R vectors of other types** in `cfunction()`
  (logical, raw, complex), and a matrix shorthand.
- **MIR's parallel generator** (an upstream branch), if a consumer
  generates enough code for it to matter.
- **A Python package** over the same vendored C and patches. PyPI has no
  MIR binding; `mirjit` and `c2mir` were free names on 2026-11-12.
- **`mir2c`**, to show a consumer's lowered code as C, for review.

## 14. Open questions

- **Q1.** Answered 2026-11-13: Rtools needs no build fix, only two
  one-line warning fixes and the #496 fix, all small enough for upstream
  ([spike results](https://github.com/pedrobtz/mirspike/blob/main/RESULTS.md)).
- **Q2.** Should `cfunction()` accept several functions in one source and
  return them as a list, or one function per call? This RFC takes one,
  named by the source's only non-`static` function.
- **Q3.** Is a second maintainer for the vendored MIR plausible, given
  that upstream has one principal author? This RFC records the risk and
  pins the commit; it does not answer it.
- **Q4.** Is v0.1.0 a CRAN submission or a GitHub release? The board's
  `CRAN intent` for this card decides; this RFC assumes `Planned`,
  because `zupmxode` cannot reach CRAN otherwise.
- **Q5.** Where W+X memory is refused, what runs? (a) A dual-mapped code
  allocator, one file mapped read-execute and read-write as TinyCC's
  `CONFIG_SELINUX` does, patched into MIR so it writes through the second
  mapping: both tiers then work. (b) `MIR_interp_arr` without a shim, with
  `zupmxode` adapting its call. (c) No tier: `zumir` reports it, and
  `zupmxode` brings its own IR interpreter for that case only. The spike
  showed the problem, not the fix; (a) is this RFC's proposal. The
  maintainer decides it at the start of the stage that builds tier
  detection (2026-11-13); option (a) touches every code write in
  `mir.c`, since MIR's code allocator has no address to write through.
- **Q6.** Should `rfunction()` take argument shapes from use, as §4.6
  proposes, or from an explicit `types =` argument? Inference keeps the
  call as plain R; an explicit argument makes a scalar-or-vector
  ambiguity impossible. This RFC takes inference, with `types =` as an
  override if Stage 6 finds a case inference gets wrong.
