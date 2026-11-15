.libPaths("/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt/lib")
library(zumir)
txt <- "
m: module
   export f
f: func d, d:x
   local i64:v, i64:t, i64:r, d:res
   mov v, 127
   mov t, 5
   dbge L1, x, 1.0
   mov t, 7
L1:
   ands r, v, t
   ui2d res, r
   ret res
   endfunc
   endmodule
"
tier <- commandArgs(TRUE)[1]
g <- mir_function(txt, "f", tier = tier); cat("compiled\n"); print(g(0)); print(g(2))
