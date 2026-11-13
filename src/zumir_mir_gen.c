/* MIR's code generator (mir-gen.c includes the target's mir-gen-<target>.c),
 * compiled only where MIR has a target (zumir_mir.h). */
#include "zumir_mir.h"

#if ZUMIR_HAVE_MIR
#include "zumir_mir_gcc.h"
#include "zumir_mir_stdio.h"
#include "vendor/mir/mir-gen.c"
#else
/* ISO C forbids an empty translation unit. */
typedef int zumir_mir_gen_c_unused;
#endif
