/* Test Suite: SD Card SPI Block Device
 * Tests SD card block read/write using sdblockdevice driver
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include <string.h>
#include "nextp8.h"
#include "funcval.h"
#include "mmio.h"
#include "sdblockdevice.h"

/* Global SD block device instance */
static struct _sd_block_device sd_device;

/* Buffer for block operations (512 bytes) */
static uint8_t block_buffer[512];
static uint8_t compare_buffer[512];

/* Test 1: Initialize SD card */
static int test_sd_init(void)
{
    test_puts("  Initializing SD card... ");

    _sd_construct(&sd_device, 0, SD_TRX_FREQUENCY, SD_CRC_ENABLED);

    int result = _sd_init(&sd_device);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (error ");
        uart_print_hex_word(result);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 2: Read boot sector (Volume ID) */
static int test_read_boot_sector(void)
{
    test_puts("  Reading boot sector (sector 0)... ");

    int result = _sd_read(&sd_device, block_buffer, 0, 512);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (error ");
        uart_print_hex_word(result);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Check for boot sector signature (0x55AA at offset 510) */
    if (block_buffer[510] != 0x55 || block_buffer[511] != 0xAA) {
        test_puts("FAIL (invalid boot signature: ");
        uart_print_hex_word(block_buffer[510]);
        test_puts(" ");
        uart_print_hex_word(block_buffer[511]);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 3: Read FAT 1 (multiple blocks) */
static int test_read_fat1(void)
{
    test_puts("  Reading FAT 1 (4 sectors)... ");

    /* Typical FAT starts at sector 1 (after boot sector) */
    /* Read 4 sectors (2048 bytes) into block_buffer and compare_buffer */
    for (int i = 0; i < 4; i++) {
        int result = _sd_read(&sd_device, block_buffer + (i * 512), 1 + i, 512);

        if (result != SD_BLOCK_DEVICE_OK) {
            test_puts("FAIL (error ");
            uart_print_hex_word(result);
            test_puts(" at sector ");
            uart_print_hex_word(1 + i);
            test_puts(")");
            test_print_crlf();
            return TEST_FAIL;
        }
    }

    /* Check for non-zero data (FAT should have some content) */
    int has_data = 0;
    for (int i = 0; i < 2048; i++) {
        if (block_buffer[i] != 0) {
            has_data = 1;
            break;
        }
    }

    if (!has_data) {
        test_puts("FAIL (FAT appears empty)");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 4: Read FAT 2 and compare with FAT 1 */
static int test_read_fat2_compare(void)
{
    test_puts("  Reading FAT 2 and comparing... ");

    /* Save FAT 1 data to compare_buffer */
    memcpy(compare_buffer, block_buffer, 512);

    /* Typical FAT 2 starts right after FAT 1
     * For FAT16, each FAT is typically 64-256 sectors
     * We'll assume FAT size is 64 sectors, so FAT 2 starts at sector 65 */
    int fat_size = 64; /* sectors */
    int fat2_start = 1 + fat_size;

    /* Read first sector of FAT 2 */
    int result = _sd_read(&sd_device, block_buffer, fat2_start, 512);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (error ");
        uart_print_hex_word(result);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Compare FAT 1 and FAT 2 (should be identical) */
    if (memcmp(compare_buffer, block_buffer, 512) != 0) {
        test_puts("FAIL (FAT 1 and FAT 2 differ)");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 5: Single block write - read sector 1 and verify it's zeros */
static int test_write_prep_check_zeros(void)
{
    test_puts("  Checking sector 1 is zeros... ");

    /* Read sector 1 (first sector after boot sector) */
    int result = _sd_read(&sd_device, block_buffer, 1, 512);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (read error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Check all bytes are zero */
    for (int i = 0; i < 512; i++) {
        if (block_buffer[i] != 0) {
            test_puts("FAIL (sector 1 not zeros - UNSAFE TO PROCEED)");
            test_print_crlf();
            return TEST_FAIL;
        }
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 6: Write test pattern to sector 1 */
static int test_write_pattern(void)
{
    test_puts("  Writing test pattern to sector 1... ");

    /* Create test pattern */
    for (int i = 0; i < 512; i++) {
        block_buffer[i] = i & 0xFF;
    }

    int result = _sd_program(&sd_device, block_buffer, 1, 512);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (write error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 7: Read back and verify pattern */
static int test_verify_pattern(void)
{
    test_puts("  Verifying test pattern... ");

    /* Clear buffer first */
    memset(compare_buffer, 0, 512);

    /* Read sector 1 */
    int result = _sd_read(&sd_device, compare_buffer, 1, 512);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (read error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Verify pattern */
    for (int i = 0; i < 512; i++) {
        if (compare_buffer[i] != (i & 0xFF)) {
            test_puts("FAIL (pattern mismatch at byte ");
            uart_print_hex_word(i);
            test_puts(")");
            test_print_crlf();
            return TEST_FAIL;
        }
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 8: Write zeros back to sector 1 */
static int test_restore_zeros(void)
{
    test_puts("  Restoring zeros to sector 1... ");

    /* Fill buffer with zeros */
    memset(block_buffer, 0, 512);

    int result = _sd_program(&sd_device, block_buffer, 1, 512);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (write error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 9: Verify zeros restored */
static int test_verify_zeros(void)
{
    test_puts("  Verifying zeros restored... ");

    /* Read sector 1 */
    int result = _sd_read(&sd_device, block_buffer, 1, 512);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (read error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Check all bytes are zero */
    for (int i = 0; i < 512; i++) {
        if (block_buffer[i] != 0) {
            test_puts("FAIL (byte ");
            uart_print_hex_word(i);
            test_puts(" not zero)");
            test_print_crlf();
            return TEST_FAIL;
        }
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test suite array */
TEST_SUITE(06_sdspi,
           sd_init,
           read_boot_sector,
           read_fat1,
           read_fat2_compare,
           write_prep_check_zeros,
           write_pattern,
           verify_pattern,
           restore_zeros,
           verify_zeros);
