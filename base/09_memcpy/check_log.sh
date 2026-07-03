#!/bin/bash
# Check FuncVal log output for Test 9 (Memcpy Performance)

set -e

LOG_FILE=${1:-run.log}

if [ ! -f "$LOG_FILE" ]; then
    echo "FAIL: Log file not found: $LOG_FILE" >&2
    exit 1
fi

if grep -q "KiB time:" "$LOG_FILE"; then
    echo "  ✓ Memcpy time reported" >&2
else
    echo "FAIL: No memcpy time found" >&2
    exit 1
fi

# Check that time is non-zero (timer must have advanced)
TIME_OUTPUT=$(grep -o "[0-9]*KiB time: [0-9]* us" "$LOG_FILE" | tail -1)
if [ -z "$TIME_OUTPUT" ]; then
    echo "FAIL: Could not parse memcpy time" >&2
    exit 1
fi

TIME_US=$(echo "$TIME_OUTPUT" | grep -oP '\d+(?= us,)')
if [ -z "$TIME_US" ]; then
    echo "FAIL: Could not extract time in us" >&2
    exit 1
fi

if [ "$TIME_US" -eq 0 ]; then
    echo "FAIL: memcpy time is 0 us (timer didn't advance)" >&2
    exit 1
else
    echo "  ✓ Time is non-zero (${TIME_US} us)" >&2
fi

if grep -q "PASS" "$LOG_FILE"; then
    echo "  ✓ Found PASS message" >&2
else
    echo "FAIL: No PASS message found" >&2
    exit 1
fi

if grep -q "FAIL" "$LOG_FILE"; then
    echo "FAIL: Found FAIL message in log" >&2
    exit 1
else
    echo "  ✓ No FAIL messages found" >&2
fi
