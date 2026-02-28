/*
 * Test 4: SRAM Write
 * Tests SRAM write and readback
 *
 * Test 4.1: Single Word Write and Readback
 * Test 4.2: Byte Write Test
 * Test 4.3: Write Pattern Test
 * Test 4.4: Read-Modify-Write Test
 * Test 4.5: High Memory Write Test
 * Test 4.6: Boundary Condition Tests
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

    /* Test 4.1: Single Word Write and Readback */
test_4_1:
    SET_POST(#0x01)

    /* Write 0xDEAD to address 0x10000 */
    move.l  #0x10000, a0
    move.w  #0xDEAD, (a0)

    /* Read back from 0x10000 */
    move.w  (a0), d0

    /* Compare with expected value */
    cmpi.w  #0xDEAD, d0
    bne.s   fail_4_1

pass_4_1:
    SET_POST(#0x02)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_4_2

fail_4_1:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

    /* Test 4.2: Byte Write Test */
test_4_2:
    SET_POST(#0x03)

    /* Write byte 0x12 to 0x20000 (upper byte) */
    move.l  #0x20000, a0
    move.b  #0x12, (a0)

    /* Write byte 0x34 to 0x20001 (lower byte) */
    move.b  #0x34, 1(a0)

    /* Read word from 0x20000 */
    move.w  (a0), d0

    /* Verify = 0x1234 */
    cmpi.w  #0x1234, d0
    bne.s   fail_4_2

pass_4_2:
    SET_POST(#0x04)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_4_3

fail_4_2:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

    /* Test 4.3: Write Pattern Test */
test_4_3:
    SET_POST(#0x05)

    /* Write pattern: 0xAAAA, 0x5555... (128 words) */
    move.l  #0x30000, a0
    move.w  #127, d2        /* Counter for 128 words */

write_pattern:
    move.w  #0xAAAA, (a0)+
    move.w  #0x5555, (a0)+
    dbra    d2, write_pattern

    /* Read back and verify */
    move.l  #0x30000, a0
    move.w  #127, d2

verify_pattern:
    move.w  (a0)+, d0
    cmpi.w  #0xAAAA, d0
    bne.s   fail_4_3

    move.w  (a0)+, d0
    cmpi.w  #0x5555, d0
    bne.s   fail_4_3

    dbra    d2, verify_pattern

pass_4_3:
    SET_POST(#0x06)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_4_4

fail_4_3:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

    /* Test 4.4: Read-Modify-Write Test */
test_4_4:
    SET_POST(#0x07)

    /* Initialize location to 0 */
    move.l  #0x40000, a0
    move.w  #0x0000, (a0)

    /* Perform 4 RMW cycles */
    moveq   #3, d2          /* Loop 4 times */

rmw_loop:
    move.w  (a0), d0        /* Read */
    addi.w  #0x1111, d0     /* Modify */
    move.w  d0, (a0)        /* Write */

    dbra    d2, rmw_loop

    /* Verify final value = 0x4444 */
    move.w  (a0), d0
    cmpi.w  #0x4444, d0
    bne.s   fail_4_4

pass_4_4:
    SET_POST(#0x08)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_4_5

fail_4_4:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

    /* Test 4.5: High Memory Write Test */
test_4_5:
    SET_POST(#0x09)

    /* Write 0x5678 to address 0x2FFFFE (near top of SRAM) */
    move.l  #0x2FFFFE, a0
    move.w  #0x5678, (a0)

    /* Read back from 0x2FFFFE */
    move.w  (a0), d0

    /* Verify = 0x5678 */
    cmpi.w  #0x5678, d0
    bne.s   fail_4_5

pass_4_5:
    SET_POST(#0x0A)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_4_6

fail_4_5:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

    /* Test 4.6: Boundary Condition Tests */
test_4_6:
    SET_POST(#0x0B)

    /* Write/read at 0x0FFFFE (1MB boundary - 2) */
    move.l  #0x0FFFFE, a0
    move.w  #0x1111, (a0)
    move.w  (a0), d0
    cmpi.w  #0x1111, d0
    bne.s   fail_4_6

    /* Write/read at 0x100000 (1MB boundary) */
    move.l  #0x100000, a0
    move.w  #0x2222, (a0)
    move.w  (a0), d0
    cmpi.w  #0x2222, d0
    bne.s   fail_4_6

    /* Write/read at 0x1FFFFE (2MB boundary - 2) */
    move.l  #0x1FFFFE, a0
    move.w  #0x3333, (a0)
    move.w  (a0), d0
    cmpi.w  #0x3333, d0
    bne.s   fail_4_6

    /* Write/read at 0x200000 (2MB boundary) */
    move.l  #0x200000, a0
    move.w  #0x4444, (a0)
    move.w  (a0), d0
    cmpi.w  #0x4444, d0
    bne.s   fail_4_6

pass_4_6:
    SET_POST(#0x0C)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   all_pass

fail_4_6:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

all_pass:
    SET_POST(#0x3F)
    lea     done_msg(pc), a0
    UART_PUTS(a0)

    SHUTDOWN()

    .section .rodata
pass_msg:
    .asciz  "PASS\n"
fail_msg:
    .asciz  "FAIL\n"
done_msg:
    .asciz  "SRAM Write Tests Complete\n"
