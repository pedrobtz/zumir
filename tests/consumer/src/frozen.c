/* A translation unit built against the frozen v1 header (D21): it must run
 * against the current zumir. Returns x * 3 - 1 computed by MIR. */
#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#include "frozen/zumir_v1.h"

SEXP zc_frozen_c(SEXP x) {
  const char *why;
  if (zumir_api_load(&why) == NULL) Rf_error("frozen v1 header: %s", why);
  int status;
  zumir_ctx *z = zumir_ctx_new(&status);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t mod = MIR_new_module(ctx, "m");
  MIR_type_t rt = MIR_T_D;
  MIR_var_t a = {MIR_T_D, "x", 0};
  MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 1, &a);
  zumir_emitter e;
  zumir_emit_begin(&e, ctx, fi);
  MIR_reg_t rx = MIR_reg(ctx, "x", fi->u.func);
  MIR_reg_t r = zumir_emit_sub(&e, zumir_emit_mul(&e, rx, zumir_emit_const(&e, 3.0)),
                               zumir_emit_const(&e, 1.0));
  zumir_emit_return_value(&e, r);
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  double (*f)(double) =
    (double (*)(double)) zumir_compile(z, mod, "f", ZUMIR_TIER_AUTO, NULL, &status);
  double out = f != NULL ? f(Rf_asReal(x)) : NA_REAL;
  zumir_ctx_free(z);
  return Rf_ScalarReal(out);
}
