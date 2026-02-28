/* Main entry point for nextp8 Functional Validation Tests
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "funcval.h"
#include "nextp8.h"

/* Main entry point called from crt1.c */
int main(void)
{
    int result;

    /* Print welcome message */
    uart_puts("=== nextp8 Functional Validation Test ===");
    uart_print_crlf();

    /* Print platform name */
    uart_puts("Platform: ");
    uart_puts(platform_get_name());
    uart_print_crlf();
    uart_print_crlf();

    /* Initialize test framework */
    test_init();

    /* Run test suite - pass NULL to run all suites from linker sections */
    result = test_run_suite(NULL);

    /* Print shutdown message */
    uart_puts("Shutting down...");
    uart_print_crlf();

    return result;
}
