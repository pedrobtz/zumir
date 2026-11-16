# Internal, for tests: C source to a compiled function through c2mir (design
# §4.5 steps 1 to 4), the compile path cfunction() shares.
#
# `source` is C text; `fname` the function to return; the source is read
# after zumir's prelude, which declares D6's names and reads no system
# header (D5). A compile error is a zumir_compile_error carrying the
# `line` and `column` in `source` that c2mir's first error points at; a
# call to a name outside D6's list is a zumir_mir_error from the link.
c2mir_compile <- function(source, fname, tier = NULL, name = "cfunction") {
  if (!is.character(source) || length(source) != 1L || is.na(source)) {
    zumir_stop("argument", "c2mir_compile(): `source` must be one string of C.")
  }
  tier <- resolve_tier(tier, "c2mir_compile")
  zumir_check(.Call(zumir_c2mir_compile, source, fname, tier, name))
}
