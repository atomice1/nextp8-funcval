/* nextp8 Functional Validation Library
 * Public API - Include this header in tests
 *
 * Copyright (C) 2026 Chris January
 */

#ifndef FUNCVAL_H
#define FUNCVAL_H

#include <stdbool.h>
#include <stdint.h>

/* Test case and suite structures */
struct test_case {
    const char *name;
    int (*fn)(void);
    void (*setup)(void);   /* Called before running test (can be NULL) */
    void (*cleanup)(void); /* Called after running test (can be NULL) */
};

struct test_suite {
    const char *name;
    struct test_case *test_cases;
    void (*setup)(void);   /* Called before running suite tests (can be NULL) */
    void (*cleanup)(void); /* Called after running suite tests (can be NULL) */
};

/* Variadic macros - include standalone generated header with up to 104 args */
#include "variadic_macros.h"

/* TRANSFORM macro - uses VARCOUNT from variadic_macros.h */
#define TRANSFORM(...) \
    GLUE(TRANSFORM_N_, VARCOUNT(__VA_ARGS__))(__VA_ARGS__)


/* TEST_CASE/TEST_CASE_SETUP_CLEANUP macros expand to a tuple that
 * TEST_SUITE can convert into a test_case initializer.
 */
#define TEST_CASE_SETUP_CLEANUP(NAME_, SETUP_, CLEANUP_) (NAME_, SETUP_, CLEANUP_)
#define TEST_CASE(NAME_) TEST_CASE_SETUP_CLEANUP(NAME_, NULL, NULL)

#define TEST_CASE_INIT(NAME_, SETUP_, CLEANUP_) \
    {.name = #NAME_, .fn = test_##NAME_, .setup = SETUP_, .cleanup = CLEANUP_},

#define TEST_CASE_EXPAND(ARG_) TEST_CASE_EXPAND_I(TEST_CASE_WRAP(ARG_))
#define TEST_CASE_EXPAND_I(ARGS_) TEST_CASE_INIT ARGS_

#define PROBE() ~, 1
#define SECOND(A_, B_, ...) B_
#define IS_PROBE(...) SECOND(__VA_ARGS__, 0)
#define IS_PAREN(X_) IS_PROBE(IS_PAREN_PROBE X_)
#define IS_PAREN_PROBE(...) PROBE()

#define TEST_CASE_WRAP(ARG_) TEST_CASE_WRAP_I(IS_PAREN(ARG_), ARG_)
#define TEST_CASE_WRAP_I(IS_PAREN_, ARG_) TEST_CASE_WRAP_II(IS_PAREN_, ARG_)
#define TEST_CASE_WRAP_II(IS_PAREN_, ARG_) GLUE(TEST_CASE_WRAP_, IS_PAREN_)(ARG_)
#define TEST_CASE_WRAP_0(ARG_) TEST_CASE_SETUP_CLEANUP(ARG_, NULL, NULL)
#define TEST_CASE_WRAP_1(ARG_) ARG_

/* TEST_SUITE macro - creates named test suite with all functions
 * Example:
 *   TEST_SUITE(my_suite, foo, TEST_CASE(bar), TEST_CASE_SETUP_CLEANUP(baz, baz_setup, baz_cleanup));
 *
 * Expands to:
 *   static struct test_case my_suite_test_cases[] = {
 *       {.name = "foo", .fn = test_foo, .setup = NULL, .cleanup = NULL},
 *       {.name = "bar", .fn = test_bar, .setup = NULL, .cleanup = NULL},
 *       {.name = "baz", .fn = test_baz, .setup = baz_setup, .cleanup = baz_cleanup},
 *       {.name = NULL, .fn = NULL}
 *   };
 *   __attribute__((section(".test_suite_entry"), used))
 *   struct test_suite my_suite = {
 *       .name = #my_suite,
 *       .test_cases = my_suite_test_cases,
 *       .setup = NULL,
 *       .cleanup = NULL
 *   };
 */

#define TEST_SUITE(SUITE_NAME_, ...) \
    static struct test_case GLUE(__test_suite_, GLUE(SUITE_NAME_, _test_cases))[] = { \
        TRANSFORM(__VA_ARGS__) \
        {.name = NULL, .fn = NULL} \
    }; \
    __attribute__((section(".test_suite_entry"), used)) \
    struct test_suite GLUE(__test_suite_, SUITE_NAME_) = { \
        .name = #SUITE_NAME_, \
        .test_cases = GLUE(__test_suite_, GLUE(SUITE_NAME_, _test_cases)), \
        .setup = NULL, \
        .cleanup = NULL \
    };

/* TEST_SUITE_SETUP_CLEANUP macro - creates test suite with setup/cleanup functions
 * Example:
 *   void my_suite_setup(void) { ... }
 *   void my_suite_cleanup(void) { ... }
 *   TEST_SUITE_SETUP_CLEANUP(my_suite, my_suite_setup, my_suite_cleanup, foo, bar, baz);
 */
#define TEST_SUITE_SETUP_CLEANUP(SUITE_NAME_, SETUP_, CLEANUP_, ...) \
    static struct test_case GLUE(__test_suite_, GLUE(SUITE_NAME_, _test_cases))[] = { \
        TRANSFORM(__VA_ARGS__) \
        {.name = NULL, .fn = NULL} \
    }; \
    __attribute__((section(".test_suite_entry"), used)) \
    struct test_suite GLUE(__test_suite_, SUITE_NAME_) = { \
        .name = #SUITE_NAME_, \
        .test_cases = GLUE(__test_suite_, GLUE(SUITE_NAME_, _test_cases)), \
        .setup = SETUP_, \
        .cleanup = CLEANUP_ \
    };

/* Platform detection */
extern bool platform_is_spectrum_next;
extern bool platform_is_simulation;
extern bool platform_is_model;
extern bool platform_is_interactive;
extern bool platform_has_testbench;

void platform_detect(void);
const char *platform_get_name(void);

/* UART functions */
void uart_init(void);
void uart_write_byte(uint8_t byte);
void uart_puts(const char *str);
void uart_print_crlf(void);
void uart_print_hex_long(uint32_t value);
void uart_print_hex_word(uint16_t value);
void uart_print_hex_byte(uint8_t value);
void uart_print_dec(uint32_t value);

/* Timer functions */
uint64_t timer_get_us(void);
void usleep(uint32_t microseconds);
void delay_10us(void);

/* Screen functions */
extern uint16_t screen_cursor_x;
extern uint16_t screen_cursor_y;

void screen_clear(void);
void screen_set_pixel(uint16_t x, uint16_t y, uint8_t color);
void screen_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);
void screen_scroll(uint16_t pixels);
void screen_flip(void);
void screen_putchar(char c);
void screen_puts(const char *str);
void screen_print_hex_long(uint32_t value);
void screen_print_hex_word(uint16_t value);
void screen_print_hex_byte(uint8_t value);
void screen_print_dec(uint32_t value);

/* Keyboard input functions */
void wait_for_any_key(void);
uint8_t read_keyboard_scancode(void);
char read_keyboard_char(void);

/* Watchdog functions */
extern uint32_t watchdog_timeout_us;
extern uint32_t watchdog_start_time_hi;
extern uint32_t watchdog_start_time_lo;
extern void (*watchdog_handler)(void);
extern uint8_t watchdog_active;
extern volatile uint32_t vblank_count;

void watchdog_start(uint32_t timeout_us, void (*handler)(void));
void watchdog_restart(void);
void watchdog_set_timeout(uint32_t timeout_us);
void watchdog_stop(void);
void watchdog_vblank_handler(void);
int watchdog_check(void);

/* wait_vsync - Wait for the next VBlank interrupt.
 * Requires VBlank interrupts to be enabled (e.g. via watchdog_start).
 * Returns 0 if a VBlank was seen, 1 if timed out after ~100 ms. */
int wait_vsync(void);

/* Test framework functions */
extern uint32_t test_total_count;
extern uint32_t test_pass_count;
extern uint32_t test_fail_count;
extern uint32_t test_skip_count;
extern uint32_t test_xfail_count;
extern uint32_t test_xpass_count;
extern uint32_t test_timeout_count;
extern uint32_t test_watchdog_timeout;

void test_init(void);
void test_puts(const char *str);
void test_print_crlf(void);
void test_print_dec(uint32_t value);
void test_print_hex_long(uint32_t value);
void test_print_hex_word(uint16_t value);
void test_print_hex_byte(uint8_t value);
void test_print_summary(void);
int test_run_suite(struct test_suite *specific_suite);
int test_assert_eq_long(uint32_t actual, uint32_t expected);
int test_assert_eq_word(uint16_t actual, uint16_t expected);
void test_set_post_code(uint8_t code);
void test_set_watchdog_timeout(uint32_t timeout_us);

/* Test return codes */
#define TEST_PASS           0
#define TEST_FAIL          -1
#define TEST_XFAIL         -2  /* Expected failure (known issue) */
#define TEST_TIMEOUT       -3
#define TEST_SKIP           1
#define TEST_XPASS          2  /* Unexpected pass (was expected to fail) */

/* Legacy aliases */
#define TEST_EXPECTED_FAIL   TEST_XFAIL
#define TEST_UNEXPECTED_PASS TEST_XPASS

#endif /* FUNCVAL_H */
