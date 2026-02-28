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

/* Variadic macro machinery for TEST_CASE expansion
 * These macros count arguments and expand a transformation across them
 */

#define EVAL(...) __VA_ARGS__

#define VARCOUNT(...) \
    EVAL(VARCOUNT_I(__VA_ARGS__, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, \
                                   50, 49, 48, 47, 46, 45, 44, 43, 42, 41, \
                                   40, 39, 38, 37, 36, 35, 34, 33, 32, 31, \
                                   30, 29, 28, 27, 26, 25, 24, 23, 22, 21, \
                                   20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
                                   10, 9, 8, 7, 6, 5, 4, 3, 2, 1,))

#define VARCOUNT_I(_,  _60,_59,_58,_57,_56,_55,_54,_53,_52,_51, \
                       _50,_49,_48,_47,_46,_45,_44,_43,_42,_41, \
                       _40,_39,_38,_37,_36,_35,_34,_33,_32,_31, \
                       _30,_29,_28,_27,_26,_25,_24,_23,_22,_21, \
                       _20,_19,_18,_17,_16,_15,_14,_13,_12,_11, \
                       _10,_9,_8,_7,_6,_5,_4,_3,_2,X_,...) X_

#define GLUE(X, Y) GLUE_I(X, Y)
#define GLUE_I(X, Y) X##Y

/* Transform macro - applies TEST_CASE to each test function name
 * Direct expansion approach: dispatch on argument count
 */
#define TRANSFORM(...) \
    GLUE(TRANSFORM_N_, VARCOUNT(__VA_ARGS__))(__VA_ARGS__)

/* Direct count-based macro definitions
 * Each TRANSFORM_N_K applies TEST_CASE to K arguments
 */
#define TRANSFORM_N_1(A1) TEST_CASE_EXPAND(A1)
#define TRANSFORM_N_2(A1, A2) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2)
#define TRANSFORM_N_3(A1, A2, A3) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3)
#define TRANSFORM_N_4(A1, A2, A3, A4) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4)
#define TRANSFORM_N_5(A1, A2, A3, A4, A5) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5)
#define TRANSFORM_N_6(A1, A2, A3, A4, A5, A6) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6)
#define TRANSFORM_N_7(A1, A2, A3, A4, A5, A6, A7) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7)
#define TRANSFORM_N_8(A1, A2, A3, A4, A5, A6, A7, A8) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8)
#define TRANSFORM_N_9(A1, A2, A3, A4, A5, A6, A7, A8, A9) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9)
#define TRANSFORM_N_10(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10)
#define TRANSFORM_N_11(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11)
#define TRANSFORM_N_12(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11) TEST_CASE_EXPAND(A12)
#define TRANSFORM_N_13(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11) TEST_CASE_EXPAND(A12) TEST_CASE_EXPAND(A13)
#define TRANSFORM_N_14(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11) TEST_CASE_EXPAND(A12) TEST_CASE_EXPAND(A13) TEST_CASE_EXPAND(A14)
#define TRANSFORM_N_15(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11) TEST_CASE_EXPAND(A12) TEST_CASE_EXPAND(A13) TEST_CASE_EXPAND(A14) TEST_CASE_EXPAND(A15)
#define TRANSFORM_N_16(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11) TEST_CASE_EXPAND(A12) TEST_CASE_EXPAND(A13) TEST_CASE_EXPAND(A14) TEST_CASE_EXPAND(A15) TEST_CASE_EXPAND(A16)
#define TRANSFORM_N_17(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11) TEST_CASE_EXPAND(A12) TEST_CASE_EXPAND(A13) TEST_CASE_EXPAND(A14) TEST_CASE_EXPAND(A15) TEST_CASE_EXPAND(A16) TEST_CASE_EXPAND(A17)
#define TRANSFORM_N_18(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11) TEST_CASE_EXPAND(A12) TEST_CASE_EXPAND(A13) TEST_CASE_EXPAND(A14) TEST_CASE_EXPAND(A15) TEST_CASE_EXPAND(A16) TEST_CASE_EXPAND(A17) TEST_CASE_EXPAND(A18)
#define TRANSFORM_N_19(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11) TEST_CASE_EXPAND(A12) TEST_CASE_EXPAND(A13) TEST_CASE_EXPAND(A14) TEST_CASE_EXPAND(A15) TEST_CASE_EXPAND(A16) TEST_CASE_EXPAND(A17) TEST_CASE_EXPAND(A18) TEST_CASE_EXPAND(A19)
#define TRANSFORM_N_20(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20) TEST_CASE_EXPAND(A1) TEST_CASE_EXPAND(A2) TEST_CASE_EXPAND(A3) TEST_CASE_EXPAND(A4) TEST_CASE_EXPAND(A5) TEST_CASE_EXPAND(A6) TEST_CASE_EXPAND(A7) TEST_CASE_EXPAND(A8) TEST_CASE_EXPAND(A9) TEST_CASE_EXPAND(A10) TEST_CASE_EXPAND(A11) TEST_CASE_EXPAND(A12) TEST_CASE_EXPAND(A13) TEST_CASE_EXPAND(A14) TEST_CASE_EXPAND(A15) TEST_CASE_EXPAND(A16) TEST_CASE_EXPAND(A17) TEST_CASE_EXPAND(A18) TEST_CASE_EXPAND(A19) TEST_CASE_EXPAND(A20)

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

void platform_detect(void);
const char *platform_get_name(void);

/* UART functions */
void uart_init(void);
void uart_write_byte(uint8_t byte);
void uart_puts(const char *str);
void uart_print_crlf(void);
void uart_print_hex_long(uint32_t value);
void uart_print_hex_word(uint16_t value);
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

void watchdog_start(uint32_t timeout_us, void (*handler)(void));
void watchdog_restart(void);
void watchdog_set_timeout(uint32_t timeout_us);
void watchdog_stop(void);
void watchdog_vblank_handler(void);
int watchdog_check(void);

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
