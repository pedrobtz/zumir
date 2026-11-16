source("common.R")
expr <- parse(text = commandArgs(TRUE)[1])[[1]]
r <- tryCatch(eval(expr), error = function(e) paste0("ERR<", class(e)[1], "> ", conditionMessage(e)))
cat("RESULT:", paste(format(unlist(r)), collapse = " "), "| depth", tryCatch(zumir:::test_invalid_module("syntax"), error = function(e) e$trap$depth), "| live", live(), "\n")
