# MIR text builders and inputs for test-mir-function.R and test-imports.R.

call1_text <- function(fn) {
  sprintf(
    "m: module\n export f\n p: proto d, d:x\n import %s\nf: func d, d:x\n local d:r\n call p, %s, r, x\n ret r\n endfunc\nendmodule",
    fn,
    fn
  )
}

call2_text <- function(fn) {
  sprintf(
    "m: module\n export f\n p: proto d, d:x, d:y\n import %s\nf: func d, d:x, d:y\n local d:r\n call p, %s, r, x, y\n ret r\n endfunc\nendmodule",
    fn,
    fn
  )
}

inputs <- function() {
  c(
    0,
    -0,
    0.5,
    -0.5,
    1,
    -1,
    2.75,
    -2.75,
    10,
    1e-300,
    5e-324,
    1e300,
    Inf,
    -Inf,
    NaN,
    NA
  )
}

port_grid <- function() {
  v <- c(
    0,
    -0,
    1,
    -1,
    0.5,
    -0.5,
    2,
    -2,
    3,
    -3,
    5.5,
    -5.5,
    7,
    1e15,
    -1e15,
    2^53,
    1e300,
    5e-324,
    Inf,
    -Inf,
    NaN,
    NA
  )
  g <- expand.grid(x = v, y = v)
  list(x = g$x, y = g$y)
}

add_text <- function() {
  "
  m: module
     export add
  add: func d, d:a, d:b
     local d:r
     dadd r, a, b
     ret r
     endfunc
     endmodule"
}

int_text <- function(type, op = "add") {
  sprintf(
    "m: module\n export f\nf: func %s, %s:a, %s:b\n local i64:r\n %s r, a, b\n ret r\n endfunc\nendmodule",
    type,
    type,
    type,
    op
  )
}
