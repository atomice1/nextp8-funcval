/* Interactive Menu for nextp8 Functional Validation Tests
 *
 * Allows selection of individual test suites or all tests
 * Displays results and loops for multiple test runs
 *
 * Uses keyboard for input and screen for display
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "funcval.h"
#include "nextp8.h"

/* External symbols from linker */
extern struct test_suite __test_suite_start[];
extern struct test_suite __test_suite_end[];

/* Count test suites */
static uint16_t count_suites(void)
{
    return (uint16_t)(__test_suite_end - __test_suite_start);
}

/* Display menu header and list of test suites on screen */
static void display_menu(void)
{
    struct test_suite *suite;
    uint16_t suite_num = 1;
    char num_str[3];

    screen_clear();

    screen_puts("=== Test Suite Menu ===\n\n");

    /* Option 0: Run all tests */
    screen_puts("0: All Tests\n\n");

    /* List each test suite with its name */
    for (suite = __test_suite_start; suite < __test_suite_end; suite++, suite_num++) {
        /* Format number */
        if (suite_num < 10) {
            num_str[0] = '0' + suite_num;
            num_str[1] = 0;
        } else {
            num_str[0] = '0' + (suite_num / 10);
            num_str[1] = '0' + (suite_num % 10);
            num_str[2] = 0;
        }

        screen_puts(num_str);
        screen_puts(": ");
        screen_puts(suite->name);
        screen_puts("\n");
    }

    if (platform_is_spectrum_next || platform_is_model) {
        screen_flip();
    }
}

/* Get user input via keyboard - accepts 1-2 digit numbers with backspace
 * Displays input on screen
 * Returns value 0-N where N is number of test suites
 */
static uint16_t get_menu_selection(void)
{
    uint8_t input_buffer[3] = {0, 0, 0};
    uint8_t input_len = 0;
    uint16_t max_suite = count_suites();
    uint16_t selection = 0;
    uint8_t ch;
    uint16_t input_cursor_x;
    uint16_t input_cursor_y;

    screen_puts("\nSelect suite (0-");
    screen_print_dec(max_suite);
    screen_puts("): ");
    screen_flip();

    input_cursor_x = screen_cursor_x;
    input_cursor_y = screen_cursor_y;

    while (1) {
        /* Get key input */
        ch = read_keyboard_char();

        /* Backspace - delete last character */
        if (ch == 0x08 || ch == 0x7F) {  /* Backspace or Delete */
            if (input_len > 0) {
                input_len--;
                input_buffer[input_len] = 0;
            }
        }
        /* Return/Enter - submit selection */
        else if (ch == 0x0D || ch == 0x0A) {  /* CR or LF */
            if (input_len > 0) {
                selection = 0;
                for (uint8_t i = 0; i < input_len; i++) {
                    selection = selection * 10 + (input_buffer[i] - '0');
                }

                /* Validate selection */
                if (selection <= max_suite) {
                    return selection;
                }
                /* Invalid - clear and try again */
                input_len = 0;
                input_buffer[0] = 0;
            }
        }
        /* Digit - add to input buffer */
        else if (ch >= '0' && ch <= '9' && input_len < 2) {
            input_buffer[input_len] = ch;
            input_len++;
        }

        /* Show current input */
        screen_fill_rect(input_cursor_x, input_cursor_y, 8, 6, 0);  /* Clear previous input */
        screen_cursor_x = input_cursor_x;
        screen_cursor_y = input_cursor_y;
        screen_puts(input_buffer);

        screen_flip();
    }
}

/* Wait for a newly pressed key before continuing */
static void wait_for_key(void)
{
    screen_puts("\nPress any key to continue...");
    screen_flip();
    wait_for_any_key();
    screen_clear();
    screen_flip();
}

/* Main menu loop */
int main(void)
{
    struct test_suite *selected_suite;
    uint16_t selection;
    uint16_t num_suites;

    /* Platform is already initialized in crt1.c/startup
     * Log welcome message to UART */
    uart_puts("=== nextp8 Functional Validation Test Menu ===");
    uart_print_crlf();
    uart_puts("Platform: ");
    uart_puts(platform_get_name());
    uart_print_crlf();
    uart_print_crlf();

    num_suites = count_suites();
    uart_puts("Available test suites: ");
    uart_print_dec(num_suites);
    uart_print_crlf();
    uart_print_crlf();

    /* Main menu loop */
    while (1) {
        /* Display menu and get selection */
        display_menu();
        selection = get_menu_selection();

        if (selection > num_suites) {
            continue;  /* Invalid selection, try again */
        }

        /* Clear screen for test output */
        screen_clear();

        /* Initialize test framework for this run */
        test_init();

        /* Run selected test suite(s) */
        if (selection == 0) {
            /* Run all suites */
            test_run_suite(NULL);
        } else {
            /* Run specific suite (selection is 1-based, convert to 0-based index) */
            selected_suite = &__test_suite_start[selection - 1];
            test_run_suite(selected_suite);
        }

        /* Wait for user to view results */
        wait_for_key();
    }

    return 0;  /* Unreachable */
}
