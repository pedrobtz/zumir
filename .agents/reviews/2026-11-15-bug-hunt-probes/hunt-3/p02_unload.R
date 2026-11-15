source("common.R")
args <- commandArgs(TRUE); mode <- args[1]; kind <- args[2]
f <- mk(kind, "native")
print(f(1,2))
cat("live", live(), "\n")
if (mode == "unload_call") {
  unloadNamespace("zumir")
  cat("unloaded; dll present:", "zumir" %in% names(getLoadedDLLs()), "\n")
  r <- tryCatch(f(1,2), error=function(e) paste("ERR", paste(class(e),collapse="/"), conditionMessage(e)))
  print(r)
} else if (mode == "unload_gc") {
  unloadNamespace("zumir")
  rm(f); gc(); gc()
  cat("survived gc after unload\n")
} else if (mode == "unload_exit") {
  unloadNamespace("zumir")
  cat("exiting with live closure\n")
} else if (mode == "unload_reload_call") {
  unloadNamespace("zumir")
  library(zumir)
  cat("reloaded; live:", .Call(zumir:::zumir_test_live_arenas), "\n")
  r <- tryCatch(f(1,2), error=function(e) paste("ERR", paste(class(e),collapse="/"), conditionMessage(e)))
  print(r)
  g <- mk(kind, "native"); print(g(1,2))
  rm(f); gc(); cat("gc ok; live:", .Call(zumir:::zumir_test_live_arenas), "\n")
} else if (mode == "detach") {
  detach("package:zumir")
  print(f(1,2)); library(zumir); print(f(1,2))
} else if (mode == "dynunload") {
  library.dynam.unload("zumir", system.file(package="zumir"))
  rm(f); gc()
  cat("survived\n")
}
cat("END\n")
