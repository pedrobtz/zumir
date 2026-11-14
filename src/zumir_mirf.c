/* mir_function() (design §5): MIR text in, an R function out.
 *
 * The text is read into a fresh context. The named function must be
 * exported by its module, and take and return only numbers: the integer
 * types (i8 to u64), f and d, with at most one result. A second module is
 * then built beside it, with one function of a fixed C signature,
 *
 *   void _zumir_call (int64 *args, int64 *res)
 *
 * which imports the named function, loads each argument from its 8-byte
 * slot as the parameter's type (an integer as int64, f from a double), calls
 * it, and stores the result widened back to 8 bytes (integers sign- or
 * zero-extended from their declared width, f as a double). R calls that
 * one signature on either tier, so no per-signature C trampoline is needed.
 *
 * In R, 8- to 32-bit results are integers and 64-bit ones (and u32) are
 * doubles; a value R cannot hold exactly is an error, never a rounding. */
#include <math.h>
#include <string.h>

#include "zumir_ctx.h"

#if ZUMIR_HAVE_MIR
#include "zumir_trap.h"

static int numeric_type(MIR_type_t t) {
  return (t >= MIR_T_I8 && t <= MIR_T_U64) || t == MIR_T_F || t == MIR_T_D;
}

static const char *type_name(MIR_type_t t) {
  switch (t) {
  case MIR_T_I8: return "i8";
  case MIR_T_U8: return "u8";
  case MIR_T_I16: return "i16";
  case MIR_T_U16: return "u16";
  case MIR_T_I32: return "i32";
  case MIR_T_U32: return "u32";
  case MIR_T_I64: return "i64";
  case MIR_T_U64: return "u64";
  case MIR_T_F: return "f";
  case MIR_T_D: return "d";
  default: return "?";
  }
}

typedef struct {
  const char *text;
  const char *name;
  MIR_item_t func; /* the function named `name`, if any */
  int exported;
} find_arg;

/* Reads the text, then looks for the named function and its export. */
static void body_scan_find(zumir_rctx *z, void *arg) {
  find_arg *f = arg;
  MIR_context_t ctx = z->ctx;
  MIR_scan_string(ctx, f->text);
  for (MIR_module_t m = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx)); m != NULL;
       m = DLIST_NEXT(MIR_module_t, m))
    for (MIR_item_t it = DLIST_HEAD(MIR_item_t, m->items); it != NULL;
         it = DLIST_NEXT(MIR_item_t, it)) {
      if (it->item_type == MIR_func_item && strcmp(it->u.func->name, f->name) == 0)
        f->func = it;
      if (it->item_type == MIR_export_item && strcmp(MIR_item_name(ctx, it), f->name) == 0)
        f->exported = 1;
    }
}

static void body_wrapper(zumir_rctx *z, void *arg) {
  MIR_item_t target = arg;
  MIR_context_t ctx = z->ctx;
  MIR_func_t tf = target->u.func;
  size_t nargs = tf->nargs, nres = tf->nres;
  MIR_var_t *pvars = MIR_malloc(MIR_get_alloc(ctx), sizeof(MIR_var_t) * (nargs + 1));
  for (size_t i = 0; i < nargs; i++) {
    pvars[i] = VARR_GET(MIR_var_t, tf->vars, i);
    pvars[i].size = 0;
  }
  MIR_new_module(ctx, "_zumir_mirf");
  MIR_item_t proto = MIR_new_proto_arr(ctx, "_zumir_p", nres, tf->res_types, nargs, pvars);
  MIR_item_t imp = MIR_new_import(ctx, tf->name);
  MIR_var_t wargs[2] = {{MIR_T_P, "a", 0}, {MIR_T_P, "r", 0}};
  MIR_item_t w = MIR_new_func_arr(ctx, "_zumir_call", 0, NULL, 2, wargs);
  MIR_func_t wf = w->u.func;
  MIR_reg_t ra = MIR_reg(ctx, "a", wf), rr = MIR_reg(ctx, "r", wf);
  MIR_op_t *ops = MIR_malloc(MIR_get_alloc(ctx), sizeof(MIR_op_t) * (nargs + 3));
  char nm[32];
  for (size_t i = 0; i < nargs; i++) {
    MIR_type_t t = pvars[i].type;
    MIR_disp_t disp = (MIR_disp_t) (8 * i);
    snprintf(nm, sizeof nm, "x%zu", i);
    if (t == MIR_T_D) {
      MIR_reg_t x = MIR_new_func_reg(ctx, wf, MIR_T_D, nm);
      MIR_append_insn(ctx, w, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, x),
                                           MIR_new_mem_op(ctx, MIR_T_D, disp, ra, 0, 1)));
      ops[3 + i] = MIR_new_reg_op(ctx, x);
    } else if (t == MIR_T_F) {
      MIR_reg_t d = MIR_new_func_reg(ctx, wf, MIR_T_D, nm);
      snprintf(nm, sizeof nm, "xf%zu", i);
      MIR_reg_t x = MIR_new_func_reg(ctx, wf, MIR_T_F, nm);
      MIR_append_insn(ctx, w, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, d),
                                           MIR_new_mem_op(ctx, MIR_T_D, disp, ra, 0, 1)));
      MIR_append_insn(ctx, w, MIR_new_insn(ctx, MIR_D2F, MIR_new_reg_op(ctx, x),
                                           MIR_new_reg_op(ctx, d)));
      ops[3 + i] = MIR_new_reg_op(ctx, x);
    } else {
      MIR_reg_t x = MIR_new_func_reg(ctx, wf, MIR_T_I64, nm);
      MIR_append_insn(ctx, w, MIR_new_insn(ctx, MIR_MOV, MIR_new_reg_op(ctx, x),
                                           MIR_new_mem_op(ctx, MIR_T_I64, disp, ra, 0, 1)));
      ops[3 + i] = MIR_new_reg_op(ctx, x);
    }
  }
  MIR_reg_t res = 0;
  MIR_type_t rt = nres == 1 ? tf->res_types[0] : MIR_T_UNDEF;
  if (nres == 1)
    res = MIR_new_func_reg(ctx, wf, rt == MIR_T_D ? MIR_T_D : rt == MIR_T_F ? MIR_T_F : MIR_T_I64,
                           "res");
  ops[0] = MIR_new_ref_op(ctx, proto);
  ops[1] = MIR_new_ref_op(ctx, imp);
  size_t nops = 2;
  if (nres == 1) ops[nops++] = MIR_new_reg_op(ctx, res);
  for (size_t i = 0; i < nargs; i++) ops[nops++] = ops[3 + i];
  /* ops[3 + i] were placed for nres == 1; shift when there is no result. */
  if (nres == 0)
    for (size_t i = 0; i < nargs; i++) ops[2 + i] = ops[3 + i];
  MIR_append_insn(ctx, w, MIR_new_insn_arr(ctx, MIR_CALL, nops, ops));
  if (nres == 1) {
    MIR_op_t slot_d = MIR_new_mem_op(ctx, MIR_T_D, 0, rr, 0, 1);
    MIR_op_t slot_i = MIR_new_mem_op(ctx, MIR_T_I64, 0, rr, 0, 1);
    if (rt == MIR_T_D) {
      MIR_append_insn(ctx, w, MIR_new_insn(ctx, MIR_DMOV, slot_d, MIR_new_reg_op(ctx, res)));
    } else if (rt == MIR_T_F) {
      MIR_reg_t d = MIR_new_func_reg(ctx, wf, MIR_T_D, "resd");
      MIR_append_insn(ctx, w, MIR_new_insn(ctx, MIR_F2D, MIR_new_reg_op(ctx, d),
                                           MIR_new_reg_op(ctx, res)));
      MIR_append_insn(ctx, w, MIR_new_insn(ctx, MIR_DMOV, slot_d, MIR_new_reg_op(ctx, d)));
    } else {
      MIR_insn_code_t ext = rt == MIR_T_I8    ? MIR_EXT8
                            : rt == MIR_T_U8  ? MIR_UEXT8
                            : rt == MIR_T_I16 ? MIR_EXT16
                            : rt == MIR_T_U16 ? MIR_UEXT16
                            : rt == MIR_T_I32 ? MIR_EXT32
                            : rt == MIR_T_U32 ? MIR_UEXT32
                                              : MIR_MOV;
      MIR_append_insn(ctx, w, MIR_new_insn(ctx, ext, MIR_new_reg_op(ctx, res),
                                           MIR_new_reg_op(ctx, res)));
      MIR_append_insn(ctx, w, MIR_new_insn(ctx, MIR_MOV, slot_i, MIR_new_reg_op(ctx, res)));
    }
  }
  MIR_append_insn(ctx, w, MIR_new_ret_insn(ctx, 0));
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  MIR_free(MIR_get_alloc(ctx), ops);
  MIR_free(MIR_get_alloc(ctx), pvars);
}

/* Compiles `name` from MIR text on `tier` ("native" or "interp") with
 * import set `imports` (0 D6's, 2 rfunction()'s, for tests). Returns
 * list(context, arg_names, arg_types, res_type), or a zumir_status. */
SEXP zumir_mirf_new(SEXP text, SEXP name, SEXP tier, SEXP imports) {
  int t = strcmp(CHAR(STRING_ELT(tier, 0)), "native") == 0 ? ZUMIR_ITIER_NATIVE
                                                            : ZUMIR_ITIER_INTERP;
  SEXP x = PROTECT(zumir_rctx_new_sexp(t, Rf_asInteger(imports)));
  if (TYPEOF(x) != EXTPTRSXP) {
    UNPROTECT(1);
    return x;
  }
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  find_arg f = {CHAR(STRING_ELT(text, 0)), CHAR(STRING_ELT(name, 0)), NULL, 0};
  if (!zumir_rctx_run(z, body_scan_find, &f)) {
    UNPROTECT(1);
    return zumir_status("mir", z->type, z->message);
  }
  char why[512];
  why[0] = '\0';
  MIR_func_t fn = f.func != NULL ? f.func->u.func : NULL;
  if (fn == NULL) {
    snprintf(why, sizeof why, "the MIR text has no function '%.200s'", f.name);
  } else if (!f.exported) {
    snprintf(why, sizeof why, "'%.200s' is not exported: add `export %.200s` to its module",
             f.name, f.name);
  } else if (fn->vararg_p) {
    snprintf(why, sizeof why, "'%.200s' takes variable arguments", f.name);
  } else if (fn->nres > 1) {
    snprintf(why, sizeof why, "'%.200s' returns %u values; at most one is supported", f.name,
             (unsigned) fn->nres);
  } else if (fn->nres == 1 && !numeric_type(fn->res_types[0])) {
    snprintf(why, sizeof why, "'%.200s' returns a %s, not a number", f.name,
             MIR_type_str(z->ctx, fn->res_types[0]));
  } else {
    for (size_t i = 0; i < fn->nargs; i++) {
      MIR_var_t v = VARR_GET(MIR_var_t, fn->vars, i);
      if (!numeric_type(v.type)) {
        snprintf(why, sizeof why, "argument '%.100s' of '%.100s' is a %s, not a number", v.name,
                 f.name, MIR_type_str(z->ctx, v.type));
        break;
      }
    }
  }
  if (why[0] != '\0') {
    UNPROTECT(1);
    return zumir_status("argument", 0, why);
  }
  if (!zumir_rctx_run(z, body_wrapper, f.func)) {
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
  z->arg_types = malloc(sizeof(int) * (fn->nargs + 1));
  if (z->arg_types == NULL) Rf_error("cannot allocate");
  SEXP names = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t) fn->nargs));
  SEXP types = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t) fn->nargs));
  for (size_t i = 0; i < fn->nargs; i++) {
    MIR_var_t v = VARR_GET(MIR_var_t, fn->vars, i);
    z->arg_types[i] = v.type;
    SET_STRING_ELT(names, (R_xlen_t) i, Rf_mkChar(v.name));
    SET_STRING_ELT(types, (R_xlen_t) i, Rf_mkChar(type_name(v.type)));
  }
  const char *rn[] = {"context", "arg_names", "arg_types", "res_type", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, rn));
  SET_VECTOR_ELT(res, 0, x);
  SET_VECTOR_ELT(res, 1, names);
  SET_VECTOR_ELT(res, 2, types);
  SET_VECTOR_ELT(res, 3, Rf_mkString(z->nres == 1 ? type_name(z->res_type) : ""));
  UNPROTECT(4);
  return res;
}

/* The R value of an integer result of MIR type t, or a zumir_status. */
static SEXP int_result(int64_t v, MIR_type_t t) {
  if (t == MIR_T_I64 || t == MIR_T_U64 || t == MIR_T_U32) {
    /* A double holds integers exactly up to 2^53. The test is on the
     * integer: converting first would round 2^53 + 1 to 2^53. */
    const uint64_t two53 = (uint64_t) 1 << 53;
    int beyond = t == MIR_T_U64 ? (uint64_t) v > two53
                                : (v > (int64_t) two53 || v < -(int64_t) two53);
    if (beyond)
      return zumir_status("argument", 0,
                          "the result does not fit a double exactly (beyond 2^53)");
    return Rf_ScalarReal(t == MIR_T_U64 ? (double) (uint64_t) v : (double) v);
  }
  if (v == (int64_t) NA_INTEGER)
    return zumir_status("argument", 0, "the result -2147483648 is NA in R");
  return Rf_ScalarInteger((int) v);
}

/* Calls the function mir_function() compiled, with `args` a list of length-1
 * numbers. Returns its value, NULL for none, or a zumir_status. */
SEXP zumir_mirf_call(SEXP x, SEXP args) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  if (z->fn == NULL || z->arg_types == NULL) return zumir_status("context", 0, "not compiled");
  if (TYPEOF(args) != VECSXP || XLENGTH(args) != z->nargs)
    return zumir_status("argument", 0, "wrong number of arguments");
  int64_t *slots = (int64_t *) R_alloc((size_t) z->nargs + 1, sizeof(int64_t));
  char why[256];
  for (int i = 0; i < z->nargs; i++) {
    SEXP v = VECTOR_ELT(args, i);
    MIR_type_t t = z->arg_types[i];
    if ((TYPEOF(v) != REALSXP && TYPEOF(v) != INTSXP) || XLENGTH(v) != 1) {
      snprintf(why, sizeof why, "argument %d must be one number", i + 1);
      return zumir_status("argument", 0, why);
    }
    if (t == MIR_T_D || t == MIR_T_F) {
      double d = TYPEOF(v) == INTSXP ? (INTEGER(v)[0] == NA_INTEGER ? NA_REAL : INTEGER(v)[0])
                                     : REAL(v)[0];
      memcpy(&slots[i], &d, sizeof d);
      continue;
    }
    double d = TYPEOF(v) == INTSXP ? (INTEGER(v)[0] == NA_INTEGER ? NA_REAL : INTEGER(v)[0])
                                   : REAL(v)[0];
    double lo, hi;
    switch (t) {
    case MIR_T_I8: lo = -128; hi = 127; break;
    case MIR_T_U8: lo = 0; hi = 255; break;
    case MIR_T_I16: lo = -32768; hi = 32767; break;
    case MIR_T_U16: lo = 0; hi = 65535; break;
    case MIR_T_I32: lo = -2147483648.0; hi = 2147483647.0; break;
    case MIR_T_U32: lo = 0; hi = 4294967295.0; break;
    case MIR_T_U64: lo = 0; hi = 18446744073709549568.0; break;
    default: lo = -9223372036854775808.0; hi = 9223372036854774784.0; break;
    }
    if (ISNAN(d) || d != floor(d) || d < lo || d > hi) {
      snprintf(why, sizeof why, "argument %d must be a whole number that fits a %s", i + 1,
               type_name(t));
      return zumir_status("argument", 0, why);
    }
    slots[i] = t == MIR_T_U64 ? (int64_t) (uint64_t) d : (int64_t) d;
  }
  int64_t res = 0;
  /* ISO C has no cast from an object pointer to a function pointer; the
   * bytes are copied instead (the address MIR returned is code). */
  void (*fn)(int64_t *, int64_t *);
  memcpy(&fn, &z->fn, sizeof fn);
  fn(slots, &res);
  if (z->nres == 0) return R_NilValue;
  if (z->res_type == MIR_T_D || z->res_type == MIR_T_F) {
    double d;
    memcpy(&d, &res, sizeof d);
    return Rf_ScalarReal(d);
  }
  return int_result(res, z->res_type);
}

/* MIR text read and written back (MIR's run-test round trip), or a
 * zumir_status. `path` is a scratch file for MIR_output. */
static void body_scan_text(zumir_rctx *z, void *arg) {
  MIR_scan_string(z->ctx, (const char *) arg);
}

SEXP zumir_mir_roundtrip(SEXP text, SEXP path) {
  SEXP x = PROTECT(zumir_rctx_new_sexp(ZUMIR_ITIER_INTERP, 0));
  if (TYPEOF(x) != EXTPTRSXP) {
    UNPROTECT(1);
    return x;
  }
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (!zumir_rctx_run(z, body_scan_text, (void *) CHAR(STRING_ELT(text, 0)))) {
    UNPROTECT(1);
    return zumir_status("mir", z->type, z->message);
  }
  FILE *f = fopen(CHAR(STRING_ELT(path, 0)), "wb");
  if (f == NULL) Rf_error("cannot open the scratch file");
  zumir_trap t;
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    MIR_output(z->ctx, f);
    zumir_trap_pop(&t);
  } else {
    zumir_rctx_fail(z, &t);
  }
  fclose(f);
  UNPROTECT(1);
  if (z->failed) return zumir_status("mir", z->type, z->message);
  return R_NilValue;
}
#else
SEXP zumir_mirf_new(SEXP text, SEXP name, SEXP tier, SEXP imports) {
  (void) text;
  (void) name;
  (void) tier;
  (void) imports;
  return zumir_status("context", 0, "zumir was built without MIR: no target");
}
SEXP zumir_mirf_call(SEXP x, SEXP args) {
  (void) x;
  (void) args;
  return zumir_status("context", 0, "zumir was built without MIR");
}
SEXP zumir_mir_roundtrip(SEXP text, SEXP path) {
  (void) text;
  (void) path;
  return zumir_status("context", 0, "zumir was built without MIR");
}
#endif
