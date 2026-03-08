#!/bin/sh
# make-release.sh - build a release tarball for nextp8-funcval
# Usage: make-release.sh [output.tar.gz] [--no-bitstream]
set -e

scriptdir="$(cd "$(dirname "$0")" && pwd)"
basedir="${scriptdir}/.."
funcvaldir="${basedir}"

version="$(date +%Y%m%d)"
DESTDIR="$(mktemp -d)"
tarball="${1:-nextp8-funcval_${version}.tar.gz}"

# Resolve --no-bitstream flag regardless of argument position
no_bitstream=0
for arg in "$@"; do
    [ "$arg" = "--no-bitstream" ] && no_bitstream=1
done

if [ -f "${tarball}" ]; then rm "${tarball}"; fi

MACHINES="${DESTDIR}/machines/nextp8"

# ── Base directory structure ──────────────────────────────────────────────────
mkdir -p "${MACHINES}"

# Base test suite subdirs (8.3-safe names)
for dir in \
    base/b01post \
    base/b02uart \
    base/b03srmrd \
    base/b04srmwr \
    base/b05vram \
    base/b06tmr \
    base/b07scrn \
    base/b08intr
do
    mkdir -p "${MACHINES}/${dir}"
done

# Main test suite subdirs (8.3-safe names)
for dir in \
    main/m01ver \
    main/m03joy \
    main/m04mouse \
    main/m05akbd \
    main/m05bkbd \
    main/m06sdspi \
    main/m07i2c \
    main/m09uart \
    main/m10digi \
    main/m12pal \
    main/m13p8aud \
    main/m14ovly
do
    mkdir -p "${MACHINES}/${dir}"
done

# ── core.cfg ──────────────────────────────────────────────────────────────────
cp "${funcvaldir}/machines/nextp8/core.cfg" "${MACHINES}/"

# ── Bitstream ─────────────────────────────────────────────────────────────────
if [ "${no_bitstream}" -eq 0 ]; then
    cp "${basedir}/../nextp8-core/nextp8-issue5.runs/impl_1/nextp8_top_issue5.bit" "${MACHINES}/core.bit"
fi

# ── Default ROM (menu) ────────────────────────────────────────────────────────
cp "${funcvaldir}/main/00_menu/00_menu.bin" "${MACHINES}/00_menu.bin"

# ── Base test ROMs (src longname → dest 8.3-safe dir, test.bin) ──────────────
cp "${funcvaldir}/base/01_post_code/01_post_code.bin"         "${MACHINES}/base/b01post/test.bin"
cp "${funcvaldir}/base/02_uart_output/02_uart_output.bin"     "${MACHINES}/base/b02uart/test.bin"
cp "${funcvaldir}/base/03_sram_read/03_sram_read.bin"         "${MACHINES}/base/b03srmrd/test.bin"
cp "${funcvaldir}/base/04_sram_write/04_sram_write.bin"       "${MACHINES}/base/b04srmwr/test.bin"
cp "${funcvaldir}/base/05_vram_access/05_vram_access.bin"     "${MACHINES}/base/b05vram/test.bin"
cp "${funcvaldir}/base/06_timers/06_timers.bin"               "${MACHINES}/base/b06tmr/test.bin"
cp "${funcvaldir}/base/07_screen_output/07_screen_output.bin" "${MACHINES}/base/b07scrn/test.bin"
cp "${funcvaldir}/base/08_interrupts/08_interrupts.bin"       "${MACHINES}/base/b08intr/test.bin"

# ── Main test ROMs (src longname → dest 8.3-safe dir, test.bin) ──────────────
cp "${funcvaldir}/main/01_version_info/01_version_info.bin"           "${MACHINES}/main/m01ver/test.bin"
cp "${funcvaldir}/main/03_joystick/03_joystick.bin"                   "${MACHINES}/main/m03joy/test.bin"
cp "${funcvaldir}/main/04_mouse/04_mouse.bin"                         "${MACHINES}/main/m04mouse/test.bin"
cp "${funcvaldir}/main/05a_built_in_keyboard/05a_built_in_keyboard.bin" "${MACHINES}/main/m05akbd/test.bin"
cp "${funcvaldir}/main/05b_external_keyboard/05b_external_keyboard.bin" "${MACHINES}/main/m05bkbd/test.bin"
cp "${funcvaldir}/main/06_sdspi/06_sdspi.bin"                         "${MACHINES}/main/m06sdspi/test.bin"
cp "${funcvaldir}/main/07_i2c/07_i2c.bin"                             "${MACHINES}/main/m07i2c/test.bin"
cp "${funcvaldir}/main/09_uart_esp/09_uart_esp.bin"                   "${MACHINES}/main/m09uart/test.bin"
cp "${funcvaldir}/main/10_digital_audio/10_digital_audio.bin"         "${MACHINES}/main/m10digi/test.bin"
cp "${funcvaldir}/main/12_palette/12_palette.bin"                     "${MACHINES}/main/m12pal/test.bin"
cp "${funcvaldir}/main/13_p8audio/13_p8audio.bin"                     "${MACHINES}/main/m13p8aud/test.bin"
cp "${funcvaldir}/main/14_overlay/14_overlay.bin"                     "${MACHINES}/main/m14ovly/test.bin"

# ── Create tarball ────────────────────────────────────────────────────────────
abstarball="$(readlink -f "${tarball}")"
oldpwd="${PWD}"
cd "${DESTDIR}"
tar -czf "${abstarball}" machines/
cd "${oldpwd}"
rm -rf "${DESTDIR}"

echo "Created: ${abstarball}"
