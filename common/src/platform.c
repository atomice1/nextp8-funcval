/* Platform Detection for nextp8 Functional Validation Library
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdbool.h>
#include <stdint.h>
#include "funcval.h"
#include "mmio.h"
#include "nextp8.h"

/* Platform detection flags */
bool platform_is_spectrum_next = false;
bool platform_is_simulation = false;
bool platform_is_model = false;
bool platform_is_interactive = false;
bool platform_has_testbench = false;

/* platform_detect - Detect which platform we're running on
 *
 * Sets global flags:
 * - platform_is_spectrum_next: true if running on Spectrum Next hardware
 * - platform_is_simulation: true if running in FuncVal testbench simulation
 * - platform_is_model: true if running on sQLux-nextp8 software model
 * - platform_is_interactive: true if running on an interactive platform
 * - platform_has_testbench: true if running on a platform with the funcval testbench
 *
 * Detection logic:
 * BUILD_TIMESTAMP comes from USR_ACCESSE2 (Xilinx primitive):
 *   - xsim simulation model always returns 0x12345678
 *   - real hardware returns the actual FPGA build Unix timestamp (non-zero, != sentinel)
 *   - sQLux model returns 0 (no USR_ACCESSE2 implementation)
 * HW_VERSION_LO low byte:
 *   - sQLux model: 0xFE (interactive) or 0xFF (non-interactive)
 *   - hardware/simulation: 0x00 (PATCH_VERSION)
 *
 * 1. BUILD_TIMESTAMP == 0x12345678  -> xsim FuncVal testbench simulation
 * 2. BUILD_TIMESTAMP != 0           -> Spectrum Next real hardware
 * 3. HW_VERSION_LO byte == 0xFE    -> sQLux model, interactive
 * 4. else                           -> sQLux model, non-interactive
 */
void platform_detect(void)
{
    /* Clear all platform flags */
    platform_is_spectrum_next = false;
    platform_is_simulation = false;
    platform_is_model = false;
    platform_is_interactive = false;
    platform_has_testbench = false;

    /* Read BUILD_TIMESTAMP (from USR_ACCESSE2 in RTL) */
    uint32_t build_timestamp = MMIO_REG32(_BUILD_TIMESTAMP);

    if (build_timestamp == 0x12345678) {
        /* xsim always returns this sentinel for USR_ACCESSE2 */
        platform_is_simulation = true;
        platform_is_interactive = false;
        platform_has_testbench = true;
        return;
    }

    if (build_timestamp != 0) {
        /* Real FPGA hardware: USR_ACCESSE2 contains actual build timestamp */
        platform_is_spectrum_next = true;
        platform_is_interactive = true;
        return;
    }

    /* BUILD_TIMESTAMP == 0: sQLux model */
    uint8_t hw_version_lo = MMIO_REG16(_HW_VERSION_LO) & 0xff;
    platform_is_model = true;
    platform_is_interactive = (hw_version_lo == 0xfe);
    platform_has_testbench = true;
}

/* platform_get_name - Get platform name as string
 * Returns pointer to platform name string
 */
const char *platform_get_name(void)
{
    if (platform_is_spectrum_next)
        return "Spectrum Next";
    if (platform_is_simulation)
        return "FuncVal Testbench";
    return "sQLux Model";
}
