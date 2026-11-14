/* rfunction()'s two C entries (design §4.6): compile the MIR text the R
 * walker emits, with rfunction()'s import list (D18), and call it. The
 * generated function is
 *
 *   int64 rf (double *K, double *S, double **VP, int64 *VL, int64 *R)
 *
 * K the constant pool (R passes the doubles, so no number is ever printed
 * into MIR text), S the scalar arguments, VP and VL the vectors (arguments,
 * then the vector locals R allocated) and their lengths, R[0] the result
 * (a double, or the index of the vector returned). The return value is a
 * status: 0, or a run-time error or interrupt for the R wrapper to raise. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zumir_ctx.h"
#include "zumir_trap.h"

#if ZUMIR_HAVE_MIR
static void body_output(zumir_rctx *z, void *arg) {
  MIR_output(z->ctx, (FILE *) arg);
}

/* Writes the context's modules, as MIR prints them, to `path` (a test
 * entry's scratch file): the two routes are compared on this text. */
static int output_to(zumir_rctx *z, SEXP path) {
  if (path == R_NilValue) return 1;
  FILE *f = fopen(CHAR(STRING_ELT(path, 0)), "wb");
  if (f == NULL) return 0;
  int ok = zumir_rctx_run(z, body_output, f);
  fclose(f);
  return ok;
}

static SEXP compile_rf(SEXP x, zumir_rctx *z, MIR_module_t module, int t) {
  int status;
  zumir_rctx_compile(z, module, "rf", t, NULL, &status);
  if (status != ZUMIR_ST_OK) {
    return status == ZUMIR_ST_MIR ? zumir_status("mir", z->type, z->message)
                                  : zumir_status("context", 0, "cannot compile on this tier");
  }
  return x;
}

static int tier_of(SEXP tier) {
  return strcmp(CHAR(STRING_ELT(tier, 0)), "native") == 0 ? ZUMIR_ITIER_NATIVE
                                                           : ZUMIR_ITIER_INTERP;
}

/* The text route (D17): the walker's MIR text, read by MIR's scanner. */
SEXP zumir_rf_new(SEXP text, SEXP tier, SEXP path) {
  int t = tier_of(tier);
  SEXP x = PROTECT(zumir_rctx_new_sexp(t, ZUMIR_IMPORTS_RFUNCTION));
  if (TYPEOF(x) != EXTPTRSXP) {
    UNPROTECT(1);
    return x;
  }
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  SEXP res = PROTECT(zumir_ctx_scan(x, text));
  if (res != R_NilValue) {
    UNPROTECT(2);
    return res;
  }
  if (!output_to(z, path)) {
    UNPROTECT(2);
    return zumir_status("context", 0, "cannot write the module's text");
  }
  SEXP out = compile_rf(x, z, NULL, t);
  UNPROTECT(2);
  return out;
}

/* ---- The emitter route (D17) ----
 *
 * The same lowering, built with MIR's API instead of read from text. R
 * passes the function's registers (names and types), its imports, and its
 * lines: an opcode and its operands, or a label (opcode ""). An operand is
 * written as in MIR's text, in the small grammar the walker emits:
 *
 *   123, -8                  an integer
 *   z3, vp1, K               a register
 *   L4, Lst3, L4_b           a label (one the lines define)
 *   p_d1, zumir_r_exp        a prototype or an import
 *   d:8(K), d:-8(vp1, z2, 8) memory: type, displacement, base[, index, scale]
 *
 * Every string is read out of R before the trap is set, so nothing inside
 * it calls R. The module is the text route's item for item: module "rfm",
 * export rf, the four prototypes, the imports, then rf, and each label made
 * where it first appears, as MIR's scanner makes them; MIR_output prints
 * the two the same. */

typedef struct {
  const char *op; /* "" for a label */
  int nargs;
  const char **args;
} rf_line;

/* What body_build needs, every string and every scratch array taken
 * before the trap is set. */
typedef struct {
  int nregs;
  const char **reg_names;
  MIR_type_t *reg_types; /* MIR_T_D or MIR_T_I64 */
  int nimports;
  const char **imports;
  int nlines;
  rf_line *lines;
  int maxargs;
  MIR_item_t *import_items; /* nimports */
  const char **label_names; /* nlines */
  MIR_insn_t *labels;       /* nlines */
  MIR_op_t *ops;            /* maxargs */
  MIR_module_t module;      /* out */
} rf_build;

typedef struct {
  MIR_context_t ctx;
  MIR_item_t func;
  rf_build *b;
  MIR_item_t protos[4];
  int nlabels;
} rf_emit;

static const char *proto_names[4] = {"p_d1", "p_d2", "p_id", "p_i0"};

static MIR_insn_t label_for(rf_emit *e, const char *name) {
  rf_build *b = e->b;
  for (int i = 0; i < e->nlabels; i++)
    if (strcmp(b->label_names[i], name) == 0) return b->labels[i];
  b->label_names[e->nlabels] = name;
  b->labels[e->nlabels] = MIR_new_label(e->ctx);
  return b->labels[e->nlabels++];
}

static int is_label(rf_emit *e, const char *name) {
  for (int i = 0; i < e->b->nlines; i++)
    if (e->b->lines[i].op[0] == '\0' && strcmp(e->b->lines[i].args[0], name) == 0) return 1;
  return 0;
}

static MIR_item_t ref_item(rf_emit *e, const char *name) {
  for (int i = 0; i < 4; i++)
    if (strcmp(proto_names[i], name) == 0) return e->protos[i];
  for (int i = 0; i < e->b->nimports; i++)
    if (strcmp(e->b->imports[i], name) == 0) return e->b->import_items[i];
  return NULL;
}

static void bad_operand(rf_emit *e, const char *a) {
  MIR_get_error_func(e->ctx)(MIR_wrong_type_error, "rfunction(): unknown operand %s", a);
}

static MIR_reg_t reg_named(rf_emit *e, const char *name, size_t len) {
  char buf[64];
  if (len == 0 || len >= sizeof buf) bad_operand(e, name);
  memcpy(buf, name, len);
  buf[len] = '\0';
  return MIR_reg(e->ctx, buf, e->func->u.func);
}

/* "t:disp(base[, index, scale])" */
static MIR_op_t mem_operand(rf_emit *e, const char *a) {
  const char *colon = strchr(a, ':');
  MIR_type_t t = MIR_T_D;
  if (colon - a == 1 && a[0] == 'd')
    t = MIR_T_D;
  else if (colon - a == 3 && strncmp(a, "i64", 3) == 0)
    t = MIR_T_I64;
  else
    bad_operand(e, a);
  char *end;
  long disp = strtol(colon + 1, &end, 10);
  if (*end != '(') bad_operand(e, a);
  const char *p = end + 1, *q = p;
  while (*q && *q != ',' && *q != ')') q++;
  MIR_reg_t base = reg_named(e, p, (size_t) (q - p)), index = 0;
  long scale = 1;
  if (*q == ',') {
    p = q + 1;
    while (*p == ' ') p++;
    q = p;
    while (*q && *q != ',' && *q != ')') q++;
    index = reg_named(e, p, (size_t) (q - p));
    if (*q != ',') bad_operand(e, a);
    scale = strtol(q + 1, &end, 10);
    q = end;
  }
  if (*q != ')' || q[1] != '\0') bad_operand(e, a);
  return MIR_new_mem_op(e->ctx, t, (MIR_disp_t) disp, base, index, (MIR_scale_t) scale);
}

static MIR_op_t operand(rf_emit *e, const char *a) {
  if (a[0] == '-' || (a[0] >= '0' && a[0] <= '9')) {
    char *end;
    long long v = strtoll(a, &end, 10);
    if (*end != '\0') bad_operand(e, a);
    return MIR_new_int_op(e->ctx, (int64_t) v);
  }
  if (strchr(a, ':') != NULL) return mem_operand(e, a);
  if (is_label(e, a)) return MIR_new_label_op(e->ctx, label_for(e, a));
  MIR_item_t item = ref_item(e, a);
  if (item != NULL) return MIR_new_ref_op(e->ctx, item);
  return MIR_new_reg_op(e->ctx, reg_named(e, a, strlen(a)));
}

static MIR_insn_code_t code_named(rf_emit *e, const char *op) {
  for (int c = 0; c < MIR_INSN_BOUND; c++)
    if (strcmp(MIR_insn_name(e->ctx, (MIR_insn_code_t) c), op) == 0) return (MIR_insn_code_t) c;
  MIR_get_error_func(e->ctx)(MIR_wrong_type_error, "rfunction(): unknown opcode %s", op);
  return MIR_INSN_BOUND;
}

static void body_build(zumir_rctx *z, void *arg) {
  rf_build *b = arg;
  MIR_context_t ctx = z->ctx;
  rf_emit e = {ctx, NULL, b, {NULL, NULL, NULL, NULL}, 0};
  b->module = MIR_new_module(ctx, "rfm");
  MIR_new_export(ctx, "rf");
  MIR_type_t td = MIR_T_D, ti = MIR_T_I64;
  MIR_var_t x = {MIR_T_D, "x", 0}, xy[2] = {{MIR_T_D, "x", 0}, {MIR_T_D, "y", 0}};
  e.protos[0] = MIR_new_proto_arr(ctx, "p_d1", 1, &td, 1, &x);
  e.protos[1] = MIR_new_proto_arr(ctx, "p_d2", 1, &td, 2, xy);
  e.protos[2] = MIR_new_proto_arr(ctx, "p_id", 1, &ti, 1, &x);
  e.protos[3] = MIR_new_proto_arr(ctx, "p_i0", 1, &ti, 0, NULL);
  for (int i = 0; i < b->nimports; i++) b->import_items[i] = MIR_new_import(ctx, b->imports[i]);
  MIR_var_t args[5] = {{MIR_T_P, "K", 0}, {MIR_T_P, "S", 0}, {MIR_T_P, "VP", 0},
                       {MIR_T_P, "VL", 0}, {MIR_T_P, "R", 0}};
  e.func = MIR_new_func_arr(ctx, "rf", 1, &ti, 5, args);
  for (int i = 0; i < b->nregs; i++)
    MIR_new_func_reg(ctx, e.func->u.func, b->reg_types[i], b->reg_names[i]);
  for (int i = 0; i < b->nlines; i++) {
    rf_line *l = &b->lines[i];
    if (l->op[0] == '\0') {
      MIR_append_insn(ctx, e.func, label_for(&e, l->args[0]));
      continue;
    }
    MIR_insn_code_t code = code_named(&e, l->op);
    for (int k = 0; k < l->nargs; k++) b->ops[k] = operand(&e, l->args[k]);
    MIR_append_insn(ctx, e.func, MIR_new_insn_arr(ctx, code, (size_t) l->nargs, b->ops));
  }
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
}

static const char *string_at(SEXP x, R_xlen_t i) {
  return CHAR(STRING_ELT(x, i));
}

/* spec: list(reg_names, reg_types ("d" or "i64"), imports, ops, args), ops
 * and args one per line. */
SEXP zumir_rf_build(SEXP spec, SEXP tier, SEXP path) {
  int t = tier_of(tier);
  SEXP x = PROTECT(zumir_rctx_new_sexp(t, ZUMIR_IMPORTS_RFUNCTION));
  if (TYPEOF(x) != EXTPTRSXP) {
    UNPROTECT(1);
    return x;
  }
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  SEXP names = VECTOR_ELT(spec, 0), types = VECTOR_ELT(spec, 1), imports = VECTOR_ELT(spec, 2),
       ops = VECTOR_ELT(spec, 3), args = VECTOR_ELT(spec, 4);
  rf_build b;
  memset(&b, 0, sizeof b);
  /* R_alloc can raise; the context is already owned by x, whose finalizer
   * frees it, so nothing native is lost if it does. */
  b.nregs = (int) XLENGTH(names);
  b.reg_names = (const char **) R_alloc((size_t) b.nregs + 1, sizeof(char *));
  b.reg_types = (MIR_type_t *) R_alloc((size_t) b.nregs + 1, sizeof(MIR_type_t));
  for (int i = 0; i < b.nregs; i++) {
    b.reg_names[i] = string_at(names, i);
    b.reg_types[i] = strcmp(string_at(types, i), "d") == 0 ? MIR_T_D : MIR_T_I64;
  }
  b.nimports = (int) XLENGTH(imports);
  b.imports = (const char **) R_alloc((size_t) b.nimports + 1, sizeof(char *));
  b.import_items = (MIR_item_t *) R_alloc((size_t) b.nimports + 1, sizeof(MIR_item_t));
  for (int i = 0; i < b.nimports; i++) b.imports[i] = string_at(imports, i);
  b.nlines = (int) XLENGTH(ops);
  b.lines = (rf_line *) R_alloc((size_t) b.nlines + 1, sizeof(rf_line));
  for (int i = 0; i < b.nlines; i++) {
    SEXP a = VECTOR_ELT(args, i);
    rf_line *l = &b.lines[i];
    l->op = string_at(ops, i);
    l->nargs = (int) XLENGTH(a);
    l->args = (const char **) R_alloc((size_t) l->nargs + 1, sizeof(char *));
    for (int k = 0; k < l->nargs; k++) l->args[k] = string_at(a, k);
    if (l->nargs > b.maxargs) b.maxargs = l->nargs;
  }
  b.label_names = (const char **) R_alloc((size_t) b.nlines + 1, sizeof(char *));
  b.labels = (MIR_insn_t *) R_alloc((size_t) b.nlines + 1, sizeof(MIR_insn_t));
  b.ops = (MIR_op_t *) R_alloc((size_t) b.maxargs + 1, sizeof(MIR_op_t));
  if (!zumir_rctx_run(z, body_build, &b)) {
    UNPROTECT(1);
    return zumir_status("mir", z->type, z->message);
  }
  if (!output_to(z, path)) {
    UNPROTECT(1);
    return zumir_status("context", 0, "cannot write the module's text");
  }
  SEXP out = compile_rf(x, z, b.module, t);
  UNPROTECT(1);
  return out;
}
typedef int64_t (*rf_fn)(const double *K, const double *S, double *const *VP, const int64_t *VL,
                         int64_t *R);

/* Calls the compiled function. Returns list(status, value, nan, fmod):
 * value is R[0] as a double (the result) or as an index (a vector result),
 * nan and fmod say where R would have warned. */
SEXP zumir_rf_call(SEXP x, SEXP consts, SEXP scalars, SEXP vectors, SEXP vector_result) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  if (z->fn == NULL) return zumir_status("context", 0, "not compiled");
  R_xlen_t nv = XLENGTH(vectors);
  double **vp = (double **) R_alloc((size_t) nv + 1, sizeof(double *));
  int64_t *vl = (int64_t *) R_alloc((size_t) nv + 1, sizeof(int64_t));
  for (R_xlen_t i = 0; i < nv; i++) {
    SEXP v = VECTOR_ELT(vectors, i);
    vl[i] = (int64_t) XLENGTH(v);
    vp[i] = XLENGTH(v) > 0 ? REAL(v) : NULL;
  }
  int64_t r[2] = {0, 0};
  zumir_r_nan_warned = 0;
  zumir_r_fmod_warned = 0;
  /* ISO C has no cast from an object pointer to a function pointer; the
   * bytes are copied instead (the address MIR returned is code). */
  rf_fn fn;
  memcpy(&fn, &z->fn, sizeof fn);
  int64_t status = fn(REAL(consts), REAL(scalars), vp, vl, r);
  int nan = zumir_r_nan_warned, fmod = zumir_r_fmod_warned;
  zumir_r_nan_warned = 0;
  zumir_r_fmod_warned = 0;
  double value;
  if (Rf_asLogical(vector_result) == TRUE)
    value = (double) r[0];
  else
    memcpy(&value, &r[0], sizeof value);
  const char *names[] = {"status", "value", "nan", "fmod", "poll", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_ScalarInteger((int) status));
  SET_VECTOR_ELT(res, 1, Rf_ScalarReal(value));
  SET_VECTOR_ELT(res, 2, Rf_ScalarLogical(nan));
  SET_VECTOR_ELT(res, 3, Rf_ScalarLogical(fmod));
  /* The poll's error message (a time limit), "" for an interrupt. */
  SET_VECTOR_ELT(res, 4, Rf_mkString(zumir_r_poll_message));
  zumir_r_poll_message[0] = '\0';
  UNPROTECT(1);
  return res;
}
#else
SEXP zumir_rf_new(SEXP text, SEXP tier, SEXP path) {
  (void) text;
  (void) tier;
  (void) path;
  return zumir_status("context", 0, "zumir was built without MIR: no target");
}
SEXP zumir_rf_build(SEXP spec, SEXP tier, SEXP path) {
  (void) spec;
  (void) tier;
  (void) path;
  return zumir_status("context", 0, "zumir was built without MIR: no target");
}
SEXP zumir_rf_call(SEXP x, SEXP consts, SEXP scalars, SEXP vectors, SEXP vector_result) {
  (void) x;
  (void) consts;
  (void) scalars;
  (void) vectors;
  (void) vector_result;
  return zumir_status("context", 0, "zumir was built without MIR");
}
#endif
