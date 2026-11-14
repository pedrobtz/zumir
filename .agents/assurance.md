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

| Check         | Status  | Where              |
| ------------- | ------- | ------------------ |
| sanitizers    | done    | native-checks.yaml |
| valgrind      | done    | native-checks.yaml |
| rchk          | done    | native-checks.yaml |
| gctorture     | done    | gctorture.yaml     |
| analyzers     | done    | native-checks.yaml |
| alloc-failure | done    | alloc-failure.yaml |
| mutation      | gap #11 |                    |
| fuzz          | done    | fuzz.yaml          |
| fuzz-reach    | done    | fuzz.yaml          |

Stage 1 (#3) wired `native-checks.yaml` and `gctorture.yaml` on
2026-11-13, without `sanitizers`: its instrumentation guard fails a build
with nothing to instrument, so it joined with MIR in Stage 2 (#4). Stage 3
(#5) wires
`alloc-failure`, with the error trap it exercises; Stage 8 (#8) wires
`fuzz`, with c2mir, the first parser of untrusted input that needs it;
Stage 11 (#11) schedules `mutation` over the C tests.

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
| Vendored MIR, built and patched | MIR's own c2mir-free test targets at the pinned commit (28; `inline-test`, which builds c2mir's driver, left out: c2mir is tested on c-tests below), their expected outputs, run on the vendored copy built without R, with R's compiler and flags, on macOS, Windows and Linux x86-64 and aarch64 | tools/ci/platform.sh, in platforms.yaml |
| Vendored MIR, provenance | the fork commit, file for file, and that commit the upstream pin plus exactly the patch list, one commit each; its canary leaves the last patch out and must fail | tools/verify-vendor, in vendor.yaml |
| Compiled code against CRAN's rules | the shared object's imports (`nm -u`, `objdump -p` on Windows) hold no exit, abort, printf, puts, stderr, stdout, sprintf or rand; its exports are `R_init_zumir` alone | tools/ci/platform.sh, in platforms.yaml, on every runner |
| MIR generator: module to machine code | the same three right-hand sides in C from the leg's compiler, bitwise, 40 draws each, on both tiers | tests/consumer/tests/testthat/test-consumer.R, built and run by tools/ci/platform.sh on every runner |
| MIR generator on aarch64 | the same C built with `-ffp-contract=off` (tests/consumer/src/Makevars), bitwise | tests/consumer, on ubuntu-24.04-arm and macos-latest (platform.sh) |
| MIR interpreter | native code on the same module, bitwise, on the three spike right-hand sides (50 random draws each, and 0, -0, ±Inf, NaN, NA, subnormal and the largest double) | tests/testthat/test-tiers.R |
| Error trap: invalid modules | MIR's own error type (syntax, operand mode, undeclared reference) as a `zumir_mir_error` each time, the context finished and no trap left; in a child process 10³ failures of each kind all caught, the session alive and RSS growth under 4 MB for the second 10³ (a leaked context per failure is about 97 MB) | tests/testthat/test-error-trap.R |
| Allocation failure inside MIR | each of MIR's first 260 allocations failed in turn through zumir's allocator hook: a `zumir_mir_error` of `MIR_alloc_error` or the module's own type every time, never a crash, and no arena left (`zumir_test_live_arenas`); the weekly `alloc-failure` sweep fails R's allocations too | tests/testthat/test-error-trap.R; alloc-failure.yaml |
| Error trap on threads | each of two threads gets its own 500 errors, with its own MIR error type, and leaves no trap behind; under ASan and UBSan on Linux | tests/c/test-trap-threads.c, in tools/ci/platform.sh |
| MIR's exit, abort and stderr | what MIR writes to stderr reaches zumir's buffer, a caller's FILE is untouched, exit and abort land in the trap or, with none, become an R error | tests/testthat/test-mir-stdio.R |
| Executable memory refused (Linux) | under a seccomp filter in a child R process: tier `"none"`, `compile` false on both tiers with MIR's code-memory error and `EACCES`, and the process alive | tests/testthat/test-tier.R, on ubuntu-latest (R CMD check) and ubuntu-24.04-arm (platform.sh) |
| Tier probe | each tier's four capabilities, `arith` against `x * 2 + 1` and `ext` against libm's `exp`, bitwise; `zumir_tier()` from them | tests/testthat/test-tier.R |
| C API table v1 | a consumer package that links it, emits three right-hand sides, and compares them with C; links no CRAN-flagged symbol; the header compiles alone as C99 and C++11 with -pedantic -Werror | tests/consumer; tools/ci/platform.sh |
| C API compatibility (D21) | a unit built against the frozen v1 header runs; another MIR ABI number or a newer version gets `NULL` and a reason | tests/consumer (src/frozen/zumir_v1.h); tests/testthat/test-imports.R |
| Generated code's memory accesses | valgrind with `--smc-check=all` on the native tier, ASan on the interpreter tier, over the `rfunction()` and `cfunction()` suites | `rfunction()`: valgrind with `--smc-check=all` (native-checks.yaml) and ASan over the suite, whose random and route tests run both tiers; `cfunction()`: gap #11 |
| Double-expression emitter | the three right-hand sides written with it, bitwise against C; each comparison and branch helper against C on every pair of -Inf, -1, -0, 0, 1, Inf, NaN, NA | tests/consumer |
| Consumer imports | an import resolves in the context that registered it, is a link error in one that did not, and is not under D6's list (`mir_function()`) | tests/consumer; `cfunction()` isolation gap #9: tests/testthat/test-cfunction.R |
| `mir_function()`: MIR text in | MIR's 17 `mir-tests/*.mir` read and written back to a fixed point; hand-computed values of test1, test2, test6 and test7 on both tiers; argument limits straddled per integer type | tests/testthat/test-mir-function.R |
| c2mir: C source to MIR | the `c-tests` corpus by its own rules, through zumir's compile path (the prelude, no header read from a file, D6's list) plus a test-only stdio.h, stdlib.h and assert.h for the harness's printf, abort and exit: 1,069 tests (34 `.expect` outputs, 235 `.expectrc` exit statuses, exit status 0 for the rest), 1,017 passing on both tiers and the 52 others listed with their reasons in tools/c-tests-expected.txt; a broken `.expect` is reported (not vacuous); on Linux, the full profile also runs it under ASan and UBSan | tests/c/c-tests-runner.c, tools/ci/platform.sh, in platforms.yaml |
| c2mir: robustness | libFuzzer over the compile path, seeded with tests/fuzz/corpus: the same input gives the same verdict and printed module twice, and leaves no arena and no trap; before CI, every c-tests file through it under ASan and UBSan | tests/fuzz/fuzz-c2mir.c, in fuzz.yaml |
| c2mir diagnostics | an error planted at a known line and column is reported there, in the user's source, not the prelude, with blank and comment lines at the top counted; a warning before the error does not move it; no header is read from a file (stdio.h, a path, a file in the working directory), and `__has_include` agrees | tests/testthat/test-c2mir-errors.R |
| Import resolver (D6, D14, D18) | R's own exp, log, sqrt and 18 more on the same inputs, bitwise; the C ports of R's %%, %/%, min and max against R on a 22 x 22 grid of signs, zeros, non-finite values and NA, directly and through MIR; a name outside a context's list is a link error | tests/testthat/test-imports.R |
| c2mir prelude | the prelude declares exactly D6's names (both lists read); every name links and returns what the C library returns through MIR's own import, and what R's function returns where R's is the C library's; fma, ldexp, frexp, modf and the string functions against exact answers; printf, abort, exit, malloc and R's own names are link errors; the harness headers exist only in the runner's build | tests/testthat/test-prelude.R |
| `cfunction()` marshalling | a reference written in R for every example and every argument and return type | gap #9: tests/testthat/test-cfunction.R |
| `rfunction()`: R subset to native | R evaluating the same function on the same inputs, compared with `identical(num.eq = FALSE)`, which already tells `0` from `-0` and `NA` from `NaN`; value, attributes, warnings and error class asserted separately; R's side in R's AST evaluator with the JIT off (R 4.6's byte compiler can turn `-0` into `0`) | tests/testthat/helper-rfunction.R (`compare_with_r()`), used by every rfunction() test file |
| `rfunction()`, constructs | each operator, math function, loop form and logical rule against R on signed zeros, subnormals, `±Inf`, `NA`, `NaN` and empty vectors, and under a hostile session | tests/testthat/test-rfunction-ops.R |
| `rfunction()`, random functions | R on generated functions within the subset, on random inputs, seed printed and the function saved; 10² per leg, 10⁴ on the full profile; native against interpreter bitwise | tests/testthat/test-rfunction-random.R |
| `rfunction()`, refusals | each construct outside §4.6 refused with its name; each D22 rule beside R running the same function | tests/testthat/test-rfunction-refuse.R |
| `rfunction()` through the emitter | the MIR-text route: the same module printed with `MIR_output` for every construct and random function, and the same results, both routes against R | tests/testthat/test-rfunction-routes.R |
<!-- markdownlint-enable MD013 -->

## Objects

One row per object that outlives the call that made it. "Sequences" is
the test of its out-of-order use (abort then read, use after end, use
after free). The guard columns name the test, or say why the guard
cannot apply.

<!-- markdownlint-disable MD013 -->
| Object | Sequences | Fork | Unwind | Owner |
| --- | --- | --- | --- | --- |
| MIR context held by R (external pointer) | free twice, use after free, `readRDS` of a saved one, an R error between two MIR calls, a MIR error then more calls (each a `zumir_context_error`, free still works): tests/testthat/test-context-sequences.R | a forked child gets the parent's answer (`parallel::mcparallel`): same file | an R error between two MIR calls leaves the context usable: same file; MIR errors never longjmp past the entry's trap (D4) | the finalizer frees the context and its arena: `zumir_info()$memory` back to baseline after `gc()`, under gctorture too: tests/testthat/test-finalizer.R |
| Compiled function from `cfunction()`, `rfunction()` or `mir_function()` | gap #23 | gap #23 | `rfunction()`: an endless `while` and a nested `for` of 10¹² iterations stopped by `setTimeLimit()` with R's message in a child process, which then runs the function again (D23): tests/testthat/test-rfunction-errors.R; `cfunction()` and `mir_function()`: n/a, user code as in `.Call()`; errors return a status | gap #23 |
| `zumir_ctx` owned by a consumer through the C API | an error then more calls (placeholders, no crash) then compile (NULL, `ZUMIR_E_FAILED`, the message) then free, 10^3 times with RSS flat; free `NULL`; in C++ a destructor across failing calls runs once each time: tests/consumer | n/a: the consumer owns fork policy | n/a: every entry, `MIR_` shims included, returns and never longjmps (D20) | the consumer frees it with `zumir_ctx_free`; a failed one is released from its arena, never walked by `MIR_finish`: tests/consumer |
| Interpreter contexts, one per thread (D10) | 8 threads, a context each, against the serial run bitwise: tests/testthat/test-threads.R | n/a: created and freed inside one parallel region | n/a: no R call inside the region | each thread frees its own; no arena left: tests/testthat/test-threads.R |
<!-- markdownlint-enable MD013 -->

## Limits

One row per limit the package enforces. "Straddled by" is the test of
`limit - 1`, `limit` and `limit + 1` on every path that reaches it.

<!-- markdownlint-disable MD013 -->
| Limit | Unit | Straddled by |
| --- | --- | --- |
| `cfunction()` length argument against `length(x)` | elements | gap #9: tests/testthat/test-cfunction.R |
| Vector length against C `int` (`.Machine$integer.max`) | elements | `rfunction()`: `limit + 1` (an ALTREP `1:2^31`) refused before any data pointer: tests/testthat/test-rfunction-errors.R; `limit - 1` and `limit` without 16 GB, and `cfunction()`: gap #24 |
| `rfunction()` index range, 1 to `length(x)` | elements | 0, 1, `length`, `length + 1`, `NA`, `1.5`, reading and writing: tests/testthat/test-rfunction-errors.R |
| `rfunction()` `numeric()` size and `seq_len()` argument, a whole number from 0 | elements | -1, 0, 1, `NA`, 2.5 and `Inf`: tests/testthat/test-rfunction-errors.R |
| `rfunction()` integer counter arithmetic, `.Machine$integer.max` | integer value | `limit - 1`, `limit`, `limit + 1` upward and downward, and `46340L * 46340L` against `46341L * 46341L`: tests/testthat/test-rfunction-errors.R |
| `rfunction()` interrupt poll, every 2¹⁶ loop iterations (D23) | iterations | `2^16 - 1`, `2^16` and `2^16 + 1` iterations of `for` and `while`: tests/testthat/test-rfunction-errors.R |
| C API table size, read before any appended field (D21) | bytes; table version | the table's size is the header's `sizeof`; versions 0, 1 and 2 refused, served, refused: tests/consumer |
<!-- markdownlint-enable MD013 -->
