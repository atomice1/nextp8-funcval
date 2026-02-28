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
 * Detection logic (per FUNCTIONAL_VALIDATION_PLAN.txt):
 * 1. If ADDR_BUILD_TIMESTAMP_LO is non-zero: Spectrum Next platform
 * 2. Else if ADDR_HW_VERSION_LO is non-zero: simulation (FuncVal testbench)
 * 3. Else: sQLux-nextp8 software model
 */
void platform_detect(void)
{
    /* Clear all platform flags */
    platform_is_spectrum_next = false;
    platform_is_simulation = false;
    platform_is_model = false;
    platform_is_interactive = false;
    platform_has_testbench = false;

    /* Check ADDR_BUILD_TIMESTAMP_LO (0x800004) */
    if (MMIO_REG16(_BUILD_TIMESTAMP_LO) != 0) {
        platform_is_spectrum_next = true;
        platform_is_interactive = true;
        return;
    }

    /* Check ADDR_HW_VERSION_LO (0x800008) */
    uint8_t hw_version_lo = MMIO_REG16(_HW_VERSION_LO) & 0xff;
    if (hw_version_lo < 0xfe || hw_version_lo > 0xff) {
        platform_is_simulation = true;
        platform_is_interactive = false;
        platform_has_testbench = true;
        return;
    }

    /* Must be sQLux-nextp8 model */
    platform_is_model = true;
    platform_is_interactive = (hw_version_lo & 0xff) == 0xfe;
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
