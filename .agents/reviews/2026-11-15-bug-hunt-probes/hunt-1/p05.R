source("common.R")
show <- function(label, expr) { w <- NULL; r <- withCallingHandlers(tryCatch(expr, error = function(e) paste0("ERR<", class(e)[1], "> ", conditionMessage(e))), warning = function(x) { w <<- conditionMessage(x); invokeRestart("muffleWarning") }); cat(sprintf("%-28s %s%s\n", label, paste(deparse(r, control = c("digits17", "keepNA", "keepInteger", "showAttributes")), collapse = ""), if (is.null(w)) "" else paste("  WARN:", w))) }
mk <- function(ty) mir_function(sprintf("m: module\n export f\nf: func %s, %s:a\n ret a\n endfunc\n endmodule", ty, ty), "f")
vals <- list(1, 1L, TRUE, NA, NA_integer_, NA_real_, NaN, Inf, -1, 2.5, -0, "1", NULL, numeric(0), c(1, 2), factor("a"), Sys.Date(), list(1), c(a = 1), matrix(1), 127, 128, 255, 256, -128, -129, 32767, 32768, 65535, 65536, 2147483647, 2147483648, -2147483648, -2147483649, 4294967295, 4294967296, 2^53, 2^53 + 2, -2^53, -2^53-2, 2^63, 2^63 - 1024, -2^63, 2^64, 2^64-2048, 1e300, 1e-320, 16777217, 0.1, 1i, as.raw(1), structure(1, class = "foo"), 1e39, 3.5e38)
for (ty in c("i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "f", "d")) {
  f <- mk(ty); print(f)
  for (v in vals) show(paste(ty, paste(deparse(v), collapse = "")), f(v))
  show(paste(ty, "NULL"), f(NULL)); show(paste(ty, "missing"), f())
  show(paste(ty, "named"), f(a = 1)); show(paste(ty, "wrongname"), f(b = 1)); show(paste(ty, "extra"), f(1, 2))
}
