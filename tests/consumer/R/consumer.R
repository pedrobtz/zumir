# Thin wrappers over the consumer's entries (src/); tests call these.
zc_rhs <- function(model, tier, y, par) {
  .Call(zc_rhs_c, as.integer(model), tier, y, par)
}
zc_cmp <- function(what, code, a, b) {
  .Call(zc_cmp_c, what, code, as.double(a), as.double(b))
}
zc_errors <- function(n) .Call(zc_errors_c, as.integer(n))
zc_errors_cpp <- function(n) .Call(zc_errors_cpp_c, as.integer(n))
zc_frozen <- function(x) .Call(zc_frozen_c, as.double(x))
zc_getter <- function(version, mir_abi) {
  .Call(zc_getter_c, as.integer(version), as.integer(mir_abi))
}
zc_table <- function() .Call(zc_table_c)
zc_expit <- function(x, register) {
  .Call(zc_expit_c, as.double(x), as.logical(register))
}
zc_bad_opcode <- function() .Call(zc_bad_opcode_c)
zc_emit_limits <- function(ncalls, nargs, tier) {
  .Call(zc_emit_limits_c, as.integer(ncalls), as.integer(nargs), tier)
}
zc_emit_names <- function(x, tier) .Call(zc_emit_names_c, as.double(x), tier)
zc_tier_lock <- function() .Call(zc_tier_lock_c)
zc_null_args <- function() .Call(zc_null_args_c)
zc_interp_compile <- function() .Call(zc_interp_compile_c)
zc_interp_call <- function(x, a) .Call(zc_interp_call_c, x, as.double(a))
