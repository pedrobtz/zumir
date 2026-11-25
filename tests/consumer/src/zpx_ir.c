/* Copied from pedrobtz/zupmxode at 5f637a1, src/core/ir.c, unchanged but for
 * its include paths: the IR zupmxode lowers, lowered as zupmxode lowers
 * it, for test-zupmxode.R. Copy it again with tools/zpx-fixture.R's
 * fixture when zupmxode's IR or lowering changes. */
/* The IR's verifier, its reference evaluator and the math table
 * (src/core/ir.h). Builds without R. */
#include "zpx_ir.h"

#include <math.h>

/* ---- The math table ---------------------------------------------------- */

double zpx_logit(double x) { return log(x / (1.0 - x)); }

/* 1 / (1 + exp(-x)), written so that neither side overflows. */
double zpx_expit(double x) {
  if (x >= 0) return 1.0 / (1.0 + exp(-x));
  double e = exp(x);
  return e / (1.0 + e);
}

/* The standard normal distribution function, erfc(z) / 2 with
 * z = -x / sqrt(2). z is formed with its rounding error dz, and the result
 * corrected by erfc's derivative, -2 / sqrt(pi) exp(-z^2), times dz: left
 * alone, the error of z grows as x^2 units in the last place in the lower
 * tail (40 at x = -10). */
double zpx_probitinv(double x) {
  const double hi = 0.70710678118654757;   /* 1 / sqrt(2), rounded */
  const double lo = -4.8336466567264567e-17; /* 1 / sqrt(2) - hi */
  const double two_over_sqrt_pi = 1.1283791670955126;
  if (!isfinite(x)) return isnan(x) ? x : (x > 0 ? 1.0 : 0.0);
  double z = -x * hi;
  double dz = fma(-x, hi, -z) + -x * lo;
  return 0.5 * (erfc(z) - two_over_sqrt_pi * exp(-z * z) * dz);
}

/* The standard normal quantile: Wichura's algorithm AS 241 (PPND16), good
 * to about 1e-16 relative, which R's qnorm() also uses. */
double zpx_probit(double p) {
  double q, r, val;
  if (isnan(p)) return p;
  if (p < 0 || p > 1) return NAN;
  if (p == 0) return -INFINITY;
  if (p == 1) return INFINITY;
  q = p - 0.5;
  if (fabs(q) <= 0.425) {
    r = 0.180625 - q * q;
    val = q * (((((((r * 2509.0809287301226727 + 33430.575583588128105) * r +
                    67265.770927008700853) * r + 45921.953931549871457) * r +
                  13731.693765509461125) * r + 1971.5909503065514427) * r +
                133.14166789178437745) * r + 3.387132872796366608) /
          (((((((r * 5226.495278852545925 + 28729.085735721942674) * r +
                39307.89580009271061) * r + 21213.794301586595867) * r +
              5394.1960214247511077) * r + 687.1870074920579083) * r +
            42.313330701600911252) * r + 1.0);
    return val;
  }
  r = q < 0 ? p : 1.0 - p;
  r = sqrt(-log(r));
  if (r <= 5.0) {
    r -= 1.6;
    val = (((((((r * 7.7454501427834140764e-4 + 0.0227238449892691845833) * r +
                0.24178072517745061177) * r + 1.27045825245236838258) * r +
              3.64784832476320460504) * r + 5.7694972214606914055) * r +
            4.6303378461565452959) * r + 1.42343711074968357734) /
          (((((((r * 1.05075007164441684324e-9 + 5.475938084995344946e-4) * r +
                0.0151986665636164571966) * r + 0.14810397642748007459) * r +
              0.68976733498510000455) * r + 1.6763848301838038494) * r +
            2.05319162663775882187) * r + 1.0);
  } else {
    r -= 5.0;
    val = (((((((r * 2.01033439929228813265e-7 + 2.71155556874348757815e-5) * r +
                0.0012426609473880784386) * r + 0.026532189526576123093) * r +
              0.29656057182850489123) * r + 1.7848265399172913358) * r +
            5.4637849111641143699) * r + 6.6579046435011037772) /
          (((((((r * 2.04426310338993978564e-15 + 1.4215117583164458887e-7) * r +
                1.8463183175100546818e-5) * r + 7.868691311456132591e-4) * r +
              0.0148753612908506148525) * r + 0.13692988092273580531) * r +
            0.59983220655588793769) * r + 1.0);
  }
  return q < 0.0 ? -val : val;
}

/* R's min() and max() of two numbers: NaN when either is. */
double zpx_min(double a, double b) {
  if (isnan(a) || isnan(b)) return a + b;
  return b < a ? b : a;
}

double zpx_max(double a, double b) {
  if (isnan(a) || isnan(b)) return a + b;
  return b > a ? b : a;
}

/* R's x ^ y. It is C's pow() except at the edges: anything to the power 0,
 * and 1 to any power, is 1 even with NaN; a zero base has no sign (0 to a
 * positive power is 0, to a negative one Inf); a negative base to an
 * infinite power is NaN, and so is -Inf to a power that is not an
 * integer; -Inf to a negative integer is 0. Squares are x * x, exactly. */
double zpx_pow(double x, double y) {
  if (y == 0.0 || x == 1.0) return 1.0;
  if (isnan(x) || isnan(y)) return x + y;
  if (y == 2.0) return x * x;
  if (x == 0.0) return y > 0.0 ? 0.0 : INFINITY;
  if (x < 0.0) {
    if (isinf(y)) return NAN;
    if (isinf(x)) {
      if (y != floor(y)) return NAN;
      if (y < 0.0) return 0.0;
      return fmod(y, 2.0) != 0.0 ? x : -x;
    }
  }
  return pow(x, y);
}

/* The gamma function: NaN at its poles (zero and the negative integers),
 * as R gives, where C's tgamma() gives an infinity at zero. */
double zpx_gamma(double x) {
  if (x == 0.0 || (x < 0.0 && x == floor(x))) return NAN;
  return tgamma(x);
}

/* log |gamma(x)|, re-entrant: the C library's lgamma() writes the sign of
 * gamma to a global on some systems, a race once blocks run on worker
 * threads (design §4.4). Below 15, log |tgamma(x)| (tgamma is re-entrant);
 * from 15, Stirling's series to the term in x^-9, exact to rounding there
 * (mingw-w64's library lgamma() also overflowed at 1e300 and was Inf at
 * 2^-1074); for large negative x, the reflection formula. Poles give
 * +Inf, as R's lgamma(). */
double zpx_lgamma(double x) {
  if (isnan(x)) return x;
  if (isinf(x)) return INFINITY;
  if (x <= 0 && x == floor(x)) return INFINITY;
  if (fabs(x) < 1e-17) return -log(fabs(x));
  if (x >= 15) {
    const double half_log_two_pi = 0.91893853320467274;
    double l = log(x), r = 1.0 / x, r2 = r * r;
    double series = r * (1.0 / 12 - r2 * (1.0 / 360 - r2 * (1.0 / 1260 - r2 * (1.0 / 1680 -
                                                                                r2 / 1188))));
    return (x - 0.5) * l - x + half_log_two_pi + series;
  }
  if (x > -15) return log(fabs(tgamma(x)));
  /* gamma(x) gamma(1 - x) = pi / sin(pi x), with 1 - x >= 16. */
  const double pi = 3.14159265358979323846;
  double sp = sin(pi * (x - floor(x)));
  return log(pi / fabs(sp)) - zpx_lgamma(1 - x);
}

static double fn_abs(double x) { return fabs(x); }

/* sqrt() compiled here, where the compiler emits the hardware's correctly
 * rounded square root, rather than taken from the C library by name:
 * mingw-w64's library sqrt() returns 1 for 1 - 2^-53. */
double zpx_sqrt(double x) { return sqrt(x); }

static const struct {
  zpx_fn1 fn;
  const char *name;
} math1[ZPX_FN_NFN1] = {
  [ZPX_FN_EXP] = {exp, "exp"},
  [ZPX_FN_LOG] = {log, "log"},
  [ZPX_FN_SQRT] = {zpx_sqrt, "zpx_sqrt"},
  [ZPX_FN_ABS] = {fn_abs, "zpx_abs"},
  [ZPX_FN_FLOOR] = {floor, "floor"},
  [ZPX_FN_CEILING] = {ceil, "ceil"},
  [ZPX_FN_SIN] = {sin, "sin"},
  [ZPX_FN_COS] = {cos, "cos"},
  [ZPX_FN_TAN] = {tan, "tan"},
  [ZPX_FN_GAMMA] = {zpx_gamma, "zpx_gamma"},
  [ZPX_FN_LGAMMA] = {zpx_lgamma, "zpx_lgamma"},
  [ZPX_FN_LOGIT] = {zpx_logit, "zpx_logit"},
  [ZPX_FN_EXPIT] = {zpx_expit, "zpx_expit"},
  [ZPX_FN_PROBIT] = {zpx_probit, "zpx_probit"},
  [ZPX_FN_PROBITINV] = {zpx_probitinv, "zpx_probitinv"},
};

static const struct {
  zpx_fn2 fn;
  const char *name;
} math2[ZPX_FN_NFN2_END - ZPX_FN_POW] = {
  {zpx_pow, "zpx_pow"},
  {zpx_min, "zpx_min"},
  {zpx_max, "zpx_max"},
};

zpx_fn1 zpx_math1(int f) {
  return f >= 0 && f < ZPX_FN_NFN1 ? math1[f].fn : NULL;
}

zpx_fn2 zpx_math2(int f) {
  return f >= ZPX_FN_POW && f < ZPX_FN_NFN2_END ? math2[f - ZPX_FN_POW].fn : NULL;
}

const char *zpx_math_name(int f) {
  if (f >= 0 && f < ZPX_FN_NFN1) return math1[f].name;
  if (f >= ZPX_FN_POW && f < ZPX_FN_NFN2_END) return math2[f - ZPX_FN_POW].name;
  return NULL;
}

/* ---- The verifier ------------------------------------------------------ */

const char *zpx_ir_error_message(int error) {
  switch (error) {
  case ZPX_IR_OK: return "ok";
  case ZPX_IR_E_SHORT: return "the IR is shorter than its header";
  case ZPX_IR_E_MAGIC: return "the IR does not start with its magic number";
  case ZPX_IR_E_VERSION: return "the IR version is not one this package reads";
  case ZPX_IR_E_COUNT: return "a count in the IR header is negative or above its limit";
  case ZPX_IR_E_LENGTH: return "the IR's length is not what its header says";
  case ZPX_IR_E_CONSTS: return "the constant pool's length is not what the header says";
  case ZPX_IR_E_OPCODE: return "an IR instruction has an unknown opcode";
  case ZPX_IR_E_OPERAND: return "an IR instruction has an operand out of range";
  case ZPX_IR_E_JUMP: return "an IR jump does not go forward within its block";
  }
  return "unknown IR error";
}

static int in(int32_t v, int n) { return v >= 0 && v < n; }

/* Checks one instruction; the operands are op[1..4]. */
static int check_insn(const int32_t *op, int index, const zpx_ir *ir, int b) {
  int nreg = ir->block[b].nreg, nout = ir->block[b].nout, ninsn = ir->block[b].ninsn;
  int32_t d = op[1], x = op[2], y = op[3], z = op[4];
  int ok;
  switch (op[0]) {
  case ZPX_OP_CONST: ok = in(d, nreg) && in(x, ir->nconst) && y == 0 && z == 0; break;
  case ZPX_OP_LOADT: ok = in(d, nreg) && x == 0 && y == 0 && z == 0; break;
  case ZPX_OP_LOADY: ok = in(d, nreg) && in(x, ir->ny) && y == 0 && z == 0; break;
  case ZPX_OP_LOADP: ok = in(d, nreg) && in(x, ir->npar) && y == 0 && z == 0; break;
  case ZPX_OP_LOADW: ok = in(d, nreg) && in(x, ir->nwork) && y == 0 && z == 0; break;
  case ZPX_OP_STOREO: ok = in(d, nout) && in(x, nreg) && y == 0 && z == 0; break;
  case ZPX_OP_STOREW: ok = in(d, ir->nwork) && in(x, nreg) && y == 0 && z == 0; break;
  case ZPX_OP_MOV:
  case ZPX_OP_NEG:
  case ZPX_OP_NOT: ok = in(d, nreg) && in(x, nreg) && y == 0 && z == 0; break;
  case ZPX_OP_ADD:
  case ZPX_OP_SUB:
  case ZPX_OP_MUL:
  case ZPX_OP_DIV:
  case ZPX_OP_LT:
  case ZPX_OP_LE:
  case ZPX_OP_GT:
  case ZPX_OP_GE:
  case ZPX_OP_EQ:
  case ZPX_OP_NE:
  case ZPX_OP_AND:
  case ZPX_OP_OR: ok = in(d, nreg) && in(x, nreg) && in(y, nreg) && z == 0; break;
  case ZPX_OP_CALL1:
    ok = in(d, nreg) && zpx_math1(x) != NULL && in(y, nreg) && z == 0;
    break;
  case ZPX_OP_CALL2:
    ok = in(d, nreg) && zpx_math2(x) != NULL && in(y, nreg) && in(z, nreg);
    break;
  case ZPX_OP_JMP:
    if (x != 0 || y != 0 || z != 0) return ZPX_IR_E_OPERAND;
    return d > index && d <= ninsn ? ZPX_IR_OK : ZPX_IR_E_JUMP;
  case ZPX_OP_JMPZ:
    if (!in(d, nreg) || y != 0 || z != 0) return ZPX_IR_E_OPERAND;
    return x > index && x <= ninsn ? ZPX_IR_OK : ZPX_IR_E_JUMP;
  default: return ZPX_IR_E_OPCODE;
  }
  return ok ? ZPX_IR_OK : ZPX_IR_E_OPERAND;
}

static int count_ok(int32_t v, int32_t limit) { return v >= 0 && v <= limit; }

int zpx_ir_verify(const int32_t *code, size_t ncode, const double *consts, size_t nconsts,
                  zpx_ir *ir, int *where_block, int *where_insn) {
  const size_t head = ZPX_IR_HEADER + 3 * ZPX_IR_NBLOCKS;
  size_t at = head;
  if (where_block != NULL) *where_block = -1;
  if (where_insn != NULL) *where_insn = -1;
  if (code == NULL || ncode < head) return ZPX_IR_E_SHORT;
  if (code[0] != ZPX_IR_MAGIC) return ZPX_IR_E_MAGIC;
  if (code[1] != ZPX_IR_VERSION) return ZPX_IR_E_VERSION;
  if (!count_ok(code[2], ZPX_IR_MAX_SLOTS) || !count_ok(code[3], ZPX_IR_MAX_SLOTS) ||
      !count_ok(code[4], ZPX_IR_MAX_SLOTS) || !count_ok(code[5], ZPX_IR_MAX_CONSTS))
    return ZPX_IR_E_COUNT;
  ir->ny = code[2];
  ir->npar = code[3];
  ir->nwork = code[4];
  ir->nconst = code[5];
  if (nconsts != (size_t) ir->nconst || (nconsts > 0 && consts == NULL)) return ZPX_IR_E_CONSTS;
  ir->consts = consts;
  for (int b = 0; b < ZPX_IR_NBLOCKS; b++) {
    const int32_t *d = code + ZPX_IR_HEADER + 3 * b;
    if (!count_ok(d[0], ZPX_IR_MAX_REGS) || !count_ok(d[1], ZPX_IR_MAX_SLOTS) ||
        !count_ok(d[2], ZPX_IR_MAX_INSNS)) {
      if (where_block != NULL) *where_block = b;
      return ZPX_IR_E_COUNT;
    }
    ir->block[b].nreg = d[0];
    ir->block[b].nout = d[1];
    ir->block[b].ninsn = d[2];
    /* At most 5 blocks of 1e6 instructions of 5 ints: no overflow. */
    if ((size_t) d[2] * ZPX_IR_WIDTH > ncode - at) return ZPX_IR_E_LENGTH;
    ir->block[b].start = at;
    ir->block[b].code = code + at;
    at += (size_t) d[2] * ZPX_IR_WIDTH;
  }
  if (at != ncode) return ZPX_IR_E_LENGTH;
  for (int b = 0; b < ZPX_IR_NBLOCKS; b++) {
    for (int i = 0; i < ir->block[b].ninsn; i++) {
      int e = check_insn(ir->block[b].code + (size_t) i * ZPX_IR_WIDTH, i, ir, b);
      if (e != ZPX_IR_OK) {
        if (where_block != NULL) *where_block = b;
        if (where_insn != NULL) *where_insn = i;
        return e;
      }
    }
  }
  return ZPX_IR_OK;
}

/* ---- The reference evaluator ------------------------------------------- */

void zpx_ir_eval(const zpx_ir *ir, int block, double t, const double *y, double *out,
                 const double *par, double *work, double *regs) {
  const int32_t *code = ir->block[block].code;
  int n = ir->block[block].ninsn;
  for (int r = 0; r < ir->block[block].nreg; r++) regs[r] = 0.0;
  for (int i = 0; i < n;) {
    const int32_t *op = code + (size_t) i * ZPX_IR_WIDTH;
    int32_t d = op[1], a = op[2], b = op[3], c = op[4];
    i++;
    switch (op[0]) {
    case ZPX_OP_CONST: regs[d] = ir->consts[a]; break;
    case ZPX_OP_LOADT: regs[d] = t; break;
    case ZPX_OP_LOADY: regs[d] = y[a]; break;
    case ZPX_OP_LOADP: regs[d] = par[a]; break;
    case ZPX_OP_LOADW: regs[d] = work[a]; break;
    case ZPX_OP_STOREO: out[d] = regs[a]; break;
    case ZPX_OP_STOREW: work[d] = regs[a]; break;
    case ZPX_OP_MOV: regs[d] = regs[a]; break;
    case ZPX_OP_ADD: regs[d] = regs[a] + regs[b]; break;
    case ZPX_OP_SUB: regs[d] = regs[a] - regs[b]; break;
    case ZPX_OP_MUL: regs[d] = regs[a] * regs[b]; break;
    case ZPX_OP_DIV: regs[d] = regs[a] / regs[b]; break;
    case ZPX_OP_NEG: regs[d] = -regs[a]; break;
    case ZPX_OP_CALL1: regs[d] = zpx_math1(a)(regs[b]); break;
    case ZPX_OP_CALL2: regs[d] = zpx_math2(a)(regs[b], regs[c]); break;
    case ZPX_OP_LT: regs[d] = regs[a] < regs[b] ? 1.0 : 0.0; break;
    case ZPX_OP_LE: regs[d] = regs[a] <= regs[b] ? 1.0 : 0.0; break;
    case ZPX_OP_GT: regs[d] = regs[a] > regs[b] ? 1.0 : 0.0; break;
    case ZPX_OP_GE: regs[d] = regs[a] >= regs[b] ? 1.0 : 0.0; break;
    case ZPX_OP_EQ: regs[d] = regs[a] == regs[b] ? 1.0 : 0.0; break;
    case ZPX_OP_NE: regs[d] = regs[a] != regs[b] ? 1.0 : 0.0; break;
    case ZPX_OP_AND: regs[d] = (regs[a] != 0.0 && regs[b] != 0.0) ? 1.0 : 0.0; break;
    case ZPX_OP_OR: regs[d] = (regs[a] != 0.0 || regs[b] != 0.0) ? 1.0 : 0.0; break;
    case ZPX_OP_NOT: regs[d] = regs[a] == 0.0 ? 1.0 : 0.0; break;
    case ZPX_OP_JMP: i = d; break;
    case ZPX_OP_JMPZ:
      if (regs[d] == 0.0) i = a;
      break;
    }
  }
}
