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
#include "funcval_tb.h"

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

/* Funcval VGA loopback: 128x128 array of 0rgb 12-bit pixels at this base address */
#define FUNCVAL_VGA_FB_BASE 0x390000

/* Test 1: Write and readback all back palette slots */
static int test_back_palette_readback(void)
{
    test_puts("  Testing back palette write/readback... ");
    test_print_crlf();

    volatile uint16_t *palette_back = (volatile uint16_t *)PALETTE_BACK_BASE;
    uint16_t test_values[8];

    /* Write test pattern to all 8 slots (each slot packs 2 palette entries) */
    for (int i = 0; i < 8; i++) {
        test_values[i] = (i << 8) | (7 - i);
        palette_back[i] = test_values[i];
    }

    /* Read back and verify */
    int errors = 0;
    for (int i = 0; i < 8; i++) {
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

    return TEST_PASS;
}

/* Test 2: Check back palette becomes front after flip */
static int test_palette_flip(void)
{
    test_puts("  Testing palette flip (back->front)... ");
    test_print_crlf();

    volatile uint16_t *palette_back = (volatile uint16_t *)PALETTE_BACK_BASE;
    volatile uint16_t *palette_front = (volatile uint16_t *)PALETTE_FRONT_BASE;

    /* Write unique values to all 8 back palette slots.
     * Each palette byte stores a 5-bit PICO-8 colour index in the format
     * used by p8video: bit 7 = index bit 4, bits 6:4 = 0 (ignored on write,
     * returned as 0 on read), bits 3:0 = index bits 3:0.
     * Legal byte values: 0x00-0x0F (index 0-15) or 0x80-0x8F (index 16-31).
     * Use extended indices 16-31 (bytes 0x80-0x8F) so they are distinct from
     * the power-on default palette (which maps colour n to index n, all ≤ 15). */
    uint16_t unique_values[8];
    for (int i = 0; i < 8; i++) {
        uint8_t hi = (uint8_t)(0x80 | (i * 2));      /* PICO-8 index 16 + i*2 */
        uint8_t lo = (uint8_t)(0x80 | (i * 2 + 1));  /* PICO-8 index 17 + i*2 */
        unique_values[i] = ((uint16_t)hi << 8) | lo;
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
    for (int i = 0; i < 8; i++) {
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

    /* Set back palette with the mapping.
     * Each word slot packs two 5-bit PICO-8 colour indices.
     * Byte format for index c: bits[3:0]=c[3:0], bit[7]=c[4]. */
    volatile uint16_t *palette_back = (volatile uint16_t *)PALETTE_BACK_BASE;
    for (int i = 0; i < 8; i++) {
        uint8_t e0 = palette_mapping[i * 2];
        uint8_t e1 = palette_mapping[i * 2 + 1];
        uint8_t b0 = (e0 & 0x0F) | ((e0 & 0x10) << 3);
        uint8_t b1 = (e1 & 0x0F) | ((e1 & 0x10) << 3);
        palette_back[i] = ((uint16_t)b0 << 8) | b1;
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
    screen_cursor_x = 0;
    screen_cursor_y = 0;

    /* Copy off-screen buffer to back framebuffer and flip to display.
     * screen_flip():
     *   1. Copies the off-screen buffer to back frame buffer.
     *   2. Sets VFRONTREQ = !VFRONT to request the flip.
     *   3. Polls until VFRONT matches VFRONTREQ (flip has occurred). */
    test_puts("  [DBG] vblank_count before flip: ");
    uart_print_hex_long(vblank_count);
    test_print_crlf();

    screen_flip();

    test_puts("  [DBG] vblank_count after flip: ");
    uart_print_hex_long(vblank_count);
    test_print_crlf();

    /* screen_flip() polls VFRONT and returns right at vsync N (the flip
     * vsync).  Its VBlank interrupt may not have fired yet, so a single
     * wait_vsync() call catches vsync N itself rather than the *next* frame.
     * Call twice: first consumes vsync N, second waits for vsync N+1 at which
     * point the VGA model has captured a complete new frame with the updated
     * palette and framebuffer content. */
    wait_vsync();  /* consume vsync N (the flip vsync) */
    int vsync_timeout = wait_vsync();  /* wait for vsync N+1 (full new frame) */

    test_puts("  [DBG] vblank_count after wait_vsync: ");
    uart_print_hex_long(vblank_count);
    if (vsync_timeout)
        test_puts(" (TIMED OUT!)");
    test_print_crlf();

    /* Check if we're in interactive mode */
    if (platform_is_interactive) {
        test_puts("  Check screen shows colored stripes with names");
        test_print_crlf();

        wait_for_any_key();
    } else {
        /* Verify using VGA loopback MMIO region (128x128 0rgb pixels at FUNCVAL_VGA_FB_BASE).
         * Sample one pixel from the center of each stripe and compare against the
         * expected 0rgb value derived from the system palette table. */
        volatile uint16_t *vga_fb = (volatile uint16_t *)FUNCVAL_VGA_FB_BASE;
        int errors = 0;

        for (int i = 0; i < 16; i++) {
            /* Centre pixel of stripe i: x=64, y=i*8+4 */
            uint16_t pixel = vga_fb[(i * 8 + 4) * 128 + 64];

            uint8_t pico8_idx = palette_mapping[i];
            uint8_t r = pico8_colors[pico8_idx].r;
            uint8_t g = pico8_colors[pico8_idx].g;
            uint8_t b = pico8_colors[pico8_idx].b;
            uint16_t expected = ((uint16_t)(r >> 4) << 8) |
                                 ((uint16_t)(g >> 4) << 4) |
                                  (b >> 4);

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
                if (errors >= 5)
                    break;
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

    return TEST_PASS;
}

void software_init_hook(void)
{
    platform_detect();
    if (platform_is_simulation)
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_VGA) = 1;
}

/* Test suite array */
TEST_SUITE(12_palette,
           back_palette_readback,
           palette_flip,
           palette_screen_output);
