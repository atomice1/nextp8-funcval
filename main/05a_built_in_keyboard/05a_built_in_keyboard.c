/* Test Suite: Built-in Keyboard Matrix - Spectrum Next
 * Tests MMIO registers for Spectrum Next built-in keyboard matrix
 * Tests all key glyphs on the keyboard via matrix register writes (0x380080-0x380087)
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "funcval_tb.h"
#include "mmio.h"

/* Row 0 */
#define KEY_CAP_SH_ROW    0
#define KEY_CAP_SH_COL    0x01
#define KEY_CAP_SH_REG    (FUNCVAL_KB_MATRIX_BASE + KEY_CAP_SH_ROW)

#define KEY_Z_ROW         0
#define KEY_Z_COL         0x02
#define KEY_Z_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_Z_ROW)

#define KEY_X_ROW         0
#define KEY_X_COL         0x04
#define KEY_X_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_X_ROW)

#define KEY_C_ROW         0
#define KEY_C_COL         0x08
#define KEY_C_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_C_ROW)

#define KEY_V_ROW         0
#define KEY_V_COL         0x10
#define KEY_V_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_V_ROW)

#define KEY_UP_ARR_ROW    0
#define KEY_UP_ARR_COL    0x40
#define KEY_UP_ARR_REG    (FUNCVAL_KB_MATRIX_BASE + KEY_UP_ARR_ROW)

#define KEY_EXTEND_ROW    0
#define KEY_EXTEND_COL    0x20
#define KEY_EXTEND_REG    (FUNCVAL_KB_MATRIX_BASE + KEY_EXTEND_ROW)

/* Row 1 */
#define KEY_A_ROW         1
#define KEY_A_COL         0x01
#define KEY_A_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_A_ROW)

#define KEY_S_ROW         1
#define KEY_S_COL         0x02
#define KEY_S_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_S_ROW)

#define KEY_D_ROW         1
#define KEY_D_COL         0x04
#define KEY_D_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_D_ROW)

#define KEY_F_ROW         1
#define KEY_F_COL         0x08
#define KEY_F_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_F_ROW)

#define KEY_G_ROW         1
#define KEY_G_COL         0x10
#define KEY_G_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_G_ROW)

#define KEY_GRAPH_ROW     1
#define KEY_GRAPH_COL     0x40
#define KEY_GRAPH_REG     (FUNCVAL_KB_MATRIX_BASE + KEY_GRAPH_ROW)

#define KEY_CAP_LK_ROW    1
#define KEY_CAP_LK_COL    0x20
#define KEY_CAP_LK_REG    (FUNCVAL_KB_MATRIX_BASE + KEY_CAP_LK_ROW)

/* Row 2 */
#define KEY_Q_ROW         2
#define KEY_Q_COL         0x01
#define KEY_Q_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_Q_ROW)

#define KEY_W_ROW         2
#define KEY_W_COL         0x02
#define KEY_W_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_W_ROW)

#define KEY_E_ROW         2
#define KEY_E_COL         0x04
#define KEY_E_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_E_ROW)

#define KEY_R_ROW         2
#define KEY_R_COL         0x08
#define KEY_R_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_R_ROW)

#define KEY_T_ROW         2
#define KEY_T_COL         0x10
#define KEY_T_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_T_ROW)

#define KEY_INV_VID_ROW   2
#define KEY_INV_VID_COL   0x40
#define KEY_INV_VID_REG   (FUNCVAL_KB_MATRIX_BASE + KEY_INV_VID_ROW)

#define KEY_TRUE_VID_ROW  2
#define KEY_TRUE_VID_COL  0x20
#define KEY_TRUE_VID_REG  (FUNCVAL_KB_MATRIX_BASE + KEY_TRUE_VID_ROW)

/* Row 3 */
#define KEY_1_ROW         3
#define KEY_1_COL         0x01
#define KEY_1_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_1_ROW)

#define KEY_2_ROW         3
#define KEY_2_COL         0x02
#define KEY_2_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_2_ROW)

#define KEY_3_ROW         3
#define KEY_3_COL         0x04
#define KEY_3_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_3_ROW)

#define KEY_4_ROW         3
#define KEY_4_COL         0x08
#define KEY_4_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_4_ROW)

#define KEY_5_ROW         3
#define KEY_5_COL         0x10
#define KEY_5_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_5_ROW)

#define KEY_EDIT_ROW      3
#define KEY_EDIT_COL      0x40
#define KEY_EDIT_REG      (FUNCVAL_KB_MATRIX_BASE + KEY_EDIT_ROW)

#define KEY_BREAK_ROW     3
#define KEY_BREAK_COL     0x20
#define KEY_BREAK_REG     (FUNCVAL_KB_MATRIX_BASE + KEY_BREAK_ROW)

/* Row 4 */
#define KEY_0_ROW         4
#define KEY_0_COL         0x01
#define KEY_0_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_0_ROW)

#define KEY_9_ROW         4
#define KEY_9_COL         0x02
#define KEY_9_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_9_ROW)

#define KEY_8_ROW         4
#define KEY_8_COL         0x04
#define KEY_8_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_8_ROW)

#define KEY_7_ROW         4
#define KEY_7_COL         0x08
#define KEY_7_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_7_ROW)

#define KEY_6_ROW         4
#define KEY_6_COL         0x10
#define KEY_6_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_6_ROW)

#define KEY_QUOTE_ROW     4
#define KEY_QUOTE_COL     0x40
#define KEY_QUOTE_REG     (FUNCVAL_KB_MATRIX_BASE + KEY_QUOTE_ROW)

#define KEY_SEMICOLON_ROW 4
#define KEY_SEMICOLON_COL 0x20
#define KEY_SEMICOLON_REG (FUNCVAL_KB_MATRIX_BASE + KEY_SEMICOLON_ROW)

/* Row 5 */
#define KEY_P_ROW         5
#define KEY_P_COL         0x01
#define KEY_P_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_P_ROW)

#define KEY_O_ROW         5
#define KEY_O_COL         0x02
#define KEY_O_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_O_ROW)

#define KEY_I_ROW         5
#define KEY_I_COL         0x04
#define KEY_I_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_I_ROW)

#define KEY_U_ROW         5
#define KEY_U_COL         0x08
#define KEY_U_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_U_ROW)

#define KEY_Y_ROW         5
#define KEY_Y_COL         0x10
#define KEY_Y_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_Y_ROW)

#define KEY_DOT_ROW       5
#define KEY_DOT_COL       0x40
#define KEY_DOT_REG       (FUNCVAL_KB_MATRIX_BASE + KEY_DOT_ROW)

#define KEY_COMMA_ROW     5
#define KEY_COMMA_COL     0x20
#define KEY_COMMA_REG     (FUNCVAL_KB_MATRIX_BASE + KEY_COMMA_ROW)

/* Row 6 */
#define KEY_ENTER_ROW     6
#define KEY_ENTER_COL     0x01
#define KEY_ENTER_REG     (FUNCVAL_KB_MATRIX_BASE + KEY_ENTER_ROW)

#define KEY_L_ROW         6
#define KEY_L_COL         0x02
#define KEY_L_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_L_ROW)

#define KEY_K_ROW         6
#define KEY_K_COL         0x04
#define KEY_K_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_K_ROW)

#define KEY_J_ROW         6
#define KEY_J_COL         0x08
#define KEY_J_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_J_ROW)

#define KEY_H_ROW         6
#define KEY_H_COL         0x10
#define KEY_H_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_H_ROW)

#define KEY_RIGHT_ARR_ROW 6
#define KEY_RIGHT_ARR_COL 0x40
#define KEY_RIGHT_ARR_REG (FUNCVAL_KB_MATRIX_BASE + KEY_RIGHT_ARR_ROW)

#define KEY_DEL_ROW       6
#define KEY_DEL_COL       0x20
#define KEY_DEL_REG       (FUNCVAL_KB_MATRIX_BASE + KEY_DEL_ROW)

/* Row 7 */
#define KEY_SPACE_ROW     7
#define KEY_SPACE_COL     0x01
#define KEY_SPACE_REG     (FUNCVAL_KB_MATRIX_BASE + KEY_SPACE_ROW)

#define KEY_SYM_SH_ROW    7
#define KEY_SYM_SH_COL    0x02
#define KEY_SYM_SH_REG    (FUNCVAL_KB_MATRIX_BASE + KEY_SYM_SH_ROW)

#define KEY_M_ROW         7
#define KEY_M_COL         0x04
#define KEY_M_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_M_ROW)

#define KEY_N_ROW         7
#define KEY_N_COL         0x08
#define KEY_N_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_N_ROW)

#define KEY_B_ROW         7
#define KEY_B_COL         0x10
#define KEY_B_REG         (FUNCVAL_KB_MATRIX_BASE + KEY_B_ROW)

#define KEY_DOWN_ARR_ROW  7
#define KEY_DOWN_ARR_COL  0x40
#define KEY_DOWN_ARR_REG  (FUNCVAL_KB_MATRIX_BASE + KEY_DOWN_ARR_ROW)

#define KEY_LEFT_ARR_ROW  7
#define KEY_LEFT_ARR_COL  0x20
#define KEY_LEFT_ARR_REG  (FUNCVAL_KB_MATRIX_BASE + KEY_LEFT_ARR_ROW)

/* USB HID scancodes for verification (per USB spec 1.11) */
#define MATRIX_BIT_CAP_LK       0x39  /* Caps Lock */
#define MATRIX_BIT_CAP_SH       0xe1  /* Left Shift */
#define MATRIX_BIT_EXTEND       0xe4  /* Right Control (extend key) */

#define MATRIX_BIT_Q            0x14
#define MATRIX_BIT_W            0x1A
#define MATRIX_BIT_E            0x08
#define MATRIX_BIT_R            0x15
#define MATRIX_BIT_T            0x17
#define MATRIX_BIT_Y            0x1C
#define MATRIX_BIT_U            0x18
#define MATRIX_BIT_I            0x0C
#define MATRIX_BIT_O            0x12
#define MATRIX_BIT_P            0x13

#define MATRIX_BIT_A            0x04
#define MATRIX_BIT_S            0x16
#define MATRIX_BIT_D            0x07
#define MATRIX_BIT_F            0x09
#define MATRIX_BIT_G            0x0A
#define MATRIX_BIT_H            0x0B
#define MATRIX_BIT_J            0x0D
#define MATRIX_BIT_K            0x0E
#define MATRIX_BIT_L            0x0F
#define MATRIX_BIT_SEMICOLON    0x33
#define MATRIX_BIT_QUOTE        0x34
#define MATRIX_BIT_ENTER        0x28

#define MATRIX_BIT_GRAPH        0x47  /* Scroll Lock */
#define MATRIX_BIT_1            0x1E
#define MATRIX_BIT_2            0x1F
#define MATRIX_BIT_3            0x20
#define MATRIX_BIT_4            0x21
#define MATRIX_BIT_5            0x22
#define MATRIX_BIT_6            0x23
#define MATRIX_BIT_7            0x24
#define MATRIX_BIT_8            0x25
#define MATRIX_BIT_9            0x26
#define MATRIX_BIT_0            0x27
#define MATRIX_BIT_MINUS        0x2D
#define MATRIX_BIT_EQUAL        0x2E
#define MATRIX_BIT_BACKSPACE    0x2A

#define MATRIX_BIT_Z            0x1D
#define MATRIX_BIT_X            0x1B
#define MATRIX_BIT_C            0x06
#define MATRIX_BIT_V            0x19
#define MATRIX_BIT_B            0x05
#define MATRIX_BIT_N            0x11
#define MATRIX_BIT_M            0x10
#define MATRIX_BIT_COMMA        0x36
#define MATRIX_BIT_DOT          0x37
#define MATRIX_BIT_SLASH        0x38
#define MATRIX_BIT_RIGHT_ARR    0x4F
#define MATRIX_BIT_LEFT_ARR     0x50
#define MATRIX_BIT_DOWN_ARR     0x51
#define MATRIX_BIT_UP_ARR       0x52

#define MATRIX_BIT_SPACE        0x2C
#define MATRIX_BIT_DEL          0x2A
#define MATRIX_BIT_INV_VID      0x4E
#define MATRIX_BIT_TRUE_VID     0x4B
#define MATRIX_BIT_BREAK        0x29
#define MATRIX_BIT_EDIT         0x39 /* Caps Lock */
#define MATRIX_BIT_SYM_SH       0xE2 /* Left Alt */

/* Helper: Check if a bit is set in the 256-bit keyboard matrix */
static int is_matrix_bit_set(volatile uint8_t *matrix_base, uint16_t bit_index)
{
    uint16_t byte_index = bit_index >> 3;  /* Divide by 8 */
    uint8_t bit_mask = 1 << (bit_index & 0x7);  /* Modulo 8 */
    return (matrix_base[byte_index] & bit_mask) != 0;
}

/* Helper: Clear latched bit */
static void clear_latched_bit(uint16_t matrix_bit)
{
    uint16_t byte_index = matrix_bit >> 3;
    uint8_t bit_mask = 1 << (matrix_bit & 0x7);
    volatile uint8_t *latched_write = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;
    latched_write[byte_index] = bit_mask;
}

/* Helper: Write to built-in keyboard matrix register */
static void set_keyboard_matrix_key(uint32_t reg, uint8_t col_mask, int press, const char *key_name)
{
    volatile uint8_t *kb_reg = (volatile uint8_t *)reg;

    if (platform_is_interactive) {
        /* Hardware: prompt user to press/release key */
        if (press) {
            test_puts("Press built-in keyboard key: '");
            test_puts(key_name);
            test_puts("'\n");
            screen_flip();
        } else {
            test_puts("Release built-in keyboard key: '");
            test_puts(key_name);
            test_puts("'\n");
            screen_flip();
        }
    } else {
        /* Simulation: write directly to keyboard matrix register.
         * Spectrum Next keyboard columns are active-low: clear the bit to press,
         * set all bits (0xFF) to release. col_mask identifies the column bit. */
        if (press) {
            *kb_reg = ~col_mask;  /* Clear column bit (active-low = pressed) */
        } else {
            *kb_reg = 0xFF;  /* All columns high = all released */
        }
        usleep(1500);  /* 1.5ms: covers 2 full keyboard scan cycles */
    }
}

/* Helper: Test a key via built-in keyboard matrix interface */
static int test_builtin_key_input(uint32_t reg, uint8_t col_mask, uint16_t matrix_bit, const char *key_name)
{
    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX;
    volatile uint8_t *matrix_latched = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;

    /* Step 1: Press key via built-in keyboard matrix register */
    set_keyboard_matrix_key(reg, col_mask, 1, key_name);

    if (platform_is_interactive) {
        /* Hardware: poll with timeout ~10 seconds */
        uint64_t start_time = timer_get_us();

        while (timer_get_us() - start_time < 10000000) {
            if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
                test_puts("Key detected\n");
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }
        test_puts("\n");
    }

    /* Step 2: Check current matrix has bit set */
    if (!is_matrix_bit_set(matrix, matrix_bit)) {
        test_puts("2. Matrix bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" not set\n");
        return TEST_FAIL;
    }

    /* Step 3: Check latched matrix has bit set */
    if (!is_matrix_bit_set(matrix_latched, matrix_bit)) {
        test_puts("3. Latched bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" not set\n");
        return TEST_FAIL;
    }

    /* Step 4: Release key via built-in keyboard matrix register */
    set_keyboard_matrix_key(reg, col_mask, 0, key_name);

    if (platform_is_interactive) {
        /* Hardware: wait for current to be cleared */
        uint64_t start_time = timer_get_us();

        while (timer_get_us() - start_time < 5000000) {
            if (!is_matrix_bit_set(matrix, matrix_bit)) {
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }
        test_puts("\n");
    }

    /* Step 5: Check current matrix has bit cleared */
    if (is_matrix_bit_set(matrix, matrix_bit)) {
        test_puts("5. Matrix bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" not cleared\n");
        return TEST_FAIL;
    }

    /* Step 6: Check latched matrix STILL has the bit set */
    if (!is_matrix_bit_set(matrix_latched, matrix_bit)) {
        test_puts("6. Latched bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" cleared prematurely\n");
        return TEST_FAIL;
    }

    /* Step 7: Clear latched state (write 1 to clear) */
    clear_latched_bit(matrix_bit);

    /* Step 8: Verify latched state cleared */
    if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
        test_puts("8. Latched bit 0x");
        uart_print_hex_word(matrix_bit);
        test_puts(" not cleared\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test: CAPS SHIFT (built-in keyboard) */
static int test_builtin_key_cap_sh(void)
{
    return test_builtin_key_input(KEY_CAP_SH_REG, KEY_CAP_SH_COL, MATRIX_BIT_CAP_SH, "CAP SHIFT");
}

/* Test: Z key (built-in keyboard) */
static int test_builtin_key_z(void)
{
    return test_builtin_key_input(KEY_Z_REG, KEY_Z_COL, MATRIX_BIT_Z, "Z");
}

/* Test: X key (built-in keyboard) */
static int test_builtin_key_x(void)
{
    return test_builtin_key_input(KEY_X_REG, KEY_X_COL, MATRIX_BIT_X, "X");
}

/* Test: C key (built-in keyboard) */
static int test_builtin_key_c(void)
{
    return test_builtin_key_input(KEY_C_REG, KEY_C_COL, MATRIX_BIT_C, "C");
}

/* Test: V key (built-in keyboard) */
static int test_builtin_key_v(void)
{
    return test_builtin_key_input(KEY_V_REG, KEY_V_COL, MATRIX_BIT_V, "V");
}

/* Test: Right arrow key (built-in keyboard) */
static int test_builtin_key_right_arr(void)
{
    return test_builtin_key_input(KEY_RIGHT_ARR_REG, KEY_RIGHT_ARR_COL, MATRIX_BIT_RIGHT_ARR, "RIGHT ARROW");
}

/* Test: Extend key (built-in keyboard) */
static int test_builtin_key_extend(void)
{
    return test_builtin_key_input(KEY_EXTEND_REG, KEY_EXTEND_COL, MATRIX_BIT_EXTEND, "EXTEND");
}

/* Test: A key (built-in keyboard) */
static int test_builtin_key_a(void)
{
    return test_builtin_key_input(KEY_A_REG, KEY_A_COL, MATRIX_BIT_A, "A");
}

/* Test: S key (built-in keyboard) */
static int test_builtin_key_s(void)
{
    return test_builtin_key_input(KEY_S_REG, KEY_S_COL, MATRIX_BIT_S, "S");
}

/* Test: D key (built-in keyboard) */
static int test_builtin_key_d(void)
{
    return test_builtin_key_input(KEY_D_REG, KEY_D_COL, MATRIX_BIT_D, "D");
}

/* Test: F key (built-in keyboard) */
static int test_builtin_key_f(void)
{
    return test_builtin_key_input(KEY_F_REG, KEY_F_COL, MATRIX_BIT_F, "F");
}

/* Test: G key (built-in keyboard) */
static int test_builtin_key_g(void)
{
    return test_builtin_key_input(KEY_G_REG, KEY_G_COL, MATRIX_BIT_G, "G");
}

/* Test: Left arrow key (built-in keyboard) */
static int test_builtin_key_left_arr(void)
{
    return test_builtin_key_input(KEY_LEFT_ARR_REG, KEY_LEFT_ARR_COL, MATRIX_BIT_LEFT_ARR, "LEFT ARROW");
}

/* Test: Caps Lock key (built-in keyboard) */
static int test_builtin_key_cap_lk(void)
{
    return test_builtin_key_input(KEY_CAP_LK_REG, KEY_CAP_LK_COL, MATRIX_BIT_CAP_LK, "CAPS LOCK");
}

/* Test: Q key (built-in keyboard) */
static int test_builtin_key_q(void)
{
    return test_builtin_key_input(KEY_Q_REG, KEY_Q_COL, MATRIX_BIT_Q, "Q");
}

/* Test: W key (built-in keyboard) */
static int test_builtin_key_w(void)
{
    return test_builtin_key_input(KEY_W_REG, KEY_W_COL, MATRIX_BIT_W, "W");
}

/* Test: E key (built-in keyboard) */
static int test_builtin_key_e(void)
{
    return test_builtin_key_input(KEY_E_REG, KEY_E_COL, MATRIX_BIT_E, "E");
}

/* Test: R key (built-in keyboard) */
static int test_builtin_key_r(void)
{
    return test_builtin_key_input(KEY_R_REG, KEY_R_COL, MATRIX_BIT_R, "R");
}

/* Test: T key (built-in keyboard) */
static int test_builtin_key_t(void)
{
    return test_builtin_key_input(KEY_T_REG, KEY_T_COL, MATRIX_BIT_T, "T");
}

/* Test: Down arrow key (built-in keyboard) */
static int test_builtin_key_down_arr(void)
{
    return test_builtin_key_input(KEY_DOWN_ARR_REG, KEY_DOWN_ARR_COL, MATRIX_BIT_DOWN_ARR, "DOWN ARROW");
}

/* Test: Graph key (built-in keyboard) */
static int test_builtin_key_graph(void)
{
    return test_builtin_key_input(KEY_GRAPH_REG, KEY_GRAPH_COL, MATRIX_BIT_GRAPH, "GRAPH");
}

/* Test: 1 key (built-in keyboard) */
static int test_builtin_key_1(void)
{
    return test_builtin_key_input(KEY_1_REG, KEY_1_COL, MATRIX_BIT_1, "1");
}

/* Test: 2 key (built-in keyboard) */
static int test_builtin_key_2(void)
{
    return test_builtin_key_input(KEY_2_REG, KEY_2_COL, MATRIX_BIT_2, "2");
}

/* Test: 3 key (built-in keyboard) */
static int test_builtin_key_3(void)
{
    return test_builtin_key_input(KEY_3_REG, KEY_3_COL, MATRIX_BIT_3, "3");
}

/* Test: 4 key (built-in keyboard) */
static int test_builtin_key_4(void)
{
    return test_builtin_key_input(KEY_4_REG, KEY_4_COL, MATRIX_BIT_4, "4");
}

/* Test: 5 key (built-in keyboard) */
static int test_builtin_key_5(void)
{
    return test_builtin_key_input(KEY_5_REG, KEY_5_COL, MATRIX_BIT_5, "5");
}

/* Test: Up arrow key (built-in keyboard) */
static int test_builtin_key_up_arr(void)
{
    return test_builtin_key_input(KEY_UP_ARR_REG, KEY_UP_ARR_COL, MATRIX_BIT_UP_ARR, "UP ARROW");
}

/* Test: True Video key (built-in keyboard) */
static int test_builtin_key_true_vid(void)
{
    return test_builtin_key_input(KEY_TRUE_VID_REG, KEY_TRUE_VID_COL, MATRIX_BIT_TRUE_VID, "TRUE VIDEO");
}

/* Test: 0 key (built-in keyboard) */
static int test_builtin_key_0(void)
{
    return test_builtin_key_input(KEY_0_REG, KEY_0_COL, MATRIX_BIT_0, "0");
}

/* Test: 9 key (built-in keyboard) */
static int test_builtin_key_9(void)
{
    return test_builtin_key_input(KEY_9_REG, KEY_9_COL, MATRIX_BIT_9, "9");
}

/* Test: 8 key (built-in keyboard) */
static int test_builtin_key_8(void)
{
    return test_builtin_key_input(KEY_8_REG, KEY_8_COL, MATRIX_BIT_8, "8");
}

/* Test: 7 key (built-in keyboard) */
static int test_builtin_key_7(void)
{
    return test_builtin_key_input(KEY_7_REG, KEY_7_COL, MATRIX_BIT_7, "7");
}

/* Test: 6 key (built-in keyboard) */
static int test_builtin_key_6(void)
{
    return test_builtin_key_input(KEY_6_REG, KEY_6_COL, MATRIX_BIT_6, "6");
}

/* Test: Dot key (built-in keyboard) */
static int test_builtin_key_dot(void)
{
    return test_builtin_key_input(KEY_DOT_REG, KEY_DOT_COL, MATRIX_BIT_DOT, "DOT");
}

/* Test: Inverse Video key (built-in keyboard) */
static int test_builtin_key_inv_vid(void)
{
    return test_builtin_key_input(KEY_INV_VID_REG, KEY_INV_VID_COL, MATRIX_BIT_INV_VID, "INVERSE VIDEO");
}

/* Test: P key (built-in keyboard) */
static int test_builtin_key_p(void)
{
    return test_builtin_key_input(KEY_P_REG, KEY_P_COL, MATRIX_BIT_P, "P");
}

/* Test: O key (built-in keyboard) */
static int test_builtin_key_o(void)
{
    return test_builtin_key_input(KEY_O_REG, KEY_O_COL, MATRIX_BIT_O, "O");
}

/* Test: I key (built-in keyboard) */
static int test_builtin_key_i(void)
{
    return test_builtin_key_input(KEY_I_REG, KEY_I_COL, MATRIX_BIT_I, "I");
}

/* Test: U key (built-in keyboard) */
static int test_builtin_key_u(void)
{
    return test_builtin_key_input(KEY_U_REG, KEY_U_COL, MATRIX_BIT_U, "U");
}

/* Test: Y key (built-in keyboard) */
static int test_builtin_key_y(void)
{
    return test_builtin_key_input(KEY_Y_REG, KEY_Y_COL, MATRIX_BIT_Y, "Y");
}

/* Test: Comma key (built-in keyboard) */
static int test_builtin_key_comma(void)
{
    return test_builtin_key_input(KEY_COMMA_REG, KEY_COMMA_COL, MATRIX_BIT_COMMA, "COMMA");
}

/* Test: Break key (built-in keyboard) */
static int test_builtin_key_break(void)
{
    return test_builtin_key_input(KEY_BREAK_REG, KEY_BREAK_COL, MATRIX_BIT_BREAK, "BREAK");
}

/* Test: Enter key (built-in keyboard) */
static int test_builtin_key_enter(void)
{
    return test_builtin_key_input(KEY_ENTER_REG, KEY_ENTER_COL, MATRIX_BIT_ENTER, "ENTER");
}

/* Test: L key (built-in keyboard) */
static int test_builtin_key_l(void)
{
    return test_builtin_key_input(KEY_L_REG, KEY_L_COL, MATRIX_BIT_L, "L");
}

/* Test: K key (built-in keyboard) */
static int test_builtin_key_k(void)
{
    return test_builtin_key_input(KEY_K_REG, KEY_K_COL, MATRIX_BIT_K, "K");
}

/* Test: J key (built-in keyboard) */
static int test_builtin_key_j(void)
{
    return test_builtin_key_input(KEY_J_REG, KEY_J_COL, MATRIX_BIT_J, "J");
}

/* Test: H key (built-in keyboard) */
static int test_builtin_key_h(void)
{
    return test_builtin_key_input(KEY_H_REG, KEY_H_COL, MATRIX_BIT_H, "H");
}

/* Test: Quote key (built-in keyboard) */
static int test_builtin_key_quote(void)
{
    return test_builtin_key_input(KEY_QUOTE_REG, KEY_QUOTE_COL, MATRIX_BIT_QUOTE, "QUOTE");
}

/* Test: Edit key (built-in keyboard) */
static int test_builtin_key_edit(void)
{
    return test_builtin_key_input(KEY_EDIT_REG, KEY_EDIT_COL, MATRIX_BIT_EDIT, "EDIT");
}

/* Test: Space key (built-in keyboard) */
static int test_builtin_key_space(void)
{
    return test_builtin_key_input(KEY_SPACE_REG, KEY_SPACE_COL, MATRIX_BIT_SPACE, "SPACE");
}

/* Test: Symbol Shift key (built-in keyboard) */
static int test_builtin_key_sym_sh(void)
{
    return test_builtin_key_input(KEY_SYM_SH_REG, KEY_SYM_SH_COL, MATRIX_BIT_SYM_SH, "SYMBOL SHIFT");
}

/* Test: M key (built-in keyboard) */
static int test_builtin_key_m(void)
{
    return test_builtin_key_input(KEY_M_REG, KEY_M_COL, MATRIX_BIT_M, "M");
}

/* Test: N key (built-in keyboard) */
static int test_builtin_key_n(void)
{
    return test_builtin_key_input(KEY_N_REG, KEY_N_COL, MATRIX_BIT_N, "N");
}

/* Test: B key (built-in keyboard) */
static int test_builtin_key_b(void)
{
    return test_builtin_key_input(KEY_B_REG, KEY_B_COL, MATRIX_BIT_B, "B");
}

/* Test: Semicolon key (built-in keyboard) */
static int test_builtin_key_semicolon(void)
{
    return test_builtin_key_input(KEY_SEMICOLON_REG, KEY_SEMICOLON_COL, MATRIX_BIT_SEMICOLON, "SEMICOLON");
}

/* Test: Delete key (built-in keyboard) */
static int test_builtin_key_del(void)
{
    return test_builtin_key_input(KEY_DEL_REG, KEY_DEL_COL, MATRIX_BIT_DEL, "DELETE");
}

/* Test: Multi-key latching via built-in keyboard matrix */
static int test_builtin_multi_key_latching(void)
{
    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX;
    volatile uint8_t *matrix_latched = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;

    /* Clear any previous latched state for test keys */
    clear_latched_bit(MATRIX_BIT_A);
    clear_latched_bit(MATRIX_BIT_S);

    if (platform_is_interactive) {
        test_puts("This test will press both A and S keys via built-in keyboard\n");
        screen_flip();
    }

    /* Press A, wait for latch, release A, wait for release */
    set_keyboard_matrix_key(KEY_A_REG, KEY_A_COL, 1, "A");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_A) &&
               timer_get_us() - t < 10000000)
            usleep(10000);
        if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_A))
            return TEST_TIMEOUT;
    }
    set_keyboard_matrix_key(KEY_A_REG, KEY_A_COL, 0, "A");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (is_matrix_bit_set(matrix, MATRIX_BIT_A) &&
               timer_get_us() - t < 5000000)
            usleep(10000);
    }

    /* Press S, wait for latch, release S, wait for release */
    set_keyboard_matrix_key(KEY_S_REG, KEY_S_COL, 1, "S");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_S) &&
               timer_get_us() - t < 10000000)
            usleep(10000);
        if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_S))
            return TEST_TIMEOUT;
    }
    set_keyboard_matrix_key(KEY_S_REG, KEY_S_COL, 0, "S");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (is_matrix_bit_set(matrix, MATRIX_BIT_S) &&
               timer_get_us() - t < 5000000)
            usleep(10000);
    }

    /* Verify both are latched */
    if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_A) ||
        !is_matrix_bit_set(matrix_latched, MATRIX_BIT_S)) {
        test_puts("Both keys not latched: A=");
        uart_print_hex_word(is_matrix_bit_set(matrix_latched, MATRIX_BIT_A));
        test_puts(" S=");
        uart_print_hex_word(is_matrix_bit_set(matrix_latched, MATRIX_BIT_S));
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Clear only A key */
    clear_latched_bit(MATRIX_BIT_A);
    usleep(100);

    if (is_matrix_bit_set(matrix_latched, MATRIX_BIT_A)) {
        test_puts("A key not cleared\n");
        return TEST_FAIL;
    }

    if (!is_matrix_bit_set(matrix_latched, MATRIX_BIT_S)) {
        test_puts("S key cleared unexpectedly\n");
        return TEST_FAIL;
    }

    /* Clear S key */
    clear_latched_bit(MATRIX_BIT_S);
    usleep(100);

    if (is_matrix_bit_set(matrix_latched, MATRIX_BIT_A) ||
        is_matrix_bit_set(matrix_latched, MATRIX_BIT_S)) {
        test_puts("Keys not both cleared\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

void software_init_hook(void)
{
    platform_detect();
    if (platform_is_simulation)
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_KB) = 1;
}

/* Define test suite - all Spectrum Next keyboard keys */
TEST_SUITE(05a_keyboard_builtin,
    builtin_key_cap_sh,
    builtin_key_z,
    builtin_key_x,
    builtin_key_c,
    builtin_key_v,
    builtin_key_up_arr,
    builtin_key_extend,
    builtin_key_a,
    builtin_key_s,
    builtin_key_d,
    builtin_key_f,
    builtin_key_g,
    builtin_key_graph,
    builtin_key_cap_lk,
    builtin_key_q,
    builtin_key_w,
    builtin_key_e,
    builtin_key_r,
    builtin_key_t,
    builtin_key_inv_vid,
    builtin_key_true_vid,
    builtin_key_1,
    builtin_key_2,
    builtin_key_3,
    builtin_key_4,
    builtin_key_5,
    builtin_key_edit,
    builtin_key_break,
    builtin_key_0,
    builtin_key_9,
    builtin_key_8,
    builtin_key_7,
    builtin_key_6,
    builtin_key_quote,
    builtin_key_semicolon,
    builtin_key_p,
    builtin_key_o,
    builtin_key_i,
    builtin_key_u,
    builtin_key_y,
    builtin_key_dot,
    builtin_key_comma,
    builtin_key_enter,
    builtin_key_l,
    builtin_key_k,
    builtin_key_j,
    builtin_key_h,
    builtin_key_right_arr,
    builtin_key_del,
    builtin_key_space,
    builtin_key_sym_sh,
    builtin_key_m,
    builtin_key_n,
    builtin_key_b,
    builtin_key_down_arr,
    builtin_key_left_arr,
    builtin_multi_key_latching
);
