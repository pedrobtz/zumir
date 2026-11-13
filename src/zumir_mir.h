/* Which MIR this build has, and whether it has one at all.
 *
 * MIR's mir.c and mir-gen.c stop with #error on a CPU it does not target
 * (design §4.2, "Other CPUs"). zumir must still install there, so the
 * wrapper files zumir_mir.c and zumir_mir_gen.c compile MIR only when
 * ZUMIR_HAVE_MIR is 1, and every entry reports that there is no target
 * otherwise (§7). Windows aarch64 is left out too (D25): MIR has no
 * Windows ABI for it. ZUMIR_NO_MIR, set by the build (the ZUMIR_NO_MIR
 * environment variable, through Makevars), forces MIR out on a CPU that
 * has it, so the no-target path is tested on a leg that can run it. */
#ifndef ZUMIR_MIR_H
#define ZUMIR_MIR_H

/* The upstream commit src/vendor/mir/ is built from (src/vendor/PROVENANCE);
 * tools/verify-vendor checks that the two agree. */
#define ZUMIR_MIR_COMMIT "dc2c763f1c8bc746102ce87933d866335f9e6c06"

#if defined(ZUMIR_NO_MIR)
#define ZUMIR_HAVE_MIR 0
#elif defined(_WIN32) && !(defined(__x86_64__) || defined(_M_AMD64))
#define ZUMIR_HAVE_MIR 0
#elif defined(__x86_64__) || defined(_M_AMD64) || defined(__aarch64__) || \
  defined(__PPC64__) || defined(__s390x__)
#define ZUMIR_HAVE_MIR 1
#elif defined(__riscv) && __riscv_xlen == 64 && __riscv_flen == 64 && \
  __riscv_float_abi_double && __riscv_mul && __riscv_div && \
  __riscv_compressed
/* mir.c's own condition: rv64imafdc, without the 128-bit Q extension. */
#define ZUMIR_HAVE_MIR 1
#else
#define ZUMIR_HAVE_MIR 0
#endif

#endif
