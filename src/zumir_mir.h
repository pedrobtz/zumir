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

/* The local patches, as src/vendor/PROVENANCE lists them (zumir_info()).
 * tools/verify-vendor checks the two agree. */
#define ZUMIR_MIR_PATCHES \
  "ea7f1643 Fix two warnings Rtools gcc 14 reports",\
  "773a4834 Release code memory with a zero size on Windows",\
  "dafb8e1b Remove variables clang -Wall reports as unused",\
  "f35a0021 Drop the semicolons after DEF_* macros at file scope",\
  "6edb5ce2 Give mem_page_size a prototype",\
  "1571f134 Use snprintf with the buffer's size instead of sprintf",\
  "0b8063f7 Clear curr_func and curr_module first in MIR_finish",\
  "4c27a18f Return NULL from MIR_init2 and _MIR_init instead of exiting",\
  "5803194c Load unaligned hash and reduce keys with memcpy",\
  "1c4b1cca Declare MIR_insn's operands as a flexible array member",\
  "c65e7044 Report code-memory failures through the error function",\
  "3e643e83 Spill float arguments in the Win64 interpreter shim",\
  "ca84f3e4 End MIR_output_item's expr data case",\
  "0068c2fd Drop the semicolons after DEF_* macros at file scope in c2mir",\
  "df2df6f5 Remove a variable c2mir declares and never uses",\
  "3762da20 Use snprintf with the buffer's size in c2mir",\
  "3bfb8d45 Let an embedder give headers in memory and forbid header files",\
  "7f6c4a03 Allocate and free all of c2mir's memory with MIR's allocator",\
  "f7672f28 Fold constant shifts without undefined behaviour in c2mir",\
  "96763195 Read a declaration's decl_spec only when the node is a declaration",\
  "b6e92b74 Fold integer constants by MIR's wrapping rules, without undefined behaviour",\
  "2ad09397 Read call offsets and addresses in x86-64 code with memcpy",\
  "55047839 Interpret integer operations by MIR's wrapping rules, and memory with memcpy",\
  "feacf4fe Stop at the end of the unit after a ## concatenation",\
  "4fb6f621 Fix what mingw gcc reports on c2mir's Windows x86-64 path",\
  "06104552 Empty the buffer when a macro argument has no start marker",\
  "46ed8157 Generate no code from a source with parse errors",\
  "a603752b Keep the current stream valid after the end of the unit",\
  "89750416 Grow both of the interpreter's call arrays",\
  "f1c087be Refuse a label that is not in the function",\
  "1d1d3ea6 Renumber the SSA edges copy_prop swaps, and skip operands with none",\
  "4c9aab06 Convert to _Bool by comparing with zero in c2mir",\
  "6a2b27b6 Convert floating values from 2^63 up to unsigned long long in c2mir",\
  "9dd2334d Give float.h C11's digits and the platform's long double, and NULL",\
  "e70b8330 Type character constants as int, and wide strings by their elements",\
  "124486f0 Convert _Generic's operand and #if's conditional as C11 does",\
  "0557c2a1 Alias a bit-field access by the member's type in c2mir",\
  "3c282253 Add MIR_interp_prepare: allocate at link what a call would",\
  "75b5075b Fold floating constants to integer types without undefined behaviour",\
  "06e8281a Fold signed addition and subtraction by wrapping in c2mir",\
  "b4c30634 Fold every signed operation by wrapping in c2mir"

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
