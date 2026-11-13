# RFC 0073: `zumir`, native code from R without a compiler, with MIR

Adopted from [RFC 0073](https://github.com/pedrobtz/packages/blob/72859143843cf3d7407bcbcaec2fd41d677978ae/rfcs/not-started/0073-zumir-native-code-without-a-compiler.md)
on 2026-11-13. This file is the single source of truth for the design; the
RFC is now a pointer here.

- **Status:** Proposed. Merging this file adopts the design as the starting
  point for the package's own `.agents/design.md`; each decision in section
  12 stays the maintainer's until the package repository records it.
  The MIR spike ([results](https://github.com/pedrobtz/mirspike/blob/main/RESULTS.md), 2026-11-13) decided that the package is
  built; RFC 0047 `zupmxode` rests on that answer.
- **Date:** 2026-11-12.
- **Packages:** `zumir` (not yet created; crandb returns `not_found` and
  there is no `pedrobtz/zumir` repository, both checked 2026-11-12).
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
  2026-11-12: no MIR binding (`pymir` is an unrelated project). Nothing
  has been built.

## 1. What zumir is

`zumir` brings MIR, Vladimir Makarov's lightweight JIT compiler, to R in
two ways:

1. **A provider** (`LinkingTo: zumir`) for family packages that generate
   code: they build MIR functions through a versioned C API table and get
   back native function pointers, or an interpreter where native code
   cannot be made. `zupmxode` (RFC 0047) is the first consumer.
2. **C without a toolchain** for R users: `cfunction()` compiles a short
   C function with MIR's own C compiler and returns an R function that
   calls it. No Rtools, no Xcode command-line tools, no `gcc`.

MIR's README measures its generator at about 250 µs for a small C
program, about a hundredth of `gcc -O2`'s time, with generated code
within about ten per cent of `gcc -O2`'s speed, and its interpreter at
six to fourteen times slower than generated code. It targets x86-64 and
aarch64 on Linux and macOS, and ppc64le, s390x and riscv64 on Linux.

The one-line statement that governs every decision below:

> **Native code from R without a compiler, and MIR never takes R
> down.**

What it is not:

- **Not a full C compiler.** `c2mir` implements C11 with gaps (open
  issues #394 and #286), and `cfunction()` takes plain numeric functions,
  not programs (§5, D5).
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
- **Two tiers**, native and interpreter, chosen automatically with
  override, reported by `zumir_info()`.

Out of v0.1.0 (§13): caching generated code between sessions, MIR's
parallel generator branch, a Python package, `mir2c`, C code that
includes R's headers or calls R's API.

## 3. Position in the `zu*` family

| | zumir |
| --- | --- |
| Role | provider and tool |
| R prefix | `mir_`, with `cfunction()` |
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
runs there is open (Q5).

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

zumir_info()                 # MIR commit, targets, tier, patches, memory
zumir_tier()                 # "native" or "interp" for this session
cfunction(src, tier = "interp")
```

Argument types: `int`, `double`, `const int *`, `const double *`,
`int *` and `double *`, with lengths named by an `int` argument or fixed.
Return types: `void`, `int`, `double`. Inputs are not copied unless the
C type is non-`const`; outputs named in `out` are allocated by the
wrapper and returned, as a list when there are several.

## 6. Errors

- Compile errors from `c2mir` carry the line and column of the user's
  source, not of the prelude.
- MIR errors, in either API, become a `zumir_error` condition from R and
  a status code from C. The process never exits on MIR's account (D4).
- Calls through `cfunction()` validate types and lengths in R before the
  call. Inside the call the code is the user's; a crash there is a crash,
  as it is in `.Call()`.
- Native code unavailable is not an error: the interpreter runs and
  `zumir_info()` says why.

## 7. Limits and portability

- **Tested targets**: x86-64 on Linux, macOS and Windows; aarch64 on
  macOS and Linux. These are the r-actions legs.
- **Built, not tested here**: ppc64le, s390x and riscv64 on Linux, which
  MIR targets and its own CI tests. Debian builds R on them.
- **Elsewhere**: the package installs and reports that MIR has no
  target, and `cfunction()` and the provider API return an error
  condition and a status code.
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

- A subset of MIR's own `c-tests` and `mir-tests`, run through the
  vendored copy on every leg, on both tiers.
- Every `cfunction()` example against a reference written in R.
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

- **D1 (M), 2026-11-12.** One package does both jobs: it vendors MIR for
  the family and gives R users `cfunction()`. The provider alone would
  have no use outside the family; `cfunction()` is what makes the
  package worth publishing by itself, and it shares every line of the
  vendoring and the error trap.
- **D2 (C).** MIR is vendored from a pinned `master` commit, not from the
  v1.0.0 tag. The tag is from May 2024; `master` carries two and a half
  years of fixes. `tools/vendor.sh` records the commit, and an update is
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
- **D9 (C).** `c2mir` ships in v0.1.0, not later. Without it there is no
  `cfunction()` and so no reason to publish; with it, `zupmxode` still
  never compiles C text, because it emits MIR directly.

## 13. Later versions, in the order they are expected

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
  showed the problem, not the fix; (a) is this RFC's proposal.
