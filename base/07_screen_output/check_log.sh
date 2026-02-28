#!/bin/bash
# Check FuncVal log output for Test 6 (Screen Output)

set -e

LOG_FILE=${1:-run.log}

if [ ! -f "$LOG_FILE" ]; then
    echo "FAIL: Log file not found: $LOG_FILE" >&2
    exit 1
fi

# Check for expected UART messages
if ! grep -q "Test 7.1" "$LOG_FILE"; then
    echo "FAIL: Test 7.1 message not found" >&2
    exit 1
fi
echo "  ✓ Test 7.1 message found" >&2

if grep -q "FAIL" "$LOG_FILE"; then
    echo "FAIL: Found FAIL message in log" >&2
    exit 1
else
    echo "  ✓ No FAIL messages found" >&2
fi

# Count screenshots
SCREENSHOT_COUNT=$(ls -1 screenshot_*.ppm 2>/dev/null | wc -l)
if [ $SCREENSHOT_COUNT -lt 1 ]; then
    echo "FAIL: Expected 1 screenshots, found $SCREENSHOT_COUNT" >&2
    ls -la screenshot_*.ppm 2>/dev/null >&2 || true
    exit 1
fi
echo "  ✓ Found $SCREENSHOT_COUNT screenshots" >&2
