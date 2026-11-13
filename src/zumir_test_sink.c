/* Internal test entry for zumir_mir_stdio.h, compiled under the same
 * redirecting macros as MIR's sources, so the test sees exactly what MIR's
 * calls would do. tests/testthat/test-mir-stdio.R. */
#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

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

/* MIR's exit (status) as zumir handles it before Stage 3: an R error with
 * what MIR wrote to stderr first. */
SEXP zumir_test_exit(SEXP text) {
  zumir_mir_message_clear();
  fprintf(stderr, "%s", CHAR(STRING_ELT(text, 0)));
  exit(3);
}
