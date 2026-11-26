## Submission

This is a new submission.

## What the package does at run time

zumir generates machine code at run time. It vendors MIR
(<https://github.com/vnmakarov/mir>, MIT), a small just-in-time compiler,
and compiles numeric functions in memory: C source given to `cfunction()`,
an R function given to `mir_compile()`, MIR's own intermediate code given
to `mir_function()`, or code that another package builds through zumir's
C interface (`LinkingTo: zumir`). This lets users and packages get native
code with no system compiler installed.

CRAN already has a package that does this: `luajr` generates machine code
at run time with LuaJIT.

* No code is downloaded. What is compiled is only what the user, or a
  package calling zumir, passes in the session.
* The code is written to memory that is writable and executable. When
  zumir loads, it checks once that the system allows such memory. Where it
  does not (SELinux `deny_execmem`, some seccomp policies), nothing is
  compiled, `zumir_tier()` returns `"none"`, and every compiler returns an
  R error. The process is never ended. On macOS this relies on the
  `com.apple.security.cs.allow-jit` entitlement that CRAN's R build has.
* Compiled code can call only a fixed list of functions: the C maths
  library, `memcpy`, `memmove`, `memset`, `memcmp` and `strlen`. Any other
  name is a link error. `cfunction()` reads no header file.
* The vendored library writes nothing to `stdout` or `stderr` and never
  calls `exit` or `abort`. Its output goes into the message of an R
  condition. The patches that do this are listed in
  `src/vendor/PROVENANCE`, and CI checks the built library for those
  symbols.
* Examples and tests that compile skip where `zumir_tier()` is `"none"`.

## Test environments

The checks run in CI through `pedrobtz/r-actions`, which replaces
win-builder, the macOS builder and R-hub:

* `R CMD check --as-cran` on macOS (R release), Windows (R release and
  R-devel) and Ubuntu (R release and oldrel-1).
* R-devel in CRAN-like Linux containers: `clang23`, `ubuntu-clang` and
  `ubuntu-gcc16`.
* Without suggested packages (`nosuggests`) and without long doubles
  (`nold`).
* ASan and UBSan, valgrind, LTO, rchk, gctorture and compiler static
  analysers.

## R CMD check results

0 errors | 0 warnings | 1 note

* This is a new submission.

## Method References

There are no published references describing the methods in this
package. MIR itself is described in its repository
(<https://github.com/vnmakarov/mir>), and zumir's own design follows no
publication.
