# Whether this build should have compiled MIR in (src/zumir_mir.h): on the
# CPUs MIR targets, except Windows aarch64 (D25), and not in a build made
# with ZUMIR_NO_MIR=1, which tools/ci/platform.sh announces by setting
# ZUMIR_EXPECT_NO_MIR=1 when it runs the tests.
expected_mir_target <- function(
  arch = R.version$arch,
  os = .Platform$OS.type,
  forced_out = Sys.getenv("ZUMIR_EXPECT_NO_MIR") == "1"
) {
  if (forced_out) {
    return(FALSE)
  }
  arch <- sub("^arm64$", "aarch64", arch)
  if (os == "windows") {
    return(arch == "x86_64")
  }
  arch %in% c("x86_64", "aarch64", "powerpc64le", "ppc64le", "s390x", "riscv64")
}
