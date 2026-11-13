# The R suite as CI runs it, against the installed package: run from the
# package root after `R CMD INSTALL .`, as `Rscript tools/ci-run-tests.R`.
# A file, not `Rscript -e`, because a multi-line `-e` dies on Windows before
# it runs anything.
#
# The tests run in the package's namespace, as under R CMD check, so internal
# functions are visible to them.
#
# Shuffled (the definition of done's `devtools::test(shuffle = TRUE)`, on every
# run): the tests in each file run in a random order, so one that leans on
# another's leftovers fails here. The seed is printed; ZUMIR_TEST_SEED replays
# an order.
library(testthat)
library(zumir)
seed <- suppressWarnings(as.integer(Sys.getenv("ZUMIR_TEST_SEED")))
if (is.na(seed)) {
  seed <- as.integer(Sys.time()) %% 100000L
}
cat("test order seed:", seed, "(ZUMIR_TEST_SEED to replay)\n")
set.seed(seed)
test_dir(
  "tests/testthat",
  package = "zumir",
  load_package = "installed",
  reporter = "summary",
  stop_on_failure = TRUE,
  shuffle = TRUE
)
