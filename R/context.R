# MIR contexts held by R (src/zumir_ctx.c). Internal until Stage 5's
# mir_function() and C API: the tests build, compile and call through these.
# Each returns its value or signals a zumir_mir_error (MIR raised; the
# context is failed and can only be freed) or a zumir_context_error (the
# context cannot be used).

context_new <- function(tier = c("native", "interp")) {
  tier <- match.arg(tier)
  zumir_check(.Call(zumir_ctx_new, tier))
}

context_free <- function(ctx) {
  invisible(.Call(zumir_ctx_free, ctx))
}

context_state <- function(ctx) {
  .Call(zumir_ctx_state, ctx)
}

context_begin <- function(ctx, name = "m") {
  invisible(zumir_check(.Call(zumir_ctx_begin, ctx, name)))
}

context_end <- function(ctx) {
  invisible(zumir_check(.Call(zumir_ctx_end, ctx)))
}

context_scan <- function(ctx, text) {
  invisible(zumir_check(.Call(zumir_ctx_scan, ctx, text)))
}

context_link <- function(ctx, name) {
  invisible(zumir_check(.Call(zumir_ctx_link, ctx, name)))
}

context_call_d <- function(ctx, x) {
  zumir_check(.Call(zumir_ctx_call_d, ctx, as.double(x)))
}

# Test-only: the spike's model right-hand sides (src/zumir_test_models.c).
context_emit_model <- function(ctx, model) {
  invisible(zumir_check(.Call(zumir_test_emit_model, ctx, as.integer(model))))
}

context_call_rhs <- function(ctx, model, y, par) {
  zumir_check(.Call(zumir_test_call_rhs, ctx, as.integer(model), y, par))
}

# A model compiled on `tier` in a fresh context, ready to call.
compile_model <- function(model, tier) {
  ctx <- context_new(tier)
  context_begin(ctx, "m")
  context_emit_model(ctx, model)
  context_end(ctx)
  context_link(ctx, "rhs")
  ctx
}

#' @export
print.zumir_context <- function(x, ...) {
  s <- context_state(x)
  if (!isTRUE(s$live)) {
    cat("<zumir_context: freed>\n")
  } else {
    cat(sprintf(
      "<zumir_context: %s%s, %s bytes>\n",
      s$tier,
      if (isTRUE(s$failed)) ", failed" else "",
      format(s$bytes, scientific = FALSE)
    ))
  }
  invisible(x)
}
