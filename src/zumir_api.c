/* The C API table (design §4.3, D3, D20, D21): zumir_api_v1, registered
 * with R_RegisterCCallable("zumir", "zumir_api_get"). Its MIR_ entries are
 * shims with MIR's names and signatures. Each finds the zumir context that
 * owns the MIR context (the arena's owner), does nothing on a failed one,
 * and otherwise sets the trap, calls MIR and clears the trap. When MIR
 * raises, the shim records the error in the context, which is failed from
 * then on, and returns a placeholder: zero for a register or an operand,
 * and for an item, module, function or instruction a static zeroed object
 * (an item's u.func points at a zeroed function), so that consumer code
 * such as `fi->u.func` never dereferences NULL. No longjmp crosses the
 * consumer's frames: a trap lives inside one shim call. */
#include <stdarg.h>
#include <string.h>

#include "zumir_ctx.h"

#if ZUMIR_HAVE_MIR
#define ZUMIR_NO_MIR_MACROS
#include "../inst/include/zumir.h"
#include "zumir_trap.h"

size_t zumir_mir_code_nops(MIR_context_t ctx, MIR_insn_code_t code);
unsigned zumir_probe_flags(void);

/* ---- Placeholders for a failed context ---- */
static struct MIR_func dummy_func;
static struct MIR_item dummy_item;
static struct MIR_module dummy_module;
/* An insn has a flexible array of operands, so its placeholder is bytes. */
static max_align_t dummy_insn_space[(sizeof(struct MIR_insn) + 4 * sizeof(MIR_op_t)) /
                                      sizeof(max_align_t) + 1];
static const MIR_op_t zero_op;

static void init_dummies(void) {
  dummy_item.u.func = &dummy_func;
}

static zumir_rctx *owner(MIR_context_t ctx) {
  return ctx == NULL ? NULL : (zumir_rctx *) zumir_mir_owner(ctx);
}

/* A shim: RET name(PARAMS), calling MIR's own CALL; FAIL when the context
 * is failed or MIR raises. */
#define SHIM(RET, NAME, FAIL, PARAMS, CALL)          \
  static RET shim_##NAME PARAMS {                    \
    zumir_rctx *z = owner(ctx);                      \
    if (z == NULL || z->failed) return FAIL;         \
    zumir_trap t;                                    \
    zumir_trap_push(&t);                             \
    if (setjmp(t.env) == 0) {                        \
      RET r = CALL;                                  \
      zumir_trap_pop(&t);                            \
      return r;                                      \
    }                                                \
    zumir_rctx_fail(z, &t);                          \
    return FAIL;                                     \
  }

#define SHIMV(NAME, PARAMS, CALL)                    \
  static void shim_##NAME PARAMS {                   \
    zumir_rctx *z = owner(ctx);                      \
    if (z == NULL || z->failed) return;              \
    zumir_trap t;                                    \
    zumir_trap_push(&t);                             \
    if (setjmp(t.env) == 0) {                        \
      CALL;                                          \
      zumir_trap_pop(&t);                            \
      return;                                        \
    }                                                \
    zumir_rctx_fail(z, &t);                          \
  }

#define ITEM_FAIL (&dummy_item)
#define INSN_FAIL ((MIR_insn_t) (void *) dummy_insn_space)

SHIM(MIR_module_t, MIR_new_module, &dummy_module, (MIR_context_t ctx, const char *name),
     MIR_new_module(ctx, name))
SHIMV(MIR_finish_module, (MIR_context_t ctx), MIR_finish_module(ctx))
SHIM(MIR_item_t, MIR_new_import, ITEM_FAIL, (MIR_context_t ctx, const char *name),
     MIR_new_import(ctx, name))
SHIM(MIR_item_t, MIR_new_export, ITEM_FAIL, (MIR_context_t ctx, const char *name),
     MIR_new_export(ctx, name))
SHIM(MIR_item_t, MIR_new_forward, ITEM_FAIL, (MIR_context_t ctx, const char *name),
     MIR_new_forward(ctx, name))
SHIM(MIR_item_t, MIR_new_bss, ITEM_FAIL, (MIR_context_t ctx, const char *name, size_t len),
     MIR_new_bss(ctx, name, len))
SHIM(MIR_item_t, MIR_new_data, ITEM_FAIL,
     (MIR_context_t ctx, const char *name, MIR_type_t el_type, size_t nel, const void *els),
     MIR_new_data(ctx, name, el_type, nel, els))
SHIM(MIR_item_t, MIR_new_string_data, ITEM_FAIL,
     (MIR_context_t ctx, const char *name, MIR_str_t str), MIR_new_string_data(ctx, name, str))
SHIM(MIR_item_t, MIR_new_proto_arr, ITEM_FAIL,
     (MIR_context_t ctx, const char *name, size_t nres, MIR_type_t *res_types, size_t nargs,
      MIR_var_t *vars),
     MIR_new_proto_arr(ctx, name, nres, res_types, nargs, vars))
SHIM(MIR_item_t, MIR_new_vararg_proto_arr, ITEM_FAIL,
     (MIR_context_t ctx, const char *name, size_t nres, MIR_type_t *res_types, size_t nargs,
      MIR_var_t *vars),
     MIR_new_vararg_proto_arr(ctx, name, nres, res_types, nargs, vars))
SHIM(MIR_item_t, MIR_new_func_arr, ITEM_FAIL,
     (MIR_context_t ctx, const char *name, size_t nres, MIR_type_t *res_types, size_t nargs,
      MIR_var_t *vars),
     MIR_new_func_arr(ctx, name, nres, res_types, nargs, vars))
SHIM(MIR_item_t, MIR_new_vararg_func_arr, ITEM_FAIL,
     (MIR_context_t ctx, const char *name, size_t nres, MIR_type_t *res_types, size_t nargs,
      MIR_var_t *vars),
     MIR_new_vararg_func_arr(ctx, name, nres, res_types, nargs, vars))
SHIMV(MIR_finish_func, (MIR_context_t ctx), MIR_finish_func(ctx))
SHIM(MIR_reg_t, MIR_new_func_reg, 0,
     (MIR_context_t ctx, MIR_func_t func, MIR_type_t type, const char *name),
     MIR_new_func_reg(ctx, func, type, name))
SHIM(MIR_reg_t, MIR_reg, 0, (MIR_context_t ctx, const char *reg_name, MIR_func_t func),
     MIR_reg(ctx, reg_name, func))
SHIM(MIR_type_t, MIR_reg_type, MIR_T_UNDEF, (MIR_context_t ctx, MIR_reg_t reg, MIR_func_t func),
     MIR_reg_type(ctx, reg, func))
SHIM(const char *, MIR_item_name, "", (MIR_context_t ctx, MIR_item_t item),
     MIR_item_name(ctx, item))
SHIM(MIR_item_t, MIR_get_module_item, NULL,
     (MIR_context_t ctx, MIR_module_t module, const char *name),
     MIR_get_module_item(ctx, module, name))
SHIM(MIR_insn_t, MIR_new_insn_arr, INSN_FAIL,
     (MIR_context_t ctx, MIR_insn_code_t code, size_t nops, MIR_op_t *ops),
     MIR_new_insn_arr(ctx, code, nops, ops))
SHIM(MIR_insn_t, MIR_new_label, INSN_FAIL, (MIR_context_t ctx), MIR_new_label(ctx))
SHIMV(MIR_append_insn, (MIR_context_t ctx, MIR_item_t func, MIR_insn_t insn),
      MIR_append_insn(ctx, func, insn))
SHIMV(MIR_prepend_insn, (MIR_context_t ctx, MIR_item_t func, MIR_insn_t insn),
      MIR_prepend_insn(ctx, func, insn))
SHIMV(MIR_insert_insn_after,
      (MIR_context_t ctx, MIR_item_t func, MIR_insn_t after, MIR_insn_t insn),
      MIR_insert_insn_after(ctx, func, after, insn))
SHIMV(MIR_insert_insn_before,
      (MIR_context_t ctx, MIR_item_t func, MIR_insn_t before, MIR_insn_t insn),
      MIR_insert_insn_before(ctx, func, before, insn))
SHIMV(MIR_remove_insn, (MIR_context_t ctx, MIR_item_t func, MIR_insn_t insn),
      MIR_remove_insn(ctx, func, insn))
SHIM(MIR_op_t, MIR_new_reg_op, zero_op, (MIR_context_t ctx, MIR_reg_t reg),
     MIR_new_reg_op(ctx, reg))
SHIM(MIR_op_t, MIR_new_int_op, zero_op, (MIR_context_t ctx, int64_t v), MIR_new_int_op(ctx, v))
SHIM(MIR_op_t, MIR_new_uint_op, zero_op, (MIR_context_t ctx, uint64_t v),
     MIR_new_uint_op(ctx, v))
SHIM(MIR_op_t, MIR_new_float_op, zero_op, (MIR_context_t ctx, float v),
     MIR_new_float_op(ctx, v))
SHIM(MIR_op_t, MIR_new_double_op, zero_op, (MIR_context_t ctx, double v),
     MIR_new_double_op(ctx, v))
SHIM(MIR_op_t, MIR_new_ref_op, zero_op, (MIR_context_t ctx, MIR_item_t item),
     MIR_new_ref_op(ctx, item))
SHIM(MIR_op_t, MIR_new_mem_op, zero_op,
     (MIR_context_t ctx, MIR_type_t type, MIR_disp_t disp, MIR_reg_t base, MIR_reg_t index,
      MIR_scale_t scale),
     MIR_new_mem_op(ctx, type, disp, base, index, scale))
SHIM(MIR_op_t, MIR_new_label_op, zero_op, (MIR_context_t ctx, MIR_label_t label),
     MIR_new_label_op(ctx, label))
SHIM(const char *, MIR_insn_name, "", (MIR_context_t ctx, MIR_insn_code_t code),
     MIR_insn_name(ctx, code))
SHIMV(MIR_output_module, (MIR_context_t ctx, FILE *f, MIR_module_t module),
      MIR_output_module(ctx, f, module))

/* ---- Variadic shims: the operands are read here, then MIR's array form ---- */

#define MAX_OPS 64

/* An insn of nops operands from ap, or the failure placeholder. */
static MIR_insn_t insn_from_va(MIR_context_t ctx, MIR_insn_code_t code, size_t nops, va_list ap) {
  MIR_op_t ops[MAX_OPS];
  if (nops > MAX_OPS) {
    zumir_rctx *z = owner(ctx);
    if (z != NULL && !z->failed) {
      zumir_trap t;
      t.type = MIR_call_op_error;
      snprintf(t.message, sizeof(t.message), "zumir: more than %d operands; use MIR_new_insn_arr",
               MAX_OPS);
      zumir_rctx_fail(z, &t);
    }
    return INSN_FAIL;
  }
  for (size_t i = 0; i < nops; i++) ops[i] = va_arg(ap, MIR_op_t);
  return shim_MIR_new_insn_arr(ctx, code, nops, ops);
}

static MIR_insn_t shim_MIR_new_insn(MIR_context_t ctx, MIR_insn_code_t code, ...) {
  zumir_rctx *z = owner(ctx);
  if (z == NULL || z->failed) return INSN_FAIL;
  /* MIR_new_insn refuses calls, rets and the rest itself; reading their
   * operand count here would read nothing. */
  size_t nops = zumir_mir_code_nops(ctx, code);
  if (nops == 0 || MIR_call_code_p(code) || code == MIR_RET || code == MIR_SWITCH ||
      code == MIR_USE || code == MIR_PHI || code == MIR_UNSPEC) {
    zumir_trap t;
    t.type = MIR_call_op_error;
    snprintf(t.message, sizeof(t.message),
             "use MIR_new_insn_arr or MIR_new_{call,ret}_insn for this instruction");
    zumir_rctx_fail(z, &t);
    return INSN_FAIL;
  }
  va_list ap;
  va_start(ap, code);
  MIR_insn_t insn = insn_from_va(ctx, code, nops, ap);
  va_end(ap);
  return insn;
}

#define VARIADIC_INSN(NAME, CODE)                                          \
  static MIR_insn_t shim_##NAME(MIR_context_t ctx, size_t nops, ...) {    \
    va_list ap;                                                            \
    va_start(ap, nops);                                                    \
    MIR_insn_t insn = insn_from_va(ctx, CODE, nops, ap);                   \
    va_end(ap);                                                            \
    return insn;                                                           \
  }
VARIADIC_INSN(MIR_new_call_insn, MIR_CALL)
VARIADIC_INSN(MIR_new_jcall_insn, MIR_JCALL)
VARIADIC_INSN(MIR_new_ret_insn, MIR_RET)

/* MIR_new_func and MIR_new_proto take nargs (type, name) pairs. */
#define MAX_ARGS 64
static size_t vars_from_va(size_t nargs, va_list ap, MIR_var_t *vars) {
  if (nargs > MAX_ARGS) return (size_t) -1;
  for (size_t i = 0; i < nargs; i++) {
    vars[i].type = (MIR_type_t) va_arg(ap, int);
    vars[i].name = va_arg(ap, const char *);
    vars[i].size = 0;
  }
  return nargs;
}

static MIR_item_t too_many_args(MIR_context_t ctx) {
  zumir_rctx *z = owner(ctx);
  if (z != NULL && !z->failed) {
    zumir_trap t;
    t.type = MIR_call_op_error;
    snprintf(t.message, sizeof(t.message), "zumir: more than %d arguments; use the _arr form",
             MAX_ARGS);
    zumir_rctx_fail(z, &t);
  }
  return ITEM_FAIL;
}

static MIR_item_t shim_MIR_new_func(MIR_context_t ctx, const char *name, size_t nres,
                                    MIR_type_t *res_types, size_t nargs, ...) {
  MIR_var_t vars[MAX_ARGS];
  va_list ap;
  va_start(ap, nargs);
  size_t n = vars_from_va(nargs, ap, vars);
  va_end(ap);
  if (n == (size_t) -1) return too_many_args(ctx);
  return shim_MIR_new_func_arr(ctx, name, nres, res_types, nargs, vars);
}

static MIR_item_t shim_MIR_new_proto(MIR_context_t ctx, const char *name, size_t nres,
                                     MIR_type_t *res_types, size_t nargs, ...) {
  MIR_var_t vars[MAX_ARGS];
  va_list ap;
  va_start(ap, nargs);
  size_t n = vars_from_va(nargs, ap, vars);
  va_end(ap);
  if (n == (size_t) -1) return too_many_args(ctx);
  return shim_MIR_new_proto_arr(ctx, name, nres, res_types, nargs, vars);
}

/* ---- zumir's own entries ---- */

static zumir_ctx *api_ctx_new(int *status) {
  int st;
  zumir_rctx *z = zumir_rctx_new(-1, ZUMIR_IMPORTS_CONSUMER, &st);
  if (status != NULL) *status = st;
  return (zumir_ctx *) z;
}

static void api_ctx_free(zumir_ctx *z) {
  zumir_rctx_free((zumir_rctx *) z);
}

static MIR_context_t api_mir(zumir_ctx *z) {
  return z == NULL ? NULL : ((zumir_rctx *) z)->ctx;
}

static int api_status(zumir_ctx *z, const char **message) {
  zumir_rctx *r = (zumir_rctx *) z;
  if (r == NULL) {
    if (message != NULL) *message = "NULL context";
    return ZUMIR_E_ARG;
  }
  if (message != NULL) *message = r->failed ? r->message : "";
  return r->failed ? ZUMIR_E_MIR : ZUMIR_OK;
}

static void *api_compile(zumir_ctx *z, MIR_module_t module, const char *name, int tier,
                         int *tier_out, int *status) {
  int st, t = tier == ZUMIR_TIER_NATIVE   ? 0
              : tier == ZUMIR_TIER_INTERP ? 1
                                          : -1;
  int got = -1;
  zumir_rctx *r = (zumir_rctx *) z;
  void *fn = NULL;
  if (r == NULL || module == NULL || module == &dummy_module ||
      (tier != ZUMIR_TIER_AUTO && t < 0)) {
    st = r != NULL && r->failed ? ZUMIR_E_FAILED : ZUMIR_E_ARG;
  } else {
    fn = zumir_rctx_compile(r, module, name, t, &got, &st);
  }
  /* zumir_rctx_compile's statuses are the table's codes. */
  if (status != NULL) *status = st;
  if (tier_out != NULL)
    *tier_out = got == 0 ? ZUMIR_TIER_NATIVE : got == 1 ? ZUMIR_TIER_INTERP : ZUMIR_TIER_AUTO;
  return fn;
}

static int api_import(zumir_ctx *z, const char *name, void *addr) {
  zumir_rctx *r = (zumir_rctx *) z;
  if (r == NULL || name == NULL || addr == NULL) return ZUMIR_E_ARG;
  if (r->failed) return ZUMIR_E_FAILED;
  if (r->nextra == r->cap_extra) {
    size_t cap = r->cap_extra == 0 ? 8 : 2 * r->cap_extra;
    zumir_import_entry *e = realloc(r->extra, cap * sizeof(*e));
    if (e == NULL) return ZUMIR_E_NO_MEMORY;
    r->extra = e;
    r->cap_extra = cap;
  }
  /* The name must outlive the context's links; the consumer's string may
   * not, so it is copied. */
  size_t n = strlen(name) + 1;
  char *copy = malloc(n);
  if (copy == NULL) return ZUMIR_E_NO_MEMORY;
  memcpy(copy, name, n);
  r->extra[r->nextra].name = copy;
  r->extra[r->nextra].addr = addr;
  r->nextra++;
  return ZUMIR_OK;
}

static zumir_api_v1 table = {
  .version = ZUMIR_API_VERSION,
  .size = (uint32_t) sizeof(zumir_api_v1),
  .mir_abi = ZUMIR_MIR_ABI,
  .capabilities = 0,
  .zumir_ctx_new = api_ctx_new,
  .zumir_ctx_free = api_ctx_free,
  .zumir_mir = api_mir,
  .zumir_status = api_status,
  .zumir_compile = api_compile,
  .zumir_import = api_import,
  .MIR_new_module = shim_MIR_new_module,
  .MIR_finish_module = shim_MIR_finish_module,
  .MIR_new_import = shim_MIR_new_import,
  .MIR_new_export = shim_MIR_new_export,
  .MIR_new_forward = shim_MIR_new_forward,
  .MIR_new_bss = shim_MIR_new_bss,
  .MIR_new_data = shim_MIR_new_data,
  .MIR_new_string_data = shim_MIR_new_string_data,
  .MIR_new_proto_arr = shim_MIR_new_proto_arr,
  .MIR_new_proto = shim_MIR_new_proto,
  .MIR_new_vararg_proto_arr = shim_MIR_new_vararg_proto_arr,
  .MIR_new_func_arr = shim_MIR_new_func_arr,
  .MIR_new_func = shim_MIR_new_func,
  .MIR_new_vararg_func_arr = shim_MIR_new_vararg_func_arr,
  .MIR_finish_func = shim_MIR_finish_func,
  .MIR_new_func_reg = shim_MIR_new_func_reg,
  .MIR_reg = shim_MIR_reg,
  .MIR_reg_type = shim_MIR_reg_type,
  .MIR_item_name = shim_MIR_item_name,
  .MIR_get_module_item = shim_MIR_get_module_item,
  .MIR_new_insn_arr = shim_MIR_new_insn_arr,
  .MIR_new_insn = shim_MIR_new_insn,
  .MIR_new_call_insn = shim_MIR_new_call_insn,
  .MIR_new_jcall_insn = shim_MIR_new_jcall_insn,
  .MIR_new_ret_insn = shim_MIR_new_ret_insn,
  .MIR_new_label = shim_MIR_new_label,
  .MIR_append_insn = shim_MIR_append_insn,
  .MIR_prepend_insn = shim_MIR_prepend_insn,
  .MIR_insert_insn_after = shim_MIR_insert_insn_after,
  .MIR_insert_insn_before = shim_MIR_insert_insn_before,
  .MIR_remove_insn = shim_MIR_remove_insn,
  .MIR_new_reg_op = shim_MIR_new_reg_op,
  .MIR_new_int_op = shim_MIR_new_int_op,
  .MIR_new_uint_op = shim_MIR_new_uint_op,
  .MIR_new_float_op = shim_MIR_new_float_op,
  .MIR_new_double_op = shim_MIR_new_double_op,
  .MIR_new_ref_op = shim_MIR_new_ref_op,
  .MIR_new_mem_op = shim_MIR_new_mem_op,
  .MIR_new_label_op = shim_MIR_new_label_op,
  .MIR_insn_name = shim_MIR_insn_name,
  .MIR_output_module = shim_MIR_output_module,
};

/* The getter consumers fetch with R_GetCCallable (zumir_api_load()). */
static const zumir_api_v1 *zumir_api_get(uint32_t version, uint32_t mir_abi, const char **why) {
  static const char *newer =
    "this zumir is too old: the consumer asks for a newer C API version than it has; update zumir";
  static const char *abi =
    "the consumer was built against another MIR ABI: reinstall it from source against this "
    "zumir";
  if (version > ZUMIR_API_VERSION || version == 0) {
    if (why != NULL) *why = newer;
    return NULL;
  }
  if (mir_abi != ZUMIR_MIR_ABI) {
    if (why != NULL) *why = abi;
    return NULL;
  }
  table.capabilities = zumir_probe_flags();
  if (why != NULL) *why = NULL;
  return &table;
}

void zumir_api_register(void) {
  init_dummies();
  R_RegisterCCallable("zumir", "zumir_api_get", (DL_FUNC) zumir_api_get);
}

/* Test entry: zumir_api_get with any version and ABI, as consumers built
 * against other headers would call it. Returns the reason, or "" for a
 * table. */
SEXP zumir_test_api_get(SEXP version, SEXP mir_abi) {
  const char *why = NULL;
  const zumir_api_v1 *api =
    zumir_api_get((uint32_t) Rf_asInteger(version), (uint32_t) Rf_asInteger(mir_abi), &why);
  return Rf_mkString(api != NULL ? "" : why);
}
#else
void zumir_api_register(void) {}
SEXP zumir_test_api_get(SEXP version, SEXP mir_abi) {
  (void) version;
  (void) mir_abi;
  return Rf_mkString("zumir was built without MIR");
}
#endif
