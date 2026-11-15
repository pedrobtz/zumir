.libPaths("/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt/lib")
library(zumir)
a <- commandArgs(TRUE)
body <- list(
v1 = "   mov v, 127\n   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   ands r, v, t\n   uext32 r, r\n   ui2d res, r\n   ret res",
v2 = "   mov v, 127\n   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   ands r, v, t\n   uext32 r2, r\n   ui2d res, r2\n   ret res",
v3 = "   mov v, 127\n   mov t, 5\n   dlt r2, x, 1.0\n   bfs L1, r2\n   mov t, 7\nL1:\n   uext32 v, v\n  ext32 t, t\n   uext32 r2, t\n   ands r, v, r2\n   uext32 r2, r\n   ui2d res, r2\n   ret res",
v5 = "   mov v, 127\n   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   and r, v, t\n   uext32 r2, r\n   i2d res, r2\n   ret res",
v6 = "   mov v, 127\n   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   and r, v, t\n   ui2d res, r\n   ret res",
v7 = "   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   uext32 r2, t\n   ui2d res, r2\n   ret res",
v8 = "   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   uext32 r2, t\n   i2d res, r2\n   ret res",
v9 = "   d2i t, x\n   uext32 r2, t\n   ui2d res, r2\n   ret res",
v10 = "   d2i t, x\n   uext32 r2, t\n   add r2, r2, 1\n   ret 1.0",
v11 = "   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   uext32 r2, t\n   add r, r2, 1\n   i2d res, r\n  ret res",
v12 = "   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   uext16 r2, t\n   i2d res, r2\n  ret res",
v13 = "   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   ext32 r2, t\n   i2d res, r2\n  ret res",
v4 = "   mov v, 127\n   mov t, 5\n   dbge L1, x, 1.0\n   mov t, 7\nL1:\n   and r, v, t\n   uext32 r2, r\n   ui2d res, r2\n   ret res"
)[[a[2]]]
txt <- paste0("m: module\n   export f\nf: func d, d:x\n   local i64:v, i64:t, i64:r, i64:r2, d:res\n", body, "\n   endfunc\n   endmodule\n")
g <- mir_function(txt, "f", tier = a[1]); cat(a[2], "compiled\n"); print(c(g(0), g(2)))
