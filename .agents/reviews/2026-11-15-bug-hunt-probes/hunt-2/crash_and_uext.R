# FINDING: native-tier compile segfaults on  uext(and(<reg holding a constant>, <reg>))
# usage: Rscript crash_and_uext.R native|interp  c|mir
.libPaths("/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt/lib")
library(zumir)
a <- commandArgs(TRUE)
if (a[2] == "c") {
  g <- cfunction("double f(int n) { unsigned mask = 0xff; unsigned r = mask & (unsigned)n; return r; }", tier = a[1])
  cat("compiled\n"); print(g(511L))
} else {
  g <- mir_function("m: module
   export f
f: func d, d:x
   local i64:v, i64:t, i64:r, i64:r2, d:res
   mov v, 127
   mov t, 5
   dbge L1, x, 1.0
   mov t, 7
L1:
   and r, v, t
   uext32 r2, r
   i2d res, r2
   ret res
   endfunc
   endmodule", "f", tier = a[1])
  cat("compiled\n"); print(c(g(0), g(2)))
}
