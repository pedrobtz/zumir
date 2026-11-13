# Tier detection (design §4.4, Q5 (c)): the load-time probe, its four
# capabilities per tier, and the probe under refused W+X memory.

test_that("the probe reports four capabilities for each of two tiers", {
  caps <- zumir_info()$capabilities
  expect_s3_class(caps, "data.frame")
  expect_identical(caps$tier, rep(c("native", "interp"), each = 4))
  expect_identical(
    caps$capability,
    rep(c("compile", "entry", "run", "external"), 2)
  )
  expect_type(caps$ok, "logical")
  # A reason exactly where a capability is missing.
  expect_identical(is.na(caps$reason), caps$ok)
})

test_that("with MIR and executable memory, both tiers run, native first", {
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  caps <- zumir_info()$capabilities
  expect_true(all(caps$ok), label = paste(caps$reason, collapse = "; "))
  expect_identical(zumir_tier(), "native")
})

test_that("zumir_tier() follows the capabilities", {
  caps <- zumir_info()$capabilities
  tier <- zumir_tier()
  expect_true(tier %in% c("native", "interp", "none"))
  native <- all(caps$ok[caps$tier == "native"])
  interp <- all(caps$ok[caps$tier == "interp"])
  want <- if (native) {
    "native"
  } else if (interp) {
    "interp"
  } else {
    "none"
  }
  expect_identical(tier, want)
})

test_that("W+X memory refused: no tier, a reason, and no signal (Linux)", {
  skip_on_os(c("windows", "mac", "solaris"))
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  lib <- installed_lib()
  skip_if(is.null(lib), "zumir is not installed: run under R CMD check")
  res <- run_child(
    lib,
    c(
      "library(zumir)",
      "cat('before:', zumir_tier(), '\\n')",
      "bites <- .Call(zumir:::zumir_test_deny_wx)",
      "cat('filter:', bites, '\\n')",
      "zumir:::probe_tiers()",
      "info <- zumir_info()",
      "cat('after:', info$tier, '\\n')",
      "for (i in seq_len(nrow(info$capabilities))) with(info$capabilities[i, ],",
      "  cat('cap:', tier, capability, ok, reason, '\\n'))",
      "cat('alive\\n')"
    )
  )
  out <- res$output
  # The filter is the test's precondition, not the behaviour under test: a
  # host that forbids seccomp filters cannot run this check.
  skip_if(!("filter: TRUE " %in% out), "cannot install a seccomp filter here")
  expect_identical(res$status, 0L)
  expect_true("before: native " %in% out)
  expect_true("after: none " %in% out)
  expect_true("alive" %in% out)
  caps <- grep("^cap:", out, value = TRUE)
  expect_length(caps, 8L)
  compile <- grep("^cap: (native|interp) compile FALSE", caps, value = TRUE)
  expect_length(compile, 2L)
  # MIR's code-memory error (the fork's D11 patch), with the errno.
  expect_true(
    all(grepl("code memory", compile)),
    label = paste(compile, collapse = "\n")
  )
  expect_true(
    all(grepl("Permission denied", compile)),
    label = paste(compile, collapse = "\n")
  )
})
