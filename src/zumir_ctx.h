/* MIR contexts held by R (design §4.3 "Lifetimes", D20). An external
 * pointer tagged "zumir_context" owns one zumir_rctx; its finalizer frees
 * the MIR context and its arena. Every MIR call on it goes through
 * zumir_rctx_run(), which sets the trap: an error marks the context
 * failed, after which only freeing it does anything. */
#ifndef ZUMIR_CTX_H
#define ZUMIR_CTX_H

#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#include "zumir_mir.h"

#include "zumir_imports.h"

/* zumir_rctx_compile()'s statuses: inst/include/zumir.h's ZUMIR_OK and
 * ZUMIR_E_*. */
enum {
  ZUMIR_ST_OK = 0,
  ZUMIR_ST_MIR = 1,
  ZUMIR_ST_FAILED = 2,
  ZUMIR_ST_NO_TIER = 3,
  ZUMIR_ST_NOT_FOUND = 4,
  ZUMIR_ST_ARG = 5,
  ZUMIR_ST_NO_MEMORY = 6
};

#if ZUMIR_HAVE_MIR
#include "zumir_mir_api.h"

/* Tiers inside zumir; the C API's ZUMIR_TIER_AUTO/NATIVE/INTERP are 0/1/2
 * and are mapped at the table. */
enum { ZUMIR_ITIER_NATIVE = 0, ZUMIR_ITIER_INTERP = 1 };

typedef struct zumir_rctx {
  MIR_context_t ctx;
  zumir_arena *arena;
  int tier;   /* ZUMIR_ITIER_NATIVE or ZUMIR_ITIER_INTERP; -1 not chosen yet */
  int gen;    /* MIR_gen_init was called */
  int failed; /* MIR raised: only freeing does anything now */
  int type;   /* the error's MIR_error_type_t */
  char message[1024];
  int map_errno; /* errno of a refused code mapping, if any */
  void *fn;      /* the linked function, once linked */
  int imports;   /* ZUMIR_IMPORTS_* */
  zumir_import_entry *extra; /* the consumer's imports (D14) */
  size_t nextra, cap_extra;
  /* mir_function()'s signature, once built */
  int nargs, nres;
  int *arg_types, res_type;
} zumir_rctx;

/* A new zumir context with its MIR context, outside R: for the C API.
 * Returns NULL with *status set. tier is -1 (chosen at compile) or a
 * ZUMIR_ITIER_*. */
zumir_rctx *zumir_rctx_new(int tier, int imports, int *status);
void zumir_rctx_free(zumir_rctx *z);
/* Records the trap's error in z: z is failed from now on. */
void zumir_rctx_fail(zumir_rctx *z, const void *trap);
/* Loads `module`, links it on `tier` (ZUMIR_ITIER_*, or -1 for the session's
 * default) with the context's imports, and returns the address of function
 * `name` in it, or NULL with *status set. */
void *zumir_rctx_compile(zumir_rctx *z, MIR_module_t module, const char *name, int tier,
                         int *tier_out, int *status);
/* 1 when every capability of `tier` held in the load-time probe. */
int zumir_probe_tier_ok(int tier);
/* An external pointer of class zumir_context owning a new context on
 * `tier` with import set `imports`, or a zumir_status. */
SEXP zumir_rctx_new_sexp(int tier, int imports);

typedef void (*zumir_body)(zumir_rctx *z, void *arg);

/* Runs body under a trap. Returns 1 when it returned, 0 when MIR raised
 * (the context is then failed) or the context was already failed. */
int zumir_rctx_run(zumir_rctx *z, zumir_body body, void *arg);

/* The zumir_rctx behind x, or NULL with *status set to a zumir_status
 * list for R to signal: x is not a context, or it was freed or restored
 * by readRDS, or (when usable is nonzero) it is failed. */
zumir_rctx *zumir_rctx_get(SEXP x, int usable, SEXP *status);
#endif

/* list(kind, type, message) of class "zumir_status": what R/errors.R
 * turns into a condition. kind is "mir" (MIR raised; type is its
 * MIR_error_type_t) or "context" (the context cannot be used). */
SEXP zumir_status(const char *kind, int type, const char *message);

#endif
