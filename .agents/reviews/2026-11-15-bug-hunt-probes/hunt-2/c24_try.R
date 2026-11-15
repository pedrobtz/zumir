# The R error from MIR_new_insn(bad opcode) is catchable by tryCatch on the main thread: it is a longjmp through the consumer's C frames.
H <- "/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt"
.libPaths(c(file.path(H, "hunt-2", "zlib"), file.path(H, "lib"))); library(zc2)
before <- zumir::zumir_info()$memory$contexts
r <- tryCatch(probe(24L, 1L, FALSE, 1.5), error = function(e) paste("R error caught:", conditionMessage(e)))
print(r); invisible(gc()); cat("contexts before:", before, " after (the consumer never reached zumir_ctx_free):", zumir::zumir_info()$memory$contexts, "\n")
