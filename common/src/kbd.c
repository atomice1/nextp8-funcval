/* Keyboard input functions for funcval library */

#include <stdbool.h>
#include <stdint.h>
#include "mmio.h"
#include "nextp8.h"

/* Clear latched key state for all keys */
void clear_latched_keys(void)
{
    for (int i=0;i<32;++i)
        MMIO_REG8(_KEYBOARD_MATRIX_LATCHED + i) = 0xff;
    MMIO_REG8(_JOYSTICK0_LATCHED) = 0xff;
    MMIO_REG8(_JOYSTICK1_LATCHED) = 0xff;
}

void wait_for_any_key(void)
{
    clear_latched_keys();
    bool key_pressed = false;
    do {
        for (int i=0;i<32;++i) {
            if (MMIO_REG8(_KEYBOARD_MATRIX_LATCHED + i) != 0)
                key_pressed = true;
        }
        if (MMIO_REG8(_JOYSTICK0_LATCHED) != 0)
            key_pressed = true;

        if (MMIO_REG8(_JOYSTICK1_LATCHED) != 0)
            key_pressed = true;
    } while (!key_pressed);
    clear_latched_keys();
}

/* Return if the key with the given scancode is currently pressed or not.
 */
static bool is_down(volatile uint8_t *base, unsigned index)
{
    return base[index >> 3] & (1 << (index & 0x7));
}

/* Clear latched key state for a specific key index */
static void clear_key(unsigned index)
{
    MMIO_REG8(_KEYBOARD_MATRIX_LATCHED + (index >> 3)) = (1 << (index & 0x7));
}

/* Wait for and read a keyboard key press
 */
uint8_t read_keyboard_scancode(void)
{
    uint8_t scancode = 0;
    volatile uint8_t *matrix = (volatile uint8_t *)_KEYBOARD_MATRIX_LATCHED;

    do {
        for (unsigned i=0;i<256;++i) {
            bool down = is_down(matrix, i);
            if (down) {
                scancode = i;
                clear_key(i);
                break;
            }
        }
    } while (scancode == 0);

    return scancode;
}

/* Key mapping table
   nextp8 scan code (based on PS/2 code set 2) to ASCII
   Format: Each index represents a PS/2 scancode, value is ASCII character or 0 if no mapping */
static char keytable[256] = {
    0,      // 0x00
    0,      // 0x01 F9
    0,      // 0x02
    0,      // 0x03 F5
    0,      // 0x04 F3
    0,      // 0x05 F1
    0,      // 0x06 F2
    0,      // 0x07 F12
    0,      // 0x08
    0,      // 0x09 F10
    0,      // 0x0a F8
    0,      // 0x0b F6
    0,      // 0x0c F4
    '\t',   // 0x0d TAB
    '`',    // 0x0e ` ~
    0,      // 0x0f
    0,      // 0x10
    0,      // 0x11 LALT
    0,      // 0x12 LSHIFT
    0,      // 0x13 (reserved)
    0,      // 0x14 LCTRL
    'q',    // 0x15 Q
    '1',    // 0x16 1
    0,      // 0x17
    0,      // 0x18
    0,      // 0x19
    'z',    // 0x1a Z
    's',    // 0x1b S
    'a',    // 0x1c A
    'w',    // 0x1d W
    '2',    // 0x1e 2
    0,      // 0x1f
    0,      // 0x20
    'c',    // 0x21 C
    'x',    // 0x22 X
    'd',    // 0x23 D
    'e',    // 0x24 E
    '4',    // 0x25 4
    '3',    // 0x26 3
    0,      // 0x27
    0,      // 0x28
    ' ',    // 0x29 SPACE
    'v',    // 0x2a V
    'f',    // 0x2b F
    't',    // 0x2c T
    'r',    // 0x2d R
    '5',    // 0x2e 5
    0,      // 0x2f
    0,      // 0x30
    'n',    // 0x31 N
    'b',    // 0x32 B
    'h',    // 0x33 H
    'g',    // 0x34 G
    'y',    // 0x35 Y
    '6',    // 0x36 6
    0,      // 0x37
    0,      // 0x38
    0,      // 0x39
    'j',    // 0x3a J
    0,      // 0x3b
    'k',    // 0x3c K
    'l',    // 0x3d L
    ';',    // 0x3e
    '\'',   // 0x3f
    0,      // 0x40
    ',',    // 0x41 , <
    0,      // 0x42
    'i',    // 0x43 I
    'o',    // 0x44 O
    '0',    // 0x45 0
    'p',    // 0x46 P
    0,      // 0x47
    0,      // 0x48
    '.',    // 0x49 . >
    '/',    // 0x4a / ?
    0,      // 0x4b
    0,      // 0x4c
    0,      // 0x4d
    '-',    // 0x4e - _
    0,      // 0x4f
    0,      // 0x50
    '\'',   // 0x51
    0,      // 0x52
    0,      // 0x53
    '[',    // 0x54 [ {
    '=',    // 0x55 = +
    0,      // 0x56
    0,      // 0x57
    0,      // 0x58 CAPSLOCK
    0,      // 0x59 RSHIFT
    '\n',   // 0x5a RETURN
    ']',    // 0x5b ] }
    0,      // 0x5c
    '\\',   // 0x5d \ |
    0,      // 0x5e
    0,      // 0x5f
    0,      // 0x60
    0,      // 0x61
    0,      // 0x62
    0,      // 0x63
    0,      // 0x64
    0,      // 0x65
    '\b',   // 0x66 BACKSPACE
    0,      // 0x67
    0,      // 0x68
    0,      // 0x69 END (E0 69)
    0,      // 0x6a
    0,      // 0x6b LEFT (E0 6B)
    0,      // 0x6c HOME (E0 6C)
    0,      // 0x6d
    0,      // 0x6e
    0,      // 0x6f
    0,      // 0x70 KP_0
    0,      // 0x71 KP_PERIOD
    0,      // 0x72 DOWN (E0 72)
    0,      // 0x73 KP_5
    0,      // 0x74 RIGHT (E0 74)
    0,      // 0x75 UP (E0 75)
    0,      // 0x76 ESCAPE
    0,      // 0x77 NUMLOCK
    0,      // 0x78 F11
    0,      // 0x79 KP_PLUS
    0,      // 0x7a KP_3 / PAGEDOWN (E0 7A)
    0,      // 0x7b KP_MINUS
    0,      // 0x7c KP_MULTIPLY
    0,      // 0x7d KP_9 / PAGEUP (E0 7D)
    0,      // 0x7e SCROLLLOCK
    0,      // 0x7f
    0,      // 0x80
    0,      // 0x81
    0,      // 0x82
    0,      // 0x83 F7
};

/* Wait for and read a keyboard character */
char read_keyboard_char(void)
{
    uint8_t scancode = read_keyboard_scancode();
    return keytable[scancode];
}
