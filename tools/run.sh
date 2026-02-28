#!/bin/bash
# Unified runner for FuncVal tests
# Usage: run.sh <sim|model> <rom_base|rom_file> <log_file>
# If rom_base has no extension, .mem is used for sim and .bin for model.

set -e

MODE=$1
ROM_BASE=$2
LOG_FILE=$3

if [ -z "$MODE" ] || [ -z "$ROM_BASE" ] || [ -z "$LOG_FILE" ]; then
    echo "Usage: $0 <sim|model> <rom_base|rom_file> <log_file>"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

case "$MODE" in
    sim)
        if [[ "$ROM_BASE" == *.mem ]]; then
            ROM_FILE="$ROM_BASE"
        else
            ROM_FILE="${ROM_BASE}.mem"
        fi
        set -x
        exec "$SCRIPT_DIR/run_simulation.sh" "$ROM_FILE" "$LOG_FILE"
        ;;
    model)
        if [[ "$ROM_BASE" == *.bin ]]; then
            ROM_FILE="$ROM_BASE"
        else
            ROM_FILE="${ROM_BASE}.bin"
        fi
        set -x
        exec "$SCRIPT_DIR/run_model.sh" "$ROM_FILE" "$LOG_FILE"
        ;;
    *)
        echo "ERROR: Unknown mode: $MODE"
        echo "Usage: $0 <sim|model> <rom_base|rom_file> <log_file>"
        exit 1
        ;;
esac
