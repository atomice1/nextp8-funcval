/* Test 1: POST Code MMIO Register
 * Tests POST code register writes and GPIO routing
 */

#include "m68k_defs.h"
#include "base_macros.h"

    .section .interrupt_vector, "ax"
    .global _vectors

__interrupt_vector:
    .long 0x100000          /* Initial SP */
    .long _start            /* Initial PC */
    .fill 254, 4, 0         /* Rest of vectors */

    .section .text._start, "ax"
    .global _start

_start:
    /* Initialize stack */
    move.l  IMM(0x100000), sp

    /* Test 1.1: Single POST Code Write */
test_1_1:
    SET_POST(IMM(0x01))
    nop
    nop
    nop
    nop
    SET_POST(IMM(0x02))
    nop
    nop
    nop
    nop
    SET_POST(IMM(0x3F))

    /* Test 1.2: POST Code Increment Pattern */
test_1_2:
    move.w  IMM(0), d0

loop_1_2:
    SET_POST(d0)

    /* Short delay */
    move.w  IMM(20), d1
delay_1_2:
    dbra    d1, delay_1_2

    addq.w  IMM(1), d0
    cmp.w   IMM(64), d0
    blt.s   loop_1_2

    /* Test 1.3: POST Code Bit Pattern Test */
test_1_3:
    SET_POST(IMM(0x15))
    nop
    nop
    nop
    nop
    SET_POST(IMM(0x2A))
    nop
    nop
    nop
    nop
    SET_POST(IMM(0x0F))
    nop
    nop
    nop
    nop
    SET_POST(IMM(0x30))

    SHUTDOWN()

