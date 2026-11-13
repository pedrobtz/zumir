# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working
with code in this repository.

## Shared conventions

The portfolio’s shared rules are synced into `.claude/shared/` from
`pedrobtz/packages:claude/shared/`. Never edit them here. Where this
file and a shared file disagree, this file wins for this package.

@.claude/shared/workflow.md @.claude/shared/r-package.md
@.claude/shared/ci.md @.claude/shared/native-code.md
@.claude/shared/correctness.md @.claude/shared/assurance.md

## What this is

`zumir` brings MIR, Vladimir Makarov’s lightweight JIT compiler, to R in
three ways. First, as a provider: family packages (`zupmxode` first) use
`LinkingTo: zumir` and a versioned C API table to build MIR functions
and get back native function pointers, or the interpreter where native
code can’t be made. Second, as `cfunction()`: R users compile a short
numeric C function with MIR’s own `c2mir` compiler, with no Rtools,
Xcode tools or `gcc` needed. Third, as `rfunction()`: a numeric R
function, in the subset of design §4.6, compiled to native code that
returns exactly what R returns. There are no `Imports`, only
`Suggests: testthat`. MIR is vendored, and the whole package is one
shared object. It is not a full C compiler, not a sandbox (user code
runs with [`.Call()`](https://rdrr.io/r/base/CallExternal.html)’s
rights), and not a wrapper around the system toolchain the way `inline`
and `Rcpp` are.

The specification is
[`.agents/design.md`](https://pedrobtz.github.io/zumir/.agents/design.md),
adopted from RFC 0073. It outranks this file: `CLAUDE.md` orients, the
design decides.

## Current state

Scaffolded 2026-11-13 with `create-pkg.sh`; no functionality yet. The
roadmap in `.agents/roadmap.md` takes it to v0.1.0.

Stage 0, the MIR spike, ran outside this repository in
[`pedrobtz/mirspike`](https://github.com/pedrobtz/mirspike) and decided
that `zumir` is built (its `RESULTS.md`, 2026-11-13). Its harness (the
three right-hand sides, the seccomp W+X test, the error loop) is the
starting point for Stages 2 to 4.

## Versions and dependents

The current version’s parent issue is
[\#1](https://github.com/pedrobtz/zumir/issues/1) (`v0.1.0`). Sibling
packages that depend on this one before CRAN does: `zupmxode` (RFC
0047), which lowers its IR to MIR through the C API.

## Commands

Only the shared commands so far.

## Architecture

To be written as stages land; see `.agents/design.md`. The scaffold set
up a C native layer (`src/zumir.c`, registered with `useDynLib`).

## Invariants that are easy to break

These are fixed by the design before any code exists:

- **MIR never ends the R process** (D4). MIR’s default error handler
  calls `exit(1)`, and `MIR_init` can reach it before a handler is
  installed. Every context gets `zumir`’s handler through
  `MIR_set_error_func`, the paths that run before that are patched, and
  every entry from R or from the table is a recovery point. A missed
  path kills the analyst’s session.
- **No `longjmp` crosses a consumer’s frames.** Every `zumir_` entry in
  the C API returns a status code. Errors are caught at the `zumir`
  boundary (§4.3).
- **The C API table only grows.** Its first field is the version, and
  later versions only append fields. Consumers compiled against an older
  header must keep working.
- **No `exit`, `abort`, `stdout` or `stderr` in the shared object**
  (§11). MIR’s output goes to a buffer that becomes the R condition’s
  message.
- **`cfunction()` reads no system header** (D5), and its import resolver
  resolves only the fixed list in D6. Anything else is a link error, so
  the same source behaves the same everywhere and can’t reach R’s
  internals.
- **MIR’s symbols are hidden** from Stage 1, so they can’t clash with
  another package that vendors MIR.

## Testing

None yet: no conformance suite, no helpers, no runtime to report. Design
§8 plans a subset of MIR’s own `c-tests` and `mir-tests` on both tiers.

## Continuous integration

The package calls `pedrobtz/r-actions` for `R-CMD-check.yaml` (profile
`auto`) and `coverage.yaml`. `pkgdown.yaml` builds the site with
`r-lib/actions`. The roadmap marks Stage 6 `full`.

## Vendored native code

MIR (MIT), from `vnmakarov/mir` at `master` commit `dc2c763`
(2026-10-23). **Deviation from `native-code.md`:** that file says to pin
a stable release, never `master`. D2 pins `master` on purpose, because
the only tag (v1.0.0, May 2024) misses two and a half years of fixes. A
bump re-runs Stage 0’s checks. MIR goes under `src/vendor/mir/`, kept as
upstream plus a numbered patch queue in `tools/patches/`, never a fork
(D12). `tools/vendor.sh` records the commit, the file list and the
patches (§4.1, §4.2). Not vendored yet: fill in the file list and every
local patch when Stage 2 lands. The README’s Mozilla Public License
mention is about NanoJIT, not any MIR file (spike question 7).
