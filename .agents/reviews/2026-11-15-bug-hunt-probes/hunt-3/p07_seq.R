source("common.R")
z <- asNamespace("zumir")
ctx_of <- function(f) { b <- body(f); found <- NULL; walk <- function(x) { if (typeof(x) == "externalptr") found <<- x else if (is.call(x) || is.list(x)) for (i in seq_along(x)) walk(x[[i]]) }; walk(b); found }
y <- c(1, 0.5); 
for (tier in c("native", "interp")) {
  cat("=====", tier, "\n")
  c1 <- z$compile_model(1L, tier)
  ny <- 2
  ref <- tryCatch(z$context_call_rhs(c1, 1L, c(1,2), c(.1,.2,.3)), error = function(e) conditionMessage(e))
  print(ref)
  chk("free", z$context_free(c1)); chk("free twice", z$context_free(c1))
  chk("call after free", z$context_call_rhs(c1, 1L, c(1,2), c(.1,.2,.3)))
  chk("begin after free", z$context_begin(c1)); chk("link after free", z$context_link(c1, "rhs"))
  chk("state after free", z$context_state(c1)$live)
  # nested begin -> failed
  c2 <- z$context_new(tier); z$context_begin(c2, "m")
  chk("begin twice", z$context_begin(c2, "n"))
  chk("state failed", unlist(z$context_state(c2)[c("live","failed","linked")]))
  chk("emit on failed", z$context_emit_model(c2, 1L)); chk("end on failed", z$context_end(c2)); chk("link on failed", z$context_link(c2, "rhs"))
  chk("call on failed", z$context_call_rhs(c2, 1L, c(1,2), c(.1,.2,.3)))
  z$context_free(c2)
  # link before end
  c3 <- z$context_new(tier); z$context_begin(c3, "m"); z$context_emit_model(c3, 1L)
  chk("link before end", z$context_link(c3, "rhs")); chk("state", unlist(z$context_state(c3)[c("live","failed","linked")]))
  rm(c3); gc()
  # link twice, then add second module and relink
  c4 <- z$compile_model(1L, tier); chk("link twice", z$context_link(c4, "rhs"))
  chk("call after relink", identical(z$context_call_rhs(c4, 1L, c(1,2), c(.1,.2,.3)), ref))
  chk("link wrong name", z$context_link(c4, "nope")); chk("state", unlist(z$context_state(c4)[c("live","failed","linked")]))
  chk("call after notfound link", identical(z$context_call_rhs(c4, 1L, c(1,2), c(.1,.2,.3)), ref))
  chk("scan 2nd module", z$context_scan(c4, paste0(mirtext, "\n")))
  chk("link add", z$context_link(c4, "add"))
  chk("link rhs again", z$context_link(c4, "rhs"))
  chk("call after 2nd module", identical(z$context_call_rhs(c4, 1L, c(1,2), c(.1,.2,.3)), ref))
  chk("scan dup module name+func", z$context_scan(c4, paste0(mirtext, "\n")))
  chk("link after dup", z$context_link(c4, "add")); chk("state", unlist(z$context_state(c4)[c("live","failed","linked")]))
  chk("call after failed", z$context_call_rhs(c4, 1L, c(1,2), c(.1,.2,.3)))
  rm(c4); gc()
  # R error between begin and end
  c5 <- z$context_new(tier); z$context_begin(c5, "m"); try(stop("user error mid-way"), silent = TRUE); rm(c5); gc()
  chk("live after abandon", live())
  # closures: free under the closure
  for (kind in c("mirf","cfun","rf")) {
    f <- mk(kind, tier); g <- f; cx <- ctx_of(f)
    z$context_free(cx)
    chk(paste(kind, "call after ctx freed"), f(1,2)); chk(paste(kind, "copy after ctx freed"), g(1,2))
    f2 <- mk(kind, tier); cx2 <- ctx_of(f2)
    # garbage types on the entries
    chk(paste(kind, "not-a-context"), .Call(z$zumir_ctx_state, 1L)$live)
    e <- new.env(); 
    fake <- unserialize(serialize(cx2, NULL)); chk(paste(kind, "restored state"), z$context_state(fake)$live); chk("free restored", z$context_free(fake))
    chk(paste(kind, "orig after freeing restored"), f2(1,2))
  }
  # cross-entry misuse: mirf ctx to rf_call etc => must be classed, not crash? (internal)
  gc(); chk("live end", live())
}
