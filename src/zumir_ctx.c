/* zumir contexts: one MIR context, its arena, its imports and its state
 * (zumir_ctx.h). R holds them as external pointers; the C API (zumir_api.c)
 * hands them out as zumir_ctx. Every MIR call on one goes through
 * zumir_rctx_run(), under the trap. */
#include <string.h>

#include "zumir_ctx.h"
#include "zumir_trap.h"

SEXP zumir_status(const char *kind, int type, const char *message) {
  const char *names[] = {"kind", "type", "message", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_mkString(kind));
  SET_VECTOR_ELT(res, 1, Rf_ScalarInteger(type));
  SET_VECTOR_ELT(res, 2, Rf_mkString(message));
  Rf_setAttrib(res, R_ClassSymbol, Rf_mkString("zumir_status"));
  UNPROTECT(1);
  return res;
}

#if ZUMIR_HAVE_MIR
#include "vendor/mir/mir-gen.h"

static SEXP context_tag(void) {
  return Rf_install("zumir_context");
}

void zumir_rctx_fail(zumir_rctx *z, const void *trap) {
  const zumir_trap *t = trap;
  z->failed = 1;
  z->type = t->type;
  memcpy(z->message, t->message, sizeof(z->message));
  if (z->arena != NULL) {
    z->map_errno = zumir_arena_map_errno(z->arena);
    /* MIR's state after a trapped error is not trusted (design §4.3): the
     * context is released from its arena, never walked by MIR_finish. A
     * link failure on a context whose generator had started crashed
     * MIR_finish (Stage 5). */
    zumir_arena_mark_failed(z->arena);
  }
}

int zumir_rctx_run(zumir_rctx *z, zumir_body body, void *arg) {
  if (z->failed) return 0;
  zumir_trap t;
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    body(z, arg);
    zumir_trap_pop(&t);
    return 1;
  }
  zumir_rctx_fail(z, &t);
  return 0;
}

zumir_rctx *zumir_rctx_get(SEXP x, int usable, SEXP *status) {
  *status = R_NilValue;
  if (TYPEOF(x) != EXTPTRSXP || R_ExternalPtrTag(x) != context_tag()) {
    *status = zumir_status("context", 0, "not a zumir context");
    return NULL;
  }
  zumir_rctx *z = R_ExternalPtrAddr(x);
  if (z == NULL) {
    *status = zumir_status("context", 0,
                           "the context was freed, or restored from a saved session");
    return NULL;
  }
  if (usable && z->failed) {
    *status = zumir_status("context", z->type, "the context failed earlier; it can only be freed");
    return NULL;
  }
  return z;
}

void zumir_rctx_free(zumir_rctx *z) {
  if (z == NULL) return;
  zumir_mir_free(z->ctx, z->arena, z->gen);
  for (size_t i = 0; i < z->nextra; i++) free((void *) z->extra[i].name);
  free(z->extra);
  free(z->arg_types);
  free(z);
}

static void start_gen(zumir_rctx *z) {
  if (z->gen) return;
  MIR_gen_init(z->ctx);
  z->gen = 1;
  MIR_gen_set_optimize_level(z->ctx, 2);
}

static void body_new(zumir_rctx *z, void *arg) {
  (void) arg;
  z->ctx = zumir_mir_new((zumir_arena *volatile *) &z->arena);
  zumir_arena_set_owner(z->arena, z);
  if (z->tier == ZUMIR_ITIER_NATIVE) start_gen(z);
}

zumir_rctx *zumir_rctx_new(int tier, int imports, int *status) {
  zumir_rctx *z = calloc(1, sizeof(*z));
  if (z == NULL) {
    *status = ZUMIR_ST_NO_MEMORY;
    return NULL;
  }
  z->tier = tier;
  z->imports = imports;
  if (!zumir_rctx_run(z, body_new, NULL)) {
    *status = ZUMIR_ST_MIR;
    zumir_rctx_free(z);
    return NULL;
  }
  *status = ZUMIR_ST_OK;
  return z;
}

int zumir_probe_tier_ok(int tier);

typedef struct {
  MIR_module_t module; /* NULL: every module of the context */
  const char *name;
  int tier;
  void *fn;
} compile_arg;

static void body_compile(zumir_rctx *z, void *arg) {
  compile_arg *c = arg;
  MIR_context_t ctx = z->ctx;
  if (c->tier == ZUMIR_ITIER_NATIVE) start_gen(z);
  if (c->module != NULL) {
    MIR_load_module(ctx, c->module);
  } else {
    for (MIR_module_t m = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx)); m != NULL;
         m = DLIST_NEXT(MIR_module_t, m))
      MIR_load_module(ctx, m);
  }
  zumir_imports_load(ctx, z->imports, z->extra, z->nextra);
  MIR_link(ctx, c->tier == ZUMIR_ITIER_NATIVE ? MIR_set_gen_interface : MIR_set_interp_interface,
           NULL);
  c->fn = NULL;
  for (MIR_module_t m = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx)); m != NULL;
       m = DLIST_NEXT(MIR_module_t, m)) {
    if (c->module != NULL && m != c->module) continue;
    for (MIR_item_t it = DLIST_HEAD(MIR_item_t, m->items); it != NULL;
         it = DLIST_NEXT(MIR_item_t, it))
      if (it->item_type == MIR_func_item && strcmp(it->u.func->name, c->name) == 0)
        c->fn = it->addr;
  }
}

void *zumir_rctx_compile(zumir_rctx *z, MIR_module_t module, const char *name, int tier,
                         int *tier_out, int *status) {
  if (z == NULL || name == NULL) {
    *status = ZUMIR_ST_ARG;
    return NULL;
  }
  if (z->failed) {
    *status = ZUMIR_ST_FAILED;
    return NULL;
  }
  if (tier < 0) tier = z->tier;
  if (tier < 0)
    tier = zumir_probe_tier_ok(ZUMIR_ITIER_NATIVE)   ? ZUMIR_ITIER_NATIVE
           : zumir_probe_tier_ok(ZUMIR_ITIER_INTERP) ? ZUMIR_ITIER_INTERP
                                                    : -1;
  if (tier < 0 || !zumir_probe_tier_ok(tier)) {
    *status = ZUMIR_ST_NO_TIER;
    return NULL;
  }
  /* One tier per context: generation redirects the thunks linked code
   * calls through, so the tiers are not mixed (design §4.3). */
  if (z->tier >= 0 && z->tier != tier && z->fn != NULL) {
    *status = ZUMIR_ST_ARG;
    return NULL;
  }
  z->tier = tier;
  compile_arg c = {module, name, tier, NULL};
  if (!zumir_rctx_run(z, body_compile, &c)) {
    *status = ZUMIR_ST_MIR;
    return NULL;
  }
  if (c.fn == NULL) {
    *status = ZUMIR_ST_NOT_FOUND;
    return NULL;
  }
  z->fn = c.fn;
  if (tier_out != NULL) *tier_out = tier;
  *status = ZUMIR_ST_OK;
  return c.fn;
}

static void finalize(SEXP x) {
  zumir_rctx *z = R_ExternalPtrAddr(x);
  R_ClearExternalPtr(x);
  zumir_rctx_free(z);
}

/* An external pointer of class zumir_context owning a new context on
 * `tier` ("native" or "interp") with import set `imports`, or a
 * zumir_status. */
SEXP zumir_rctx_new_sexp(int tier, int imports) {
  /* The external pointer and its finalizer exist before the context, so
   * an R allocation error cannot strand it. */
  SEXP x = PROTECT(R_MakeExternalPtr(NULL, context_tag(), R_NilValue));
  R_RegisterCFinalizerEx(x, finalize, TRUE);
  Rf_setAttrib(x, R_ClassSymbol, Rf_mkString("zumir_context"));
  zumir_rctx *z = calloc(1, sizeof(*z));
  if (z == NULL) Rf_error("cannot allocate a zumir context");
  z->tier = tier;
  z->imports = imports;
  R_SetExternalPtrAddr(x, z);
  if (!zumir_rctx_run(z, body_new, NULL)) {
    SEXP st = PROTECT(zumir_status("mir", z->type, z->message));
    R_ClearExternalPtr(x);
    zumir_rctx_free(z);
    UNPROTECT(2);
    return st;
  }
  UNPROTECT(1);
  return x;
}

static int tier_code(SEXP s_tier) {
  return strcmp(CHAR(STRING_ELT(s_tier, 0)), "native") == 0 ? ZUMIR_ITIER_NATIVE
                                                             : ZUMIR_ITIER_INTERP;
}

SEXP zumir_ctx_new(SEXP s_tier) {
  return zumir_rctx_new_sexp(tier_code(s_tier), ZUMIR_IMPORTS_D6);
}

/* Frees the context now. Freeing again, or a context restored by readRDS,
 * does nothing. */
SEXP zumir_ctx_free(SEXP x) {
  if (TYPEOF(x) == EXTPTRSXP && R_ExternalPtrTag(x) == context_tag()) finalize(x);
  return R_NilValue;
}

/* list(live, tier, failed, type, message, bytes, map_errno, linked). */
SEXP zumir_ctx_state(SEXP x) {
  const char *names[] = {"live", "tier", "failed", "type", "message",
                         "bytes", "map_errno", "linked", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  zumir_rctx *z = (TYPEOF(x) == EXTPTRSXP && R_ExternalPtrTag(x) == context_tag())
                    ? R_ExternalPtrAddr(x)
                    : NULL;
  SET_VECTOR_ELT(res, 0, Rf_ScalarLogical(z != NULL));
  if (z != NULL) {
    SET_VECTOR_ELT(res, 1, Rf_mkString(z->tier == ZUMIR_ITIER_NATIVE ? "native" : "interp"));
    SET_VECTOR_ELT(res, 2, Rf_ScalarLogical(z->failed));
    SET_VECTOR_ELT(res, 3, Rf_ScalarInteger(z->type));
    SET_VECTOR_ELT(res, 4, Rf_mkString(z->message));
    SET_VECTOR_ELT(res, 5, Rf_ScalarReal(z->arena ? (double) zumir_arena_bytes(z->arena) : 0));
    SET_VECTOR_ELT(res, 6, Rf_ScalarInteger(z->map_errno));
    SET_VECTOR_ELT(res, 7, Rf_ScalarLogical(z->fn != NULL));
  }
  UNPROTECT(1);
  return res;
}

/* Bytes and contexts held across the process (zumir_info()$memory). */
SEXP zumir_memory(void) {
  const char *names[] = {"contexts", "bytes", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_ScalarReal((double) zumir_mir_live_arenas()));
  SET_VECTOR_ELT(res, 1, Rf_ScalarReal(zumir_mir_live_bytes()));
  UNPROTECT(1);
  return res;
}

static void body_module(zumir_rctx *z, void *arg) {
  MIR_new_module(z->ctx, (const char *) arg);
}

static void body_end_module(zumir_rctx *z, void *arg) {
  (void) arg;
  MIR_finish_module(z->ctx);
}

/* Starts a module: the first of two MIR calls that may have R code (and an
 * R error) between them. */
SEXP zumir_ctx_begin(SEXP x, SEXP name) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  if (!zumir_rctx_run(z, body_module, (void *) CHAR(STRING_ELT(name, 0))))
    return zumir_status("mir", z->type, z->message);
  return R_NilValue;
}

SEXP zumir_ctx_end(SEXP x) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  if (!zumir_rctx_run(z, body_end_module, NULL)) return zumir_status("mir", z->type, z->message);
  return R_NilValue;
}

static void body_scan(zumir_rctx *z, void *arg) {
  MIR_scan_string(z->ctx, (const char *) arg);
}

/* Reads MIR text into the context (MIR's own syntax errors raise). */
SEXP zumir_ctx_scan(SEXP x, SEXP text) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  if (!zumir_rctx_run(z, body_scan, (void *) CHAR(STRING_ELT(text, 0))))
    return zumir_status("mir", z->type, z->message);
  return R_NilValue;
}

/* Loads every module, links them on the context's tier with its imports,
 * and keeps the address of function `name`. */
SEXP zumir_ctx_link(SEXP x, SEXP name) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  int status;
  zumir_rctx_compile(z, NULL, CHAR(STRING_ELT(name, 0)), z->tier, NULL, &status);
  if (status == ZUMIR_ST_MIR) return zumir_status("mir", z->type, z->message);
  if (status == ZUMIR_ST_NOT_FOUND) return zumir_status("context", 0, "no function of that name");
  if (status == ZUMIR_ST_NO_TIER) return zumir_status("context", 0, "no tier runs here");
  if (status != ZUMIR_ST_OK) return zumir_status("context", 0, "cannot link");
  return R_NilValue;
}

/* Calls the linked function as double f(double). Generated code runs
 * outside the trap: it is not MIR. */
SEXP zumir_ctx_call_d(SEXP x, SEXP arg) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  if (z->fn == NULL) return zumir_status("context", 0, "nothing linked yet");
  double (*f)(double) = (double (*)(double)) z->fn;
  return Rf_ScalarReal(f(Rf_asReal(arg)));
}
#else
SEXP zumir_ctx_new(SEXP s_tier) {
  (void) s_tier;
  return zumir_status("context", 0, "zumir was built without MIR: no target");
}
SEXP zumir_ctx_free(SEXP x) {
  (void) x;
  return R_NilValue;
}
SEXP zumir_ctx_state(SEXP x) {
  (void) x;
  return R_NilValue;
}
SEXP zumir_memory(void) {
  const char *names[] = {"contexts", "bytes", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_ScalarReal(0));
  SET_VECTOR_ELT(res, 1, Rf_ScalarReal(0));
  UNPROTECT(1);
  return res;
}
#define NO_MIR_ENTRY(name, ...)                                         \
  SEXP name(__VA_ARGS__) {                                              \
    return zumir_status("context", 0, "zumir was built without MIR"); \
  }
NO_MIR_ENTRY(zumir_ctx_begin, SEXP x, SEXP name)
NO_MIR_ENTRY(zumir_ctx_end, SEXP x)
NO_MIR_ENTRY(zumir_ctx_scan, SEXP x, SEXP text)
NO_MIR_ENTRY(zumir_ctx_link, SEXP x, SEXP name)
NO_MIR_ENTRY(zumir_ctx_call_d, SEXP x, SEXP arg)
#endif
