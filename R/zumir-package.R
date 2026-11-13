#' @keywords internal
"_PACKAGE"

## usethis namespace: start
#' @useDynLib zumir, .registration = TRUE
## usethis namespace: end

## mockable bindings: start
## mockable bindings: end
NULL

.onLoad <- function(libname, pkgname) {
  # The tier probe runs before anything else maps code (design §4.4).
  probe_tiers()
}

.onUnload <- function(libpath) {
  library.dynam.unload("zumir", libpath)
}
