# This file is part of the standard setup for testthat.
# It is recommended that you do not modify it.
#
# Where should you do additional test configuration?
# Learn more about the roles of various files in:
# * https://r-pkgs.org/testing-design.html#sec-tests-files-overview
# * https://testthat.r-lib.org/articles/special-files.html

library(testthat)
library(zumir)

# Shuffled within each file, as tools/ci-run-tests.R runs them, so an order
# dependence shows on every R CMD check leg, not on one platform. The seed
# is printed in testthat.Rout; ZUMIR_TEST_SEED replays an order.
seed <- suppressWarnings(as.integer(Sys.getenv("ZUMIR_TEST_SEED")))
if (is.na(seed)) {
  seed <- as.integer(Sys.time()) %% 100000L
}
cat("test order seed:", seed, "(ZUMIR_TEST_SEED to replay)\n")
set.seed(seed)
test_check("zumir", shuffle = TRUE)
