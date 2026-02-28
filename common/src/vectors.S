/*
 * M68000 Interrupt Vector Table
 * Defines the complete 256-entry exception vector table
 *
 * This file is compiled and added to libfuncval.a
 * Provides the interrupt_vectors symbol
 */

    .extern _start
    .extern watchdog_vblank_handler

    .section .interrupt_vector, "ax"
    .global __interrupt_vector

__interrupt_vector:
    /* 0: Initial Stack Pointer */
    .long   0x100000

    /* 1: Initial Program Counter */
    .long   _start

    /* 2: Bus Error */
    .long   default_exception_handler

    /* 3: Address Error */
    .long   default_exception_handler

    /* 4: Illegal Instruction */
    .long   default_exception_handler

    /* 5: Zero Divide */
    .long   default_exception_handler

    /* 6: CHK Instruction */
    .long   default_exception_handler

    /* 7: TRAPV Instruction */
    .long   default_exception_handler

    /* 8: Privilege Violation */
    .long   default_exception_handler

    /* 9: Trace */
    .long   default_exception_handler

    /* 10: Line 1010 Emulator */
    .long   default_exception_handler

    /* 11: Line 1111 Emulator */
    .long   default_exception_handler

    /* 12-14: Reserved */
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler

    /* 15: Uninitialized Interrupt */
    .long   default_exception_handler

    /* 16-23: Reserved */
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler

    /* 24: Spurious Interrupt */
    .long   default_exception_handler

    /* 25-31: Level 1-7 Autovector Interrupts */
    .long   default_exception_handler   /* Level 1 */
    .long   watchdog_vblank_handler     /* Level 2 - VBlank watchdog */
    .long   default_exception_handler   /* Level 3 */
    .long   default_exception_handler   /* Level 4 */
    .long   default_exception_handler   /* Level 5 */
    .long   default_exception_handler   /* Level 6 */
    .long   default_exception_handler   /* Level 7 (NMI) */

    /* 32-47: TRAP #0-15 Instructions */
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler

    /* 48-63: Reserved */
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler
    .long   default_exception_handler

    /* 64-255: User Interrupt Vectors */
    .rept   192
    .long   default_exception_handler
    .endr

    .section .text

/*
 * default_exception_handler - Default handler for unhandled exceptions
 * Prints exception information and halts
 */
    .global default_exception_handler
default_exception_handler:
    /* TODO: Print exception information */
    /* For now, just halt */
    stop    #0x2700
    bra.s   default_exception_handler
