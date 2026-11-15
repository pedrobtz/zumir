#!/bin/bash
# bt.sh file.c [tier]: backtrace of the crash under lldb
R -d "lldb --batch -o run -o bt -o quit --" --no-save -f runc.R --args "$@" 2>&1 | grep -A25 "stop reason" | head -40
