// Errors through the table from C++ (D20): an object whose destructor
// counts lives across MIR calls that fail. No call longjmps through this
// frame, so the destructor runs exactly once per iteration.
#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#include <zumir.h>

namespace {
int destroyed = 0;

struct Counter {
  ~Counter() { destroyed++; }
};

void one_failure() {
  Counter guard;
  int status;
  zumir_ctx *z = zumir_ctx_new(&status);
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t mod = MIR_new_module(ctx, "bad");
  // A syntax-free error: an instruction with operands of the wrong mode.
  MIR_type_t rt = MIR_T_D;
  MIR_var_t a = {MIR_T_D, "a", 0};
  MIR_item_t fi = MIR_new_func_arr(ctx, "f", 1, &rt, 1, &a);
  MIR_reg_t ra = MIR_reg(ctx, "a", fi->u.func);
  MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, ra),
                                        MIR_new_reg_op(ctx, ra), MIR_new_reg_op(ctx, ra)));
  MIR_finish_func(ctx);
  MIR_finish_module(ctx);
  zumir_compile(z, mod, "f", ZUMIR_TIER_AUTO, nullptr, &status);
  zumir_ctx_free(z);
}
}  // namespace

extern "C" SEXP zc_errors_cpp_c(SEXP s_n) {
  const char *why;
  if (zumir_api_ == nullptr && zumir_api_load(&why) == nullptr) Rf_error("zumir: %s", why);
  int n = Rf_asInteger(s_n);
  destroyed = 0;
  for (int i = 0; i < n; i++) one_failure();
  return Rf_ScalarInteger(destroyed);
}
