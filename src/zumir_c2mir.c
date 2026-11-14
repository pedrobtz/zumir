/* c2mir, MIR's C compiler (design §4.5), compiled only where MIR has a
 * target (zumir_mir.h). MIR's exit, abort and stderr are redirected as for
 * MIR's own sources (zumir_mir_stdio.h), and c2mir reads no header from a
 * file (D5): the fork's C2MIR_NO_FILE_INCLUDES, with zumir's prelude
 * headers given in memory (zumir_prelude.c). */
#include "zumir_mir.h"

#if ZUMIR_HAVE_MIR
#include <math.h>
#include <time.h>
#include <wchar.h>
#include "zumir_prelude.h"
/* tests/c/c-tests-runner.c alone builds this file with
 * ZUMIR_C2MIR_TEST_HEADERS, adding a stdio.h, a stdlib.h and an assert.h
 * that declare the corpus harness's printf, abort and exit (#20). The package never
 * defines it, so no cfunction() source can see them. */
#ifdef ZUMIR_C2MIR_TEST_HEADERS
extern const char zumir_test_stdio_h[], zumir_test_stdlib_h[], zumir_test_assert_h[];
#define ZUMIR_C2MIR_TEST_INCLUDES \
  , {"stdio.h", zumir_test_stdio_h}, {"stdlib.h", zumir_test_stdlib_h}, \
    {"assert.h", zumir_test_assert_h}
#else
#define ZUMIR_C2MIR_TEST_INCLUDES
#endif
#define C2MIR_NO_FILE_INCLUDES 1
#define C2MIR_EXTRA_INCLUDES \
  {"math.h", zumir_prelude_math_h}, {"string.h", zumir_prelude_string_h} \
  ZUMIR_C2MIR_TEST_INCLUDES
#include "zumir_mir_gcc.h"
#include "zumir_mir_stdio.h"
/* zumir_mir_stdio.h includes <windows.h> on Windows, whose IGNORE, CONST
 * and VOID macros would rename c2mir's node codes (N_IGNORE, N_CONST,
 * N_VOID, made by pasting through its REP macros). c2mir uses none of
 * them, and every system header is already included. */
#undef IGNORE
#undef CONST
#undef VOID
#include "vendor/mir/c2mir/c2mir.c"
#else
/* ISO C forbids an empty translation unit. */
typedef int zumir_c2mir_c_unused;
#endif
