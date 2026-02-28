/* Test Suite: Palette Registers
 * Tests palette memory access for 16-color palette
 * Tests palette flipping and screen output
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include <string.h>
#include "nextp8.h"
#include "funcval.h"
#include "mmio.h"

/* Palette addresses */
#define PALETTE_BACK_BASE  0xc08000
#define PALETTE_FRONT_BASE 0xc08010

/* PICO-8 color RGB values (for verification) */
static const struct {
    uint8_t r, g, b;
    const char *name;
} pico8_colors[32] = {
    {0, 0, 0, "black"}, {29, 43, 83, "dark-blue"}, {126, 37, 83, "dark-purple"},
    {0, 135, 81, "dark-green"}, {171, 82, 54, "brown"}, {95, 87, 79, "dark-grey"},
    {194, 195, 199, "light-grey"}, {255, 241, 232, "white"}, {255, 0, 77, "red"},
    {255, 163, 0, "orange"}, {255, 236, 39, "yellow"}, {0, 228, 54, "green"},
    {41, 173, 255, "blue"}, {131, 118, 156, "lavender"}, {255, 119, 168, "pink"},
    {255, 204, 170, "light-peach"}, {41, 24, 20, "brownish-black"},
    {17, 29, 53, "darker-blue"}, {66, 33, 54, "darker-purple"},
    {18, 83, 89, "blue-green"}, {116, 47, 41, "dark-brown"},
    {73, 51, 59, "darker-grey"}, {162, 136, 121, "medium-grey"},
    {243, 239, 125, "light-yellow"}, {190, 18, 80, "dark-red"},
    {255, 108, 36, "dark-orange"}, {168, 231, 46, "lime-green"},
    {0, 181, 67, "medium-green"}, {6, 90, 181, "true-blue"},
    {117, 70, 101, "mauve"}, {255, 110, 89, "dark-peach"},
    {255, 157, 129, "peach"}
};

/* Helper: Convert RGB888 to RGB565 */
static uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/* Test 1: Write and readback all back palette slots */
static int test_back_palette_readback(void)
{
    test_puts("  Testing back palette write/readback... ");
    test_print_crlf();

    volatile uint16_t *palette_back = (volatile uint16_t *)PALETTE_BACK_BASE;
    uint16_t test_values[16];

    /* Write test pattern to all 16 slots */
    for (int i = 0; i < 16; i++) {
        test_values[i] = (i << 8) | (15 - i);
        palette_back[i] = test_values[i];
    }

    /* Read back and verify */
    int errors = 0;
    for (int i = 0; i < 16; i++) {
        uint16_t readback = palette_back[i];
        if (readback != test_values[i]) {
            test_puts("    Slot ");
            uart_print_hex_word(i);
            test_puts(": expected 0x");
            uart_print_hex_word(test_values[i]);
            test_puts(", got 0x");
            uart_print_hex_word(readback);
            test_print_crlf();
            errors++;
        }
    }

    if (errors > 0) {
        test_puts("  FAIL (");
        uart_print_hex_word(errors);
        test_puts(" errors)");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("  PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 2: Check back palette becomes front after flip */
static int test_palette_flip(void)
{
    test_puts("  Testing palette flip (back->front)... ");
    test_print_crlf();

    volatile uint16_t *palette_back = (volatile uint16_t *)PALETTE_BACK_BASE;
    volatile uint16_t *palette_front = (volatile uint16_t *)PALETTE_FRONT_BASE;

    /* Write unique values to back palette */
    uint16_t unique_values[16];
    for (int i = 0; i < 16; i++) {
        unique_values[i] = 0xA000 | (i * 0x111);
        palette_back[i] = unique_values[i];
    }

    /* Read current VFRONT */
    uint8_t vfront_before = MMIO_REG8(_VFRONT);

    /* Request flip: VFRONTREQ = !VFRONT */
    MMIO_REG8(_VFRONTREQ) = !vfront_before;

    /* Wait for flip to complete */
    uint64_t start_time = MMIO_REG64(_UTIMER_1MHZ);
    while (MMIO_REG8(_VFRONT) == vfront_before) {
        if ((MMIO_REG64(_UTIMER_1MHZ) - start_time) > 100000) {
            test_puts("  FAIL (flip timeout)");
            test_print_crlf();
            return TEST_FAIL;
        }
    }

    /* Verify front palette now contains back palette values */
    int errors = 0;
    for (int i = 0; i < 16; i++) {
        uint16_t front_val = palette_front[i];
        if (front_val != unique_values[i]) {
            test_puts("    Slot ");
            uart_print_hex_word(i);
            test_puts(": expected 0x");
            uart_print_hex_word(unique_values[i]);
            test_puts(", got 0x");
            uart_print_hex_word(front_val);
            test_print_crlf();
            errors++;
        }
    }

    if (errors > 0) {
        test_puts("  FAIL (");
        uart_print_hex_word(errors);
        test_puts(" errors)");
        test_print_crlf();
        return TEST_FAIL;
    }

    test_puts("  PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test 3: Verify palette is used in screen output */
static int test_palette_screen_output(void)
{
    test_puts("  Testing palette in screen output... ");
    test_print_crlf();

    /* Clear screen */
    screen_clear();

    /* Mapping: palette index -> PICO-8 color index (mix of base 0-15 and extended 16-31) */
    uint8_t palette_mapping[16] = {
        0,  /* black */
        8,  /* red */
        12, /* blue */
        11, /* green */
        10, /* yellow */
        14, /* pink */
        6,  /* light-grey */
        7,  /* white */
        16, /* brownish-black (extended) */
        24, /* dark-red (extended) */
        28, /* true-blue (extended) */
        27, /* medium-green (extended) */
        26, /* lime-green (extended) */
        30, /* dark-peach (extended) */
        22, /* medium-grey (extended) */
        31  /* peach (extended) */
    };

    /* Set back palette with the mapping */
    volatile uint16_t *palette_back = (volatile uint16_t *)PALETTE_BACK_BASE;
    for (int i = 0; i < 16; i++) {
        uint8_t pico8_idx = palette_mapping[i];
        uint16_t rgb565 = rgb_to_rgb565(pico8_colors[pico8_idx].r,
                                        pico8_colors[pico8_idx].g,
                                        pico8_colors[pico8_idx].b);
        palette_back[i] = rgb565;
    }

    /* Draw horizontal stripes, 8 pixels high each */
    for (int i = 0; i < 16; i++) {
        screen_fill_rect(0, i * 8, 128, 8, i);
    }

    /* Draw color names on each stripe */
    for (int i = 0; i < 16; i++) {
        screen_cursor_x = 2;
        screen_cursor_y = i * 8;
        screen_puts(pico8_colors[palette_mapping[i]].name);
    }

    /* Flip screen to display */
    uint8_t vfront_before = MMIO_REG8(_VFRONT);
    MMIO_REG8(_VFRONTREQ) = !vfront_before;

    /* Wait for flip */
    uint64_t start_time = MMIO_REG64(_UTIMER_1MHZ);
    while (MMIO_REG8(_VFRONT) == vfront_before) {
        if ((MMIO_REG64(_UTIMER_1MHZ) - start_time) > 100000) {
            test_puts("  FAIL (screen flip timeout)");
            test_print_crlf();
            return TEST_FAIL;
        }
    }

    /* Check if we're in interactive mode */
    if (platform_is_interactive) {
        test_puts("  Check screen shows colored stripes with names");
        test_print_crlf();
        test_puts("  Displaying for 3 seconds...");
        test_print_crlf();

        /* Wait 3 seconds for visual inspection */
        uint64_t wait_start = MMIO_REG64(_UTIMER_1MHZ);
        while ((MMIO_REG64(_UTIMER_1MHZ) - wait_start) < 3000000) {
            /* Busy wait */
        }
    } else {
        /* Verify using framebuffer loopback (testbench mode) */
        test_puts("  Verifying RGB values via framebuffer...");
        test_print_crlf();

        /* Sample middle pixel of each stripe */
        volatile uint16_t *fb_front = (volatile uint16_t *)_FRONT_BUFFER_BASE;
        int errors = 0;

        for (int i = 0; i < 16; i++) {
            /* Sample at (64, i*8+4) - middle of stripe */
            uint16_t pixel = fb_front[((i * 8 + 4) * 128) + 64];
            uint8_t pico8_idx = palette_mapping[i];
            uint16_t expected = rgb_to_rgb565(pico8_colors[pico8_idx].r,
                                             pico8_colors[pico8_idx].g,
                                             pico8_colors[pico8_idx].b);

            if (pixel != expected) {
                test_puts("    Stripe ");
                uart_print_hex_word(i);
                test_puts(" (");
                test_puts(pico8_colors[pico8_idx].name);
                test_puts("): expected 0x");
                uart_print_hex_word(expected);
                test_puts(", got 0x");
                uart_print_hex_word(pixel);
                test_print_crlf();
                errors++;
                if (errors >= 5) break;  /* Limit error output */
            }
        }

        if (errors > 0) {
            test_puts("  FAIL (");
            uart_print_hex_word(errors);
            test_puts(" pixel mismatches)");
            test_print_crlf();
            return TEST_FAIL;
        }
    }

    test_puts("  PASS");
    test_print_crlf();
    return TEST_PASS;
}

/* Test suite array */
TEST_SUITE(12_palette,
           back_palette_readback,
           palette_flip,
           palette_screen_output);
