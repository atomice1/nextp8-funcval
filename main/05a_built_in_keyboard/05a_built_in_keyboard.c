/* Test Suite: Built-in Keyboard Matrix
 * Tests MMIO registers for Spectrum Next built-in keyboard matrix
 * Tests representative keys via matrix register writes (0x380080-0x380087)
 * Duplicates 05_keyboard tests but for the built-in keyboard hardware interface
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "funcval_tb.h"
#include "mmio.h"

/* Key mappings: row, bit position, and register value
 * Format: (row_index, bit_mask, row_register, key_description)
 */
#define KEY_A_ROW       1
#define KEY_A_BIT       0x01
#define KEY_A_REG       (FUNCVAL_KB_MATRIX_BASE + KEY_A_ROW)

#define KEY_S_ROW       1
#define KEY_S_BIT       0x02
#define KEY_S_REG       (FUNCVAL_KB_MATRIX_BASE + KEY_S_ROW)

#define KEY_SPACE_ROW   7
#define KEY_SPACE_BIT   0x01
#define KEY_SPACE_REG   (FUNCVAL_KB_MATRIX_BASE + KEY_SPACE_ROW)

#define KEY_ENTER_ROW   6
#define KEY_ENTER_BIT   0x01
#define KEY_ENTER_REG   (FUNCVAL_KB_MATRIX_BASE + KEY_ENTER_ROW)

#define KEY_ESC_ROW     3
#define KEY_ESC_BIT     0x20  /* Bit 5 */
#define KEY_ESC_REG     (FUNCVAL_KB_MATRIX_BASE + KEY_ESC_ROW)

#define KEY_LEFT_ROW    7
#define KEY_LEFT_BIT    0x20  /* Bit 5 (extended Left arrow) */
#define KEY_LEFT_REG    (FUNCVAL_KB_MATRIX_BASE + KEY_LEFT_ROW)

#define KEY_RIGHT_ROW   6
#define KEY_RIGHT_BIT   0x40  /* Bit 6 (extended Right arrow) */
#define KEY_RIGHT_REG   (FUNCVAL_KB_MATRIX_BASE + KEY_RIGHT_ROW)

/* PS/2 matrix bit format for verification (these are also stored in hardware matrix) */
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

/* Helper: Write to built-in keyboard matrix register */
static void set_keyboard_matrix_key(uint32_t reg, uint8_t bit_mask, int press, const char *key_name)
{
    volatile uint8_t *kb_reg = (volatile uint8_t *)reg;

    if (platform_is_interactive) {
        /* Hardware: prompt user to press/release key */
        if (press) {
            test_puts("Press built-in keyboard key: '");
            test_puts(key_name);
            test_puts("'\n");
            screen_flip();
        } else {
            test_puts("Release built-in keyboard key: '");
            test_puts(key_name);
            test_puts("'\n");
            screen_flip();
        }
    } else {
        /* Simulation: write directly to keyboard matrix register.
         * Spectrum Next keyboard columns are active-low: clear the bit to press,
         * set all bits (0xFF) to release. bit_mask identifies the column bit. */
        if (press) {
            *kb_reg = ~bit_mask;  /* Clear column bit (active-low = pressed) */
        } else {
            *kb_reg = 0xFF;  /* All columns high = all released */
        }
        usleep(1500);  /* 1.5ms: covers 2 full keyboard scan cycles (clk_sys=11MHz, 8192 cycles=748us each) */
    }
}

/* Helper: Test a key via built-in keyboard matrix interface */
static int test_builtin_key_input(uint32_t reg, uint8_t bit_mask, uint16_t matrix_bit, const char *key_name)
{
    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX;
    volatile uint8_t *matrix_latched = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;

    /* Step 1: Press key via built-in keyboard matrix register */
    set_keyboard_matrix_key(reg, bit_mask, 1, key_name);

    if (platform_is_interactive) {
        /* Hardware: poll with timeout ~10 seconds */
        uint64_t start_time = timer_get_us();

        while (timer_get_us() - start_time < 10000000) {
            if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
                test_puts("Key detected\n");
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }
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

    /* Step 4: Release key via built-in keyboard matrix register */
    set_keyboard_matrix_key(reg, bit_mask, 0, key_name);

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
    uint8_t bit_mask_latch = 1 << (matrix_bit & 0x7);
    volatile uint8_t *latched_write = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;
    latched_write[byte_index] = bit_mask_latch;

    /* Step 8: Verify latched state cleared */
    if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
        test_puts("8. Latched bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" not cleared\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test: A key (built-in keyboard) */
static int test_builtin_key_a(void)
{
    return test_builtin_key_input(KEY_A_REG, KEY_A_BIT, MATRIX_BIT_A, "A (built-in)");
}

/* Test: S key (built-in keyboard) */
static int test_builtin_key_s(void)
{
    return test_builtin_key_input(KEY_S_REG, KEY_S_BIT, MATRIX_BIT_S, "S (built-in)");
}

/* Test: Space key (built-in keyboard) */
static int test_builtin_key_space(void)
{
    return test_builtin_key_input(KEY_SPACE_REG, KEY_SPACE_BIT, MATRIX_BIT_SPACE, "SPACE (built-in)");
}

/* Test: Enter key (built-in keyboard) */
static int test_builtin_key_enter(void)
{
    return test_builtin_key_input(KEY_ENTER_REG, KEY_ENTER_BIT, MATRIX_BIT_ENTER, "ENTER (built-in)");
}

/* Test: ESC key (built-in keyboard) */
static int test_builtin_key_esc(void)
{
    return test_builtin_key_input(KEY_ESC_REG, KEY_ESC_BIT, MATRIX_BIT_ESC, "ESC (built-in)");
}

/* Test: Left arrow key (built-in keyboard) */
static int test_builtin_key_left(void)
{
    return test_builtin_key_input(KEY_LEFT_REG, KEY_LEFT_BIT, MATRIX_BIT_LEFT, "LEFT ARROW (built-in)");
}

/* Test: Right arrow key (built-in keyboard) */
static int test_builtin_key_right(void)
{
    return test_builtin_key_input(KEY_RIGHT_REG, KEY_RIGHT_BIT, MATRIX_BIT_RIGHT, "RIGHT ARROW (built-in)");
}

/* Test: Multi-key latching via built-in keyboard matrix */
static int test_builtin_multi_key_latching(void)
{
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

    if (platform_is_interactive) {
        test_puts("This test will press both A and S keys via built-in keyboard\n");
        screen_flip();
    }

    /* Press A, wait for latch, release A, wait for release */
    set_keyboard_matrix_key(KEY_A_REG, KEY_A_BIT, 1, "A (built-in)");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_A) &&
               timer_get_us() - t < 10000000)
            usleep(10000);
        if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_A))
            return TEST_TIMEOUT;
    }
    set_keyboard_matrix_key(KEY_A_REG, KEY_A_BIT, 0, "A (built-in)");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (is_matrix_bit_set(matrix, MATRIX_BIT_A) &&
               timer_get_us() - t < 5000000)
            usleep(10000);
    }

    /* Press S, wait for latch, release S, wait for release */
    set_keyboard_matrix_key(KEY_S_REG, KEY_S_BIT, 1, "S (built-in)");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_S) &&
               timer_get_us() - t < 10000000)
            usleep(10000);
        if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_S))
            return TEST_TIMEOUT;
    }
    set_keyboard_matrix_key(KEY_S_REG, KEY_S_BIT, 0, "S (built-in)");
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

void software_init_hook(void)
{
    platform_detect();
    if (platform_is_simulation)
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_KB) = 1;
}

/* Define test suite */
TEST_SUITE(05a_keyboard_builtin,
    builtin_key_a,
    builtin_key_s,
    builtin_key_space,
    builtin_key_enter,
    builtin_key_esc,
    builtin_key_left,
    builtin_key_right,
    builtin_multi_key_latching
);
