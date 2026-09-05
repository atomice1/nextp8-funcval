/* Test Suite: External PS/2 Keyboard - Full US Layout
 * Tests MMIO registers for PS/2 keyboard interface
 * Tests all key glyphs on US layout PS/2 keyboard
 * Hardware reports USB HID scancodes
 *
 * Copyright (C) 2026 Chris January
 */

#include <stdint.h>
#include "nextp8.h"
#include "funcval.h"
#include "funcval_tb.h"
#include "mmio.h"

/* PS/2 Scancode definitions (Set 2) - sent to keyboard, hardware reports USB HID */
/* Reference: PS/2 Keyboard Controller Specification */
/* Notes: Extended keys use E0 prefix */

/* Note on scancode mappings:
 * - PS/2 Set 2 scancodes are sent to the keyboard controller
 * - The keyboard controller converts to USB HID scancodes for reporting
 * - Femto8 provides the authoritative mapping: ps2_scancodes.py -> usb_hid_scancodes.py
 */

/* Modifier keys - PS/2 make codes from Femto8 ps2_scancodes.py */
#define PS2_SCANCODE_LCTRL      0x14  /* PS/2: left control (0x14) */
#define PS2_SCANCODE_LSHIFT     0x12  /* PS/2: left shift (0x12) */
#define PS2_SCANCODE_LALT       0x11  /* PS/2: left alt (0x11) */
#define PS2_SCANCODE_LGUI       0xE01F /* Extended: left GUI (0xE0 0x1F) */
#define PS2_SCANCODE_RCTRL      0xE014 /* Extended: right control (0xE0 0x14) */
#define PS2_SCANCODE_RSHIFT     0x59   /* PS/2: right shift (0x59) */
#define PS2_SCANCODE_RALT       0xE011 /* Extended: right alt (0xE0 0x11) */
#define PS2_SCANCODE_RGUI       0xE027 /* Extended: right GUI (0xE0 0x27) */
#define PS2_SCANCODE_CAPSLOCK   0x58  /* PS/2: CapsLock */
#define PS2_SCANCODE_NUMLOCK    0x77  /* PS/2: NumberLock */
#define PS2_SCANCODE_SCROLLLOCK 0x7E  /* PS/2: ScrollLock */

/* Control keys */
#define PS2_SCANCODE_ESCAPE     0x76  /* PS/2: escape */
#define PS2_SCANCODE_ENTER      0x5A  /* PS/2: enter */
#define PS2_SCANCODE_SPACE      0x29  /* PS/2: space */
#define PS2_SCANCODE_BACKSPACE  0x66  /* PS/2: backspace */
#define PS2_SCANCODE_TAB        0x0D  /* PS/2: tab */
#define PS2_SCANCODE_INSERT     0xE070  /* PS/2: insert */
#define PS2_SCANCODE_DELETE     0xE071  /* PS/2: delete */
#define PS2_SCANCODE_HOME       0xE06C  /* PS/2: home */
#define PS2_SCANCODE_END        0xE069  /* PS/2: end */
#define PS2_SCANCODE_PAGEUP     0xE07D  /* PS/2: page up */
#define PS2_SCANCODE_PAGEDOWN   0xE07A  /* PS/2: page down */
/* PS/2 pause scancode: E1 14 77 E1 F0 14 F0 77 (8 bytes, requires special handling) */
/* PS2_SCANCODE_PAUSE is defined as uint64_t in test_key_pause function */
#define PS2_SCANCODE_PRINTSCREEN 0xE012E07C /* Extended: print screen (E0 12 E0 7C) */

/* Arrow keys (extended) */
#define PS2_SCANCODE_LEFT       0xE06B /* Extended: cursor left */
#define PS2_SCANCODE_RIGHT      0xE074 /* Extended: cursor right */
#define PS2_SCANCODE_UP         0xE075 /* Extended: cursor up */
#define PS2_SCANCODE_DOWN       0xE072 /* Extended: cursor down */

/* Function keys */
#define PS2_SCANCODE_F1         0x05  /* PS/2: F1 */
#define PS2_SCANCODE_F2         0x06  /* PS/2: F2 */
#define PS2_SCANCODE_F3         0x04  /* PS/2: F3 */
#define PS2_SCANCODE_F4         0x0C  /* PS/2: F4 */
#define PS2_SCANCODE_F5         0x03  /* PS/2: F5 */
#define PS2_SCANCODE_F6         0x0B  /* PS/2: F6 */
#define PS2_SCANCODE_F7         0x83  /* PS/2: F7 */
#define PS2_SCANCODE_F8         0x0A  /* PS/2: F8 */
#define PS2_SCANCODE_F9         0x01  /* PS/2: F9 */
#define PS2_SCANCODE_F10        0x09  /* PS/2: F10 */
#define PS2_SCANCODE_F11        0x78  /* PS/2: F11 */
#define PS2_SCANCODE_F12        0x07  /* PS/2: F12 */

/* Numpad keys (extended) */
#define PS2_SCANCODE_NUMPAD_0   0x70  /* PS/2: (keypad) 0 */
#define PS2_SCANCODE_NUMPAD_1   0x69  /* PS/2: (keypad) 1 */
#define PS2_SCANCODE_NUMPAD_2   0x72  /* PS/2: (keypad) 2 */
#define PS2_SCANCODE_NUMPAD_3   0x7A  /* PS/2: (keypad) 3 */
#define PS2_SCANCODE_NUMPAD_4   0x6B  /* PS/2: (keypad) 4 */
#define PS2_SCANCODE_NUMPAD_5   0x73  /* PS/2: (keypad) 5 */
#define PS2_SCANCODE_NUMPAD_6   0x74  /* PS/2: (keypad) 6 */
#define PS2_SCANCODE_NUMPAD_7   0x6C  /* PS/2: (keypad) 7 */
#define PS2_SCANCODE_NUMPAD_8   0x75  /* PS/2: (keypad) 8 */
#define PS2_SCANCODE_NUMPAD_9   0x7D  /* PS/2: (keypad) 9 */
#define PS2_SCANCODE_NUMPAD_DOT 0x71  /* PS/2: (keypad) . */
#define PS2_SCANCODE_NUMPAD_ENTER 0xE05A /* Extended: (keypad) enter */
#define PS2_SCANCODE_NUMPAD_PLUS  0x79  /* PS/2: (keypad) + */
#define PS2_SCANCODE_NUMPAD_MINUS 0x7B  /* PS/2: (keypad) - */
#define PS2_SCANCODE_NUMPAD_MUL   0x7C  /* PS/2: (keypad) * */
#define PS2_SCANCODE_NUMPAD_DIV   0xE04A /* Extended: (keypad) / */
#define PS2_SCANCODE_NUMPAD_EQUAL 0x76  /* PS/2: (keypad) = */

/* Main row keys (top row, numbers) */
#define PS2_SCANCODE_1          0x16  /* PS/2: 1 */
#define PS2_SCANCODE_2          0x1E  /* PS/2: 2 */
#define PS2_SCANCODE_3          0x26  /* PS/2: 3 */
#define PS2_SCANCODE_4          0x25  /* PS/2: 4 */
#define PS2_SCANCODE_5          0x2E  /* PS/2: 5 */
#define PS2_SCANCODE_6          0x36  /* PS/2: 6 */
#define PS2_SCANCODE_7          0x3D  /* PS/2: 7 */
#define PS2_SCANCODE_8          0x3E  /* PS/2: 8 */
#define PS2_SCANCODE_9          0x46  /* PS/2: 9 */
#define PS2_SCANCODE_0          0x45  /* PS/2: 0 (zero) */
#define PS2_SCANCODE_MINUS      0x4E  /* PS/2: - */
#define PS2_SCANCODE_EQUALS     0x55  /* PS/2: = */
#define PS2_SCANCODE_BACKSLASH  0x5D  /* PS/2: \\ */
#define PS2_SCANCODE_GRAVE      0x0E  /* PS/2: ` (back tick) */

/* QWERTY row */
#define PS2_SCANCODE_Q          0x15  /* PS/2: Q */
#define PS2_SCANCODE_W          0x1D  /* PS/2: W */
#define PS2_SCANCODE_E          0x24  /* PS/2: E */
#define PS2_SCANCODE_R          0x2D  /* PS/2: R */
#define PS2_SCANCODE_T          0x2C  /* PS/2: T */
#define PS2_SCANCODE_Y          0x35  /* PS/2: Y */
#define PS2_SCANCODE_U          0x3C  /* PS/2: U */
#define PS2_SCANCODE_I          0x43  /* PS/2: I */
#define PS2_SCANCODE_O          0x44  /* PS/2: O */
#define PS2_SCANCODE_P          0x4D  /* PS/2: P */
#define PS2_SCANCODE_LBRACKET   0x54  /* PS/2: [ */
#define PS2_SCANCODE_RBRACKET   0x5B  /* PS/2: ] */

/* ASDF row */
#define PS2_SCANCODE_A          0x1C  /* PS/2: A */
#define PS2_SCANCODE_S          0x1B  /* PS/2: S */
#define PS2_SCANCODE_D          0x23  /* PS/2: D */
#define PS2_SCANCODE_F          0x2B  /* PS/2: F */
#define PS2_SCANCODE_G          0x34  /* PS/2: G */
#define PS2_SCANCODE_H          0x33  /* PS/2: H */
#define PS2_SCANCODE_J          0x3B  /* PS/2: J */
#define PS2_SCANCODE_K          0x42  /* PS/2: K */
#define PS2_SCANCODE_L          0x4B  /* PS/2: L */
#define PS2_SCANCODE_SEMICOLON  0x4C  /* PS/2: ; */
#define PS2_SCANCODE_APOSTROPHE 0x52  /* PS/2: ' */
#define PS2_SCANCODE_HASH       0x53  /* UK: # */

/* ZXC row */
#define PS2_SCANCODE_Z          0x1A  /* PS/2: Z */
#define PS2_SCANCODE_X          0x22  /* PS/2: X */
#define PS2_SCANCODE_C          0x21  /* PS/2: C */
#define PS2_SCANCODE_V          0x2A  /* PS/2: V */
#define PS2_SCANCODE_B          0x32  /* PS/2: B */
#define PS2_SCANCODE_N          0x31  /* PS/2: N */
#define PS2_SCANCODE_M          0x3A  /* PS/2: M */
#define PS2_SCANCODE_COMMA      0x41  /* PS/2: , */
#define PS2_SCANCODE_PERIOD     0x49  /* PS/2: . */
#define PS2_SCANCODE_SLASH      0x4A  /* PS/2: / */

/* USB HID scancodes (as reported by hardware after PS/2 to USB conversion) */
/* Reference: Femto8 usb_hid_scancodes.py */
/* Notes: These are the values the hardware reports after PS/2 Set 2 scancodes are converted */

#define USB_HID_SCANCODE_NONE           0x00
#define USB_HID_SCANCODE_ERROR_ROLLOVER 0x01
#define USB_HID_SCANCODE_POST_FAIL      0x02
#define USB_HID_SCANCODE_ERROR_UNDEFINED 0x03

#define USB_HID_SCANCODE_A              0x04
#define USB_HID_SCANCODE_B              0x05
#define USB_HID_SCANCODE_C              0x06
#define USB_HID_SCANCODE_D              0x07
#define USB_HID_SCANCODE_E              0x08
#define USB_HID_SCANCODE_F              0x09
#define USB_HID_SCANCODE_G              0x0A
#define USB_HID_SCANCODE_H              0x0B
#define USB_HID_SCANCODE_I              0x0C
#define USB_HID_SCANCODE_J              0x0D
#define USB_HID_SCANCODE_K              0x0E
#define USB_HID_SCANCODE_L              0x0F
#define USB_HID_SCANCODE_M              0x10
#define USB_HID_SCANCODE_N              0x11
#define USB_HID_SCANCODE_O              0x12
#define USB_HID_SCANCODE_P              0x13
#define USB_HID_SCANCODE_Q              0x14
#define USB_HID_SCANCODE_R              0x15
#define USB_HID_SCANCODE_S              0x16
#define USB_HID_SCANCODE_T              0x17
#define USB_HID_SCANCODE_U              0x18
#define USB_HID_SCANCODE_V              0x19
#define USB_HID_SCANCODE_W              0x1A
#define USB_HID_SCANCODE_X              0x1B
#define USB_HID_SCANCODE_Y              0x1C
#define USB_HID_SCANCODE_Z              0x1D

#define USB_HID_SCANCODE_1              0x1E
#define USB_HID_SCANCODE_2              0x1F
#define USB_HID_SCANCODE_3              0x20
#define USB_HID_SCANCODE_4              0x21
#define USB_HID_SCANCODE_5              0x22
#define USB_HID_SCANCODE_6              0x23
#define USB_HID_SCANCODE_7              0x24
#define USB_HID_SCANCODE_8              0x25
#define USB_HID_SCANCODE_9              0x26
#define USB_HID_SCANCODE_0              0x27

#define USB_HID_SCANCODE_ENTER          0x28
#define USB_HID_SCANCODE_ESCAPE         0x29
#define USB_HID_SCANCODE_BACKSPACE      0x2A
#define USB_HID_SCANCODE_TAB            0x2B
#define USB_HID_SCANCODE_SPACE          0x2C
#define USB_HID_SCANCODE_MINUS          0x2D
#define USB_HID_SCANCODE_EQUAL          0x2E
#define USB_HID_SCANCODE_LEFTBRACE      0x2F
#define USB_HID_SCANCODE_RIGHTBRACE     0x30
#define USB_HID_SCANCODE_BACKSLASH      0x31
#define USB_HID_SCANCODE_HASHTILDE      0x32
#define USB_HID_SCANCODE_SEMICOLON      0x33
#define USB_HID_SCANCODE_APOSTROPHE     0x34
#define USB_HID_SCANCODE_GRAVE          0x35
#define USB_HID_SCANCODE_COMMA          0x36
#define USB_HID_SCANCODE_DOT            0x37
#define USB_HID_SCANCODE_SLASH          0x38

#define USB_HID_SCANCODE_CAPSLOCK       0x39
#define USB_HID_SCANCODE_F1             0x3A
#define USB_HID_SCANCODE_F2             0x3B
#define USB_HID_SCANCODE_F3             0x3C
#define USB_HID_SCANCODE_F4             0x3D
#define USB_HID_SCANCODE_F5             0x3E
#define USB_HID_SCANCODE_F6             0x3F
#define USB_HID_SCANCODE_F7             0x40
#define USB_HID_SCANCODE_F8             0x41
#define USB_HID_SCANCODE_F9             0x42
#define USB_HID_SCANCODE_F10            0x43
#define USB_HID_SCANCODE_F11            0x44
#define USB_HID_SCANCODE_F12            0x45
#define USB_HID_SCANCODE_SYSRQ          0x46
#define USB_HID_SCANCODE_SCROLLLOCK     0x47
#define USB_HID_SCANCODE_PAUSE          0x48
#define USB_HID_SCANCODE_INSERT         0x49
#define USB_HID_SCANCODE_HOME           0x4A
#define USB_HID_SCANCODE_PAGEUP         0x4B
#define USB_HID_SCANCODE_DELETE         0x4C
#define USB_HID_SCANCODE_END            0x4D
#define USB_HID_SCANCODE_PAGEDOWN       0x4E
#define USB_HID_SCANCODE_RIGHT          0x4F
#define USB_HID_SCANCODE_LEFT           0x50
#define USB_HID_SCANCODE_DOWN           0x51
#define USB_HID_SCANCODE_UP             0x52
#define USB_HID_SCANCODE_NUMLOCK        0x53
#define USB_HID_SCANCODE_KP_SLASH       0x54
#define USB_HID_SCANCODE_KP_ASTERISK    0x55
#define USB_HID_SCANCODE_KP_MINUS       0x56
#define USB_HID_SCANCODE_KP_PLUS        0x57
#define USB_HID_SCANCODE_KP_ENTER       0x58
#define USB_HID_SCANCODE_KP_1           0x59
#define USB_HID_SCANCODE_KP_2           0x5A
#define USB_HID_SCANCODE_KP_3           0x5B
#define USB_HID_SCANCODE_KP_4           0x5C
#define USB_HID_SCANCODE_KP_5           0x5D
#define USB_HID_SCANCODE_KP_6           0x5E
#define USB_HID_SCANCODE_KP_7           0x5F
#define USB_HID_SCANCODE_KP_8           0x60
#define USB_HID_SCANCODE_KP_9           0x61
#define USB_HID_SCANCODE_KP_0           0x62
#define USB_HID_SCANCODE_KP_DOT         0x63

#define USB_HID_SCANCODE_LEFTCTRL       0xE0
#define USB_HID_SCANCODE_LEFTSHIFT      0xE1
#define USB_HID_SCANCODE_LEFTALT        0xE2
#define USB_HID_SCANCODE_LEFTMETA       0xE3
#define USB_HID_SCANCODE_RIGHTCTRL      0xE4
#define USB_HID_SCANCODE_RIGHTSHIFT     0xE5
#define USB_HID_SCANCODE_RIGHTALT       0xE6
#define USB_HID_SCANCODE_RIGHTMETA      0xE7

/* Helper: Send PS/2 scancode (make code) for simulation or prompt user */
static void send_key_press(uint16_t scancode, const char *key_name)
{
    volatile uint8_t *scancode_reg = (volatile uint8_t *)FUNCVAL_KB_SCANCODE;

    if (platform_is_interactive) {
        /* Hardware: prompt user to press key */
        test_puts("Press and hold '");
        test_puts(key_name);
        test_puts("' key\n");
        screen_flip();
    } else {
        /* Simulation: write scancode(s) to testbench register */
        uint8_t high = (scancode >> 8) & 0xFF;
        uint8_t low = scancode & 0xFF;

        /* Send extended prefix if high byte is 0xE0 */
        if (high == 0xE0) {
            *scancode_reg = 0xE0;
            usleep(4000);
        }

        /* Send scancode byte(s) */
        *scancode_reg = low;
        usleep(4000);

        /* For multi-byte scancodes (e.g., Pause), send additional bytes */
        if (high == 0xE1) {
            *scancode_reg = low;
            usleep(4000);
            *scancode_reg = 0xF0;
            usleep(4000);
            *scancode_reg = low;
            usleep(4000);
        }
    }
}

/* Helper: Send PS/2 break code for simulation or prompt user */
static void send_key_release(uint16_t scancode, const char *key_name)
{
    volatile uint8_t *scancode_reg = (volatile uint8_t *)FUNCVAL_KB_SCANCODE;

    if (platform_is_interactive) {
        /* Hardware: prompt user to release key */
        test_puts("Release '");
        test_puts(key_name);
        test_puts("' key\n");
        screen_flip();
    } else {
        /* Simulation: write break code to testbench register */
        uint8_t high = (scancode >> 8) & 0xFF;
        uint8_t low = scancode & 0xFF;

        /* Send extended prefix if needed */
        if (scancode & 0xFF00) {
            *scancode_reg = 0xE0;
            usleep(4000);
        }

        /* Send break prefix */
        *scancode_reg = 0xF0;
        usleep(4000);

        /* Send scancode bytes */
        *scancode_reg = low;
        usleep(4000);

        /* For multi-byte scancodes (e.g., Pause), send additional bytes */
        if (high == 0xE1) {
            *scancode_reg = low;
            usleep(4000);
            *scancode_reg = 0xF0;
            usleep(4000);
            *scancode_reg = low;
            usleep(4000);
        }
    }
}

/* Helper: Check if a bit is set in the 256-bit keyboard matrix */
static int is_matrix_bit_set(volatile uint8_t *matrix_base, uint16_t bit_index)
{
    uint16_t byte_index = bit_index >> 3;  /* Divide by 8 */
    uint8_t bit_mask = 1 << (bit_index & 0x7);  /* Modulo 8 */
    return (matrix_base[byte_index] & bit_mask) != 0;
}

/* Helper: Test a key with all verification steps */
/* Sends PS/2 scancode, but checks for USB HID scancode in matrix */
static int key_input(uint16_t ps2_scancode, uint16_t usb_scancode, const char *key_name)
{
    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX;
    volatile uint8_t *matrix_latched = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;

    /* Step 1: Send key press (simulation: write scancode, hardware: prompt user) */
    send_key_press(ps2_scancode, key_name);

    if (platform_is_interactive) {
        /* Hardware: poll with timeout ~10 seconds */
        uint64_t start_time = timer_get_us();
        int detected = 0;
        uint16_t matrix_bit = usb_scancode & 0xFF;

        while (timer_get_us() - start_time < 10000000) {
            /* Detect via latched matrix so latch is preserved for step 3 */
            if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
                detected = 1;
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }

        if (!detected)
            return TEST_TIMEOUT;

        test_puts("\n");
    }

    /* Step 2: Check current matrix has bit set (use USB HID scancode) */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (!is_matrix_bit_set(matrix, matrix_bit)) {
            test_puts("2. Matrix bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" not set\n");
            return TEST_FAIL;
        }
    }

    /* Step 3: Check latched matrix has bit set (use USB HID scancode) */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (!is_matrix_bit_set(matrix_latched, matrix_bit)) {
            test_puts("3. Latched bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" not set\n");
            return TEST_FAIL;
        }
    }

    /* Step 4: Send key release (simulation: write break code, hardware: prompt user) */
    send_key_release(ps2_scancode, key_name);

    if (platform_is_interactive) {
        /* Hardware: wait for current to be cleared */
        uint64_t start_time = timer_get_us();
        uint16_t matrix_bit = usb_scancode & 0xFF;

        while (timer_get_us() - start_time < 5000000) {
            if (!is_matrix_bit_set(matrix, matrix_bit)) {
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }
        test_puts("\n");
    }

    /* Step 5: Check current matrix has bit cleared (use USB HID scancode) */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (is_matrix_bit_set(matrix, matrix_bit)) {
            test_puts("5. Matrix bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" not cleared\n");
            return TEST_FAIL;
        }
    }

    /* Step 6: Check latched matrix STILL has the bit set (use USB HID scancode) */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (!is_matrix_bit_set(matrix_latched, matrix_bit)) {
            test_puts("6. Latched bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" cleared prematurely\n");
            return TEST_FAIL;
        }
    }

    /* Step 7: Clear latched state (write 1 to clear) */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        uint16_t byte_index = matrix_bit >> 3;
        uint8_t bit_mask = 1 << (matrix_bit & 0x7);
        volatile uint8_t *latched_write = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;
        latched_write[byte_index] = bit_mask;
    }

    /* Step 8: Verify latched state cleared (use USB HID scancode) */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
            test_puts("8. Latched bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" not cleared\n");
            return TEST_FAIL;
        }
    }

    return TEST_PASS;
}

/* Test functions for modifier keys */
static int test_key_lctrl(void)
{
    return key_input(PS2_SCANCODE_LCTRL, USB_HID_SCANCODE_LEFTCTRL, "Left Ctrl");
}

static int test_key_lshift(void)
{
    return key_input(PS2_SCANCODE_LSHIFT, USB_HID_SCANCODE_LEFTSHIFT, "Left Shift");
}

static int test_key_lalt(void)
{
    return key_input(PS2_SCANCODE_LALT, USB_HID_SCANCODE_LEFTALT, "Left Alt");
}

static int test_key_lgui(void)
{
    return key_input(PS2_SCANCODE_LGUI, USB_HID_SCANCODE_LEFTMETA, "Left GUI");
}

static int test_key_rctrl(void)
{
    return key_input(PS2_SCANCODE_RCTRL, USB_HID_SCANCODE_RIGHTCTRL, "Right Ctrl");
}

static int test_key_rshift(void)
{
    return key_input(PS2_SCANCODE_RSHIFT, USB_HID_SCANCODE_RIGHTSHIFT, "Right Shift");
}

static int test_key_ralt(void)
{
    return key_input(PS2_SCANCODE_RALT, USB_HID_SCANCODE_RIGHTALT, "Right Alt");
}

static int test_key_rgui(void)
{
    return key_input(PS2_SCANCODE_RGUI, USB_HID_SCANCODE_RIGHTMETA, "Right GUI");
}

static int test_key_capslock(void)
{
    return key_input(PS2_SCANCODE_CAPSLOCK, USB_HID_SCANCODE_CAPSLOCK, "Caps Lock");
}

static int test_key_numlock(void)
{
    return key_input(PS2_SCANCODE_NUMLOCK, USB_HID_SCANCODE_NUMLOCK, "Num Lock");
}

static int test_key_scrolllock(void)
{
    return key_input(PS2_SCANCODE_SCROLLLOCK, USB_HID_SCANCODE_SCROLLLOCK, "Scroll Lock");
}

/* Test functions for control keys */
static int test_key_escape(void)
{
    return key_input(PS2_SCANCODE_ESCAPE, USB_HID_SCANCODE_ESCAPE, "Escape");
}

static int test_key_enter(void)
{
    return key_input(PS2_SCANCODE_ENTER, USB_HID_SCANCODE_ENTER, "Enter");
}

static int test_key_space(void)
{
    return key_input(PS2_SCANCODE_SPACE, USB_HID_SCANCODE_SPACE, "Space");
}

static int test_key_backspace(void)
{
    return key_input(PS2_SCANCODE_BACKSPACE, USB_HID_SCANCODE_BACKSPACE, "Backspace");
}

static int test_key_tab(void)
{
    return key_input(PS2_SCANCODE_TAB, USB_HID_SCANCODE_TAB, "Tab");
}

static int test_key_insert(void)
{
    return key_input(PS2_SCANCODE_INSERT, USB_HID_SCANCODE_INSERT, "Insert");
}

static int test_key_delete(void)
{
    return key_input(PS2_SCANCODE_DELETE, USB_HID_SCANCODE_DELETE, "Delete");
}

static int test_key_home(void)
{
    return key_input(PS2_SCANCODE_HOME, USB_HID_SCANCODE_HOME, "Home");
}

static int test_key_end(void)
{
    return key_input(PS2_SCANCODE_END, USB_HID_SCANCODE_END, "End");
}

static int test_key_pageup(void)
{
    return key_input(PS2_SCANCODE_PAGEUP, USB_HID_SCANCODE_PAGEUP, "Page Up");
}

static int test_key_pagedown(void)
{
    return key_input(PS2_SCANCODE_PAGEDOWN, USB_HID_SCANCODE_PAGEDOWN, "Page Down");
}

#if 0
/* Pause scancode: E1 14 77 E1 F0 14 F0 77 (8 bytes) */
/* Test function for pause key (special handling for multi-byte scancode) */
static int test_key_pause(void)
{
    volatile uint8_t *scancode_reg = (volatile uint8_t *)FUNCVAL_KB_SCANCODE;
    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX;
    volatile uint8_t *matrix_latched = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;
    volatile uint8_t *latched_write = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;
    uint8_t pause_scancode[8] = {0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77};
    uint16_t usb_scancode = USB_HID_SCANCODE_PAUSE;

    if (platform_is_interactive) {
        test_puts("Press and hold 'Pause' key\n");
        screen_flip();
    } else {
        /* Simulation: write scancode bytes to testbench register */
        for (int i = 0; i < 8; i++) {
            *scancode_reg = pause_scancode[i];
            if (i == 0 || i == 3) {
                usleep(50);  /* Short delay after E1 prefix */
            } else if (i == 4) {
                usleep(50);  /* Short delay after F0 prefix */
            } else {
                usleep(4000);  /* Normal delay */
            }
        }
    }

    if (platform_is_interactive) {
        /* Hardware: poll with timeout ~10 seconds */
        uint64_t start_time = timer_get_us();
        int detected = 0;
        uint16_t matrix_bit = usb_scancode & 0xFF;

        while (timer_get_us() - start_time < 10000000) {
            if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
                detected = 1;
                break;
            }
            usleep(10000); /* Poll every 10ms */
            test_puts(".");
        }

        if (!detected)
            return TEST_TIMEOUT;

        test_puts("\n");
    }

    /* Check current matrix has bit set */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (!is_matrix_bit_set(matrix, matrix_bit)) {
            test_puts("Matrix bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" not set\n");
            return TEST_FAIL;
        }
    }

    /* Check latched matrix has bit set */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (!is_matrix_bit_set(matrix_latched, matrix_bit)) {
            test_puts("Latched bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" not set\n");
            return TEST_FAIL;
        }
    }

    if (platform_is_interactive) {
        test_puts("Release 'Pause' key\n");
        screen_flip();
    } else {
        /* Send break code: F0 E1 14 77 */
        uint8_t break_scancode[4] = {0xF0, 0xE1, 0x14, 0x77};
        for (int i = 0; i < 4; i++) {
            *scancode_reg = break_scancode[i];
            if (i == 0) {
                usleep(50);
            } else {
                usleep(6000);
            }
        }
    }

    /* Check current matrix has bit cleared */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (is_matrix_bit_set(matrix, matrix_bit)) {
            test_puts("Matrix bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" not cleared\n");
            return TEST_FAIL;
        }
    }

    /* Check latched matrix still has bit set */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (!is_matrix_bit_set(matrix_latched, matrix_bit)) {
            test_puts("Latched bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" cleared prematurely\n");
            return TEST_FAIL;
        }
    }

    /* Clear latched state */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        latched_write[matrix_bit >> 3] = 1 << (matrix_bit & 0x7);
    }

    /* Verify latched state cleared */
    {
        uint16_t matrix_bit = usb_scancode & 0xFF;
        if (is_matrix_bit_set(matrix_latched, matrix_bit)) {
            test_puts("Latched bit 0x");
            test_print_hex_word(matrix_bit);
            test_puts(" not cleared\n");
            return TEST_FAIL;
        }
    }

    return TEST_PASS;
}
#endif

/* Test functions for arrow keys */
static int test_key_left(void)
{
    return key_input(PS2_SCANCODE_LEFT, USB_HID_SCANCODE_LEFT, "Left Arrow");
}

static int test_key_right(void)
{
    return key_input(PS2_SCANCODE_RIGHT, USB_HID_SCANCODE_RIGHT, "Right Arrow");
}

static int test_key_up(void)
{
    return key_input(PS2_SCANCODE_UP, USB_HID_SCANCODE_UP, "Up Arrow");
}

static int test_key_down(void)
{
    return key_input(PS2_SCANCODE_DOWN, USB_HID_SCANCODE_DOWN, "Down Arrow");
}

/* Test functions for function keys */
static int test_key_f1(void)
{
    return key_input(PS2_SCANCODE_F1, USB_HID_SCANCODE_F1, "F1");
}

static int test_key_f2(void)
{
    return key_input(PS2_SCANCODE_F2, USB_HID_SCANCODE_F2, "F2");
}

static int test_key_f3(void)
{
    return key_input(PS2_SCANCODE_F3, USB_HID_SCANCODE_F3, "F3");
}

static int test_key_f4(void)
{
    return key_input(PS2_SCANCODE_F4, USB_HID_SCANCODE_F4, "F4");
}

static int test_key_f5(void)
{
    return key_input(PS2_SCANCODE_F5, USB_HID_SCANCODE_F5, "F5");
}

static int test_key_f6(void)
{
    return key_input(PS2_SCANCODE_F6, USB_HID_SCANCODE_F6, "F6");
}

static int test_key_f7(void)
{
    return key_input(PS2_SCANCODE_F7, USB_HID_SCANCODE_F7, "F7");
}

static int test_key_f8(void)
{
    return key_input(PS2_SCANCODE_F8, USB_HID_SCANCODE_F8, "F8");
}

static int test_key_f9(void)
{
    return key_input(PS2_SCANCODE_F9, USB_HID_SCANCODE_F9, "F9");
}

static int test_key_f10(void)
{
    return key_input(PS2_SCANCODE_F10, USB_HID_SCANCODE_F10, "F10");
}

static int test_key_f11(void)
{
    return key_input(PS2_SCANCODE_F11, USB_HID_SCANCODE_F11, "F11");
}

static int test_key_f12(void)
{
    return key_input(PS2_SCANCODE_F12, USB_HID_SCANCODE_F12, "F12");
}

/* Test functions for numpad keys */
static int test_key_numpad_0(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_0, USB_HID_SCANCODE_KP_0, "Numpad 0");
}

static int test_key_numpad_1(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_1, USB_HID_SCANCODE_KP_1, "Numpad 1");
}

static int test_key_numpad_2(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_2, USB_HID_SCANCODE_KP_2, "Numpad 2");
}

static int test_key_numpad_3(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_3, USB_HID_SCANCODE_KP_3, "Numpad 3");
}

static int test_key_numpad_4(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_4, USB_HID_SCANCODE_KP_4, "Numpad 4");
}

static int test_key_numpad_5(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_5, USB_HID_SCANCODE_KP_5, "Numpad 5");
}

static int test_key_numpad_6(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_6, USB_HID_SCANCODE_KP_6, "Numpad 6");
}

static int test_key_numpad_7(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_7, USB_HID_SCANCODE_KP_7, "Numpad 7");
}

static int test_key_numpad_8(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_8, USB_HID_SCANCODE_KP_8, "Numpad 8");
}

static int test_key_numpad_9(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_9, USB_HID_SCANCODE_KP_9, "Numpad 9");
}

static int test_key_numpad_dot(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_DOT, USB_HID_SCANCODE_KP_DOT, "Numpad .");
}

static int test_key_numpad_enter(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_ENTER, USB_HID_SCANCODE_KP_ENTER, "Numpad Enter");
}

static int test_key_numpad_plus(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_PLUS, USB_HID_SCANCODE_KP_PLUS, "Numpad +");
}

static int test_key_numpad_minus(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_MINUS, USB_HID_SCANCODE_KP_MINUS, "Numpad -");
}

static int test_key_numpad_mul(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_MUL, USB_HID_SCANCODE_KP_ASTERISK, "Numpad *");
}

static int test_key_numpad_div(void)
{
    return key_input(PS2_SCANCODE_NUMPAD_DIV, USB_HID_SCANCODE_KP_SLASH, "Numpad /");
}

/* Test functions for main row keys */
static int test_key_1(void)
{
    return key_input(PS2_SCANCODE_1, USB_HID_SCANCODE_1, "1");
}

static int test_key_2(void)
{
    return key_input(PS2_SCANCODE_2, USB_HID_SCANCODE_2, "2");
}

static int test_key_3(void)
{
    return key_input(PS2_SCANCODE_3, USB_HID_SCANCODE_3, "3");
}

static int test_key_4(void)
{
    return key_input(PS2_SCANCODE_4, USB_HID_SCANCODE_4, "4");
}

static int test_key_5(void)
{
    return key_input(PS2_SCANCODE_5, USB_HID_SCANCODE_5, "5");
}

static int test_key_6(void)
{
    return key_input(PS2_SCANCODE_6, USB_HID_SCANCODE_6, "6");
}

static int test_key_7(void)
{
    return key_input(PS2_SCANCODE_7, USB_HID_SCANCODE_7, "7");
}

static int test_key_8(void)
{
    return key_input(PS2_SCANCODE_8, USB_HID_SCANCODE_8, "8");
}

static int test_key_9(void)
{
    return key_input(PS2_SCANCODE_9, USB_HID_SCANCODE_9, "9");
}

static int test_key_0(void)
{
    return key_input(PS2_SCANCODE_0, USB_HID_SCANCODE_0, "0");
}

static int test_key_minus(void)
{
    return key_input(PS2_SCANCODE_MINUS, USB_HID_SCANCODE_MINUS, "Minus (-)");
}

static int test_key_equals(void)
{
    return key_input(PS2_SCANCODE_EQUALS, USB_HID_SCANCODE_EQUAL, "Equals (=)");
}

static int test_key_backslash(void)
{
    return key_input(PS2_SCANCODE_BACKSLASH, USB_HID_SCANCODE_BACKSLASH, "Backslash (\\)");
}

static int test_key_grave(void)
{
    return key_input(PS2_SCANCODE_GRAVE, USB_HID_SCANCODE_GRAVE, "Grave (`)");
}

/* Test functions for QWERTY row */
static int test_key_q(void)
{
    return key_input(PS2_SCANCODE_Q, USB_HID_SCANCODE_Q, "Q");
}

static int test_key_w(void)
{
    return key_input(PS2_SCANCODE_W, USB_HID_SCANCODE_W, "W");
}

static int test_key_e(void)
{
    return key_input(PS2_SCANCODE_E, USB_HID_SCANCODE_E, "E");
}

static int test_key_r(void)
{
    return key_input(PS2_SCANCODE_R, USB_HID_SCANCODE_R, "R");
}

static int test_key_t(void)
{
    return key_input(PS2_SCANCODE_T, USB_HID_SCANCODE_T, "T");
}

static int test_key_y(void)
{
    return key_input(PS2_SCANCODE_Y, USB_HID_SCANCODE_Y, "Y");
}

static int test_key_u(void)
{
    return key_input(PS2_SCANCODE_U, USB_HID_SCANCODE_U, "U");
}

static int test_key_i(void)
{
    return key_input(PS2_SCANCODE_I, USB_HID_SCANCODE_I, "I");
}

static int test_key_o(void)
{
    return key_input(PS2_SCANCODE_O, USB_HID_SCANCODE_O, "O");
}

static int test_key_p(void)
{
    return key_input(PS2_SCANCODE_P, USB_HID_SCANCODE_P, "P");
}

static int test_key_lbracket(void)
{
    return key_input(PS2_SCANCODE_LBRACKET, USB_HID_SCANCODE_LEFTBRACE, "Left Bracket ([)");
}

static int test_key_rbracket(void)
{
    return key_input(PS2_SCANCODE_RBRACKET, USB_HID_SCANCODE_RIGHTBRACE, "Right Bracket (])");
}

/* Test functions for ASDF row */
static int test_key_a(void)
{
    return key_input(PS2_SCANCODE_A, USB_HID_SCANCODE_A, "A");
}

static int test_key_s(void)
{
    return key_input(PS2_SCANCODE_S, USB_HID_SCANCODE_S, "S");
}

static int test_key_d(void)
{
    return key_input(PS2_SCANCODE_D, USB_HID_SCANCODE_D, "D");
}

static int test_key_f(void)
{
    return key_input(PS2_SCANCODE_F, USB_HID_SCANCODE_F, "F");
}

static int test_key_g(void)
{
    return key_input(PS2_SCANCODE_G, USB_HID_SCANCODE_G, "G");
}

static int test_key_h(void)
{
    return key_input(PS2_SCANCODE_H, USB_HID_SCANCODE_H, "H");
}

static int test_key_j(void)
{
    return key_input(PS2_SCANCODE_J, USB_HID_SCANCODE_J, "J");
}

static int test_key_k(void)
{
    return key_input(PS2_SCANCODE_K, USB_HID_SCANCODE_K, "K");
}

static int test_key_l(void)
{
    return key_input(PS2_SCANCODE_L, USB_HID_SCANCODE_L, "L");
}

static int test_key_semicolon(void)
{
    return key_input(PS2_SCANCODE_SEMICOLON, USB_HID_SCANCODE_SEMICOLON, "Semicolon (;)");
}

static int test_key_apostrophe(void)
{
    return key_input(PS2_SCANCODE_APOSTROPHE, USB_HID_SCANCODE_APOSTROPHE, "Apostrophe (')");
}

/* Test functions for ZXC row */
static int test_key_z(void)
{
    return key_input(PS2_SCANCODE_Z, USB_HID_SCANCODE_Z, "Z");
}

static int test_key_x(void)
{
    return key_input(PS2_SCANCODE_X, USB_HID_SCANCODE_X, "X");
}

static int test_key_c(void)
{
    return key_input(PS2_SCANCODE_C, USB_HID_SCANCODE_C, "C");
}

static int test_key_v(void)
{
    return key_input(PS2_SCANCODE_V, USB_HID_SCANCODE_V, "V");
}

static int test_key_b(void)
{
    return key_input(PS2_SCANCODE_B, USB_HID_SCANCODE_B, "B");
}

static int test_key_n(void)
{
    return key_input(PS2_SCANCODE_N, USB_HID_SCANCODE_N, "N");
}

static int test_key_m(void)
{
    return key_input(PS2_SCANCODE_M, USB_HID_SCANCODE_M, "M");
}

static int test_key_comma(void)
{
    return key_input(PS2_SCANCODE_COMMA, USB_HID_SCANCODE_COMMA, "Comma (,)");
}

static int test_key_period(void)
{
    return key_input(PS2_SCANCODE_PERIOD, USB_HID_SCANCODE_DOT, "Period (.)");
}

static int test_key_slash(void)
{
    return key_input(PS2_SCANCODE_SLASH, USB_HID_SCANCODE_SLASH, "Slash (/)");
}

/* Test: Multi-key latching - verify multiple keys can be pressed and latched */
static int test_multi_key_latching(void)
{
    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX;
    volatile uint8_t *matrix_latched = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;
    volatile uint8_t *latched_write = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;

    /* Clear any previous latched state for test keys */
    uint16_t byte_a = USB_HID_SCANCODE_A >> 3;
    uint8_t mask_a = 1 << (USB_HID_SCANCODE_A & 0x7);
    latched_write[byte_a] = mask_a;

    uint16_t byte_s = USB_HID_SCANCODE_S >> 3;
    uint8_t mask_s = 1 << (USB_HID_SCANCODE_S & 0x7);
    latched_write[byte_s] = mask_s;

    if (platform_is_interactive) {
        test_puts("This test will press both A and S keys\n");
        screen_flip();
    }

    /* Press A, wait for latch, release A, wait for release */
    send_key_press(PS2_SCANCODE_A, "A");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (!is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_A) &&
               timer_get_us() - t < 10000000)
            usleep(10000);
        if (!is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_A))
            return TEST_TIMEOUT;
    }
    send_key_release(PS2_SCANCODE_A, "A");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (is_matrix_bit_set(matrix, USB_HID_SCANCODE_A) &&
               timer_get_us() - t < 5000000)
            usleep(10000);
    }

    /* Press S, wait for latch, release S, wait for release */
    send_key_press(PS2_SCANCODE_S, "S");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (!is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_S) &&
               timer_get_us() - t < 10000000)
            usleep(10000);
        if (!is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_S))
            return TEST_TIMEOUT;
    }
    send_key_release(PS2_SCANCODE_S, "S");
    if (platform_is_interactive) {
        uint64_t t = timer_get_us();
        while (is_matrix_bit_set(matrix, USB_HID_SCANCODE_S) &&
               timer_get_us() - t < 5000000)
            usleep(10000);
    }

    /* Verify both are latched */
    if (!is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_A) ||
        !is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_S)) {
        test_puts("Both keys not latched: A=");
        test_print_hex_word(is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_A));
        test_puts(" S=");
        test_print_hex_word(is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_S));
        test_puts("\n");
        return TEST_FAIL;
    }

    /* Clear only A key */
    latched_write[byte_a] = mask_a;
    usleep(100);

    if (is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_A)) {
        test_puts("A key not cleared\n");
        return TEST_FAIL;
    }

    if (!is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_S)) {
        test_puts("S key cleared unexpectedly\n");
        return TEST_FAIL;
    }

    /* Clear S key */
    latched_write[byte_s] = mask_s;
    usleep(100);

    if (is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_A) ||
        is_matrix_bit_set(matrix_latched, USB_HID_SCANCODE_S)) {
        test_puts("Keys not both cleared\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test suite setup */
static void suite_setup(void)
{
    if (platform_is_simulation)
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_KB) = 1;
}

/* Test suite cleanup */
static void suite_cleanup(void)
{
    if (platform_is_simulation)
        MMIO_REG16(FUNCVAL_MODEL_DEBUG_KB) = 0;
}

/* Define test suite - all keys from US layout keyboard */
TEST_SUITE_SETUP_CLEANUP(05b_external_keyboard, suite_setup, suite_cleanup,
    key_lctrl,
    key_lshift,
    key_lalt,
    key_lgui,
    key_rctrl,
    key_rshift,
    key_ralt,
    key_rgui,
    key_capslock,
    key_numlock,
    key_scrolllock,
    key_escape,
    key_enter,
    key_space,
    key_backspace,
    key_tab,
    key_insert,
    key_delete,
    key_home,
    key_end,
    key_pageup,
    key_pagedown,
    //key_pause,
    key_left,
    key_right,
    key_up,
    key_down,
    key_f1,
    key_f2,
    key_f3,
    key_f4,
    key_f5,
    key_f6,
    key_f7,
    key_f8,
    key_f9,
    key_f10,
    key_f11,
    key_f12,
    key_numpad_0,
    key_numpad_1,
    key_numpad_2,
    key_numpad_3,
    key_numpad_4,
    key_numpad_5,
    key_numpad_6,
    key_numpad_7,
    key_numpad_8,
    key_numpad_9,
    key_numpad_dot,
    key_numpad_enter,
    key_numpad_plus,
    key_numpad_minus,
    key_numpad_mul,
    key_numpad_div,
    key_1,
    key_2,
    key_3,
    key_4,
    key_5,
    key_6,
    key_7,
    key_8,
    key_9,
    key_0,
    key_minus,
    key_equals,
    key_backslash,
    key_grave,
    key_q,
    key_w,
    key_e,
    key_r,
    key_t,
    key_y,
    key_u,
    key_i,
    key_o,
    key_p,
    key_lbracket,
    key_rbracket,
    key_a,
    key_s,
    key_d,
    key_f,
    key_g,
    key_h,
    key_j,
    key_k,
    key_l,
    key_semicolon,
    key_apostrophe,
    key_z,
    key_x,
    key_c,
    key_v,
    key_b,
    key_n,
    key_m,
    key_comma,
    key_period,
    key_slash,
    multi_key_latching
);
