# MIR is compiled in where it has a code generator for the CPU, and left out
# elsewhere (design §4.2 "Other CPUs", §7, D25). ZUMIR_NO_MIR=1 at build time
# forces it out; tools/ci/platform.sh builds and tests the package that way
# on Linux. Stages 4, 5, 6 and 9 extend this file to each entry, with its
# classed error.

test_that("MIR is compiled in exactly where the build should have it", {
  expect_identical(.Call(zumir_has_target), expected_mir_target())
})

test_that("the expected target follows the platform table", {
  # The helper decides the test above, so it is held to design §7 here.
  expect_true(expected_mir_target("x86_64", "unix", FALSE))
  expect_true(expected_mir_target("aarch64", "unix", FALSE))
  expect_true(expected_mir_target("arm64", "unix", FALSE))
  expect_true(expected_mir_target("x86_64", "windows", FALSE))
  expect_false(expected_mir_target("aarch64", "windows", FALSE))
  expect_false(expected_mir_target("i686", "unix", FALSE))
  expect_false(expected_mir_target("armv7l", "unix", FALSE))
  expect_false(expected_mir_target("x86_64", "unix", TRUE))
})
