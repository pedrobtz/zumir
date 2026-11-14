/* The import resolver; see zumir_imports.c. */
#ifndef ZUMIR_IMPORTS_H
#define ZUMIR_IMPORTS_H

#include <stddef.h>

#include "zumir_mir.h"

enum { ZUMIR_IMPORTS_D6 = 0, ZUMIR_IMPORTS_CONSUMER = 1, ZUMIR_IMPORTS_RFUNCTION = 2 };

typedef struct {
  const char *name;
  void *addr;
} zumir_import_entry;

const zumir_import_entry *zumir_import_list(int kind, size_t *n);
/* D6's list alone (zumir_imports_d6.c), without R. */
const zumir_import_entry *zumir_import_d6(size_t *n);

double zumir_r_fmod(double x1, double x2);
double zumir_r_intdiv(double x1, double x2);
double zumir_r_min2(double x1, double x2);
double zumir_r_max2(double x1, double x2);
int zumir_r_poll(void);
extern _Thread_local int zumir_r_fmod_warned;
extern _Thread_local int zumir_r_nan_warned;
extern _Thread_local char zumir_r_poll_message[512];

#if ZUMIR_HAVE_MIR
#include "zumir_mir_api.h"
/* Loads the names of import set `kind` into ctx, before MIR_link; extra
 * are the consumer's own, used with ZUMIR_IMPORTS_CONSUMER. Call under a
 * trap. */
void zumir_imports_load(MIR_context_t ctx, int kind, const zumir_import_entry *extra,
                        size_t nextra);
#endif

#endif
