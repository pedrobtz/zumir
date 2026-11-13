# Assurance

How this package meets `.claude/shared/assurance.md`. CI checks this
file (r-actions `assurance.yml`), so keep the headings and table
columns as they are.

- **Statuses:** a cell is a fact, `gap #<issue>` or `n/a: <reason>`.
- **Kinds:** delete the ones that do not apply.
- **Tables:** delete a table only when no kind requires it. Delete the
  example rows.

Written 2026-11-13, before any code beyond the scaffold exists. Every row
comes from [design.md](design.md) and [roadmap.md](roadmap.md), not from
code, so almost every cell is a gap. A gap names the stage issue whose
pull request writes the test, or an `assurance` issue where no stage
planned an oracle. Each stage pull request turns its rows into facts.

## Kinds

`compiled` `untrusted-input` `spec` `lifecycle`

<!-- Evidence: compiled, src/ and vendored MIR (design §4.1); untrusted-input,
C and R source and MIR text from users (§4.5, §4.6, §5); spec, MIR's
textual IR and C11 through c2mir (§5, §4.5); lifecycle, contexts and
compiled functions held by external pointers (§4.3, §5). Not emits: the
package writes no bytes another program reads. -->

## Required checks

`Where` names the workflow file under `.github/workflows/` that runs the
check. Add `(own)` when that workflow is the package's own rather than a
call to r-actions.

| Check         | Status | Where              |
| ------------- | ------ | ------------------ |
| sanitizers    | gap #3 |                    |
| valgrind      | gap #3 |                    |
| rchk          | gap #3 |                    |
| gctorture     | gap #3 |                    |
| analyzers     | gap #3 |                    |
| alloc-failure | gap #5 |                    |
| mutation      | gap #11 |                   |
| fuzz          | gap #8 |                    |
| fuzz-reach    | gap #8 |                    |

Stage 1 (#3) wires `native-checks.yaml`; Stage 3 (#5) wires
`alloc-failure`, with the error trap it exercises; Stage 7 (#8) wires
`fuzz`, with c2mir, the first parser of untrusted input that needs it;
Stage 10 (#11) schedules `mutation` over the C tests.

## Boundaries

One row per boundary the package crosses. An oracle knows the answer:

- a conformance suite or test vectors;
- a reference implementation;
- a round trip;
- equivalent inputs compared;
- the bytes the other side received.

"No crash" is not an oracle.

<!-- markdownlint-disable MD013 -->
| Boundary | Oracle | Carried by |
| --- | --- | --- |
| Vendored MIR, built and patched | MIR's own c2mir-free test targets at the pinned commit, their expected outputs, run on the patched copy built without R | gap #4: tools/ci/platform.sh |
| Vendored MIR, provenance | upstream at the pinned commit plus exactly the patch queue | gap #4: tools/verify-vendor |
| Compiled code against CRAN's rules | `nm -u` on the built objects finds no exit, abort, printf, puts, stderr, stdout, sprintf or rand | gap #5: tools/ci/platform.sh |
| MIR generator: module to machine code | the same function in C from the leg's compiler, bitwise on x86-64 | gap #7: tests/consumer/ |
| MIR generator on aarch64 | the same C built with `-ffp-contract=off`, bitwise | gap #18 |
| MIR interpreter | native code on the same module, bitwise, on the three spike right-hand sides | gap #6: tests/testthat/test-tiers.R |
| Error trap: invalid modules | a `zumir_error` of the documented class each time, the session alive and RSS flat over 10³ failures of three kinds, through an internal test entry point | gap #5: tests/testthat/test-error-trap.R |
| Executable memory refused (Linux) | the documented tier and reason, not a signal, under a seccomp filter in a forked child | gap #6: tests/testthat/test-tier.R |
| C API table v1 | a consumer package that links it, emits three right-hand sides, and compares them with C | gap #7: tests/consumer/ |
| Double-expression emitter | each comparison and branch helper against C on NaN and Inf operands | gap #7: tests/consumer/ |
| Consumer imports | an import resolves in its own context only, and never in `cfunction()` | gap #7: tests/consumer/ |
| `mir_function()`: MIR text in | MIR's `run-test` round trip (text read, written back, compared) on `mir-tests/*.mir`, and hand-written expected results for the ones that return a value | gap #19 |
| c2mir: C source to MIR | the `c-tests` corpus by its own rules (272 `.expect` outputs, exit status for the rest), with a test-only import list for printf, abort and exit | gap #20 |
| c2mir diagnostics | an error planted at a known line and column is reported there, in the user's source, not the prelude | gap #8: tests/testthat/test-c2mir-errors.R |
| Prelude and import resolver (D6) | R's own exp, log, sqrt and the rest on the same inputs; a name outside D6 is a link error | gap #8: tests/testthat/test-prelude.R |
| `cfunction()` marshalling | a reference written in R for every example and every argument and return type | gap #9: tests/testthat/test-cfunction.R |
| `rfunction()`: R subset to native | R evaluating the same function on the same inputs, compared with `identical(num.eq = FALSE)` and a sign-of-zero check | gap #21 |
| `rfunction()`, random functions | R on generated functions within the subset, on random inputs, seed printed | gap #17: tests/testthat/test-rfunction-random.R |
| `rfunction()`, refusals | each construct outside §4.6 refused with its name | gap #17: tests/testthat/test-rfunction-refuse.R |
<!-- markdownlint-enable MD013 -->

## Objects

One row per object that outlives the call that made it. "Sequences" is
the test of its out-of-order use (abort then read, use after end, use
after free). The guard columns name the test, or say why the guard
cannot apply.

<!-- markdownlint-disable MD013 -->
| Object | Sequences | Fork | Unwind | Owner |
| --- | --- | --- | --- | --- |
| MIR context held by R (external pointer) | gap #22 | gap #22 | gap #22 | gap #6: finalizer frees the context and its code; tests/testthat/test-finalizer.R |
| Compiled function from `cfunction()`, `rfunction()` or `mir_function()` | gap #23 | gap #23 | n/a: a call into generated code cannot be interrupted by R; errors return a status | gap #23 |
| `zumir_ctx` owned by a consumer through the C API | gap #7: compile after free and free twice, in the consumer package | n/a: the consumer owns fork policy | n/a: `zumir_` entries return a status and never longjmp | gap #7: the consumer frees it with `zumir_ctx_free` |
| Interpreter contexts, one per thread (D10) | gap #6: tests/testthat/test-threads.R | n/a: created and freed inside one parallel region | n/a: no R call inside the region | gap #6: tests/testthat/test-threads.R |
<!-- markdownlint-enable MD013 -->

## Limits

One row per limit the package enforces. "Straddled by" is the test of
`limit - 1`, `limit` and `limit + 1` on every path that reaches it.

<!-- markdownlint-disable MD013 -->
| Limit | Unit | Straddled by |
| --- | --- | --- |
| `cfunction()` length argument against `length(x)` | elements | gap #9: tests/testthat/test-cfunction.R |
| Vector length against C `int` (`.Machine$integer.max`) | elements | gap #24 |
| `rfunction()` index range, 1 to `length(x)` | elements | gap #17: tests/testthat/test-rfunction-errors.R |
| `rfunction()` `numeric()` size, a whole number from 0 | elements | gap #17: tests/testthat/test-rfunction-errors.R |
<!-- markdownlint-enable MD013 -->
