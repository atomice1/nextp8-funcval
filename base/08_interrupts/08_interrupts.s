/* Test 8: 60Hz Interrupt Tests
 *
 * Test 8.1: Interrupt Occurrence - verify ~60 interrupts in 1 second
 * Test 8.2: Timing Accuracy - measure time between interrupts
 *
 * Uses VBlank interrupt from p8video module (60Hz).
 */

#include "m68k_defs.h"
#include "base_macros.h"

/* Interrupt counter (BSS) */
.section .bss
.align 4
interrupt_count:
    .long 0
cycle_count:
    .long 0
is_spectrum_next:
    .byte 0
is_simulation:
    .byte 0
is_model:
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

    /* Detect platform type */
    bsr     detect_simulation

    /* Run all interrupt tests */
    bsr     test_8_1

    /* Skip timing-dependent tests on model (not cycle-accurate) */
    tst.b   is_model
    bne.s   .skip_timing_tests

    bsr     test_8_2

.skip_timing_tests:

    /* All tests complete */
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
    bne.s   detected_spectrum_next

    /* Check ADDR_HW_VERSION_LO  */
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

/* Test 8.1: Interrupt Occurrence Test
 * Count interrupts for 40ms
 * Expected count: 2 (60Hz ± margin)
 */
test_8_1:
    movem.l d2-d4, -(sp)
    SET_POST(#0x01)

    /* Clear interrupt counter */
    clr.l   interrupt_count

    /* Enable VBlank interrupt */
    SET_POST(#0x2)
    move.l  #_VBLANK_INTR_CTRL, a0
    move.b  #0x0001, (a0)          /* Enable bit 0 */
    SET_POST(#0x3)

    /* Enable CPU interrupts (level 2) */
    move.w  #0x2000, sr            /* Clear interrupt mask */
    SET_POST(#0x4)

    /* Wait 33 ms using 1 MHz user timer (33,000 ticks) */
    move.l  #_UTIMER_1MHZ, a0
    move.l  (a0), d0        /* Latch timer, read high 32 bits */
    move.l  4(a0), d1       /* Read low 32 bits */
wait_33ms:
    move.l  (a0), d2        /* Latch timer, read high 32 bits */
    move.l  4(a0), d3       /* Read low 32 bits */
    move.l  d3, d4
    sub.l   d1, d4
    cmp.l   #33000, d4
    blt.w   wait_33ms
    SET_POST(#0x5)

    /* Disable interrupts */
    move.w  #0x2700, sr            /* Mask all interrupts */
    SET_POST(#0x6)

    /* Disable VBlank interrupt */
    move.l  #_VBLANK_INTR_CTRL, a0
    move.b  #0x0000, (a0)
    SET_POST(#0x7)

    /* Report count via UART */
    lea     test_8_1_msg(pc), a0
    UART_PUTS(a0)
    move.l  interrupt_count, d0
    bsr     uart_put_decimal
    lea     crlf(pc), a0
    UART_PUTS(a0)

    /* Check count: model expects 1-120, others expect exactly 2 */
    move.l  interrupt_count, d0
    tst.b   is_model
    beq.s   test_8_1_range_check
    cmp.l   #1, d0
    blt.s   test_8_1_fail
    cmp.l   #120, d0
    bgt.s   test_8_1_fail
    bra.s   test_8_1_pass

test_8_1_range_check:
    cmp.l   #2, d0
    blt.s   test_8_1_fail
    cmp.l   #2, d0
    bgt.s   test_8_1_fail

test_8_1_pass:

    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    SET_POST(#0x02)
    movem.l (sp)+, d2-d4
    rts

test_8_1_fail:
    lea     fail_msg(pc), a0
    UART_PUTS(a0)
    SET_POST(#0x3F)
    movem.l (sp)+, d2-d4
    rts

/* Test 8.2: Interrupt Timing Accuracy Test
 * Measure time between interrupts using the 1 MHz timer
 * Expected: 16665-16667 ticks (60Hz)
 */
test_8_2:
    SET_POST(#0x03)

    /* Skip timing-dependent test on model */
    tst.b   is_model
    bne.w   test_8_2_pass

    /* Clear interrupt counter and cycle count */
    clr.l   interrupt_count
    clr.l   cycle_count

    /* Enable VBlank interrupt */
    move.l  #_VBLANK_INTR_CTRL, a0
    move.b  #0x0001, (a0)

    /* Enable CPU interrupts */
    move.w  #0x2000, sr

    /* Wait for first interrupt to sync */
wait_first:
    tst.l   interrupt_count
    beq.s   wait_first

    /* Clear counter, start measuring */
    clr.l   interrupt_count
    move.l  #_UTIMER_1MHZ, a1
    move.l  (a1), d0        /* Latch timer, read high 32 bits */
    move.l  4(a1), d1       /* Read low 32 bits */

    /* Wait for next interrupt */
wait_next:
    tst.l   interrupt_count
    beq.s   wait_next

    /* Read timer again */
    move.l  (a1), d2        /* Latch timer, read high 32 bits */
    move.l  4(a1), d3       /* Read low 32 bits */

    /* Store delta (low 32 bits are enough for 16.6 ms) */
    move.l  d3, d4
    sub.l   d1, d4
    move.l  d4, cycle_count

    /* Disable interrupts */
    move.w  #0x2700, sr
    move.l  #_VBLANK_INTR_CTRL, a0
    move.b  #0x0000, (a0)

    /* Report tick count */
    lea     test_8_2_msg(pc), a0
    UART_PUTS(a0)
    move.l  cycle_count, d0
    bsr     uart_put_decimal
    lea     crlf(pc), a0
    UART_PUTS(a0)

    /* Model: only require interrupts to occur */
    tst.b   is_model
    bne.s   test_8_2_pass

    /* Check if count is between 16665 and 16667 ticks */
    move.l  cycle_count, d0
    cmp.l   #16665, d0
    blo.s   test_8_2_fail
    cmp.l   #16667, d0
    bhi.s   test_8_2_fail

test_8_2_pass:

    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    SET_POST(#0x04)
    rts

test_8_2_fail:
    lea     fail_msg(pc), a0
    UART_PUTS(a0)
    SET_POST(#0x3F)
    rts

/* VBlank interrupt handler (Level 2 autovector) */
vblank_handler:
    movem.l d0-d7/a0-a6, -(sp)

    /* Increment interrupt counter */
    SET_POST(#0x11)
    addq.l  #1, interrupt_count
    /* Acknowledge VBlank interrupt */
    move.l  #_VBLANK_INTR_CTRL, a0
    move.b  #0x0001, (a0)          /* Write bit 0 to acknowledge */
    SET_POST(#0x12)

    movem.l (sp)+, d0-d7/a0-a6
    rte

/* Default exception handler */
default_handler:
    SET_POST(#0x3F)
    bra.s   default_handler


/* Output decimal number to UART
 * Input: d0.l = number
 */
uart_put_decimal:
    movem.l d0-d4/a0, -(sp)

    move.l  d0, d1                 /* working value */
    moveq   #0, d2                 /* printed flag */

    move.l  #1000000000, d4
    bsr     print_digit
    move.l  #100000000, d4
    bsr     print_digit
    move.l  #10000000, d4
    bsr     print_digit
    move.l  #1000000, d4
    bsr     print_digit
    move.l  #100000, d4
    bsr     print_digit
    move.l  #10000, d4
    bsr     print_digit
    move.l  #1000, d4
    bsr     print_digit
    move.l  #100, d4
    bsr     print_digit
    move.l  #10, d4
    bsr     print_digit
    move.l  #1, d4
    bsr     print_digit

    movem.l (sp)+, d0-d4/a0
    rts

print_digit:
    movem.l d2-d4, -(sp)
    moveq   #0, d3
print_digit_loop:
    cmp.l   d4, d1
    blt.s   print_digit_done
    sub.l   d4, d1
    addq.b  #1, d3
    bra.s   print_digit_loop

print_digit_done:
    tst.b   d2
    bne.s   print_digit_emit
    tst.b   d3
    bne.s   print_digit_emit
    cmp.l   #1, d4
    bne.s   print_digit_ret

print_digit_emit:
    add.b   #'0', d3
    move.b  d3, d0
    bsr     uart_putc
    moveq   #1, d2

print_digit_ret:
    movem.l (sp)+, d2-d4
    rts

/* Send single character to UART */
uart_putc:
    movem.l a0/d0/d1, -(sp)
    UART_WRITE_BYTE(d0)
    movem.l (sp)+, a0/d0/d1
    rts

.section .rodata
.align 2

test_8_1_msg:
    .asciz "Test 8.1: Interrupt count (expect 2 exactly): "

test_8_2_msg:
    .asciz "Test 8.2: Interval ticks (expect 16665-16667): "

pass_msg:
    .asciz "PASS\n"

fail_msg:
    .asciz "FAIL\n"

crlf:
    .asciz "\n"

all_done_msg:
    .asciz "All interrupt tests complete\n"
