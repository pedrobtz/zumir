# One function per refusal site of R/mir_compile.R (#82): each must be a
# zumir_compile_error, never a value R would not give.
refuse_cases <- function() {
  no_name_for <- function(a) a
  body(no_name_for) <- call("for", 1, quote(1:2), 1)
  list(
    dots = function(...) 1,
    default = function(a = 1) a,
    empty_body = function(a) {},
    vector_as_number = function(x) {
      s <- x[1]
      exp(x)
    },
    vector_as_logical = function(x) {
      s <- x[1]
      if (x) 1 else 2
    },
    na_integer = function(a) a + NA_integer_,
    string_constant = function(a) a + "s",
    complex_constant = function(a) a + 1i,
    math_named = function(a) exp(x = a),
    abs_integer = function(a) abs(1L),
    min_three = function(a) min(a, 1, 2),
    min_integers = function(a) min(1L, 2L),
    if_as_value = function(a) a + (if (a > 0) 1 else 2),
    t_and_f = function(a) if (T) a else 1,
    loop_var_after = function(n) {
      s <- 0
      for (i in seq_len(n)) {
        s <- s + 1
      }
      s + i
    },
    read_before_assign = function(a) {
      if (a > 0) {
        b <- 1
      }
      b
    },
    unknown_name = function(a) a + zz,
    length_of_constant = function(a) length(1),
    length_of_scalar = function(a) length(a) + a,
    minus_logical = function(a) -TRUE,
    vector_arith = function(x) {
      s <- x[1]
      x * 2
    },
    logical_arith = function(a) TRUE + 1L,
    vector_compare = function(x) {
      s <- x[1]
      x > 1
    },
    is_na_vector = function(x) {
      s <- x[1]
      as.double(is.na(x))
    },
    string_index = function(x) x["a"],
    matrix_index = function(x) x[1, 2],
    ends_in_assignment = function(a) {
      b <- a
    },
    ends_in_for = function(n) {
      for (i in seq_len(n)) {
        1
      }
    },
    ends_in_while = function(a) {
      b <- a
      while (b > 0) {
        b <- b - 1
      }
    },
    break_outside = function(a) {
      break
      a
    },
    return_empty = function(a) return(),
    assign_call = function(a) {
      names(a) <- "x"
      a
    },
    assign_argument = function(a) {
      a <- 1
      a
    },
    assign_vector_whole = function(n) {
      y <- numeric(n)
      y <- 1
      1
    },
    assign_loop_var = function(n) {
      s <- 0
      for (i in seq_len(n)) {
        i <- 1
      }
      s
    },
    copy_vector = function(x) {
      s <- x[1]
      y <- x
      1
    },
    integer_local = function(a) {
      b <- 1L
      a
    },
    numeric_in_loop = function(n) {
      for (i in seq_len(n)) {
        y <- numeric(2)
      }
      1
    },
    numeric_twice = function(n) {
      y <- numeric(n)
      y <- numeric(n)
      1
    },
    numeric_no_size = function(a) {
      y <- numeric()
      a
    },
    numeric_size_call = function(n) {
      y <- numeric(sum(n))
      1
    },
    index_assign_two = function(n) {
      y <- numeric(n)
      y[1, 2] <- 1
      1
    },
    index_assign_argument = function(x) {
      x[1] <- 1
      1
    },
    index_assign_unknown = function(a) {
      z[1] <- 1
      a
    },
    if_without_else = function(a) if (a > 0) 1,
    loop_bound_string = function(a) {
      s <- 0
      for (i in 1:"a") {
        s <- s + 1
      }
      s
    },
    loop_var_not_name = no_name_for,
    loop_var_argument = function(a) {
      s <- 0
      for (a in 1:2) {
        s <- s + 1
      }
      s
    },
    loop_over_vector = function(a) {
      s <- 0
      for (i in c(1, 2)) {
        s <- s + 1
      }
      s
    },
    no_result = function(a) {
      while (TRUE) {}
    },
    empty_result_block = function(x) {
      if (x > 0) {} else {
        1
      }
    },
    empty_block_after_return = function(x) {
      if (x > 0) {
        return(x)
      }
      {}
    },
    attributed_logical = local({
      f <- function(x) x + K
      body(f)[[3]] <- c(a = TRUE)
      f
    }),
    numeric_named = function(n) {
      y <- numeric(nonsense = n)
      1
    }
  )
}
