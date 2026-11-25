/* Copied from pedrobtz/zupmxode at 5f637a1, src/core/ir.h, unchanged but for
 * its include paths: the IR zupmxode lowers, lowered as zupmxode lowers
 * it, for test-zupmxode.R. Copy it again with tools/zpx-fixture.R's
 * fixture when zupmxode's IR or lowering changes. */
/* The IR (design §4.1, D13): the package's own representation of a
 * compiled model, which the lowering turns into MIR and which a saved model
 * stores (D18). Builds without R; nothing here includes R's headers.
 *
 * A model is two arrays: an int32 header-and-code vector and a constant
 * pool of doubles.
 *
 *   header[0]  ZPX_IR_MAGIC
 *   header[1]  ZPX_IR_VERSION
 *   header[2]  ny     states
 *   header[3]  npar   parameters (thetas, etas, covariates: one vector)
 *   header[4]  nwork  work slots shared by the blocks of one evaluation
 *   header[5]  nconst entries of the constant pool
 *   then ZPX_IR_NBLOCKS block descriptors of 3 ints each, in block order:
 *     nreg   registers the block uses
 *     nout   entries of the block's output array
 *     ninsn  instructions
 *   then the instructions of every block, block after block, each
 *   ZPX_IR_WIDTH ints: opcode, then four operands. Operands an opcode does
 *   not use are 0.
 *
 * A block is a function
 *
 *   void block(double t, const double *y, double *out,
 *              const double *par, double *work);
 *
 * over a file of nreg double registers, all 0.0 on entry. Jumps go forward
 * only, so every block terminates, and an instruction index one past the
 * last ends the block. The verifier is what makes a pair of arrays safe to
 * lower or evaluate: an accepted IR reads and writes only inside the
 * arrays its header declares. */
#ifndef ZUPMXODE_IR_H
#define ZUPMXODE_IR_H

#include <stddef.h>
#include <stdint.h>

#define ZPX_IR_MAGIC 0x5a50584f /* "ZPXO" */
#define ZPX_IR_VERSION 1
#define ZPX_IR_HEADER 6
#define ZPX_IR_NBLOCKS 5
#define ZPX_IR_WIDTH 5

/* Limits (assurance Limits rows). Counts, per block or per model. */
#define ZPX_IR_MAX_INSNS 1000000 /* instructions in one block */
#define ZPX_IR_MAX_REGS 65536    /* registers in one block */
#define ZPX_IR_MAX_SLOTS 65536   /* ny, npar, nwork, nout: each */
#define ZPX_IR_MAX_CONSTS 1000000

enum zpx_block { ZPX_PRE = 0, ZPX_IVL = 1, ZPX_RHS = 2, ZPX_POST = 3, ZPX_ERR = 4 };

/* Opcodes. d, a, b are registers; i an index into y, par, work or out; k
 * into the constant pool; f into the math table; j an instruction index
 * in the same block, greater than the jump's own. */
enum zpx_op {
  ZPX_OP_CONST = 1,  /* d k:   r[d] = const[k] */
  ZPX_OP_LOADT = 2,  /* d:     r[d] = t */
  ZPX_OP_LOADY = 3,  /* d i:   r[d] = y[i] */
  ZPX_OP_LOADP = 4,  /* d i:   r[d] = par[i] */
  ZPX_OP_LOADW = 5,  /* d i:   r[d] = work[i] */
  ZPX_OP_STOREO = 6, /* i a:   out[i] = r[a] */
  ZPX_OP_STOREW = 7, /* i a:   work[i] = r[a] */
  ZPX_OP_MOV = 8,    /* d a:   r[d] = r[a] */
  ZPX_OP_ADD = 10,   /* d a b: r[d] = r[a] + r[b] */
  ZPX_OP_SUB = 11,
  ZPX_OP_MUL = 12,
  ZPX_OP_DIV = 13,
  ZPX_OP_NEG = 14,   /* d a */
  ZPX_OP_CALL1 = 20, /* d f a:   r[d] = f(r[a]) */
  ZPX_OP_CALL2 = 21, /* d f a b: r[d] = f(r[a], r[b]) */
  ZPX_OP_LT = 30,    /* d a b: r[d] = r[a] < r[b] ? 1 : 0, C's meaning on NaN */
  ZPX_OP_LE = 31,
  ZPX_OP_GT = 32,
  ZPX_OP_GE = 33,
  ZPX_OP_EQ = 34,
  ZPX_OP_NE = 35,
  ZPX_OP_AND = 36,   /* d a b: (r[a] != 0 && r[b] != 0) ? 1 : 0 */
  ZPX_OP_OR = 37,
  ZPX_OP_NOT = 38,   /* d a:   r[a] == 0 ? 1 : 0 */
  ZPX_OP_JMP = 40,   /* j */
  ZPX_OP_JMPZ = 41   /* a j:   if r[a] == 0 jump to j */
};

/* The math table: CALL1 and CALL2 name a function by its index here. The
 * order is part of the IR format; append only. */
enum zpx_fn {
  ZPX_FN_EXP = 0, ZPX_FN_LOG, ZPX_FN_SQRT, ZPX_FN_ABS, ZPX_FN_FLOOR,
  ZPX_FN_CEILING, ZPX_FN_SIN, ZPX_FN_COS, ZPX_FN_TAN, ZPX_FN_GAMMA,
  ZPX_FN_LGAMMA, ZPX_FN_LOGIT, ZPX_FN_EXPIT, ZPX_FN_PROBIT,
  ZPX_FN_PROBITINV,
  ZPX_FN_NFN1, /* one-argument functions end here */
  ZPX_FN_POW = 32, ZPX_FN_MIN, ZPX_FN_MAX,
  ZPX_FN_NFN2_END
};

typedef double (*zpx_fn1)(double);
typedef double (*zpx_fn2)(double, double);

/* The function for a CALL1 or CALL2 index, and the symbol name the
 * lowering imports it by; NULL for an index that is not in the table. */
zpx_fn1 zpx_math1(int f);
zpx_fn2 zpx_math2(int f);
const char *zpx_math_name(int f);

/* Functions of the table that the C library does not have. */
double zpx_logit(double x);
double zpx_expit(double x);
double zpx_probit(double p);
double zpx_probitinv(double x);
double zpx_min(double a, double b);
double zpx_max(double a, double b);
double zpx_pow(double x, double y);
double zpx_sqrt(double x);
double zpx_gamma(double x);
double zpx_lgamma(double x);

/* A verified view of an IR. */
typedef struct zpx_ir {
  int ny, npar, nwork, nconst;
  struct {
    int nreg, nout, ninsn;
    const int32_t *code; /* ninsn * ZPX_IR_WIDTH ints */
    size_t start;        /* offset of code in the int vector */
  } block[ZPX_IR_NBLOCKS];
  const double *consts;
} zpx_ir;

/* Verification errors: 0 is success. */
enum {
  ZPX_IR_OK = 0,
  ZPX_IR_E_SHORT,   /* fewer ints than the header and descriptors need */
  ZPX_IR_E_MAGIC,
  ZPX_IR_E_VERSION,
  ZPX_IR_E_COUNT,   /* a count negative or above its limit */
  ZPX_IR_E_LENGTH,  /* the code length is not what the descriptors say */
  ZPX_IR_E_CONSTS,  /* the constant pool's length is not nconst */
  ZPX_IR_E_OPCODE,
  ZPX_IR_E_OPERAND, /* an operand out of its range */
  ZPX_IR_E_JUMP     /* a jump not forward, or past the end */
};

/* Checks code[0..ncode) and consts[0..nconsts) and fills *ir. On failure
 * returns the error and, when where is not NULL, the block and the
 * instruction it was found at (-1 for the header). */
int zpx_ir_verify(const int32_t *code, size_t ncode, const double *consts, size_t nconsts,
                  zpx_ir *ir, int *where_block, int *where_insn);

const char *zpx_ir_error_message(int error);

/* Evaluates one block of a verified IR, as the lowered code would: the
 * reference the tiers are compared with in the tests and the oracle of
 * the verifier's fuzz harness. regs holds the block's nreg doubles. Not an
 * execution tier (design §2, D15). */
void zpx_ir_eval(const zpx_ir *ir, int block, double t, const double *y, double *out,
                 const double *par, double *work, double *regs);

#endif
