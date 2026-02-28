/* Test Suite: Version Info and Build Timestamp
 * Tests MMIO registers for hardware version and build timestamp
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "funcval.h"
#include "mmio.h"
#include "nextp8.h"

/* Test 1: Read build timestamp registers */
static int test_build_timestamp(void)
{
    uint32_t build_timestamp = MMIO_REG32(_BUILD_TIMESTAMP);

    test_puts("BUILD_TIMESTAMP: ");
    test_print_hex_long(build_timestamp);
    test_print_crlf();

    return TEST_PASS;
}

/* Test 2: Read hardware version registers */
static int test_hw_version(void)
{
    uint32_t hw_version = MMIO_REG32(_HW_VERSION);

    test_puts("HW_VERSION: ");
    test_print_hex_long(hw_version);
    test_print_crlf();

    return TEST_PASS;
}

/* Test 3: Platform detection */
static int test_platform_detect(void)
{
    platform_detect();

    const char *name = platform_get_name();

    test_puts("Platform: ");
    test_puts(name);
    test_print_crlf();

    return TEST_PASS;
}

/* Test 4: Read and write 32-bit debug register */
static int test_debug_register(void)
{
    uint32_t orig = MMIO_REG32(_DEBUG_REG);

    MMIO_REG32(_DEBUG_REG) = 0xDEADBEEF;

    uint32_t readback = MMIO_REG32(_DEBUG_REG);

    MMIO_REG32(_DEBUG_REG) = orig;

    if (readback != 0xDEADBEEF)
        return TEST_FAIL;

    return TEST_PASS;
}

/* Test 5: Read reset type register */
static int test_reset_type(void)
{
    uint8_t reset_type = MMIO_REG8(_RESET_TYPE);

    test_puts("RESET_TYPE: ");
    test_print_dec(reset_type);
    test_print_crlf();

    /* Reset type should be 0-2 */
    if (reset_type > 2)
        return TEST_FAIL;

    return TEST_PASS;
}

/* Test suite array */
TEST_SUITE(01_version_info,
           build_timestamp,
           hw_version,
           platform_detect,
           debug_register,
           reset_type);
