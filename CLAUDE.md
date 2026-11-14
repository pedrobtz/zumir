# CLAUDE.md

<!-- markdownlint-disable-next-line MD013 -->
This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Shared conventions

The portfolio's shared rules are synced into `.claude/shared/` from
`pedrobtz/packages:claude/shared/`. Never edit them here. Where this file and
a shared file disagree, this file wins for this package.

@.claude/shared/workflow.md
@.claude/shared/r-package.md
@.claude/shared/ci.md
@.claude/shared/native-code.md
@.claude/shared/correctness.md
@.claude/shared/assurance.md

## What this is

`zumir` brings MIR, Vladimir Makarov's lightweight JIT compiler, to R in three
ways. First, as a provider: family packages (`zupmxode` first) use
`LinkingTo: zumir` and a versioned C API table to build MIR functions and get
back native function pointers, or MIR's interpreter where MIR has no code
generator for the CPU. Both tiers need executable memory (design §4.4, Q5). Second, as `cfunction()`: R users compile a short numeric C function
with MIR's own `c2mir` compiler, with no Rtools, Xcode tools or `gcc` needed.
Third, as `mir_compile()`: a numeric R function, in the subset of design §4.6,
compiled to native code that returns exactly what R returns.
There are no `Imports`, only `Suggests: testthat`. MIR is vendored, and the
whole package is one shared object. It is not a full C compiler, not a sandbox
(user code runs with `.Call()`'s rights), and not a wrapper around the system
toolchain the way `inline` and `Rcpp` are.

The specification is [`.agents/design.md`](.agents/design.md), adopted from
RFC 0073. It outranks this file: `CLAUDE.md` orients, the design decides.

## Current state

Scaffolded 2026-11-13 with `create-pkg.sh`. Stage 1 (package identity)
gave it its `DESCRIPTION`, routine registration with dynamic lookup off,
and the CI and assurance wiring. Stage 2 vendored MIR, clean on every
leg; Stage 3 added the error trap; Stage 4 contexts held by R, the
load-time tier probe, and the first exports, `zumir_info()` and
`zumir_tier()`. Q5 is decided (c): where W+X memory is refused, no tier
runs and `zumir_tier()` is `"none"`. Stage 5 added the C API table
(`inst/include/zumir.h`, `src/zumir_api.c`), the import resolver with
zumir's ports of R's arithmetic (`src/zumir_imports.c`), and
`mir_function()` (`src/zumir_mirf.c`). Stage 6 added `mir_compile()`: an R
walker in `R/mir_compile.R` that emits MIR text, the `.Call` bridge
`src/zumir_rf.c`, and ports of R 4.6's math wrappers. Stage 8 vendored
c2mir (`src/zumir_c2mir.c`, with the fork's `C2MIR_NO_FILE_INCLUDES`)
and the compile path `c2mir_compile()` (`src/zumir_cfun.c`, with the
prelude in `src/zumir_prelude.c` and D6's table in
`src/zumir_imports_d6.c`); `cfunction()` is Stage 9. Stage 7 added the
emitter route (`zumir_rf_build`), which builds the module with MIR's API
from the lowering's instructions; the text route stays, internal
(`rf_make(route = "text")`), as its oracle. The roadmap in `.agents/roadmap.md` takes it to v0.1.0.

Stage 0, the MIR spike, ran outside this repository in
[`pedrobtz/mirspike`](https://github.com/pedrobtz/mirspike) and decided that
`zumir` is built (its `RESULTS.md`, 2026-11-13). Its harness (the three
right-hand sides, the seccomp W+X test, the error loop) is the starting
point for Stages 2 to 4.

## Versions and dependents

The current version's parent issue is
[#1](https://github.com/pedrobtz/zumir/issues/1) (`v0.1.0`). Sibling packages that depend on
this one before CRAN does: `zupmxode` (RFC 0047), which lowers its IR to MIR
through the C API.

## Commands

The shared commands, plus:

```sh
R CMD INSTALL . && Rscript tools/ci-run-tests.R   # the suite as CI runs it
R CMD INSTALL tests/consumer                      # the stand-in C API consumer
Rscript -e 'testthat::test_dir("tests/consumer/tests/testthat",
  package = "zumirconsumer", load_package = "installed")'
ZUMIR_TEST_SEED=<n> Rscript tools/ci-run-tests.R  # replay a shuffled order
bash tools/ci/platform.sh <macos|windows|ubuntu>  # the per-OS checks
```

`tools/ci-run-tests.R` runs the installed package's tests in its
namespace, shuffled, with the seed printed. Tests that start a child R
process (`helper-child.R`) skip under `devtools::load_all()`, where there
is no installed copy, so run this or `devtools::check()` before claiming
them green.

## Architecture

To be written as stages land; see `.agents/design.md`. The scaffold set up a
C native layer (`src/zumir.c`, registered with `useDynLib`).

## Invariants that are easy to break

These are fixed by the design before any code exists:

- **MIR never ends the R process** (D4). MIR's default error handler calls
  `exit(1)`, and `MIR_init` can reach it before a handler is installed. Every
  context gets `zumir`'s handler through `MIR_set_error_func`, the paths that
  run before that are patched, and every entry from R or from the table is a
  recovery point. A missed path kills the analyst's session.
- **Every call into MIR is under a trap** (`src/zumir_trap.h`): push, then
  `setjmp` in the entry's own frame, MIR calls, pop. Nothing between the
  `setjmp` and the MIR calls' end may call R or allocate R memory, and a
  context that raised may only be finished, under a trap of its own.
- **`mir.h` only through `src/zumir_mir_api.h`.** Its inline
  `MIR_init2` writes to `stderr`; the header puts the redirection first.
- **A context R holds is a `zumir_context` external pointer**
  (`src/zumir_ctx.c`); every MIR call on it goes through
  `zumir_rctx_run()`, and entries return a `zumir_status` that
  `zumir_check()` signals in R, so no R error is raised under a trap.
- **Contexts only from `zumir_mir_new(&arena)`**, under a trap, and freed
  only by `zumir_mir_free(ctx, arena)`: the arena's allocator raises
  instead of returning NULL (MIR does not always check), and after a
  failed allocation the arena is freed directly, never by `MIR_finish`,
  which would walk half-built objects. Never call `MIR_finish` yourself.
- **No `longjmp` crosses a consumer's frames.** Every `zumir_` entry in the
  C API returns a status code, and every `MIR_` entry is a shim that traps
  and records the error in its context, which can then only be freed
  (§4.3, D20). A trap set in one call never covers a later one.
- **A MIR update can break consumers' binaries.** The table carries a MIR
  ABI number; bump it whenever an exposed `mir.h` enum or struct changes
  (D21).
- **`zumir_api_v1`'s layout is frozen** in
  `tests/consumer/src/frozen/zumir_v1.h`; the consumer builds a unit
  against it on every runner. Never edit that file: append fields to the
  table, and add `zumir_v2.h` beside it when v2 ships.
- **The C API table only grows.** Its first field is the version, and later
  versions only append fields. Consumers compiled against an older header
  must keep working.
- **No `exit`, `abort`, `stdout` or `stderr` in the shared object** (§11).
  MIR's output goes to a buffer that becomes the R condition's message.
- **The interpreter is not shareable across threads** (D10): one interpreted
  function called from eight threads gave wrong answers in the spike. On that
  tier each thread compiles in its own context; native code is shared.
- **`mir_compile()` answers as R does or errors** (D16): every non-NaN result
  equals R's bit for bit, and where matching R is not cheap it refuses. Its
  tests compare with R through `identical(num.eq = FALSE)` (#21).
- **`cfunction()` reads no system header** (D5): no header at all is read
  from a file (the fork's `C2MIR_NO_FILE_INCLUDES`), and the prelude's
  declarations are exactly D6's table, which `test-prelude.R` checks
  both ways. Its import resolver
  resolves only the fixed list in D6. Anything else is a link error, so the
  same source behaves the same everywhere and can't reach R's internals.
- **MIR's symbols are hidden**, so they can't clash with another package
  that vendors MIR. `R_init_zumir` is the only export; `platform.sh` checks
  it on every runner. A new source file gets `$(C_VISIBILITY)` too.

## Testing

`test-prelude.R` and `test-c2mir-errors.R` (Stage 8): the prelude
against D6's table, each name against the C library or R, the
diagnostics' line and column, and no header file read. The `c-tests`
corpus runs through the same compile path in
`tests/c/c-tests-runner.c` (`platform.sh`), and must fail exactly the
tests in `tools/c-tests-expected.txt`; `tests/fuzz/fuzz-c2mir.c` is the
libFuzzer target (`fuzz.yaml`).
`mir_compile()`'s oracle is R itself, through `compare_with_r()` in
`helper-mir-compile.R`: `identical(num.eq = FALSE)`, with R's side run
uncompiled and with the JIT off (R 4.6's byte compiler can turn `-0`
into `0`). `test-mir-compile-ops.R` (each construct),
`test-mir-compile-errors.R` (run-time errors, warnings, limits and the time
limit in a child process), `test-mir-compile-refuse.R` (compile-time
refusals beside R running the same function) and
`test-mir-compile-random.R` (random functions from the helper's
generator; 10² per run, 10⁴ when `R_ACTIONS_PROFILE=full`;
`ZUMIR_RF_SEED=<n>` replays one, and a difference is saved under
`tempdir()`), and `test-mir-compile-routes.R` (the two routes print the same
module and give R's results; each of MIR's allocations failed in the
emitter). `tools/bench-mir-compile-routes.R` times the routes for
`.agents/benchmarks.md`.
`test-load.R`: the installed package loads in a child R process with
dynamic symbol lookup off, and unloading it removes its DLL.
Stage 4's: `test-tier.R` (the probe; W+X refused under seccomp in a child
process, Linux), `test-tiers.R` (native against interpreter, bitwise, on the
spike's three models in `src/zumir_test_models.c`), `test-threads.R`,
`test-finalizer.R` and `test-context-sequences.R`, through the internal
context wrappers in `R/context.R`. `test-error-trap.R`: MIR's errors as `zumir_mir_error` conditions through
an internal entry (`test_invalid_module()`), an error inside
`MIR_finish_func` then `MIR_finish`, and in a child process 10³ failures of
each kind with RSS flat, and every one of MIR's first 260 allocations
failed in turn (`.Call(zumir_test_fail_alloc, n)`) with no crash and no
arena left. `tests/c/test-trap-threads.c` (built without R by
`platform.sh`): the trap on two threads at once. `test-mir-stdio.R`: writes
to MIR's `stderr` reach the buffer, other
`FILE`s are untouched, and `exit` becomes an R error, through an internal
entry compiled under the same macros. `test-no-target.R`: MIR is compiled
in exactly where `helper-target.R`'s
platform table says; `platform.sh` runs the suite again on a build with
`ZUMIR_NO_MIR=1`, with `ZUMIR_EXPECT_NO_MIR=1`. `helper-child.R` holds the
child-process helpers (`installed_lib()`, `run_child()`); Stage 3's
error-trap loop uses them too. Design §8
plans MIR's own tests, `mir-tests` through `mir_function()`, and `c-tests`
through `c2mir` by the corpus's own rules, on both tiers; `.agents/assurance.md`
lists every oracle and its gap.

## Continuous integration

Every workflow calls `pedrobtz/r-actions` at `@v1`, except `coverage.yaml`,
which holds a write token and is pinned to a commit:

- `R-CMD-check.yaml`: `r-cmd-check.yml`, `profile: auto`, with a Windows
  R-devel row added to `runners` (full profile only).
- `native-checks.yaml`: sanitizers (with ASan), valgrind (with
  `--smc-check=all`, for the code MIR writes), lto, rchk,
  analyzers and cran-special. `analyzers` runs off pull requests only, with
  180 minutes: `-fanalyzer` on vendored MIR outran the 45-minute default,
  and `exclude` does not skip the analysis (pedrobtz/r-actions#113).
- `gctorture.yaml`: the wide step on pull requests under a 10-minute
  budget; step 20 daily (while the roadmap is implemented) and on dispatch.
- `platforms.yaml`: `tools/ci/platform.sh` on macOS arm64, Windows and
  Linux x86-64 and aarch64 (macOS x86-64 too on the schedule and on
  dispatch): MIR's own tests on the vendored copy, the symbol guards, and
  on Linux the build with MIR forced out.
- `alloc-failure.yaml`: weekly, on dispatch and on `full-ci` pull
  requests, fails each allocation in turn while `tools/alloc-exercise.R`
  runs one section per job (`ZUMIR_ALLOC_EXERCISE`): the error trap's
  three invalid modules, and `mir_compile()` through the emitter. The
  sweep is capped at 3,000 allocations, hence a job each.
- `fuzz.yaml`: libFuzzer over `tests/fuzz/fuzz-c2mir.c`, seeded by
  `tests/fuzz/corpus`; brief on pull requests, long on the schedule.
- `vendor.yaml`: `tools/verify-vendor`, and a pull request that changes
  `src/vendor/mir/` must change `src/vendor/PROVENANCE`.
- `coverage.yaml` with `native: true`; `pkgdown.yaml`; `assurance.yaml`
  with `ungated-jobs: fail`.

`vendor-upstream.yml` is not called: it compares release tags, and MIR is
pinned to a commit past the only one (D2), so it would always report a
difference.

Every job tests the labels in its `if:` (`ci.md`, Labels). The scheduled
slot is Saturday 08:00 to 09:59 UTC, a different minute per workflow;
gctorture's daily run is at 09:56. The roadmap marks Stages 2 to 9
`full`, with the closing three.

## Vendored native code

MIR (MIT), from `vnmakarov/mir` at commit
`dc2c763f1c8bc746102ce87933d866335f9e6c06` (`master`, 2026-10-23), always
named by its full hash. **Deviation from `native-code.md`:** that file says
to pin a stable release. D2 (the maintainer's decision) pins this commit on
purpose, because the only tag (v1.0.0, May 2024) misses two and a half years
of fixes. A bump re-runs the spike's checks.

MIR's core, generator, interpreter and c2mir are under `src/vendor/mir/`:
the 85 files in `tools/mir-files.txt`. They are compiled only through
`src/zumir_mir.c`, `src/zumir_mir_gen.c` and `src/zumir_c2mir.c`, which
include `mir.c`, `mir-gen.c` and `c2mir/c2mir.c` behind the target guard
in `src/zumir_mir.h` (`-Ivendor/mir`, for c2mir's own includes).

**Never edit `src/vendor/mir/` in place.** It is copied from the fork
[`pedrobtz/mir`](https://github.com/pedrobtz/mir) (D12, as `qio` vendors
carquet): the fork's `master` mirrors upstream, its `zumir` branch is the
upstream pin plus one commit per patch, and each patch also sits alone on
a `fix/` branch for upstreaming (#30). Change the `zumir` branch, push,
update `Fork commit:` and the patch list in `src/vendor/PROVENANCE`, and
run `tools/vendor.sh`. `tools/verify-vendor` (in `vendor.yaml`) fails on a
file edited in place, a series that differs from the list, or a changed
`mir.h` ABI (`tools/mir-abi.txt`, D21). **Deviation from
`native-code.md`:** the fork's commits, not `tools/patches/`, are the patch
record.

The patches, one commit each, are listed with their reasons in
`src/vendor/PROVENANCE` (23 on 2026-11-14): warnings and CRAN symbols,
Windows code memory, init and finish paths, code-memory errors, and from
Stage 8 c2mir's allocator, memory-only headers, and the undefined
behaviour the c-tests corpus found under the sanitizers in c2mir, the
generator and the interpreter.
Built differently instead of patched: `MIR_DIRECT_DISPATCH`, no
`-fsigned-char` (MIR's tests pass with unsigned `char`), and symbol hiding
by `$(C_VISIBILITY)` and `src/zumir-win.def`.

MIR's `exit`, `abort` and writes to `stderr` are redirected by macros in
`src/zumir_mir_stdio.h`, which the wrappers include just before MIR's
sources, to `src/zumir_mir_stdio.c`: a per-thread message buffer and one
exit hook, an R error until Stage 3 makes it the trap. Its sprintf calls
are snprintf on the fork. `src/zumir_mir_gcc.h` gives MIR's sources gcc's
`-fno-tree-sra` by pragma: without it Rtools gcc miscompiles the
interpreter. `platform.sh` fails on any CRAN-flagged import. The README's Mozilla Public License mention is about NanoJIT, not
any MIR file (spike question 7).
