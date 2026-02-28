#!/bin/bash
# Check FuncVal log output for Test 1 (POST Code Output)

set -e

LOG_FILE=${1:-run.log}

if [ ! -f "$LOG_FILE" ]; then
    echo "FAIL: Log file not found: $LOG_FILE" >&2
    exit 1
fi

EXPECTED_POSTS=(
    1
    2
    63
    21
    42
    15
    48
)

for code in "${EXPECTED_POSTS[@]}"; do
    if ! grep -q "POST: $code" "$LOG_FILE"; then
        echo "FAIL: Missing POST code: $code" >&2
        exit 1
    else
        echo "  ✓ Found POST: $code" >&2
    fi
done
