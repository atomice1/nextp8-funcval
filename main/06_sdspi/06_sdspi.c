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
#include "funcval_tb.h"

#define BLOCK_SIZE 512

/* Global SD block device instance */
static struct _sd_block_device sd_device;

/* Buffer for block operations */
#define FAT_READ_SECTORS 4
static uint8_t block_buffer[FAT_READ_SECTORS * BLOCK_SIZE]; /* large enough for multi-sector FAT reads */
static uint8_t compare_buffer[BLOCK_SIZE];

/* BPB-derived layout (populated after reading boot sector in test 2) */
static uint32_t bpb_fat1_sector;  /* First sector of FAT 1 */
static uint32_t bpb_fat2_sector;  /* First sector of FAT 2 */
static uint32_t bpb_data_sector;  /* First sector of data area */

/* Test 1: Initialize SD card */
static int test_sd_init(void)
{
    test_puts("  Initializing SD card... ");

    _sd_construct(&sd_device, 0, SD_TRX_FREQUENCY, SD_CRC_ENABLED);

    int result = _sd_init(&sd_device);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (error ");
        test_print_hex_word(result);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

#include "funcval_tb.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

/* Test 2: Read boot sector (Volume ID) */
static int test_read_boot_sector(void)
{
    test_puts("  Reading boot sector (sector 0)... ");

    int result = _sd_read(&sd_device, block_buffer, 0, 512);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (error ");
        test_print_hex_word(result);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    if (block_buffer[510] != 0x55 || block_buffer[511] != 0xAA) {
        test_puts("FAIL (invalid boot signature: ");
        test_print_hex_byte(block_buffer[510]);
        test_puts(" ");
        test_print_hex_byte(block_buffer[511]);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Parse BPB fields to derive FAT and data area locations */
    uint16_t reserved_sectors = (uint16_t)block_buffer[0x0E] |
                                 ((uint16_t)block_buffer[0x0F] << 8);
    uint8_t  num_fats         = block_buffer[0x10];
    uint16_t root_entry_count = (uint16_t)block_buffer[0x11] |
                                 ((uint16_t)block_buffer[0x12] << 8);
    uint16_t fat_size         = (uint16_t)block_buffer[0x16] |
                                 ((uint16_t)block_buffer[0x17] << 8);
    uint32_t root_dir_sectors = ((uint32_t)root_entry_count * 32 + BLOCK_SIZE - 1)
                                 / BLOCK_SIZE;

    bpb_fat1_sector = reserved_sectors;
    bpb_fat2_sector = reserved_sectors + fat_size;
    bpb_data_sector = reserved_sectors + (uint32_t)num_fats * fat_size
                      + root_dir_sectors;

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

#pragma GCC pop_options

/* Test 3: Read FAT 1 (multiple blocks) */
static int test_read_fat1(void)
{
    test_puts("  Reading FAT 1 (4 sectors)... ");

    /* Read 4 sectors of FAT 1 into block_buffer */
    for (int i = 0; i < 4; i++) {
        int result = _sd_read(&sd_device, block_buffer + (i * BLOCK_SIZE),
                              (bpb_fat1_sector + i) * BLOCK_SIZE, BLOCK_SIZE);

        if (result != SD_BLOCK_DEVICE_OK) {
            test_puts("FAIL (error ");
            test_print_hex_word(result);
            test_puts(" at sector ");
            test_print_hex_word(1 + i);
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

    /* Save first sector of FAT 1 to compare_buffer */
    memcpy(compare_buffer, block_buffer, BLOCK_SIZE);

    /* Read first sector of FAT 2 */
    int result = _sd_read(&sd_device, block_buffer, bpb_fat2_sector * BLOCK_SIZE, BLOCK_SIZE);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (error ");
        test_print_hex_word(result);
        test_puts(")");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Compare first sector of FAT 1 and FAT 2 (should be identical) */
    if (memcmp(compare_buffer, block_buffer, BLOCK_SIZE) != 0) {
        test_puts("FAIL (FAT 1 and FAT 2 differ)");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Flag: set once we have confirmed the test sector is zero and are about to
 * write the pattern.  Used by suite cleanup to know whether to restore zeros. */
static int write_attempted = 0;

/* Test 5: Check test sector is zeros, write pattern, read back and verify */
static int test_write_verify_pattern(void)
{
    test_puts("  Checking test sector is zeros... ");

    /* Read a sector well into the data area (10 sectors past data start) */
    int result = _sd_read(&sd_device, block_buffer,
                          (bpb_data_sector + 10) * BLOCK_SIZE, BLOCK_SIZE);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (read error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    /* Check all bytes are zero - do not proceed with write if not clean */
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (block_buffer[i] != 0) {
            test_puts("FAIL (test sector not zeros - UNSAFE TO PROCEED)");
            test_print_crlf();
            return TEST_FAIL;
        }
    }

    /* Sector confirmed clean - write the pattern */
    write_attempted = 1;

    test_puts("OK, writing test pattern... ");

    for (int i = 0; i < BLOCK_SIZE; i++) {
        block_buffer[i] = i & 0xFF;
    }

    result = _sd_program(&sd_device, block_buffer,
                         (bpb_data_sector + 10) * BLOCK_SIZE, BLOCK_SIZE);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (write error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("OK, verifying... ");

    /* Clear buffer before read-back */
    memset(compare_buffer, 0, BLOCK_SIZE);

    result = _sd_read(&sd_device, compare_buffer,
                      (bpb_data_sector + 10) * BLOCK_SIZE, BLOCK_SIZE);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (read error)");
        test_print_crlf();
        return TEST_FAIL;
    }

    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (compare_buffer[i] != (i & 0xFF)) {
            test_puts("FAIL (pattern mismatch at byte ");
            test_print_hex_word(i);
            test_puts(")");
            test_print_crlf();
            return TEST_FAIL;
        }
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Suite cleanup: restore zeros if we wrote the pattern */
static void suite_cleanup(void)
{
    if (!write_attempted)
        return;

    test_puts("  Cleanup: restoring zeros to test sector... ");

    memset(block_buffer, 0, BLOCK_SIZE);

    int result = _sd_program(&sd_device, block_buffer,
                             (bpb_data_sector + 10) * BLOCK_SIZE, BLOCK_SIZE);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (write error)");
        test_print_crlf();
        return;
    }

    result = _sd_read(&sd_device, block_buffer,
                      (bpb_data_sector + 10) * BLOCK_SIZE, BLOCK_SIZE);

    if (result != SD_BLOCK_DEVICE_OK) {
        test_puts("FAIL (read error)");
        test_print_crlf();
        return;
    }

    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (block_buffer[i] != 0) {
            test_puts("FAIL (byte ");
            test_print_hex_word(i);
            test_puts(" not zero)");
            test_print_crlf();
            return;
        }
    }

    test_puts("PASS");
    test_print_crlf();
}

void software_init_hook(void)
{
    platform_detect();
    if (platform_is_simulation)
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_SDSPI) = 1;
}

/* Test suite */
TEST_SUITE_SETUP_CLEANUP(06_sdspi, NULL, suite_cleanup,
                         sd_init,
                         read_boot_sector,
                         read_fat1,
                         read_fat2_compare,
                         write_verify_pattern);
