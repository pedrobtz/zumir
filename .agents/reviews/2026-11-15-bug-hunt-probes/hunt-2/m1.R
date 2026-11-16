source("common.R")
mf <- function(txt, name = "f", tier) try_(mir_function(txt, name, tier = tier))
one <- function(label, txt, call, name = "f") for (tier in c("native", "interp")) {
  g <- mf(txt, name, tier)
  if (inherits(g, "caught")) show(paste(label, tier, "[compile]"), g) else show(paste(label, tier), try_(call(g)))
}
# 2. same name in two modules: exported one is d->d, the later unexported is (i64,i64)->i64
one("same name two modules", "
m1: module
    export f
f:  func d, d:a
    local d:r
    dadd r, a, 1.5
    ret r
    endfunc
    endmodule
m2: module
f:  func i64, i64:a, i64:b
    local i64:r
    add r, a, b
    ret r
    endfunc
    endmodule
", function(g) list(formals = names(formals(g)), value = g(3, 4)))
one("same name two modules, both exported", "
m1: module
    export f
f:  func d, d:a
    local d:r
    dadd r, a, 1.5
    ret r
    endfunc
    endmodule
m2: module
    export f
f:  func i64, i64:a, i64:b
    local i64:r
    add r, a, b
    ret r
    endfunc
    endmodule
", function(g) list(formals = names(formals(g)), value = g(3, 4)))
# 20. user exports a function named like a D6 import
one("user-defined exp exported and called from another module", "
m1: module
    export exp
exp: func d, d:a
    local d:r
    dadd r, a, 1.0
    ret r
    endfunc
    endmodule
m2: module
    export f
    import exp
p:  proto d, d:a
f:  func d, d:a
    local d:r
    call p, exp, r, a
    ret r
    endfunc
    endmodule
", function(g) g(1))
one("user-defined exp called directly", "
m1: module
    export exp
exp: func d, d:a
    local d:r
    dadd r, a, 1.0
    ret r
    endfunc
    endmodule
", function(g) g(1), name = "exp")
# 3. reserved names
one("function named _zumir_call", "m: module\n export _zumir_call\n_zumir_call: func i64, i64:a\n ret a\n endfunc\n endmodule\n", function(g) g(1), name = "_zumir_call")
one("module named _zumir_mirf, proto _zumir_p", "_zumir_mirf: module\n export f\n_zumir_p: proto i64, i64:a\nf: func i64, i64:a\n ret a\n endfunc\n endmodule\n", function(g) g(1))
one("arg named zumir_mirf_call", "m: module\n export f\nf: func i64, i64:zumir_mirf_call\n ret zumir_mirf_call\n endfunc\n endmodule\n", function(g) g(7))
one("arg named zumir_check", "m: module\n export f\nf: func i64, i64:zumir_check\n ret zumir_check\n endfunc\n endmodule\n", function(g) g(7))
# 11 imports outside the list
for (nm in c("Rf_error", "R_NaInt", "printf", "malloc", "exit", "system", "R_CheckUserInterrupt", "zumir_r_pow", "R_pow", "sqrtf", "abs", "_zumir_call", "environ"))
  one(paste("import", nm), sprintf("m: module\n export f\n import %s\np: proto d, d:a\nf: func d, d:a\n local d:r\n call p, %s, r, a\n ret r\n endfunc\n endmodule\n", nm, nm), function(g) g(4))
one("import sqrt", "m: module\n export f\n import sqrt\np: proto d, d:a\nf: func d, d:a\n local d:r\n call p, sqrt, r, a\n ret r\n endfunc\n endmodule\n", function(g) g(4))
one("import never called", "m: module\n export f\n import system\nf: func d, d:a\n ret a\n endfunc\n endmodule\n", function(g) g(4))
one("forward/extern data import", "m: module\n export f\n import R_NaInt\nf: func i64\n local i64:r\n mov r, i32:(R_NaInt)\n ret r\n endfunc\n endmodule\n", function(g) g())
# malformed / truncated
good <- "m: module\n export f\nf: func i64, i64:a\n local i64:r\n add r, a, 1\n ret r\n endfunc\n endmodule\n"
for (cut in c(0, 1, 5, 10, 20, 30, 40, 50, 60, 70, 75, 80, nchar(good) - 1)) one(paste("truncated at", cut), substr(good, 1, cut), function(g) g(1))
for (bad in c("", " ", "\n", "garbage", "m: module\n", "m: module\nendmodule\n", "m: module\n export f\n endmodule\n", "f: func i64\n ret 1\n endfunc\n",
  "m: module\n export f\nf: func i64\n ret 1\n endfunc\n", "m: module\n export f\nf: func i64\n ret 1\n endmodule\n", "m: module\n export f\nf: func i64\n ret 1\n endfunc\n endmodule\n endmodule\n",
  "m: module\n export f\nf: func i64\n jmp nowhere\n ret 1\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64\n ret 1.5\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64\n add 1, 2, 3\n ret 1\n endfunc\n endmodule\n",
  "m: module\n export f\nf: func i64\n bogus x\n ret 1\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64\n ret undefinedreg\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64, i64:a, i64:a\n ret a\n endfunc\n endmodule\n",
  "m: module\n export f\nf: func i64\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64\n ret\n endfunc\n endmodule\n", "m: module\n export f\nf: func\n ret 1\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64, i64, i64:a\n ret a, a\n endfunc\n endmodule\n",
  "m: module\n export f\nf: func p, p:a\n ret a\n endfunc\n endmodule\n", "m: module\n export f\nf: func ld, ld:a\n ret a\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64, ...\n ret 1\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64, blk:16(a)\n ret 1\n endfunc\n endmodule\n",
  "m: module\n export f\nf: func i64, i64:a\n local d:a\n ret a\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64\n local i64:r\n mov r, 99999999999999999999999\n ret r\n endfunc\n endmodule\n", "m: module\n export f\nf: func d\n local d:r\n dmov r, 1e999\n ret r\n endfunc\n endmodule\n",
  "m: module\n export f\nf: func i64\n local i64:r\n div r, 1, 0\n ret r\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64\n ret 1\n ret 2\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64\nL: jmp L\n endfunc\n endmodule\x01\n", "m: module\n export f\nf: func i64\n ret \"str\"\n endfunc\n endmodule\n",
  "m: module\n export f\nf: func i32, i32:a\n local i64:r\n mov r, a\n ret r\n endfunc\n endmodule\n", "m: module\n export f\n export f\nf: func i64\n ret 1\n endfunc\nf: func i64\n ret 2\n endfunc\n endmodule\n", "m: module\n export f\nf: func i64\n ret 1\n endfunc\n endmodule\nm: module\n export g\ng: func i64\n ret 2\n endfunc\n endmodule\n"))
  one(paste("bad:", gsub("\n", "\\\\n", substr(bad, 1, 80))), bad, function(g) if (length(formals(g))) g(1) else g())
show("name not there", mf(good, "g", "native")); show("name NA", mf(good, NA_character_, "native")); show("name empty", mf(good, "", "native")); show("text NA", mf(NA_character_, "f", "native")); show("text vec", mf(c(good, good), "f", "native")); show("tier bad", mf(good, "f", "jit")); show("tier NA", mf(good, "f", NA)); show("tier none", mf(good, "f", "none"))
show("long name", mf(good, strrep("x", 5000), "native"))
