# zumir (development version)

# zumir 0.1.0

* Initial CRAN submission.

* Bundles MIR, Vladimir Makarov's JIT compiler (core, code generator,
  interpreter and its C compiler `c2mir`), at upstream commit `dc2c763`,
  with local patches that make it build cleanly under R's compilers and
  remove the undefined behaviour and memory errors its own C test corpus
  found under the sanitizers. MIR can no longer end the R session or
  print from it: its exits and messages go to zumir. `c2mir` reads no
  header file: C code compiles the same on every machine. On a CPU MIR has no code generator for, and on Windows on ARM,
  the package installs without it.

* `cfunction()` compiles one C function, in memory, with MIR's own C
  compiler, and returns an R function that calls it: no Rtools, Xcode
  tools or `gcc` needed. Output arguments are copies, so the caller's
  vectors are never changed, and no system header is read, so a source
  compiles the same everywhere. Arguments are checked and passed in C, so
  a call costs under a microsecond.

* `mir_function()` compiles a function from MIR's textual form, natively or
  on MIR's interpreter, and returns an R function that calls it.

* `mir_compile()` compiles a numeric R function, in a subset of R, to native
  code that returns exactly what R returns, warnings and errors included.
  Where matching R is not cheap it refuses, at compile time or at the
  call, with a classed error. Compiled loops stop for Ctrl-C and
  `setTimeLimit()`.

* Packages can build and compile MIR code through zumir's C API: put
  `LinkingTo: zumir` in DESCRIPTION and include `zumir.h`. MIR's errors
  are reported through the context, never by ending the R session.

* A critical review before release (#54 to #108) fixed what it found, each
  with a test:
  - in the vendored MIR: a code generator crash, an interpreter overflow
    on calls of 95 or more arguments, labels used but never placed,
    `_Bool` and `unsigned long long` conversions, `float.h`, `NULL`,
    character constant types, `_Generic`, `#if`'s `?:` and bit-field reads;
  - `mir_compile()` refuses an empty result block and calls with argument
    counts R refuses, where it gave a value;
  - `cfunction()` reads its signature after the preprocessor, so a comment
    or `#if 0` can no longer make it write the caller's vector; it accepts
    functions of no parameters and refuses factors and dates for pointers;
  - an interpreted function's first call no longer allocates, so it cannot
    fail outside zumir's error handling; the C API turns every misuse it
    can detect into a status; unloading zumir with a compiled function
    alive no longer crashes R.

* `zumir_info()` reports the vendored MIR and its patches, which tiers run
  in this session and why the others do not, and the memory MIR contexts
  hold. `zumir_tier()` is the default tier: `"native"`, `"interp"`, or
  `"none"` where the system refuses writable and executable memory.
