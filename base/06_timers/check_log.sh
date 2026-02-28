#!/bin/bash
# Check FuncVal log output for Test 6 (Timers)

set -e

LOG_FILE=${1:-run.log}

if [ ! -f "$LOG_FILE" ]; then
    echo "FAIL: Log file not found: $LOG_FILE" >&2
    exit 1
fi

if grep -q "All timer tests complete" "$LOG_FILE"; then
    echo "  ✓ Tests passed" >&2
else
    echo "FAIL: No 'All timer tests complete' message found" >&2
    exit 1
fi

if grep -q "FAIL" "$LOG_FILE"; then
    echo "FAIL: Found FAIL message in log" >&2
    exit 1
else
    echo "  ✓ No FAIL messages found" >&2
fi
