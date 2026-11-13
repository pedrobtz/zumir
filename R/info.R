#' What zumir has, and what runs here
#'
#' `zumir_info()` reports the vendored MIR, whether it has a code generator
#' for this CPU, which tiers run in this session and why the others do not,
#' and the memory MIR contexts hold. `zumir_tier()` is the tier code runs on
#' by default.
#'
#' At load, zumir compiles and calls a small function natively and on MIR's
#' interpreter. Each tier is reported as four capabilities, which fail
#' separately: `compile` (MIR builds the function on this tier), `entry`
#' (it has an address C can call), `run` (calling it gives the right
#' answer) and `external` (it can call a C library function, `exp`). A
#' tier is available when all four hold.
#'
#' Both tiers need memory that is writable and executable, the interpreter
#' too, since MIR generates small call bridges for it. Where the system
#' refuses that memory (SELinux `deny_execmem`, some seccomp policies and
#' hardened containers), neither tier runs, and `zumir_tier()` is `"none"`.
#' The `reason` column says what was refused. On macOS, R needs the
#' `com.apple.security.cs.allow-jit` entitlement that CRAN's build carries.
#'
#' @returns `zumir_info()`: a list with
#'   * `mir_commit`: the upstream MIR commit vendored;
#'   * `mir_patches`: the local patches, one per element;
#'   * `target`: `TRUE` when MIR has a code generator for this CPU and was
#'     compiled in;
#'   * `tier`: as `zumir_tier()`;
#'   * `capabilities`: a data frame with columns `tier`, `capability`, `ok`
#'     and `reason` (`NA` where `ok`);
#'   * `memory`: a list with `contexts`, the MIR contexts alive, and
#'     `bytes`, the memory they hold.
#'
#'   `zumir_tier()`: `"native"`, `"interp"` or `"none"`.
#' @export
#' @examples
#' zumir_tier()
#' info <- zumir_info()
#' info$capabilities
zumir_info <- function() {
  build <- .Call(zumir_mir_build)
  list(
    mir_commit = build$commit,
    mir_patches = build$patches,
    target = .Call(zumir_has_target),
    tier = zumir_tier(),
    capabilities = capabilities_frame(),
    memory = .Call(zumir_memory)
  )
}

#' @rdname zumir_info
#' @export
zumir_tier <- function() {
  caps <- capabilities_frame()
  available <- function(tier) all(caps$ok[caps$tier == tier])
  if (available("native")) {
    "native"
  } else if (available("interp")) {
    "interp"
  } else {
    "none"
  }
}

capabilities_frame <- function() {
  res <- .Call(zumir_probe_result)
  if (is.null(res)) {
    probe_tiers()
    res <- .Call(zumir_probe_result)
  }
  data.frame(
    tier = res$tier,
    capability = res$capability,
    ok = res$ok,
    reason = res$reason,
    stringsAsFactors = FALSE
  )
}

# Runs the tier probe now, replacing the result .onLoad stored. The tests
# call it after refusing W+X memory in a child process.
probe_tiers <- function() {
  invisible(.Call(zumir_probe_run))
}
