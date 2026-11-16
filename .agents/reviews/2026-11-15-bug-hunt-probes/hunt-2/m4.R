source("common.R")
id <- function(ty, tier) mir_function(sprintf("m: module\n export f\nf: func %s, %s:a\n ret a\n endfunc\n endmodule\n", ty, ty), "f", tier = tier)
cst <- function(ty, val, tier) mir_function(sprintf("m: module\n export f\nf: func %s\n local i64:r\n mov r, %s\n ret r\n endfunc\n endmodule\n", ty, val), "f", tier = tier)
fmt <- function(v) if (inherits(v, "caught")) paste0("<", v$cls[1], "> ", v$msg) else paste0(typeof(v), ":", format(v, digits = 17))
lims <- list(i8 = c(-129, -128, 127, 128), u8 = c(-1, 0, 255, 256), i16 = c(-32769, -32768, 32767, 32768), u16 = c(-1, 0, 65535, 65536),
  i32 = c(-2147483649, -2147483648, -2147483647, 2147483647, 2147483648), u32 = c(-1, 0, 4294967295, 4294967296),
  i64 = c(-9223372036854775808, -9223372036854777856, -9007199254740993, -9007199254740992, 9007199254740992, 9007199254740994, 9223372036854774784, 9223372036854775808),
  u64 = c(-1, 0, 9007199254740992, 9007199254740994, 9223372036854775808, 18446744073709549568, 18446744073709551616))
for (ty in names(lims)) for (v in c(lims[[ty]], 1.5, NA, NaN, Inf, -Inf, -0)) {
  r <- sapply(c("native", "interp"), function(t) fmt(try_(id(ty, t)(v))))
  cat(sprintf("%-4s arg %-22s native=%s%s\n", ty, format(v, digits = 20), r[1], if (r[1] != r[2]) paste("  INTERP DIFFERS:", r[2]) else ""))
}
for (ty in c("i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64")) for (val in c("-1", "128", "255", "256", "32768", "65536", "2147483647", "2147483648", "-2147483648", "-2147483649", "4294967295", "4294967296", "9007199254740992", "9007199254740993", "-9007199254740992", "-9007199254740993", "9223372036854775807", "-9223372036854775808")) {
  r <- sapply(c("native", "interp"), function(t) fmt(try_(cst(ty, val, t)())))
  cat(sprintf("%-4s ret %-22s native=%s%s\n", ty, val, r[1], if (r[1] != r[2]) paste("  INTERP DIFFERS:", r[2]) else ""))
}
for (v in list(0.1, 1e39, -1e39, 1e-46, NaN, NA_real_, Inf, 16777217, 3.4028235677973366e38, 3.4028234e38, NA_integer_, 5L, -0)) {
  r <- sapply(c("native", "interp"), function(t) fmt(try_(id("f", t)(v))))
  cat(sprintf("f    arg %-22s native=%s%s\n", format(v, digits = 20), r[1], if (r[1] != r[2]) paste("  INTERP DIFFERS:", r[2]) else ""))
}
for (v in list(NA_real_, NaN, NA_integer_, -0, 5e-324, "a", TRUE, NULL, c(1, 2), list(1), 1i, factor("a"))) {
  r <- sapply(c("native", "interp"), function(t) { o <- try_(id("d", t)(v)); if (!inherits(o, "caught") && is.na(o)) paste0(fmt(o), if (is.nan(o) && !is.na(o)) "" else "", " bits=", paste(rev(as.character(writeBin(o, raw()))), collapse = "")) else fmt(o) })
  cat(sprintf("d    arg %-22s native=%s%s\n", paste(deparse(v), collapse = ""), r[1], if (r[1] != r[2]) paste("  INTERP DIFFERS:", r[2]) else ""))
}
g <- id("d", "native"); show("missing arg", try_(g())); show("extra arg", try_(g(1, 2))); show("named", try_(g(a = 3)))
