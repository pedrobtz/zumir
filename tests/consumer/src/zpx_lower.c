/* Copied from pedrobtz/zupmxode at 5f637a1, src/zpx_lower.c, unchanged but for
 * its include paths: the IR zupmxode lowers, lowered as zupmxode lowers
 * it, for test-zupmxode.R. Copy it again with tools/zpx-fixture.R's
 * fixture when zupmxode's IR or lowering changes. */
/* Lowering (design §4.7): a verified IR (src/core/ir.h) becomes one MIR
 * function per block, through zumir's C API and its double emitter, and
 * zumir generates code for it on one tier. MIR is never vendored here
 * (D14): everything goes through the table zumir_api_load() fetches. */
#include <string.h>

#include "zpx_lower.h"

#include "zumir.h"

static const char *block_name[ZPX_IR_NBLOCKS] = {"zpx_pre", "zpx_ivl", "zpx_rhs", "zpx_post",
                                                 "zpx_err"};

/* The math functions zumir cannot find in the C math library by name. */
static int register_imports(zumir_ctx *z) {
  static const int fns[] = {ZPX_FN_SQRT,  ZPX_FN_ABS,    ZPX_FN_GAMMA,     ZPX_FN_LGAMMA,
                            ZPX_FN_LOGIT, ZPX_FN_EXPIT,  ZPX_FN_PROBIT,    ZPX_FN_PROBITINV};
  for (size_t i = 0; i < sizeof(fns) / sizeof(fns[0]); i++) {
    int st = zumir_import(z, zpx_math_name(fns[i]), (void *) zpx_math1(fns[i]));
    if (st != ZUMIR_OK) return st;
  }
  for (int f = ZPX_FN_POW; f < ZPX_FN_NFN2_END; f++) {
    int st = zumir_import(z, zpx_math_name(f), (void *) zpx_math2(f));
    if (st != ZUMIR_OK) return st;
  }
  return ZUMIR_OK;
}

/* An int64 0/1 register as a double 0/1 register. */
static void to_double(zumir_emitter *e, MIR_reg_t d, MIR_reg_t c) {
  zumir_emit_insn(e, MIR_new_insn(e->ctx, MIR_I2D, zumir_emit_r(e, d), zumir_emit_r(e, c)));
}

/* Emits block b of ir as the body of func. labels and regs are scratch of
 * ninsn + 1 and nreg entries. */
static void emit_block(MIR_context_t ctx, MIR_item_t func, const zpx_ir *ir, int b,
                       MIR_label_t *labels, MIR_reg_t *regs) {
  zumir_emitter em, *e = &em;
  MIR_func_t f = func->u.func;
  MIR_reg_t t = MIR_reg(ctx, "t", f), y = MIR_reg(ctx, "y", f), o = MIR_reg(ctx, "o", f),
            p = MIR_reg(ctx, "p", f), w = MIR_reg(ctx, "w", f);
  const int32_t *code = ir->block[b].code;
  int n = ir->block[b].ninsn;
  zumir_emit_begin(e, ctx, func);
  for (int i = 0; i <= n; i++) labels[i] = NULL;
  for (int i = 0; i < n; i++) {
    const int32_t *op = code + (size_t) i * ZPX_IR_WIDTH;
    int j = op[0] == ZPX_OP_JMP ? op[1] : op[0] == ZPX_OP_JMPZ ? op[2] : -1;
    if (j >= 0 && labels[j] == NULL) labels[j] = zumir_emit_label(e);
  }
  for (int r = 0; r < ir->block[b].nreg; r++) {
    regs[r] = zumir_emit_reg(e, MIR_T_D);
    zumir_emit_insn(e, MIR_new_insn(ctx, MIR_DMOV, zumir_emit_r(e, regs[r]),
                                    MIR_new_double_op(ctx, 0.0)));
  }
  MIR_reg_t zero = zumir_emit_const(e, 0.0);
  for (int i = 0; i < n; i++) {
    const int32_t *op = code + (size_t) i * ZPX_IR_WIDTH;
    int32_t d = op[1], a = op[2], c = op[3], x = op[4];
    if (labels[i] != NULL) zumir_emit_place(e, labels[i]);
    switch (op[0]) {
    case ZPX_OP_CONST:
      zumir_emit_insn(e, MIR_new_insn(ctx, MIR_DMOV, zumir_emit_r(e, regs[d]),
                                      MIR_new_double_op(ctx, ir->consts[a])));
      break;
    case ZPX_OP_LOADT: zumir_emit_move(e, regs[d], t); break;
    case ZPX_OP_LOADY:
      zumir_emit_insn(e, MIR_new_insn(ctx, MIR_DMOV, zumir_emit_r(e, regs[d]),
                                      zumir_emit_at(e, y, a)));
      break;
    case ZPX_OP_LOADP:
      zumir_emit_insn(e, MIR_new_insn(ctx, MIR_DMOV, zumir_emit_r(e, regs[d]),
                                      zumir_emit_at(e, p, a)));
      break;
    case ZPX_OP_LOADW:
      zumir_emit_insn(e, MIR_new_insn(ctx, MIR_DMOV, zumir_emit_r(e, regs[d]),
                                      zumir_emit_at(e, w, a)));
      break;
    case ZPX_OP_STOREO: zumir_emit_store(e, o, d, regs[a]); break;
    case ZPX_OP_STOREW: zumir_emit_store(e, w, d, regs[a]); break;
    case ZPX_OP_MOV: zumir_emit_move(e, regs[d], regs[a]); break;
    case ZPX_OP_ADD:
    case ZPX_OP_SUB:
    case ZPX_OP_MUL:
    case ZPX_OP_DIV: {
      MIR_insn_code_t k = op[0] == ZPX_OP_ADD   ? MIR_DADD
                          : op[0] == ZPX_OP_SUB ? MIR_DSUB
                          : op[0] == ZPX_OP_MUL ? MIR_DMUL
                                                : MIR_DDIV;
      zumir_emit_insn(e, MIR_new_insn(ctx, k, zumir_emit_r(e, regs[d]), zumir_emit_r(e, regs[a]),
                                      zumir_emit_r(e, regs[c])));
      break;
    }
    case ZPX_OP_NEG:
      zumir_emit_insn(e, MIR_new_insn(ctx, MIR_DNEG, zumir_emit_r(e, regs[d]),
                                      zumir_emit_r(e, regs[a])));
      break;
    case ZPX_OP_CALL1:
      zumir_emit_move(e, regs[d], zumir_emit_call1(e, zpx_math_name(a), regs[c]));
      break;
    case ZPX_OP_CALL2:
      zumir_emit_move(e, regs[d], zumir_emit_call2(e, zpx_math_name(a), regs[c], regs[x]));
      break;
    case ZPX_OP_LT:
    case ZPX_OP_LE:
    case ZPX_OP_GT:
    case ZPX_OP_GE:
    case ZPX_OP_EQ:
    case ZPX_OP_NE: {
      MIR_insn_code_t k = op[0] == ZPX_OP_LT   ? MIR_DLT
                          : op[0] == ZPX_OP_LE ? MIR_DLE
                          : op[0] == ZPX_OP_GT ? MIR_DGT
                          : op[0] == ZPX_OP_GE ? MIR_DGE
                          : op[0] == ZPX_OP_EQ ? MIR_DEQ
                                               : MIR_DNE;
      to_double(e, regs[d], zumir_emit_cmp(e, k, regs[a], regs[c]));
      break;
    }
    case ZPX_OP_AND:
    case ZPX_OP_OR: {
      MIR_reg_t ca = zumir_emit_cmp(e, MIR_DNE, regs[a], zero);
      MIR_reg_t cb = zumir_emit_cmp(e, MIR_DNE, regs[c], zero);
      MIR_reg_t r = zumir_emit_reg(e, MIR_T_I64);
      zumir_emit_insn(e, MIR_new_insn(ctx, op[0] == ZPX_OP_AND ? MIR_AND : MIR_OR,
                                      zumir_emit_r(e, r), zumir_emit_r(e, ca),
                                      zumir_emit_r(e, cb)));
      to_double(e, regs[d], r);
      break;
    }
    case ZPX_OP_NOT: to_double(e, regs[d], zumir_emit_cmp(e, MIR_DEQ, regs[a], zero)); break;
    case ZPX_OP_JMP: zumir_emit_jump(e, labels[d]); break;
    case ZPX_OP_JMPZ: zumir_emit_branch(e, MIR_DBEQ, regs[d], zero, labels[a]); break;
    }
  }
  if (labels[n] != NULL) zumir_emit_place(e, labels[n]);
  zumir_emit_return(e);
}

const char *zpx_lower_api(void) {
  const char *why = NULL;
  if (zumir_api_load(&why) == NULL) return why;
  return NULL;
}

int zpx_lower_tier_available(int tier) {
  if (zumir_api_ == NULL) return 0;
  uint32_t caps = zumir_api_->capabilities;
  uint32_t need = ZUMIR_CAP_COMPILE | ZUMIR_CAP_ENTRY | ZUMIR_CAP_RUN | ZUMIR_CAP_EXTERNAL;
  if (tier == ZPX_TIER_NATIVE) return (caps & need) == need;
  if (tier == ZPX_TIER_INTERP) return (caps & (need << 4)) == (need << 4);
  return 0;
}

int zpx_lower(const zpx_ir *ir, int tier, zpx_lowered *out, void *scratch, const char **message,
              int *where_block) {
  MIR_label_t *labels;
  MIR_reg_t *regs;
  int status;
  *message = NULL;
  *where_block = -1;
  memset(out, 0, sizeof(*out));
  if (zumir_api_ == NULL) return ZPX_LOWER_E_API;
  out->z = zumir_ctx_new(&status);
  /* A context MIR cannot make is a memory failure: zumir reports MIR's
   * allocation error there as ZUMIR_E_MIR and frees the context with its
   * message (pedrobtz/zumir#111), and MIR's initialisation fails for
   * nothing else. */
  if (out->z == NULL) return ZPX_LOWER_E_NO_MEMORY;
  status = register_imports(out->z);
  if (status != ZUMIR_OK) {
    zumir_status(out->z, message);
    return status == ZUMIR_E_NO_MEMORY ? ZPX_LOWER_E_NO_MEMORY : ZPX_LOWER_E_MIR;
  }
  MIR_context_t ctx = zumir_mir(out->z);
  labels = scratch;
  /* scratch holds the largest block's labels, then its registers. */
  size_t nl = 0;
  for (int b = 0; b < ZPX_IR_NBLOCKS; b++)
    if ((size_t) ir->block[b].ninsn + 1 > nl) nl = (size_t) ir->block[b].ninsn + 1;
  regs = (MIR_reg_t *) (labels + nl);
  MIR_module_t mods[ZPX_IR_NBLOCKS];
  for (int b = 0; b < ZPX_IR_NBLOCKS; b++) {
    mods[b] = MIR_new_module(ctx, block_name[b]);
    MIR_item_t func = MIR_new_func(ctx, block_name[b], 0, NULL, 5, MIR_T_D, "t", MIR_T_P, "y",
                                   MIR_T_P, "o", MIR_T_P, "p", MIR_T_P, "w");
    emit_block(ctx, func, ir, b, labels, regs);
    MIR_finish_func(ctx);
    MIR_finish_module(ctx);
  }
  int want = tier == ZPX_TIER_NATIVE ? ZUMIR_TIER_NATIVE : ZUMIR_TIER_INTERP;
  for (int b = 0; b < ZPX_IR_NBLOCKS; b++) {
    int got = 0;
    void *fn = zumir_compile(out->z, mods[b], block_name[b], want, &got, &status);
    if (fn == NULL) {
      *where_block = b;
      if (status == ZUMIR_E_MIR || status == ZUMIR_E_FAILED) zumir_status(out->z, message);
      if (status == ZUMIR_E_NO_MEMORY) return ZPX_LOWER_E_NO_MEMORY;
      return status == ZUMIR_E_NO_TIER ? ZPX_LOWER_E_NO_TIER : ZPX_LOWER_E_MIR;
    }
    out->fn[b] = (zpx_block_fn) fn;
  }
  out->tier = tier;
  return ZPX_LOWER_OK;
}

size_t zpx_lower_scratch_size(const zpx_ir *ir) {
  size_t nl = 0, nr = 0;
  for (int b = 0; b < ZPX_IR_NBLOCKS; b++) {
    if ((size_t) ir->block[b].ninsn + 1 > nl) nl = (size_t) ir->block[b].ninsn + 1;
    if ((size_t) ir->block[b].nreg > nr) nr = (size_t) ir->block[b].nreg;
  }
  return nl * sizeof(MIR_label_t) + nr * sizeof(MIR_reg_t);
}

void zpx_lower_free(zpx_lowered *l) {
  if (l == NULL || l->z == NULL) return;
  zumir_ctx_free(l->z);
  l->z = NULL;
  for (int b = 0; b < ZPX_IR_NBLOCKS; b++) l->fn[b] = NULL;
}
