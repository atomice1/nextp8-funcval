/* Test Suite: Keyboard Input
 * Tests MMIO registers for keyboard matrix and latched state
 * Tests representative keys: letters, special keys, and extended keys (arrows)
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "funcval_tb.h"
#include "mmio.h"

/* PS/2 Scancode definitions (Set 2) */
#define PS2_SCANCODE_A          0x1C
#define PS2_SCANCODE_S          0x1B
#define PS2_SCANCODE_SPACE      0x29
#define PS2_SCANCODE_ENTER      0x5A
#define PS2_SCANCODE_ESC        0x76
#define PS2_SCANCODE_LEFT       0x6B    /* Requires 0xE0 prefix */
#define PS2_SCANCODE_RIGHT      0x74    /* Requires 0xE0 prefix */

#define PS2_EXTENDED_PREFIX     0xE0
#define PS2_BREAK_PREFIX        0xF0

/* Matrix bit indices (extended keys are OR'ed with 0x80) */
#define MATRIX_BIT_A            0x1C
#define MATRIX_BIT_S            0x1B
#define MATRIX_BIT_SPACE        0x29
#define MATRIX_BIT_ENTER        0x5A
#define MATRIX_BIT_ESC          0x76
#define MATRIX_BIT_LEFT         (0x6B | 0x80)  /* 0xEB */
#define MATRIX_BIT_RIGHT        (0x74 | 0x80)  /* 0xF4 */

/* Helper: Check if a bit is set in the 256-bit keyboard matrix */
static int is_matrix_bit_set(volatile uint8_t *matrix_base, uint16_t bit_index)
{
    uint16_t byte_index = bit_index >> 3;  /* Divide by 8 */
    uint8_t bit_mask = 1 << (bit_index & 0x7);  /* Modulo 8 */
    return (matrix_base[byte_index] & bit_mask) != 0;
}

/* Helper: Send PS/2 scancode (make code) for simulation or prompt user */
static void send_key_press(uint8_t scancode, int is_extended, const char *key_name)
{
    if (platform_is_interactive) {
        /* Hardware: prompt user to press key */
        test_puts("Press and hold '");
        test_puts(key_name);
        test_puts("' key\n");
        screen_flip();
    } else {
        /* Simulation: write scancode(s) to testbench register */
        volatile uint8_t *scancode_reg = (volatile uint8_t *)FUNCVAL_KB_SCANCODE;

        if (is_extended) {
            *scancode_reg = PS2_EXTENDED_PREFIX;
            usleep(50);
        }
        *scancode_reg = scancode;
        usleep(4000);
    }
}

/* Helper: Send PS/2 break code for simulation or prompt user */
static void send_key_release(uint8_t scancode, int is_extended, const char *key_name)
{
    if (platform_is_interactive) {
        /* Hardware: prompt user to release key */
        test_puts("Release '");
        test_puts(key_name);
        test_puts("' key\n");
        screen_flip();
    } else {
        /* Simulation: write break code to testbench register */
        volatile uint8_t *scancode_reg = (volatile uint8_t *)FUNCVAL_KB_SCANCODE;

        if (is_extended) {
            *scancode_reg = PS2_EXTENDED_PREFIX;
            usleep(50);
        }
        *scancode_reg = PS2_BREAK_PREFIX;
        usleep(50);
        *scancode_reg = scancode;
        usleep(6000);
    }
}

/* Helper: Test a key with all verification steps */
static int test_key_input(uint8_t scancode, int is_extended, uint16_t matrix_bit, const char *key_name)
{
    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX;
    volatile uint8_t *matrix_latched = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;

    /* Step 1: Send key press (simulation: write scancode, hardware: prompt user) */
    send_key_press(scancode, is_extended, key_name);

    if (platform_is_interactive) {
        /* Hardware: poll with timeout ~10 seconds */
        uint64_t start_time = timer_get_us();
        int detected = 0;

        while (timer_get_us() - start_time < 10000000) {
            /* Detect via latched matrix so latch is preserved for step 3 */
            if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
                detected = 1;
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }

        if (!detected)
            return TEST_TIMEOUT;

        test_puts("\n");
    }

    /* Step 2: Check current matrix has bit set */
    if (!is_matrix_bit_set(matrix, matrix_bit)) {
        test_puts("2. Matrix bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" not set\n");
        return TEST_FAIL;
    }

    /* Step 3: Check latched matrix has bit set */
    if (!is_matrix_bit_set(matrix_latched, matrix_bit)) {
        test_puts("3. Latched bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" not set\n");
        return TEST_FAIL;
    }

    /* Step 4: Send key release (simulation: write break code, hardware: prompt user) */
    send_key_release(scancode, is_extended, key_name);

    if (platform_is_interactive) {
        /* Hardware: wait for current to be cleared */
        uint64_t start_time = timer_get_us();

        while (timer_get_us() - start_time < 5000000) {
            if (!is_matrix_bit_set(matrix, matrix_bit)) {
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }
        test_puts("\n");
    }

    /* Step 5: Check current matrix has bit cleared */
    if (is_matrix_bit_set(matrix, matrix_bit)) {
        test_puts("5. Matrix bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" not cleared\n");
        return TEST_FAIL;
    }

    /* Step 6: Check latched matrix STILL has the bit set */
    if (!is_matrix_bit_set(matrix_latched, matrix_bit)) {
        test_puts("6. Latched bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" cleared prematurely\n");
        return TEST_FAIL;
    }

    /* Step 7: Clear latched state (write 1 to clear) */
    uint16_t byte_index = matrix_bit >> 3;
    uint8_t bit_mask = 1 << (matrix_bit & 0x7);
    volatile uint8_t *latched_write = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;
    latched_write[byte_index] = bit_mask;

    /* Step 8: Verify latched state cleared */
    if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
        test_puts("8. Latched bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" not cleared\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test: A key */
static int test_key_a(void)
{
    if (platform_is_simulation)
        return TEST_SKIP;
    return test_key_input(PS2_SCANCODE_A, 0, MATRIX_BIT_A, "A");
}

/* Test: S key */
static int test_key_s(void)
{
    return test_key_input(PS2_SCANCODE_S, 0, MATRIX_BIT_S, "S");
}

/* Test: Space key */
static int test_key_space(void)
{
    if (platform_is_simulation)
        return TEST_SKIP;
    return test_key_input(PS2_SCANCODE_SPACE, 0, MATRIX_BIT_SPACE, "SPACE");
}

/* Test: Enter key */
static int test_key_enter(void)
{
    if (platform_is_simulation)
        return TEST_SKIP;
    return test_key_input(PS2_SCANCODE_ENTER, 0, MATRIX_BIT_ENTER, "ENTER");
}

/* Test: Escape key */
static int test_key_esc(void)
{
    if (platform_is_simulation)
        return TEST_SKIP;
    return test_key_input(PS2_SCANCODE_ESC, 0, MATRIX_BIT_ESC, "ESC");
}

/* Test: Left Arrow key (extended) */
static int test_key_left(void)
{
    if (platform_is_simulation)
        return TEST_SKIP;
    return test_key_input(PS2_SCANCODE_LEFT, 1, MATRIX_BIT_LEFT, "LEFT ARROW");
}

/* Test: Right Arrow key (extended) */
static int test_key_right(void)
{
    if (platform_is_simulation)
        return TEST_SKIP;
    return test_key_input(PS2_SCANCODE_RIGHT, 1, MATRIX_BIT_RIGHT, "RIGHT ARROW");
}

/* Test: Multi-key latching - verify each key can be cleared independently */
static int test_multi_key_latching(void)
{
    if (platform_is_simulation)
        return TEST_SKIP;

    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX;
    volatile uint8_t *matrix_latched = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;
    volatile uint8_t *latched_write = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;

    /* Clear any previous latched state for test keys */
    uint16_t byte_a = MATRIX_BIT_A >> 3;
    uint8_t mask_a = 1 << (MATRIX_BIT_A & 0x7);
    latched_write[byte_a] = mask_a;

    uint16_t byte_s = MATRIX_BIT_S >> 3;
    uint8_t mask_s = 1 << (MATRIX_BIT_S & 0x7);
    latched_write[byte_s] = mask_s;

    /* Press A, wait for latch, release A, wait for release */
    send_key_press(PS2_SCANCODE_A, 0, "A");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_A) &&
               timer_get_us() - t < 10000000)
            usleep(10000);
        if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_A))
            return TEST_TIMEOUT;
    }
    send_key_release(PS2_SCANCODE_A, 0, "A");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (is_matrix_bit_set(matrix, MATRIX_BIT_A) &&
               timer_get_us() - t < 5000000)
            usleep(10000);
    }

    /* Press S, wait for latch, release S, wait for release */
    send_key_press(PS2_SCANCODE_S, 0, "S");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_S) &&
               timer_get_us() - t < 10000000)
            usleep(10000);
        if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_S))
            return TEST_TIMEOUT;
    }
    send_key_release(PS2_SCANCODE_S, 0, "S");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (is_matrix_bit_set(matrix, MATRIX_BIT_S) &&
               timer_get_us() - t < 5000000)
            usleep(10000);
    }

    /* Verify both are latched */
    if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_A) ||
        !is_matrix_bit_set(matrix_latched, MATRIX_BIT_S)) {
        test_puts("Both keys not latched: A=");
        uart_print_hex_word(is_matrix_bit_set(matrix_latched, MATRIX_BIT_A));
        test_puts(" S=");
        uart_print_hex_word(is_matrix_bit_set(matrix_latched, MATRIX_BIT_S));
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Clear only A key */
    latched_write[byte_a] = mask_a;
    usleep(100);

    if (is_matrix_bit_set(matrix_latched, MATRIX_BIT_A)) {
        test_puts("A key not cleared\n");
        return TEST_FAIL;
    }

    if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_S)) {
        test_puts("S key cleared unexpectedly\n");
        return TEST_FAIL;
    }

    /* Clear S key */
    latched_write[byte_s] = mask_s;
    usleep(100);

    if (is_matrix_bit_set(matrix_latched, MATRIX_BIT_A) ||
        is_matrix_bit_set(matrix_latched, MATRIX_BIT_S)) {
        test_puts("Keys not both cleared\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Wait for PS/2 controller init before sending packets.
 */
void software_init_hook(void)
{
    platform_detect();
    if (platform_is_simulation) {
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_KB) = 1;
        usleep(200000); /* 200ms init time */
    }
}

/* Define test suite */
TEST_SUITE(05_keyboard,
    key_a,
    key_s,
    key_space,
    key_enter,
    key_esc,
    key_left,
    key_right,
    multi_key_latching
);
