# Conditions for errors MIR reports (design §6). A trapped MIR error comes
# back from C as list(type, status, message); type is MIR's
# MIR_error_type_t, or -1 for MIR's exit or abort. The message arrives
# trimmed: nothing here may need memory beyond the condition itself, since
# the error may be that memory ran out (no regex: R's own crashed under
# alloc-failure).
mir_condition <- function(res, call = NULL) {
  structure(
    class = c("zumir_mir_error", "zumir_error", "error", "condition"),
    list(
      message = res$message,
      call = call,
      mir_type = res$type,
      status = res$status
    )
  )
}

# Internal, for tests until Stages 4 and 5 give MIR an R API: builds and
# fails a module of `kind` ("syntax", "operand" or "import") through the
# error trap, and signals the result as a zumir_mir_error. The list C
# returned is attached as `trap`, so a test can read `finished` and `depth`.
test_invalid_module <- function(kind, finish = TRUE) {
  res <- .Call(zumir_test_invalid, kind, finish)
  if (is.null(res)) {
    return(invisible(NULL))
  }
  cnd <- mir_condition(res)
  cnd$trap <- res
  stop(cnd)
}

# A .Call result that may be a zumir_status (src/zumir_ctx.h): returns the
# result, or signals the status as a condition. kind "mir": MIR raised, the
# context is failed. kind "context": the context cannot be used (freed,
# restored from a saved session, failed earlier, not a context).
zumir_check <- function(res, call = sys.call(-1)) {
  if (!inherits(res, "zumir_status")) {
    return(res)
  }
  if (identical(res$kind, "mir")) {
    stop(mir_condition(
      list(
        type = res$type,
        status = 0L,
        message = res$message
      ),
      call = call
    ))
  }
  if (identical(res$kind, "compile")) {
    # c2mir's diagnostics, with where the first one points in the user's
    # source (the prelude's lines are not counted: a #line directive).
    stop(structure(
      class = c("zumir_compile_error", "zumir_error", "error", "condition"),
      list(
        message = res$message,
        call = call,
        line = res$line,
        column = res$column
      )
    ))
  }
  class <- if (identical(res$kind, "argument")) {
    "zumir_argument_error"
  } else {
    "zumir_context_error"
  }
  stop(structure(
    class = c(class, "zumir_error", "error", "condition"),
    list(message = res$message, call = call, mir_type = res$type)
  ))
}
