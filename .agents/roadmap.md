# zumir: roadmap

Status: draft 2026-11-13. Implements [design.md](design.md).
Target: v0.1.0, the first CRAN release.
Tracking: [v0.1.0 (#1)](https://github.com/pedrobtz/zumir/issues/1), with one
sub-issue per stage; [CRAN-submission (#15)](https://github.com/pedrobtz/zumir/issues/15).

Moved here from the design's §10 on 2026-11-13, with the portfolio's
identity, assurance and closing stages around it. The spike's findings
split the design's error trap from its contexts and tiers; `rfunction()`
(D15, D17) adds Stages 6 and 7; the import resolver moved into Stage 5
(D18). Revised 2026-11-13 after an independent review of both files and
the assurance inventory (`.agents/assurance.md`, issues #18 to #24).
Revised again 2026-11-13 after two outside reviews: the table's error
shims and ABI (D20, D21), `rfunction()`'s value rules and interrupts
(D22, D23), `cfunction()`'s pointers (D24), Windows aarch64 (D25), and Q5
narrowed to (a) or (c). Stage 9 is now `full`, `zupmxode` is tried
against the table at Stage 5, and the random differential is split
between pull requests and the full profile.

## Stages

| Stage | Title | CI | Issue | Pull request | Done |
| --- | --- | --- | --- | --- | --- |
| 0 | [MIR spike](#stage-0--mir-spike) | n/a | [#2](https://github.com/pedrobtz/zumir/issues/2) | [mirspike](https://github.com/pedrobtz/mirspike) | 2026-11-13 |
| 1 | [Package identity](#stage-1--package-identity) | quick | [#3](https://github.com/pedrobtz/zumir/issues/3) | | |
| 2 | [Vendored MIR, clean on every leg](#stage-2--vendored-mir-clean-on-every-leg) | full | [#4](https://github.com/pedrobtz/zumir/issues/4) | | |
| 3 | [Error trap](#stage-3--error-trap) | full | [#5](https://github.com/pedrobtz/zumir/issues/5) | | |
| 4 | [Contexts, tiers and zumir_info()](#stage-4--contexts-tiers-and-zumir_info) | full | [#6](https://github.com/pedrobtz/zumir/issues/6) | | |
| 5 | [C API and mir_function()](#stage-5--c-api-and-mir_function) | full | [#7](https://github.com/pedrobtz/zumir/issues/7) | | |
| 6 | [rfunction()](#stage-6--rfunction) | quick | [#17](https://github.com/pedrobtz/zumir/issues/17) | | |
| 7 | [rfunction() through the emitter](#stage-7--rfunction-through-the-emitter) | full | [#26](https://github.com/pedrobtz/zumir/issues/26) | | |
| 8 | [c2mir and the prelude](#stage-8--c2mir-and-the-prelude) | full | [#8](https://github.com/pedrobtz/zumir/issues/8) | | |
| 9 | [cfunction()](#stage-9--cfunction) | full | [#9](https://github.com/pedrobtz/zumir/issues/9) | | |
| 10 | [Benchmarks and README](#stage-10--benchmarks-and-readme) | quick | [#10](https://github.com/pedrobtz/zumir/issues/10) | | |
| 11 | [Assurance](#stage-11--assurance) | quick | [#11](https://github.com/pedrobtz/zumir/issues/11) | | |
| 12 | [Full CI](#stage-12--full-ci) | full | [#12](https://github.com/pedrobtz/zumir/issues/12) | | |
| 13 | [v0.1.0 readiness](#stage-13--v010-readiness) | full | [#13](https://github.com/pedrobtz/zumir/issues/13) | | |
| 14 | [Review](#stage-14--review) | full | [#14](https://github.com/pedrobtz/zumir/issues/14) | | |

Oracles, chosen here for every boundary:

- **MIR itself**: MIR's own c2mir-free tests (`adt`, `simplify`, `io`,
  `scan`, `interp`, `gen`, `readme-example`) at the pinned commit, and later
  the `c-tests` subset, run on the vendored, patched copy.
- **Generated code**: the same function in C compiled by the leg's
  compiler, bitwise on x86-64 and to one ulp on aarch64 (design §7); and
  native against interpreter, bitwise everywhere.
- **`c2mir`**: the leg's system C compiler on the same source (the `c-tests`
  expected outputs, and a differential on the fuzz corpus).
- **`cfunction()`**: a reference written in R for every example.
- **`rfunction()`**: R itself, the same function evaluated by R on the
  same inputs, compared with `identical(num.eq = FALSE)`.
- **Generated code's memory accesses**: valgrind with `--smc-check=all`
  on the native tier (ASan does not see JIT code), and ASan on the
  interpreter tier, where the program's loads and stores are C.
- **Threads**: the serial result, bitwise.
- **Error recovery**: the session alive and its RSS flat across 10³
  failures; the condition class.
- **CRAN's compiled-code rules**: `nm -u` on the built objects, and
  `tools/verify-vendor` (upstream plus exactly the patch queue).

## Stage 0 — MIR spike

**Issue.** [#2](https://github.com/pedrobtz/zumir/issues/2), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1). Closed when it is created: the work
is done.

**CI.** n/a: built and run outside this repository, in the throwaway
package [`pedrobtz/mirspike`](https://github.com/pedrobtz/mirspike).

**Goal.** Decide whether `zumir` is built, by testing MIR alone on the
r-actions legs.

**Work.** Seven questions: (1) build under Rtools gcc-mingw (#163, #496),
macOS arm64 and x86-64, and the r-devel gcc and clang containers; (2) emit
three right-hand sides through the MIR API (one compartment; two
compartments with Michaelis–Menten; ten states) and time generation; (3) ns
per call against hand-written C and rxode2; (4) compile once, call from 8
OpenMP threads; (5) `MAP_JIT` on macOS arm64 and Linux with W+X refused;
(6) the interpreter against the README's 13.7×; (7) MIR's own tests, and
the licence of every vendored file.

| Outcome | Decision |
| --- | --- |
| all seven pass | build `zumir`; `zupmxode`'s D2 stands |
| only Windows fails | build `zumir`, patch Windows in Stage 2, interpreter on Windows until then |
| the interpreter is too slow to be a fallback | build `zumir`; `zupmxode` adds its own IR interpreter for platforms without native code |
| native code is not fast enough, or MIR will not build | do not build `zumir`; `zupmxode` returns to a hand-written interpreter (RFC 0047 §11.1) |

**Result.** First row, recorded in the spike's
[RESULTS.md](https://github.com/pedrobtz/mirspike/blob/main/RESULTS.md):
MIR builds on every leg, generates a model in under 1 ms, runs at 0.7 to
1.3× the C compiler's speed, is thread-safe once compiled, and passes its
own tests. Its findings are in the design: the patches (§4.2), the
interpreter's per-thread context (§4.3, D10), code-memory failures (§4.4,
D11) and Q5.

**Design.** [§4](design.md#4-architecture), [§10](design.md#10-roadmap).

## Stage 1 — Package identity

**Issue.** [#3](https://github.com/pedrobtz/zumir/issues/3), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `quick`.

**Goal.** The repository is a real, checked package with the portfolio's
CI and assurance wiring, and no MIR yet.

**Work.**

- `DESCRIPTION`: Title, Description, `Authors@R` (`aut`, `cre`, `cph`, and
  Vladimir Makarov as `cph` for MIR), `Language: en-GB`, MIT licence,
  `Version: 0.0.0.9000`, `SystemRequirements` for the C standard MIR
  needs.
- `.Rbuildignore` as `.claude/shared/r-package.md` lists (pkg-init has
  added the entries).
- `R-CMD-check.yaml` calling r-actions `r-cmd-check.yml@v1` with
  `profile: auto` and the Windows R-devel runner row; a free weekly
  `schedule:` slot.
- `native-checks.yaml` calling sanitizers, valgrind, rchk, gctorture,
  analyzers, lto and cran-special, as zuexact does.
- `platforms.yaml` calling `platforms.yml@v1` with `tools/ci/platform.sh`,
  which runs nothing yet; `containers` stays r-cmd-check's job.
- `.agents/assurance.md` from `templates/assurance.md`, with the kinds
  `compiled`, `untrusted-input` (C and MIR source from users), `spec` (MIR
  text and C11 through `c2mir`) and `lifecycle` (contexts and compiled
  functions held by external pointers), and the `assurance.yml` caller
  with `ungated-jobs: fail`.
- The test runner: `tools/ci-run-tests.R`, shuffled with its seed printed,
  with coverage.
- The labels `full-ci`, `ci:minimal`, `ci:docs` and `stage`.
- `CLAUDE.md` (written by pkg-init) brought up to date.

**Tests.**

- The scaffold's placeholder test replaced by one on `.onLoad`: the
  package loads and unloads cleanly, in `tests/testthat/test-load.R`.

**Exit criteria.**

- R CMD check passes on the quick profile, with 0 warnings and 0 notes
  apart from "New submission".
- A quick-profile pull request run finishes in under five minutes.
- `assurance.yml` passes on the file, with every check row `done` or
  `gap #<issue>`.

**Design.** [§3](design.md#3-position-in-the-zu-family).

## Stage 2 — Vendored MIR, clean on every leg

**Issue.** [#4](https://github.com/pedrobtz/zumir/issues/4), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full — a vendored library added: sanitizers, valgrind, rchk, and
every platform leg, with R CMD check's compiler diagnostics on CRAN's
flags`.

**Goal.** MIR's core, generator and interpreter build into the package on
every r-actions leg with no warning R CMD check counts, and MIR's own tests
pass on each.

**Work.**

- `tools/vendor.sh` (from mirspike): pinned commit, file list, checksums,
  patch queue re-applied; `tools/verify-vendor`; `src/vendor/mir/` with
  `LICENSE`; `src/vendor/PROVENANCE`; `inst/COPYRIGHTS`.
- The patch queue in `tools/patches/`, each offered upstream (D12): the
  Windows warnings and `VirtualFree` (#496), the stray semicolons,
  `MIR_DIRECT_DISPATCH`, `-fsigned-char` removed, hidden visibility for
  every `MIR_`/`_MIR_` symbol. Upstream pull request #490 ("Make mir.h
  self-contained in strict ISO C", merged 2026-10-23) may already cover
  part of the `-pedantic` work: check whether the pinned commit has it
  before writing that patch.
- The build on a CPU MIR does not target: MIR left out (§4.2 "Other
  CPUs"), so the package installs and reports no target. The same guard
  on Windows aarch64 (D25).
- `tools/vendor.sh` diffs the parts of `mir.h` a consumer compiles in
  (enums, `MIR_op_t` and its structs) against the pinned commit, so an
  update that needs a MIR ABI bump (D21) cannot pass unnoticed.
- `tools/ci/platform.sh` is the in-repository home of the checks D2
  re-runs on every MIR update; Stages 3 to 5 add theirs to it.
- `tools/ci/platform.sh` runs MIR's own tests on the vendored, patched
  copy, built standalone without R, on every runner of `platforms.yml`
  (macOS arm64 and x86-64, Windows, Linux x86-64 and aarch64): `adt-test
  simplify-test io-test scan-test interp-test readme-example-test` and
  the `gen-test` targets except `inline-test`, which builds `c2mir`
  (Stage 8). No job of its own: the r-devel containers are covered by
  R CMD check's `containers` job running the package's tests.
- An `nm -u` guard in CI that fails on `exit`, `abort`, `printf`, `puts`,
  `stderr`, `stdout`, `sprintf`, `rand`. It is expected to fail until
  Stage 3; it is wired here with an allow-list that Stage 3 empties.

**Tests.**

- MIR's own test targets: their expected outputs, in
  `tools/ci/platform.sh` on every leg.
- Vendor integrity: `tools/verify-vendor` (upstream at the pinned commit
  plus exactly the queue), in CI.
- Symbol hiding: `nm` on the shared object shows no `MIR_` export, in
  `tools/ci/platform.sh`.
- No target: the package built with MIR forced out (an environment
  variable read by the build) installs and loads, and an internal
  `.Call(zumir_has_target)` returns `FALSE`, in
  `tests/testthat/test-no-target.R`, on one Linux leg. Stage 4 extends
  the file to `zumir_info()`, and Stages 5, 6 and 9 to their entries,
  each with a classed error.

**Exit criteria.**

- R CMD check on the full profile has no compiler-diagnostic WARNING on
  any leg, including Windows (Rtools) and gcc 16 with `-pedantic`; the
  only allowed warning is "checking compiled code", which Stage 3 removes.
- MIR's own tests pass on every runner.
- `verify-vendor` passes; every patch has an upstream issue or pull request
  linked in `PROVENANCE`.

**Design.** [§4.1](design.md#41-what-is-vendored),
[§4.2](design.md#42-patches), D2, D7, D12.

## Stage 3 — Error trap

**Issue.** [#5](https://github.com/pedrobtz/zumir/issues/5), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full — longjmp out of vendored code, and an R allocation in
the error path: valgrind, ASan, and alloc-failure wired here`.

**Goal.** MIR never ends, prints from or crashes the R process on its own
account.

**Work.**

- Patches: every `exit`, `abort`, `fprintf(stderr, …)` and `sprintf` in the
  vendored files routed through the context's error function or a
  `MIR_FATAL` hook; `MIR_init2`'s version check and `_MIR_init`'s first
  allocation return NULL; `MIR_finish` clears `curr_func`/`curr_module`
  (mirspike patch 0001).
- `zumir`'s handler: formats into a buffer, `longjmp`s to the entry point
  that called MIR, thread-local.
- The `nm -u` guard's allow-list emptied.
- `alloc-failure.yml` wired.
- An internal test entry point, `.Call(zumir_test_invalid, kind, n)`,
  that builds and fails a module of each kind through the trap; no R API
  for modules exists until Stages 4 and 5.

**Tests.**

- Three invalid modules (a syntax error, an operand type error, an
  undefined import), each 10³ times through the internal entry point: a
  `zumir_error` condition each time, the session alive, RSS flat, in
  `tests/testthat/test-error-trap.R` (run in a child R process through
  `system2()`, so a crash fails the test rather than the run).
- An error inside `MIR_finish_func` followed by finishing the context:
  no crash, in the same file.
- The trap on two threads at once: each thread's error reported to that
  thread, in a C test under `tests/c/`.
- No CRAN-flagged symbol: `nm -u`, in CI.

**Exit criteria.**

- "checking compiled code" is clean on every leg; no allowed warnings
  remain.
- The error-recovery loop passes under valgrind and ASan.
- `alloc-failure` runs and its findings are fixed or filed.

**Design.** [§4.2](design.md#42-patches), [§6](design.md#6-errors), D4.

## Stage 4 — Contexts, tiers and zumir_info()

**Issue.** [#6](https://github.com/pedrobtz/zumir/issues/6), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full — first external pointers and finalizers, and a Linux path
that refuses executable memory: valgrind, ASan, gctorture at step 20`.

**Goal.** Contexts are owned by R objects, executable memory is
detected without crashing, and the session's tier is known and reported.

**Work.**

- First, Q5 decided by the maintainer and written into the design:
  (a), the dual-mapped allocator as a fallback, or (c), no tier. Option
  (b) is struck (design §4.4). If it chooses (a), split this stage in the
  roadmap before starting: first a mirspike-style spike of the allocator
  alone under SELinux `deny_execmem` and the seccomp filter, then the
  patch, which touches every code write in `mir.c`, as a pull request of
  its own.
- Contexts created with `MIR_init2` and counting allocators; finalizers
  free contexts and their code.
- The code-memory patch (D11): every `mem_map`/`mem_protect` result
  checked.
- Q5's answer implemented: a dual-mapped code allocator if (a) is
  chosen.
- The tier probe (§4.4), in `.onLoad` before anything else maps code:
  arithmetic and an `exp` import, natively and on the interpreter, with
  the four capabilities reported separately; able to tell a missing
  `MAP_JIT` entitlement on macOS. `zumir_info()` (MIR commit, targets,
  tier and capabilities with why, patches, memory); `zumir_tier()`.

**Tests.**

- Tier detection with W+X refused: a seccomp filter in a forked child
  (mirspike's harness) gives the documented tier, capabilities and
  reason, not a signal, in `tests/testthat/test-tier.R` (Linux). Under
  (c), the interpreter is reported unavailable too, since its thunks and
  external-call bridges need the same memory.
- Native against interpreter: the three spike right-hand sides, bitwise,
  in `tests/testthat/test-tiers.R`.
- Contexts freed: memory reported by `zumir_info()` returns to its
  baseline after `gc()`, under gctorture too, in
  `tests/testthat/test-finalizer.R`.
- Threads: native code compiled once on 8 threads, and the interpreter with
  a context per thread (D10), both against the serial result bitwise, in
  `tests/testthat/test-threads.R`.
- Contexts out of order: free twice, use after the finalizer, `readRDS`
  of a saved one, use in a forked child, an R error between two MIR calls,
  a MIR error then more calls on the failed context; each ends in its
  documented state (design §4.3 "Lifetimes", D20;
  [#22](https://github.com/pedrobtz/zumir/issues/22)), in
  `tests/testthat/test-context-sequences.R`.

**Exit criteria.**

- Every Tests line holds on the full profile, including the Linux W+X
  test on ubuntu-latest and ubuntu-24.04-arm.
- A dispatched step-20 gctorture run is green on the pull request's last
  commit.

**Design.** [§4.3](design.md#43-the-c-api) (memory, threads),
[§4.4](design.md#44-tier-detection), [§7](design.md#7-limits-and-portability),
D10, D11, D20, Q5.

## Stage 5 — C API and mir_function()

**Issue.** [#7](https://github.com/pedrobtz/zumir/issues/7), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full — the first R_RegisterCCallable table and a consumer
package built against it on every platform leg`.

**Goal.** A consumer package can link `zumir`, build MIR through the API
table and call the result; R users can run MIR text.

**Work.**

- `zumir_api_get` registered with `R_RegisterCCallable`, returning
  `zumir_api_v1` with its version, size, MIR ABI number and capability
  flags first (D21); `inst/include/zumir.h`, standalone C99, with
  `zumir_api_load()` and inline wrappers.
- The `MIR_` entries as shims (D20): trap, call, record the error in the
  context, return; a failed context no-ops until freed; `zumir_status()`.
- `mir_function()`.
- The double-expression emitter in `zumir.h` (D13), moved from the
  spike's emitter; `zumir_import()` for consumer imports (D14).
- The import resolver, moved here from c2mir's stage because
  `rfunction()` needs it first (D18): D6's list for `cfunction()`,
  consumer imports, and `rfunction()`'s internal list with `R_pow`, the
  C ports of R's `%%`, `%/%`, `min` and `max`, and the interrupt poll
  through `R_ToplevelExec()` (D23).
- A consumer test package under `tests/consumer/`, built in CI against the
  installed `zumir`, that emits the three spike right-hand sides, with a
  C++ source file beside its C.
- `zupmxode` tried against the table before Stage 6 starts: its lowering
  prototype, or its Stage 2 if that exists, builds against this stage's
  header and runs one model end to end on both tiers, with compile time
  against call count (the break-even) recorded in
  `.agents/benchmarks.md`. A table change it needs lands in this stage,
  while no other consumer exists.

**Tests.**

- The consumer's three right-hand sides against the same functions in C
  built with `-ffp-contract=off`, bitwise on every leg ([#18](https://github.com/pedrobtz/zumir/issues/18)), in
  the consumer's tests.
- `mir_function()` on the design's `add` example, and on MIR's own
  `mir-tests/*.mir` by `run-test`'s round trip plus hand-written expected
  results ([#19](https://github.com/pedrobtz/zumir/issues/19)), in `tests/testthat/test-mir-function.R`.
- The resolver: each D6 math function returns what R's own function
  returns on the same inputs, bitwise; a name outside a context's list is
  a link error with class `zumir_error`; the C ports of `%%`, `%/%`,
  `min` and `max` against R on negative, zero, non-finite and `NA`
  operands, in `tests/testthat/test-imports.R`.
- The header compiles alone as C99 with `-pedantic`, in CI.
- Errors through the table (D20): an invalid instruction mid-module,
  then more `MIR_` calls (no effect, no crash), then `zumir_compile()`
  (`NULL` and the recorded status), then `zumir_ctx_free()`; in C, and in
  C++ with an object whose destructor counts, which runs exactly once;
  10³ times with RSS flat, in the consumer's tests.
- Compatibility (D21): the consumer built against a frozen copy of the
  v1 header runs against the current provider; a header with a
  different MIR ABI number gets `NULL` and a reason from
  `zumir_api_load()`; a request for a newer table version than the
  provider has gets `NULL` and a reason, in the consumer's tests.
- The emitter: the consumer's three right-hand sides are written with it,
  so the bitwise comparison above covers it; each comparison and branch
  helper against C on `NaN` and `±Inf` operands, in the consumer's tests.
- A consumer import (`expit`) resolves in the consumer's context and not
  in another context, nor under D6's list (an internal entry point
  links against that list alone), in the consumer's tests. The same
  check through `cfunction()` itself is Stage 9's.

**Exit criteria.**

- The consumer package builds and passes on the full profile.
- `zupmxode` ran one model against the table, on both tiers.
- Every Tests line exists and was seen to fail.

**Design.** [§4.3](design.md#43-the-c-api), [§5](design.md#5-public-r-api-complete-v010-surface)
(`mir_function()`), D3, D6, D13, D14, D18, D20, D21.

## Stage 6 — rfunction()

**Issue.** [#17](https://github.com/pedrobtz/zumir/issues/17), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `quick`: R code that emits MIR text, compiled by Stage 5's
`mir_function()` path and resolver; no new native code.

**Goal.** A numeric R function in §4.6's subset compiles to native code
through MIR text, and returns exactly what R returns.

**Work.**

- The R walker: the subset check, argument shapes (Q6, including
  `seq_along()`), logical and integer locals, `numeric()` sizes; the D22
  rules (names resolved to base from `environment(f)`, read-only
  arguments, no defaults, top-level vector locals never assigned whole,
  definite assignment); compile errors naming the construct and its
  position.
- Lowering to MIR text, with the status word the generated code returns
  for run-time errors, warnings and interrupts (§4.6), and the poll on
  every loop's back edge (D23).
- The wrapper: `double` arguments without attributes checked, sizes
  evaluated in R, outputs allocated, one call, errors, warnings and
  interrupts raised from the status; `tier =`.
- A generator of random functions within the subset, for the
  differential test, with a time limit per function; a failing function
  is saved with its seed as a reproducer.

**Tests.** Every comparison with R uses the [#21](https://github.com/pedrobtz/zumir/issues/21) helper:
`identical(num.eq = FALSE)`, which already tells `0` from `-0` and `NA`
from `NaN`, with only the NA-or-NaN case §4.6 allows; value, attributes,
warnings and error class asserted separately.

- Each construct against R on chosen inputs (`NA`, `NaN`, `±Inf`, `-0`,
  subnormals, empty vectors): the operators; `^`; `%%` and `%/%` on
  negative, zero and non-finite operands; two-argument `min`/`max` with
  `NA` and `NaN`; `&&`/`||` with `NA` (`NA && FALSE` is `FALSE`); every
  loop form, `1:0` counting down, `break`/`next`; logical locals in
  arithmetic; in `tests/testthat/test-rfunction-ops.R`.
- Random functions against R, on random inputs, seed printed on
  failure: about 10² functions per leg on the quick profile and on CRAN,
  10⁴ per leg on the full profile (weekly, dispatched, `full-ci`), read
  from `R_ACTIONS_PROFILE`, in `tests/testthat/test-rfunction-random.R`.
- Run-time errors: `if (NA)` and `while (NA)` with R's message, `if
  (NaN)` with R's other message, both of class `zumir_error`, in
  `tests/testthat/test-rfunction-errors.R`.
- Warnings: `log(-1)`, `sqrt(-1)` and `%%`'s loss of accuracy give R's
  warning once per call, in the same file.
- D16's errors where R would answer differently, each straddled: an
  index at `0`, `1`, `length`, `length + 1`, `NA` and `1.5`; a `numeric()`
  size and a `seq_len()` argument at `-1`, `0`, `NA` and `2.5`; `a:b` with
  an `NA` or fractional bound; integer counter overflow at
  `46340L * 46340L` and `46341L * 46341L`; a non-`double` argument; an
  argument with names, with `dim`, and of class `Date` and `difftime`; a
  scalar argument of length 0 and 3; in the same file.
- Interrupts (D23): `while (TRUE) {}` and a `for` loop of 10⁹
  iterations, under `setTimeLimit(elapsed = 1)` in a child R process,
  end with R's time-limit error within seconds, and the session then
  runs the function again normally; a loop of `2^16 - 1`, `2^16` and
  `2^16 + 1` iterations gives R's answer, in the same file.
- A vector argument longer than `.Machine$integer.max`, refused before
  any data pointer is taken, with an ALTREP sequence ([#24](https://github.com/pedrobtz/zumir/issues/24)), in
  the same file.
- Refusals at compile time: a call to another R function, `<<-`, a list,
  a data frame, `repeat`, string values, a result R would type as
  integer or logical, each with its name, in
  `tests/testthat/test-rfunction-refuse.R`. Also each D22 rule, with the
  case that motivated it: `exp` or `+` rebound in `environment(f)`; a
  local named `exp` beside a call to `exp()`; a formal with a default;
  `...`; `n <- n + 1` on an argument; `numeric()` inside a loop and
  inside a branch; `z <- y` and a second `numeric()` for one name; a
  local assigned on one branch only and then read; the loop variable
  read after its loop. Each refused function is also run by R, to show
  that the refusal guards a real difference or an R error, in the same
  file.
- Native against interpreter: identical on the random functions, in
  `tests/testthat/test-rfunction-random.R`.
- Under a hostile session (`OutDec = ","`, `digits = 3`, `LC_ALL=C`):
  results unchanged, in `tests/testthat/test-rfunction-ops.R`.

**Exit criteria.**

- Every Tests line exists, compares with R through the #21 helper, and
  was seen to fail.
- The random differential runs 10² functions per leg on the quick
  profile, and a dispatched full-profile run 10⁴ per leg, without a
  difference; the quick run stays inside its five minutes.

**Design.** [§4.6](design.md#46-rfunction), [§5](design.md#5-public-r-api-complete-v010-surface),
[§6](design.md#6-errors), D15, D16, D17, D18, D22, D23, Q6.

## Stage 7 — rfunction() through the emitter

**Issue.** [#26](https://github.com/pedrobtz/zumir/issues/26), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full — a new .Call bridge that builds MIR from R values,
holding R objects and native state across MIR calls: valgrind (with
`--smc-check=all`, so generated code is checked too), ASan with the
interpreter tier, rchk`.

**Goal.** `rfunction()` builds MIR through the §4.3 emitter, without
text, and the text route proves it right (D17).

**Work.**

- The `.Call` bridge: the walker's lowering drives the emitter's helpers
  instead of writing text; the text route kept, selectable, as the
  oracle.
- Compile-time benchmark of both routes into `.agents/benchmarks.md`.

**Tests.**

- Same module: for every random function of Stage 6's generator and
  every construct test, the module the bridge builds, printed with
  `MIR_output`, equals the text route's module, in
  `tests/testthat/test-rfunction-routes.R`.
- Same results: both routes against R through the #21 helper, in the
  same file.
- The bridge under an R error raised mid-lowering (an interrupt, an
  allocation failure under `alloc-failure`): no leak, the context freed,
  in the same file.

**Exit criteria.**

- Every Tests line holds on the full profile; the bridge is the default
  route; the benchmark row is recorded.

**Design.** [§4.6](design.md#46-rfunction), D13, D17.

## Stage 8 — c2mir and the prelude

**Issue.** [#8](https://github.com/pedrobtz/zumir/issues/8), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full — a vendored library added (c2mir) and a parser of untrusted
input: ASan, valgrind and a fuzz harness`.

**Goal.** C source compiles in memory to MIR on every leg, reading no
system header, with only the allowed library names resolving.

**Work.**

- `c2mir/` vendored with its per-target headers; its patches queued: the
  no-exit treatment, its seven `abort ()` calls, and a message callback
  in place of `message_file` (§4.2).
- The prelude and the built-in headers (§4.5, D5), over Stage 5's
  resolver with D6's list.
- The `c-tests` runner by the corpus's own rules, with a test-only
  import list for `printf`, `abort` and `exit` ([#20](https://github.com/pedrobtz/zumir/issues/20)).
- Diagnostics mapped back to the user's line and column.
- A standalone fuzz target over `cfunction()`'s compile path, built
  without R, and `fuzz.yml` wired with a corpus.

**Tests.**

- The prelude's declarations match the resolver's list: every declared
  name links, and a C call to it returns what R's own function returns,
  in `tests/testthat/test-prelude.R`.
- A name outside D6's list is a link error with class `zumir_error`, and
  a test-only import is not visible on `cfunction()`'s compile path, in
  the same file.
- Diagnostics: a syntax error on line 3 of the user's source is reported
  at line 3, in `tests/testthat/test-c2mir-errors.R`.
- The `c-tests` corpus through the vendored `c2mir` on both tiers: the
  272 `.expect` outputs compared, the rest by exit status; the passing
  subset and the reason for each exclusion recorded, in
  `tools/ci/platform.sh` ([#20](https://github.com/pedrobtz/zumir/issues/20)).
- Fuzzing: no crash, and each crash input filed upstream with a
  reproducer. This proves robustness only; correctness is the line
  above.

**Exit criteria.**

- Every Tests line holds on the full profile.
- The fuzz target runs in CI and its findings are fixed or filed.
- D19 settled: either `c2mir` passes the `c-tests` subset that
  `cfunction()`'s numeric functions need and has no open crash, or the
  TinyCC alternative is written up in the design for the maintainer.

**Design.** [§4.5](design.md#45-cfunction) steps 1 to 4, D5, D6, D9, D19.

## Stage 9 — cfunction()

**Issue.** [#9](https://github.com/pedrobtz/zumir/issues/9), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full — a new .Call wrapper that marshals R vectors into
pointers for user code, with copies for outputs: valgrind (with
`--smc-check=all`), ASan with the interpreter tier, rchk`.

**Goal.** §5's surface is complete, and documented for its users.

**Work.**

- `cfunction()` with its signature parser, argument checks, output
  allocation and copies (D24), and the one `.Call` wrapper; `tier =`.
- The Get started article's chunks switched to `eval = TRUE`, its
  `rfunction()` section included.
- A "What `rfunction()` accepts" article: the subset, every D16 and D22
  difference from R with its reason, two-argument `min`/`max` only
  (`min(x)` is refused), and the interrupt poll; linked from
  `?rfunction`.
- Every exported function's roxygen page complete, with examples that
  run on every tier.

**Tests.**

- Every §5 example and every argument and return type against a
  reference written in R, in `tests/testthat/test-cfunction.R`.
- Refusals: wrong type, wrong length, a missing `n`, an unsupported
  signature, a non-`const` pointer not named in `out` (D24), each with
  its condition class, in the same file.
- Outputs (D24): a non-`const` pointer seeded from the caller's value
  returns the modified copy and leaves the caller's vector unchanged
  (checked bitwise before and after, and with a second name bound to
  it); one not supplied starts zero-filled; a return value with two
  outputs comes back as a named list in the documented order, in the
  same file.
- Imports through `cfunction()` itself: a consumer's import and a name
  from `rfunction()`'s list (`R_pow`) are link errors, in the same file.
- Lengths at the limit: `n` equal to `length(x)`, one less, one more; and
  a vector longer than `.Machine$integer.max` refused before any data
  pointer is taken, with an ALTREP sequence ([#24](https://github.com/pedrobtz/zumir/issues/24)), in the same
  file.
- The returned closure keeps its context alive through `rm()` and
  `gc()`, fails with a classed error after `readRDS`, and gives the
  parent's answer in a forked child ([#23](https://github.com/pedrobtz/zumir/issues/23)); the same for
  `rfunction()` and `mir_function()` closures, in
  `tests/testthat/test-closures.R`.
- `tier = "interp"` and native give identical results, in the same file.
- Values under a hostile session (`LC_ALL=C`, `OutDec = ","`), unchanged.

**Exit criteria.**

- Every Tests line holds on the full profile; both articles build with
  their code evaluated.

**Design.** [§4.5](design.md#45-cfunction) step 5, [§5](design.md#5-public-r-api-complete-v010-surface),
[§6](design.md#6-errors), D16, D22, D24, Q2.

## Stage 10 — Benchmarks and README

**Issue.** [#10](https://github.com/pedrobtz/zumir/issues/10), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `quick`.

**Goal.** §9 is measured and recorded, and the README sells the package.

**Work.**

- `tools/run-benchmarks` and `.agents/benchmarks.md` for every §9 row.
- One look at the macOS Intel runner's slow interpreter (34 to 60× in the
  spike), recorded with what it found.
- The interpreter's cost under `MIR_DIRECT_DISPATCH` against the spike's
  computed-goto figures, on every leg (§9).
- README written with `pkg-readme`; `cran-comments.md`'s explanation of
  run-time code generation (the `luajr` precedent).

**Tests.**

- Benchmarks are not tests; the §9 table is the record.

**Exit criteria.**

- Every §9 row has a measured value on at least Linux x86-64, macOS arm64
  and Windows.

**Design.** [§9](design.md#9-performance-targets).

## Stage 11 — Assurance

**Issue.** [#11](https://github.com/pedrobtz/zumir/issues/11), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `quick`.

**Goal.** `.claude/shared/assurance.md` is met for `compiled`,
`untrusted-input`, `spec` and `lifecycle`.

**Work.**

- `mutation` on a schedule over the C tests; the fuzz `reach-floor` set.
- Every row of `.agents/assurance.md` filled; mutation survivors and the
  fuzz-reach and coverage gap lists read; missing oracles filed.

**Tests.**

- Whatever the gap lists show is missing, each with its oracle, in the
  file the row names.

**Exit criteria.**

- `assurance.yml` green; every remaining `gap #<issue>` closed or moved
  under "After 0.1.0" with a reason.

**Design.** [§8](design.md#8-testing).

## Stage 12 — Full CI

**Issue.** [#12](https://github.com/pedrobtz/zumir/issues/12), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full`.

**Goal.** The full r-actions profile is green on the default branch's code.

**Work.**

- Run the full profile (`full-ci`) and fix what it finds: valgrind, ASan,
  every platform leg, `nosuggests`, `nold`, cran-special.
- Dispatch gctorture at step 20 and fix what it finds, with any open
  `gctorture` issue.

**Exit criteria.**

- A full-profile run green on the pull request, a dispatched one green on
  the merge commit, and a step-20 gctorture run green on the pull
  request's last commit.

## Stage 13 — v0.1.0 readiness

**Issue.** [#13](https://github.com/pedrobtz/zumir/issues/13), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full`.

**Goal.** The package is ready to submit, apart from review.

**Work.**

- `Version: 0.1.0` and its `NEWS.md` heading.
- `cran-comments.md` listing the r-actions legs as test environments.
- `urlchecker::url_check()` and `spelling::spell_check_package()` clean.
- `zupmxode` builds and passes its Stage 2 against this package (§11).

**Exit criteria.**

- A full-profile run green on the merge commit.
- §11's criteria hold.

**Design.** [§11](design.md#11-acceptance-criteria-for-v010).

## Stage 14 — Review

**Issue.** [#14](https://github.com/pedrobtz/zumir/issues/14), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1). No pull request closes it.

**CI.** `full`.

**Goal.** The release candidate has survived both reviews.

**Work.**

1. `pkg-critical-review`: blocking and required findings become
   sub-issues of this issue, fixed together in one pull request.
2. `pkg-cran-review`, after those fixes.

**Exit criteria.**

- Every blocking and required review issue closed; the `pkg-cran-review`
  table clean; `NEWS.md` reflects any user-visible fix; a full-profile run
  green on the default branch, whose head becomes the release commit.

## After 0.1.0

- [§13](design.md#13-later-versions-in-the-order-they-are-expected):
  caching generated code, more `cfunction()` types, the parallel
  generator, a Python package, `mir2c`. None is needed by `zupmxode` 0.1.0.
- ppc64le, s390x and riscv64 (D7): built when the host matches, not tested
  or promised.

## Blocking open questions

- [Q6](design.md#14-open-questions) is settled by Stage 6 (inference,
  with `types =` only if needed); it does not block it.
- [Q5](design.md#14-open-questions): what runs where W+X memory is
  refused, now (a) the dual-mapped allocator as a fallback or (c) no
  tier. It is decided before Stage 5 freezes the table's capability
  flags. The maintainer decides it as Stage 4's first task; Stage 4
  can start, but not finish, before then.
- [Q2](design.md#14-open-questions) blocks Stage 9: one function per
  `cfunction()` source, or several. It meets D24 in the same wrapper,
  so the maintainer reads both before the stage starts.
- [Q4](design.md#14-open-questions) decides whether Stage 13 ends in a
  CRAN submission; the board's `CRAN intent` answers it.
