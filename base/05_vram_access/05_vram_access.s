/*
 * Test 5: VRAM Read/Write
 * Tests VRAM access and buffer flipping
 *
 * Test 5.1: Back Buffer Write and Readback
 * Test 5.2: Buffer Flip Test
 * Test 5.3: Dual Buffer Independence Test
 * Test 5.4: VRAM Address Range Test
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

    /* Test 5.1: Back Buffer Write and Readback */
test_5_1:
    SET_POST(#0x01)

    /* Write 0xBEEF to VRAM offset 0 (back buffer) */
    move.l  #_BACK_BUFFER_BASE, a0
    move.w  #0xBEEF, (a0)

    /* Read back from VRAM offset 0 */
    move.w  (a0), d0

    /* Verify = 0xBEEF */
    cmpi.w  #0xBEEF, d0
    bne.s   fail_5_1

pass_5_1:
    SET_POST(#0x02)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_5_2
fail_5_1:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)
test_5_2:
    SET_POST(#0x03)

    /* Write 0x1234 to VRAM offset 0x100 (back buffer) */
    move.l  #_BACK_BUFFER_BASE, a0
    move.w  #0x1234, 0x100(a0)

    /* Flip buffers (back becomes front) */
    bsr     flip_buffers

    /* Read from VRAM offset 0x100 (now front buffer) */
    move.l  #_FRONT_BUFFER_BASE, a0
    move.w  0x100(a0), d0

    /* Verify still = 0x1234 */
    cmpi.w  #0x1234, d0
    bne.s   fail_5_2

pass_5_2:
    SET_POST(#0x04)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_5_3
fail_5_2:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)
test_5_3:
    SET_POST(#0x05)

    /* Test dual buffer independence */
    /* Write 0xAAAA to back buffer offset 0 */
    move.l  #_BACK_BUFFER_BASE, a0
    move.w  #0xAAAA, (a0)

    /* Flip buffers (back becomes front, front becomes back) */
    bsr     flip_buffers

    /* Write 0x5555 to new back buffer offset 0 */
    move.l  #_BACK_BUFFER_BASE, a0
    move.w  #0x5555, (a0)

    /* Read from front buffer - should still be 0xAAAA */
    move.l  #_FRONT_BUFFER_BASE, a0
    move.w  (a0), d0

    /* Verify = 0xAAAA (not 0x5555) */
    cmpi.w  #0xAAAA, d0
    bne.s   fail_5_3

pass_5_3:
    SET_POST(#0x06)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra     test_5_4
fail_5_3:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)
test_5_4:
    SET_POST(#0x07)

    move.l  #_BACK_BUFFER_BASE, a0

    /* Write test pattern to first word (offset 0) */
    move.w  #0x1111, (a0)

    /* Write test pattern to middle (offset 0x1000) */
    move.w  #0x2222, 0x1000(a0)

    /* Write test pattern to last word (offset 0x1FFE) */
    move.w  #0x3333, 0x1FFE(a0)

    /* Read back all three locations */
    move.w  (a0), d0
    cmpi.w  #0x1111, d0
    bne.s   fail_5_4

    move.w  0x1000(a0), d0
    cmpi.w  #0x2222, d0
    bne.s   fail_5_4

    move.w  0x1FFE(a0), d0
    cmpi.w  #0x3333, d0
    bne.s   fail_5_4

pass_5_4:
    SET_POST(#0x08)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   all_pass
fail_5_4:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

all_pass:
    SET_POST(#0x3F)
    lea     done_msg(pc), a0
    UART_PUTS(a0)

    SHUTDOWN()

/* Subroutine: Clear both VRAM buffers */
clear_vram:
    move.l  #_BACK_BUFFER_BASE, a0
    move.w  #(_FRAME_BUFFER_SIZE/2-1), d0  /* Word count */
clear_back:
    move.w  #0x0000, (a0)+
    dbra    d0, clear_back

    move.l  #_FRONT_BUFFER_BASE, a0
    move.w  #(_FRAME_BUFFER_SIZE/2-1), d0
clear_front:
    move.w  #0x0000, (a0)+
    dbra    d0, clear_front

    rts

/* Subroutine: Flip VRAM buffers */
flip_buffers:
    movem.l d2, -(sp)
    /* Read current VFRONT register */
    move.l  #0x80001D, a0       /* ADDR_VFRONT = 0x80001D */
    move.b  (a0), d0            /* d0 = current VFRONT value (0 or 1) */

    /* Calculate new VFRONT value (toggle bit 0) */
    eor.b   #0x01, d0           /* d0 = current VFRONT XOR 1 */

    /* Write new value to VFRONTREQ to request flip */
    move.b  d0, (a0)            /* ADDR_VFRONTREQ = 0x80001D (same address, write operation) */

    /* Poll VFRONT until it matches the requested value (with timeout) */
    move.l  #1000000, d2        /* Timeout counter (large 32-bit value) */
flip_wait:
    move.b  (a0), d1            /* d1 = current VFRONT */
    cmp.b   d0, d1              /* compare with requested value */
    beq.s   flip_done           /* exit if equal */

    subq.l  #1, d2              /* decrement 32-bit timeout */
    bne.s   flip_wait           /* loop if not zero */

flip_done:
    movem.l (sp)+, d2
    rts

/* Subroutine: Send null-terminated string via UART */
    .section .rodata
pass_msg:
    .asciz  "PASS\n"
fail_msg:
    .asciz  "FAIL\n"
done_msg:
    .asciz  "VRAM Tests Complete\n"
