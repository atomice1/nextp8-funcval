/* Test 7: Screen Output Tests
 *
 * Test 7.1: Color Bars - vertical R/G/B/White bars
 * Test 7.2: Concentric Border Boxes - nested colour frames around screen edge
 * Test 7.3: Single Pixel - white pixel at screen center
 * Test 7.4: Horizontal Line - white line across middle
 *
 * Tests verify screen output by:
 * 1. Request buffer flip (vfrontreq = !vfront)
 * 2. Wait for buffer flip to complete (frontreq == vfront)
 * 3. Wait 10us for next frame to start drawing
 * 4. Wait for vsync interrupt (next frame complete)
 * 5. Take screenshot
 * 6. Check VGA framebuffer (model/sim only, automatic verification)
 * 7. Manual prompt (model/Next only): wait for Z (pass) or X (fail)
 */

#include "m68k_defs.h"
#include "base_macros.h"
#include "funcval_tb.h"

/* Keyboard scancodes */
#define SCANCODE_Z              0x1A    /* Z key - validation pass */
#define SCANCODE_X              0x22    /* X key - validation fail */

/* Interrupt flags */
.section .bss
.align 4
vblank_occurred:
    .long 0
test_failed:
    .byte 0
    .align 4

.section .interrupt_vector, "ax"
    .global _vectors

__interrupt_vector:
    .long   0x100000        /* 0: Initial SP (1MB) */
    .long _start            /* 1: Initial PC */
    /* Exception vectors */
    .long default_handler   /* 2: Bus error */
    .long default_handler   /* 3: Address error */
    .long default_handler   /* 4: Illegal instruction */
    .long default_handler   /* 5: Divide by zero */
    .long default_handler   /* 6: CHK */
    .long default_handler   /* 7: TRAPV */
    .long default_handler   /* 8: Privilege violation */
    .long default_handler   /* 9: Trace */
    .long default_handler   /* 10: Line A emulator */
    .long default_handler   /* 11: Line F emulator */
    .space  16              /* 12-15: Reserved */
    .long default_handler   /* 16: Uninitialized interrupt */
    .space  28              /* 17-23: Reserved */
    .long default_handler   /* 24: Spurious interrupt */
    /* Autovector interrupts (Level 1-7) */
    .long default_handler   /* 25: Level 1 */
    .long vblank_handler    /* 26: Level 2 (VBlank) */
    .long default_handler   /* 27: Level 3 */
    .long default_handler   /* 28: Level 4 */
    .long default_handler   /* 29: Level 5 */
    .long default_handler   /* 30: Level 6 */
    .long default_handler   /* 31: Level 7 */
    .space  768             /* 32-255: Trap vectors and user */

.section .text._start, "ax"
.global _start

_start:
    /* Initialize stack */
    move.l  #0x100000, sp

    /* Clear debug registers */
    movel.  #0x80000a, a0
    move.w  #0, (a0)
    movel.  #0x80000c, a0
    move.w  #0, (a0)

    /* Initialize UART baud rate */
    UART_INIT()

    /* Register usage:
     * a6 = POST code register address
     * a5 = UART control register address
     * a4 = UART data register address
     */
    move.l  #_POST_CODE, a6
    move.l  #_UART_CTRL, a5
    move.l  #_UART_DATA, a4

    /* Detect if running in simulation mode */
    bsr     detect_simulation

    /* Enable interrupts (allow VBlank interrupt at level 2) */
    /* Set interrupt mask to level 1 (allow level 2+ interrupts) */
    andi.w  #0xF8FF, sr         /* Clear interrupt mask (bits 10-8) */
    ori.w   #0x0100, sr         /* Set mask to level 1 (0x0100 = 001 in bits 10-8) */

    /* Enable VBlank interrupt in hardware */
    move.l  #_VBLANK_INTR_CTRL, a0
    move.b  #0x01, (a0)         /* Bit 0 = enable interrupt */

    bsr     test_7_1
    bsr     flip_and_wait_vsync
    bsr     verify_border_boxes
    bsr     check_screen_and_wait

    /* Check if any test failed */
    tst.b   test_failed
    bne.s   tests_failed

    /* All tests complete */
    SET_POST(#0x3F)
    lea     all_done_msg(pc), a0
    UART_PUTS(a0)
    bra.s   shutdown_exit

tests_failed:
    SET_POST(#0x3F)
    lea     tests_failed_msg(pc), a0
    UART_PUTS(a0)

shutdown_exit:
    SHUTDOWN()

/* Detect platform type
 * Sets platform flags:
 * - is_spectrum_next: 1 if running on Spectrum Next hardware
 * - is_simulation: 1 if running in FuncVal testbench simulation
 * - is_model: 1 if running on sQLux-nextp8 software model
 *
 * Detection logic:
 * 1. If ADDR_BUILD_TIMESTAMP_LO is non-zero: Spectrum Next platform
 * 2. Else if ADDR_HW_VERSION_LO is non-zero: simulation (FuncVal testbench)
 * 3. Else: sQLux-nextp8 software model
 */
detect_simulation:
    movem.l d0/a0, -(sp)

    /* Clear all platform flags */
    clr.b   is_spectrum_next
    clr.b   is_simulation
    clr.b   is_model
    clr.b   is_interactive
    clr.b   has_testbench

    /* Check ADDR_BUILD_TIMESTAMP_LO (0x800004) */
    move.l  #0x800004, a0
    move.w  (a0), d0

    beq.s   check_hw_version
    cmp.w   #0x5678, d0
    beq.s   check_hw_version
    bra.s   detected_spectrum_next /* Otherwise it's Spectrum Next */

check_hw_version:
    /* Check ADDR_HW_VERSION_LO (0x800008) */
    move.l  #0x800009, a0
    move.b  (a0), d0        /* Read patch_version from 0x800009 */

    /* Check if hw_version_lo is in range [0xfe, 0xff] */
    cmp.b   #0xfe, d0
    blo.s   detected_simulation  /* If < 0xfe, it's simulation */
    cmp.b   #0xff, d0
    bhi.s   detected_simulation  /* If > 0xff, it's simulation */

    /* Must be sQLux-nextp8 model (hw_version_lo is 0xfe or 0xff) */
    move.b  #1, is_model
    move.b  #1, has_testbench
    /* Check if hw_version_lo == 0xfe (interactive model) */
    cmp.b   #0xfe, d0
    bne.s   detect_done
    move.b  #1, is_interactive
    bra.s   detect_done

detected_spectrum_next:
    move.b  #1, is_spectrum_next
    move.b  #1, is_interactive      /* Spectrum Next is interactive */
    bra.s   detect_done

detected_simulation:
    move.b  #1, is_simulation
    move.b  #1, has_testbench

detect_done:
    movem.l (sp)+, d0/a0
    rts

/* Flip buffers and wait for vsync
 * Steps:
 * 1. Request buffer flip (vfrontreq = !vfront)
 * 2. Wait for buffer flip to complete (frontreq == vfront)
 * 3. Wait 10us for next frame to start drawing
 * 4. Wait for vsync interrupt (next frame complete)
 */
flip_and_wait_vsync:
    movem.l d0-d2/a0, -(sp)

    SET_POST(#16)

    /* Step 1-2: Flip buffers and wait for flip to complete */
    bsr     flip_buffers

    SET_POST(#20)

    /* Step 3: Wait for next frame to start drawing */
    move.w  #10000, d0
wait_10us_flip:
    dbra    d0, wait_10us_flip

    SET_POST(#21)

    /* Step 4: Wait for vsync interrupt (next frame complete) */
    clr.l   vblank_occurred

    /* Poll for vblank with timeout */
    move.l  #10000000, d2       /* Large timeout */
wait_vblank_flip:
    tst.l   vblank_occurred
    bne.s   vblank_done_flip
    subq.l  #1, d2
    bne.s   wait_vblank_flip
    /* Timeout - continue anyway */

vblank_done_flip:
    SET_POST(#22)

    movem.l (sp)+, d0-d2/a0
    rts

/* Check screen output and wait for user validation
 * Follows the sequence:
 * 1. Take screenshot
 * 2. Check VGA framebuffer (model/sim only, automatic verification via verify routine)
 * 3. Manual prompt (model/Next only): wait for Z (pass) or X (fail)
 */
check_screen_and_wait:
    movem.l d0-d2/a0, -(sp)

    /* Step 1: Take screenshot */
    SET_POST(#13)
    bsr     take_screenshot
    SET_POST(#14)

    /* Step 2: Automatic verification on model/sim (skipped on Next) */
    /* The verify routine is called from each test already, so skip here */

    /* Step 3: Manual validation prompt (only on hardware) */
    tst.b   is_interactive
    beq.w   check_done

    /* Display prompt on model and Next */
    lea     validation_prompt_msg(pc), a0
    UART_PUTS(a0)

wait_key_loop:
    /* Read latched keyboard matrix */
    move.l  #_KEYBOARD_MATRIX_LATCHED, a0

    /* Check Z key (scancode 0x1A = 26)
     * Byte offset = 26 / 8 = 3, Bit offset = 26 % 8 = 2 */
    move.b  (SCANCODE_Z / 8)(a0), d0
    btst    #(SCANCODE_Z % 8), d0
    bne.s   validation_pass

    /* Check X key (scancode 0x22 = 34)
     * Byte offset = 34 / 8 = 4, Bit offset = 34 % 8 = 2 */
    move.b  (SCANCODE_X / 8)(a0), d0
    btst    #(SCANCODE_X % 8), d0
    bne.s   validation_fail

    bra.s   wait_key_loop

validation_pass:
    lea     validation_pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   validation_done

validation_fail:
    lea     validation_fail_msg(pc), a0
    UART_PUTS(a0)
    move.b  #1, test_failed

validation_done:
    /* Brief delay to debounce */
    move.w  #5000, d0
wait_debounce:
    dbra    d0, wait_debounce

check_done:
    movem.l (sp)+, d0-d2/a0
    rts

/* VBlank interrupt handler (Level 2 autovector) */
vblank_handler:
    movem.l d0/a0, -(sp)

    SET_POST(#30)

    /* Set flag to indicate vblank occurred */
    move.l  #1, vblank_occurred

    SET_POST(#29)

    /* Acknowledge VBlank interrupt */
    move.l  #_VBLANK_INTR_CTRL, a0
    move.b  #0x01, (a0)         /* Write bit 0 to acknowledge */

    SET_POST(#31)

    movem.l (sp)+, d0/a0
    rte

/* Default exception handler */
default_handler:
    SET_POST(#0xEE)
    lea     exception_msg(pc), a0
    UART_PUTS(a0)
    bra.s   default_handler

/* Set one pixel in the back buffer
 * Input:  d0.w = x (0-127), d1.w = y (0-127), d2.b = color (0-15)
 * Saves and restores all modified registers; clobbers nothing visible to caller
 */
set_pixel:
    movem.l d0-d3/a0, -(sp)

    /* Compute byte offset in the 4-bit-per-pixel buffer:
     * byte_offset = (y * 128 + x) / 2                    */
    move.w  d0, d3               /* d3 = x (save for parity test below) */
    move.w  d1, d0               /* d0 = y */
    lsl.w   #7, d0               /* d0 = y * 128 */
    add.w   d3, d0               /* d0 = y*128 + x */
    lsr.w   #1, d0               /* d0 = byte offset */

    move.l  #_BACK_BUFFER_BASE, a0
    add.w   d0, a0               /* a0 = address of the packed byte */

    /* Select nibble based on x parity */
    btst    #0, d3               /* test bit 0 of original x */
    bne.s   set_pixel_odd

    /* x is even: set low nibble */
    move.b  (a0), d0
    and.b   #0xF0, d0            /* clear low nibble */
    or.b    d2, d0
    move.b  d0, (a0)
    bra.s   set_pixel_done

set_pixel_odd:
    /* x is odd: set high nibble */
    move.b  d2, d0
    lsl.b   #4, d0               /* d0 = color << 4 */
    move.b  (a0), d1
    and.b   #0x0F, d1            /* clear high nibble */
    or.b    d0, d1
    move.b  d1, (a0)

set_pixel_done:
    movem.l (sp)+, d0-d3/a0
    rts

/* Test 7.1: Concentric colour border boxes
 * Draws 4 nested single-pixel-wide border rectangles:
 *   Box 0 (outermost): (0,0)-(127,127), colour 8  (red,    #FF004D)
 *   Box 1:             (1,1)-(126,126), colour 9  (orange, #FFA300)
 *   Box 2:             (2,2)-(125,125), colour 10 (yellow, #FFEC27)
 *   Box 3:             (3,3)-(124,124), colour 11 (green,  #00E436)
 *
 * Register allocation:
 *   d7 = box depth n (0-3)
 *   d6 = colour = 8 + n
 *   d3 = 127 - n  (right/bottom boundary, inclusive)
 *   d4 = 126 - n  (last y for left/right columns)
 *   d0/d1/d2 = x/y/colour passed to set_pixel
 */
test_7_1:
    movem.l d3-d7, -(sp)
    SET_POST(#0x07)

    /* Clear both VRAM buffers */
    bsr     clear_vram

    clr.w   d7                   /* box depth n = 0 */

draw_box_loop:
    /* Compute bounds for this box */
    move.w  d7, d6
    add.b   #8, d6               /* colour = 8 + n */

    move.w  #127, d3
    sub.w   d7, d3               /* 127 - n */

    move.w  #126, d4
    sub.w   d7, d4               /* 126 - n */

    /* ---- Top row: y = n, x = n..127-n ---- */
    move.w  d7, d1               /* y = n */
    move.w  d7, d0               /* x = n */
draw_top_row:
    move.w  d6, d2
    bsr     set_pixel
    addq.w  #1, d0
    cmp.w   d3, d0
    ble.s   draw_top_row

    /* ---- Bottom row: y = 127-n, x = n..127-n ---- */
    move.w  d3, d1               /* y = 127 - n */
    move.w  d7, d0               /* x = n */
draw_bottom_row:
    move.w  d6, d2
    bsr     set_pixel
    addq.w  #1, d0
    cmp.w   d3, d0
    ble.s   draw_bottom_row

    /* ---- Left column: x = n, y = n+1..126-n ---- */
    move.w  d7, d0               /* x = n */
    move.w  d7, d1
    addq.w  #1, d1               /* y = n + 1 (skip corners already drawn) */
draw_left_col:
    move.w  d6, d2
    bsr     set_pixel
    addq.w  #1, d1
    cmp.w   d4, d1
    ble.s   draw_left_col

    /* ---- Right column: x = 127-n, y = n+1..126-n ---- */
    move.w  d3, d0               /* x = 127 - n */
    move.w  d7, d1
    addq.w  #1, d1               /* y = n + 1 */
draw_right_col:
    move.w  d6, d2
    bsr     set_pixel
    addq.w  #1, d1
    cmp.w   d4, d1
    ble.s   draw_right_col

    addq.w  #1, d7
    cmp.w   #4, d7
    blt.s   draw_box_loop

    SET_POST(#0x08)
    lea     test_7_1_msg(pc), a0
    UART_PUTS(a0)

    movem.l (sp)+, d3-d7
    rts

/* Verify concentric border boxes
 *
 * PICO-8 palette in 4-bit 0RGB format (as seen in VGA downsampler output):
 *   Colour 8  = #FF004D → 0x0F04
 *   Colour 9  = #FFA300 → 0x0FA0
 *   Colour 10 = #FFEC27 → 0x0FE2
 *   Colour 11 = #00E436 → 0x00E3
 *
 * Checks 16 pixels (4 per edge):
 *   Top:    (x=64, y=0..3)
 *   Right:  (x=127..124, y=64)
 *   Bottom: (x=64, y=127..124)
 *   Left:   (x=0..3, y=64)
 *
 * Table layout: 16 entries of (x.w, y.w, expected_vga.w)
 * read_vga_pixel preserves d1-d3 and a0, so the loop state survives each call.
 */
verify_border_boxes:
    movem.l d0-d3/a0, -(sp)

    /* Skip verification on real Spectrum Next hardware */
    tst.b   has_testbench
    beq.w   verify_boxes_done

    lea     border_check_table(pc), a0
    move.w  #15, d2              /* 16 entries, decrement loop counter */

verify_boxes_loop:
    move.w  (a0)+, d0            /* x */
    move.w  (a0)+, d1            /* y */
    move.w  (a0)+, d3            /* expected VGA pixel (0RGB 4-bit) */
    /* read_vga_pixel: d0=x in, d1=y in; preserves d1-d3/a0; returns d0 */
    bsr     read_vga_pixel
    cmp.w   d3, d0
    bne.s   verify_boxes_fail
    dbra    d2, verify_boxes_loop

    lea     verify_pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   verify_boxes_done

verify_boxes_fail:
    SET_POST(#0x24)
    lea     verify_fail_msg(pc), a0
    UART_PUTS(a0)
    move.b  #1, test_failed

verify_boxes_done:
    movem.l (sp)+, d0-d3/a0
    rts

/* Read VGA pixel at PICO-8 coordinates (d0.w, d1.w) -> returns pixel in d0.w
 * Input: d0.w = X coordinate (0-127), d1.w = Y coordinate (0-127)
 * Output: d0.w = 16-bit pixel value (0RGB format, 4-bit R/G/B each)
 *
 * VGA framebuffer is 128x128 downsampled buffer (1/6 scale)
 * Each PICO-8 pixel (px, py) maps directly to downsampled buffer (px, py)
 * Downsampled buffer samples VGA at center of 6x6 block: (px*6+3+130, py*6+3)
 * Word offset = py * 128 + px
 *
 * Clobbers: d0-d3, a0
 */
read_vga_pixel:
    movem.l d1-d3/a0, -(sp)

    /* d0 = PICO-8 x (0-127), d1 = PICO-8 y (0-127) */
    /* Downsampled buffer is 128x128, direct mapping */
    /* Offset = y * 128 + x */

    move.w  d1, d2               /* d2 = y */
    lsl.w   #7, d2               /* d2 = y * 128 (shift left 7 bits) */
    add.w   d0, d2               /* d2 = y*128 + x = word offset */

    /* Convert word offset to byte offset (multiply by 2) */
    lsl.l   #1, d2               /* d2 *= 2 for byte offset (32-bit shift) */

    /* Read 16-bit word from downsampled VGA framebuffer */
    move.l  #FUNCVAL_VGA_FB, a0
    add.l   d2, a0               /* a0 += byte offset */
    move.w  (a0), d0             /* Read 16-bit word at offset */

    /* Copy sampled word to the debug register */
    movel.  #0x80000c, a0
    move.w  d0, (a0)

    movem.l (sp)+, d1-d3/a0
    rts

/* Clear both VRAM buffers (back and front) */
clear_vram:
    movem.l d0/a0, -(sp)

    /* Clear back buffer */
    move.l  #_BACK_BUFFER_BASE, a0
    move.w  #(_FRAME_BUFFER_SIZE/2)-1, d0
clear_back:
    move.w  #0, (a0)+
    dbra    d0, clear_back

    /* Clear front buffer */
    move.l  #_FRONT_BUFFER_BASE, a0
    move.w  #(_FRAME_BUFFER_SIZE/2)-1, d0
clear_front:
    move.w  #0, (a0)+
    dbra    d0, clear_front

    movem.l (sp)+, d0/a0
    rts

/* Subroutine: Flip VRAM buffers */
flip_buffers:
    movem.l d2, -(sp)

    SET_POST(#17)
    /* Read current VFRONT register */
    move.l  #0x80001D, a0       /* ADDR_VFRONT = 0x80001D */
    move.b  (a0), d0            /* d0 = current VFRONT value (0 or 1) */

    /* Calculate new VFRONT value (toggle bit 0) */
    eor.b   #0x01, d0           /* d0 = current VFRONT XOR 1 */

    /* Write new value to VFRONTREQ to request flip */
    move.b  d0, (a0)            /* ADDR_VFRONTREQ = 0x80001D (same address, write operation) */

    /* Poll VFRONT until it matches the requested value (with timeout) */
    move.l  #100000000, d2        /* Timeout counter (large 32-bit value) */
flip_wait:
    move.b  (a0), d1            /* d1 = current VFRONT */
    cmp.b   d0, d1              /* compare with requested value */
    beq.s   flip_done           /* exit if equal */

    subq.l  #1, d2              /* decrement 32-bit timeout */
    bne.s   flip_wait           /* loop if not zero */

flip_done:
    SET_POST(#19)
    movem.l (sp)+, d2
    rts

/* Trigger screenshot capture
 * Writes to FuncVal testbench screenshot register
 */
take_screenshot:
    movem.l d0/a0, -(sp)

    /* Write to screenshot register (any value triggers capture) */
    move.l  #FUNCVAL_SCREENSHOT, a0
    move.b  #0x01, (a0)

    /* Brief delay to allow capture to complete */
    move.w  #1000, d0
screenshot_delay:
    dbra    d0, screenshot_delay

    movem.l (sp)+, d0/a0
    rts

h_line_x_coords:
    .word 32, 64, 96

/* border_check_table: 16 entries of (x.w, y.w, expected_vga_pixel.w)
 * VGA downsampler 4-bit 0RGB values for PICO-8 colours:
 *   8  = #FF004D → 0x0F04
 *   9  = #FFA300 → 0x0FA0
 *   10 = #FFEC27 → 0x0FE2
 *   11 = #00E436 → 0x00E3
 */
border_check_table:
    /* Top edge: (x=64, y=0..3) */
    .word  64,   0, 0x0F04
    .word  64,   1, 0x0FA0
    .word  64,   2, 0x0FE2
    .word  64,   3, 0x00E3
    /* Right edge: (x=127..124, y=64) */
    .word 127,  64, 0x0F04
    .word 126,  64, 0x0FA0
    .word 125,  64, 0x0FE2
    .word 124,  64, 0x00E3
    /* Bottom edge: (x=64, y=127..124) */
    .word  64, 127, 0x0F04
    .word  64, 126, 0x0FA0
    .word  64, 125, 0x0FE2
    .word  64, 124, 0x00E3
    /* Left edge: (x=0..3, y=64) */
    .word   0,  64, 0x0F04
    .word   1,  64, 0x0FA0
    .word   2,  64, 0x0FE2
    .word   3,  64, 0x00E3
border_check_table_end:

.section .bss
/* Platform detection flags */
is_spectrum_next:
    .byte 0
is_simulation:
    .byte 0
is_model:
    .byte 0
is_interactive:
    .byte 0
has_testbench:
    .byte 0

.section .rodata
test_7_1_msg:
    .asciz "Test 7.1: Look for concentric colour border boxes\n"

all_done_msg:
    .asciz "All screen output tests PASSED\n"

tests_failed_msg:
    .asciz "One or more screen output tests FAILED\n"

press_z_msg:
    .asciz "Press Z key to continue...\n"

validation_prompt_msg:
    .asciz "Verify screen output - Press Z (pass) or X (fail): "

validation_pass_msg:
    .asciz "User validation: PASS\n"

validation_fail_msg:
    .asciz "User validation: FAIL\n"

verify_pass_msg:
    .asciz "VGA framebuffer verification: PASS\n"

verify_fail_msg:
    .asciz "VGA framebuffer verification: FAIL\n"

exception_msg:
    .asciz "EXCEPTION OCCURRED\n"
