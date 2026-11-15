.libPaths(c(if (nzchar(Sys.getenv("ZUMIR_LIB"))) Sys.getenv("ZUMIR_LIB") else "/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt/lib", .libPaths()))
library(zumir)
live <- function() .Call(zumir:::zumir_test_live_arenas)
mirtext <- "m: module
   export add
add: func d, d:a, d:b
   local d:r
   dadd r, a, b
   ret r
   endfunc
   endmodule"
mk <- function(kind, tier) switch(kind,
  mirf = mir_function(mirtext, "add", tier = tier),
  cfun = cfunction("double add(double a, double b) { return a + b; }", tier = tier),
  rf = mir_compile(function(a, b) a + b, tier = tier))
chk <- function(label, expr) {
  r <- tryCatch(expr, error = function(e) paste0("ERR<", paste(class(e), collapse="/"), ">: ", conditionMessage(e)))
  cat(sprintf("%-50s %s\n", label, paste(format(r), collapse=" ")))
  invisible(r)
}
