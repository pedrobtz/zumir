# zumir's C API through a consumer package, as zupmxode will use it (design
# §4.3; a stand-in for the zupmxode trial, roadmap Stage 5).

models <- function() {
  list(
    list(neq = 2, npar = 3),
    list(neq = 3, npar = 6),
    list(neq = 10, npar = 9)
  )
}

test_that("the three right-hand sides equal C's, bitwise, on both tiers", {
  skip_if(zumir::zumir_tier() == "none", "no tier runs here")
  set.seed(20261113)
  for (tier in c("native", "interp")) {
    for (k in 1:3) {
      m <- models()[[k]]
      for (i in 1:40) {
        y <- runif(m$neq, 0, 100)
        par <- runif(m$npar, 0.1, 50)
        r <- zumirconsumer:::zc_rhs(k, tier, y, par)
        expect_identical(r$tier, tier)
        expect_identical(r$mir, r$c, info = paste(tier, "model", k, "draw", i))
      }
    }
  }
})

test_that("each comparison and branch helper agrees with C on NaN and Inf", {
  skip_if(zumir::zumir_tier() == "none", "no tier runs here")
  v <- c(-Inf, -1, -0, 0, 1, Inf, NaN, NA)
  g <- expand.grid(a = v, b = v)
  for (what in c("cmp", "branch", "branch_true")) {
    for (code in c("eq", "ne", "lt", "le", "gt", "ge")) {
      r <- zumirconsumer:::zc_cmp(what, code, g$a, g$b)
      expect_identical(r$mir, r$c, info = paste(what, code))
    }
  }
})

test_that("an error mid-module: no effect, no crash, NULL from compile, 10^3 times", {
  skip_if(zumir::zumir_tier() == "none", "no tier runs here")
  r <- zumirconsumer:::zc_errors(1000)
  expect_identical(r$null_compile, 1000L)
  expect_identical(r$recorded, 1000L)
  # zumir_compile on a failed context reports it as failed.
  expect_identical(r$failed_status, 1000L)
  # Later calls returned placeholders, not NULL.
  expect_identical(r$placeholders, 1000L)
  # zumir_status() has MIR's message.
  expect_identical(r$message, 1000L)
})

test_that("RSS stays flat across 10^3 failed contexts", {
  skip_if(zumir::zumir_tier() == "none", "no tier runs here")
  rss <- function() {
    gc()
    .Call(zumir:::zumir_test_rss)
  }
  zumirconsumer:::zc_errors(1000)
  r1 <- rss()
  zumirconsumer:::zc_errors(1000)
  r2 <- rss()
  skip_if(is.na(r1), "RSS cannot be read here")
  # A leaked context per failure is about 97 KB, 97 MB per 10^3.
  expect_lt(r2 - r1, 16 * 1024^2)
  expect_identical(zumir::zumir_info()$memory$contexts, 0)
})

test_that("from C++, a destructor across failing calls runs exactly once each time", {
  skip_if(zumir::zumir_tier() == "none", "no tier runs here")
  expect_identical(zumirconsumer:::zc_errors_cpp(1000), 1000L)
})

test_that("a unit built against the frozen v1 header runs against this zumir", {
  skip_if(zumir::zumir_tier() == "none", "no tier runs here")
  expect_identical(zumirconsumer:::zc_frozen(2.5), 2.5 * 3 - 1)
})

test_that("the table's size and version are the header's; versions straddled", {
  t <- zumirconsumer:::zc_table()
  # size covers exactly the fields this header declares.
  expect_identical(t[1], t[2])
  expect_identical(t[3:4], c(1, 1))
  # Version limit - 1, limit, limit + 1: 0 is refused, 1 served, 2 refused.
  expect_match(zumirconsumer:::zc_getter(0, 1), "newer C API version")
  expect_identical(zumirconsumer:::zc_getter(1, 1), "")
  expect_match(zumirconsumer:::zc_getter(2, 1), "newer C API version")
})

test_that("another MIR ABI, or a newer table version, gets NULL and a reason", {
  expect_identical(zumirconsumer:::zc_getter(1, 1), "")
  expect_match(zumirconsumer:::zc_getter(1, 2), "another MIR ABI")
  expect_match(zumirconsumer:::zc_getter(2, 1), "newer C API version")
})

test_that("a consumer import resolves in its context only, and not under D6's list", {
  skip_if(zumir::zumir_tier() == "none", "no tier runs here")
  ok <- zumirconsumer:::zc_expit(0.3, TRUE)
  expect_identical(as.numeric(ok), 1 / (1 + exp(-0.3)))
  # Not registered in this context: a link error, recorded.
  no <- zumirconsumer:::zc_expit(0.3, FALSE)
  expect_true(is.na(no))
  expect_match(attr(no, "message"), "expit")
  # D6's list alone (cfunction()'s and mir_function()'s) does not have it.
  expect_error(
    zumir::mir_function(
      "m: module\n export f\n p: proto d, d:x\n import expit\nf: func d, d:x\n local d:r\n call p, expit, r, x\n ret r\n endfunc\nendmodule",
      "f"
    ),
    class = "zumir_mir_error"
  )
})
