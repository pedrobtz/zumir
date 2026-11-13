/* The one way a zumir file includes mir.h. mir.h has inline functions that
 * write to stderr and call exit (MIR_init2), so the redirection in
 * zumir_mir_stdio.h comes first, as it does for MIR's own sources. A file
 * that includes vendor/mir/mir.h directly links stderr, which R CMD check
 * and tools/ci/platform.sh both report. */
#ifndef ZUMIR_MIR_API_H
#define ZUMIR_MIR_API_H

#include "zumir_mir_stdio.h"
#include "vendor/mir/mir.h"

/* zumir_mir_glue.c. Both raise the trap, so call them under one. */
void zumir_mir_error_func(MIR_error_type_t type, const char *format, ...) ZUMIR_NORETURN;
/* A new context with zumir's error function and trapping allocator. If it
 * raises, there is no context to finish: what MIR had allocated is lost,
 * which happens only when an allocation fails. */
MIR_context_t zumir_mir_new(void);

#endif
