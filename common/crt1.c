/* C Runtime Initialization for nextp8 Functional Validation Tests
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include <string.h>
#include "nextp8.h"

#define STACK_SIZE (8 << 10) /* 8KB */

extern const int __interrupt_vector[];
extern void __reset (void);

extern const char __data_load[] __attribute__ ((aligned (4)));
extern char __data_start[] __attribute__ ((aligned (4)));
extern char __bss_start[] __attribute__ ((aligned (4)));
extern char __end[] __attribute__ ((aligned (4)));
char *__initial_stack;
void *__heap_limit;

extern void hardware_init_hook (void) __attribute__ ((weak));
extern void software_init_hook (void) __attribute__ ((weak));

extern int main (void);

/* Forward declarations for weak hook functions */
void uart_init(void);
void platform_detect(void);

/* Set POST code */
static inline void set_postcode(uint8_t code) {
    *(volatile uint8_t *)_POST_CODE = code;
}

/* This is called from crt0.S */
void __start1 (char *initial_stack)
{
    int result;

    set_postcode(5);

    if (hardware_init_hook)
        hardware_init_hook ();

    set_postcode(6);

    /* Initialize memory */
    if ((char *)__data_load != (char *)__data_start)
        memcpy (__data_start, __data_load, __bss_start - __data_start);
    memset (__bss_start, 0, __end - __bss_start);

    __initial_stack = initial_stack;
    __heap_limit = initial_stack - STACK_SIZE;

    set_postcode(7);

    if (software_init_hook)
        software_init_hook ();

    set_postcode(8);

    /* Call main function */
    result = main();

    set_postcode(result ? 0x3F : 0x3E); /* FAIL : SUCCESS */

    /* Shutdown */
    *(volatile uint8_t *)_RESET_REQ = 0xFF;

    /* Halt (should not reach here) */
    while (1);
}

/* Weak default hardware init hook */
void __attribute__ ((weak)) hardware_init_hook (void)
{
    /* Force reference to __interrupt_vector */
    (void)__interrupt_vector;

    /* Initialize UART for output */
    uart_init();
}

void __attribute__ ((weak)) software_init_hook (void)
{
    /* Detect platform */
    platform_detect();
}