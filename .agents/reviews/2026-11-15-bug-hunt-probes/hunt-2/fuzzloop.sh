#!/bin/bash
# fuzzloop.sh seed nprog : restarts after a crash, keeping the crashing source
cd "$(dirname "$0")"; seed=$1; n=$2; p=1; mkdir -p fuzzcrash
while [ $p -le $n ]; do
  Rscript fuzz.R $seed $n $p > fuzz-log-$seed-$p.txt 2>&1; rc=$?
  grep -h "^FAIL\|^done\|^skip" fuzz-log-$seed-$p.txt
  if [ $rc -eq 0 ]; then break; fi
  cur=$(awk '{print $2}' fuzz-progress-$seed)
  cp fuzz-current-$seed.c fuzzcrash/crash-$seed-$cur.c; echo "CRASH rc=$rc seed=$seed p=$cur"
  p=$((cur+1))
done
