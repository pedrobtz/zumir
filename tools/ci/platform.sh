#!/bin/sh
# tools/ci/platform.sh <os>: zumir's per-OS checks, run once on each runner of
# platforms.yaml (macos, windows or ubuntu), with R_ACTIONS_PROFILE set to
# quick or full. Every check is a line here, in order; set -e names the one
# that failed.
#
# Nothing to check yet. Stage 2 adds MIR's own tests on the vendored, patched
# copy built without R, and the symbol checks; Stages 3 to 5 add theirs.
set -eu
os=$1
echo "platform.sh: $os, profile ${R_ACTIONS_PROFILE:-unset}: no checks yet"
