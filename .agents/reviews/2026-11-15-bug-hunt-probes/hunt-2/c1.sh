#!/bin/bash
# c1.sh id tier thread x
cd "$(dirname "$0")"
perl -e 'alarm 60; exec @ARGV' Rscript c1.R "$@" > c1.tmp 2>&1; rc=$?
if [ $rc -ne 0 ]; then echo "id=$1 tier=$2 thr=$3 x=$4 | !!!! rc=$rc $(grep -m1 'caught\|Error' c1.tmp)"; else cat c1.tmp; fi
