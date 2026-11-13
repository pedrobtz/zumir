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

#if ZUMIR_HAVE_MIR
#include "zumir_mir_api.h"

enum { ZUMIR_TIER_NATIVE = 0, ZUMIR_TIER_INTERP = 1 };

typedef struct zumir_rctx {
  MIR_context_t ctx;
  zumir_arena *arena;
  int tier;   /* ZUMIR_TIER_NATIVE or ZUMIR_TIER_INTERP */
  int gen;    /* MIR_gen_init was called */
  int failed; /* MIR raised: only freeing does anything now */
  int type;   /* the error's MIR_error_type_t */
  char message[1024];
  int map_errno; /* errno of a refused code mapping, if any */
  void *fn;      /* the linked function, once linked */
} zumir_rctx;

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
