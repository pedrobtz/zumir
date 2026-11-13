/* zumir.h: MIR for R packages, through zumir's C API (design §4.3).
 *
 * A package that generates code puts `LinkingTo: zumir` in DESCRIPTION,
 * includes this header, and fetches the table once:
 *
 *   const char *why;
 *   if (zumir_api_load(&why) == NULL) Rf_error("zumir: %s", why);
 *   int status;
 *   zumir_ctx *z = zumir_ctx_new(&status);
 *   MIR_context_t ctx = zumir_mir(z);
 *   MIR_module_t mod = MIR_new_module(ctx, "m");
 *   ... MIR_new_func_arr, MIR_append_insn, MIR_finish_func ...
 *   MIR_finish_module(ctx);
 *   int tier;
 *   void *fn = zumir_compile(z, mod, "rhs", ZUMIR_TIER_AUTO, &tier, &status);
 *   ... call fn: a C function pointer on either tier, or NULL ...
 *   zumir_ctx_free(z);   frees the code with the context
 *
 * The MIR_ names below are not MIR's functions: they are zumir's shims
 * with MIR's names and signatures (D20). Each traps MIR's errors; after
 * one, the context is failed, every MIR_ call on it does nothing and
 * returns a placeholder (never NULL for an item, so `fi->u.func` stays
 * safe), zumir_compile() returns NULL with the recorded status, and
 * zumir_status() gives the message. A failed context can only be freed.
 * No call longjmps across the caller's frames.
 *
 * Standalone C99, and C++. Each translation unit that uses the MIR_ macros
 * calls zumir_api_load() first: the table pointer is per unit. Define
 * ZUMIR_NO_MIR_MACROS to call through the table explicitly instead
 * (zumir_api_->MIR_new_module(...)).
 *
 * The table only grows (D21): fields are appended, never moved, and a
 * consumer reads a field only when `size` says it is there. MIR's types
 * come from zumir/mir.h, the copy zumir is built with; ZUMIR_MIR_ABI
 * changes whenever a type a consumer compiles in from it changes, and
 * zumir_api_get() then refuses a consumer built against another. */
#ifndef ZUMIR_H
#define ZUMIR_H

#include <stddef.h>
#include <stdint.h>

#include <R_ext/Rdynload.h>

#include "zumir/mir.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZUMIR_API_VERSION 1
/* Counts changes to what a consumer compiles in from zumir/mir.h. */
#define ZUMIR_MIR_ABI 1

/* Status codes: 0 is success. */
enum {
  ZUMIR_OK = 0,
  ZUMIR_E_MIR = 1,      /* MIR reported an error; zumir_status() has it */
  ZUMIR_E_FAILED = 2,   /* the context failed earlier; free it */
  ZUMIR_E_NO_TIER = 3,  /* no tier runs here (zumir_tier() is "none") */
  ZUMIR_E_NOT_FOUND = 4, /* no function of that name in the module */
  ZUMIR_E_ARG = 5,      /* a NULL or invalid argument */
  ZUMIR_E_NO_MEMORY = 6
};

enum { ZUMIR_TIER_AUTO = 0, ZUMIR_TIER_NATIVE = 1, ZUMIR_TIER_INTERP = 2 };

/* Capability flags (design §4.4): bit k for the native tier, bit 4 + k for
 * the interpreter, k = 0 compile, 1 entry, 2 run, 3 external. */
#define ZUMIR_CAP_COMPILE 0x1u
#define ZUMIR_CAP_ENTRY 0x2u
#define ZUMIR_CAP_RUN 0x4u
#define ZUMIR_CAP_EXTERNAL 0x8u
#define ZUMIR_CAPS_NATIVE 0x0fu
#define ZUMIR_CAPS_INTERP 0xf0u

typedef struct zumir_ctx zumir_ctx;

typedef struct zumir_api_v1 {
  /* Never moved: */
  uint32_t version; /* ZUMIR_API_VERSION of the provider */
  uint32_t size;    /* sizeof the provider's table */
  uint32_t mir_abi; /* ZUMIR_MIR_ABI of the provider */
  uint32_t capabilities;

  /* zumir's own entries; each returns a status or sets *status. */
  zumir_ctx *(*zumir_ctx_new)(int *status);
  void (*zumir_ctx_free)(zumir_ctx *z); /* NULL, or twice: does nothing */
  MIR_context_t (*zumir_mir)(zumir_ctx *z);
  int (*zumir_status)(zumir_ctx *z, const char **message);
  void *(*zumir_compile)(zumir_ctx *z, MIR_module_t module, const char *name, int tier,
                         int *tier_out, int *status);
  int (*zumir_import)(zumir_ctx *z, const char *name, void *addr);

  /* MIR's API, as shims (D20). */
  MIR_module_t (*MIR_new_module)(MIR_context_t ctx, const char *name);
  void (*MIR_finish_module)(MIR_context_t ctx);
  MIR_item_t (*MIR_new_import)(MIR_context_t ctx, const char *name);
  MIR_item_t (*MIR_new_export)(MIR_context_t ctx, const char *name);
  MIR_item_t (*MIR_new_forward)(MIR_context_t ctx, const char *name);
  MIR_item_t (*MIR_new_bss)(MIR_context_t ctx, const char *name, size_t len);
  MIR_item_t (*MIR_new_data)(MIR_context_t ctx, const char *name, MIR_type_t el_type, size_t nel,
                             const void *els);
  MIR_item_t (*MIR_new_string_data)(MIR_context_t ctx, const char *name, MIR_str_t str);
  MIR_item_t (*MIR_new_proto_arr)(MIR_context_t ctx, const char *name, size_t nres,
                                  MIR_type_t *res_types, size_t nargs, MIR_var_t *vars);
  MIR_item_t (*MIR_new_proto)(MIR_context_t ctx, const char *name, size_t nres,
                              MIR_type_t *res_types, size_t nargs, ...);
  MIR_item_t (*MIR_new_vararg_proto_arr)(MIR_context_t ctx, const char *name, size_t nres,
                                         MIR_type_t *res_types, size_t nargs, MIR_var_t *vars);
  MIR_item_t (*MIR_new_func_arr)(MIR_context_t ctx, const char *name, size_t nres,
                                 MIR_type_t *res_types, size_t nargs, MIR_var_t *vars);
  MIR_item_t (*MIR_new_func)(MIR_context_t ctx, const char *name, size_t nres,
                             MIR_type_t *res_types, size_t nargs, ...);
  MIR_item_t (*MIR_new_vararg_func_arr)(MIR_context_t ctx, const char *name, size_t nres,
                                        MIR_type_t *res_types, size_t nargs, MIR_var_t *vars);
  void (*MIR_finish_func)(MIR_context_t ctx);
  MIR_reg_t (*MIR_new_func_reg)(MIR_context_t ctx, MIR_func_t func, MIR_type_t type,
                                const char *name);
  MIR_reg_t (*MIR_reg)(MIR_context_t ctx, const char *reg_name, MIR_func_t func);
  MIR_type_t (*MIR_reg_type)(MIR_context_t ctx, MIR_reg_t reg, MIR_func_t func);
  const char *(*MIR_item_name)(MIR_context_t ctx, MIR_item_t item);
  MIR_item_t (*MIR_get_module_item)(MIR_context_t ctx, MIR_module_t module, const char *name);
  MIR_insn_t (*MIR_new_insn_arr)(MIR_context_t ctx, MIR_insn_code_t code, size_t nops,
                                 MIR_op_t *ops);
  MIR_insn_t (*MIR_new_insn)(MIR_context_t ctx, MIR_insn_code_t code, ...);
  MIR_insn_t (*MIR_new_call_insn)(MIR_context_t ctx, size_t nops, ...);
  MIR_insn_t (*MIR_new_jcall_insn)(MIR_context_t ctx, size_t nops, ...);
  MIR_insn_t (*MIR_new_ret_insn)(MIR_context_t ctx, size_t nops, ...);
  MIR_insn_t (*MIR_new_label)(MIR_context_t ctx);
  void (*MIR_append_insn)(MIR_context_t ctx, MIR_item_t func, MIR_insn_t insn);
  void (*MIR_prepend_insn)(MIR_context_t ctx, MIR_item_t func, MIR_insn_t insn);
  void (*MIR_insert_insn_after)(MIR_context_t ctx, MIR_item_t func, MIR_insn_t after,
                                MIR_insn_t insn);
  void (*MIR_insert_insn_before)(MIR_context_t ctx, MIR_item_t func, MIR_insn_t before,
                                 MIR_insn_t insn);
  void (*MIR_remove_insn)(MIR_context_t ctx, MIR_item_t func, MIR_insn_t insn);
  MIR_op_t (*MIR_new_reg_op)(MIR_context_t ctx, MIR_reg_t reg);
  MIR_op_t (*MIR_new_int_op)(MIR_context_t ctx, int64_t v);
  MIR_op_t (*MIR_new_uint_op)(MIR_context_t ctx, uint64_t v);
  MIR_op_t (*MIR_new_float_op)(MIR_context_t ctx, float v);
  MIR_op_t (*MIR_new_double_op)(MIR_context_t ctx, double v);
  MIR_op_t (*MIR_new_ref_op)(MIR_context_t ctx, MIR_item_t item);
  MIR_op_t (*MIR_new_mem_op)(MIR_context_t ctx, MIR_type_t type, MIR_disp_t disp, MIR_reg_t base,
                             MIR_reg_t index, MIR_scale_t scale);
  MIR_op_t (*MIR_new_label_op)(MIR_context_t ctx, MIR_label_t label);
  const char *(*MIR_insn_name)(MIR_context_t ctx, MIR_insn_code_t code);
  void (*MIR_output_module)(MIR_context_t ctx, FILE *f, MIR_module_t module);
} zumir_api_v1;

/* The getter registered with R_RegisterCCallable("zumir", "zumir_api_get"):
 * the table, or NULL with *why set when this zumir cannot serve `version`
 * or was built against another MIR ABI. */
typedef const zumir_api_v1 *(*zumir_api_get_t)(uint32_t version, uint32_t mir_abi,
                                                const char **why);

/* This unit's table pointer, set by zumir_api_load(). */
static const zumir_api_v1 *zumir_api_ = NULL;

static inline const zumir_api_v1 *zumir_api_load(const char **why) {
  /* Through void (*)(void), the cast gcc and clang accept between function
   * pointer types under -Wcast-function-type. */
  zumir_api_get_t get =
    (zumir_api_get_t) (void (*)(void)) R_GetCCallable("zumir", "zumir_api_get");
  const char *reason = "zumir_api_get is not registered";
  const zumir_api_v1 *api = NULL;
  if (get != NULL) api = get(ZUMIR_API_VERSION, ZUMIR_MIR_ABI, &reason);
  if (why != NULL) *why = api == NULL ? reason : NULL;
  zumir_api_ = api;
  return api;
}

#ifndef ZUMIR_NO_MIR_MACROS
#define zumir_ctx_new zumir_api_->zumir_ctx_new
#define zumir_ctx_free zumir_api_->zumir_ctx_free
#define zumir_mir zumir_api_->zumir_mir
#define zumir_status zumir_api_->zumir_status
#define zumir_compile zumir_api_->zumir_compile
#define zumir_import zumir_api_->zumir_import
#define MIR_new_module zumir_api_->MIR_new_module
#define MIR_finish_module zumir_api_->MIR_finish_module
#define MIR_new_import zumir_api_->MIR_new_import
#define MIR_new_export zumir_api_->MIR_new_export
#define MIR_new_forward zumir_api_->MIR_new_forward
#define MIR_new_bss zumir_api_->MIR_new_bss
#define MIR_new_data zumir_api_->MIR_new_data
#define MIR_new_string_data zumir_api_->MIR_new_string_data
#define MIR_new_proto_arr zumir_api_->MIR_new_proto_arr
#define MIR_new_proto zumir_api_->MIR_new_proto
#define MIR_new_vararg_proto_arr zumir_api_->MIR_new_vararg_proto_arr
#define MIR_new_func_arr zumir_api_->MIR_new_func_arr
#define MIR_new_func zumir_api_->MIR_new_func
#define MIR_new_vararg_func_arr zumir_api_->MIR_new_vararg_func_arr
#define MIR_finish_func zumir_api_->MIR_finish_func
#define MIR_new_func_reg zumir_api_->MIR_new_func_reg
#define MIR_reg zumir_api_->MIR_reg
#define MIR_reg_type zumir_api_->MIR_reg_type
#define MIR_item_name zumir_api_->MIR_item_name
#define MIR_get_module_item zumir_api_->MIR_get_module_item
#define MIR_new_insn_arr zumir_api_->MIR_new_insn_arr
#define MIR_new_insn zumir_api_->MIR_new_insn
#define MIR_new_call_insn zumir_api_->MIR_new_call_insn
#define MIR_new_jcall_insn zumir_api_->MIR_new_jcall_insn
#define MIR_new_ret_insn zumir_api_->MIR_new_ret_insn
#define MIR_new_label zumir_api_->MIR_new_label
#define MIR_append_insn zumir_api_->MIR_append_insn
#define MIR_prepend_insn zumir_api_->MIR_prepend_insn
#define MIR_insert_insn_after zumir_api_->MIR_insert_insn_after
#define MIR_insert_insn_before zumir_api_->MIR_insert_insn_before
#define MIR_remove_insn zumir_api_->MIR_remove_insn
#define MIR_new_reg_op zumir_api_->MIR_new_reg_op
#define MIR_new_int_op zumir_api_->MIR_new_int_op
#define MIR_new_uint_op zumir_api_->MIR_new_uint_op
#define MIR_new_float_op zumir_api_->MIR_new_float_op
#define MIR_new_double_op zumir_api_->MIR_new_double_op
#define MIR_new_ref_op zumir_api_->MIR_new_ref_op
#define MIR_new_mem_op zumir_api_->MIR_new_mem_op
#define MIR_new_label_op zumir_api_->MIR_new_label_op
#define MIR_insn_name zumir_api_->MIR_insn_name
#define MIR_output_module zumir_api_->MIR_output_module

/* ---- The double emitter (D13) -------------------------------------------
 * Straight-line double code with branches, as a lowering from an IR needs:
 * registers, loads and stores at an index from a pointer register (y[i],
 * par[k]), the four operations, negation, constants, comparisons,
 * branches, and calls to the math table by name, each import and its
 * prototype made once per emitter. Every helper goes through the shims, so
 * after an error it does nothing; check once, at zumir_compile(). */

#define ZUMIR_EMIT_MAX_CALLS 32

typedef struct zumir_emitter {
  MIR_context_t ctx;
  MIR_item_t func;
  MIR_func_t f;
  unsigned ntmp;
  int ncalls;
  struct {
    const char *name;
    int nargs;
    MIR_item_t proto, import;
  } calls[ZUMIR_EMIT_MAX_CALLS];
} zumir_emitter;

/* Starts emitting into `func` (from MIR_new_func_arr), in `ctx`. */
static inline void zumir_emit_begin(zumir_emitter *e, MIR_context_t ctx, MIR_item_t func) {
  e->ctx = ctx;
  e->func = func;
  e->f = func->u.func;
  e->ntmp = 0;
  e->ncalls = 0;
}

static inline MIR_reg_t zumir_emit_reg(zumir_emitter *e, MIR_type_t type) {
  char name[32];
  unsigned n = e->ntmp++;
  /* "z" then the number in decimal: no snprintf, nothing locale-bound. */
  int k = 0;
  char digits[16];
  do {
    digits[k++] = (char) ('0' + n % 10);
    n /= 10;
  } while (n > 0 && k < 15);
  name[0] = '_';
  name[1] = 'z';
  for (int i = 0; i < k; i++) name[2 + i] = digits[k - 1 - i];
  name[2 + k] = '\0';
  return MIR_new_func_reg(e->ctx, e->f, type, name);
}

static inline void zumir_emit_insn(zumir_emitter *e, MIR_insn_t insn) {
  MIR_append_insn(e->ctx, e->func, insn);
}

static inline MIR_op_t zumir_emit_r(zumir_emitter *e, MIR_reg_t r) {
  return MIR_new_reg_op(e->ctx, r);
}

/* base[index], doubles: base is a pointer register. */
static inline MIR_op_t zumir_emit_at(zumir_emitter *e, MIR_reg_t base, long index) {
  return MIR_new_mem_op(e->ctx, MIR_T_D, (MIR_disp_t) index * (MIR_disp_t) sizeof(double), base,
                        0, 1);
}

static inline MIR_reg_t zumir_emit_load(zumir_emitter *e, MIR_reg_t base, long index) {
  MIR_reg_t r = zumir_emit_reg(e, MIR_T_D);
  zumir_emit_insn(e, MIR_new_insn(e->ctx, MIR_DMOV, zumir_emit_r(e, r), zumir_emit_at(e, base, index)));
  return r;
}

static inline void zumir_emit_store(zumir_emitter *e, MIR_reg_t base, long index, MIR_reg_t r) {
  zumir_emit_insn(e, MIR_new_insn(e->ctx, MIR_DMOV, zumir_emit_at(e, base, index), zumir_emit_r(e, r)));
}

static inline MIR_reg_t zumir_emit_const(zumir_emitter *e, double v) {
  MIR_reg_t r = zumir_emit_reg(e, MIR_T_D);
  zumir_emit_insn(e, MIR_new_insn(e->ctx, MIR_DMOV, zumir_emit_r(e, r), MIR_new_double_op(e->ctx, v)));
  return r;
}

/* code is MIR_DADD, MIR_DSUB, MIR_DMUL or MIR_DDIV. */
static inline MIR_reg_t zumir_emit_op(zumir_emitter *e, MIR_insn_code_t code, MIR_reg_t a,
                                      MIR_reg_t b) {
  MIR_reg_t r = zumir_emit_reg(e, MIR_T_D);
  zumir_emit_insn(e, MIR_new_insn(e->ctx, code, zumir_emit_r(e, r), zumir_emit_r(e, a),
                                  zumir_emit_r(e, b)));
  return r;
}

static inline MIR_reg_t zumir_emit_add(zumir_emitter *e, MIR_reg_t a, MIR_reg_t b) {
  return zumir_emit_op(e, MIR_DADD, a, b);
}
static inline MIR_reg_t zumir_emit_sub(zumir_emitter *e, MIR_reg_t a, MIR_reg_t b) {
  return zumir_emit_op(e, MIR_DSUB, a, b);
}
static inline MIR_reg_t zumir_emit_mul(zumir_emitter *e, MIR_reg_t a, MIR_reg_t b) {
  return zumir_emit_op(e, MIR_DMUL, a, b);
}
static inline MIR_reg_t zumir_emit_div(zumir_emitter *e, MIR_reg_t a, MIR_reg_t b) {
  return zumir_emit_op(e, MIR_DDIV, a, b);
}

static inline MIR_reg_t zumir_emit_neg(zumir_emitter *e, MIR_reg_t a) {
  MIR_reg_t r = zumir_emit_reg(e, MIR_T_D);
  zumir_emit_insn(e, MIR_new_insn(e->ctx, MIR_DNEG, zumir_emit_r(e, r), zumir_emit_r(e, a)));
  return r;
}

/* An int64 register holding 1 or 0: code is MIR_DEQ, MIR_DNE, MIR_DLT,
 * MIR_DLE, MIR_DGT or MIR_DGE, with C's meaning on NaN (only DNE holds). */
static inline MIR_reg_t zumir_emit_cmp(zumir_emitter *e, MIR_insn_code_t code, MIR_reg_t a,
                                       MIR_reg_t b) {
  MIR_reg_t r = zumir_emit_reg(e, MIR_T_I64);
  zumir_emit_insn(e, MIR_new_insn(e->ctx, code, zumir_emit_r(e, r), zumir_emit_r(e, a),
                                  zumir_emit_r(e, b)));
  return r;
}

static inline MIR_label_t zumir_emit_label(zumir_emitter *e) {
  return MIR_new_label(e->ctx);
}

/* Places a label made by zumir_emit_label() here. */
static inline void zumir_emit_place(zumir_emitter *e, MIR_label_t label) {
  zumir_emit_insn(e, label);
}

static inline void zumir_emit_jump(zumir_emitter *e, MIR_label_t label) {
  zumir_emit_insn(e, MIR_new_insn(e->ctx, MIR_JMP, MIR_new_label_op(e->ctx, label)));
}

/* Branches to label when `a code b`: code is MIR_DBEQ, MIR_DBNE, MIR_DBLT,
 * MIR_DBLE, MIR_DBGT or MIR_DBGE, C's meaning on NaN. */
static inline void zumir_emit_branch(zumir_emitter *e, MIR_insn_code_t code, MIR_reg_t a,
                                     MIR_reg_t b, MIR_label_t label) {
  zumir_emit_insn(e, MIR_new_insn(e->ctx, code, MIR_new_label_op(e->ctx, label),
                                  zumir_emit_r(e, a), zumir_emit_r(e, b)));
}

/* Branches to label when the int64 register c is nonzero (from
 * zumir_emit_cmp). */
static inline void zumir_emit_branch_true(zumir_emitter *e, MIR_reg_t c, MIR_label_t label) {
  zumir_emit_insn(e, MIR_new_insn(e->ctx, MIR_BT, MIR_new_label_op(e->ctx, label),
                                  zumir_emit_r(e, c)));
}

/* r = a: copies a double register. */
static inline void zumir_emit_move(zumir_emitter *e, MIR_reg_t r, MIR_reg_t a) {
  zumir_emit_insn(e, MIR_new_insn(e->ctx, MIR_DMOV, zumir_emit_r(e, r), zumir_emit_r(e, a)));
}

/* name(args...) for a function of nargs doubles returning a double,
 * resolved at zumir_compile() from the context's imports: the C math
 * library, or what the consumer registered with zumir_import(). */
static inline MIR_reg_t zumir_emit_call(zumir_emitter *e, const char *name, int nargs,
                                        const MIR_reg_t *args) {
  int k;
  MIR_op_t ops[3 + 4];
  MIR_reg_t r = zumir_emit_reg(e, MIR_T_D);
  if (nargs < 0 || nargs > 4) return r;
  for (k = 0; k < e->ncalls; k++)
    if (e->calls[k].nargs == nargs && e->calls[k].name[0] == name[0]) {
      const char *a = e->calls[k].name, *b = name;
      while (*a != '\0' && *a == *b) a++, b++;
      if (*a == *b) break;
    }
  if (k == e->ncalls) {
    MIR_type_t res = MIR_T_D;
    MIR_var_t vars[4];
    char proto_name[64];
    const char *p = "_zp_";
    int n = 0;
    if (k == ZUMIR_EMIT_MAX_CALLS) return r;
    for (int i = 0; i < nargs; i++) {
      vars[i].type = MIR_T_D;
      vars[i].name = i == 0 ? "a" : i == 1 ? "b" : i == 2 ? "c" : "d";
      vars[i].size = 0;
    }
    while (*p != '\0') proto_name[n++] = *p++;
    for (p = name; *p != '\0' && n < 62; p++) proto_name[n++] = *p;
    proto_name[n++] = (char) ('0' + nargs);
    proto_name[n] = '\0';
    e->calls[k].name = name;
    e->calls[k].nargs = nargs;
    e->calls[k].proto = MIR_new_proto_arr(e->ctx, proto_name, 1, &res, (size_t) nargs, vars);
    e->calls[k].import = MIR_new_import(e->ctx, name);
    e->ncalls++;
  }
  ops[0] = MIR_new_ref_op(e->ctx, e->calls[k].proto);
  ops[1] = MIR_new_ref_op(e->ctx, e->calls[k].import);
  ops[2] = zumir_emit_r(e, r);
  for (int i = 0; i < nargs; i++) ops[3 + i] = zumir_emit_r(e, args[i]);
  zumir_emit_insn(e, MIR_new_insn_arr(e->ctx, MIR_CALL, (size_t) (3 + nargs), ops));
  return r;
}

static inline MIR_reg_t zumir_emit_call1(zumir_emitter *e, const char *name, MIR_reg_t a) {
  return zumir_emit_call(e, name, 1, &a);
}

static inline MIR_reg_t zumir_emit_call2(zumir_emitter *e, const char *name, MIR_reg_t a,
                                         MIR_reg_t b) {
  MIR_reg_t args[2];
  args[0] = a;
  args[1] = b;
  return zumir_emit_call(e, name, 2, args);
}

/* return; ends a void function. */
static inline void zumir_emit_return(zumir_emitter *e) {
  zumir_emit_insn(e, MIR_new_ret_insn(e->ctx, 0));
}

/* return r; ends a function returning a double. */
static inline void zumir_emit_return_value(zumir_emitter *e, MIR_reg_t r) {
  zumir_emit_insn(e, MIR_new_ret_insn(e->ctx, 1, zumir_emit_r(e, r)));
}
#endif /* ZUMIR_NO_MIR_MACROS */

#ifdef __cplusplus
}
#endif

#endif
