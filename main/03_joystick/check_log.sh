#!/bin/bash
# Check FuncVal log output for Test 03 (Joystick)

set -e

LOG_FILE=${1:-run.log}

if [ ! -f "$LOG_FILE" ]; then
    echo "FAIL: Log file not found: $LOG_FILE" >&2
    exit 1
fi

# Check for overall pass
if ! grep -q "OVERALL: PASS" "$LOG_FILE"; then
    echo "FAIL: No 'OVERALL: PASS' message found" >&2
    exit 1
else
    echo "  ✓ OVERALL: PASS" >&2
fi

# Check for no overall fail
if grep -q "OVERALL: FAIL" "$LOG_FILE"; then
    echo "FAIL: Found 'OVERALL: FAIL' message in log" >&2
    exit 1
fi

echo "  ✓ All checks passed" >&2
exit 0
