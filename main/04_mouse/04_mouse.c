/* Test Suite: Mouse Input
 * Tests MMIO registers for mouse movement and button state
 * Tests mouse movements (up, down, left, right), scroll (up, down), and button latching
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "funcval_tb.h"
#include "mmio.h"

/* Mouse button bit definitions (PS/2 standard) */
#define MOUSE_BTN_LEFT      0x01
#define MOUSE_BTN_RIGHT     0x02
#define MOUSE_BTN_MIDDLE    0x04
#define MOUSE_BTN_X1        0x08
#define MOUSE_BTN_X2        0x10

/* Helper: Write buttons/X/Y/Z in the required order so Z triggers the update */
static inline void tb_mouse_send(uint16_t buttons, int16_t x, int16_t y, int16_t z)
{
    *(volatile uint16_t *)FUNCVAL_MOUSE_BUTTONS = buttons;
    *(volatile uint16_t *)FUNCVAL_MOUSE_X       = (uint16_t)x;
    *(volatile uint16_t *)FUNCVAL_MOUSE_Y       = (uint16_t)y;
    *(volatile uint16_t *)FUNCVAL_MOUSE_Z       = (uint16_t)z; /* triggers update */
}

/* Helper: Test mouse movement with all verification steps */
static int test_mouse_movement_axis(int16_t x_delta, int16_t y_delta, int16_t z_delta,
                                     const char *description)
{
    volatile uint16_t *mouse_x = (volatile uint16_t *)_MOUSE_X;
    volatile uint16_t *mouse_y = (volatile uint16_t *)_MOUSE_Y;
    volatile uint16_t *mouse_z = (volatile uint16_t *)_MOUSE_Z;

    /* Step 1: Read initial values */
    int16_t initial_x = (int16_t)*mouse_x;
    int16_t initial_y = (int16_t)*mouse_y;
    int16_t initial_z = (int16_t)*mouse_z;

    /* Step 2: Trigger movement */
    if (platform_is_interactive) {
        test_puts("Mouse: ");
        test_puts(description);
        test_puts(" ('X' to skip)\n");

        /* Hardware: poll with timeout ~10 seconds */
        uint32_t start_time = timer_get_us();
        int detected = 0;

        while (timer_get_us() - start_time < 10000000) {
            char c = read_keyboard_scancode();
            if (c == 0x22) { /* 'X' key */
                return TEST_SKIP;
            }

            /* Check if movement detected on the expected axis */
            int16_t current_x = (int16_t)*mouse_x;
            int16_t current_y = (int16_t)*mouse_y;
            int16_t current_z = (int16_t)*mouse_z;

            int16_t delta_x = current_x - initial_x;
            int16_t delta_y = current_y - initial_y;
            int16_t delta_z = current_z - initial_z;

            /* Check for expected movement direction */
            if ((x_delta > 0 && delta_x > 0) || (x_delta < 0 && delta_x < 0) ||
                (y_delta > 0 && delta_y > 0) || (y_delta < 0 && delta_y < 0) ||
                (z_delta > 0 && delta_z > 0) || (z_delta < 0 && delta_z < 0)) {
                detected = 1;
                break;
            }

            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }

        if (!detected)
            return TEST_TIMEOUT;

        test_puts("\n");
    } else {
        tb_mouse_send(0, x_delta, y_delta, z_delta);
        usleep(10); /* 10us delay for movement to propagate */
    }

    /* Step 3: Verify movement occurred in expected direction */
    int16_t final_x = (int16_t)*mouse_x;
    int16_t final_y = (int16_t)*mouse_y;
    int16_t final_z = (int16_t)*mouse_z;

    int16_t actual_delta_x = final_x - initial_x;
    int16_t actual_delta_y = final_y - initial_y;
    int16_t actual_delta_z = final_z - initial_z;

    /* Check movement in expected direction */
    if (x_delta > 0 && actual_delta_x <= 0) {
        test_puts("3. X movement wrong direction: ");
        uart_print_hex_word((uint16_t)actual_delta_x);
        test_puts("\n");
        return TEST_FAIL;
    }
    if (x_delta < 0 && actual_delta_x >= 0) {
        test_puts("3. X movement wrong direction: ");
        uart_print_hex_word((uint16_t)actual_delta_x);
        test_puts("\n");
        return TEST_FAIL;
    }
    if (y_delta > 0 && actual_delta_y <= 0) {
        test_puts("3. Y movement wrong direction: ");
        uart_print_hex_word((uint16_t)actual_delta_y);
        test_puts("\n");
        return TEST_FAIL;
    }
    if (y_delta < 0 && actual_delta_y >= 0) {
        test_puts("3. Y movement wrong direction: ");
        uart_print_hex_word((uint16_t)actual_delta_y);
        test_puts("\n");
        return TEST_FAIL;
    }
    if (z_delta > 0 && actual_delta_z <= 0) {
        test_puts("3. Z movement wrong direction: ");
        uart_print_hex_word((uint16_t)actual_delta_z);
        test_puts("\n");
        return TEST_FAIL;
    }
    if (z_delta < 0 && actual_delta_z >= 0) {
        test_puts("3. Z movement wrong direction: ");
        uart_print_hex_word((uint16_t)actual_delta_z);
        test_puts("\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Helper: Test mouse button with all verification steps */
static int test_mouse_button_input(uint8_t button_bits, const char *description)
{
    volatile uint8_t *mouse_buttons = (volatile uint8_t *)_MOUSE_BUTTONS;
    volatile uint8_t *mouse_buttons_latched = (volatile uint8_t *)_MOUSE_BUTTONS_LATCHED;

    /* Step 1: Set button */
    if (platform_is_interactive) {
        test_puts("Mouse: ");
        test_puts(description);
        test_puts(" ('X' to skip)\n");
        screen_flip();

        /* Hardware: poll with timeout ~10 seconds */
        uint32_t start_time = timer_get_us();
        int detected = 0;

        while (timer_get_us() - start_time < 10000000) {
            char c = read_keyboard_scancode();
            if (c == 0x22) { /* 'X' key */
                return TEST_SKIP;
            }

            uint8_t current = *mouse_buttons;
            if ((current & button_bits) == button_bits) {
                detected = 1;
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }

        if (!detected)
            return TEST_TIMEOUT;

        test_puts("\n");
    } else {
        tb_mouse_send(button_bits, 0, 0, 0);
        usleep(1);
    }

    /* Step 2: Check current state has expected bits */
    uint8_t current = *mouse_buttons;
    if ((current & button_bits) != button_bits) {
        test_puts("2. MOUSE_BUTTONS: 0x");
        uart_print_hex_word(current);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 3: Check latched state has expected bits */
    uint8_t latched = *mouse_buttons_latched;
    if ((latched & button_bits) != button_bits) {
        test_puts("3. MOUSE_BUTTONS_LATCHED: 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 4: Clear button */
    if (platform_is_interactive) {
        test_puts("Release buttons ('X' to skip)\n");
        screen_flip();

        /* Hardware: wait for current to become zero */
        uint32_t start_time = timer_get_us();

        while (timer_get_us() - start_time < 5000000) {
            char c = read_keyboard_scancode();
            if (c == 0x22) { /* 'X' key */
                return TEST_SKIP;
            }

            uint8_t current_check = *mouse_buttons;
            if (current_check == 0) {
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }
        test_puts("\n");
    } else {
        tb_mouse_send(0, 0, 0, 0);
        usleep(1);
    }

    /* Step 5: Check current state is zero */
    current = *mouse_buttons;
    if (current != 0) {
        test_puts("5. MOUSE_BUTTONS: 0x");
        uart_print_hex_word(current);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 6: Check latched state STILL has the bits */
    latched = *mouse_buttons_latched;
    if ((latched & button_bits) != button_bits) {
        test_puts("6. MOUSE_BUTTONS_LATCHED: 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Step 7: Clear latched state (write 1s to clear) */
    volatile uint8_t *mouse_buttons_latched_write = (volatile uint8_t *)_MOUSE_BUTTONS_LATCHED;
    *mouse_buttons_latched_write = button_bits;

    /* Step 8: Verify latched state cleared */
    latched = *mouse_buttons_latched;
    if ((latched & button_bits) != 0) {
        test_puts("8. MOUSE_BUTTONS_LATCHED: 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test: Mouse movement right (positive X) */
static int test_move_right(void)
{
    return test_mouse_movement_axis(50, 0, 0, "move right");
}

/* Test: Mouse movement left (negative X) */
static int test_move_left(void)
{
    return test_mouse_movement_axis(-50, 0, 0, "move left");
}

/* Test: Mouse movement down (positive Y) */
static int test_move_down(void)
{
    return test_mouse_movement_axis(0, 50, 0, "move down");
}

/* Test: Mouse movement up (negative Y) */
static int test_move_up(void)
{
    return test_mouse_movement_axis(0, -50, 0, "move up");
}

/* Test: Mouse scroll wheel up (positive Z) */
static int test_scroll_up(void)
{
    return test_mouse_movement_axis(0, 0, 4, "scroll up");
}

/* Test: Mouse scroll wheel down (negative Z) */
static int test_scroll_down(void)
{
    return test_mouse_movement_axis(0, 0, -4, "scroll down");
}

/* Test: Left mouse button */
static int test_button_left(void)
{
    return test_mouse_button_input(MOUSE_BTN_LEFT, "press left button");
}

/* Test: Right mouse button */
static int test_button_right(void)
{
    return test_mouse_button_input(MOUSE_BTN_RIGHT, "press right button");
}

/* Test: Middle mouse button */
static int test_button_middle(void)
{
    return test_mouse_button_input(MOUSE_BTN_MIDDLE, "press middle button");
}

/* Test: Multi-button latching - verify each button can be cleared independently */
static int test_multi_button_latching(void)
{
    volatile uint8_t *mouse_buttons_latched = (volatile uint8_t *)_MOUSE_BUTTONS_LATCHED;

    /* Clear any previous latched state */
    volatile uint8_t *mouse_buttons_latched_write = (volatile uint8_t *)_MOUSE_BUTTONS_LATCHED;
    *mouse_buttons_latched_write = 0xFF;

    /* Press left button */
    tb_mouse_send(MOUSE_BTN_LEFT, 0, 0, 0);
    usleep(100);
    tb_mouse_send(0, 0, 0, 0);
    usleep(100);

    /* Press right button */
    tb_mouse_send(MOUSE_BTN_RIGHT, 0, 0, 0);
    usleep(100);
    tb_mouse_send(0, 0, 0, 0);
    usleep(100);

    /* Verify both are latched */
    uint8_t latched = *mouse_buttons_latched;
    if ((latched & (MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT)) != (MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT)) {
        test_puts("Both buttons not latched: 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Clear only left button */
    *mouse_buttons_latched_write = MOUSE_BTN_LEFT;
    latched = *mouse_buttons_latched;

    if (latched & MOUSE_BTN_LEFT) {
        test_puts("Left button not cleared: 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    if (!(latched & MOUSE_BTN_RIGHT)) {
        test_puts("Right button cleared unexpectedly: 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Clear right button */
    *mouse_buttons_latched_write = MOUSE_BTN_RIGHT;
    latched = *mouse_buttons_latched;

    if (latched & (MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT)) {
        test_puts("Buttons not both cleared: 0x");
        uart_print_hex_word(latched);
        test_puts("\n");
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
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_MOUSE) = 1;
        usleep(200000); /* 200ms init time */
    }
}

/* Define test suite */
TEST_SUITE(04_mouse,
    move_right,
    move_left,
    move_down,
    move_up,
    scroll_up,
    scroll_down,
    button_left,
    button_right,
    button_middle,
    multi_button_latching
);
