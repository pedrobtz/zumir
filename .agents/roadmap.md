# zumir: roadmap

Status: draft 2026-11-13. Implements [design.md](design.md).
Target: v0.1.0, the first CRAN release.
Tracking: [v0.1.0 (#1)](https://github.com/pedrobtz/zumir/issues/1), with one
sub-issue per stage; [CRAN-submission (#15)](https://github.com/pedrobtz/zumir/issues/15).

Moved here from the design's §10 on 2026-11-13. The design's six stages
become Stages 0 and 2 to 8 below, with the portfolio's identity, assurance
and closing stages around them. The spike's findings split the design's
old Stage 2 (error trap, allocators, tier detection) in two.

## Stages

| Stage | Title | CI | Issue | Pull request | Done |
| --- | --- | --- | --- | --- | --- |
| 0 | [MIR spike](#stage-0--mir-spike) | n/a | [#2](https://github.com/pedrobtz/zumir/issues/2) | [mirspike](https://github.com/pedrobtz/mirspike) | 2026-11-13 |
| 1 | [Package identity](#stage-1--package-identity) | quick | [#3](https://github.com/pedrobtz/zumir/issues/3) | | |
| 2 | [Vendored MIR, clean on every leg](#stage-2--vendored-mir-clean-on-every-leg) | full | [#4](https://github.com/pedrobtz/zumir/issues/4) | | |
| 3 | [Error trap](#stage-3--error-trap) | full | [#5](https://github.com/pedrobtz/zumir/issues/5) | | |
| 4 | [Contexts, tiers and zumir_info()](#stage-4--contexts-tiers-and-zumir_info) | full | [#6](https://github.com/pedrobtz/zumir/issues/6) | | |
| 5 | [C API and mir_function()](#stage-5--c-api-and-mir_function) | quick | [#7](https://github.com/pedrobtz/zumir/issues/7) | | |
| 6 | [c2mir, prelude and import resolver](#stage-6--c2mir-prelude-and-import-resolver) | full | [#8](https://github.com/pedrobtz/zumir/issues/8) | | |
| 7 | [cfunction()](#stage-7--cfunction) | quick | [#9](https://github.com/pedrobtz/zumir/issues/9) | | |
| 8 | [Benchmarks and README](#stage-8--benchmarks-and-readme) | quick | [#10](https://github.com/pedrobtz/zumir/issues/10) | | |
| 9 | [Assurance](#stage-9--assurance) | quick | [#11](https://github.com/pedrobtz/zumir/issues/11) | | |
| 10 | [Full CI](#stage-10--full-ci) | full | [#12](https://github.com/pedrobtz/zumir/issues/12) | | |
| 11 | [v0.1.0 readiness](#stage-11--v010-readiness) | full | [#13](https://github.com/pedrobtz/zumir/issues/13) | | |
| 12 | [Review](#stage-12--review) | full | [#14](https://github.com/pedrobtz/zumir/issues/14) | | |

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
  every `MIR_`/`_MIR_` symbol. Check first whether upstream #490 is in
  the pinned commit.
- `tools/ci/platform.sh` runs MIR's own c2mir-free tests on the vendored,
  patched copy, built standalone without R, on every runner; a container
  job does the same in the r-devel images.
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

**Exit criteria.**

- R CMD check on the full profile has no compiler-diagnostic WARNING on
  any leg, including Windows (Rtools) and gcc 16 with `-pedantic`; the
  only allowed warning is "checking compiled code", which Stage 3 removes.
- MIR's own tests pass on every runner and container.
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

**Tests.**

- Three invalid modules (a syntax error, an operand type error, an
  undefined import), each 10³ times through the entry points: a
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

- Contexts created with `MIR_init2` and counting allocators; finalizers
  free contexts and their code.
- The code-memory patch (D11): every `mem_map`/`mem_protect` result
  checked.
- Q5's answer implemented: a dual-mapped code allocator if (a) is
  chosen.
- The tier probe (§4.4); `zumir_info()` (MIR commit, targets, tier and
  why, patches, memory); `zumir_tier()`.

**Tests.**

- Tier detection with W+X refused: a seccomp filter in a forked child
  (mirspike's harness) gives the documented tier and reason, not a signal,
  in `tests/testthat/test-tier.R` (Linux).
- Native against interpreter: the three spike right-hand sides, bitwise,
  in `tests/testthat/test-tiers.R`.
- Contexts freed: memory reported by `zumir_info()` returns to its
  baseline after `gc()`, under gctorture too, in
  `tests/testthat/test-finalizer.R`.
- Threads: native code compiled once on 8 threads, and the interpreter with
  a context per thread (D10), both against the serial result bitwise, in
  `tests/testthat/test-threads.R`.

**Exit criteria.**

- Every Tests line holds on the full profile, including the Linux W+X
  test on ubuntu-latest and ubuntu-24.04-arm.
- A dispatched step-20 gctorture run is green on the pull request's last
  commit.

**Design.** [§4.3](design.md#43-the-c-api) (memory, threads),
[§4.4](design.md#44-tier-detection), [§7](design.md#7-limits-and-portability),
D10, D11, Q5.

## Stage 5 — C API and mir_function()

**Issue.** [#7](https://github.com/pedrobtz/zumir/issues/7), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `quick`.

**Goal.** A consumer package can link `zumir`, build MIR through the API
table and call the result; R users can run MIR text.

**Work.**

- `zumir_api_v1` registered with `R_RegisterCCallable`;
  `inst/include/zumir.h`, standalone C99, with inline wrappers.
- `mir_function()`.
- A consumer test package under `tests/consumer/`, built in CI against the
  installed `zumir`, that emits the three spike right-hand sides.

**Tests.**

- The consumer's three right-hand sides against the same functions in C,
  bitwise on x86-64 and to one ulp on aarch64, in the consumer's tests.
- `mir_function()` on the design's `add` example and on MIR's own
  `mir-tests/*.mir` that return a value, against their expected results, in
  `tests/testthat/test-mir-function.R`.
- The header compiles alone as C99 with `-pedantic`, in CI.
- The table only grows: a test that the version field is first and `1`, in
  the consumer package.

**Exit criteria.**

- The consumer package builds and passes on the quick profile.
- Every Tests line exists and was seen to fail.

**Design.** [§4.3](design.md#43-the-c-api), [§5](design.md#5-public-r-api-complete-v010-surface)
(`mir_function()`), D3.

## Stage 6 — c2mir, prelude and import resolver

**Issue.** [#8](https://github.com/pedrobtz/zumir/issues/8), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `full — a vendored library added (c2mir) and a parser of untrusted
input: ASan, valgrind and a fuzz harness`.

**Goal.** C source compiles in memory to MIR on every leg, reading no
system header, with only the allowed library names resolving.

**Work.**

- `c2mir/` vendored with its per-target headers; its patches queued (the
  same no-exit treatment).
- The prelude and the built-in headers (§4.5, D5); the import resolver
  with the fixed list (D6).
- Diagnostics mapped back to the user's line and column.
- A standalone fuzz target over `cfunction()`'s compile path, built
  without R, and `fuzz.yml` wired with a corpus.

**Tests.**

- The prelude's functions resolve and return what R's own `exp`, `log`,
  `sqrt`, … return, on every leg, in `tests/testthat/test-prelude.R`.
- A name outside D6's list is a link error with class `zumir_error`, in the
  same file.
- Diagnostics: a syntax error on line 3 of the user's source is reported
  at line 3, in `tests/testthat/test-c2mir-errors.R`.
- A subset of MIR's `c-tests` that needs no system header: their expected
  outputs, run through the vendored `c2mir`, in `tools/ci/platform.sh`.
- Fuzzing: no crash, and each crash input filed upstream with a
  reproducer. This proves robustness only; correctness is the line
  above.

**Exit criteria.**

- Every Tests line holds on the full profile.
- The fuzz target runs in CI and its findings are fixed or filed.

**Design.** [§4.5](design.md#45-cfunction) steps 1 to 4, D5, D6, D9.

## Stage 7 — cfunction()

**Issue.** [#9](https://github.com/pedrobtz/zumir/issues/9), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `quick`.

**Goal.** §5's surface is complete.

**Work.**

- `cfunction()` with its signature parser, argument checks, output
  allocation and the one `.Call` wrapper; `tier =`.
- The Get started article's chunks switched to `eval = TRUE`.

**Tests.**

- Every §5 example and every argument and return type against a
  reference written in R, in `tests/testthat/test-cfunction.R`.
- Refusals: wrong type, wrong length, a missing `n`, an unsupported
  signature, each with its condition class, in the same file.
- Lengths at the limit: `n` equal to `length(x)`, one less, one more, and
  a vector longer than `.Machine$integer.max` refused, in the same file.
- `tier = "interp"` and native give identical results, in the same file.
- Values under a hostile session (`LC_ALL=C`, `OutDec = ","`), unchanged.

**Exit criteria.**

- Every Tests line holds; the article builds with its code evaluated.

**Design.** [§4.5](design.md#45-cfunction) step 5, [§5](design.md#5-public-r-api-complete-v010-surface),
[§6](design.md#6-errors), Q2.

## Stage 8 — Benchmarks and README

**Issue.** [#10](https://github.com/pedrobtz/zumir/issues/10), sub-issue of
[v0.1.0](https://github.com/pedrobtz/zumir/issues/1).

**CI.** `quick`.

**Goal.** §9 is measured and recorded, and the README sells the package.

**Work.**

- `tools/run-benchmarks` and `.agents/benchmarks.md` for every §9 row.
- One look at the macOS Intel runner's slow interpreter (34 to 60× in the
  spike), recorded with what it found.
- README written with `pkg-readme`; `cran-comments.md`'s explanation of
  run-time code generation (the `luajr` precedent).

**Tests.**

- Benchmarks are not tests; the §9 table is the record.

**Exit criteria.**

- Every §9 row has a measured value on at least Linux x86-64, macOS arm64
  and Windows.

**Design.** [§9](design.md#9-performance-targets).

## Stage 9 — Assurance

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

## Stage 10 — Full CI

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

## Stage 11 — v0.1.0 readiness

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

## Stage 12 — Review

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

- [Q5](design.md#14-open-questions) blocks Stage 4: what runs where W+X
  memory is refused. Stage 4 implements the answer.
- [Q2](design.md#14-open-questions) blocks Stage 7: one function per
  `cfunction()` source, or several.
- [Q4](design.md#14-open-questions) decides whether Stage 11 ends in a
  CRAN submission; the board's `CRAN intent` answers it.
