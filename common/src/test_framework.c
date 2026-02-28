/* Test Framework for nextp8 Functional Validation Library
 * Provides test runner infrastructure with pass/fail tracking
 *
 * Copyright (C) 2026 Chris January
 *
 */

#include <stdint.h>
#include <setjmp.h>
#include "nextp8.h"
#include "funcval.h"

/* Test statistics */
uint32_t test_total_count = 0;
uint32_t test_pass_count = 0;
uint32_t test_fail_count = 0;
uint32_t test_skip_count = 0;
uint32_t test_xfail_count = 0;
uint32_t test_xpass_count = 0;
uint32_t test_timeout_count = 0;

/* Current test number */
static uint32_t test_current = 0;

/* Jump buffer for timeout handling */
static jmp_buf test_timeout_jmpbuf;
static volatile uint8_t test_timeout_armed = 0;

/* Linker-provided test suite section symbols */
extern struct test_suite __test_suite_start[];
extern struct test_suite __test_suite_end[];

/* Watchdog default timeout (10 seconds in microseconds) */
uint32_t test_watchdog_timeout = 10000000;
void screen_putchar(char c);
void screen_print_dec(uint32_t value);
void screen_flip(void);

void watchdog_start(uint32_t timeout_us, void (*handler)(void));
void watchdog_stop(void);

/* Forward declarations */
static void test_timeout_handler(void);

/* test_init - Initialize test framework
 * Resets all counters
 */
void test_init(void)
{
    test_total_count = 0;
    test_pass_count = 0;
    test_fail_count = 0;
    test_skip_count = 0;
    test_xfail_count = 0;
    test_xpass_count = 0;
    test_timeout_count = 0;
    test_current = 0;
    test_timeout_armed = 0;
}

/* test_puts - Print string to UART and screen (if not simulation) */
void test_puts(const char *str)
{
    /* Always print to UART */
    uart_puts(str);

    /* Check if we should also print to screen */
    if (platform_is_model || platform_is_spectrum_next) {
        screen_puts(str);
    }
}

/* test_print_crlf - Print newline to UART and screen (if not simulation) */
void test_print_crlf(void)
{
    /* Always print to UART */
    uart_print_crlf();

    /* Check if we should also print to screen */
    if (platform_is_model || platform_is_spectrum_next) {
        screen_putchar('\n');
    }
}

/* test_print_dec - Print decimal number to UART and screen (if not simulation) */
void test_print_dec(uint32_t value)
{
    /* Always print to UART */
    uart_print_dec(value);

    /* Check if we should also print to screen */
    if (platform_is_model || platform_is_spectrum_next) {
        screen_print_dec(value);
    }
}

/* test_print_hex_long - Print 32-bit hex to UART (no screen output for hex) */
void test_print_hex_long(uint32_t value)
{
    uart_print_hex_long(value);

    /* Check if we should also print to screen */
    if (platform_is_model || platform_is_spectrum_next) {
        screen_print_hex_long(value);
    }
}

/* test_print_hex_word - Print 16-bit hex to UART (no screen output for hex) */
void test_print_hex_word(uint16_t value)
{
    uart_print_hex_word(value);

    /* Check if we should also print to screen */
    if (platform_is_model || platform_is_spectrum_next) {
        screen_print_hex_word(value);
    }
}

/* test_print_summary - Print test results summary */
void test_print_summary(void)
{
    test_print_crlf();
    test_puts("Test Summary");
    test_print_crlf();
    test_puts("===============================");
    test_print_crlf();

    /* Total */
    test_puts("Total tests:       ");
    test_print_dec(test_total_count);
    test_print_crlf();

    /* Passed */
    test_puts("Passed:            ");
    test_print_dec(test_pass_count);
    test_print_crlf();

    /* Failed */
    if (test_fail_count) {
        test_puts("Failed:            ");
        test_print_dec(test_fail_count);
        test_print_crlf();
    }

    /* Timeouts */
    if (test_timeout_count) {
        test_puts("Timed out:         ");
        test_print_dec(test_timeout_count);
        test_print_crlf();
    }

    /* Skipped */
    if (test_skip_count) {
        test_puts("Skipped:           ");
        test_print_dec(test_skip_count);
        test_print_crlf();
    }

    /* Expected failures */
    if (test_xfail_count) {
        test_puts("Expected failures: ");
        test_print_dec(test_xfail_count);
        test_print_crlf();
    }

    /* Unexpected passes */
    if (test_xpass_count) {
        test_puts("Unexpected passes: ");
        test_print_dec(test_xpass_count);
        test_print_crlf();
    }

    test_puts("===============================");
    test_print_crlf();

    /* Overall result */
    if (test_fail_count || test_timeout_count || test_xpass_count) {
        test_puts("OVERALL: FAIL");
    } else {
        test_puts("OVERALL: PASS");
    }
    test_print_crlf();
    test_print_crlf();

    /* Flip screen buffer to show summary */
    if (platform_is_model || platform_is_spectrum_next) {
        screen_flip();
    }
}

/* test_run_fn_with_timeout - Run a test function with timeout handling
 * Uses setjmp/longjmp to handle timeouts via the watchdog
 *
 * Args:
 *   fn: pointer to test function to run (returns int status code)
 *
 * Returns:
 *   Result code from test function, or TEST_TIMEOUT if timed out
 */
static int test_run_fn_with_timeout(int (*fn)(void))
{
    /* Set up timeout handling with setjmp/longjmp */
    int result;
    if (setjmp(test_timeout_jmpbuf) == 0) {
        /* First time through - run the test */
        test_timeout_armed = 1;
        watchdog_start(test_watchdog_timeout, test_timeout_handler);

        /* Call test function */
        result = fn();

        /* Test completed normally */
        test_timeout_armed = 0;
        watchdog_stop();
    } else {
        /* Returned via longjmp from timeout handler */
        test_timeout_armed = 0;
        watchdog_stop();
        result = TEST_TIMEOUT;
    }
    return result;
}

/* test_run_suite - Run test suites
 * Iterates through test suites and their test cases
 *
 * If specific_suite is NULL, runs all test_suite structures from linker sections.
 * If specific_suite is provided, runs only that single test suite.
 *
 * For each suite, iterates through all test cases until NULL function seen
 *
 * Args:
 *   specific_suite: pointer to specific test_suite to run, or NULL to run all
 *
 * Returns:
 *   0 if all tests passed, -1 if any failed
 */
int test_run_suite(struct test_suite *specific_suite)
{
    /* Get test suite bounds from linker */
    struct test_suite *suite_ptr = __test_suite_start;
    struct test_suite *suite_end = __test_suite_end;

    /* If specific suite requested, run only that one */
    if (specific_suite != NULL) {
        suite_ptr = specific_suite;
        suite_end = specific_suite + 1;
    }

    /* Print header */
    test_puts("=== Test Suite ===");
    test_print_crlf();
    test_print_crlf();

    /* Flip screen to show header */
    if (platform_is_model || platform_is_spectrum_next) {
        screen_flip();
    }

    /* Outer loop: iterate through test suites */
    while (suite_ptr < suite_end) {
        test_puts("Suite: ");
        test_puts(suite_ptr->name);
        test_print_crlf();
        test_print_crlf();

        /* Call suite setup function if provided */
        if (suite_ptr->setup) {
            suite_ptr->setup();
        }

        /* Inner loop: iterate through test cases in this suite */
        struct test_case *test_case = suite_ptr->test_cases;
        while (test_case->fn != NULL) {
            /* Increment test number */
            test_current++;
            test_total_count++;

            /* Print test number and name */
            test_puts("Test ");
            test_print_dec(test_current);
            test_puts(": ");
            test_puts(test_case->name);
            test_puts(" - ");

            if (test_case->setup) {
                test_case->setup();
            }

            /* Set up timeout handling with setjmp/longjmp */
            int result = test_run_fn_with_timeout(test_case->fn);

            if (test_case->cleanup) {
                test_case->cleanup();
            }

            /* Check return code and update counters */
            const char *status_msg;

            if (result == TEST_PASS) {
                test_pass_count++;
                status_msg = "PASS";
            } else if (result == TEST_FAIL) {
                test_fail_count++;
                status_msg = "FAIL";
            } else if (result == TEST_XFAIL) {
                test_xfail_count++;
                status_msg = "XFAIL (expected)";
            } else if (result == TEST_TIMEOUT) {
                test_timeout_count++;
                status_msg = "TIMEOUT";
            } else if (result == TEST_SKIP) {
                test_skip_count++;
                status_msg = "SKIP";
            } else if (result == TEST_XPASS) {
                test_xpass_count++;
                status_msg = "XPASS (unexpected!)";
            } else {
                /* Unknown code = fail */
                test_fail_count++;
                status_msg = "FAIL";
            }

            test_puts(status_msg);
            test_print_crlf();

            /* Flip screen buffer to show test result */
            if (platform_is_model || platform_is_spectrum_next) {
                screen_flip();
            }

            /* Move to next test case in this suite */
            test_case++;
        }

        /* Call suite cleanup function if provided */
        if (suite_ptr->cleanup) {
            suite_ptr->cleanup();
        }

        test_print_crlf();

        /* Move to next suite */
        suite_ptr++;
    }

    /* Print summary */
    test_print_summary();

    /* Return 0 if all passed, -1 if any failed */
    if (test_fail_count || test_timeout_count || test_xpass_count)
        return -1;

    return 0;
}

/* test_assert_eq_long - Assert two 32-bit values are equal
 * Returns: 0 if equal, -1 if not equal
 */
int test_assert_eq_long(uint32_t actual, uint32_t expected)
{
    if (actual == expected)
        return 0;

    /* Not equal - print error */
    test_puts("  Assertion failed: ");
    test_puts("expected=0x");
    test_print_hex_long(expected);
    test_puts(", actual=0x");
    test_print_hex_long(actual);
    test_print_crlf();

    return -1;
}

/* test_assert_eq_word - Assert two 16-bit values are equal
 * Returns: 0 if equal, -1 if not equal
 */
int test_assert_eq_word(uint16_t actual, uint16_t expected)
{
    if (actual == expected)
        return 0;

    /* Not equal - print error */
    test_puts("  Assertion failed: ");
    test_puts("expected=0x");
    test_print_hex_word(expected);
    test_puts(", actual=0x");
    test_print_hex_word(actual);
    test_print_crlf();

    return -1;
}

/* test_set_post_code - Set POST code register
 * Convenience function for setting POST codes in tests
 */
void test_set_post_code(uint8_t code)
{
    volatile uint8_t *post_code_reg = (volatile uint8_t *)_POST_CODE;
    *post_code_reg = code;
}

/* test_timeout_handler - Default timeout handler for test watchdog
 * Called by watchdog when a test exceeds its timeout
 * Uses longjmp to immediately exit the test with a timeout status
 */
static void test_timeout_handler(void)
{
    if (test_timeout_armed) {
        test_timeout_armed = 0;
        longjmp(test_timeout_jmpbuf, 1);  /* Jump back to setjmp with value 1 */
    }
}

/* test_set_watchdog_timeout - Set watchdog timeout for tests
 * Changes the timeout value used for all subsequent tests
 */
void test_set_watchdog_timeout(uint32_t timeout_us)
{
    test_watchdog_timeout = timeout_us;
}
