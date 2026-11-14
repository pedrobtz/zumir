# c2mir's compile path (src/zumir_cfun.c) for the Stage 8 tests: compile
# a C function of doubles and call it.
c2mir_fn <- function(source, fname = "f", tier = zumir_tier()) {
  x <- c2mir_compile(source, fname, tier)
  function(...) .Call(zumir_test_c2mir_call, x, as.double(c(...)))
}

# Every D6 name, as the resolver lists it (src/zumir_imports.c).
d6_names <- function() .Call(zumir_test_import_names, 0L)

# The names a prelude header declares: the identifier before each `(`
# at the start of a declaration.
declared_names <- function(header) {
  lines <- strsplit(header, "\n", fixed = TRUE)[[1]]
  code <- paste(lines[!startsWith(lines, "#")], collapse = "\n")
  decls <- regmatches(code, gregexpr("[A-Za-z_][A-Za-z0-9_]* \\(", code))[[1]]
  sub(" \\($", "", decls)
}
