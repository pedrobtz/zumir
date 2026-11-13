# zumir 0.0.0.9000

* Initial CRAN submission.

* Bundles MIR, Vladimir Makarov's JIT compiler (core, code generator and
  interpreter), at upstream commit `dc2c763`, with six local patches that
  make it build without warnings under R's compilers. MIR can no longer
  end the R session or print from it: its exits and messages go to
  zumir. On a CPU MIR has no code generator for, and on Windows on ARM,
  the package installs without it. There are no R functions yet.
