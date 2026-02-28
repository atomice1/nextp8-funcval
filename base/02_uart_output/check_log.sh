#!/bin/bash
# Check FuncVal log output for Test 2 (UART Output)

set -e

LOG_FILE=${1:-run.log}

if [ ! -f "$LOG_FILE" ]; then
    echo "FAIL: Log file not found: $LOG_FILE" >&2
    exit 1
fi

EXPECTED_STRINGS=(
    "A"
    "HELLO"
    "nextp8 UART test"
)

for expected in "${EXPECTED_STRINGS[@]}"; do
    if ! grep -q "$expected" "$LOG_FILE"; then
        echo "FAIL: Missing expected output: '$expected'" >&2
        exit 1
    else
        echo "  ✓ Found: '$expected'" >&2
    fi
done

if ! grep -q '[]!\"#\$%&'"'"'()*+,./0-9:;<=>?@A-Z\[\^_\`a-z{|}~\\-]' "$LOG_FILE"; then
    echo "FAIL: No printable ASCII characters found" >&2
    exit 1
else
    echo "  ✓ Printable ASCII characters present" >&2
fi
