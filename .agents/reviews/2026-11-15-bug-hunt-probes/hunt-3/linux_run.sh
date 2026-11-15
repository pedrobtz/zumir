#!/bin/bash
# inside the container: install zumir, then run the unload probes and the capi probes
mkdir -p /tmp/lib && cp -R /work/linux-src /tmp/zsrc && R CMD INSTALL --no-test-load --library=/tmp/lib /tmp/zsrc > /tmp/build.log 2>&1 || { tail -30 /tmp/build.log; exit 1; }
export ZUMIR_LIB=/tmp/lib
cd /work
for m in unload_call unload_gc unload_exit unload_reload_call dynunload; do
  for k in mirf; do echo "=== $m $k"; Rscript p02_unload.R $m $k 2>&1 | tail -12; echo "exit=$?  (pipe status: ${PIPESTATUS[0]})"; done
done
echo "=== nested finalizer (linux)"; Rscript p03b_nested.R native 2>&1 | tail -4
echo "=== first-call OOM interp (linux)"; Rscript p04_alloc_call.R rf interp 10 2>&1 | head -6
