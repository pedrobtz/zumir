#!/bin/sh
# tools/ci/platform.sh <os>: zumir's per-OS checks, run once on each runner of
# platforms.yaml (os is macos, windows or ubuntu), with R_ACTIONS_PROFILE set
# to quick or full. Every check is a step here, in order; set -e names the
# one that failed. Run from the package root; it also runs locally.
#
#   1. MIR's own tests on the vendored, patched copy, built without R, with
#      R's compiler and flags (design D2: the checks every MIR bump re-runs).
#   2. The package built, then its symbols: nothing of MIR's exported, and
#      no CRAN-flagged library symbol linked.
#   3. The error trap on two threads at once (tests/c); the C API header
#      alone as C99 and C++11, and the consumer package under
#      tests/consumer; on Linux, the R suite against the package built in
#      step 2.
#   4. Linux: the package built with MIR forced out installs, loads and
#      reports no target (tests/testthat/test-no-target.R).
#
# Stages 3 to 5 add theirs.
set -eu
os=${1:?usage: tools/ci/platform.sh <macos|windows|ubuntu>}
here=$(pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
R=${R_HOME:+$R_HOME/bin/}R
Rscript=${R_HOME:+$R_HOME/bin/}Rscript
command -v Rscript >/dev/null 2>&1 && R=R && Rscript=Rscript

group() { echo "::group::$*"; }
endgroup() { echo "::endgroup::"; }

group "Toolchain"
"$Rscript" -e 'cat(R.version.string, R.version$arch, "\n")'
cc_cmd=$("$R" CMD config CC)
r_cflags=$("$R" CMD config CFLAGS)
echo "CC = $cc_cmd"
echo "CFLAGS = $r_cflags"
$cc_cmd --version | head -1
uname -m
endgroup

# ---------------------------------------------------------------------------
# 1. MIR's own tests (Stage 2). The test sources and GNUmakefile come from
# the fork at the pinned fork commit; the library sources are replaced by
# src/vendor/mir/, so what is tested is what the package compiles. Built with
# R's compiler and flags, not upstream's: no -fsigned-char (on Linux aarch64
# char is unsigned, so this is where that is shown), -DNDEBUG as R builds,
# the interpreter's MIR_DIRECT_DISPATCH, as src/zumir_mir.c sets it, and
# gcc's -fno-tree-sra below. The exit and stderr redirection is not built
# in: MIR's tests read their own stderr and exit status.
# inline-test is left out: it builds c2mir's driver; c2mir itself is tested
# on c-tests below, through zumir's compile path.
group "MIR's own tests"
fork_url=$(awk '/^  Fork:/ {print $2}' src/vendor/PROVENANCE)
fork_pin=$(awk '/^  Fork commit:/ {print $3}' src/vendor/PROVENANCE)
mir="$tmp/mir"
# No CRLF: run-test compares the .mir files it reads with what it writes.
# The setting goes into the clone's config: `git -c` on the clone alone
# does not reach the checkout below, and Windows runners set autocrlf=true.
git -c core.autocrlf=false clone -q --no-checkout "$fork_url" "$mir"
git -C "$mir" config core.autocrlf false
git -C "$mir" -c advice.detachedHead=false checkout -q "$fork_pin"
grep -v '^#' tools/mir-files.txt | grep -v '^[[:space:]]*$' | while read -r f; do
  cp "src/vendor/mir/$f" "$mir/$f"
done
# upstream's makefile tests $(CC) by name, so it gets the bare compiler; the
# rest of R's CC (a -std= flag, -arch) goes into CFLAGS.
cc_bin=${cc_cmd%% *}
cc_rest=${cc_cmd#"$cc_bin"}
mir_cflags="$cc_rest $r_cflags -fPIC -DNDEBUG -DMIR_DIRECT_DISPATCH=1 -DGITCOMMIT=$fork_pin"
# gcc gets -fno-tree-sra, as src/zumir_mir_gcc.h gives MIR's sources in the
# package: without it Rtools gcc miscompiles the interpreter.
if ! $cc_cmd --version 2>&1 | grep -qi clang; then
  mir_cflags="$mir_cflags -fno-tree-sra"
fi
targets="adt-test simplify-test io-test scan-test interp-test readme-example-test
  gen-test-loop gen-test-sieve gen-test-get-thunk-addr gen-issue219-test
  gen-inlay-test gen-test1 gen-test2 gen-test3 gen-test4 gen-test5 gen-test6
  gen-test7 gen-test8 gen-test9 gen-test10 gen-test11 gen-test12 gen-test13
  gen-test14 gen-test15 gen-test16 gen-test18"
failed=""
for t in $targets; do
  if make -C "$mir" CC="$cc_bin" CFLAGS="$mir_cflags" "$t" > "$tmp/mir-$t.log" 2>&1; then
    echo "pass $t"
  else
    echo "FAIL $t"
    tail -30 "$tmp/mir-$t.log"
    failed="$failed $t"
  fi
done
endgroup
[ -z "$failed" ] || { echo "MIR's own tests failed:$failed" >&2; exit 1; }

# ---------------------------------------------------------------------------
# 2. The installed package's symbols (Stage 2).
group "Install"
lib="$tmp/lib"
mkdir -p "$lib"
"$R" CMD INSTALL --preclean --no-test-load --library="$lib" . > "$tmp/install.log" 2>&1 ||
  { cat "$tmp/install.log"; exit 1; }
tail -3 "$tmp/install.log"
endgroup
so=$(ls "$lib"/zumir/libs/zumir.so "$lib"/zumir/libs/*/zumir.dll "$lib"/zumir/libs/zumir.dll 2>/dev/null | head -1)
[ -n "$so" ] || { echo "no shared object installed" >&2; exit 1; }
echo "shared object: $so"

# Defined, exported symbols, and undefined (imported) ones, one per line.
exports() {
  case "$os" in
    macos) nm -gU "$1" | awk '{print $NF}' | sed 's/^_//' ;;
    windows) objdump -p "$1" | awk '/^\[Ordinal\/Name Pointer\] Table/ {on = 1; next}
                                    on && /^\t\[/ {print $NF; next} on && NF == 0 {on = 0}' ;;
    *) nm -D --defined-only "$1" | awk '{print $NF}' ;;
  esac
}
imports() {
  case "$os" in
    macos) nm -u "$1" | awk '{print $NF}' | sed 's/^_//' ;;
    windows) objdump -p "$1" | awk '/DLL Name:/ {on = 1; next}
                                    on && /^\t+[0-9a-f]+ +[0-9]+ / {print $NF}' ;;
    *) nm -u "$1" | awk '{print $NF}' | sed 's/@.*//' ;;
  esac
}

group "Exported symbols"
exports "$so" | sort -u | tee "$tmp/exports"
endgroup
# MIR's names are hidden (design §4.2, "Symbol hiding"): $(C_VISIBILITY) on
# Unix, src/zumir-win.def on Windows. Only R_init_zumir is ours to export.
if grep -E 'MIR|mir|VARR|DLIST|HTAB|zumir' "$tmp/exports" | grep -vx 'R_init_zumir' > "$tmp/leaked"; then
  echo "FAIL: the shared object exports these:" >&2
  cat "$tmp/leaked" >&2
  exit 1
fi
grep -qx 'R_init_zumir' "$tmp/exports" ||
  { echo "FAIL: R_init_zumir is not exported" >&2; exit 1; }
echo "ok: R_init_zumir is the only zumir or MIR symbol exported"

group "Imported symbols"
imports "$so" | sort -u | tee "$tmp/imports"
endgroup
# The library symbols CRAN's "checking compiled code" flags, by base name:
# a leading underscore, a _chk suffix and the platform's spelling of stderr
# and stdout stripped. None may be linked (design §11: no exit, abort,
# stdout or stderr in the shared object): MIR's go through
# src/zumir_mir_stdio.h, and its sprintf calls are snprintf on the fork.
ALLOWED=""
sed -e 's/^_*//' -e 's/_chk$//' \
  -e 's/^stderrp$/stderr/' -e 's/^stdoutp$/stdout/' \
  -e 's/^acrt_iob_func$/stderr/' -e 's/^stdio_common_vsprintf$/sprintf/' \
  -e 's/^assert_fail$/abort/' -e 's/^assert_rtn$/abort/' -e 's/^assert$/abort/' \
  "$tmp/imports" | sort -u > "$tmp/bases"
flagged=$(grep -xE 'exit|abort|printf|vprintf|puts|putchar|sprintf|vsprintf|rand|srand|stderr|stdout' \
  "$tmp/bases" || true)
echo "CRAN-flagged library symbols linked: $(echo ${flagged:-none} | tr "\n" " ")"
bad=""
for s in $flagged; do
  case " $ALLOWED " in *" $s "*) ;; *) bad="$bad $s" ;; esac
done
[ -z "$bad" ] || { echo "FAIL: links CRAN-flagged symbols:$bad" >&2; exit 1; }
echo "ok: no CRAN-flagged symbol linked"

# ---------------------------------------------------------------------------
# The error trap on two threads at once (Stage 3): tests/c, built without R
# from MIR and zumir's R-free trap files, with R's compiler and flags; on
# Linux also under ASan and UBSan.
group "Trap on two threads"
trap_srcs="tests/c/test-trap-threads.c src/zumir_mir.c src/zumir_mir_gen.c src/zumir_mir_glue.c src/zumir_mir_stdio.c src/zumir_trap.c"
$cc_cmd $r_cflags -Isrc -DNDEBUG $trap_srcs -o "$tmp/test-trap-threads" -lpthread -lm
"$tmp/test-trap-threads"
if [ "$os" = ubuntu ]; then
  $cc_cmd -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -Isrc -DNDEBUG \
    $trap_srcs -o "$tmp/test-trap-threads-asan" -lpthread -lm
  "$tmp/test-trap-threads-asan"
fi
endgroup

# ---------------------------------------------------------------------------
# c2mir on MIR's c-tests corpus (Stage 8, #20): tests/c/c-tests-runner.c,
# built without R from the vendored MIR and c2mir and zumir's R-free files,
# compiles each test as cfunction() would (the prelude, no header read from
# a file, D6's list) plus the harness's printf, abort and exit, and runs
# it on each tier by c-tests/runtests.sh's rules. The tests that fail must
# be exactly those tools/c-tests-expected.txt lists, on both tiers. The
# corpus comes from the fork's clone of step 1. POSIX only: the runner
# forks a child per test, so Windows does not run it.
if [ "$os" != windows ]; then
  group "c2mir on c-tests"
  runner_srcs="tests/c/c-tests-runner.c src/zumir_mir.c src/zumir_mir_gen.c src/zumir_c2mir.c
    src/zumir_prelude.c src/zumir_imports_d6.c src/zumir_mir_glue.c src/zumir_mir_stdio.c
    src/zumir_trap.c"
  runner_flags="-Isrc -Isrc/vendor/mir -DNDEBUG -DZUMIR_C2MIR_TEST_HEADERS"
  $cc_cmd $r_cflags $runner_flags $runner_srcs -o "$tmp/c-tests-runner" -lm
  dirs="havoc new andrewchambers_c gcc lacc"
  # A line "<test> [<tag>,...] <why>" is expected only where a tag holds:
  # <os>-<arch> as below, or <os>-<arch>:<tier>; a line without brackets,
  # everywhere.
  arch=$(uname -m)
  case "$arch" in arm64) [ "$os" = macos ] || arch=aarch64 ;; esac
  platform="$os-$arch"
  echo "platform: $platform"
  for tier in gen interp; do
    grep -v '^#' tools/c-tests-expected.txt | awk -v p="$platform" -v t="$tier" '
      NF == 0 { next }
      $2 ~ /^\[/ { list = $2; gsub(/[][]/, "", list); n = split(list, ps, ",");
                    for (i = 1; i <= n; i++) if (ps[i] == p || ps[i] == p ":" t) { print $1; next }
                    next }
      { print $1 }' | sort > "$tmp/c-expected"
    "$tmp/c-tests-runner" "$mir/c-tests" "$tier" $dirs > "$tmp/c-$tier.txt"
    tail -1 "$tmp/c-$tier.txt"
    awk '$2 == "fail" {print $1}' "$tmp/c-$tier.txt" | sort > "$tmp/c-$tier-failed"
    if ! cmp -s "$tmp/c-expected" "$tmp/c-$tier-failed"; then
      echo "FAIL: c-tests on $tier differ from tools/c-tests-expected.txt (< listed, > failed):" >&2
      diff "$tmp/c-expected" "$tmp/c-$tier-failed" >&2 || true
      grep -F -f "$tmp/c-$tier-failed" "$tmp/c-$tier.txt" | grep -v -F -f "$tmp/c-expected" >&2 || true
      exit 1
    fi
    echo "ok: on $tier, exactly the listed tests fail"
  done
  # Not vacuous (#20): a passing test whose .expect is broken must fail.
  probe="$tmp/c-probe/new"
  mkdir -p "$probe"
  cp "$mir/c-tests/new/bf1.c" "$probe/"
  printf 'not what it prints\n' > "$probe/bf1.expect"
  "$tmp/c-tests-runner" "$tmp/c-probe" gen new | grep -q "^new/bf1.c fail" ||
    { echo "FAIL: a broken .expect was not reported" >&2; exit 1; }
  echo "ok: a broken .expect is reported"
  if [ "$os" = ubuntu ] && [ "${R_ACTIONS_PROFILE:-quick}" = full ]; then
    grep -v '^#' tools/c-tests-expected.txt | awk -v p="$platform" -v t=gen '
      NF == 0 { next }
      $2 ~ /^\[/ { list = $2; gsub(/[][]/, "", list); n = split(list, ps, ",");
                    for (i = 1; i <= n; i++) if (ps[i] == p || ps[i] == p ":" t) { print $1; next }
                    next }
      { print $1 }' | sort > "$tmp/c-expected"
    $cc_cmd -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all $runner_flags \
      $runner_srcs -o "$tmp/c-tests-runner-asan" -lm
    # A child the sanitizers stop shows as a failure; the list must still
    # match, so a sanitizer finding fails this step.
    ASAN_OPTIONS=detect_leaks=0 "$tmp/c-tests-runner-asan" "$mir/c-tests" gen $dirs > "$tmp/c-asan.txt"
    tail -1 "$tmp/c-asan.txt"
    awk '$2 == "fail" {print $1}' "$tmp/c-asan.txt" | sort | cmp -s "$tmp/c-expected" - ||
      { echo "FAIL: c-tests under ASan and UBSan differ from the list" >&2;
        awk '$2 == "fail"' "$tmp/c-asan.txt" | grep -v -F -f "$tmp/c-expected" >&2; exit 1; }
    echo "ok: c-tests under ASan and UBSan"
  fi
  endgroup
fi

# ---------------------------------------------------------------------------
# The C API (Stage 5): inst/include/zumir.h compiles alone as C99 and as
# C++11 with -pedantic -Werror, and the consumer package under
# tests/consumer builds against the installed zumir, passes its tests, and
# links nothing CRAN flags (the header's inline code included).
group "C API header"
rinc=$("$Rscript" -e 'cat(R.home("include"))')
printf '#include <zumir.h>\nint zumir_header_ok;\n' > "$tmp/h.c"
# R's own headers go in as system headers: R 4.6's R_ext/Boolean.h uses a
# C23 enum, which is R's business, and the check is of zumir.h.
$cc_cmd -std=c99 -pedantic -Wall -Wextra -Werror -fsyntax-only -isystem "$rinc" -Iinst/include "$tmp/h.c"
cxx_cmd=$("$R" CMD config CXX)
cp "$tmp/h.c" "$tmp/h.cpp"
$cxx_cmd -std=c++11 -pedantic -Wall -Wextra -Werror -fsyntax-only -isystem "$rinc" -Iinst/include "$tmp/h.cpp"
echo "ok: zumir.h compiles alone as C99 and C++11, -pedantic -Werror"
endgroup

group "Consumer package"
R_LIBS="$lib${R_LIBS:+:$R_LIBS}" "$R" CMD INSTALL --preclean --library="$lib" tests/consumer > "$tmp/consumer.log" 2>&1 ||
  { cat "$tmp/consumer.log"; exit 1; }
R_LIBS="$lib${R_LIBS:+:$R_LIBS}" "$Rscript" -e 'testthat::test_dir("tests/consumer/tests/testthat", package = "zumirconsumer", load_package = "installed", reporter = "summary", stop_on_failure = TRUE)'
cso=$(ls "$lib"/zumirconsumer/libs/zumirconsumer.so "$lib"/zumirconsumer/libs/*/zumirconsumer.dll "$lib"/zumirconsumer/libs/zumirconsumer.dll 2>/dev/null | head -1)
imports "$cso" | sed -e 's/^_*//' -e 's/_chk$//' -e 's/^stderrp$/stderr/' -e 's/^acrt_iob_func$/stderr/' |
  grep -xE 'exit|abort|printf|vprintf|puts|putchar|sprintf|vsprintf|stderr|stdout' > "$tmp/cflagged" || true
if [ -s "$tmp/cflagged" ]; then
  echo "FAIL: the consumer links CRAN-flagged symbols through zumir.h:" >&2
  cat "$tmp/cflagged" >&2
  exit 1
fi
echo "ok: the consumer links no CRAN-flagged symbol"
endgroup

# ---------------------------------------------------------------------------
# The R suite on this runner (Linux, Stage 4): R CMD check has no aarch64
# Linux leg, and the W+X test (test-tier.R) must run on both Linux CPUs.
if [ "$os" = ubuntu ]; then
  group "R suite"
  R_LIBS="$lib${R_LIBS:+:$R_LIBS}" NOT_CRAN=true "$Rscript" tools/ci-run-tests.R
  endgroup
fi

# ---------------------------------------------------------------------------
# 4. No target (Stage 2): MIR forced out, as on a CPU it does not target.
if [ "$os" = ubuntu ]; then
  group "No target"
  nolib="$tmp/nolib"
  mkdir -p "$nolib"
  ZUMIR_NO_MIR=1 "$R" CMD INSTALL --preclean --library="$nolib" . > "$tmp/nomir.log" 2>&1 ||
    { cat "$tmp/nomir.log"; exit 1; }
  grep -q -- '-DZUMIR_NO_MIR' "$tmp/nomir.log" ||
    { echo "FAIL: ZUMIR_NO_MIR=1 did not reach the compiler" >&2; exit 1; }
  if nm "$nolib/zumir/libs/zumir.so" | grep -q 'MIR_init'; then
    echo "FAIL: MIR was compiled in with ZUMIR_NO_MIR=1" >&2
    exit 1
  fi
  ZUMIR_EXPECT_NO_MIR=1 R_LIBS="$nolib${R_LIBS:+:$R_LIBS}" "$Rscript" tools/ci-run-tests.R
  endgroup
fi

echo "platform.sh: $os done"
