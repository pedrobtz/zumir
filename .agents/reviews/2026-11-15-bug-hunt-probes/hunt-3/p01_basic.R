source("common.R")
print(zumir_tier()); print(zumir_info()$memory)
for (tier in c("native","interp")) for (kind in c("mirf","cfun","rf")) {
  cat("==", kind, tier, "\n")
  l0 <- live()
  f <- mk(kind, tier)
  chk("call", f(1, 2))
  chk("live delta", live() - l0)
  g <- f; rm(f); gc(); 
  chk("copy after rm+gc", g(1, 2))
  # serialize
  tf <- tempfile(); saveRDS(g, tf); h <- readRDS(tf)
  chk("readRDS call", h(1, 2))
  chk("identical(g,h)", identical(g, h))
  chk("all.equal(g,h)", all.equal(g, h))
  chk("g still works", g(1,2))
  u <- unserialize(serialize(list(g), NULL))[[1]]
  chk("unserialize call", u(1,2))
  # body copy
  b <- body(g); ff <- function(a, b) NULL; body(ff) <- b; environment(ff) <- environment(g)
  chk("body transplanted", ff(1, 2))
  rm(g, h, u); gc()
  chk("transplant after rm of closure", ff(1, 2))
  # wrong args to transplant
  f3 <- function(a) NULL; body(f3) <- b; environment(f3) <- environment(ff)
  chk("transplant fewer formals", f3(1))
  rm(ff, f3, b); gc(); gc()
  chk("live delta end", live() - l0)
}
