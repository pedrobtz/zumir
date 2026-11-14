#!/bin/sh
# Builds tests/c/test-glue.c, zumir's R-free C test, with Mull for mutation.yaml: each source
# compiled in a command of its own with $MULL_CC (Mull re-parses every file
# from its recorded command line), then linked. The binaries go to
# build/mutation/. Only zumir's R-free files are mutated (mutation.yaml's
# include); MIR, compiled through src/zumir_mir.c, is vendored and left out.
set -eu
out=build/mutation
mkdir -p "$out"
cc=${MULL_CC:?MULL_CC is set by the r-actions mutation workflow}
flags="-Isrc -Isrc/vendor/mir -DNDEBUG"
for f in src/zumir_mir.c src/zumir_mir_gen.c src/zumir_c2mir.c src/zumir_prelude.c \
  src/zumir_mir_glue.c src/zumir_mir_stdio.c src/zumir_trap.c src/zumir_imports_d6.c \
  tests/c/test-glue.c; do
  $cc $flags -c "$f" -o "$out/$(basename "$f" .c).o"
done
lib="$out/zumir_mir.o $out/zumir_mir_gen.o $out/zumir_mir_glue.o $out/zumir_mir_stdio.o $out/zumir_trap.o"
$cc $lib "$out/zumir_c2mir.o" "$out/zumir_prelude.o" "$out/zumir_imports_d6.o" \
  "$out/test-glue.o" -o "$out/test-glue" -lm
