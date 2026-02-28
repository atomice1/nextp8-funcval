#!/bin/bash
# Run sQLux model with FuncVal ROM and capture output to log
# Usage: run_model.sh <rom_file.bin> <log_file>

set -e

ROM_FILE=$1
LOG_FILE=$2

if [ -z "$ROM_FILE" ] || [ -z "$LOG_FILE" ]; then
    echo "Usage: $0 <rom_file.bin> <log_file>"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SQLUX_PATH="${SQLUX_PATH:-$SCRIPT_DIR/../../sQLux-nextp8/build/sqlux}"

if [ ! -x "$SQLUX_PATH" ]; then
    echo "ERROR: sQLux not found at $SQLUX_PATH"
    echo "Build sQLux or set SQLUX_PATH"
    exit 1
fi

ROM_ABS="$(readlink -f "$ROM_FILE")"
if [ ! -f "$ROM_ABS" ]; then
    echo "ERROR: ROM file not found: $ROM_ABS"
    exit 1
fi

mkdir -p "$(dirname "$LOG_FILE")"

set -o pipefail
set -x
exec "$SQLUX_PATH" --funcval --rom1 "$ROM_ABS" 2>&1 | tee "$LOG_FILE"

