/* Minimal startup code for nextp8 Functional Validation Tests
 *
 * Copyright (C) 2026 Chris January
 */

#include "../include/m68k_defs.h"

    .text
    .extern __stack
    .extern __start1

    .globl _start
_start:
    /* Update POST code to indicate boot */
    move.l  #0x80000c, a0
    move.b  #4, (a0)

    /* Initialize stack */
    move.l  #__stack, sp
    move.l  #0, a6          /* fp = 0 */

    /* Push initial stack pointer and dummy return address for __start1 */
    move.l  sp, -(sp)
    move.l  a6, -(sp)       /* Dummy return address */
    jmp     __start1
