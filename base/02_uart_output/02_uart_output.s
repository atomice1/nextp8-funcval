/*
 * Test 2: UART Output
 * Tests UART transmit functionality
 *
 * Test 2.1: Single Character Output
 * Test 2.2: Multi-byte Output (HELLO)
 * Test 2.3: Full Line with Newline
 * Test 2.4: Character Set Test (printable ASCII)
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

    /* Configure UART baud rate (115200) */
    UART_INIT()

    /* Test 2.1: Single Character Output */
test_2_1:
    /* Write 'A' */
    UART_WRITE_BYTE(IMM(0x41))

    /* Test 2.2: Multi-byte Output "HELLO" */
test_2_2:
    lea     hello_str(pc), a3
    moveq   #5, d2          /* 5 characters */

loop_2_2:
    /* Write next character */
    move.b  (a3)+, d0
    UART_WRITE_BYTE(d0)

    subq.w  #1, d2
    bne.s   loop_2_2

    /* Test 2.3: Full Line with Newline */
test_2_3:
    lea     test_str(pc), a3
    moveq   #18, d2         /* 18 characters including \r\n */

loop_2_3:
    /* Write next character */
    move.b  (a3)+, d0
    UART_WRITE_BYTE(d0)

    subq.w  #1, d2
    bne.s   loop_2_3

    /* Test 2.4: Character Set Test (0x20-0x7E) */
test_2_4:
    moveq   #0x20, d2       /* Start with space */
    moveq   #0, d3          /* Character counter for newlines */

char_loop:
    /* Write character */
    move.b  d2, d0
    UART_WRITE_BYTE(d0)

    addq.b  #1, d2          /* Next character */
    addq.b  #1, d3          /* Increment counter */

    /* Send newline every 16 characters */
    cmpi.b  #16, d3
    bne.s   no_newline

    /* Send \r\n */
    bsr.s   send_crlf
    moveq   #0, d3          /* Reset counter */

no_newline:
    cmpi.b  #0x7F, d2       /* Check if past 0x7E */
    bne.s   char_loop

    /* Send \r\n */
    bsr.s   send_crlf

    SHUTDOWN()

/* Subroutine: Send \r\n */
send_crlf:
    /* Send \r */
    UART_WRITE_BYTE(IMM(0x0D))

    /* Send \n */
    UART_WRITE_BYTE(IMM(0x0A))
    rts

    .section .rodata
hello_str:
    .ascii  "HELLO"

test_str:
    .ascii  "nextp8 UART test\r\n"
