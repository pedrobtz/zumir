/* Internal test entry for zumir_mir_stdio.h, compiled under the same
 * redirecting macros as MIR's sources, so the test sees exactly what MIR's
 * calls would do. tests/testthat/test-mir-stdio.R. */
#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#include "zumir_trap.h"
#include "zumir_mir_stdio.h"

/* Writes `text` to stderr with fprintf, then '!' with fputc, then a perror
 * line for ENOENT, and the same fprintf and fputc to the file `path`.
 * Returns c(<what reached the sink>, <what MIR's code would have exited
 * with: nothing, since exit is not called>). */
SEXP zumir_test_sink(SEXP text, SEXP path) {
  const char *s = CHAR(STRING_ELT(text, 0));
  zumir_mir_message_clear();
  fprintf(stderr, "%s", s);
  fputc('!', stderr);
  errno = ENOENT;
  perror("p");
  FILE *f = fopen(CHAR(STRING_ELT(path, 0)), "w");
  if (f == NULL) Rf_error("cannot open the test file");
  fprintf(f, "%s", s);
  fputc('!', f);
  fclose(f);
  SEXP out = PROTECT(Rf_mkString(zumir_mir_message()));
  zumir_mir_message_clear();
  UNPROTECT(1);
  return out;
}

/* MIR's exit (status), as MIR's own code calls it: with `trapped` FALSE
 * there is no trap, and the package's handler raises an R error; with it
 * TRUE the trap catches it and c(status, message) is returned. */
SEXP zumir_test_exit(SEXP text, SEXP trapped) {
  zumir_trap t;
  zumir_mir_message_clear();
  if (Rf_asLogical(trapped) != TRUE) {
    fprintf(stderr, "%s", CHAR(STRING_ELT(text, 0)));
    exit(3);
  }
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    fprintf(stderr, "%s\n", CHAR(STRING_ELT(text, 0)));
    abort();
  }
  SEXP out = PROTECT(Rf_allocVector(STRSXP, 2));
  char status[16];
  snprintf(status, sizeof(status), "%d", t.status);
  SET_STRING_ELT(out, 0, Rf_mkChar(status));
  SET_STRING_ELT(out, 1, Rf_mkChar(t.message));
  UNPROTECT(1);
  return out;
}
