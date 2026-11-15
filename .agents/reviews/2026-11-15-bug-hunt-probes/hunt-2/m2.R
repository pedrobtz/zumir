# m2.R <i> <tier>: one malformed MIR text per child process
source("common.R")
a <- commandArgs(TRUE); i <- as.integer(a[1]); tier <- a[2]
W <- function(body, sig = "f: func i64") paste0("m: module\n export f\n", sig, "\n", body, "\n endfunc\n endmodule\n")
bad <- list(
  W(" jmp nowhere\n ret 1"), W(" ret 1.5"), W(" add 1, 2, 3\n ret 1"), W(" bogus x\n ret 1"), W(" ret undefinedreg"),
  W(" ret a", "f: func i64, i64:a, i64:a"), W(""), W(" ret"), W(" ret 1", "f: func"), W(" ret a, a", "f: func i64, i64, i64:a"),
  W(" ret a", "f: func p, p:a"), W(" ret a", "f: func ld, ld:a"), W(" ret 1", "f: func i64, ..."), W(" ret 1", "f: func i64, blk:16(a)"),
  W(" local d:a\n ret a", "f: func i64, i64:a"), W(" local i64:r\n mov r, 99999999999999999999999\n ret r"), W(" local d:r\n dmov r, 1e999\n ret r", "f: func d"),
  W(" local i64:r\n div r, 1, 0\n ret r"), W(" ret 1\n ret 2"), W(" ret \"str\""), W(" local i64:r\n mov r, a\n ret r", "f: func i32, i32:a"),
  "m: module\n export f\n export f\nf: func i64\n ret 1\n endfunc\nf: func i64\n ret 2\n endfunc\n endmodule\n",
  "m: module\n export f\nf: func i64\n ret 1\n endfunc\n endmodule\nm: module\n export g\ng: func i64\n ret 2\n endfunc\n endmodule\n",
  W(" local i64:r\n bt L1, 1\n mov r, 5\nL1:\n ret r"), W("L1:\nL1:\n ret 1"), W(" local i64:r\n mov r, i64:(r)\n ret r"), W(" local i64:r, d:x\n add r, x, 1\n ret r"),
  W(" local i64:r, d:x\n dadd x, r, 1.0\n ret r"), W(" local i64:r\n call f, r\n ret r"), W(" local i64:r\n alloca r, -1\n ret 1"), W(" local i64:r\n mov r, L9\n ret r"),
  W(" local i64:r\n switch 5, L1\nL1:\n ret 1"), W(" local i64:r\n mov r, 0\n switch r\n ret 1"), W(" ret 1", "f: func i64, i64:a\n local i64:a"), W(" local i64:r\n ret r, r"),
  W(" local i64:r\n mov r, 1\n inline p, f, r\n ret r"), W(" local d:r\n d2i r, 1\n ret 1"), W(" local i64:r\n mov\n ret 1"), W(" local i64:r\n mov r\n ret 1"), W(" local i64:r\n mov r, 1, 2\n ret 1"),
  W(" local i64:r\n jmp r\n ret 1"), W(" local i64:r\n bt 5, r\n ret 1"), W(" local i64:r\n jmpi r\n ret 1"), W(" local i64:r\n laddr r, nowhere\n ret 1"), W(" bstart 1\n ret 1"), W(" local i64:r\n bend r\n ret 1"),
  W(" local i64:r\n va_arg r, r, 1\n ret 1"), W(" local i64:r\n ret r", "f: func i8"), W(" ret 300", "f: func u8"), W(" local i64:r\n mov r, hr0\n ret r"), W(" local i64:hr0\n ret 1"), W(" local i64:fp\n mov fp, 1\n ret fp"),
  "m: module\n export f\n import f\nf: func i64\n ret 1\n endfunc\n endmodule\n", "m: module\n export f\nf: proto i64\nf: func i64\n ret 1\n endfunc\n endmodule\n", "m: module\n export f\nf: data i64 1\n endmodule\n", "m: module\n export f\nf: bss 8\n endmodule\n", "m: module\n export f\n forward f\n endmodule\n",
  "m: module\n export f\nd: data d 1.5, 2.5\nf: func d\n local d:r\n dmov r, d:(d)\n ret r\n endfunc\n endmodule\n", "m: module\n export f\nd: ref nothing\nf: func i64\n ret 1\n endfunc\n endmodule\n", "m: module\n export f\nd: expr f\nf: func i64\n ret 1\n endfunc\n endmodule\n", "m: module\n export f\nd: lref L1\nf: func i64\nL1:\n ret 1\n endfunc\n endmodule\n",
  W(paste(c(" local i64:r", rep(" add r, r, 1", 5), " ret r"), collapse = "\n")), W(" ret 1", "f: func i64, f:x"), W(" ret 1", paste0("f: func i64, ", paste0("i64:a", 1:300, collapse = ", ")))
)
if (i > length(bad)) q(status = 99)
txt <- bad[[i]]
cat(sprintf("[%d %s] %s\n", i, tier, gsub("\n", "\\\\n", substr(txt, 28, 110))))
g <- try_(mir_function(txt, "f", tier = tier))
if (inherits(g, "caught")) show("compile", g) else show("call", try_(if (length(formals(g))) do.call(g, as.list(rep(1, length(formals(g))))) else g()))
