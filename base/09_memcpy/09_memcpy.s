/*
 * Test 9: Memcpy Performance Test
 * Measures time to copy a buffer using tg68k's move.l instructions
 *
 * Uses the 1MHz UTIMER to measure copy time and reports microseconds per KB
 */

#include "m68k_defs.h"
#include "nextp8.h"

/* Override UART status constants with numeric values to avoid preprocessor
 * outputting un-evaluated C expressions like (1 << 1) which the assembler
 * cannot parse. */
#undef _UART_STATUS_READY
#undef _UART_STATUS_WRITE_ACK
#undef _UART_CTRL_WRITE_STROBE

#define _UART_STATUS_READY         2         /* (1 << 1) */
#define _UART_STATUS_WRITE_ACK     8         /* (1 << 3) */
#define _UART_CTRL_WRITE_STROBE    1         /* (1 << 0) */

/* Memcpy size in bytes (must be multiple of 4 for move.l loop) */
#define MEMCPY_SIZE               4096

/* Derived calculations */
#define MEMCPY_WORDS               (MEMCPY_SIZE / 4)
#define MEMCPY_KB                  (MEMCPY_SIZE / 1024)

/*
#define DEBUG(sentinel, reg) \
	movem.l d7/a0, -(sp); \
	move.l  #_DEBUG_REG_HI, a0; \
	move.l  sentinel, d7; \
	move.l  d7, (a0); \
	move.l  reg, (a0); \
	movem.l (sp)+, d7/a0
	*/
#define DEBUG(sentinel, reg)

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

    /* Test 9.1: MEMCPY_SIZE memcpy timing (smaller for faster testing) */
test_9_1:
    SET_POST(#0x01)

    /* Set up source buffer at 0x100000 (start of SRAM) */
    lea     src_buffer, a0
    move.l  #0x100000, a1

    /* Set up destination buffer at 0x110000 (16KB after source) */
    move.l  #0x110000, a2

    /* Read 64-bit timer before copy */
    move.l  #_UTIMER_1MHZ, a3
    move.l  (a3), d7        /* high 32 bits before */
    move.l  4(a3), d6       /* low 32 bits before */

    /* Copy MEMCPY_SIZE bytes (MEMCPY_WORDS × 4-byte words) using move.l (a0)+,(a1)+ */
    /* Use a simple loop with MEMCPY_WORDS iterations */
    move.l  #MEMCPY_WORDS, d0
copy_loop:
    move.l  (a1)+, (a2)+
    subq.l  #1, d0
    bne.s   copy_loop

    /* Read 64-bit timer after copy */
    move.l  (a3), d5        /* high 32 bits after */
    move.l  4(a3), d4       /* low 32 bits after */

    /* Calculate elapsed time (64-bit subtraction) */
    /* delta = new - old */
    sub.l   d6, d4          /* low 32 bits: d4 = d4 - d6 */
    subx.l  d7, d5          /* high 32 bits: d5 = d5 - d7 - borrow */

    /* Result is in d5:d4 (high:low) */
    /* We'll report low 32 bits (microseconds) */

    /* Check if timer advanced */
    tst.l   d4
    beq     fail_9_1        /* If zero, timer didn't advance */

    /* Print "memcpy XKiB time: Y us, Z MB/s" */
    /* where Z = (MEMCPY_SIZE * 1000000) / delta / 1000000 = MEMCPY_SIZE / delta MB/s */
    /* For integer math: Z = (MEMCPY_SIZE * 1000) / delta (KB/ms = MB/s) */

    lea     memcpy_msg(pc), a0
    UART_PUTS(a0)

    /* Print MEMCPY_KB (X) */
    move.l  #MEMCPY_KB, d0
    bsr     print_uint32

    lea     kib_msg(pc), a0
    UART_PUTS(a0)

    /* Print the 32-bit delta value in decimal (time in microseconds) */
    move.l  d4, d0          /* Move delta from d4 to d0 for print_uint32 */
    DEBUG(#0x11111111, d0)
    bsr     print_uint32

    lea     us_msg(pc), a0
    UART_PUTS(a0)

    lea     comma_msg(pc), a0
    UART_PUTS(a0)

    /* Calculate MB/s = MEMCPY_SIZE / delta */
    /* Since 1 byte/µs = 1 MB/s numerically */
    move.l  #MEMCPY_SIZE, d0
    divu    d4, d0          /* d0 = MEMCPY_SIZE / delta (quotient in low 16 bits) */
    /* Extract quotient from divu result */
    swap    d0              /* swap: moves low 16 to high 16, high 16 to low 16 */
    and.l   #0xffff0000, d0 /* mask: keep only high 16 bits (the quotient) */
    swap    d0              /* swap back: quotient now in low 16 bits */
    bsr     print_uint32

    lea     mbs_msg(pc), a0
    UART_PUTS(a0)

    lea     comma_msg(pc), a0
    UART_PUTS(a0)

    /* Calculate cycles/byte = (delta * 40) / MEMCPY_SIZE */
    /* delta is in microseconds, CPU is 40MHz (40 cycles/µs) */
    /* cycles/byte = (delta µs * 40 cycles/µs) / MEMCPY_SIZE bytes */
    move.l  d4, d0          /* delta in d0 */
    lsl.l   #2, d0          /* multiply by 4 (delta * 4) */
    lsl.l   #3, d0          /* multiply by 8 (delta * 32) */
    /* combine: delta * 40 = delta * (32 + 8) = (delta * 32) + (delta * 8) */
    move.l  d4, d1
    lsl.l   #3, d1          /* d1 = delta * 8 */
    add.l   d1, d0          /* d0 = delta * 40 */
    divu    #MEMCPY_SIZE, d0/* d0 = (delta * 40) / MEMCPY_SIZE */
    /* Extract quotient from divu result */
    swap    d0
    and.l   #0xffff0000, d0
    swap    d0
    bsr     print_uint32

    lea     cycles_per_byte_msg(pc), a0
    UART_PUTS(a0)

    lea     newline_msg(pc), a0
    UART_PUTS(a0)

pass_9_1:
    SET_POST(#0x02)
    lea     pass_msg(pc), a0
    UART_PUTS(a0)
    bra.s   all_pass

fail_9_1:
    SET_POST(#0x3F)
    lea     fail_msg(pc), a0
    UART_PUTS(a0)

all_pass:
    SET_POST(#0x3F)
    lea     done_msg(pc), a0
    UART_PUTS(a0)
    SHUTDOWN()

/* Print a 32-bit unsigned integer in decimal */
/* Input: d0 = number to print */
print_uint32:
    movem.l d1-d2/a0, -(sp)
    DEBUG(#0x01010101, d0)

    /* Handle zero case */
    tst.l   d0
    bne.s   not_zero_32
    move.b  #'0', d0
    UART_WRITE_BYTE(d0)
    bra.s   print_done_32

not_zero_32:
    /* Print digits from most significant to least */
    move.l  #1000000000, d2
    bsr     print_digit
    move.l  #100000000, d2
    bsr     print_digit
    move.l  #10000000, d2
    bsr     print_digit
    move.l  #1000000, d2
    bsr     print_digit
    move.l  #100000, d2
    bsr     print_digit
    move.l  #10000, d2
    bsr     print_digit
    move.l  #1000, d2
    bsr     print_digit
    move.l  #100, d2
    bsr     print_digit
    move.l  #10, d2
    bsr     print_digit
    move.l  #1, d2
    bsr     print_digit

print_done_32:
    movem.l (sp)+, d1-d2/a0
    rts

/* Print a single digit */
/* Input: d0 = current value, d2 = divisor */
print_digit:
    movem.l d1/d2, -(sp)
    DEBUG(#0x22222222, d0)
    DEBUG(#0x2f2f2f2f, d2)
    moveq   #0, d1
print_digit_loop:
    cmp.l   d2, d0
    blt.s   print_digit_done
    sub.l   d2, d0
    addq.l  #1, d1
    DEBUG(#0x33333333, d0)
    DEBUG(#0x44444444, d1)
    DEBUG(#0x4f4f4f4f, d2)
    bra.s   print_digit_loop

print_digit_done:
    DEBUG(#0x55555555, d0)
    tst.l   d1
    bne.s   print_digit_emit
    cmp.l   #1, d2
    bne.s   print_digit_ret

print_digit_emit:
    move.l  d0, d2
    add.b   #'0', d1
    move.b  d1, d0
    DEBUG(#0x66666666, d0)
    UART_WRITE_BYTE(d0)
    move.l  d2, d0

print_digit_ret:
    movem.l (sp)+, d1/d2
    rts

    .section .bss
    .align  4

/* Source buffer - must be at least MEMCPY_SIZE bytes */
src_buffer:
    .space  MEMCPY_SIZE

    .section .rodata

/* Messages */
memcpy_msg:
    .asciz  "memcpy "
kib_msg:
    .asciz  "KiB time: "
us_msg:
    .asciz  " us"
comma_msg:
    .asciz  ", "
mbs_msg:
    .asciz  " MB/s"
cycles_per_byte_msg:
    .asciz  " cycles/byte"
newline_msg:
    .asciz  "\n"
pass_msg:
    .asciz  "PASS\n"
fail_msg:
    .asciz  "FAIL\n"
done_msg:
    .asciz  "Memcpy test complete\n"
