/* Test Suite: Joystick Input
 * Tests MMIO registers for joystick current and latched state
 * Tests all directions (up, down, left, right) and buttons (1, 2) for both joysticks
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "funcval_tb.h"
#include "mmio.h"

/* Helper: Set joystick input for simulation or prompt hardware */
static void set_joystick_input(int joy_num, uint8_t bits, const char *description)
{
    if (platform_is_interactive) {
        /* Hardware: prompt user */
        test_puts("Joy");
        test_print_dec(joy_num);
        test_puts(": ");
        test_puts(description);
        test_puts(" ('X' to skip)\n");
    } else {
        /* Simulation/model: write directly to testbench register */
        volatile uint8_t *joy_reg = (joy_num == 0) ?
            (volatile uint8_t *)FUNCVAL_JOY0 :
            (volatile uint8_t *)FUNCVAL_JOY1;
        *joy_reg = bits;
        usleep(1); /* 1us delay as specified */
    }
}

/* Helper: Clear joystick input for simulation or prompt hardware */
static void clear_joystick_input(int joy_num)
{
    if (platform_is_interactive) {
        /* Hardware: wait for user to release - just continue after prompt */
        test_puts("Centre/release('X' to skip)\n");
    } else {
        /* Simulation/model: write 0 to testbench register */
        volatile uint8_t *joy_reg = (joy_num == 0) ?
            (volatile uint8_t *)FUNCVAL_JOY0 :
            (volatile uint8_t *)FUNCVAL_JOY1;
        *joy_reg = 0;
    }
}

/* Helper: Test a joystick direction/button with all steps */
static int test_joystick_input(int joy_num, uint8_t expected_bits, const char *description)
{
    volatile uint8_t *joy_current = (joy_num == 0) ?
        (volatile uint8_t *)_JOYSTICK0 :
        (volatile uint8_t *)_JOYSTICK1;
    volatile uint8_t *joy_latched = (joy_num == 0) ?
        (volatile uint8_t *)_JOYSTICK0_LATCHED :
        (volatile uint8_t *)_JOYSTICK1_LATCHED;

    /* Step 1: Set input (simulation: write to FUNCVAL_JOY, hardware: prompt user) */
    set_joystick_input(joy_num, expected_bits, description);

    if (platform_is_interactive) {
        /* Hardware: poll with timeout ~10 seconds */
        uint32_t start_time = timer_get_us();
        int detected = 0;

        while (timer_get_us() - start_time < 10000000) {
            char c = read_keyboard_scancode();
            if (c == 0x22) { /* 'X' key */
                return TEST_SKIP;
            }

            uint8_t current = *joy_current;
            if ((current & expected_bits) == expected_bits) {
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

    /* Step 2: Check current state has expected bits */
    uint8_t current = *joy_current;
    if ((current & expected_bits) != expected_bits) {
        test_puts("2. ");
        test_puts((joy_num == 0) ? "JOYSTICK0" : "JOYSTICK1");
        test_puts(": 0x");
        uart_print_hex_word(current);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 3: Check latched state has expected bits */
    uint8_t latched = *joy_latched;
    if ((latched & expected_bits) != expected_bits) {
        test_puts("3. ");
        test_puts((joy_num == 0) ? "JOYSTICK0_LATCHED" : "JOYSTICK1_LATCHED");
        test_puts(": 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 4: Clear input (simulation: write 0, hardware: prompt user) */
    clear_joystick_input(joy_num);

    if (platform_is_interactive) {
        /* Hardware: wait for current to become zero */
        uint32_t start_time = timer_get_us();

        while (timer_get_us() - start_time < 5000000) {
            char c = read_keyboard_scancode();
            if (c == 0x22) { /* 'X' key */
                return TEST_SKIP;
            }

            uint8_t current_check = *joy_current;
            if (current_check == 0) {
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }
        test_puts("\n");
    }

    /* Step 5: Check current state is zero */
    current = *joy_current;
    if (current != 0) {
        test_puts("5. ");
        test_puts((joy_num == 0) ? "JOYSTICK0" : "JOYSTICK1");
        test_puts(": 0x");
        uart_print_hex_word(current);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 6: Check latched state STILL has the bits */
    latched = *joy_latched;
    if ((latched & expected_bits) != expected_bits) {
        test_puts("6. ");
        test_puts((joy_num == 0) ? "JOYSTICK0_LATCHED" : "JOYSTICK1_LATCHED");
        test_puts(": 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 7: Clear latched state by writing 1s to bits we want to clear */
    *joy_latched = expected_bits; /* Write 1s to bits to clear them */

    /* Step 8: Check latched state is cleared */
    latched = *joy_latched;
    if ((latched & expected_bits) != 0) {
        test_puts("7. ");
        test_puts((joy_num == 0) ? "JOYSTICK0_LATCHED" : "JOYSTICK1_LATCHED");
        test_puts(": 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Direction tests for Joystick 0 */
static int test_joy0_up(void)
{
    return test_joystick_input(0, JOY_UP, "UP");
}

static int test_joy0_down(void)
{
    return test_joystick_input(0, JOY_DOWN, "DOWN");
}

static int test_joy0_left(void)
{
    return test_joystick_input(0, JOY_LEFT, "LEFT");
}

static int test_joy0_right(void)
{
    return test_joystick_input(0, JOY_RIGHT, "RIGHT");
}

/* Button tests for Joystick 0 */
static int test_joy0_btn1(void)
{
    return test_joystick_input(0, JOY_BTN1, "BUTTON 1");
}

static int test_joy0_btn2(void)
{
    return test_joystick_input(0, JOY_BTN2, "BUTTON 2");
}

/* Direction tests for Joystick 1 */
static int test_joy1_up(void)
{
    return test_joystick_input(1, JOY_UP, "UP");
}

static int test_joy1_down(void)
{
    return test_joystick_input(1, JOY_DOWN, "DOWN");
}

static int test_joy1_left(void)
{
    return test_joystick_input(1, JOY_LEFT, "LEFT");
}

static int test_joy1_right(void)
{
    return test_joystick_input(1, JOY_RIGHT, "RIGHT");
}

/* Button tests for Joystick 1 */
static int test_joy1_btn1(void)
{
    return test_joystick_input(1, JOY_BTN1, "BUTTON 1");
}

static int test_joy1_btn2(void)
{
    return test_joystick_input(1, JOY_BTN2, "BUTTON 2");
}

/* Multi-button tests: Press both buttons simultaneously and verify independent clearing */
static int test_multi_button_joy0(void)
{
    volatile uint8_t *joy_current = (volatile uint8_t *)_JOYSTICK0;
    volatile uint8_t *joy_latched = (volatile uint8_t *)_JOYSTICK0_LATCHED;
    uint8_t both_buttons = JOY_BTN1 | JOY_BTN2;

    /* Step 1: Set both buttons */
    set_joystick_input(0, both_buttons, "BOTH BUTTONS");

    if (platform_is_interactive) {
        uint32_t start_time = timer_get_us();
        int detected = 0;

        while (timer_get_us() - start_time < 10000000) {
            char c = read_keyboard_scancode();
            if (c == 0x22) { /* 'X' key */
                return TEST_SKIP;
            }
            uint8_t current = *joy_current;
            if ((current & both_buttons) == both_buttons) {
                detected = 1;
                break;
            }
            usleep(10000);
            test_puts(".");
        }
        if (!detected)
            return TEST_TIMEOUT;
        test_puts("\n");
    }

    /* Step 2: Check current has both buttons */
    uint8_t current = *joy_current;
    if ((current & both_buttons) != both_buttons) {
        test_puts("2. JOYSTICK0: 0x");
        test_print_hex_word(current);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 3: Check latched has both buttons */
    uint8_t latched = *joy_latched;
    if ((latched & both_buttons) != both_buttons) {
        test_puts("3. JOYSTICK0_LATCHED: 0x");
        test_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 4: Clear input */
    clear_joystick_input(0);
    test_puts("\n");

    if (!platform_is_interactive) {
        uint32_t start_time = timer_get_us();
        test_puts("    Waiting");
        while (timer_get_us() - start_time < 5000000) {
            char c = read_keyboard_scancode();
            if (c == 0x22) { /* 'X' key */
                test_puts(" SKIPPED\n");
                return TEST_SKIP;
            }
            uint8_t current_check = *joy_current;
            if (current_check == 0) {
                break;
            }
            usleep(10000);
            test_puts(".");
        }
        test_puts(" cleared\n");
    }

    /* Step 5: Check current is cleared */
    current = *joy_current;
    if (current != 0) {
        test_puts("5. JOYSTICK0: 0x");
        test_print_hex_word(current);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 6: Check latched still has both */
    latched = *joy_latched;
    if ((latched & both_buttons) != both_buttons) {
        test_puts("6. JOYSTICK0_LATCHED: 0x");
        test_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 7: Clear only BTN1 in latched */
    *joy_latched = JOY_BTN1;

    /* Step 8: Check BTN1 is cleared but BTN2 remains */
    latched = *joy_latched;
    if ((latched & JOY_BTN1) != 0) {
        test_puts("8. JOYSTICK0_LATCHED: 0x");
        test_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }
    if ((latched & JOY_BTN2) == 0) {
        test_puts("8. JOYSTICK0_LATCHED: 0x");
        test_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 9: Clear BTN2 separately */
    *joy_latched = JOY_BTN2;

    /* Step 10: Check both are now cleared */
    latched = *joy_latched;
    if ((latched & both_buttons) != 0) {
        test_puts("10. JOYSTICK0_LATCHED: 0x");
        test_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test suite array */
TEST_SUITE(03_joystick,
           joy0_up,
           joy0_down,
           joy0_left,
           joy0_right,
           joy0_btn1,
           joy0_btn2,
           joy1_up,
           joy1_down,
           joy1_left,
           joy1_right,
           joy1_btn1,
           joy1_btn2,
           multi_button_joy0);
