/* cfunction()'s prelude (design §4.5, D5, D6): the headers c2mir finds in
 * memory, since it reads none from a file. math.h and string.h declare
 * exactly the names the import resolver links (src/zumir_imports.c, D6);
 * c2mir's own built-in headers give stddef.h, stdint.h, stdbool.h,
 * stdarg.h, float.h and limits.h. test-prelude.R holds the two lists to
 * each other. */
#ifndef ZUMIR_PRELUDE_H
#define ZUMIR_PRELUDE_H

extern const char zumir_prelude_math_h[];
extern const char zumir_prelude_string_h[];

/* Prefixed to every source: the prelude's headers, then a #line directive
 * naming the user's source, so c2mir's diagnostics give the user's own
 * line numbers. The caller writes the prefix, the source's name, then
 * zumir_prelude_line0, then the source, which starts at line 1. */
extern const char zumir_prelude_prefix[];
extern const char zumir_prelude_line0[];

#endif
