# What rfunction() refuses at compile time (design §4.6, D22), each with a
# zumir_compile_error that names it. Where the refusal is a D22 rule, R
# runs the same function too, to show the difference the rule guards.

test_that("constructs outside the subset are refused by name", {
  helper <- function(a) a
  cases <- list(
    helper = function(a) helper(a),
    `<<-` = function(a) {
      s <<- a
      a
    },
    list = function(a) list(a),
    data.frame = function(a) data.frame(a = a),
    `repeat` = function(a) {
      repeat {
        break
      }
      a
    }
  )
  for (nm in names(cases)) {
    e <- expect_error(rfunction(cases[[nm]]), class = "zumir_compile_error")
    expect_match(conditionMessage(e), paste0("`", nm, "`"), fixed = TRUE)
  }
  expect_error(
    rfunction(function(a) {
      s <- "x"
      a
    }),
    class = "zumir_compile_error"
  )
})

test_that("a result R would type as integer or logical is refused", {
  for (f in list(
    function(a) 1L,
    function(a) a > 0,
    function(n) seq_len(n),
    function(a) is.na(a)
  )) {
    expect_error(rfunction(f), class = "zumir_compile_error")
    expect_false(is.double(f(1)))
  }
  expect_identical(rfunction(function(a) as.double(a > 0))(1), 1)
})

test_that("a name rebound in environment(f) is refused", {
  e <- new.env(parent = baseenv())
  e$exp <- function(x) 42
  e$`+` <- function(x, y) 42
  f <- function(a) exp(a)
  g <- function(a) a + 1
  environment(f) <- e
  environment(g) <- e
  expect_identical(f(1), 42)
  expect_identical(g(1), 42)
  expect_error(rfunction(f), class = "zumir_compile_error")
  expect_error(rfunction(g), class = "zumir_compile_error")
})

test_that("a local named like a function the body calls is refused", {
  f <- function(a) {
    exp <- 2
    exp(a) + exp
  }
  # R skips the double when it looks up a function: exp(1) + 2.
  expect_identical(f(1), exp(1) + 2)
  expect_error(rfunction(f), class = "zumir_compile_error")
})

test_that("defaults and ... are refused", {
  f <- function(a = 1) a
  expect_identical(f(), 1)
  expect_error(rfunction(f), class = "zumir_compile_error")
  g <- function(...) 1
  expect_identical(g(1, 2), 1)
  expect_error(rfunction(g), class = "zumir_compile_error")
})

test_that("arguments are read-only", {
  f <- function(n) {
    n <- n + 1
    n
  }
  expect_identical(f(1), 2)
  expect_error(rfunction(f), class = "zumir_compile_error")
  # R copies before x[1] <- 0; writing in place would change the caller's x.
  g <- function(x) {
    x[1] <- 0
    x[1]
  }
  x <- c(5, 6)
  expect_identical(g(x), 0)
  expect_identical(x, c(5, 6))
  expect_error(rfunction(g), class = "zumir_compile_error")
})

test_that("numeric() only once per name, at the top level", {
  in_loop <- function(n) {
    s <- 0
    for (i in seq_len(n)) {
      y <- numeric(2)
      y[1] <- y[1] + 1
      s <- s + y[1]
    }
    s
  }
  # R makes a new y each time: 3, where one y kept across the loop gives 6.
  expect_identical(in_loop(3), 3)
  expect_error(rfunction(in_loop), class = "zumir_compile_error")
  in_branch <- function(n) {
    if (n > 0) {
      y <- numeric(2)
    }
    0
  }
  expect_error(rfunction(in_branch), class = "zumir_compile_error")
  alias <- function(n) {
    y <- numeric(n)
    z <- y
    z[1] <- 1
    y
  }
  # R copies on modify: y stays zero.
  expect_identical(alias(2), c(0, 0))
  expect_error(rfunction(alias), class = "zumir_compile_error")
  again <- function(n) {
    y <- numeric(n)
    y[1] <- 5
    y <- numeric(2)
    y
  }
  expect_identical(again(2), c(0, 0))
  expect_error(rfunction(again), class = "zumir_compile_error")
})

test_that("a local read where it may be unassigned is refused", {
  f <- function(a) {
    if (a > 0) {
      s <- 1
    }
    s
  }
  expect_identical(f(1), 1)
  expect_error(f(-1), "not found")
  expect_error(rfunction(f), class = "zumir_compile_error")
})

test_that("a loop variable read after its loop is refused", {
  f <- function(n) {
    for (i in seq_len(n)) {}
    i
  }
  # R leaves i at the last value, or NULL when the loop never ran.
  expect_identical(f(3), 3L)
  expect_null(f(0))
  expect_error(rfunction(f), class = "zumir_compile_error")
})
