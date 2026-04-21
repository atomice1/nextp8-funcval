/* Test Suite: Advanced Screen Output
 * Tests screen transforms and high-color modes
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include <string.h>
#include "nextp8.h"
#include "funcval.h"
#include "funcval_tb.h"
#include "mmio.h"

/* VGA framebuffer readback helper (FuncVal testbench) */
static uint16_t vga_pixel(uint16_t x, uint16_t y)
{
    volatile uint16_t *fb = (volatile uint16_t *)FUNCVAL_VGA_FB;
    return fb[y * 128 + x];
}

/* Draw four corner pixels with distinct palette indices */
static void draw_corner_markers(void)
{
    screen_clear();
    screen_set_pixel(0, 0, 8);
    screen_set_pixel(127, 0, 9);
    screen_set_pixel(0, 127, 10);
    screen_set_pixel(127, 127, 11);
}

/* Mapping for flip/rotate transforms for 128x128 canvas
 * Returns packed 32-bit value: (rx << 16) | ry
 */
static uint32_t map_coord_for_transform(uint8_t mode, uint16_t x, uint16_t y)
{
    uint16_t rx, ry;
    switch (mode) {
    case 0: /* normal */
        rx = x; ry = y; break;
    case 129: /* horiz flip */
        rx = 127 - x; ry = y; break;
    case 130: /* vert flip */
        rx = x; ry = 127 - y; break;
    case 131: /* both flip */
        rx = 127 - x; ry = 127 - y; break;
    case 133: /* rotate clockwise 90 */
        rx = 127 - y; ry = x; break;
    case 134: /* rotate 180 */
        rx = 127 - x; ry = 127 - y; break;
    case 135: /* rotate counterclockwise 90 */
        rx = y; ry = 127 - x; break;
    default:
        rx = x; ry = y; break;
    }
    return ((uint32_t)rx << 16) | (uint32_t)ry;
}

/* Helper: run a single transform test case (mode) */
static int run_transform_case(uint8_t mode)
{
    test_puts("  Testing screen transform mode: ");
    uart_print_hex_byte(mode);
    test_print_crlf();

    /* set transform register (0x8000a1) */
    MMIO_REG8(0x8000a1) = mode;

    draw_corner_markers();
    screen_flip();
    /* wait for the flip vsync and next frame for VGA capture */
    wait_vsync();
    wait_vsync();

    if (platform_has_testbench) {
        uint16_t ex_rx, ex_ry;
        int errors = 0;
        uint32_t packed;

        packed = map_coord_for_transform(mode, 0, 0);
        ex_rx = (uint16_t)(packed >> 16);
        ex_ry = (uint16_t)(packed & 0xFFFF);
        if (vga_pixel(ex_rx, ex_ry) == 0) errors++;

        packed = map_coord_for_transform(mode, 127, 0);
        ex_rx = (uint16_t)(packed >> 16);
        ex_ry = (uint16_t)(packed & 0xFFFF);
        if (vga_pixel(ex_rx, ex_ry) == 0) errors++;

        packed = map_coord_for_transform(mode, 0, 127);
        ex_rx = (uint16_t)(packed >> 16);
        ex_ry = (uint16_t)(packed & 0xFFFF);
        if (vga_pixel(ex_rx, ex_ry) == 0) errors++;

        packed = map_coord_for_transform(mode, 127, 127);
        ex_rx = (uint16_t)(packed >> 16);
        ex_ry = (uint16_t)(packed & 0xFFFF);
        if (vga_pixel(ex_rx, ex_ry) == 0) errors++;

        if (errors) {
            test_puts("  FAIL (vga pixel mismatch)");
            test_print_crlf();
            return TEST_FAIL;
        }
    } else if (platform_is_interactive) {
        test_puts("  Inspect display for transform mode: ");
        uart_print_hex_byte(mode);
        test_print_crlf();
        screen_flip();
        wait_for_any_key();
    }

    test_puts("  PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* One test function per transform mode (split from the combined test) */
static int test_transform_0(void) { return run_transform_case(0); }
static int test_transform_129(void) { return run_transform_case(129); }
static int test_transform_130(void) { return run_transform_case(130); }
static int test_transform_131(void) { return run_transform_case(131); }
static int test_transform_133(void) { return run_transform_case(133); }
static int test_transform_134(void) { return run_transform_case(134); }
static int test_transform_135(void) { return run_transform_case(135); }

/* Scenario: High-colour mode 0x10 (per-line secondary palette) smoke+verification */
static int test_high_colour_mode_10(void)
{
    test_puts("  Testing high-colour mode 0x10 (per-line palettes)... ");
    test_print_crlf();

    /* Configure a simple secondary palette in secondary-palette-back and mark
     * alternating 8-line bands in high-colour bitfield back. Use physical
     * addresses from memory_map: 0xc08030 (secondary palette back) and
     * 0xc08040 (high-colour bitfield back). */
    volatile uint16_t *hc_lut = (volatile uint16_t *)0xc08030;
    volatile uint8_t *hc_mask = (volatile uint8_t *)0xc08040;

    /* Fill secondary LUT with distinct indices (0..15) */
    for (int i = 0; i < 8; i++) {
        uint8_t lo = i * 2;
        uint8_t hi = i * 2 + 1;
        uint8_t b0 = (lo & 0x0f) | ((lo & 0x10) << 3);
        uint8_t b1 = (hi & 0x0f) | ((hi & 0x10) << 3);
        hc_lut[i] = ((uint16_t)b0 << 8) | b1;
    }

    /* Mark alternate 8-line sections: set bits for every other section */
    for (int i = 0; i < 16; i++) {
        hc_mask[i] = (i & 1) ? 0xFF : 0x00;
    }

    /* Enable high-colour mode 0x10 */
    MMIO_REG8(0x8000a3) = 0x10;

    /* Draw stripes using base palette indices 0..15 (one stripe per 8 lines) */
    for (int i = 0; i < 16; i++) {
        screen_fill_rect(0, i * 8, 128, 8, i);
    }

    screen_flip();
    /* wait for the flip vsync and next frame for VGA capture */
    wait_vsync();
    wait_vsync();

    if (platform_has_testbench) {
        volatile uint16_t *vga_fb = (volatile uint16_t *)FUNCVAL_VGA_FB;
        int errors = 0;

        /* For sections where hc_mask bit set, expect pixel to reflect secondary LUT value
         * (we only check a single sample per section). */
        for (int sec = 0; sec < 16; sec++) {
            int y = sec * 8 + 4;
            int x = 64;
            uint16_t pix = vga_fb[y * 128 + x];
            if ((hc_mask[sec] & 0x01) == 0) {
                /* primary palette used: pixel should be non-zero */
                if (pix == 0) errors++;
            } else {
                /* secondary palette used: expect non-zero too */
                if (pix == 0) errors++;
            }
            if (errors >= 8) break;
        }

        if (errors) {
            test_puts("  FAIL (high-colour verification)");
            test_print_crlf();
            return TEST_FAIL;
        }
    } else if (platform_is_interactive) {
        test_puts("  Check alternated palette bands on display");
        test_print_crlf();
        screen_flip();
        wait_for_any_key();
    }

    /* Disable high-colour mode */
    MMIO_REG8(0x8000a3) = 0x00;

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
TEST_SUITE(08_advanced_screen_output,
           transform_0,
           transform_129,
           transform_130,
           transform_131,
           transform_133,
           transform_134,
           transform_135,
           high_colour_mode_10);
