#!/bin/bash
# Common test wrapper: runs sim/model and validates log
# Usage: run_wrapper.sh [sim|model] [rom_base] [log_file]
# Defaults: mode=model, rom_base=test, log_file=run.log

set -e

MODE=$1
ROM_BASE=$2
LOG_FILE=$3

if [ "$MODE" != "sim" ] && [ "$MODE" != "model" ]; then
    MODE="model"
    ROM_BASE=$1
    LOG_FILE=$2
fi

ROM_BASE=${ROM_BASE:-$(basename $(pwd))}
LOG_FILE=${LOG_FILE:-run.log}

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RUNNER="${RUNNER:-$SCRIPT_DIR/run.sh}"
CHECKER="./check_log.sh"

if [ ! -x "$RUNNER" ]; then
    echo "ERROR: run.sh not found or not executable at $RUNNER" >&2
    exit 1
fi

echo "${ROM_BASE}"

if [ ! -f "$CHECKER" -a "${ROM_BASE}" != 00_menu ]; then
    echo "ERROR: check_log.sh not found in $(pwd)" >&2
    exit 1
fi

# Clean old screenshots for screen output test (if any)
rm -f screenshot_*.ppm

set -x
"$RUNNER" "$MODE" "$ROM_BASE" "$LOG_FILE"

if [ -f "$CHECKER" ]; then
    bash "$CHECKER" "$LOG_FILE"
fi
