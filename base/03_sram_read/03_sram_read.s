/*
 * Test 3: SRAM Read
 * Tests non-prefetch SRAM reads
 *
 * Test 3.1: Single Word Read from Low Memory
 * Test 3.2: Byte Read Test
 * Test 3.3: Sequential Read Test
 * Test 3.4: High Memory Read
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

    /* Test 3.1: Single Word Read from Low Memory */
test_3_1:
    /* POST code 0x01 (test start) */
    SET_POST(#0x01)

    /* Read word from ROM test data */
    move.l  #sram_word_addr, a0
    move.w  (a0), d0

    /* Compare with expected value 0x1234 */
    cmpi.w  #0x1234, d0
    bne.s   fail_3_1

pass_3_1:
    SET_POST(#0x02)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_3_2

fail_3_1:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)
    bra.w   shutdown

    /* Test 3.2: Byte Read Test */
test_3_2:
    SET_POST(#0x03)

    /* Read upper byte from ROM test data (should be 0xAB) */
    move.l  #sram_bytes_addr, a0
    move.b  (a0), d0
    cmpi.b  #0xAB, d0
    bne.s   fail_3_2

    /* Read lower byte from 0x3001 (should be 0xCD) */
    move.b  1(a0), d1
    cmpi.b  #0xCD, d1
    bne.s   fail_3_2

pass_3_2:
    SET_POST(#0x04)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_3_3

fail_3_2:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)
    bra.w   shutdown

    /* Test 3.3: Sequential Read Test */
test_3_3:
    SET_POST(#0x05)

    /* Read 4 words starting at ROM test data */
    move.l  #sram_seq_addr, a0
    move.w  (a0)+, d0
    cmpi.w  #0x0001, d0
    bne.s   fail_3_3

    move.w  (a0)+, d0
    cmpi.w  #0x0002, d0
    bne.s   fail_3_3

    move.w  (a0)+, d0
    cmpi.w  #0x0003, d0
    bne.s   fail_3_3

    move.w  (a0)+, d0
    cmpi.w  #0x0004, d0
    bne.s   fail_3_3

pass_3_3:
    SET_POST(#0x06)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   test_3_4

fail_3_3:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)
    bra.w   shutdown

    /* Test 3.4: ROM Region Boundary Read */
test_3_4:
    SET_POST(#0x07)

    /* Read from near end of ROM region (256KB boundary) */
    move.l  #sram_boundary_addr, a0
    move.w  (a0), d0
    cmpi.w  #0x5678, d0
    bne.s   fail_3_4

pass_3_4:
    SET_POST(#0x08)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   all_pass

fail_3_4:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)
    bra.w   shutdown

all_pass:
    SET_POST(#0x3F)
    lea     done_msg(pc), a0
    UART_PUTS(a0)

shutdown:
    SHUTDOWN()

    .section .rodata
pass_msg:
    .asciz  "PASS\n"
fail_msg:
    .asciz  "FAIL\n"
done_msg:
    .asciz  "SRAM Read Tests Complete\n"

    /* ROM test data for read tests (all below 256KB boundary)
     * ROM base is 0x4000, so .org uses (addr - 0x4000) offsets.
     */
    .section .rom_data, "a"
    .org    0x3B000
sram_word_addr:
    .word   0x1234

    .org    0x3B100
sram_bytes_addr:
    .byte   0xAB, 0xCD

    .org    0x3B200
sram_seq_addr:
    .word   0x0001, 0x0002, 0x0003, 0x0004

    .org    0x3BFFE
sram_boundary_addr:
    .word   0x5678
