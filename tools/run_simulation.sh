#!/bin/bash
# Run FuncVal testbench simulation and capture output to log
# Usage: run_simulation.sh <rom_file.mem> <log_file>

set -e

ROM_FILE=$1
LOG_FILE=$2

if [ -z "$ROM_FILE" ] || [ -z "$LOG_FILE" ]; then
    echo "Usage: $0 <rom_file.mem> <log_file>"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TB_FUNCVAL_PATH="${TB_FUNCVAL_PATH:-$SCRIPT_DIR/../../nextp8-core/nextp8.srcs/tb_funcval}"

if [ ! -d "$TB_FUNCVAL_PATH" ]; then
    echo "ERROR: tb_funcval not found at $TB_FUNCVAL_PATH"
    exit 1
fi

ROM_ABS="$(readlink -f "$ROM_FILE")"
if [ ! -f "$ROM_ABS" ]; then
    echo "ERROR: ROM file not found: $ROM_ABS"
    exit 1
fi

OUTPUT_DIR="$(readlink -f "$(dirname "$LOG_FILE")")"
mkdir -p "$OUTPUT_DIR"

set -o pipefail
set -x
(
    cd "$TB_FUNCVAL_PATH"
    make sim ROM_FILE="$ROM_ABS" ${SDCARD_IMAGE:+SDCARD_IMAGE="$SDCARD_IMAGE"} ${XSIM_EXTRA_ARGS:+XSIM_EXTRA_ARGS="$XSIM_EXTRA_ARGS"} ${XSIM_WAVES:+XSIM_WAVES="$XSIM_WAVES"}
) 2>&1 | tee "$LOG_FILE"

# Move screenshots to output directory if they were generated in tb_funcval
TB_OUT_DIR="$(readlink -f "$TB_FUNCVAL_PATH")"
if [ "$TB_OUT_DIR" != "$OUTPUT_DIR" ]; then
    shopt -s nullglob
    for shot in "$TB_OUT_DIR"/screenshot_*.ppm; do
        mv -f "$shot" "$OUTPUT_DIR"/
    done
    shopt -u nullglob
fi
