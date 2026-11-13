/* The one way a zumir file includes mir.h. mir.h has inline functions that
 * write to stderr and call exit (MIR_init2), so the redirection in
 * zumir_mir_stdio.h comes first, as it does for MIR's own sources. A file
 * that includes vendor/mir/mir.h directly links stderr, which R CMD check
 * and tools/ci/platform.sh both report. */
#ifndef ZUMIR_MIR_API_H
#define ZUMIR_MIR_API_H

#include "zumir_mir_stdio.h"
#include "vendor/mir/mir.h"

/* zumir_mir_glue.c. The error function raises the trap; call the rest
 * under one too, except zumir_mir_free, which sets its own. */
void zumir_mir_error_func(MIR_error_type_t type, const char *format, ...) ZUMIR_NORETURN;

/* A context's heap blocks and code mappings, and its byte count. */
typedef struct zumir_arena zumir_arena;

/* A new context with zumir's error function and its own arena. *arena is
 * set before MIR runs, so after a raise the caller still has it to free,
 * with the context or with NULL if MIR_init2 itself raised. */
MIR_context_t zumir_mir_new(zumir_arena *volatile *arena);

/* Frees a context and its arena: MIR_finish under a trap, unless an
 * allocation failed, then whatever is still allocated or mapped. ctx may be
 * NULL; gen is nonzero when MIR_gen_init was called on it. Returns 1 when
 * MIR_finish ran and returned, else 0. */
int zumir_mir_free(MIR_context_t ctx, zumir_arena *arena, int gen);

size_t zumir_arena_bytes(const zumir_arena *arena);
/* errno of the last code mapping or protection change refused, or 0. */
int zumir_arena_map_errno(const zumir_arena *arena);
/* The owner recorded on a context's arena (a zumir_rctx), from the MIR
 * context alone: how a MIR_ shim finds the zumir context it acts on. */
void zumir_arena_set_owner(zumir_arena *arena, void *owner);
/* From now on zumir_mir_free() releases the arena directly, without
 * MIR_finish: for a context MIR raised on, whose state is not trusted. */
void zumir_arena_mark_failed(zumir_arena *arena);
void *zumir_mir_owner(MIR_context_t ctx);
/* Bytes held by all live arenas. */
double zumir_mir_live_bytes(void);
/* Arenas not yet freed, across the process: 0 when nothing leaked. */
long zumir_mir_live_arenas(void);

/* Test hook: the n-th heap allocation from now fails; 0 turns it off. */
void zumir_mir_fail_alloc_at(long n);

#endif
