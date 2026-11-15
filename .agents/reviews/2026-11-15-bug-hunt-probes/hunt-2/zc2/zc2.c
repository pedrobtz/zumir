#define R_NO_REMAP
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <zumir.h>

typedef struct { int id, tier; double x; /* out: */ int fn_null, status, zst, tier_out, extra; double value, value2; char msg[400]; } job;

static double twice_plus(double a) { return 2 * a + 100; }

/* begin a module "m" with a function  double f(double a)  and an emitter */
static MIR_item_t begin(MIR_context_t ctx, MIR_module_t *mod, zumir_emitter *e, const char *fname) {
  MIR_type_t rt = MIR_T_D; MIR_var_t a = {MIR_T_D, "a", 0};
  if (mod != NULL) *mod = MIR_new_module(ctx, "m");
  MIR_item_t fi = MIR_new_func_arr(ctx, fname, 1, &rt, 1, &a);
  zumir_emit_begin(e, ctx, fi);
  return fi;
}

static void finish(job *j, zumir_ctx *z, MIR_module_t mod, const char *name, int call) {
  const char *m = "";
  j->zst = zumir_status(z, &m);
  void *fn = zumir_compile(z, mod, name, j->tier, &j->tier_out, &j->status);
  j->fn_null = fn == NULL;
  const char *m2 = "";
  zumir_status(z, &m2);
  snprintf(j->msg, sizeof j->msg, "%s | %s", m, m2);
  if (fn != NULL && call) { double (*f)(double); memcpy(&f, &fn, sizeof f); j->value = f(j->x); }
}

static const char *d6_1[] = {"acos","asin","atan","cos","sin","tan","acosh","asinh","atanh","cosh","sinh","tanh","exp","exp2","expm1","log","log10","log1p","log2","logb","cbrt","fabs","sqrt","erf","erfc","lgamma","tgamma","ceil","floor","nearbyint","rint","round","trunc"};

static void run(job *j) {
  int st = -1;
  j->value = NA_REAL; j->value2 = NA_REAL; j->fn_null = -1; j->status = -1; j->zst = -1; j->tier_out = -1; j->extra = -1; j->msg[0] = 0;
  zumir_ctx *z = zumir_ctx_new(&st);
  if (z == NULL) { snprintf(j->msg, sizeof j->msg, "ctx_new failed %d", st); return; }
  MIR_context_t ctx = zumir_mir(z);
  MIR_module_t mod = NULL; zumir_emitter e; MIR_item_t fi; MIR_reg_t a, r;
  switch (j->id) {
  case 1: /* good: 2a+1 */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    r = zumir_emit_add(&e, zumir_emit_mul(&e, a, zumir_emit_const(&e, 2.0)), zumir_emit_const(&e, 1.0));
    zumir_emit_return_value(&e, r); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break;
  case 2: { /* bad opcode through insn_arr */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_op_t ops[3] = {MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a)};
    MIR_append_insn(ctx, fi, MIR_new_insn_arr(ctx, (MIR_insn_code_t) 9999, 3, ops));
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break; }
  case 3: { /* wrong operand count */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_op_t ops[3] = {MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a)};
    MIR_append_insn(ctx, fi, MIR_new_insn_arr(ctx, MIR_DADD, 2, ops));
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break; }
  case 4: /* bad types: integer add on double regs */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a)));
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break;
  case 5: { /* jump to a label that is never placed */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_label_t l = zumir_emit_label(&e);
    zumir_emit_jump(&e, l);
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break; }
  case 6: /* register that does not exist */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, 9999)));
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break;
  case 7: /* register 0 */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, 0)));
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break;
  case 8: { /* a register of another function */
    fi = begin(ctx, &mod, &e, "g"); MIR_reg_t x1 = zumir_emit_const(&e, 5.0); MIR_reg_t x2 = zumir_emit_const(&e, 6.0); MIR_reg_t x3 = zumir_emit_add(&e, x1, x2);
    zumir_emit_return_value(&e, x3); MIR_finish_func(ctx);
    fi = begin(ctx, NULL, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    zumir_emit_return_value(&e, x3); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break; }
  case 9: { /* a second function begun while the first is open */
    fi = begin(ctx, &mod, &e, "g"); zumir_emitter e2; MIR_item_t f2 = begin(ctx, NULL, &e2, "f"); a = MIR_reg(ctx, "a", f2->u.func);
    zumir_emit_return_value(&e2, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break; }
  case 10: /* finish_module with the function open */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func); zumir_emit_return_value(&e, a);
    MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break;
  case 11: /* compile a module never finished */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func); zumir_emit_return_value(&e, a); MIR_finish_func(ctx);
    finish(j, z, mod, "f", 1); break;
  case 12: { /* compile twice; then a name that is not there; then again */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    zumir_emit_return_value(&e, zumir_emit_add(&e, a, a)); MIR_finish_func(ctx); MIR_finish_module(ctx);
    int s1, s2, s3, t; void *f1 = zumir_compile(z, mod, "f", j->tier, &t, &s1); void *f0 = zumir_compile(z, mod, "nope", j->tier, &t, &s2);
    void *f2 = zumir_compile(z, mod, "f", j->tier, &t, &s3);
    j->extra = s1 * 100 + s2 * 10 + s3; j->fn_null = (f1 == NULL) * 100 + (f0 == NULL) * 10 + (f2 == NULL); j->status = s3;
    if (f1 != NULL) { double (*f)(double); memcpy(&f, &f1, sizeof f); j->value = f(j->x); }
    if (f2 != NULL) { double (*f)(double); memcpy(&f, &f2, sizeof f); j->value2 = f(j->x); }
    const char *m; j->zst = zumir_status(z, &m); snprintf(j->msg, sizeof j->msg, "%s same=%d", m, f1 == f2); break; }
  case 13: { /* NULL and invalid arguments */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func); zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx);
    int s[6], t; void *p[6];
    p[0] = zumir_compile(NULL, mod, "f", 0, &t, &s[0]); p[1] = zumir_compile(z, NULL, "f", 0, &t, &s[1]); p[2] = zumir_compile(z, mod, NULL, 0, &t, &s[2]);
    p[3] = zumir_compile(z, mod, "f", 7, &t, &s[3]); p[4] = zumir_compile(z, mod, "f", -1, &t, &s[4]); p[5] = zumir_compile(z, mod, "", 0, NULL, &s[5]);
    j->extra = 0; for (int i = 0; i < 6; i++) j->extra = j->extra * 10 + s[i];
    j->fn_null = 0; for (int i = 0; i < 6; i++) j->fn_null = j->fn_null * 10 + (p[i] == NULL);
    const char *m; j->zst = zumir_status(z, &m); snprintf(j->msg, sizeof j->msg, "%s", m);
    j->status = zumir_import(z, NULL, NULL) * 100 + zumir_import(z, "q", NULL) * 10 + zumir_import(NULL, "q", (void *) twice_plus);
    finish(j, z, mod, "f", 1); break; }
  case 14: case 15: case 16: { /* emitter: 32 / 33 / 31 distinct calls (acosh left out; the 33rd is hypot) */
    int n = j->id == 14 ? 32 : j->id == 15 ? 33 : 31;
    static const char *nm[] = {"acos","asin","atan","cos","sin","tan","asinh","atanh","cosh","sinh","tanh","exp","exp2","expm1","log","log10","log1p","log2","logb","cbrt","fabs","sqrt","erf","erfc","lgamma","tgamma","ceil","floor","nearbyint","rint","round","trunc"};
    double (*fs[])(double) = {acos,asin,atan,cos,sin,tan,asinh,atanh,cosh,sinh,tanh,exp,exp2,expm1,log,log10,log1p,log2,logb,cbrt,fabs,sqrt,erf,erfc,lgamma,tgamma,ceil,floor,nearbyint,rint,round,trunc};
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_reg_t acc = zumir_emit_const(&e, 0.0);
    for (int i = 0; i < n && i < 32; i++) acc = zumir_emit_add(&e, acc, zumir_emit_call1(&e, nm[i], a));
    if (n == 33) acc = zumir_emit_add(&e, acc, zumir_emit_call2(&e, "hypot", a, a));
    zumir_emit_return_value(&e, acc); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1);
    { double s = 0; for (int i = 0; i < n && i < 32; i++) s += fs[i](j->x); if (n == 33) s += hypot(j->x, j->x); j->value2 = s; j->extra = e.ncalls; }
    break; }
  case 17: { /* emitter call with 5 args */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_reg_t args[5] = {a, a, a, a, a};
    r = zumir_emit_call(&e, "fma", 5, args);
    zumir_emit_return_value(&e, r); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break; }
  case 18: { /* emitter call names from a reused buffer: sin then cos */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    char buf[16]; strcpy(buf, "sin"); MIR_reg_t s = zumir_emit_call1(&e, buf, a);
    strcpy(buf, "cos"); MIR_reg_t c = zumir_emit_call1(&e, buf, a);
    r = zumir_emit_add(&e, zumir_emit_mul(&e, s, zumir_emit_const(&e, 1000.0)), c);
    zumir_emit_return_value(&e, r); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1);
    j->value2 = sin(j->x) * 1000.0 + cos(j->x); break; }
  case 19: { /* two functions in one module, each with its own emitter, both calling sin */
    fi = begin(ctx, &mod, &e, "g"); a = MIR_reg(ctx, "a", fi->u.func);
    zumir_emit_return_value(&e, zumir_emit_call1(&e, "sin", a)); MIR_finish_func(ctx);
    fi = begin(ctx, NULL, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    zumir_emit_return_value(&e, zumir_emit_call1(&e, "sin", a)); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1);
    j->value2 = sin(j->x); break; }
  case 20: case 21: { /* MIR_new_call_insn with 64 / 65 operands (62 / 63 args) */
    int nargs = j->id == 20 ? 61 : 62; /* ops = proto, func, res, args */
    MIR_type_t rt = MIR_T_D; MIR_var_t vars[70]; static char names[70][8];
    for (int i = 0; i < nargs; i++) { snprintf(names[i], 8, "p%d", i); vars[i].type = MIR_T_D; vars[i].name = names[i]; vars[i].size = 0; }
    mod = MIR_new_module(ctx, "m");
    MIR_item_t g = MIR_new_func_arr(ctx, "g", 1, &rt, nargs, vars);
    { MIR_reg_t acc = MIR_new_func_reg(ctx, g->u.func, MIR_T_D, "acc");
      MIR_append_insn(ctx, g, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, acc), MIR_new_double_op(ctx, 0.0)));
      for (int i = 0; i < nargs; i++) MIR_append_insn(ctx, g, MIR_new_insn(ctx, MIR_DADD, MIR_new_reg_op(ctx, acc), MIR_new_reg_op(ctx, acc), MIR_new_reg_op(ctx, MIR_reg(ctx, names[i], g->u.func))));
      MIR_append_insn(ctx, g, MIR_new_ret_insn(ctx, 1, MIR_new_reg_op(ctx, acc))); MIR_finish_func(ctx); }
    MIR_item_t proto = MIR_new_proto_arr(ctx, "gp", 1, &rt, nargs, vars);
    fi = begin(ctx, NULL, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func); r = zumir_emit_reg(&e, MIR_T_D);
    MIR_op_t o = MIR_new_reg_op(ctx, a);
#define O8 o, o, o, o, o, o, o, o
    MIR_insn_t ci = nargs == 61
      ? MIR_new_call_insn(ctx, 64, MIR_new_ref_op(ctx, proto), MIR_new_ref_op(ctx, g), MIR_new_reg_op(ctx, r), O8, O8, O8, O8, O8, O8, O8, o, o, o, o, o)
      : MIR_new_call_insn(ctx, 65, MIR_new_ref_op(ctx, proto), MIR_new_ref_op(ctx, g), MIR_new_reg_op(ctx, r), O8, O8, O8, O8, O8, O8, O8, o, o, o, o, o, o);
    MIR_append_insn(ctx, fi, ci);
    zumir_emit_return_value(&e, r); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); j->value2 = nargs * j->x; break; }
  case 22: { /* label placed twice */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_label_t l = zumir_emit_label(&e); zumir_emit_place(&e, l); zumir_emit_place(&e, l);
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break; }
  case 23: { /* same insn appended twice */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_insn_t in = MIR_new_insn(ctx, MIR_DADD, MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a));
    MIR_append_insn(ctx, fi, in); MIR_append_insn(ctx, fi, in);
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); j->value2 = 4 * j->x; break; }
  case 24: /* MIR_new_insn with an opcode far out of range */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_append_insn(ctx, fi, MIR_new_insn(ctx, (MIR_insn_code_t) 1000000, MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a)));
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break;
  case 25: /* MIR_new_insn with a negative opcode */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_append_insn(ctx, fi, MIR_new_insn(ctx, (MIR_insn_code_t) -5, MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a)));
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break;
  case 26: { /* MIR_insn_name / MIR_item_name on bad input must not fail a good context? */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    const char *nm = MIR_insn_name(ctx, (MIR_insn_code_t) 99999); j->extra = nm == NULL ? 0 : (int) strlen(nm);
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); break; }
  case 27: { /* consumer import overriding a D6 name, and one registered after emit */
    j->extra = zumir_import(z, "sin", (void *) twice_plus) * 10;
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    r = zumir_emit_add(&e, zumir_emit_call1(&e, "sin", a), zumir_emit_call1(&e, "mine", a));
    zumir_emit_return_value(&e, r); MIR_finish_func(ctx); MIR_finish_module(ctx);
    j->extra += zumir_import(z, "mine", (void *) twice_plus);
    finish(j, z, mod, "f", 1); j->value2 = sin(j->x) + twice_plus(j->x); break; }
  case 28: { /* unknown import */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    zumir_emit_return_value(&e, zumir_emit_call1(&e, "Rf_error", a)); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 0); break; }
  case 29: { /* one tier per context: native then interp */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func); zumir_emit_return_value(&e, zumir_emit_neg(&e, a)); MIR_finish_func(ctx); MIR_finish_module(ctx);
    int s1, s2, t1 = -1, t2 = -1; void *f1 = zumir_compile(z, mod, "f", ZUMIR_TIER_NATIVE, &t1, &s1); void *f2 = zumir_compile(z, mod, "f", ZUMIR_TIER_INTERP, &t2, &s2);
    j->extra = s1 * 10 + s2; j->fn_null = (f1 == NULL) * 10 + (f2 == NULL); j->tier_out = t1 * 10 + t2;
    if (f1 != NULL) { double (*f)(double); memcpy(&f, &f1, sizeof f); j->value = f(j->x); }
    if (f2 != NULL) { double (*f)(double); memcpy(&f, &f2, sizeof f); j->value2 = f(j->x); }
    break; }
  case 30: { /* a second module built and compiled after the first was compiled */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func); zumir_emit_return_value(&e, zumir_emit_call1(&e, "sqrt", a)); MIR_finish_func(ctx); MIR_finish_module(ctx);
    finish(j, z, mod, "f", 1);
    MIR_module_t m2 = MIR_new_module(ctx, "m2"); MIR_type_t rt = MIR_T_D; MIR_var_t av = {MIR_T_D, "a", 0};
    MIR_item_t g = MIR_new_func_arr(ctx, "g", 1, &rt, 1, &av); zumir_emit_begin(&e, ctx, g); a = MIR_reg(ctx, "a", g->u.func);
    zumir_emit_return_value(&e, zumir_emit_call1(&e, "exp", a)); MIR_finish_func(ctx); MIR_finish_module(ctx);
    int s2, t; void *f2 = zumir_compile(z, m2, "g", j->tier, &t, &s2); j->extra = s2;
    if (f2 != NULL) { double (*f)(double); memcpy(&f, &f2, sizeof f); j->value2 = f(j->x); }
    break; }
  case 31: { /* failed build, then everything again on the same (failed) context */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a)));
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx);
    MIR_module_t m2 = MIR_new_module(ctx, "m2"); zumir_emitter e2; MIR_type_t rt = MIR_T_D; MIR_var_t av = {MIR_T_D, "a", 0};
    MIR_item_t g = MIR_new_func_arr(ctx, "g", 1, &rt, 1, &av); zumir_emit_begin(&e2, ctx, g); MIR_reg_t ga = MIR_reg(ctx, "a", g->u.func);
    zumir_emit_return_value(&e2, zumir_emit_call1(&e2, "exp", ga)); MIR_finish_func(ctx); MIR_finish_module(ctx);
    j->extra = (m2 != NULL) + 10 * (g != NULL && g->u.func != NULL) + 100 * (ga == 0) + 1000 * (MIR_get_module_item(ctx, m2, "g") == NULL);
    finish(j, z, m2, "g", 1); break; }
  case 32: { /* free twice */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func); zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1);
    zumir_ctx_free(z); zumir_ctx_free(z); j->extra = 1; return; }
  case 33: { /* NaN / -0 semantics, to compare across tiers: packs comparison bits */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    MIR_reg_t zero = zumir_emit_const(&e, 0.0), acc = zumir_emit_const(&e, 0.0), one = zumir_emit_const(&e, 1.0);
    MIR_insn_code_t cmp[] = {MIR_DEQ, MIR_DNE, MIR_DLT, MIR_DLE, MIR_DGT, MIR_DGE}; MIR_insn_code_t br[] = {MIR_DBEQ, MIR_DBNE, MIR_DBLT, MIR_DBLE, MIR_DBGT, MIR_DBGE};
    for (int i = 0; i < 6; i++) { /* acc = acc*2 + (a cmp 0) ; acc = acc*2 + branch taken */
      MIR_reg_t c = zumir_emit_cmp(&e, cmp[i], a, zero); MIR_label_t l = zumir_emit_label(&e), l2 = zumir_emit_label(&e);
      acc = zumir_emit_add(&e, acc, acc); zumir_emit_branch_true(&e, c, l); zumir_emit_jump(&e, l2); zumir_emit_place(&e, l); zumir_emit_move(&e, acc, zumir_emit_add(&e, acc, one)); zumir_emit_place(&e, l2);
      MIR_label_t l3 = zumir_emit_label(&e), l4 = zumir_emit_label(&e);
      acc = zumir_emit_add(&e, acc, acc); zumir_emit_branch(&e, br[i], a, zero, l3); zumir_emit_jump(&e, l4); zumir_emit_place(&e, l3); zumir_emit_move(&e, acc, zumir_emit_add(&e, acc, one)); zumir_emit_place(&e, l4);
    }
    /* + sign of -a through 1/(-a) when a is zero is left to the caller: return acc + neg(a)*0 */
    zumir_emit_return_value(&e, acc); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1);
    { double x = j->x, s = 0; int b[] = {x == 0, x != 0, x < 0, x <= 0, x > 0, x >= 0}; for (int i = 0; i < 6; i++) s = s * 4 + b[i] * 3; j->value2 = s; }
    break; }
  case 34: { /* neg, sub, div at the edges: returns -(a) / (a - a) etc. as given */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    r = zumir_emit_div(&e, zumir_emit_const(&e, 1.0), zumir_emit_neg(&e, a));
    zumir_emit_return_value(&e, r); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); j->value2 = 1.0 / -j->x; break; }
  case 35: { /* load/store through a pointer argument with negative and large index */
    MIR_type_t rt = MIR_T_D; MIR_var_t av[2] = {{MIR_T_P, "p", 0}, {MIR_T_D, "a", 0}};
    mod = MIR_new_module(ctx, "m"); fi = MIR_new_func_arr(ctx, "f", 1, &rt, 2, av); zumir_emit_begin(&e, ctx, fi);
    MIR_reg_t p = MIR_reg(ctx, "p", fi->u.func); a = MIR_reg(ctx, "a", fi->u.func);
    zumir_emit_store(&e, p, -1, a); zumir_emit_store(&e, p, 3, zumir_emit_load(&e, p, -1));
    zumir_emit_return_value(&e, zumir_emit_add(&e, zumir_emit_load(&e, p, 3), zumir_emit_load(&e, p, 0))); MIR_finish_func(ctx); MIR_finish_module(ctx);
    void *fn = zumir_compile(z, mod, "f", j->tier, &j->tier_out, &j->status); j->fn_null = fn == NULL;
    if (fn != NULL) { double buf[6] = {7, 7, 10, 7, 7, 7}; double (*f)(double *, double); memcpy(&f, &fn, sizeof f); j->value = f(buf + 2, j->x); j->value2 = buf[1] * 1000 + buf[5]; }
    break; }
  case 36: { /* MIR_new_func (variadic form) with 64 / 65 args: extra carries nargs via x */
    int nargs = (int) j->x; MIR_type_t rt = MIR_T_D; mod = MIR_new_module(ctx, "m");
#define A8(k) MIR_T_D, "a" #k "0", MIR_T_D, "a" #k "1", MIR_T_D, "a" #k "2", MIR_T_D, "a" #k "3", MIR_T_D, "a" #k "4", MIR_T_D, "a" #k "5", MIR_T_D, "a" #k "6", MIR_T_D, "a" #k "7"
    fi = nargs == 64 ? MIR_new_func(ctx, "f", 1, &rt, 64, A8(0), A8(1), A8(2), A8(3), A8(4), A8(5), A8(6), A8(7))
                     : MIR_new_func(ctx, "f", 1, &rt, 65, A8(0), A8(1), A8(2), A8(3), A8(4), A8(5), A8(6), A8(7), MIR_T_D, "last");
    zumir_emit_begin(&e, ctx, fi); zumir_emit_return_value(&e, MIR_reg(ctx, "a77", fi->u.func)); MIR_finish_func(ctx); MIR_finish_module(ctx);
    finish(j, z, mod, "f", 0); break; }
  case 37: { /* i32 register type, bad mem scale, unknown reg name */
    fi = begin(ctx, &mod, &e, "f"); a = MIR_reg(ctx, "a", fi->u.func);
    int which = (int) j->x;
    if (which == 0) MIR_new_func_reg(ctx, fi->u.func, MIR_T_I32, "small");
    if (which == 1) MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, a), MIR_new_mem_op(ctx, MIR_T_D, 0, a, a, 3)));
    if (which == 2) MIR_reg(ctx, "nosuch", fi->u.func);
    if (which == 3) MIR_new_func_reg(ctx, fi->u.func, MIR_T_D, "a");
    if (which == 4) MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, a), MIR_new_mem_op(ctx, (MIR_type_t) 77, 0, a, 0, 1)));
    if (which == 5) MIR_new_func_reg(ctx, fi->u.func, (MIR_type_t) 77, "odd");
    if (which == 6) MIR_new_func_reg(ctx, fi->u.func, MIR_T_D, NULL);
    if (which == 7) MIR_new_export(ctx, "nothere");
    if (which == 8) { MIR_new_forward(ctx, "fwd"); }
    if (which == 9) MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, a), MIR_new_int_op(ctx, 5)));
    if (which == 10) MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_JMP, MIR_new_reg_op(ctx, a)));
    if (which == 11) MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_DMOV, MIR_new_double_op(ctx, 1.0), MIR_new_reg_op(ctx, a)));
    if (which == 12) MIR_append_insn(ctx, fi, NULL);
    if (which == 13) MIR_append_insn(ctx, NULL, MIR_new_label(ctx));
    if (which == 14) MIR_remove_insn(ctx, fi, MIR_new_label(ctx));
    if (which == 15) MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, a), MIR_new_ref_op(ctx, NULL)));
    if (which == 16) { MIR_insn_t in = MIR_new_insn(ctx, MIR_DADD, MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a), MIR_new_reg_op(ctx, a)); MIR_insert_insn_after(ctx, fi, in, MIR_new_label(ctx)); }
    if (which == 17) MIR_append_insn(ctx, fi, MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, a), MIR_new_label_op(ctx, NULL)));
    zumir_emit_return_value(&e, a); MIR_finish_func(ctx); MIR_finish_module(ctx); finish(j, z, mod, "f", 1); j->value2 = j->x; break; }
  default: snprintf(j->msg, sizeof j->msg, "no such probe");
  }
  zumir_ctx_free(z);
}

static void *thread_main(void *arg) { run((job *) arg); return NULL; }

SEXP zc2_probe(SEXP id, SEXP tier, SEXP thread, SEXP x) {
  const char *why;
  if (zumir_api_ == NULL && zumir_api_load(&why) == NULL) Rf_error("zumir: %s", why);
  job j; memset(&j, 0, sizeof j); j.id = Rf_asInteger(id); j.tier = Rf_asInteger(tier); j.x = Rf_asReal(x);
  if (Rf_asLogical(thread)) { pthread_t t; pthread_attr_t at; pthread_attr_init(&at); pthread_attr_setstacksize(&at, 8u << 20); pthread_create(&t, &at, thread_main, &j); pthread_join(t, NULL); } else run(&j);
  const char *names[] = {"fn_null", "status", "zst", "tier", "extra", "value", "value2", "msg", ""};
  SEXP res = PROTECT(Rf_mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_ScalarInteger(j.fn_null)); SET_VECTOR_ELT(res, 1, Rf_ScalarInteger(j.status)); SET_VECTOR_ELT(res, 2, Rf_ScalarInteger(j.zst));
  SET_VECTOR_ELT(res, 3, Rf_ScalarInteger(j.tier_out)); SET_VECTOR_ELT(res, 4, Rf_ScalarInteger(j.extra)); SET_VECTOR_ELT(res, 5, Rf_ScalarReal(j.value));
  SET_VECTOR_ELT(res, 6, Rf_ScalarReal(j.value2)); SET_VECTOR_ELT(res, 7, Rf_mkString(j.msg));
  UNPROTECT(1); return res;
}
static const R_CallMethodDef calls[] = {{"zc2_probe", (DL_FUNC) &zc2_probe, 4}, {NULL, NULL, 0}};
void R_init_zc2(DllInfo *dll) { R_registerRoutines(dll, NULL, calls, NULL, NULL); R_useDynamicSymbols(dll, FALSE); }
