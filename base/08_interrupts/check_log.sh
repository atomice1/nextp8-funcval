#!/bin/bash
# Check FuncVal log output for Test 7 (Interrupts)

set -e

LOG_FILE=${1:-run.log}

if [ ! -f "$LOG_FILE" ]; then
    echo "FAIL: Log file not found: $LOG_FILE" >&2
    exit 1
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

if grep -q "Interrupt count" "$LOG_FILE"; then
    echo "  ✓ Interrupt count reported" >&2
else
    echo "WARNING: No interrupt count found (may not be implemented yet)" >&2
fi
