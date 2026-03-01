/*
 * Test 6: Timer Tests
 * Tests 1 MHz microsecond timer functionality
 *
 * Test 6.1: Timer Basic Functionality
 * Test 6.2: Timer Increment Verification
 * Test 6.3: Loop Timing Proportionality
 * Test 6.4: Buffer Flip Timing (should be ~33ms for 2 frames @ 60Hz)
 */

#include "m68k_defs.h"
#include "base_macros.h"

    .section .interrupt_vector, "ax"
    .global _vectors

__interrupt_vector:
    .long   0x100000        /* 0: Initial SP (1MB) */
    .long   _start          /* 1: Initial PC */
    .space  0x3F8           /* Fill rest of vector table with zeros */

    .section .text._start, "ax"
    .global _start

_start:
    /* Initialize stack pointer */
    move.l  #0x100000, sp

    /* Configure UART baud rate */
    UART_INIT()

    /* Detect platform type */
    bsr     detect_simulation

    /* Test 6.1: Timer basic functionality */
test_6_1:
    SET_POST(#0x01)

    /* Read 64-bit timer to verify both halves are accessible */
    move.l  #_UTIMER_1MHZ, a0
    move.l  (a0), d0        /* Read high 32 bits */
    move.l  4(a0), d1       /* Read low 32 bits */

    /* Verify we got sensible values (at least one should be non-zero) */
    or.l    d1, d0          /* OR high and low - if both zero, result is zero */
    beq.s   fail_6_1        /* All zeros is unlikely */

    /* Success */
    SET_POST(#0x02)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_6_2

fail_6_1:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

    /* Test 6.2: Verify timer increments */
test_6_2:
    SET_POST(#0x03)

    /* Read first 64-bit timer value */
    move.l  #_UTIMER_1MHZ, a0
    move.l  (a0), d1        /* Read high 32 bits into d1 */
    move.l  4(a0), d2       /* Read low 32 bits into d2 */

    /* Wait a bit for timer to increment */
    move.l  #10000, d0
wait_loop_1:
    subq.l  #1, d0          /* Decrement full 32-bit counter */
    bne.s   wait_loop_1     /* Branch if not equal to zero */

    /* Read second 64-bit timer value */
    move.l  (a0), d3        /* Read high 32 bits into d3 */
    move.l  4(a0), d4       /* Read low 32 bits into d4 */

    /* Check if timer changed (64-bit subtraction: d2 = first_lo - second_lo, d1 = first_hi - second_hi with borrow) */
    sub.l   d2, d4          /* d4 = d4 - d2 (low 32 bits) */
    subx.l  d1, d3          /* d3 = d3 - d1 (high 32 bits with borrow) */
    beq.s   fail_6_2        /* If result is zero, timer didn't advance */

    move.l #_DEBUG_REG_HI, a2
    move.l d4, (a2)         /* 3380 */

    /* Timer incremented, success */
    SET_POST(#0x04)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_6_3

fail_6_2:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

    /* Test 6.3: Loop timing
     * Run loop with N iterations, verify timer changes
     */
test_6_3:
    SET_POST(#0x05)

    /* Skip tests 6.3 and 6.4 on the model */
    tst.b   is_model
    bne.w   end_test

    /* Read first 64-bit timer value */
    move.l  #_UTIMER_1MHZ, a0
    move.l  (a0), d1        /* Read high 32 bits into d1 */
    move.l  4(a0), d2       /* Read low 32 bits into d2 */

    /* Run loop with 20000 iterations (use 32-bit loop counter) */
    move.l  #20000, d0
loop_6_3:
    subq.l  #1, d0          /* Decrement full 32-bit counter */
    bne.s   loop_6_3        /* Branch if not equal to zero */

    /* Read second 64-bit timer value */
    move.l  (a0), d5        /* Read high 32 bits into d5 */
    move.l  4(a0), d6       /* Read low 32 bits into d6 */

    /* Calculate delta */
    sub.l   d2, d6          /* d6 = d6 - d2 (low 32 bits) */
    subx.l  d1, d5          /* d5 = d5 - d1 (high 32 bits with borrow) */

    move.l #_DEBUG_REG_HI, a2
    move.l d6, (a2)         /* 334 */

    /* Check the elapsed time is approximately twice that in test 6.2 */

    /* Subtract 2 * first delta from second delta */
    sub.l   d4, d6          /* d6 = d6 - d4 (low 32 bits) */
    subx.l  d3, d5          /* d5 = d5 - d3  (high 32 bits with borrow) */

    move.l #_DEBUG_REG_HI, a2
    move.l d6, (a2)         /* -3046 */

    sub.l   d4, d6          /* d6 = d6 - d4 (low 32 bits) */
    subx.l  d3, d5          /* d5 = d5 - d3  (high 32 bits with borrow) */

    move.l #_DEBUG_REG_HI, a2
    move.l d6, (a2)         /* -6426 */

    /* Take absolute value */
    tstl   d6
    bge.s  positive_6_3
    negl   d6
    negxl  d5
positive_6_3:

    move.l #_DEBUG_REG_HI, a2
    move.l d6, (a2)         /* 6417 */

    /* Check difference is < 10 */
    clrl      d2
    moveq #9, d3
    subl d3,  d6
    subxl d2, d5

    move.l #_DEBUG_REG_HI, a0
    move.l d6, (a0)

    bgt.s     fail_6_3

    /* Success */
    SET_POST(#0x06)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_6_4

fail_6_3:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

    /* Test 6.4: Buffer flip timing
     * Flip buffer, record timer
     * Flip back and wait for completion
     * Record new timer
     * Difference should be ~33ms (2 frames @ 60Hz)
     */
test_6_4:
    SET_POST(#0x07)

    /* Read current VFRONT */
    move.l  #_VFRONTREQ, a0
    move.b  (a0), d0        /* Current VFRONT (0 or 1) */

    /* Request buffer flip (toggle VFRONT) */
    eor.b   #0x01, d0       /* d0 = ~current VFRONT */
    move.b  d0, (a0)        /* Request flip */

    move.l  #_UTIMER_1MHZ, a1

    /* Poll for flip to complete */
flip_wait_1:
    move.b  (a0), d6
    cmp.b   d0, d6
    bne.s   flip_wait_1

    /* Record first 64-bit timer value */
    move.l  (a1), d1        /* Read high 32 bits into d1 */
    move.l  4(a1), d2       /* Read low 32 bits into d2 */

    /* Flip back to original */
    eor.b   #0x01, d0       /* d0 = ~current VFRONT */
    move.b  d0, (a0)        /* Request flip back */

    /* Poll for flip to complete */
flip_wait_2:
    move.b  (a0), d6
    cmp.b   d0, d6
    bne.s   flip_wait_2

    /* Record second 64-bit timer value */
    move.l  (a1), d3        /* Read high 32 bits into d3 */
    move.l  4(a1), d4       /* Read low 32 bits into d4 */

    /* Calculate delta (64-bit subtraction) */
    sub.l   d2, d4          /* d4 = d4 - d2 (low 32 bits) */
    subx.l  d1, d3          /* d3 = d3 - d1 (high 32 bits with borrow) */

    move.l #_DEBUG_REG_HI, a2
    move.l d4, (a2)

    /* Check the elapsed time is approximately 16667 */
    clrl      d1
    movel #16667, d2
    subl d2,  d4
    subxl d1, d3

    move.l #_DEBUG_REG_HI, a2
    move.l d4, (a2)

    /* Take absolute value */
    tstl   d4
    bge.s  positive_6_4
    negl   d4
    negxl  d3
positive_6_4:

    move.l #_DEBUG_REG_HI, a2
    move.l d4, (a2)

    /* Check difference is < 2 */
    clrl      d1
    moveq #1, d2
    subl d2,  d4
    subxl d1, d3
    bgt.s     fail_6_4

    /* Success */
    SET_POST(#0x08)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.w   end_test

fail_6_4:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

    /* Test 6.5: Timer Latch - Verify timer latching mechanism
     * Reads latched timer value and verifies it hasn't changed too much
     * after a fixed delay.
     */
test_6_5:
    SET_POST(#0x09)

    /* Read initial timer value from _UTIMER_1MHZ (bits 63:48, latch trigger) */
    move.l  #_UTIMER_1MHZ, a0
    move.w  (a0), d1        /* d1 = bits [63:48] (also latches all 64 bits) */

    /* Delay loop: 65536 iterations */
    move.l  #65536, d0
delay_loop_6_5:
    subq.l  #1, d0
    bne.w   delay_loop_6_5

    /* Now read all four timer registers after delay */
    move.w  (a0), d1        /* d1 = bits [63:48] */
    move.w  2(a0), d2       /* d2 = bits [47:32] (reading _UTIMER_1MHZ + 2) */
    move.w  4(a0), d3       /* d3 = bits [31:16] (reading _UTIMER_1MHZ + 4) */
    move.w  6(a0), d4       /* d4 = bits [15:0]  (reading _UTIMER_1MHZ + 6) */

    /* Construct full 64-bit time: (d1<<48)|(d2<<32)|(d3<<16)|d4 */
    /* Result will be in d0:d1 (high:low) */
    move.l  d1, d0          /* d0 = bits [63:48] in lower 16 bits */
    swap    d0              /* Swap: now bits [63:48] in upper 16 bits */

    /* Combine d2 into d0 bits [47:32] */
    move.l  d2, d7
    clr.w   d7              /* d7 = d2 << 16 */
    or.l    d7, d0          /* d0 = (d1<<48)|(d2<<32) */

    /* Put d3:d4 into d1 */
    move.l  d3, d1          /* d1 = bits [31:16] */
    swap    d1              /* Swap: bits [31:16] now in upper 16 bits */
    or.w    d4, d1          /* OR in bits [15:0] */

    /* Verify that after the delay, timer advanced */
    /* Check if low 32 bits show >0 change */
    tst.l   d1
    beq.s   fail_6_5        /* If low 32 bits are zero, timer didn't advance */

    /* Check if change is not massive (> 100K microseconds is unreasonable for this test) */
    cmp.l   #100000, d1     /* If timer changed > 100K microseconds, something's wrong */
    bgt.s   fail_6_5

    /* Timer advanced reasonably, test passes */
    SET_POST(#0x0A)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.w   end_test

fail_6_5:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

end_test:
    SET_POST(#0x3F)
    lea     all_done_msg(pc), a0
    UART_PUTS(a0)

    SHUTDOWN()

/* Detect platform type
 * Sets platform flags:
 * - is_spectrum_next: 1 if running on Spectrum Next hardware
 * - is_simulation: 1 if running in FuncVal testbench simulation
 * - is_model: 1 if running on sQLux-nextp8 software model
 */
detect_simulation:
    movem.l d0/a0, -(sp)

    /* Clear all platform flags */
    clr.b   is_spectrum_next
    clr.b   is_simulation
    clr.b   is_model

    /* Check ADDR_BUILD_TIMESTAMP_LO */
    move.l  #_BUILD_TIMESTAMP_LO, a0
    move.w  (a0), d0
    bne.w   detected_spectrum_next

    /* Check ADDR_HW_VERSION_LO */
    move.l  #(_HW_VERSION_LO + 1), a0
    move.b  (a0), d0        /* Read patch_version from (_HW_VERSION_LO + 1) */

    /* Check if hw_version_lo is in range [0xfe, 0xff] */
    /* If < 0xfe or > 0xff, it's simulation (any other value) */
    cmp.b   #0xfe, d0
    blo.s   detected_simulation  /* If < 0xfe, it's simulation */
    cmp.b   #0xff, d0
    bhi.s   detected_simulation  /* If > 0xff, it's simulation */

    /* Must be sQLux-nextp8 model (hw_version_lo is 0xfe or 0xff) */
    move.b  #1, is_model
    bra.s   detect_done

detected_spectrum_next:
    move.b  #1, is_spectrum_next
    bra.s   detect_done

detected_simulation:
    move.b  #1, is_simulation

detect_done:
    movem.l (sp)+, d0/a0
    rts

    .section .rodata
pass_msg:
    .asciz "PASS\n"

fail_msg:
    .asciz "FAIL\n"

all_done_msg:
    .asciz "All timer tests complete\n"

    .section .bss
/* Platform detection flags */
is_spectrum_next:
    .byte 0
is_simulation:
    .byte 0
is_model:
    .byte 0


