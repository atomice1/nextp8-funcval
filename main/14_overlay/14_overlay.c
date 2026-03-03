/* Test Suite: Overlay Control and VRAM
 * Tests overlay buffer management and transparency control
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdbool.h>
#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "funcval_tb.h"
#include "mmio.h"

#define PALETTE_BACK_BASE  0xc08000

#define COLOR_RED_IDX   8
#define COLOR_GREEN_IDX 11
#define COLOR_BLUE_IDX  12
#define COLOR_WHITE_IDX 7

#define COLOR_RED_RGB   0x0F04
#define COLOR_GREEN_RGB 0x00E3
#define COLOR_BLUE_RGB  0x02AF
#define COLOR_WHITE_RGB 0x0FFE

static const uint8_t bar_colors[4] = {
    COLOR_RED_IDX,
    COLOR_GREEN_IDX,
    COLOR_BLUE_IDX,
    COLOR_WHITE_IDX
};

static uint16_t expected_rgb_for_index(uint8_t idx)
{
    switch (idx) {
    case COLOR_RED_IDX:
        return COLOR_RED_RGB;
    case COLOR_GREEN_IDX:
        return COLOR_GREEN_RGB;
    case COLOR_BLUE_IDX:
        return COLOR_BLUE_RGB;
    case COLOR_WHITE_IDX:
        return COLOR_WHITE_RGB;
    default:
        return 0x0000;
    }
}

static void overlay_set_control(uint8_t key_color, bool enable)
{
    MMIO_REG8(_OVERLAY_CONTROL) = (enable ? _OVERLAY_ENABLE_BIT : 0) | (key_color & _OVERLAY_TRANSPARENT_MASK);
}

static void flip_buffers(void)
{
    uint8_t vfront_before = MMIO_REG8(_VFRONT);
    MMIO_REG8(_VFRONTREQ) = !vfront_before;

    uint64_t start_time = MMIO_REG64(_UTIMER_1MHZ);
    while (MMIO_REG8(_VFRONT) == vfront_before) {
        if ((MMIO_REG64(_UTIMER_1MHZ) - start_time) > 100000) {
            break;
        }
    }

    /* flip_buffers() returns the moment vsync N fires (VFRONT toggles).
     * The VGA model hasn't captured the new frame yet at that instant.
     * First wait_vsync() consumes vsync N; second waits for vsync N+1
     * at which point the model has captured a complete new frame. */
    wait_vsync();
    wait_vsync();
}

static uint16_t read_vga_pixel(uint16_t x, uint16_t y)
{
    volatile uint16_t *fb = (volatile uint16_t *)FUNCVAL_VGA_FB;
    return fb[y * 128 + x];
}

static void clear_buffer(uint8_t *buf)
{
    for (int i = 0; i < _FRAME_BUFFER_SIZE; i++) {
        buf[i] = 0x00;
    }
}

static void draw_horizontal_bars(uint8_t *buf)
{
    for (int bar = 0; bar < 4; bar++) {
        uint8_t color = bar_colors[bar];
        uint8_t color_byte = (uint8_t)((color << 4) | color);
        int y_start = bar * 32;
        int y_end = y_start + 32;
        for (int y = y_start; y < y_end; y++) {
            uint8_t *row = buf + (y * 64);
            for (int i = 0; i < 64; i++) {
                row[i] = color_byte;
            }
        }
    }
}

static void draw_vertical_bars(uint8_t *buf)
{
    for (int y = 0; y < 128; y++) {
        uint8_t *row = buf + (y * 64);
        for (int bar = 0; bar < 4; bar++) {
            uint8_t color = bar_colors[bar];
            uint8_t color_byte = (uint8_t)((color << 4) | color);
            int offset = bar * 16;
            for (int i = 0; i < 16; i++) {
                row[offset + i] = color_byte;
            }
        }
    }
}

static int verify_pixel(uint16_t x, uint16_t y, uint16_t expected)
{
    uint16_t got = read_vga_pixel(x, y);
    if (got != expected) {
        test_puts("    Pixel mismatch at (");
        uart_print_hex_word(x);
        test_puts(", ");
        uart_print_hex_word(y);
        test_puts("): expected 0x");
        uart_print_hex_word(expected);
        test_puts(", got 0x");
        uart_print_hex_word(got);
        test_print_crlf();
        return 1;
    }
    return 0;
}

/* Scenario 1: Overlay VRAM separate from screen VRAM */
static int test_overlay_buffer_isolation(void)
{
    test_puts("  Scenario 1: buffer isolation... ");

    volatile uint16_t *screen_back = (volatile uint16_t *)_BACK_BUFFER_BASE;
    volatile uint16_t *overlay_back = (volatile uint16_t *)_OVERLAY_BACK_BUFFER_BASE;

    screen_back[0] = 0x1111;
    overlay_back[0] = 0x2222;

    if (screen_back[0] != 0x1111 || overlay_back[0] != 0x2222) {
        test_puts("FAIL");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Scenario 2: Horizontal bars in overlay back buffer are visible after flip */
static int test_overlay_horizontal_bars(void)
{
    test_puts("  Scenario 2: overlay horizontal bars... ");
    test_print_crlf();

    clear_buffer((uint8_t *)_BACK_BUFFER_BASE);
    clear_buffer((uint8_t *)_OVERLAY_BACK_BUFFER_BASE);
    clear_buffer((uint8_t *)_OVERLAY_FRONT_BUFFER_BASE);

    draw_horizontal_bars((uint8_t *)_OVERLAY_BACK_BUFFER_BASE);
    overlay_set_control(0, true);
    flip_buffers();

    if (platform_has_testbench) {
        int errors = 0;
        errors += verify_pixel(64, 16, expected_rgb_for_index(COLOR_RED_IDX));
        errors += verify_pixel(64, 48, expected_rgb_for_index(COLOR_GREEN_IDX));
        errors += verify_pixel(64, 80, expected_rgb_for_index(COLOR_BLUE_IDX));
        errors += verify_pixel(64, 112, expected_rgb_for_index(COLOR_WHITE_IDX));
        if (errors > 0) {
            test_puts("  FAIL");
            test_print_crlf();
            return TEST_FAIL;
        }
    }
    if (platform_is_interactive) {
        test_puts("  Check display shows 4 horizontal color bars");
        test_print_crlf();
        screen_flip();
        wait_for_any_key();
    }

    test_puts("  PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Scenario 3: Overlay key color is transparent over screen bars */
static int test_overlay_key_transparency(void)
{
    test_puts("  Scenario 3: overlay key transparency... ");
    test_print_crlf();

    clear_buffer((uint8_t *)_BACK_BUFFER_BASE);
    clear_buffer((uint8_t *)_OVERLAY_BACK_BUFFER_BASE);

    draw_vertical_bars((uint8_t *)_BACK_BUFFER_BASE);
    draw_horizontal_bars((uint8_t *)_OVERLAY_BACK_BUFFER_BASE);

    overlay_set_control(COLOR_GREEN_IDX, true);
    flip_buffers();

    if (platform_has_testbench) {
        int errors = 0;

        errors += verify_pixel(16, 16, expected_rgb_for_index(COLOR_RED_IDX));
        errors += verify_pixel(48, 16, expected_rgb_for_index(COLOR_RED_IDX));
        errors += verify_pixel(80, 16, expected_rgb_for_index(COLOR_RED_IDX));
        errors += verify_pixel(112, 16, expected_rgb_for_index(COLOR_RED_IDX));

        errors += verify_pixel(16, 48, expected_rgb_for_index(COLOR_RED_IDX));
        errors += verify_pixel(48, 48, expected_rgb_for_index(COLOR_GREEN_IDX));
        errors += verify_pixel(80, 48, expected_rgb_for_index(COLOR_BLUE_IDX));
        errors += verify_pixel(112, 48, expected_rgb_for_index(COLOR_WHITE_IDX));

        errors += verify_pixel(16, 80, expected_rgb_for_index(COLOR_BLUE_IDX));
        errors += verify_pixel(48, 80, expected_rgb_for_index(COLOR_BLUE_IDX));
        errors += verify_pixel(80, 80, expected_rgb_for_index(COLOR_BLUE_IDX));
        errors += verify_pixel(112, 80, expected_rgb_for_index(COLOR_BLUE_IDX));

        if (errors > 0) {
            test_puts("  FAIL");
            test_print_crlf();
            return TEST_FAIL;
        }
    }
    if (platform_is_interactive) {
        test_puts("  Check key-color bar is transparent over vertical bars");
        test_print_crlf();
        screen_flip();
        wait_for_any_key();
    }

    test_puts("  PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Scenario 4: Overlay bars ignore back palette changes */
static int test_overlay_palette_independence(void)
{
    test_puts("  Scenario 4: overlay ignores back palette... ");
    test_print_crlf();

    clear_buffer((uint8_t *)_BACK_BUFFER_BASE);
    clear_buffer((uint8_t *)_OVERLAY_BACK_BUFFER_BASE);
    clear_buffer((uint8_t *)_OVERLAY_FRONT_BUFFER_BASE);

    draw_horizontal_bars((uint8_t *)_OVERLAY_BACK_BUFFER_BASE);
    overlay_set_control(0, true);

    volatile uint8_t *palette_back = (volatile uint8_t *)PALETTE_BACK_BASE;
    palette_back[COLOR_RED_IDX] = 0;
    palette_back[COLOR_GREEN_IDX] = 1;
    palette_back[COLOR_BLUE_IDX] = 2;
    palette_back[COLOR_WHITE_IDX] = 3;

    flip_buffers();

    if (platform_has_testbench) {
        int errors = 0;
        errors += verify_pixel(64, 16, expected_rgb_for_index(COLOR_RED_IDX));
        errors += verify_pixel(64, 48, expected_rgb_for_index(COLOR_GREEN_IDX));
        errors += verify_pixel(64, 80, expected_rgb_for_index(COLOR_BLUE_IDX));
        errors += verify_pixel(64, 112, expected_rgb_for_index(COLOR_WHITE_IDX));
        if (errors > 0) {
            test_puts("  FAIL");
            test_print_crlf();
            return TEST_FAIL;
        }
    }
    if (platform_is_interactive) {
        test_puts("  Check overlay bars unchanged after palette change");
        test_print_crlf();
        screen_flip();
        wait_for_any_key();
    }

    test_puts("  PASS");
    test_print_crlf();
    return TEST_PASS;
}

void software_init_hook(void)
{
    platform_detect();
    if (platform_is_simulation)
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_VGA) = 1;
}

/* Test suite array */
TEST_SUITE(14_overlay,
           overlay_buffer_isolation,
           overlay_horizontal_bars,
           overlay_key_transparency,
           overlay_palette_independence);
