/* MIR contexts held by R; see zumir_ctx.h. The R-facing entries are
 * internal until Stage 5's mir_function() and table: they let the tests
 * build, compile and call the spike's models on either tier, and drive a
 * context through its out-of-order sequences. */
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

int zumir_rctx_run(zumir_rctx *z, zumir_body body, void *arg) {
  if (z->failed) return 0;
  zumir_trap t;
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    body(z, arg);
    zumir_trap_pop(&t);
    return 1;
  }
  z->failed = 1;
  z->type = t.type;
  memcpy(z->message, t.message, sizeof(z->message));
  if (z->arena != NULL) z->map_errno = zumir_arena_map_errno(z->arena);
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

static void rctx_free(zumir_rctx *z) {
  if (z == NULL) return;
  zumir_mir_free(z->ctx, z->arena, z->gen);
  free(z);
}

static void finalize(SEXP x) {
  zumir_rctx *z = R_ExternalPtrAddr(x);
  R_ClearExternalPtr(x);
  rctx_free(z);
}

static void body_new(zumir_rctx *z, void *arg) {
  (void) arg;
  z->ctx = zumir_mir_new((zumir_arena *volatile *) &z->arena);
  if (z->tier == ZUMIR_TIER_NATIVE) {
    MIR_gen_init(z->ctx);
    z->gen = 1;
    MIR_gen_set_optimize_level(z->ctx, 2);
  }
}

/* A new context for `tier` ("native" or "interp"), as an external pointer
 * of class zumir_context, or a zumir_status if MIR failed to create it. */
SEXP zumir_ctx_new(SEXP s_tier) {
  const char *tier = CHAR(STRING_ELT(s_tier, 0));
  int t = strcmp(tier, "native") == 0 ? ZUMIR_TIER_NATIVE : ZUMIR_TIER_INTERP;
  /* The external pointer and its finalizer exist before the context, so
   * an R allocation error cannot strand it. */
  SEXP x = PROTECT(R_MakeExternalPtr(NULL, context_tag(), R_NilValue));
  R_RegisterCFinalizerEx(x, finalize, TRUE);
  Rf_setAttrib(x, R_ClassSymbol, Rf_mkString("zumir_context"));
  zumir_rctx *z = calloc(1, sizeof(*z));
  if (z == NULL) Rf_error("cannot allocate a zumir context");
  z->tier = t;
  R_SetExternalPtrAddr(x, z);
  if (!zumir_rctx_run(z, body_new, NULL)) {
    SEXP st = PROTECT(zumir_status("mir", z->type, z->message));
    R_ClearExternalPtr(x);
    rctx_free(z);
    UNPROTECT(2);
    return st;
  }
  UNPROTECT(1);
  return x;
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
                    ? R_ExternalPtrAddr(x) : NULL;
  SET_VECTOR_ELT(res, 0, Rf_ScalarLogical(z != NULL));
  if (z != NULL) {
    SET_VECTOR_ELT(res, 1, Rf_mkString(z->tier == ZUMIR_TIER_NATIVE ? "native" : "interp"));
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

typedef struct {
  const char *name;
  void *fn;
} link_arg;

static void body_link(zumir_rctx *z, void *arg) {
  link_arg *la = arg;
  MIR_context_t ctx = z->ctx;
  for (MIR_module_t m = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx)); m != NULL;
       m = DLIST_NEXT(MIR_module_t, m))
    MIR_load_module(ctx, m);
  MIR_load_external(ctx, "exp", (void *) exp);
  MIR_link(ctx, z->tier == ZUMIR_TIER_NATIVE ? MIR_set_gen_interface : MIR_set_interp_interface,
           NULL);
  la->fn = NULL;
  for (MIR_module_t m = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx)); m != NULL;
       m = DLIST_NEXT(MIR_module_t, m))
    for (MIR_item_t it = DLIST_HEAD(MIR_item_t, m->items); it != NULL;
         it = DLIST_NEXT(MIR_item_t, it))
      if (it->item_type == MIR_func_item && strcmp(it->u.func->name, la->name) == 0)
        la->fn = it->addr;
}

/* Loads every module, links them on the context's tier and keeps the
 * address of function `name` for zumir_ctx_call_*. */
SEXP zumir_ctx_link(SEXP x, SEXP name) {
  SEXP st;
  zumir_rctx *z = zumir_rctx_get(x, 1, &st);
  if (z == NULL) return st;
  link_arg la = {CHAR(STRING_ELT(name, 0)), NULL};
  if (!zumir_rctx_run(z, body_link, &la)) return zumir_status("mir", z->type, z->message);
  if (la.fn == NULL) return zumir_status("context", 0, "no function of that name");
  z->fn = la.fn;
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
