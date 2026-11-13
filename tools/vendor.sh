#!/bin/sh
# tools/vendor.sh [fork-checkout]: copy MIR into src/vendor/mir/ from the fork
# (pedrobtz/mir) at the fork commit pinned in src/vendor/PROVENANCE. Run from
# the package root. Without an argument it clones the fork. Never edit
# src/vendor/mir/ by hand: change the fork's zumir branch and re-run this.
#
# To change a patch, or add one: commit it on the fork's zumir branch (one
# commit per patch, on the upstream commit), put it alone on a fix/ branch
# cut from the fork's master, then update "Fork commit:" and the patch list
# in PROVENANCE and re-run.
#
# To update MIR (design D2: every bump re-runs the spike's checks):
#   1. Fast-forward the fork's master from upstream, and rebase the zumir
#      branch onto the new upstream commit; drop a patch upstream now has.
#   2. Update "Commit:", "Date:", "Fork commit:" and the patch list.
#   3. Run this script. It fails, with a diff, if the enums and structs of
#      mir.h changed (tools/mir-abi against tools/mir-abi.txt): bump the C
#      API table's MIR ABI number (D21, from Stage 5), then re-run with
#      MIR_ABI_REVIEWED=1 to record the new tools/mir-abi.txt.
#   4. tools/verify-vendor, R CMD check, and the platforms workflow.
set -eu
PROV=src/vendor/PROVENANCE
fork_url=$(awk '/^  Fork:/ {print $2}' "$PROV")
fork_pin=$(awk '/^  Fork commit:/ {print $3}' "$PROV")
[ -n "$fork_pin" ] || { echo "no Fork commit: line in $PROV" >&2; exit 1; }

src=${1:-}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
if [ -z "$src" ]; then
  src="$tmp/mir"
  # No CRLF: the files are compared byte for byte.
  git -c core.autocrlf=false clone -q --no-checkout "$fork_url" "$src"
fi
git -C "$src" -c advice.detachedHead=false checkout -q "$fork_pin"

files=$(grep -v '^#' tools/mir-files.txt | grep -v '^[[:space:]]*$')

# The consumer-visible part of mir.h (D21), against the last import's.
tools/mir-abi "$src/mir.h" > "$tmp/abi.txt"
if ! cmp -s "$tmp/abi.txt" tools/mir-abi.txt; then
  if [ "${MIR_ABI_REVIEWED:-0}" != 1 ]; then
    echo "The enums and structs of mir.h changed (tools/mir-abi):" >&2
    diff -u tools/mir-abi.txt "$tmp/abi.txt" >&2 || true
    echo "Bump the MIR ABI number in the C API table (D21), then re-run with" >&2
    echo "MIR_ABI_REVIEWED=1 to record the new tools/mir-abi.txt." >&2
    exit 1
  fi
  cp "$tmp/abi.txt" tools/mir-abi.txt
  echo "recorded the new tools/mir-abi.txt"
fi

rm -rf src/vendor/mir
mkdir -p src/vendor/mir
for f in $files; do cp "$src/$f" "src/vendor/mir/$f"; done
echo "vendored MIR from $fork_url at $fork_pin: $(echo "$files" | wc -l | tr -d ' ') files"
echo "now run tools/verify-vendor"
