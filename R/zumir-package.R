#' @keywords internal
"_PACKAGE"

## usethis namespace: start
#' @useDynLib zumir, .registration = TRUE
## usethis namespace: end

## mockable bindings: start
## mockable bindings: end
NULL

.onLoad <- function(libname, pkgname) {
  # R's %% and %/% use long double only when R was built with it; zumir's
  # ports of them follow (src/zumir_imports.c).
  .Call(zumir_set_long_double, capabilities("long.double"))
  # R 4.6 wraps exp, cos, sqrt and others with small-argument shortcuts;
  # mir_compile()'s math follows the R it runs in.
  .Call(zumir_set_r46, getRversion() >= "4.6.0")
  # The tier probe runs before anything else maps code (design §4.4).
  probe_tiers()
}

.onUnload <- function(libpath) {
  library.dynam.unload("zumir", libpath)
}
