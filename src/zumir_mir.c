/* MIR's core and interpreter (mir.c includes mir-interp.c and the target's
 * mir-<target>.c), compiled only where MIR has a target (zumir_mir.h).
 *
 * MIR_DIRECT_DISPATCH makes the interpreter dispatch with a switch instead
 * of computed goto, which -pedantic reports as "ISO C forbids 'goto *expr;'"
 * (design §4.2). Its cost is measured in Stage 10 (§9). */
#include "zumir_mir.h"

#if ZUMIR_HAVE_MIR
#define MIR_DIRECT_DISPATCH 1
#include "vendor/mir/mir.c"
#else
/* ISO C forbids an empty translation unit. */
typedef int zumir_mir_c_unused;
#endif
