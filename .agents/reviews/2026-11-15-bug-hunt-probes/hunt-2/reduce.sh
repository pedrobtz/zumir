#!/bin/bash
# reduce.sh file.c want_status out.c : greedy line removal keeping check1.R's exit status
f=$1; want=$2; out=$3; cp $f $out; n=$(wc -l < $out)
i=$((n-1))
while [ $i -ge 3 ]; do
  sed "${i}d" $out > $out.try
  Rscript check1.R $out.try > /dev/null 2>&1; rc=$?
  if [ $rc -eq $want ]; then cp $out.try $out; fi
  i=$((i-1))
done
rm -f $out.try; cat $out
