#!/bin/bash
# usage: asanR.sh script.R args...   (runs R binary directly so DYLD_INSERT_LIBRARIES survives SIP)
HERE=/private/tmp/claude-501/-Users-pbtz-src-github-com-pedrobtz-zumir/ea75353c-3806-4bef-bc9b-60bcaeea8d12/scratchpad/hunt/hunt-3
export R_HOME=$(R RHOME)
ASAN=$(clang -print-file-name=libclang_rt.asan_osx_dynamic.dylib)
export R_SHARE_DIR=$R_HOME/share R_INCLUDE_DIR=$R_HOME/include R_DOC_DIR=$R_HOME/doc
export ZUMIR_LIB=$HERE/asan-lib
export ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:halt_on_error=1:detect_stack_use_after_return=0
export UBSAN_OPTIONS=print_stacktrace=1
script=$1; shift
DYLD_INSERT_LIBRARIES=$ASAN DYLD_LIBRARY_PATH=$R_HOME/lib exec $R_HOME/bin/exec/R --no-save --no-restore --no-echo --file=$script --args "$@"
