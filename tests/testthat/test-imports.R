# The import resolver (design D6, D14, D18) and zumir's C ports of R's
# arithmetic.

test_that("each D6 math function returns what R's own function returns, bitwise", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  # R's functions that are the C library's (R's log2, log10, round and the
  # gamma functions are R's own code, and are not compared).
  same <- list(
    exp = exp,
    log = log,
    log1p = log1p,
    expm1 = expm1,
    sqrt = sqrt,
    sin = sin,
    cos = cos,
    tan = tan,
    asin = asin,
    acos = acos,
    atan = atan,
    sinh = sinh,
    cosh = cosh,
    tanh = tanh,
    asinh = asinh,
    acosh = acosh,
    atanh = atanh,
    floor = floor,
    ceil = ceiling,
    trunc = trunc,
    fabs = abs
  )
  x <- inputs()
  for (fn in names(same)) {
    f <- mir_function(call1_text(fn), "f")
    want <- suppressWarnings(same[[fn]](x))
    got <- vapply(x, f, 0)
    expect_identical(got, want, info = fn)
  }
  a2 <- mir_function(call2_text("atan2"), "f")
  y <- rev(inputs())
  expect_identical(mapply(a2, x, y), atan2(x, y))
})

test_that("a name outside the context's list is a link error", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  for (fn in c("R_pow", "zumir_r_fmod", "printf", "Rf_error", "system")) {
    expect_error(
      mir_function(call1_text(fn), "f"),
      class = "zumir_mir_error",
      info = fn
    )
  }
  # rfunction()'s list has R_pow and the ports, and not memcpy.
  pw <- build_mir_function(call2_text("R_pow"), "f", zumir_tier(), imports = 2L)
  expect_identical(pw(2, 0.5), 2^0.5)
  expect_identical(pw(NA_real_, 0), NA_real_^0)
  expect_error(
    build_mir_function(call1_text("memcpy"), "f", zumir_tier(), imports = 2L),
    class = "zumir_mir_error"
  )
})

test_that("the ports of %% and %/% equal R's, called directly and through MIR", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  g <- port_grid()
  # R warns about loss of accuracy in some %% cases; the values are what
  # is compared.
  want_mod <- suppressWarnings(g$x %% g$y)
  want_div <- g$x %/% g$y
  expect_identical(.Call(zumir_test_port, "fmod", g$x, g$y), want_mod)
  expect_identical(.Call(zumir_test_port, "intdiv", g$x, g$y), want_div)
  fmod <- build_mir_function(
    call2_text("zumir_r_fmod"),
    "f",
    zumir_tier(),
    imports = 2L
  )
  intdiv <- build_mir_function(
    call2_text("zumir_r_intdiv"),
    "f",
    zumir_tier(),
    imports = 2L
  )
  expect_identical(mapply(fmod, g$x, g$y), want_mod)
  expect_identical(mapply(intdiv, g$x, g$y), want_div)
})

test_that("the two-argument min and max ports equal R's, NA and NaN in either order", {
  skip_if(zumir_tier() == "none", "no tier runs here")
  g <- port_grid()
  want_min <- mapply(min, g$x, g$y)
  want_max <- mapply(max, g$x, g$y)
  expect_identical(.Call(zumir_test_port, "min2", g$x, g$y), want_min)
  expect_identical(.Call(zumir_test_port, "max2", g$x, g$y), want_max)
  # NA beats NaN whichever comes first, as in R.
  expect_true(
    is.na(.Call(zumir_test_port, "min2", NaN, NA_real_)) &&
      !is.nan(.Call(zumir_test_port, "min2", NaN, NA_real_))
  )
  expect_true(
    is.na(.Call(zumir_test_port, "max2", NA_real_, NaN)) &&
      !is.nan(.Call(zumir_test_port, "max2", NA_real_, NaN))
  )
  mn <- build_mir_function(
    call2_text("zumir_r_min2"),
    "f",
    zumir_tier(),
    imports = 2L
  )
  expect_identical(mapply(mn, g$x, g$y), want_min)
})

test_that("the C API getter refuses a newer version and another MIR ABI", {
  expect_identical(
    .Call(zumir_test_api_get, 1L, 1L),
    if (.Call(zumir_has_target)) "" else "zumir was built without MIR"
  )
  skip_if_not(.Call(zumir_has_target), "built without MIR")
  expect_match(.Call(zumir_test_api_get, 2L, 1L), "newer C API version")
  expect_match(.Call(zumir_test_api_get, 0L, 1L), "newer C API version")
  expect_match(.Call(zumir_test_api_get, 1L, 2L), "another MIR ABI")
  expect_match(.Call(zumir_test_api_get, 1L, 0L), "another MIR ABI")
})
