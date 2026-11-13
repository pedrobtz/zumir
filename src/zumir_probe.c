/* Tier detection (design §4.4, Q5 (c)). At load, a small module is
 * compiled and called natively and on the interpreter, and each tier is
 * reported as four capabilities that fail separately:
 *
 *   compile   MIR loads and links the module on this tier;
 *   entry     the linked function has a C-callable address;
 *   run       calling it gives the right answer (x * 2 + 1);
 *   external  a function calling the import exp gives libm's exp, bitwise.
 *
 * A tier is available when all four hold. Where the system refuses memory
 * that is writable and executable, neither tier compiles (both need code
 * memory, §4.4), MIR reports it through the fork's code-memory patch (D11)
 * rather than a SIGSEGV, and zumir has no tier (Q5, decided (c) on
 * 2026-11-13). The probe runs in .onLoad, before anything else maps code;
 * no R API is called under its traps. */
#include <errno.h>
#include <math.h>
#include <string.h>

#include "zumir_ctx.h"

#define NCAP 4
static const char *cap_names[NCAP] = {"compile", "entry", "run", "external"};

typedef struct {
  int done;
  int ok[2][NCAP];
  char why[2][NCAP][512];
} probe_result;

static probe_result probe;

#if ZUMIR_HAVE_MIR
#include "vendor/mir/mir-gen.h"
#include "zumir_trap.h"

static const char *probe_text =
  "probe: module\n"
  "  export arith, ext\n"
  "  p_exp: proto d, d:x\n"
  "  import exp\n"
  "arith: func d, d:x\n"
  "  local d:t\n"
  "  dmul t, x, 2.0\n"
  "  dadd t, t, 1.0\n"
  "  ret t\n"
  "  endfunc\n"
  "ext: func d, d:x\n"
  "  local d:r\n"
  "  call p_exp, exp, r, x\n"
  "  ret r\n"
  "  endfunc\n"
  "endmodule\n";

/* Why a code mapping was refused, from its errno, where that says more
 * than MIR's message. */
static void map_reason(char *buf, size_t n, const char *mir_message, int err) {
#if defined(__APPLE__)
  if (err == EPERM || err == EACCES || err == EINVAL) {
    snprintf(buf, n,
             "%s; macOS refused MAP_JIT memory (%s): R may lack the "
             "com.apple.security.cs.allow-jit entitlement",
             mir_message, strerror(err));
    return;
  }
#endif
  if (err != 0)
    snprintf(buf, n, "%s (%s)", mir_message, strerror(err));
  else
    snprintf(buf, n, "%s", mir_message);
}

static void probe_tier(int tier) {
  int *ok = probe.ok[tier];
  char(*why)[512] = probe.why[tier];
  MIR_context_t volatile ctx = NULL;
  zumir_arena *volatile arena = NULL;
  volatile int gen = 0;
  void *volatile arith = NULL, *volatile ext = NULL;
  zumir_trap t;

  for (int k = 0; k < NCAP; k++) {
    ok[k] = 0;
    why[k][0] = '\0';
  }
  zumir_mir_message_clear();
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    ctx = zumir_mir_new(&arena);
    if (tier == ZUMIR_TIER_NATIVE) {
      MIR_gen_init(ctx);
      gen = 1;
      MIR_gen_set_optimize_level(ctx, 2);
    }
    MIR_scan_string(ctx, probe_text);
    MIR_module_t mod = DLIST_TAIL(MIR_module_t, *MIR_get_module_list(ctx));
    MIR_load_module(ctx, mod);
    MIR_load_external(ctx, "exp", (void *) exp);
    MIR_link(ctx, tier == ZUMIR_TIER_NATIVE ? MIR_set_gen_interface : MIR_set_interp_interface,
             NULL);
    for (MIR_item_t it = DLIST_HEAD(MIR_item_t, mod->items); it != NULL;
         it = DLIST_NEXT(MIR_item_t, it)) {
      if (it->item_type != MIR_func_item) continue;
      if (strcmp(it->u.func->name, "arith") == 0) arith = it->addr;
      if (strcmp(it->u.func->name, "ext") == 0) ext = it->addr;
    }
    zumir_trap_pop(&t);
    ok[0] = 1;
  } else {
    int err = arena != NULL ? zumir_arena_map_errno(arena) : 0;
    map_reason(why[0], sizeof(why[0]), t.message, err);
  }

  if (ok[0]) {
    ok[1] = arith != NULL && ext != NULL;
    if (!ok[1]) snprintf(why[1], sizeof(why[1]), "the linked functions have no address");
  } else {
    snprintf(why[1], sizeof(why[1]), "not compiled");
  }
  if (ok[1]) {
    double (*f)(double) = (double (*)(double)) arith;
    double (*g)(double) = (double (*)(double)) ext;
    double r = f(1.5), e = g(0.75), want = exp(0.75);
    ok[2] = r == 4.0;
    if (!ok[2]) snprintf(why[2], sizeof(why[2]), "arith(1.5) gave %.17g, not 4", r);
    ok[3] = memcmp(&e, &want, sizeof(double)) == 0;
    if (!ok[3]) snprintf(why[3], sizeof(why[3]), "exp(0.75) gave %.17g, not %.17g", e, want);
  } else {
    snprintf(why[2], sizeof(why[2]), "no entry point");
    snprintf(why[3], sizeof(why[3]), "no entry point");
  }
  zumir_mir_free(ctx, arena, gen);
}
#endif

/* Runs the probe now and keeps the result for zumir_probe_result(). */
SEXP zumir_probe_run(void) {
#if ZUMIR_HAVE_MIR
  probe_tier(ZUMIR_TIER_NATIVE);
  probe_tier(ZUMIR_TIER_INTERP);
#else
  for (int tier = 0; tier < 2; tier++)
    for (int k = 0; k < NCAP; k++) {
      probe.ok[tier][k] = 0;
      snprintf(probe.why[tier][k], sizeof(probe.why[tier][k]),
               "zumir was built without MIR: no code generator for this CPU");
    }
#endif
  probe.done = 1;
  return R_NilValue;
}

/* data.frame-ready list(tier, capability, ok, reason), 8 rows, or NULL if
 * the probe has not run. */
SEXP zumir_probe_result(void) {
  if (!probe.done) return R_NilValue;
  const char *names[] = {"tier", "capability", "ok", "reason", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SEXP tier = PROTECT(Rf_allocVector(STRSXP, 2 * NCAP));
  SEXP cap = PROTECT(Rf_allocVector(STRSXP, 2 * NCAP));
  SEXP ok = PROTECT(Rf_allocVector(LGLSXP, 2 * NCAP));
  SEXP why = PROTECT(Rf_allocVector(STRSXP, 2 * NCAP));
  for (int t = 0; t < 2; t++)
    for (int k = 0; k < NCAP; k++) {
      int i = t * NCAP + k;
      SET_STRING_ELT(tier, i, Rf_mkChar(t == 0 ? "native" : "interp"));
      SET_STRING_ELT(cap, i, Rf_mkChar(cap_names[k]));
      LOGICAL(ok)[i] = probe.ok[t][k];
      SET_STRING_ELT(why, i, probe.ok[t][k] ? NA_STRING : Rf_mkCharCE(probe.why[t][k], CE_UTF8));
    }
  SET_VECTOR_ELT(res, 0, tier);
  SET_VECTOR_ELT(res, 1, cap);
  SET_VECTOR_ELT(res, 2, ok);
  SET_VECTOR_ELT(res, 3, why);
  UNPROTECT(5);
  return res;
}
