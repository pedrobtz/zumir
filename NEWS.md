# zumir 0.0.0.9000

* Initial CRAN submission.

* Bundles MIR, Vladimir Makarov's JIT compiler (core, code generator and
  interpreter), at upstream commit `dc2c763`, with six local patches that
  make it build without warnings under R's compilers. MIR can no longer
  end the R session or print from it: its exits and messages go to
  zumir. On a CPU MIR has no code generator for, and on Windows on ARM,
  the package installs without it.

* `mir_function()` compiles a function from MIR's textual form, natively or
  on MIR's interpreter, and returns an R function that calls it.

* Packages can build and compile MIR code through zumir's C API: put
  `LinkingTo: zumir` in DESCRIPTION and include `zumir.h`. MIR's errors
  are reported through the context, never by ending the R session.

* `zumir_info()` reports the vendored MIR and its patches, which tiers run
  in this session and why the others do not, and the memory MIR contexts
  hold. `zumir_tier()` is the default tier: `"native"`, `"interp"`, or
  `"none"` where the system refuses writable and executable memory.
