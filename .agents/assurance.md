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
| mutation      | done    | mutation.yaml      |
| fuzz          | done    | fuzz.yaml          |
| fuzz-reach    | done    | fuzz.yaml          |

Stage 1 (#3) wired `native-checks.yaml` and `gctorture.yaml` on
2026-11-13, without `sanitizers`: its instrumentation guard fails a build
with nothing to instrument, so it joined with MIR in Stage 2 (#4). Stage 3
(#5) wires
`alloc-failure`, with the error trap it exercises; Stage 8 (#8) wires
`fuzz`, with c2mir, the first parser of untrusted input that needs it;
Stage 11 (#11) schedules `mutation` over the C tests.

**Analyzers (Stage 12, 2026-11-15).** `-fanalyzer` covers zumir's own C and
gates on findings, on every pull request. Vendored MIR is not analysed:
under GCC 16 the c2mir wrapper ran the compiler out of memory, so MIR's
tree and the three wrappers that include it (`zumir_mir.c`,
`zumir_mir_gen.c`, `zumir_c2mir.c`) are in `skip` (pedrobtz/r-actions#113).
MIR's code is reached instead by ASan, UBSan, valgrind, the fuzzer and
the c-tests corpus. The first run over zumir's C reported two findings,
both false positives, and both restructured so the code reads plainly:
a NULL function in `zumir_mirf_new()` returned through a message string
the analyzer could not follow, and the trap's deliberate spin is now on a
`volatile`.

**Mutation (Stage 11, 2026-11-14).** `mutation.yaml` runs Mull, on dispatch, over
zumir's R-free C (`zumir_trap.c`, `zumir_mir_glue.c`, `zumir_mir_stdio.c`,
`zumir_imports_d6.c`) with `tests/c/test-glue.c`, built by
`tools/ci/mutation-build.sh`. In an Ubuntu 24.04 container with Mull
0.35.0 and LLVM 18, `test-glue.c`'s first version killed 65% (42 of 65);
extended against those survivors, it kills 83% (52 of 62).
It found that `zumir_trap_pop()` did not pop traps left above the one
popped, as its comment says it does, leaving the stack pointing into a
frame that had returned (fixed; no path in the package was seen to leave
one), that the arena's block count was never read, and that `exit()`
stripped newlines `zumir_trap_raise()` strips again (both removed). The
10 survivors, read one by one:

- equivalent: `fail_at > 0` as `>= 0` (`alloc_count` starts above 0);
  `n > room` and `n > 0` as `>=` in the stderr buffer (equal sizes copy
  the same); `map_errno` set on a successful `mprotect` (errno is 0
  there); `finished && ctx != NULL` as `ctx == NULL` in `zumir_mir_free()`
  (a context that failed in `zumir_mir_new()` has no mapping yet);
- unreachable through c2mir, the only caller: `zm_calloc()`'s overflow
  guard (four mutants) and its block size as `num / size`, which only a
  sanitizer would see;
- `zumir_trap_raise()`'s `n > 0` as `n >= 0`: reads the byte before the
  message, an out-of-bounds read that only ASan sees.

The C that calls R (the compilers' entries, the contexts R holds, the C
API: about 2,500 lines) is loaded by R, which mull-runner can't run; its
oracles are the R suite's and the consumer's, in the tables below.

**Hand mutation (critical review, 2026-11-15).** The check on those
oracles is a sample: 21 guards in `R/` and the C glue broken one at a time
at cf9b7b6, each against its file's tests. 12 were killed. The 9
survivors were `character()` code, a negative fixed output length, a
duplicate `out` name, the `const` scalar refusal (each tested since, #83),
an integer index below 1 and u32's upper bound (#84), the flag reset after
a compiled call (equivalent for sequential calls; the nested case is #77's
test), NA-before-NaN in `combine` (fragile: the compiler's operand order),
and `> INT_MAX` as `>=` (gap #24). Re-run the sample after a large change
to R/ or the glue.

**Schedules (2026-11-15).** #53 removed every `schedule:` before the
repository went private. What these notes call the full set now runs on a
dispatch on main's head, which the readiness and release commits always
get (ci.md); there is no weekly run.

**Unverified architectures.** `src/zumir_mir.h` compiles MIR in on
ppc64, s390x and riscv64 Linux, which MIR supports, but no CI leg builds
or tests them (#87): they are unverified, not supported, until one does.

**Gap lists (Stage 11, 2026-11-14).** Native coverage
(`coverage.yaml`) and fuzz reach (`fuzz.yaml`, 75.4% of the lines of its
zumir sources; `reach-floor: 70`) were read. What they showed and what
was done:

- `zumir_api.c`, the C API, at 13% of lines: its tests are
  `tests/consumer`'s, in `platforms.yaml`, which the coverage job does not
  run. Measured as untested; tested by the consumer on every runner.
- `bad_operand()` in the emitter never entered: now 8 malformed operands,
  each a `zumir_mir_error` on both tiers
  (tests/testthat/test-mir-compile-routes.R).
- `zumir_test_math()` and `zumir_ctx_call_d()`, entries no test called:
  removed.
- fuzz reach: what no input enters is test hooks and the stderr paths,
  which the harness's inputs (C source) cannot reach;
  `tests/c/test-glue.c` covers them.

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
| Error trap: invalid modules | MIR's own error type (syntax, operand mode, undeclared reference) as a `zumir_mir_error` each time, the context finished and no trap left; in a child process 10³ failures of each kind all caught, the session alive and RSS growth under 16 MB for the second 10³ (a leaked context per failure is about 97 MB) | tests/testthat/test-error-trap.R |
| Allocation failure inside MIR | each of MIR's first 260 allocations failed in turn through zumir's allocator hook: a `zumir_mir_error` of `MIR_alloc_error` or the module's own type every time, never a crash, and no arena left (`zumir_test_live_arenas`); the `alloc-failure` sweep fails R's allocations too, in every compiler and at the interpreter's first call | tests/testthat/test-error-trap.R; alloc-failure.yaml |
| zumir's R-free C: the trap, MIR's stderr and exit, the arena, code generation, D6 | the trap's stack (nesting, an unpopped trap, a raise with none), the stderr buffer byte for byte, `exit` as a raise, the arena's live counts through a context's life and through every allocation failure in turn, MIR's and c2mir's, on both tiers, `hyp(3, 4)` compiled by c2mir equal to 7, D6's names against the design's list and its pointers against the platform's functions; under ASan and UBSan on Linux; and under Mull | tests/c/test-glue.c, in tools/ci/platform.sh and mutation.yaml |
| Error trap on threads | each of two threads gets its own 500 errors, with its own MIR error type, and leaves no trap behind; under ASan and UBSan on Linux | tests/c/test-trap-threads.c, in tools/ci/platform.sh |
| MIR's exit, abort and stderr | what MIR writes to stderr reaches zumir's buffer, a caller's FILE is untouched, exit and abort land in the trap or, with none, become an R error | tests/testthat/test-mir-stdio.R |
| Executable memory refused (Linux) | under a seccomp filter in a child R process: tier `"none"`, `compile` false on both tiers with MIR's code-memory error and `EACCES`, and the process alive | tests/testthat/test-tier.R, on ubuntu-latest (R CMD check) and ubuntu-24.04-arm (platform.sh) |
| Tier probe | each tier's four capabilities, `arith` against `x * 2 + 1` and `ext` against libm's `exp`, bitwise; `zumir_tier()` from them | tests/testthat/test-tier.R |
| C API table v1 | a consumer package that links it, emits three right-hand sides, and compares them with C; links no CRAN-flagged symbol; the header compiles alone as C99 and C++11 with -pedantic -Werror | tests/consumer; tools/ci/platform.sh |
| C API compatibility (D21) | a unit built against the frozen v1 header runs; another MIR ABI number or a newer version gets `NULL` and a reason | tests/consumer (src/frozen/zumir_v1.h); tests/testthat/test-imports.R |
| Generated code's memory accesses | valgrind with `--smc-check=all` on the native tier, ASan on the interpreter tier, over the `mir_compile()` and `cfunction()` suites | `mir_compile()`: valgrind with `--smc-check=all` (native-checks.yaml) and ASan over the suite, whose random and route tests run both tiers; `cfunction()`: the same, over tests/testthat/test-cfunction.R, which runs both tiers |
| Double-expression emitter | the three right-hand sides written with it, bitwise against C; each comparison and branch helper against C on every pair of -Inf, -1, -0, 0, 1, Inf, NaN, NA | tests/consumer |
| Consumer imports | an import resolves in the context that registered it, is a link error in one that did not, and is not under D6's list (`mir_function()`) | tests/consumer; through `cfunction()`, a consumer's `expit` and `mir_compile()`'s `R_pow` and ports are link errors: tests/testthat/test-cfunction.R |
| zupmxode's models through the C API | the IR zupmxode 5f637a1 compiles for two library models and a branching one, lowered by a copy of its lowering, against a copy of its reference evaluator, every block, bitwise (NaN by position), on both tiers; the two-compartment right-hand side against R; regenerated by tools/zpx-fixture.R | tests/consumer/tests/testthat/test-zupmxode.R |
| `mir_function()`: MIR text in | MIR's 17 `mir-tests/*.mir` read and written back to a fixed point; hand-computed values of test1, test2, test6 and test7 on both tiers; argument limits straddled per integer type, i8 to u64, i64 at ±2⁵³; a name defined in two modules refused; a user's export named like a D6 import kept; a jump to a label not in the function a MIR error on both tiers | tests/testthat/test-mir-function.R |
| c2mir: C source to MIR | the `c-tests` corpus by its own rules, through zumir's compile path (the prelude, no header read from a file, D6's list) plus a test-only stdio.h, stdlib.h and assert.h for the harness's printf, abort and exit: 1,069 tests (34 `.expect` outputs, 235 `.expectrc` exit statuses, exit status 0 for the rest), 1,017 passing on both tiers and the 52 others listed with their reasons in tools/c-tests-expected.txt; a broken `.expect` is reported (not vacuous); on Linux, the full profile also runs it under ASan and UBSan | tests/c/c-tests-runner.c, tools/ci/platform.sh, in platforms.yaml |
| c2mir: robustness | libFuzzer over the compile path, seeded with tests/fuzz/corpus (with the bug hunt's 80 generator-crash programs): the same input gives the same verdict and printed module twice, and the same verdict once more generated natively, and leaves no arena and no trap; before CI, every c-tests file through it under ASan and UBSan; c2mir's own parser is not in the reach table yet (#95) | tests/fuzz/fuzz-c2mir.c, in fuzz.yaml |
| c2mir diagnostics | an error planted at a known line and column is reported there, in the user's source, not the prelude, with blank and comment lines at the top counted; a warning before the error does not move it; no header is read from a file (stdio.h, a path, a file in the working directory), and `__has_include` agrees | tests/testthat/test-c2mir-errors.R |
| Import resolver (D6, D14, D18) | R's own exp, log, sqrt and 18 more on the same inputs, bitwise; the C ports of R's %%, %/%, min and max against R on a 22 x 22 grid of signs, zeros, non-finite values and NA, directly and through MIR; a name outside a context's list is a link error | tests/testthat/test-imports.R |
| c2mir prelude | the prelude declares exactly D6's names (both lists read); every name links and returns what the C library returns through MIR's own import, and what R's function returns where R's is the C library's; fma, ldexp, frexp, modf and the string functions against exact answers; printf, abort, exit, malloc and R's own names are link errors; the harness headers exist only in the runner's build. The one- and two-argument names' reference is `mir_function()` through the same resolver: it checks the prototypes, and tests/c/test-glue.c the bindings. The C11 cases the review found are checked against the platform's cc: `_Bool` conversions, floating to `unsigned long long` from 2⁶³, `float.h`, `NULL`, `'a'`, `_Generic`, `#if` `?:`, and a narrow bit-field on both tiers | tests/testthat/test-prelude.R |
| `cfunction()` marshalling | a reference written in R for every example and every argument and return type; arguments in order around outputs; outputs as copies (D24: the caller's vector bitwise unchanged, a second name bound to it unchanged); zero-filled outputs; the value and outputs as a named list; each scalar and vector checked and converted as `as.integer()` and `as.double()` would (a factor, `Date`, `POSIXct` or `difftime` refused for a scalar and for a pointer, logical, `Inf`, `NaN`, `±2^31`, NA into `const int *` and back as NA, a given output returned without its attributes), in C since Stage 10; the signature read from c2mir's preprocessed source, so a decoy in a comment, an `#if 0` branch, a string or a `#define const` is never read (the caller's vector checked unchanged) | tests/testthat/test-cfunction.R |
| `mir_compile()`: R subset to native | R evaluating the same function on the same inputs, compared with `identical(num.eq = FALSE)`, which already tells `0` from `-0` and `NA` from `NaN`; value, attributes, warnings and error class asserted separately; R's side in R's AST evaluator with the JIT off (R 4.6's byte compiler can turn `-0` into `0`) | tests/testthat/helper-mir-compile.R (`compare_with_r()`), used by every mir_compile() test file |
| `mir_compile()`, constructs | each operator, math function, loop form and logical rule against R on signed zeros, subnormals, `±Inf`, `NA`, `NaN` and empty vectors, and under a hostile session | tests/testthat/test-mir-compile-ops.R |
| `mir_compile()`, random functions | R on generated functions within the subset, on random inputs, seed printed and the function saved; 10² per leg, 10⁴ on the full profile (`full_profile()` resolves r-actions' `auto`); with floors on the share compiled and the share where R returned a value; the generator also draws an empty branch, an empty result block, an extra argument, an attributed logical and a double index; native against interpreter bitwise | tests/testthat/test-mir-compile-random.R |
| `mir_compile()`, refusals | each construct outside §4.6 refused with its name; each D22 rule beside R running the same function; one function per refusal site of R/mir_compile.R (54); every construct's argument count against R's | tests/testthat/test-mir-compile-refuse.R |
| `mir_compile()` through the emitter | the MIR-text route: the same module printed with `MIR_output` for every construct and random function, and the same results, both routes against R | tests/testthat/test-mir-compile-routes.R |
| Warnings across nested compiled calls | a compiled function called inside another's call (from the poll, as a finalizer runs) keeps the outer call's NaN and fmod flags: `zumir_test_nested_flags` sets them as the outer call would and runs R code that calls a compiled function | tests/testthat/test-mir-compile-errors.R |
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
| Compiled function from `cfunction()`, `mir_compile()` or `mir_function()` | `rm()` and `gc()` then a call, and a closure restored by `readRDS` (a `zumir_context_error`), for all three: tests/testthat/test-closures.R | a forked child gets the parent's answer, and the parent still does (`parallel::mcparallel`), for all three: tests/testthat/test-closures.R | `mir_compile()`: an endless `while` and a nested `for` of 10¹² iterations stopped by `setTimeLimit()` with R's message in a child process, which then runs the function again (D23): tests/testthat/test-mir-compile-errors.R; `cfunction()` and `mir_function()`: n/a, user code as in `.Call()`; errors return a status | the closure's body holds the context, which its finalizer frees once the closure is collected: tests/testthat/test-closures.R and test-finalizer.R |
| `zumir_ctx` owned by a consumer through the C API | an error then more calls (placeholders, no crash) then compile (NULL, `ZUMIR_E_FAILED`, the message) then free, 10^3 times with RSS flat; free `NULL` (a second free is undefined, as the header says); an invalid opcode and six NULL arguments each a status, on R's thread and another; a second compile from a module with exports; a not-found link then the other tier refused; in C++ a destructor across failing calls runs once each time; under ASan and UBSan on Linux (full profile): tests/consumer, tools/ci/platform.sh | n/a: the consumer owns fork policy | n/a: every entry, `MIR_` shims included, returns and never longjmps (D20) | the consumer frees it with `zumir_ctx_free`; a failed one is released from its arena, never walked by `MIR_finish`: tests/consumer |
| zumir's DLL with live compiled functions | unload the namespace with a function alive: the DLL kept, `gc()` and exit clean, reloaded and called again, the context count back to its baseline: tests/testthat/test-load.R (child process) | n/a: the DLL is the process's; a forked child inherits it mapped | n/a: unloading runs no MIR call and no R error can stop it midway | `.onUnload` unloads the DLL only when no context is alive |
| An interpreted function's first call | each compiler's function, and the consumer's, called under an allocation failure at each of its first 4 allocations: the right answer every time, nothing allocated: tests/testthat/test-error-trap.R, tests/consumer; `alloc-failure`'s `interp_call` section | n/a: prepared at link, so a forked child's first call allocates nothing either | n/a: the call raises nothing; its code, call interfaces and arrays are made at link, under the trap | the context, as above |
| Interpreter contexts, one per thread (D10) | 8 threads, a context each, against the serial run bitwise: tests/testthat/test-threads.R | n/a: created and freed inside one parallel region | n/a: no R call inside the region | each thread frees its own; no arena left: tests/testthat/test-threads.R |
<!-- markdownlint-enable MD013 -->

## Limits

One row per limit the package enforces. "Straddled by" is the test of
`limit - 1`, `limit` and `limit + 1` on every path that reaches it.

<!-- markdownlint-disable MD013 -->
| Limit | Unit | Straddled by |
| --- | --- | --- |
| `cfunction()` length argument against `length(x)` | elements | `n` at `length - 1`, `length`, `length + 1`, and negative; a supplied output one shorter than `n`, and longer (its tail kept); inputs of unequal length without `n`: tests/testthat/test-cfunction.R |
| Vector length against C `int` (`.Machine$integer.max`) | elements | `mir_compile()`: `limit + 1` (an ALTREP `1:2^31`) refused before any data pointer: tests/testthat/test-mir-compile-errors.R; `cfunction()`: `limit + 1` (the same sequence) refused, with and without `n`: tests/testthat/test-cfunction.R; `limit - 1` and `limit` without 16 GB: gap #24 |
| `mir_compile()` index range, 1 to `length(x)` | elements | 0, 1, `length`, `length + 1`, `NA`, `1.5`, reading and writing: tests/testthat/test-mir-compile-errors.R |
| `mir_compile()` `numeric()` size and `seq_len()` argument, a whole number from 0 | elements | -1, 0, 1, `NA`, 2.5 and `Inf`: tests/testthat/test-mir-compile-errors.R |
| `mir_compile()` integer counter arithmetic, `.Machine$integer.max` | integer value | `limit - 1`, `limit`, `limit + 1` upward and downward, and `46340L * 46340L` against `46341L * 46341L`: tests/testthat/test-mir-compile-errors.R |
| `mir_compile()` interrupt poll, every 2¹⁶ loop iterations (D23) | iterations | `2^16 - 1`, `2^16` and `2^16 + 1` iterations of `for` and `while` give R's sums: tests/testthat/test-mir-compile-errors.R; when the poll fires is not observed there |
| MIR's stderr buffer: 1023 bytes a write, 4095 in all | bytes | writes of 1023, 1024 and 1025 bytes then one more, and six writes of 1999: tests/c/test-glue.c |
| Trap message, 1023 bytes | bytes | a 2999-byte message cut to 1023: tests/c/test-glue.c |
| `cfunction()` slots on the stack, under 16 parameters | parameters | 15, 16 and 17 parameters, every argument in its place: tests/testthat/test-cfunction.R |
| C API table size, read before any appended field (D21) | bytes; table version | the table's size is the header's `sizeof`; versions 0, 1 and 2 refused, served, refused: tests/consumer |
| Emitter: 32 distinct calls, 4 arguments a call | calls; arguments | 32 calls and 31 plus a two-argument one compile and give C's sum; a 33rd fails the context; 4 arguments compile, 5 fail it: tests/consumer |
| Interpreter call arguments (the call arrays) | arguments | 94, 95, 96 and 300 arguments on the interpreter, against R's sum, the context then freed: tests/testthat/test-mir-function.R (child process) |
| `cfunction()` fixed output length, 0 to `.Machine$integer.max` | elements | -1, 1.5, `NA` and `limit + 1` refused; 0 and 3 written: tests/testthat/test-cfunction.R |
| `mir_function()` u32, i64 and u64 arguments | integer value | u32 at 0, 2³² - 1 and 2³²; i64 at ±2⁵³ and 2⁵³ + 2; u64 at 0, 2⁵³ and -1: tests/testthat/test-mir-function.R |
<!-- markdownlint-enable MD013 -->
