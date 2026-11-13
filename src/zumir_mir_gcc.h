/* gcc's scalar replacement of aggregates (-ftree-sra) miscompiles MIR's
 * interpreter on Windows: with R's flags, MIR's own interp-test8,
 * readme-example and gen-test-sieve crash under Rtools gcc 14, and pass
 * with -fno-tree-sra added and nothing else changed (Stage 2, #31,
 * 2026-11-13). Upstream's makefile has added -fno-tree-sra for gcc since
 * 2019, everywhere, with no reason recorded; zumir does the same for MIR's
 * sources alone. A pragma, not PKG_CFLAGS, because R CMD check reports -f
 * flags in Makevars as non-portable. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("no-tree-sra")
#endif
