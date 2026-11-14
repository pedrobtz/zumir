/* C source to a compiled function through c2mir (design §4.5 steps 1 to
 * 4): the prelude (zumir_prelude.h) and the source are read by c2mir from
 * memory, its diagnostics go to the message buffer, and the module is
 * linked against D6's list alone on the context's tier. Stage 9's
 * cfunction() builds its R closure on zumir_c2mir_compile(). */
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
} c2mir_arg;

static void body_c2mir(zumir_rctx *z, void *arg) {
  c2mir_arg *a = arg;
  struct c2mir_options options;
  memset(&options, 0, sizeof options);
  options.message_file = ZUMIR_MIR_STDERR;
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

/* Calls the function cfunction() compiled. `args` holds one R value per C
 * parameter, already of the right type and checked by R: an integer or a
 * double of length one for int and double; an integer or double vector for
 * a pointer. `kinds` per parameter: 0 int, 1 double, 2 an input pointer
 * (const), 3 an output pointer. An output is duplicated here before its
 * pointer is taken, so the caller's vector is never written (D24).
 * Returns list(value, outputs) with value NULL for void, outputs the
 * written copies in parameter order. */
SEXP zumir_cfun_call(SEXP x, SEXP args, SEXP kinds) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  if (z->fn == NULL) return zumir_status("context", 0, "not compiled");
  R_xlen_t n = XLENGTH(args);
  if (n != z->nargs || XLENGTH(kinds) != n)
    return zumir_status("argument", 0, "wrong number of arguments");
  const int *k = INTEGER(kinds);
  int nout = 0;
  for (R_xlen_t i = 0; i < n; i++) nout += k[i] == 3;
  SEXP outs = PROTECT(Rf_allocVector(VECSXP, nout));
  int64_t *slots = (int64_t *) R_alloc((size_t) n + 1, sizeof(int64_t));
  int o = 0;
  for (R_xlen_t i = 0; i < n; i++) {
    SEXP v = VECTOR_ELT(args, i);
    if (k[i] == 0) {
      slots[i] = (int64_t) INTEGER(v)[0];
    } else if (k[i] == 1) {
      double d = REAL(v)[0];
      memcpy(&slots[i], &d, sizeof d);
    } else {
      if (k[i] == 3) {
        v = Rf_duplicate(v);
        SET_VECTOR_ELT(outs, o++, v);
      }
      /* A pointer is never taken to a vector of length zero (R's data
       * pointer there is not one to dereference). */
      void *p = XLENGTH(v) == 0 ? NULL : TYPEOF(v) == REALSXP ? (void *) REAL(v) : (void *) INTEGER(v);
      slots[i] = (int64_t) (intptr_t) p;
    }
  }
  int64_t r = 0;
  void (*fn)(int64_t *, int64_t *);
  memcpy(&fn, &z->fn, sizeof fn);
  fn(slots, &r);
  SEXP value = R_NilValue;
  if (z->nres == 1) {
    if (z->res_type == MIR_T_D) {
      double d;
      memcpy(&d, &r, sizeof d);
      value = Rf_ScalarReal(d);
    } else {
      value = Rf_ScalarInteger((int) r);
    }
  }
  PROTECT(value);
  const char *rn[] = {"value", "outputs", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, rn));
  SET_VECTOR_ELT(res, 0, value);
  SET_VECTOR_ELT(res, 1, outs);
  UNPROTECT(3);
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
SEXP zumir_cfun_call(SEXP x, SEXP args, SEXP kinds) {
  (void) x;
  (void) args;
  (void) kinds;
  return zumir_status("context", 0, "zumir was built without MIR");
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
