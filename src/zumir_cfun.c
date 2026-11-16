/* C source to a compiled function through c2mir (design §4.5 steps 1 to
 * 4): the prelude (zumir_prelude.h) and the source are read by c2mir from
 * memory, its diagnostics go to the message buffer, and the module is
 * linked against D6's list alone on the context's tier. Stage 9's
 * cfunction() builds its R closure on zumir_c2mir_compile(). */
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zumir_ctx.h"
#include "zumir_prelude.h"

#if ZUMIR_HAVE_MIR
#include "vendor/mir/c2mir/c2mir.h"
#include "zumir_mir_sink.h"

/* The text c2mir reads: the prelude prefix, the source's name, the line
 * that ends the prelude (zumir_prelude.h), the source itself, and a final
 * newline (c2mir warns about a file that lacks one). */
#define SOURCE_PARTS 5
typedef struct {
  const char *part[SOURCE_PARTS];
  int k;
  size_t pos;
} source_reader;

static int source_getc(void *data) {
  source_reader *r = data;
  while (r->k < SOURCE_PARTS) {
    unsigned char c = (unsigned char) r->part[r->k][r->pos];
    if (c != '\0') {
      r->pos++;
      return c;
    }
    r->k++;
    r->pos = 0;
  }
  return EOF;
}

typedef struct {
  source_reader reader;
  const char *name;
  int ok;
  FILE *prepro; /* non-NULL: preprocess only, into this file */
} c2mir_arg;

static void body_c2mir(zumir_rctx *z, void *arg) {
  c2mir_arg *a = arg;
  struct c2mir_options options;
  memset(&options, 0, sizeof options);
  options.message_file = ZUMIR_MIR_STDERR;
  if (a->prepro != NULL) {
    options.prepro_only_p = 1;
    options.prepro_output_file = a->prepro;
  }
  zumir_mir_message_clear();
  c2mir_init(z->ctx);
  a->ok = c2mir_compile(z->ctx, &options, source_getc, &a->reader, a->name, NULL);
  c2mir_finish(z->ctx);
}

/* list(kind = "compile", type = 0, message, line, column), class
 * zumir_status: c2mir's messages, and where the first error (not a
 * warning) points: "name:line:column: ...", NA when none has a position.
 * Parsed here, not in R: no regex on an error path (R/errors.R). */
static SEXP compile_status(const char *message, const char *name) {
  int line = NA_INTEGER, column = NA_INTEGER;
  size_t n = strlen(name);
  for (const char *m = message; *m != '\0' && line == NA_INTEGER;) {
    const char *eol = strchr(m, '\n');
    if (strncmp(m, name, n) == 0 && m[n] == ':') {
      char *end;
      long l = strtol(m + n + 1, &end, 10);
      if (end != m + n + 1 && *end == ':') {
        char *end2;
        long c = strtol(end + 1, &end2, 10);
        int warning = strncmp(end2, ": warning -- ", 13) == 0;
        if (end2 != end + 1 && !warning) {
          line = (int) l;
          column = (int) c;
        }
      }
    }
    if (eol == NULL) break;
    m = eol + 1;
  }
  const char *names[] = {"kind", "type", "message", "line", "column", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_mkString("compile"));
  SET_VECTOR_ELT(res, 1, Rf_ScalarInteger(0));
  SET_VECTOR_ELT(res, 2, Rf_mkString(message[0] != '\0' ? message : "c2mir failed"));
  SET_VECTOR_ELT(res, 3, Rf_ScalarInteger(line));
  SET_VECTOR_ELT(res, 4, Rf_ScalarInteger(column));
  Rf_setAttrib(res, R_ClassSymbol, Rf_mkString("zumir_status"));
  UNPROTECT(1);
  return res;
}

/* source, the C text; fname, the function to return; tier, "native" or
 * "interp"; name, what diagnostics call the source. Returns the context,
 * its fn the function's address, or a zumir_status. */
SEXP zumir_c2mir_compile(SEXP source, SEXP fname, SEXP tier, SEXP name) {
  int t = strcmp(CHAR(STRING_ELT(tier, 0)), "native") == 0 ? ZUMIR_ITIER_NATIVE
                                                           : ZUMIR_ITIER_INTERP;
  SEXP x = PROTECT(zumir_rctx_new_sexp(t, ZUMIR_IMPORTS_D6));
  if (TYPEOF(x) != EXTPTRSXP) {
    UNPROTECT(1);
    return x;
  }
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  c2mir_arg a;
  memset(&a, 0, sizeof a);
  a.name = CHAR(STRING_ELT(name, 0));
  a.reader.part[0] = zumir_prelude_prefix;
  a.reader.part[1] = a.name;
  a.reader.part[2] = zumir_prelude_line0;
  a.reader.part[3] = CHAR(STRING_ELT(source, 0));
  a.reader.part[4] = "\n";
  if (!zumir_rctx_run(z, body_c2mir, &a)) {
    UNPROTECT(1);
    return zumir_status("mir", z->type, z->message);
  }
  if (!a.ok) {
    SEXP res = compile_status(zumir_mir_message(), a.name);
    UNPROTECT(1);
    return res;
  }
  int status;
  zumir_rctx_compile(z, NULL, CHAR(STRING_ELT(fname, 0)), t, NULL, &status);
  if (status != ZUMIR_ST_OK) {
    SEXP e = status == ZUMIR_ST_MIR ? zumir_status("mir", z->type, z->message)
             : status == ZUMIR_ST_NOT_FOUND
               ? zumir_status("argument", 0, "no function of that name in the source")
               : zumir_status("context", 0, "cannot compile on this tier");
    UNPROTECT(1);
    return e;
  }
  UNPROTECT(1);
  return x;
}

/* The prelude and source after c2mir's preprocessor, written to `path`:
 * comments gone, conditionals resolved and macros expanded, so cfunction()
 * reads the signature c2mir compiles, not a decoy in a comment, an #if 0
 * branch or a macro (a #define const). Returns TRUE, or a zumir_status. */
SEXP zumir_cfun_prepro(SEXP source, SEXP name, SEXP path) {
  SEXP x = PROTECT(zumir_rctx_new_sexp(ZUMIR_ITIER_INTERP, ZUMIR_IMPORTS_D6));
  if (TYPEOF(x) != EXTPTRSXP) {
    UNPROTECT(1);
    return x;
  }
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  c2mir_arg a;
  memset(&a, 0, sizeof a);
  a.name = CHAR(STRING_ELT(name, 0));
  a.reader.part[0] = zumir_prelude_prefix;
  a.reader.part[1] = a.name;
  a.reader.part[2] = zumir_prelude_line0;
  a.reader.part[3] = CHAR(STRING_ELT(source, 0));
  a.reader.part[4] = "\n";
  a.prepro = fopen(R_ExpandFileName(CHAR(STRING_ELT(path, 0))), "w");
  if (a.prepro == NULL) {
    UNPROTECT(1);
    return zumir_status("context", 0, "cannot write the preprocessed source");
  }
  int ran = zumir_rctx_run(z, body_c2mir, &a);
  fclose(a.prepro);
  if (!ran) {
    UNPROTECT(1);
    return zumir_status("mir", z->type, z->message);
  }
  if (!a.ok) {
    SEXP res = compile_status(zumir_mir_message(), a.name);
    UNPROTECT(1);
    return res;
  }
  UNPROTECT(1);
  return Rf_ScalarLogical(1);
}

/* ---- cfunction() (design §4.5 step 5, §5, Q2, D24) ---- */

typedef struct {
  MIR_item_t func; /* the only exported function */
  int nfuncs;      /* exported functions found */
  char names[256]; /* their names, for the error */
} single_arg;

/* The module's exported functions: c2mir exports every definition with
 * external linkage, so these are the source's non-static functions. */
static void body_single(zumir_rctx *z, void *arg) {
  single_arg *a = arg;
  MIR_context_t ctx = z->ctx;
  for (MIR_module_t m = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx)); m != NULL;
       m = DLIST_NEXT(MIR_module_t, m))
    for (MIR_item_t ex = DLIST_HEAD(MIR_item_t, m->items); ex != NULL;
         ex = DLIST_NEXT(MIR_item_t, ex)) {
      if (ex->item_type != MIR_export_item) continue;
      const char *name = MIR_item_name(ctx, ex);
      for (MIR_item_t it = DLIST_HEAD(MIR_item_t, m->items); it != NULL;
           it = DLIST_NEXT(MIR_item_t, it))
        if (it->item_type == MIR_func_item && strcmp(it->u.func->name, name) == 0) {
          if (a->nfuncs++ == 0) a->func = it;
          size_t len = strlen(a->names);
          snprintf(a->names + len, sizeof a->names - len, "%s%s", len ? ", " : "", name);
        }
    }
}

static const char *mir_type_name(MIR_type_t t) {
  switch (t) {
  case MIR_T_I32: return "i32";
  case MIR_T_I64: return "i64";
  case MIR_T_U64: return "u64"; /* c2mir passes a pointer as u64 */
  case MIR_T_P: return "p";
  case MIR_T_D: return "d";
  default: return "other";
  }
}

/* Compiles a cfunction() source: the prelude, c2mir, the source's only
 * non-static function (Q2), and the _zumir_call wrapper around it, linked
 * on `tier` against D6's list. Returns list(context, name, arg_names,
 * arg_types, res_type) with MIR's types ("i32", "i64", "u64", "p", "d", "other";
 * res_type "" for void), or a zumir_status. */
SEXP zumir_cfun_new(SEXP source, SEXP tier, SEXP name) {
  int t = strcmp(CHAR(STRING_ELT(tier, 0)), "native") == 0 ? ZUMIR_ITIER_NATIVE
                                                           : ZUMIR_ITIER_INTERP;
  SEXP x = PROTECT(zumir_rctx_new_sexp(t, ZUMIR_IMPORTS_D6));
  if (TYPEOF(x) != EXTPTRSXP) {
    UNPROTECT(1);
    return x;
  }
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  c2mir_arg a;
  memset(&a, 0, sizeof a);
  a.name = CHAR(STRING_ELT(name, 0));
  a.reader.part[0] = zumir_prelude_prefix;
  a.reader.part[1] = a.name;
  a.reader.part[2] = zumir_prelude_line0;
  a.reader.part[3] = CHAR(STRING_ELT(source, 0));
  a.reader.part[4] = "\n";
  if (!zumir_rctx_run(z, body_c2mir, &a)) {
    UNPROTECT(1);
    return zumir_status("mir", z->type, z->message);
  }
  if (!a.ok) {
    SEXP res = compile_status(zumir_mir_message(), a.name);
    UNPROTECT(1);
    return res;
  }
  single_arg one;
  memset(&one, 0, sizeof one);
  if (!zumir_rctx_run(z, body_single, &one)) {
    UNPROTECT(1);
    return zumir_status("mir", z->type, z->message);
  }
  if (one.nfuncs != 1) {
    char why[400];
    if (one.nfuncs == 0)
      snprintf(why, sizeof why, "the source defines no non-static function");
    else
      snprintf(why, sizeof why,
               "the source defines %d non-static functions (%s): one per cfunction() call; "
               "make the others static",
               one.nfuncs, one.names);
    UNPROTECT(1);
    return zumir_status("argument", 0, why);
  }
  MIR_func_t fn = one.func->u.func;
  if (fn->vararg_p || fn->nres > 1) {
    UNPROTECT(1);
    return zumir_status("argument", 0, "the function takes variable arguments");
  }
  /* A type outside the supported ones (long double, float, char, short,
   * unsigned int) is refused here, before the wrapper is built: MIR
   * rejected the wrapper's call with an internal error. */
  int bad_type = fn->nres == 1 && strcmp(mir_type_name(fn->res_types[0]), "other") == 0;
  for (size_t i = 0; i < fn->nargs && !bad_type; i++)
    bad_type = strcmp(mir_type_name(VARR_GET(MIR_var_t, fn->vars, i).type), "other") == 0;
  if (bad_type) {
    char why[400];
    snprintf(why, sizeof why,
             "cfunction(): `%.200s` returns or takes a type it does not support; it supports "
             "int, double, const int *, const double *, int * and double *, and returns void, "
             "int or double.",
             fn->name);
    UNPROTECT(1);
    return zumir_status("compile", 0, why);
  }
  if (!zumir_rctx_run(z, zumir_mirf_wrapper, one.func)) {
    UNPROTECT(1);
    return zumir_status("mir", z->type, z->message);
  }
  int status;
  zumir_rctx_compile(z, NULL, "_zumir_call", t, NULL, &status);
  if (status != ZUMIR_ST_OK) {
    SEXP res = status == ZUMIR_ST_MIR ? zumir_status("mir", z->type, z->message)
                                      : zumir_status("context", 0, "cannot compile on this tier");
    UNPROTECT(1);
    return res;
  }
  z->nargs = (int) fn->nargs;
  z->nres = (int) fn->nres;
  z->res_type = fn->nres == 1 ? fn->res_types[0] : MIR_T_UNDEF;
  SEXP names = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t) fn->nargs));
  SEXP types = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t) fn->nargs));
  for (size_t i = 0; i < fn->nargs; i++) {
    MIR_var_t v = VARR_GET(MIR_var_t, fn->vars, i);
    SET_STRING_ELT(names, (R_xlen_t) i, Rf_mkChar(v.name));
    SET_STRING_ELT(types, (R_xlen_t) i, Rf_mkChar(mir_type_name(v.type)));
  }
  const char *rn[] = {"context", "name", "arg_names", "arg_types", "res_type", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, rn));
  SET_VECTOR_ELT(res, 0, x);
  SET_VECTOR_ELT(res, 1, Rf_mkString(fn->name));
  SET_VECTOR_ELT(res, 2, names);
  SET_VECTOR_ELT(res, 3, types);
  SET_VECTOR_ELT(res, 4, Rf_mkString(z->nres == 1 ? mir_type_name(z->res_type) : ""));
  UNPROTECT(4);
  return res;
}

/* A call of cfunction()'s closure, whose body is this .Call alone: the
 * arguments are checked and marshalled here, not in R, so a call costs
 * about what a package's .Call wrapper does (design §9). `spec` is
 * cf_cspec()'s list(kinds, types, names, n, fixed, value): per parameter
 * its kind (0 int scalar, 1 double scalar, 2 const pointer, 3 output
 * pointer), its type (0 int, 1 double), its name and an output's fixed
 * length (-1 for none); the 1-based index of the length parameter (0 for
 * none); and the value's name (NA for void). The result: the value, the
 * one output, or the named list of them; NULL when there is neither.
 *
 * A refused argument, or a context that cannot run, is signalled here
 * through zumir_check(), as a classed R condition. That is a longjmp out
 * of this frame, which is safe: it happens before the compiled function is
 * called, no MIR trap is set, and everything allocated is R's. */
static SEXP cf_raise(SEXP st) {
  PROTECT(st);
  SEXP ns = PROTECT(R_FindNamespace(Rf_mkString("zumir")));
  SEXP call = PROTECT(Rf_lang3(Rf_install("zumir_check"), st, R_NilValue));
  Rf_eval(call, ns);
  UNPROTECT(3);
  return R_NilValue;
}

static SEXP cf_refuse(const char *fmt, const char *a, long long b, const char *c, long long d) {
  char msg[512];
  char body[480];
  snprintf(body, sizeof body, fmt, a, b, c, d);
  snprintf(msg, sizeof msg, "cfunction(): %s.", body);
  return cf_raise(zumir_status("argument", 0, msg));
}

/* is.numeric(): an integer or double vector, not a factor and none of the
 * classes is.numeric() refuses (a Date or a time is not its number). The
 * same test for scalars and for vectors passed by pointer. */
static int cf_is_numeric(SEXP v) {
  if (TYPEOF(v) != REALSXP && TYPEOF(v) != INTSXP) return 0;
  return !Rf_isObject(v) || !(Rf_inherits(v, "factor") || Rf_inherits(v, "Date") ||
                         Rf_inherits(v, "POSIXt") || Rf_inherits(v, "difftime"));
}

/* cf_int(): one whole number that fits an int; 0 when it is not. */
static int cf_int(SEXP v, int *out) {
  if (!cf_is_numeric(v) || XLENGTH(v) != 1) return 0;
  if (TYPEOF(v) == INTSXP) {
    if (INTEGER(v)[0] == NA_INTEGER) return 0;
    *out = INTEGER(v)[0];
    return 1;
  }
  double d = REAL(v)[0];
  if (ISNAN(d) || d != floor(d) || fabs(d) > INT_MAX) return 0;
  *out = (int) d;
  return 1;
}

/* A fresh vector of `type` holding v's elements, without its attributes:
 * as.double(v), or as.integer(v) when every non-NA element is a whole
 * number that fits (0 when one is not). */
static SEXP cf_fresh(SEXP v, int type, int *ok) {
  R_xlen_t len = XLENGTH(v);
  *ok = 1;
  if (type == 1) {
    SEXP r = Rf_allocVector(REALSXP, len);
    double *p = REAL(r);
    if (TYPEOF(v) == REALSXP) {
      if (len) memcpy(p, REAL(v), (size_t) len * sizeof(double));
    } else {
      const int *q = INTEGER(v);
      for (R_xlen_t j = 0; j < len; j++) p[j] = q[j] == NA_INTEGER ? NA_REAL : q[j];
    }
    return r;
  }
  if (TYPEOF(v) == REALSXP) {
    const double *q = REAL(v);
    for (R_xlen_t j = 0; j < len; j++) {
      if (!ISNAN(q[j]) && (q[j] != floor(q[j]) || fabs(q[j]) > INT_MAX)) {
        *ok = 0;
        return R_NilValue;
      }
    }
    SEXP r = Rf_allocVector(INTSXP, len);
    int *p = INTEGER(r);
    for (R_xlen_t j = 0; j < len; j++) p[j] = ISNAN(q[j]) ? NA_INTEGER : (int) q[j];
    return r;
  }
  SEXP r = Rf_allocVector(INTSXP, len);
  if (len) memcpy(INTEGER(r), INTEGER(v), (size_t) len * sizeof(int));
  return r;
}

SEXP zumir_cfun_invoke(SEXP x, SEXP spec, SEXP args) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return cf_raise(st);
  if (z->fn == NULL) return cf_raise(zumir_status("context", 0, "not compiled"));
  const int *kinds = INTEGER(VECTOR_ELT(spec, 0));
  const int *types = INTEGER(VECTOR_ELT(spec, 1));
  SEXP names = VECTOR_ELT(spec, 2);
  int n_at = INTEGER(VECTOR_ELT(spec, 3))[0] - 1;
  const int *fixed = INTEGER(VECTOR_ELT(spec, 4));
  SEXP value_name = VECTOR_ELT(spec, 5);
  R_xlen_t np = XLENGTH(args);
  if (np != z->nargs || XLENGTH(VECTOR_ELT(spec, 0)) != np)
    return cf_raise(zumir_status("argument", 0, "wrong number of arguments"));
#define NAME(i) CHAR(STRING_ELT(names, i))

  /* Pointer inputs first: their lengths decide n. A long vector is refused
   * before anything reads it (an ALTREP sequence is never expanded). */
  R_xlen_t min_len = -1;
  int min_at = -1, same = 1;
  for (R_xlen_t i = 0; i < np; i++) {
    if (kinds[i] != 2) continue;
    SEXP v = VECTOR_ELT(args, i);
    if (!cf_is_numeric(v)) return cf_refuse("`%s` must be a numeric vector", NAME(i), 0, "", 0);
    if (XLENGTH(v) > INT_MAX)
      return cf_refuse("`%s` is longer than .Machine$integer.max", NAME(i), 0, "", 0);
    if (min_len >= 0 && XLENGTH(v) != min_len) same = 0;
    if (min_len < 0 || XLENGTH(v) < min_len) {
      min_len = XLENGTH(v);
      min_at = (int) i;
    }
  }
  int n = 0;
  if (n_at >= 0) {
    SEXP given = VECTOR_ELT(args, n_at);
    if (Rf_isNull(given)) {
      if (min_len < 0)
        return cf_refuse("`%s` must be given: no input vector gives it", NAME(n_at), 0, "", 0);
      if (!same) return cf_refuse("the input vectors have different lengths; give `%s`", NAME(n_at), 0, "", 0);
      n = (int) min_len;
    } else {
      if (!cf_int(given, &n))
        return cf_refuse("`%s` must be one whole number that fits an int", NAME(n_at), 0, "", 0);
      if (n < 0) return cf_refuse("`%s` must not be negative", NAME(n_at), 0, "", 0);
      if (min_len >= 0 && n > min_len)
        return cf_refuse("`%s` is %lld, longer than input `%s` (%lld)", NAME(n_at), n, NAME(min_at),
                         (long long) min_len);
    }
  }

  /* The slots, on the stack for the usual few parameters; the vectors
   * passed by pointer are held in `keep` while the function runs. */
  int64_t slot_buf[16];
  int64_t *slots = np < 16 ? slot_buf : (int64_t *) R_alloc((size_t) np + 1, sizeof(int64_t));
  int npointers = 0;
  for (R_xlen_t i = 0; i < np; i++) npointers += kinds[i] >= 2;
  SEXP keep = PROTECT(npointers ? Rf_allocVector(VECSXP, np) : R_NilValue);
  int nout = 0;
  for (R_xlen_t i = 0; i < np; i++) {
    SEXP v = VECTOR_ELT(args, i);
    const char *nm = NAME(i);
    if (kinds[i] == 0) {
      int k = n;
      if (i != n_at && !cf_int(v, &k)) {
        UNPROTECT(1);
        return cf_refuse("`%s` must be one whole number that fits an int", nm, 0, "", 0);
      }
      slots[i] = (int64_t) k;
    } else if (kinds[i] == 1) {
      if (!cf_is_numeric(v) || XLENGTH(v) != 1) {
        UNPROTECT(1);
        return cf_refuse("`%s` must be one number", nm, 0, "", 0);
      }
      double d = TYPEOF(v) == REALSXP ? REAL(v)[0] : INTEGER(v)[0] == NA_INTEGER ? NA_REAL : INTEGER(v)[0];
      memcpy(&slots[i], &d, sizeof d);
    } else {
      int ok = 1;
      if (kinds[i] == 2) {
        /* An input of the parameter's type is passed as it is, no copy. */
        if (TYPEOF(v) != (types[i] == 1 ? REALSXP : INTSXP)) v = cf_fresh(v, types[i], &ok);
      } else {
        R_xlen_t len = fixed[i] >= 0 ? fixed[i] : n;
        if (Rf_isNull(v)) {
          v = Rf_allocVector(types[i] == 1 ? REALSXP : INTSXP, len);
          if (len) memset(types[i] == 1 ? (void *) REAL(v) : (void *) INTEGER(v), 0,
                          (size_t) len * (types[i] == 1 ? sizeof(double) : sizeof(int)));
        } else {
          if (!cf_is_numeric(v)) {
            UNPROTECT(1);
            return cf_refuse("`%s` must be a numeric vector", nm, 0, "", 0);
          }
          if (XLENGTH(v) > INT_MAX) {
            UNPROTECT(1);
            return cf_refuse("`%s` is longer than .Machine$integer.max", nm, 0, "", 0);
          }
          if (XLENGTH(v) < len) {
            UNPROTECT(1);
            return cf_refuse("`%s` has length %lld; the function writes %s%lld", nm,
                             (long long) XLENGTH(v), "", (long long) len);
          }
          /* D24: the C function writes a copy, never the caller's vector. */
          v = cf_fresh(v, types[i], &ok);
        }
        nout++;
      }
      if (!ok) {
        UNPROTECT(1);
        return cf_refuse("`%s` must hold whole numbers that fit an int", nm, 0, "", 0);
      }
      SET_VECTOR_ELT(keep, i, v);
      /* A pointer is never taken to a vector of length zero (R's data
       * pointer there is not one to dereference). */
      void *p = XLENGTH(v) == 0 ? NULL : TYPEOF(v) == REALSXP ? (void *) REAL(v) : (void *) INTEGER(v);
      slots[i] = (int64_t) (intptr_t) p;
    }
  }
#undef NAME

  int64_t r = 0;
  void (*fn)(int64_t *, int64_t *);
  memcpy(&fn, &z->fn, sizeof fn);
  fn(slots, &r);

  SEXP value = R_NilValue;
  int has_value = z->nres == 1;
  if (has_value) {
    if (z->res_type == MIR_T_D) {
      double d;
      memcpy(&d, &r, sizeof d);
      value = Rf_ScalarReal(d);
    } else {
      /* INT_MIN is NA_integer_, as R's C API reads it: an NA element of an
       * int input arrives as INT_MIN and comes back as NA (design §5). */
      value = Rf_ScalarInteger((int) r);
    }
  }
  PROTECT(value);
  int nitems = has_value + nout;
  SEXP res;
  if (nitems == 0) {
    res = R_NilValue;
  } else if (nitems == 1) {
    res = value;
    if (!has_value)
      for (R_xlen_t i = 0; i < np; i++)
        if (kinds[i] == 3) res = VECTOR_ELT(keep, i);
  } else {
    res = PROTECT(Rf_allocVector(VECSXP, nitems));
    SEXP rn = PROTECT(Rf_allocVector(STRSXP, nitems));
    int k = 0;
    if (has_value) {
      SET_VECTOR_ELT(res, k, value);
      SET_STRING_ELT(rn, k++, STRING_ELT(value_name, 0));
    }
    for (R_xlen_t i = 0; i < np; i++) {
      if (kinds[i] != 3) continue;
      SET_VECTOR_ELT(res, k, VECTOR_ELT(keep, i));
      SET_STRING_ELT(rn, k++, STRING_ELT(names, i));
    }
    Rf_setAttrib(res, R_NamesSymbol, rn);
    UNPROTECT(2);
  }
  UNPROTECT(2);
  return res;
}

/* Test entry: calls the compiled function as double f(double, ...) with
 * the doubles in args (0 to 3 of them). */
SEXP zumir_test_c2mir_call(SEXP x, SEXP args) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  if (z->fn == NULL) return zumir_status("context", 0, "not compiled");
  const double *v = REAL(args);
  double r;
  switch (XLENGTH(args)) {
  case 0: {
    double (*f)(void);
    memcpy(&f, &z->fn, sizeof f);
    r = f();
    break;
  }
  case 1: {
    double (*f)(double);
    memcpy(&f, &z->fn, sizeof f);
    r = f(v[0]);
    break;
  }
  case 2: {
    double (*f)(double, double);
    memcpy(&f, &z->fn, sizeof f);
    r = f(v[0], v[1]);
    break;
  }
  default: {
    double (*f)(double, double, double);
    memcpy(&f, &z->fn, sizeof f);
    r = f(v[0], v[1], v[2]);
    break;
  }
  }
  return Rf_ScalarReal(r);
}
#else
SEXP zumir_c2mir_compile(SEXP source, SEXP fname, SEXP tier, SEXP name) {
  (void) source;
  (void) fname;
  (void) tier;
  (void) name;
  return zumir_status("context", 0, "zumir was built without MIR: no target");
}
SEXP zumir_test_c2mir_call(SEXP x, SEXP args) {
  (void) x;
  (void) args;
  return zumir_status("context", 0, "zumir was built without MIR");
}
SEXP zumir_cfun_new(SEXP source, SEXP tier, SEXP name) {
  (void) source;
  (void) tier;
  (void) name;
  return zumir_status("context", 0, "zumir was built without MIR: no target");
}
SEXP zumir_cfun_invoke(SEXP x, SEXP spec, SEXP args) {
  (void) x;
  (void) spec;
  (void) args;
  return zumir_status("context", 0, "zumir was built without MIR");
}
SEXP zumir_cfun_prepro(SEXP source, SEXP name, SEXP path) {
  (void) source;
  (void) name;
  (void) path;
  return zumir_status("context", 0, "zumir was built without MIR: no target");
}
#endif

/* Test entry: the prelude's two headers, list(math.h, string.h), so the
 * tests can hold their declarations to the resolver's list. */
SEXP zumir_test_prelude(void) {
  const char *names[] = {"math.h", "string.h", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_mkString(zumir_prelude_math_h));
  SET_VECTOR_ELT(res, 1, Rf_mkString(zumir_prelude_string_h));
  UNPROTECT(1);
  return res;
}
